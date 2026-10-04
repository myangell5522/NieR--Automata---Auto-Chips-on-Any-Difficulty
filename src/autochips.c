/*
 * AutoChips — NieR: Automata plugin
 *
 * Steam build dated 17 July 2021 rejects the five auto-control chips
 * (IDs 0x0D1A through 0x0D1E) unless difficulty is Easy. Two functions
 * share the same gate:
 *
 *   lea eax, [rdx-0x0D1A]
 *   cmp eax, 4
 *   ja  allow          ; not one of those five chips
 *   call get_difficulty
 *   test eax, eax
 *   jnz reject         ; difficulty != Easy
 *
 * The short jump is the byte 0x77. Replacing it with 0xEB (jmp) always
 * takes the allow path. Difficulty itself is a global read from dozens
 * of other places, so this plugin never writes that value. Combat
 * difficulty stays whatever the player picked.
 *
 * Loaded by nier-mod-loader from data\mods\plugins. The game exe on disk
 * is not modified.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AUTOCHIPS_VERSION "1.0"

/* lea eax,[rdx-0x0D1A]; mov ebx,edx; cmp eax,4 */
static const uint8_t k_site1[] = {
    0x8D, 0x82, 0xE6, 0xF2, 0xFF, 0xFF, 0x8B, 0xDA, 0x83, 0xF8, 0x04
};

/* lea eax,[rdx-0x0D1A]; mov ebx,edx; mov rdi,rcx; cmp eax,4 */
static const uint8_t k_site2[] = {
    0x8D, 0x82, 0xE6, 0xF2, 0xFF, 0xFF, 0x8B, 0xDA, 0x48, 0x8B, 0xF9, 0x83, 0xF8, 0x04
};

typedef struct Log {
    char text[4096];
    size_t used;
} Log;

static void log_clear(Log *log)
{
    log->text[0] = 0;
    log->used = 0;
}

static void log_append(Log *log, const char *line)
{
    size_t len = strlen(line);
    if (log->used + len + 2 >= sizeof log->text)
        return;
    memcpy(log->text + log->used, line, len);
    log->used += len;
    log->text[log->used++] = '\n';
    log->text[log->used] = 0;
}

static int protect_readable(DWORD protect)
{
    if (protect & (PAGE_GUARD | PAGE_NOACCESS))
        return 0;

    switch (protect & 0xFF) {
    case PAGE_READONLY:
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return 1;
    default:
        return 0;
    }
}

/* Match the unique prefix, then a ja/jmp (77 or EB) with displacement 09. */
static int find_pattern(const uint8_t *base, size_t size,
                         const uint8_t *pat, size_t pat_len,
                         const uint8_t **match_out)
{
    int count = 0;
    const uint8_t *only = NULL;

    if (size < pat_len + 2)
        return 0;

    for (size_t i = 0; i + pat_len + 2 <= size; i++) {
        uint8_t op;
        if (memcmp(base + i, pat, pat_len) != 0)
            continue;
        op = base[i + pat_len];
        if ((op != 0x77 && op != 0xEB) || base[i + pat_len + 1] != 0x09)
            continue;
        if (count == 0)
            only = base + i;
        else
            only = NULL;
        count++;
    }

    if (count == 1 && match_out)
        *match_out = only;
    return count;
}

static int find_in_image(uint8_t *base, size_t size,
                          const uint8_t *pat, size_t pat_len,
                          const uint8_t **match_out)
{
    uint8_t *cursor = base;
    uint8_t *end = base + size;
    int count = 0;
    const uint8_t *only = NULL;

    while (cursor < end) {
        MEMORY_BASIC_INFORMATION info;
        uint8_t *region;
        uint8_t *region_end;
        SIZE_T queried = VirtualQuery(cursor, &info, sizeof info);
        if (queried == 0)
            break;

        region = (uint8_t *)info.BaseAddress;
        region_end = region + info.RegionSize;
        if (region < cursor)
            region = cursor;
        if (region_end > end)
            region_end = end;

        if (info.State == MEM_COMMIT && protect_readable(info.Protect) && region < region_end) {
            const uint8_t *local = NULL;
            int found = find_pattern(region, (size_t)(region_end - region), pat, pat_len, &local);
            if (found > 0) {
                if (count == 0 && found == 1)
                    only = local;
                else
                    only = NULL;
                count += found;
            }
        }

        if (region_end <= cursor)
            break;
        cursor = region_end;
    }

    if (count == 1 && match_out)
        *match_out = only;
    return count;
}

