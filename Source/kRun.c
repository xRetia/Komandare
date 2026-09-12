/*
 * kRun.c - Komandare environment launcher (Run dialog + transparent passthrough)
 *
 * 带参数：在本 exe 目录中找到 kInitrd.cmd，把命令行原样透传给它，并透传
 *         当前工作目录作为启动位置（CreateProcessW 的 lpCurrentDirectory
 *         置 NULL，子进程自动继承调用进程的当前目录）。
 * 无参数：弹出类 Windows「运行」对话框，输入命令后以相同链路执行。
 *
 * 控制台窗口策略：默认隐藏（CREATE_NO_WINDOW），除非 -- 目标是
 *   cmd / powershell / pwsh / bash / sh，或 .cmd / .bat / .sh / .ps1 脚本，
 *   或能解析为 console 子系统的 PE 可执行文件。命令行首参为 --console 时
 *   无论目标如何都显示控制台窗口。
 * 界面语言跟随系统 UI 语言（中文或英文）。
 *
 * 编译（零依赖原生 PE，无 CRT，GUI 子系统避免自身闪现控制台窗口）：
 *   windres kRun.rc -O coff -o kRun.res
 *   x86_64-pc-cygwin-gcc -nostdlib -mwindows -Wl,-e,main -Wl,--subsystem,windows \
 *       -o kRun.exe kRun.c kRun.res -lkernel32 -luser32 -lshell32
 */

#include <windows.h>

#define IDD_KRUN     101
#define IDI_KRUN     200
#define IDC_LABEL    1001
#define IDC_RUNEDIT  1002

static int g_zh;            /* 1 = 中文界面，0 = English UI (detected from OS UI language) */
static int g_force_console; /* 1 = 命令行带 --console，强制显示控制台窗口 */

/* ============ wide string helpers ============ */

void __main(void) {}

void* memset(void* dst, int c, size_t n)
{
    unsigned char* d = (unsigned char*)dst;
    while (n--) *d++ = (unsigned char)c;
    return dst;
}

