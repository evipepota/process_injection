#include <Windows.h>
#include <stdio.h>
#include <winternl.h>

#pragma comment(lib, "advapi32.lib")

void main(int argc, char *argv[]) {
    DWORD pid = strtoul(argv[1], NULL, 0);
    printf("pid: %li\n", pid);

    HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid);
    if (proc == NULL) {
        printf("open process failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("open process success\n");
    }

    char *filename = argv[2];
    char currentDir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDir);
    char libPath[MAX_PATH];
    snprintf(libPath, MAX_PATH, "%s\\%s", currentDir, filename);

    DWORD pathSize = strlen(libPath) + 1;

    size_t converted = 0;
    wchar_t *DllFullPath;
    DllFullPath = (wchar_t *)malloc(pathSize * sizeof(wchar_t));
    mbstowcs_s(&converted, DllFullPath, pathSize, libPath, _TRUNCATE);

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");

    LPVOID remoteLibPath = VirtualAllocEx(proc, NULL, sizeof(DllFullPath), MEM_COMMIT, PAGE_READWRITE);
    if (remoteLibPath == NULL) {
        printf("virtual alloc failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("virtual alloc success\n");
    }

    BOOL res = WriteProcessMemory(proc, remoteLibPath, DllFullPath, sizeof(DllFullPath), NULL);
    if (!res) {
        printf("write memory failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("write memory success\n");
    }

    FARPROC LdrLoadDll = GetProcAddress(hNtdll, "LdrLoadDll");

    UNICODE_STRING name;
    PHANDLE Module;
    name.Buffer = DllFullPath;
    name.Length = wcslen(name.Buffer) * sizeof(wchar_t);
    name.MaximumLength = name.Length + sizeof(wchar_t);

    HANDLE hThread = CreateRemoteThread(
        proc,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)LdrLoadDll((wchar_t *)0, 0, &name, &Module),
        remoteLibPath,
        0,
        NULL);
    if (hThread == NULL) {
        printf("create remote thread failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("success\n");
    }

    return;
}
