/*
 * kmd-win.c - Komandare Windows Terminal launcher
 *
 * 与 kmd.exe（ConEmu 启动器）等价，但优先使用 Windows Terminal 承载。
 * 功能：
 *   1. 检测 Windows Terminal 是否安装且版本达标（默认 >= 1.23，可用
 *      Config\Terminal.ini 的 MinVersion 覆盖），不达标/缺失则回退启动
 *      kmd64.exe / kmd32.exe。
 *   2. 通过 JSON Fragment 给内置 Command Prompt profile 注入
 *      pathTranslationStyle（默认 wsl，可配置）。
 *   3. 启动目录遵循当前工作目录（可用 StartDirectory 配置覆盖）。
 *   4. 读取 Config\Terminal.ini 自定义行为。
 *
 * 编译（零依赖原生 PE，无 CRT，GUI 子系统避免启动时闪现控制台窗口）：
 *   x86_64-pc-cygwin-gcc -nostdlib -Wl,-e,main -Wl,--subsystem,windows \
 *       -o kmd.exe kmd.c -luser32 -lkernel32 -ladvapi32 -lshell32 kmd-win.res
 */

#include <windows.h>
#include <shellapi.h>

#define DEFAULT_MIN_MAJOR 1
#define DEFAULT_MIN_MINOR 23

#define CMD_PROFILE_GUID L"{0caa0dad-35be-5f56-a8ff-afceeeaa6101}"

#define CONFIG_FILENAME   L"Terminal.ini"
#define FRAGMENT_FILENAME L"kmd-win.json"

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

static int wsz_str_to_int(const wchar_t* s)
{
    int v = 0;
    while (*s >= L'0' && *s <= L'9') { v = v * 10 + (*s - L'0'); s++; }
    return v;
}

/* ============ char (ASCII) helpers ============ */

