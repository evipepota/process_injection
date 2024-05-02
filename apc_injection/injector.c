#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>

void GetProcessThreads(DWORD dwPID, HANDLE* lpThreads) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0x0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);
    if (!Thread32First(hSnapshot, &te)) {
        CloseHandle(hSnapshot);
        return;
    }

    HANDLE hThread = NULL;
    int i = 0;
    do {
        if (te.th32OwnerProcessID == dwPID) {
            hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, te.th32ThreadID);
            if (hThread != NULL) {
                lpThreads[i] = hThread;
                i++;
            }
        }
    } while (Thread32Next(hSnapshot, &te));
}

void main(int argc, char* argv[]) {
    DWORD pid = strtoul(argv[1], NULL, 0);
    printf("pid: %li\n", pid);

    HANDLE proc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
        FALSE,
        pid);
    if (proc == NULL) {
        printf("open process failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("open process success\n");
    }

    char* filename = argv[2];
    char currentDir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDir);
    char libPath[MAX_PATH];
    snprintf(libPath, MAX_PATH, "%s\\%s", currentDir, filename);

    DWORD pathSize = strlen(libPath) + 1;

    LPSTR remoteLibPath = VirtualAllocEx(
        proc,
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
        proc,
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

    HANDLE lpThreads[1000] = {0};
    GetProcessThreads(pid, &lpThreads);
    for (int i = 0; i < 1000; i++) {
        if (lpThreads[i] != 0) {
            printf("thread: %d\n", lpThreads[i]);
            HANDLE apc = QueueUserAPC((PAPCFUNC)LoadLibrary, lpThreads[i], (ULONG_PTR)remoteLibPath);
            Sleep(200);
            CloseHandle(lpThreads[i]);
        }
    }
    return;
}
