// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

bool APIENTRY DllMain(HMODULE hModule, const DWORD ulReasonForCall, LPVOID lpReserved)
{
	switch (ulReasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		MessageBox(nullptr, L"... is loading", L"MyDll.dll", MB_OK);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default: break;
	}
	return true;
}

