//#include "definitions.h"

#include <Windows.h>
#include <intrin.h>
#include <string>
#include <TlHelp32.h>
#include <psapi.h>
#include <detours/detours.h>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

decltype(&GetSidSubAuthority) pGetSidSubAuthority;

PDWORD WINAPI hook_GetSidSubAuthority(PSID  pSid, DWORD nSubAuthority)
{
	PDWORD ret = pGetSidSubAuthority(pSid, nSubAuthority);
	if (GetLastError() == 0) {
		if (*ret >= 0x3000 && *ret < 0x10000) { // just a sanity check
			DWORD oriIL = *ret;
			*ret = 0x2000; // Medium IL, check condition: (unsigned int)(ilLevel - 0x2000) <= 0xFFF
			OutputDebugStringA(std::format("[PunchDataExchangeHost] replace TokenIL from 0x{:x} to 0x{:x}", oriIL, *ret).c_str());
			SetLastError(0);
		}
	}
	return ret;
}

bool hook() {
	*(void**)&pGetSidSubAuthority = GetProcAddress(LoadLibraryA("KernelBase.dll"), "GetSidSubAuthority");
	
	LONG ret = 0;
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	ret = DetourUpdateThread(GetCurrentThread());
	ret = DetourAttach(&(LPVOID&)pGetSidSubAuthority, hook_GetSidSubAuthority);
	ret = DetourTransactionCommit();
	return ret == NO_ERROR;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call != DLL_PROCESS_ATTACH)
		return TRUE;

	OutputDebugStringA("[PunchDataExchangeHost] Loaded!");

	if (!hook()) {
		OutputDebugStringA("[PunchDataExchangeHost] Hook Failed!");
		return FALSE;
	}
	OutputDebugStringA("[PunchDataExchangeHost] Hook Success!");
	return TRUE;
}