static int write_jmp(uint8_t *opcode)
{
    DWORD old_protect = 0;
    DWORD ignored = 0;

    if (!VirtualProtect(opcode, 1, PAGE_EXECUTE_READWRITE, &old_protect))
        return 0;
    *opcode = 0xEB;
    FlushInstructionCache(GetCurrentProcess(), opcode, 1);
    VirtualProtect(opcode, 1, old_protect, &ignored);
    return *opcode == 0xEB;
}

static void apply_site(Log *log, const char *name, uint8_t *image, size_t image_size,
                        const uint8_t *pat, size_t pat_len)
{
    const uint8_t *match = NULL;
    int count = find_in_image(image, image_size, pat, pat_len, &match);
    uint8_t *opcode;
    unsigned long long rva;

    if (count != 1 || !match) {
        char line[160];
        snprintf(line, sizeof line, "%s: %d match(es), left untouched", name, count);
        log_append(log, line);
        return;
    }

    opcode = (uint8_t *)(match + pat_len);
    rva = (unsigned long long)(opcode - image);

    if (*opcode == 0xEB) {
        char line[160];
        snprintf(line, sizeof line, "%s: already applied at RVA 0x%llX", name, rva);
        log_append(log, line);
        return;
    }
    if (*opcode != 0x77) {
        char line[160];
        snprintf(line, sizeof line, "%s: unexpected opcode 0x%02X", name, *opcode);
        log_append(log, line);
        return;
    }
    if (!write_jmp(opcode)) {
        char line[160];
        snprintf(line, sizeof line, "%s: VirtualProtect failed (%lu)", name, GetLastError());
        log_append(log, line);
        return;
    }
    {
        char line[160];
        snprintf(line, sizeof line, "%s: patched at RVA 0x%llX", name, rva);
        log_append(log, line);
    }
}

static int path_is_game(const wchar_t *path)
{
    const wchar_t suffix[] = L"nierautomata.exe";
    size_t path_len = wcslen(path);
    size_t suffix_len = (sizeof suffix / sizeof suffix[0]) - 1;
    const wchar_t *tail;

    if (path_len < suffix_len)
        return 0;
    tail = path + (path_len - suffix_len);
    for (size_t i = 0; i < suffix_len; i++) {
        wchar_t c = tail[i];
        if (c >= L'A' && c <= L'Z')
            c = (wchar_t)(c - L'A' + L'a');
        if (c != suffix[i])
            return 0;
    }
    return 1;
}

static void patch_process(Log *log)
{
    wchar_t process_path[MAX_PATH];
    char process_utf8[MAX_PATH * 3];
    DWORD path_len;
    uint8_t *image;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS64 *nt;

    log_append(log, "NieR AutoChips " AUTOCHIPS_VERSION);

    path_len = GetModuleFileNameW(NULL, process_path, MAX_PATH);
    if (path_len == 0 || path_len >= MAX_PATH) {
        log_append(log, "Could not read the process path.");
        return;
    }

    WideCharToMultiByte(CP_UTF8, 0, process_path, -1, process_utf8, sizeof process_utf8, NULL, NULL);
    {
        char line[MAX_PATH * 3 + 32];
        snprintf(line, sizeof line, "Process: %s", process_utf8);
        log_append(log, line);
    }

    if (!path_is_game(process_path)) {
        log_append(log, "Host is not NieRAutomata.exe. Nothing was patched.");
        return;
    }

    image = (uint8_t *)GetModuleHandleW(NULL);
    if (!image) {
        log_append(log, "GetModuleHandle failed.");
        return;
    }

    dos = (IMAGE_DOS_HEADER *)image;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        log_append(log, "Process image has no DOS header.");
        return;
    }
    nt = (IMAGE_NT_HEADERS64 *)(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
        log_append(log, "Process image is not a 64-bit PE.");
        return;
    }

    apply_site(log, "Site 1", image, nt->OptionalHeader.SizeOfImage, k_site1, sizeof k_site1);
    apply_site(log, "Site 2", image, nt->OptionalHeader.SizeOfImage, k_site2, sizeof k_site2);
}