static size_t strlen_a(const char* s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static int strcmp_a(const char* a, const char* b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static int streq_a(const char* a, const char* b)
{
    return strcmp_a(a, b) == 0;
}

static char* strchr_a(char* s, char c)
{
    while (*s) { if (*s == c) return s; s++; }
    return NULL;
}

static void strcpy_a(char* dst, size_t cap, const char* src)
{
    size_t n = 0;
    while (n + 1 < cap && *src) dst[n++] = *src++;
    dst[n] = 0;
}

static int atoi_a(const char* s)
{
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

static void to_lower_a(char* s)
{
    for (; *s; s++) if (*s >= 'A' && *s <= 'Z') *s += 'a' - 'A';
}

/* trim leading/trailing spaces, tabs, CR, LF in place */
static void trim_a(char* s)
{
    char* p = s;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (p != s) {
        char* q = s;
        while (*p) *q++ = *p++;
        *q = 0;
    }
    size_t n = strlen_a(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n'))
        s[--n] = 0;
}

/* case-insensitive compare (ASCII) */
static BOOL ieq_a(const char* v, const char* ref)
{
    while (*v && *ref) {
        char a = *v++, b = *ref++;
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return FALSE;
    }
    return *v == 0 && *ref == 0;
}

/* ============ config ============ */

typedef struct {
    int     useWT;              /* 1 = prefer wt with fallback, 0 = always kmd */
    wchar_t fallback64[MAX_PATH];
    wchar_t fallback32[MAX_PATH];
    int     minMajor;
    int     minMinor;
    wchar_t startDir[MAX_PATH]; /* empty = use current dir */
    char    pathStyle[16];      /* none/wsl/cygwin/msys2/mingw */
} Config;

static void cfg_defaults(Config* c)
{
    c->useWT = 1;
    wcpy(c->fallback64, MAX_PATH, L"kmd64.exe");
    wcpy(c->fallback32, MAX_PATH, L"kmd32.exe");
    c->minMajor = DEFAULT_MIN_MAJOR;
    c->minMinor = DEFAULT_MIN_MINOR;
    c->startDir[0] = 0;
    strcpy_a(c->pathStyle, sizeof(c->pathStyle), "wsl");
}

/* ASCII -> wide (assumes ASCII input) */
static void ascii_to_w(wchar_t* dst, size_t cap, const char* src)
{
    size_t n = 0;
    while (n + 1 < cap && *src) dst[n++] = (wchar_t)(unsigned char)*src++;
    dst[n] = 0;
}

static void load_config(Config* c, const wchar_t* exeDir)
{
    cfg_defaults(c);

    wchar_t iniPath[MAX_PATH];
    wcpy(iniPath, MAX_PATH, exeDir);
    wcat(iniPath, MAX_PATH, L"\\Config\\");
    wcat(iniPath, MAX_PATH, CONFIG_FILENAME);

    HANDLE h = CreateFileW(iniPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;

    static char buf[8192];
    DWORD rd = 0;
    BOOL ok = ReadFile(h, buf, sizeof(buf) - 1, &rd, NULL);
    CloseHandle(h);

    if (!ok) return;
    buf[rd] = 0;

    char section[64] = "";
    char* line = buf;
    while (*line) {
        char* nl = line;
        while (*nl && *nl != '\n') nl++;
        char save = *nl;
        *nl = 0;

        char* p = line;
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '[') {
            char* end = strchr_a(p, ']');
            if (end) { *end = 0; strcpy_a(section, sizeof(section), p + 1); }
        } else if (*p && *p != ';' && *p != '#') {
            char* eq = strchr_a(p, '=');
            if (eq) {
                *eq = 0;
                char* key = p;
                char* val = eq + 1;
                trim_a(key);
                trim_a(val);

                if (streq_a(section, "Terminal")) {
                    if (ieq_a(key, "UseWindowsTerminal")) {
                        c->useWT = !(ieq_a(val, "no") || ieq_a(val, "0") || ieq_a(val, "false"));
                    } else if (ieq_a(key, "Fallback64")) {
                        ascii_to_w(c->fallback64, MAX_PATH, val);
                    } else if (ieq_a(key, "Fallback32")) {
                        ascii_to_w(c->fallback32, MAX_PATH, val);
                    } else if (ieq_a(key, "MinVersion")) {
                        char* dot = strchr_a(val, '.');
                        if (dot) {
                            *dot = 0;
                            int maj = atoi_a(val);
                            int min = atoi_a(dot + 1);
                            *dot = '.';
                            if (maj > 0) { c->minMajor = maj; c->minMinor = min; }
                        } else {
                            int maj = atoi_a(val);
                            if (maj > 0) { c->minMajor = maj; c->minMinor = 0; }
                        }
                    }
                } else if (streq_a(section, "Startup")) {
                    if (ieq_a(key, "StartDirectory")) {
                        if (*val) ascii_to_w(c->startDir, MAX_PATH, val);
                        else c->startDir[0] = 0;
                    }
                } else if (streq_a(section, "Profile")) {
                    if (ieq_a(key, "PathTranslationStyle")) {
                        char v[128];
                        strcpy_a(v, sizeof(v), val);
                        to_lower_a(v);
                        trim_a(v);
                        if (ieq_a(v, "none") || ieq_a(v, "wsl") || ieq_a(v, "cygwin") ||
                            ieq_a(v, "msys2") || ieq_a(v, "mingw")) {
                            strcpy_a(c->pathStyle, sizeof(c->pathStyle), v);
                        }
                    }
                }
            }
        }
        *nl = save;
        line = nl;
        if (*line == '\r') line++;
        if (*line == '\n') line++;
    }
}

/* ============ registry version check ============ */

static BOOL parse_segments(const wchar_t* seg, int* major, int* minor)
{
    const wchar_t* dot = wchr(seg, L'.');
    if (!dot) return FALSE;
    *major = wsz_str_to_int(seg);
    *minor = wsz_str_to_int(dot + 1);
    return TRUE;
}

/* 返回 TRUE 表示 Windows Terminal 已安装且版本 >= 配置的最低版本 */
static BOOL check_wt_version(const Config* c)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    BOOL found = FALSE;
    DWORD idx = 0;
    const wchar_t prefix[] = L"Microsoft.WindowsTerminal_";
    for (;;) {
        wchar_t name[512];
        DWORD nameSize = 512;
        if (RegEnumKeyExW(hKey, idx, name, &nameSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;
        idx++;
        if (!wprefix(name, prefix)) continue;
        int maj = 0, min = 0;
        if (parse_segments(name + wlen(prefix), &maj, &min)) {
            if (maj > c->minMajor || (maj == c->minMajor && min >= c->minMinor)) {
                found = TRUE;
                break;
            }
        }
    }
    RegCloseKey(hKey);
    return found;
}

/* ============ locate wt.exe ============ */

static BOOL find_wt(wchar_t* out, size_t cap)
{
    wchar_t lad[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", lad, MAX_PATH) == 0)
        return FALSE;
    wcpy(out, cap, lad);
    wcat(out, cap, L"\\Microsoft\\WindowsApps\\wt.exe");
    return (GetFileAttributesW(out) != INVALID_FILE_ATTRIBUTES);
}

/* ============ fragment provisioning ============ */

static void build_fragment(char* out, size_t cap, const char* style)
{
    static const char prefix[] =
        "{\n"
        "  \"profiles\": [\n"
        "    {\n"
        "      \"updates\": \"{0caa0dad-35be-5f56-a8ff-afceeeaa6101}\",\n"
        "      \"pathTranslationStyle\": \"";
    static const char suffix[] = "\"\n"
        "    }\n"
        "  ]\n"
        "}\n";
    size_t n = 0;
    for (const char* s = prefix; *s && n + 1 < cap; ) out[n++] = *s++;
    for (const char* s = style; *s && n + 1 < cap; ) out[n++] = *s++;
    for (const char* s = suffix; *s && n + 1 < cap; ) out[n++] = *s++;
    out[n] = 0;
}

static BOOL ensure_fragment(const char* style)
{
    wchar_t lad[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", lad, MAX_PATH) == 0)
        return FALSE;

    wchar_t fragDir[MAX_PATH];
    wcpy(fragDir, MAX_PATH, lad);
    wcat(fragDir, MAX_PATH, L"\\Microsoft\\Windows Terminal\\Fragments\\Komandare");
    CreateDirectoryW(fragDir, NULL);

    wchar_t fragPath[MAX_PATH];
    wcpy(fragPath, MAX_PATH, fragDir);
    wcat(fragPath, MAX_PATH, L"\\");
    wcat(fragPath, MAX_PATH, FRAGMENT_FILENAME);

    char want[512];
    build_fragment(want, sizeof(want), style);

    HANDLE h = CreateFileW(fragPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        char buf[512];
        DWORD rd = 0;
        BOOL ok = ReadFile(h, buf, sizeof(buf) - 1, &rd, NULL);
        CloseHandle(h);
        if (ok) {
            buf[rd] = 0;
            if (rd == (DWORD)strlen_a(want) && strcmp_a(buf, want) == 0)
                return TRUE;
        }
    }

    h = CreateFileW(fragPath, GENERIC_WRITE, 0, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD written = 0;
    DWORD len = (DWORD)strlen_a(want);
    BOOL ok = WriteFile(h, want, len, &written, NULL);
    CloseHandle(h);
    return ok && written == len;
}

/* ============ launching helpers ============ */

/* 启动回退终端（优先 64 位，其次 32 位）。返回 TRUE 表示已启动。 */
static BOOL launch_fallback(const Config* c, const wchar_t* exeDir,
                            const wchar_t* workDir)
{
    wchar_t path[MAX_PATH];

    wcpy(path, MAX_PATH, exeDir);
    wcat(path, MAX_PATH, L"\\");
    wcat(path, MAX_PATH, c->fallback64);
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
        wcpy(path, MAX_PATH, exeDir);
        wcat(path, MAX_PATH, L"\\");
        wcat(path, MAX_PATH, c->fallback32);
    }
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpFile = path;
    sei.lpDirectory = (workDir && workDir[0]) ? workDir : exeDir;
    sei.nShow = SW_SHOWNORMAL;
    BOOL ok = ShellExecuteExW(&sei);
    if (ok && sei.hProcess) CloseHandle(sei.hProcess);
    return ok;
}

/* ============ PATH bootstrap ============ */

/*
 * 把 exe 所在目录前置到 PATH。
 *
 * 为什么：wt 启动的是裸命令 "cmd /k kInitrd.cmd bash"。cmd 解析裸命令时
 * 先查当前目录，找不到就沿 PATH 搜索——如果本机已装有一份 Komandare
 * 且其目录在 PATH 里，就会错误地启动另一份环境的 kInitrd.cmd（实测踩坑）。
 * 把本 exe 目录放到 PATH 首位后，裸命令必然解析到同目录的 kInitrd.cmd。
 * 相比传完整路径，该方案对含空格的安装目录免疫（wt 参数转发会吃引号）。
 *
 * 注意：ShellExecuteExW 启动的是同进程环境的子进程，SetEnvironmentVariableW
 * 的修改会被子进程继承，wt/conhost/cmd 链路均可见。
 */
static void prepend_exe_dir_to_path(const wchar_t* exeDir)
{
    static wchar_t oldPath[32768];
    DWORD n = GetEnvironmentVariableW(L"PATH", oldPath, 32768);
    if (n == 0 || n >= 32768) {
        /* PATH 为空或异常：直接只写 exeDir */
        SetEnvironmentVariableW(L"PATH", exeDir);
        return;
    }
    /* 已是首位则无需重复前置 */
    if (wlen(oldPath) >= wlen(exeDir) && wprefix(oldPath, exeDir))
        return;
    static wchar_t newPath[32768];
    int written = wsprintfW(newPath, L"%s;%s", exeDir, oldPath);
    if (written > 0 && written < 32768)
        SetEnvironmentVariableW(L"PATH", newPath);
}

/* ============ entry point ============ */

int main(void)
{
    /* 本 exe 所在目录 */
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    wchar_t* sl = wrchr(exeDir, L'\\');
    if (sl) *sl = 0;

    /* 把 exe 目录前置到 PATH：保证裸命令 kInitrd.cmd 解析到本安装目录
     * （否则 PATH 里若注册了另一份 Komandare，会被抢先命中） */
    prepend_exe_dir_to_path(exeDir);

    /* 当前工作目录 */
    wchar_t curDir[MAX_PATH];
    DWORD curLen = GetCurrentDirectoryW(MAX_PATH, curDir);

    /* 加载配置 */
    Config cfg;
    load_config(&cfg, exeDir);

    /* 启动目录：StartDirectory 优先，否则当前目录 */
    wchar_t workDir[MAX_PATH];
    if (cfg.startDir[0]) {
        wcpy(workDir, MAX_PATH, cfg.startDir);
    } else if (curLen > 0 && curLen < MAX_PATH) {
        wcpy(workDir, MAX_PATH, curDir);
    } else {
        workDir[0] = 0;
    }

    /* 决定是否使用 Windows Terminal */
    wchar_t wtPath[MAX_PATH];
    BOOL wtFound = find_wt(wtPath, MAX_PATH);
    BOOL useWT = cfg.useWT && wtFound && check_wt_version(&cfg);

    if (!useWT) {
        if (!launch_fallback(&cfg, exeDir, workDir)) {
            MessageBoxW(NULL,
                        L"既未找到可用的 Windows Terminal，回退终端也启动失败。",
                        L"kmd-win", MB_OK | MB_ICONERROR);
        }
        ExitProcess(0);
    }

    /* 注入 fragment（确保 pathTranslationStyle 配置生效） */
    if (!ensure_fragment(cfg.pathStyle)) {
        MessageBoxW(NULL,
                    L"写入 Windows Terminal Fragment 失败。\n"
                    L"请检查 %LOCALAPPDATA%\\Microsoft\\Windows Terminal\\Fragments 目录权限。",
                    L"kmd-win", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    /* wt 命令行：wt.exe -d <工作目录> cmd /k kInitrd.cmd bash
     * 注意：目录尾的反斜杠会转义结尾引号（"D:\" -> 引号失效，wt 参数解析
     * 错乱后回落到默认 profile），必须去掉尾部反斜杠；盘根 "D:\" -> "D:"
     * 对 cmd/cd 语义等价。 */
    static wchar_t cmd[MAX_PATH * 2];
    if (workDir[0]) {
        static wchar_t cleanDir[MAX_PATH];  /* static: 避免 main 栈帧超 4KB 触发 chkstk */
        wcpy(cleanDir, MAX_PATH, workDir);
        size_t dn = wlen(cleanDir);
        while (dn > 0 && cleanDir[dn - 1] == L'\\') cleanDir[--dn] = 0;
        /* 全是反斜杠（不太可能）或仅 "D:" 形式：直接用原 workDir */
        if (dn == 0) wcpy(cleanDir, MAX_PATH, workDir);
        wsprintfW(cmd, L"-d \"%s\" cmd /k kInitrd.cmd bash", cleanDir);
    } else {
        wsprintfW(cmd, L"cmd /k kInitrd.cmd bash");
    }

    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpFile = wtPath;
    sei.lpParameters = cmd;
    sei.lpDirectory = (workDir[0] ? workDir : exeDir);
    sei.nShow = SW_SHOWNORMAL;

    BOOL ok = ShellExecuteExW(&sei);
    if (!ok || (DWORD)(UINT_PTR)sei.hInstApp <= 32) {
        /* wt 启动失败回退 kmd */
        launch_fallback(&cfg, exeDir, workDir);
    } else if (sei.hProcess) {
        CloseHandle(sei.hProcess);
    }
    ExitProcess(0);
}