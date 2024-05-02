#include <Windows.h>
#include <stdio.h>

void main(int argc, char* argv[]) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    HANDLE hoge = CreateProcessA(
        "C:\\Windows\\System32\\Notepad.exe",
        NULL, NULL, NULL, FALSE,
        CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    WaitForSingleObject(pi.hProcess, 5000);

    if (hoge == NULL) {
        printf("create process failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("create process success\n");
    }

    char* filename = argv[1];
    char currentDir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDir);
    char libPath[MAX_PATH];
    snprintf(libPath, MAX_PATH, "%s\\%s", currentDir, filename);

    DWORD pathSize = strlen(libPath) + 1;

    LPSTR remoteLibPath = VirtualAllocEx(
        pi.hProcess,
        NULL,
        pathSize,
        MEM_COMMIT,
        PAGE_READWRITE);
    if (remoteLibPath == NULL) {
        printf("virtual alloc failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("virtual alloc success\n");
    }

    BOOL res = WriteProcessMemory(
        pi.hProcess,
        remoteLibPath,
        libPath,
        pathSize,
        NULL);
    if (!res) {
        printf("write memory failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("write memory success\n");
    }

    BOOL apc = QueueUserAPC((PAPCFUNC)LoadLibrary, pi.hThread, (ULONG_PTR)remoteLibPath);
    while (WAIT_IO_COMPLETION == WaitForSingleObjectEx(apc, INFINITE, TRUE));
    if (apc == NULL) {
        printf("queue user apc failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("queue user apc success\n");
    }
    ResumeThread(pi.hThread);
    return;
}
