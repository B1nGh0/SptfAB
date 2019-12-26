#include <windows.h>
#include <tchar.h>

#ifdef __cplusplus
#define ptr_typeof decltype
#else
#define ptr_typeof(x) LPVOID
#endif

#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC optimize("-O2")
#elif defined _MSC_VER
#pragma optimize("g", on)
#endif

typedef void (*fWrapped)(void);
fWrapped oDWriteCreateFactory = NULL;
EXTERN_C __declspec(dllexport) void DWriteCreateFactory(void) { oDWriteCreateFactory(); }

#ifdef __GNUC__
#pragma GCC pop_options
#elif defined _MSC_VER
#pragma optimize("", on)
#endif

void Wrapper(void)
{
	static HMODULE Wrapped = NULL;
	TCHAR Path[MAX_PATH];
	if(!Wrapped) {
		ExpandEnvironmentStrings(_T("%SystemRoot%\\System32\\DWrite.dll"), Path, _countof(Path));
		if((Wrapped = LoadLibrary(Path)))
			oDWriteCreateFactory = (fWrapped)GetProcAddress(Wrapped, "DWriteCreateFactory");
	} else {
		FreeLibrary(Wrapped);
		Wrapped = NULL;
	}
}

void Patch(void)
{
	DWORD *Scan, Patchs[] = {0x00646100, 0x00696900, 0x2F736461, 0x2F736969, 0x00736461, 0x00736969, 0x0064612F, 0x0069692F};
	DWORD_PTR Base = (DWORD_PTR)GetModuleHandle(NULL);
	PIMAGE_DOS_HEADER Dos = (PIMAGE_DOS_HEADER)Base;
	PIMAGE_NT_HEADERS Nt = (PIMAGE_NT_HEADERS)(Base + Dos->e_lfanew);
	if(Nt->OptionalHeader.SizeOfImage < sizeof(*Scan)) return;
	for(DWORD_PTR i = 0; i <= Nt->OptionalHeader.SizeOfImage - sizeof(*Scan); i++) {
		Scan = (ptr_typeof(Scan))(Base + i);
		for(DWORD_PTR j = 0; j < _countof(Patchs) / 2; j++)
			if(*Scan == Patchs[j * 2]) {
				DWORD OldProtect;
				VirtualProtect(Scan, sizeof(*Scan), PAGE_EXECUTE_READWRITE, &OldProtect);
				*Scan = Patchs[j * 2 + 1];
				VirtualProtect(Scan, sizeof(*Scan), OldProtect, &OldProtect);
				break;
			}
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch(fdwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		Patch();
	case DLL_PROCESS_DETACH:
		Wrapper();
	}
	return TRUE;
}

#ifdef NOSTDLIB
EXTERN_C BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { return DllMain(hinstDLL, fdwReason, lpvReserved); }
#endif