static size_t wlen(const wchar_t* s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void wcat(wchar_t* dst, size_t cap, const wchar_t* src)
{
    size_t n = wlen(dst);
    while (n + 1 < cap && *src) dst[n++] = *src++;
    dst[n] = 0;
}

static void wcpy(wchar_t* dst, size_t cap, const wchar_t* src)
{
    size_t n = 0;
    while (n + 1 < cap && *src) dst[n++] = *src++;
    dst[n] = 0;
}

static wchar_t* wchr(const wchar_t* s, wchar_t c)
{
    while (*s) { if (*s == c) return (wchar_t*)s; s++; }
    return NULL;
}

static wchar_t* wrchr(wchar_t* s, wchar_t c)
{
    wchar_t* last = NULL;
    while (*s) { if (*s == c) last = s; s++; }
    return last;
}

static BOOL wprefix(const wchar_t* s, const wchar_t* p)
{
    while (*p) {
        if (*s != *p) return FALSE;
        s++; p++;
    }
    return TRUE;
}

static BOOL wprefix_i(const wchar_t* s, const wchar_t* p)
{
    while (*p) {
        wchar_t a = *s++, b = *p++;
        if (a >= L'A' && a <= L'Z') a += L'a' - L'A';
        if (b >= L'A' && b <= L'Z') b += L'a' - L'A';
        if (a != b) return FALSE;
    }
    return TRUE;
}

static BOOL wendswith_i(const wchar_t* s, const wchar_t* suf)
{
    size_t ls = wlen(s), lx = wlen(suf);
    if (lx > ls) return FALSE;
    return wprefix_i(s + (ls - lx), suf);
}

static BOOL wieq(const wchar_t* a, const wchar_t* b)
{
    return wlen(a) == wlen(b) && wprefix_i(a, b);
}

/* trim leading/trailing spaces, tabs, CR, LF in place */
static void trim_w(wchar_t* s)
{
    wchar_t* p = s;
    while (*p == L' ' || *p == L'\t' || *p == L'\r' || *p == L'\n') p++;
    if (p != s) {
        wchar_t* q = s;
        while (*p) *q++ = *p++;
        *q = 0;
    }
    size_t n = wlen(s);
    while (n > 0 && (s[n-1] == L' ' || s[n-1] == L'\t' || s[n-1] == L'\r' || s[n-1] == L'\n'))
        s[--n] = 0;
}

/* ============ PATH bootstrap ============ */

static void prepend_exe_dir_to_path(const wchar_t* exeDir)
{
    static wchar_t oldPath[32768];
    DWORD n = GetEnvironmentVariableW(L"PATH", oldPath, 32768);
    if (n == 0 || n >= 32768) {
        SetEnvironmentVariableW(L"PATH", exeDir);
        return;
    }
    if (wlen(oldPath) >= wlen(exeDir) && wprefix(oldPath, exeDir))
        return;
    static wchar_t newPath[32768];
    int written = wsprintfW(newPath, L"%s;%s", exeDir, oldPath);
    if (written > 0 && written < 32768)
        SetEnvironmentVariableW(L"PATH", newPath);
}

/* ============ transparent launcher ============ */

static wchar_t g_exeDir[MAX_PATH];

/* ============ console window policy ============ */

/* TRUE if <path> is a WIN32 GUI-subsystem PE (subsystem == 2) */
static BOOL pe_is_gui(const wchar_t* path)
{
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    static unsigned char buf[4096];
    DWORD rd = 0;
    BOOL ok = ReadFile(h, buf, sizeof(buf), &rd, NULL);
    CloseHandle(h);
    if (!ok || rd < 0x40) return FALSE;
    if (buf[0] != 'M' || buf[1] != 'Z') return FALSE;
    DWORD peOff = (DWORD)buf[0x3C] | ((DWORD)buf[0x3D] << 8) |
                  ((DWORD)buf[0x3E] << 16) | ((DWORD)buf[0x3F] << 24);
    if ((DWORD)peOff + 0x5A + 2 > rd) return FALSE;
    if (buf[peOff] != 'P' || buf[peOff+1] != 'E') return FALSE;
    if (buf[peOff+2] != 0 || buf[peOff+3] != 0) return FALSE;
    unsigned optMagic = (unsigned)buf[peOff+24] | ((unsigned)buf[peOff+25] << 8);
    if (optMagic != 0x10B && optMagic != 0x20B) return FALSE;
    unsigned subsys = (unsigned)buf[peOff+24+68] | ((unsigned)buf[peOff+24+69] << 8);
    return subsys == 2;
}

/* extract first command token (quote-aware) into tok; returns its length */
static size_t first_token(const wchar_t* t, wchar_t* tok, size_t cap)
{
    while (*t == L' ' || *t == L'\t') t++;
    size_t n = 0;
    if (*t == L'"') {
        t++;
        while (*t && *t != L'"' && n + 1 < cap) tok[n++] = *t++;
        if (*t == L'"') t++;
    } else {
        while (*t && *t != L' ' && *t != L'\t' && n + 1 < cap) tok[n++] = *t++;
    }
    tok[n] = 0;
    return n;
}

static const wchar_t* base_name(const wchar_t* t)
{
    const wchar_t* last = t;
    for (; *t; t++) {
        if (*t == L'\\' || *t == L'/') last = t + 1;
    }
    return last;
}

/* 1 = console subsystem, 0 = GUI subsystem, -1 = not found */
static int resolve_subsystem(const wchar_t* dir, const wchar_t* tok)
{
    static wchar_t path[MAX_PATH];
    wcpy(path, MAX_PATH, dir);
    if (dir[0]) wcat(path, MAX_PATH, L"\\");
    wcat(path, MAX_PATH, tok);
    if (!wendswith_i(path, L".exe") &&
        GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
        wcat(path, MAX_PATH, L".exe");
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return -1;
    return pe_is_gui(path) ? 0 : 1;
}

/* walk the PATH environment for tok; returns resolve_subsystem verdict */
static int resolve_in_path(const wchar_t* tok)
{
    static wchar_t pathenv[32768];
    DWORD n = GetEnvironmentVariableW(L"PATH", pathenv, 32768);
    if (n == 0 || n >= 32768) return -1;
    wchar_t* seg = pathenv;
    for (;;) {
        wchar_t* sep = wchr(seg, L';');
        if (sep) *sep = 0;
        int st = seg[0] ? resolve_subsystem(seg, tok) : -1;
        if (sep) *sep = L';';
        if (st >= 0) return st;
        if (!sep) return -1;
        seg = sep + 1;
    }
}

/* TRUE = show a console window for this target */
static BOOL is_console_target(const wchar_t* target)
{
    static wchar_t tok[1024];
    if (!first_token(target, tok, 1024)) return TRUE;
    const wchar_t* base = base_name(tok);

    if (wendswith_i(base, L".cmd") || wendswith_i(base, L".bat") ||
        wendswith_i(base, L".sh")  || wendswith_i(base, L".ps1"))
        return TRUE;

    if (wieq(base, L"cmd") || wieq(base, L"cmd.exe") ||
        wieq(base, L"powershell") || wieq(base, L"powershell.exe") ||
        wieq(base, L"pwsh") || wieq(base, L"pwsh.exe") ||
        wieq(base, L"bash") || wieq(base, L"bash.exe") ||
        wieq(base, L"sh")   || wieq(base, L"sh.exe"))
        return TRUE;

    int st = -1;
    if (wchr(tok, L'\\') || wchr(tok, L'/')) {
        st = resolve_subsystem(L"", tok);
    } else {
        static wchar_t binRoot[MAX_PATH], unixBin[MAX_PATH];
        wcpy(binRoot, MAX_PATH, g_exeDir); wcat(binRoot, MAX_PATH, L"\\Binary");
        wcpy(unixBin, MAX_PATH, g_exeDir); wcat(unixBin, MAX_PATH, L"\\Unixlike\\bin");
        st = resolve_subsystem(L"", tok);
        if (st < 0) st = resolve_subsystem(g_exeDir, tok);
        if (st < 0) st = resolve_subsystem(binRoot, tok);
        if (st < 0) st = resolve_subsystem(unixBin, tok);
        if (st < 0) st = resolve_in_path(tok);
    }
    if (st >= 0) return st != 0;
    return FALSE; /* 解析不到一律隐藏 */
}

/*
 * 启动 "cmd /d /s /c "<exeDir>\kInitrd.cmd" <target>"。
 * /d 跳过 AutoRun（避免二次注入）；/s 保留末尾成对引号，保证含空格的
 * 安装目录与参数原样展开。lpCurrentDirectory 置 NULL 即透传当前工作目录。
 */
static BOOL run_client(const wchar_t* target)
{
    wchar_t kinit[MAX_PATH];
    wcpy(kinit, MAX_PATH, g_exeDir);
    wcat(kinit, MAX_PATH, L"\\kInitrd.cmd");
    if (GetFileAttributesW(kinit) == INVALID_FILE_ATTRIBUTES) {
        wchar_t msg[640];
        wsprintfW(msg, g_zh ? L"未找到 %s\n当前目录可能不是 Komandare 安装根。"
                            : L"%s not found.\nThis does not look like a Komandare installation folder.",
                  kinit);
        MessageBoxW(NULL, msg, L"Komandare", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    prepend_exe_dir_to_path(g_exeDir);

    wchar_t comspec[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"COMSPEC", comspec, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        wcpy(comspec, MAX_PATH, L"C:\\Windows\\System32\\cmd.exe");

    static wchar_t cmdline[32768];
    wsprintfW(cmdline, L"\"%s\" /d /s /c \"\"%s\" %s\"", comspec, kinit, target);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    DWORD flags = (g_force_console || is_console_target(target)) ? 0 : CREATE_NO_WINDOW;
    BOOL ok = CreateProcessW(comspec, cmdline, NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi);
    if (!ok) {
        wchar_t msg[640];
        wsprintfW(msg, g_zh ? L"启动失败（错误码 %lu）。"
                            : L"Launch failed (error %lu).", GetLastError());
        MessageBoxW(NULL, msg, L"Komandare", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return TRUE;
}

/* ============ Run dialog ============ */

static void set_dlg_texts(HWND hwnd)
{
    if (g_zh) {
        SetWindowTextW(hwnd, L"Komandare 运行");
        SetDlgItemTextW(hwnd, IDC_LABEL, L"键入要在 Komandare 环境下运行的命令：");
    } else {
        SetWindowTextW(hwnd, L"Komandare Run");
        SetDlgItemTextW(hwnd, IDC_LABEL, L"Run a program in the Komandare environment:");
    }
    SetDlgItemTextW(hwnd, IDOK, g_zh ? L"确定" : L"OK");
    SetDlgItemTextW(hwnd, IDCANCEL, g_zh ? L"取消" : L"Cancel");
}

static INT_PTR CALLBACK dlg_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        set_dlg_texts(hwnd);

        HICON hIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_KRUN));
        if (hIcon) {
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }

        RECT wa, wr;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
        GetWindowRect(hwnd, &wr);
        int wx = wa.left + 24;
        int wy = wa.bottom - (wr.bottom - wr.top) - 48;
        SetWindowPos(hwnd, HWND_TOPMOST, wx, wy, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

        SetFocus(GetDlgItem(hwnd, IDC_RUNEDIT));
        return FALSE;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            static wchar_t input[32768];
            input[0] = 0;
            GetDlgItemTextW(hwnd, IDC_RUNEDIT, input, 32768);
            trim_w(input);
            if (wprefix_i(input, L"--console") &&
                (input[10] == 0 || input[10] == L' ' || input[10] == L'\t')) {
                g_force_console = 1;
                wchar_t* p = input;
                while (*p && *p != L' ' && *p != L'\t') p++;
                while (*p == L' ' || *p == L'\t') p++;
                wchar_t* q = input;
                while (*p) *q++ = *p++;
                *q = 0;
            }
            if (input[0] == 0) {
                MessageBeep(MB_OK);
                return TRUE;
            }
            if (run_client(input))
                EndDialog(hwnd, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/* ============ entry point ============ */

int main(void)
{
    g_zh = (PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE);

    GetModuleFileNameW(NULL, g_exeDir, MAX_PATH);
    wchar_t* sl = wrchr(g_exeDir, L'\\');
    if (sl) *sl = 0;

    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc > 1 && argv[1][0] != 0) {
        if (wieq(argv[1], L"--console")) {
            g_force_console = 1;
            if (argc > 2 && argv[2][0] != 0)
                run_client(argv[2]);
        } else {
            run_client(argv[1]);
        }
    }
    if (argv) LocalFree(argv);

    if (argc > 1)
        ExitProcess(0);

    INT_PTR dlg = DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_KRUN), NULL, dlg_proc, 0);
    (void)dlg;
    ExitProcess(0);
}