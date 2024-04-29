#include <Windows.h>
#include <stdio.h>

void main(int argc, char *argv[]) {
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

	LPSTR remoteLibPath = VirtualAllocEx(
		proc,
		NULL,
		pathSize,
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
		libPath,
		pathSize,
		NULL
	);
	if (!res) {
		printf("write memory failed\n");
		printf("reason: %d\n", GetLastError());
	}
	else {
		printf("write memory success\n");
	}

	HANDLE hThread = CreateRemoteThread(
		proc,
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)LoadLibrary,
		remoteLibPath,
		0,
		NULL
	);
	if (hThread == NULL) {
		printf("create remote thread failed\n");
		printf("reason: %d\n", GetLastError());
	}
	else {
		printf("success\n");
	}
	
	return;
}
