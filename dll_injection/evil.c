#include <Windows.h>
#pragma comment(lib, "user32")

void evil() {
	MessageBox(NULL, "injected", "injected", MB_SYSTEMMODAL);
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		evil();
	}
	return TRUE;
}
