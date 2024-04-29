#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "advapi32.lib")

typedef NTSTATUS(NTAPI* pNtCreateThreadEx) (
	OUT PHANDLE hThread,
	IN ACCESS_MASK DesiredAccess,
	IN PVOID ObjectAttributes,
	IN HANDLE ProcessHandle,
	IN PVOID lpStartAddress,
	IN PVOID lpParameter,
	IN ULONG Flags,
	IN SIZE_T StackZeroBits,
	IN SIZE_T SizeOfStackCommit,
	IN SIZE_T SizeOfStackReserve,
	OUT PVOID lpBytesBuffer
	);

typedef struct _LSA_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} LSA_UNICODE_STRING, * PLSA_UNICODE_STRING, UNICODE_STRING, * PUNICODE_STRING;

typedef NTSTATUS(NTAPI* pRtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
typedef NTSTATUS(NTAPI* pLdrLoadDll)(PWCHAR, ULONG, PUNICODE_STRING, PHANDLE);
typedef DWORD64(WINAPI* _NtCreateThreadEx64)(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, LPVOID ObjectAttributes, HANDLE ProcessHandle, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, BOOL CreateSuspended, DWORD64 dwStackSize, DWORD64 dw1, DWORD64 dw2, LPVOID Unknown);

typedef struct _THREAD_DATA
{
	pRtlInitUnicodeString fnRtlInitUnicodeString;
	pLdrLoadDll fnLdrLoadDll;
	UNICODE_STRING UnicodeString;
	WCHAR DllName[260];
	PWCHAR DllPath;
	ULONG Flags;
	HANDLE ModuleHandle;
}THREAD_DATA, * PTHREAD_DATA;

HANDLE WINAPI ThreadProc(PTHREAD_DATA data) {
	data->fnRtlInitUnicodeString(&data->UnicodeString, data->DllName);
	data->fnLdrLoadDll(data->DllPath, data->Flags, &data->UnicodeString, &data->ModuleHandle);
	return data->ModuleHandle;
}

DWORD WINAPI ThreadProcEnd() {
	return 0;
}

void main(int argc, char* argv[]) {
	DWORD pid = strtoul(argv[1], NULL, 0);
	printf("pid: %li\n", pid);

	HANDLE proc = OpenProcess(
		PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
		FALSE,
		pid
	);
	if (proc == NULL) {
		printf("open process failed\n");
		printf("reason: %d\n", GetLastError());
	}
	else {
		printf("open process success\n");
	}


	char* filename = argv[2];
	char currentDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, currentDir);
	char libPath[MAX_PATH];
	snprintf(libPath, MAX_PATH, "%s\\%s", currentDir, filename);

	DWORD pathSize = strlen(libPath) + 1;
	size_t converted = 0;
	wchar_t* DllFullPath;
	DllFullPath = (wchar_t*)malloc(pathSize * sizeof(wchar_t));
	mbstowcs_s(&converted, DllFullPath, pathSize, libPath, _TRUNCATE);

	THREAD_DATA data;
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	data.fnRtlInitUnicodeString = (pRtlInitUnicodeString)GetProcAddress(hNtdll, "RtlInitUnicodeString");
	data.fnLdrLoadDll = (pLdrLoadDll)GetProcAddress(hNtdll, "LdrLoadDll");
	memcpy(data.DllName, DllFullPath, (wcslen(DllFullPath) + 1) * sizeof(WCHAR));
	data.DllPath = NULL;
	data.Flags = 0;
	data.ModuleHandle = INVALID_HANDLE_VALUE;

	LPVOID remoteLibPath = VirtualAllocEx(
		proc,
		NULL,
		4096,
		MEM_COMMIT,
		PAGE_READWRITE
	);
	if (remoteLibPath == NULL) {
		printf("virtual alloc failed\n");
		printf("reason: %d\n", GetLastError());
	}
	else {
		printf("virtual alloc success\n");
	}

	BOOL res = WriteProcessMemory(
		proc,
		remoteLibPath,
		&data,
		sizeof(data),
		NULL
	);
	if (!res) {
		printf("write memory failed\n");
		printf("reason: %d\n", GetLastError());
	}
	else {
		printf("write memory success\n");
	}

	DWORD SizeOfCode = (DWORD)ThreadProcEnd - (DWORD)ThreadProc;
	LPVOID pCode = VirtualAllocEx(proc, NULL, SizeOfCode, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (pCode == NULL) {
		printf("virtual alloc failed\n");
		printf("reason: %d\n", GetLastError());
	}
	else {
		printf("virtual alloc success\n");
	}
	BOOL bWriteOK = WriteProcessMemory(proc, pCode, (PVOID)ThreadProc, SizeOfCode, NULL);
	if (!bWriteOK) {
		printf("write memory failed\n");
		printf("reason: %d\n", GetLastError());
	}
	else {
		printf("write memory success\n");
	}

	pNtCreateThreadEx ntCTEx = (pNtCreateThreadEx)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtCreateThreadEx");

	HANDLE ht;
	ntCTEx(&ht, 0x1FFFFF, NULL, proc, (LPTHREAD_START_ROUTINE)pCode, remoteLibPath, FALSE, NULL, NULL, NULL, NULL);

	if (ht == NULL) {
		printf("create thread failed\n");
		return;
	}
	else {
		printf("create thread success\n");
	}
	return;
}