static void write_log_file(HINSTANCE self, const Log *log)
{
    wchar_t path[MAX_PATH];
    DWORD path_len;
    wchar_t *dot = NULL;
    wchar_t *slash;
    HANDLE file;
    DWORD written = 0;

    path_len = GetModuleFileNameW(self, path, MAX_PATH);
    if (path_len == 0 || path_len >= MAX_PATH)
        return;

    slash = path;
    for (DWORD i = 0; i < path_len; i++) {
        if (path[i] == L'\\' || path[i] == L'/')
            slash = path + i + 1;
        if (path[i] == L'.')
            dot = path + i;
    }
    if (!dot || dot < slash)
        return;
    if ((size_t)(dot - path) + 5 >= MAX_PATH)
        return;
    dot[1] = L'l';
    dot[2] = L'o';
    dot[3] = L'g';
    dot[4] = 0;

    file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;
    WriteFile(file, log->text, (DWORD)log->used, &written, NULL);
    CloseHandle(file);
}

#ifndef AUTOCHIPS_FILE_CHECK

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        Log log;
        DisableThreadLibraryCalls(instance);
        log_clear(&log);
        patch_process(&log);
        write_log_file(instance, &log);
    }
    return TRUE;
}

#else

static int scan_file(const char *path)
{
    HANDLE file;
    LARGE_INTEGER file_size;
    uint8_t *data;
    DWORD read_total = 0;
    const uint8_t *match1 = NULL;
    const uint8_t *match2 = NULL;
    int count1;
    int count2;
    int ok;

    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Could not open %s (%lu)\n", path, GetLastError());
        return 1;
    }
    if (!GetFileSizeEx(file, &file_size) || file_size.QuadPart <= 0 || file_size.QuadPart > 64 * 1024 * 1024) {
        fprintf(stderr, "Unexpected file size\n");
        CloseHandle(file);
        return 1;
    }

    data = (uint8_t *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)file_size.QuadPart);
    if (!data) {
        CloseHandle(file);
        return 1;
    }
    while (read_total < (DWORD)file_size.QuadPart) {
        DWORD chunk = 0;
        if (!ReadFile(file, data + read_total, (DWORD)file_size.QuadPart - read_total, &chunk, NULL) || chunk == 0) {
            fprintf(stderr, "Read failed\n");
            HeapFree(GetProcessHeap(), 0, data);
            CloseHandle(file);
            return 1;
        }
        read_total += chunk;
    }
    CloseHandle(file);

    count1 = find_pattern(data, (size_t)file_size.QuadPart, k_site1, sizeof k_site1, &match1);
    count2 = find_pattern(data, (size_t)file_size.QuadPart, k_site2, sizeof k_site2, &match2);

    if (count1 == 1) {
        printf("site1 count=1 offset=0x%llX opcode=%02X\n",
               (unsigned long long)(match1 - data), match1[sizeof k_site1]);
    } else {
        printf("site1 count=%d\n", count1);
    }
    if (count2 == 1) {
        printf("site2 count=1 offset=0x%llX opcode=%02X\n",
               (unsigned long long)(match2 - data), match2[sizeof k_site2]);
    } else {
        printf("site2 count=%d\n", count2);
    }

    ok = count1 == 1 && count2 == 1 &&
         match1[sizeof k_site1] == 0x77 && match2[sizeof k_site2] == 0x77;
    HeapFree(GetProcessHeap(), 0, data);
    return ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: check_patterns.exe <NieRAutomata.exe>\n");
        return 2;
    }
    return scan_file(argv[1]);
}

#endif
