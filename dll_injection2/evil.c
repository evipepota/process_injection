#include <Windows.h>
#pragma comment(lib, "user32")

void evil() {
    DWORD pid = GetCurrentProcessId();
    char msg[100];
    sprintf(msg, "injected! pid: %li", pid);
    MessageBox(NULL, msg, "injected", MB_SYSTEMMODAL);
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        evil();
    }
    return TRUE;
}
