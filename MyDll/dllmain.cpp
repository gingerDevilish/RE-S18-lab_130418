// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

void GetDebugPrivs();
BOOL CALLBACK EnumProc(HWND hWnd, LPARAM p);
LRESULT APIENTRY FilterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern __declspec(dllimport) HWND	g_hGCLWnd;
extern __declspec(dllimport) char	g_szCmdLine[4096];

#pragma data_seg (".shared")
HWND	g_hWnd = 0;
char	g_szCmdLine[4096] = { '\0' };
#pragma data_seg ()

#pragma comment(linker,"/SECTION:.shared,RWS") 
WNDPROC wndProcOriginal = NULL;


bool APIENTRY DllMain(HMODULE hModule, const DWORD ulReasonForCall, LPVOID lpReserved)
{
	switch (ulReasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		MessageBox(nullptr, L"... is loading", L"MyDll.dll", MB_OK);
		EnumWindows(EnumProc, GetCurrentProcessId()); //EnumProc is a EnumWindowsProc callback,
		//must return TRUE to continue and FALSE to stop enumeration
		//is applied to every top-level window
		
		if (g_hWnd)
		{
			LPWSTR sz = NULL;

			SetWindowText(g_hWnd, L"Hooked Task Manager");

			HMENU hMenu = GetMenu(g_hWnd);
			int numMenus = GetMenuItemCount(hMenu);

			HMENU hCheck = GetSubMenu(hMenu, numMenus - 1);

			GetMenuString(hMenu, numMenus - 1, sz,
				sizeof(sz), MF_BYPOSITION);

			if (wcscmp(sz, L"Extensions"))
			{
				HMENU hPopup = CreatePopupMenu();

				AppendMenu(hPopup, MF_STRING, 2112, L"Get Extended Info");
				AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_POPUP,
					(UINT_PTR)hPopup, L"Extensions");

				//Subclass the window with our own window procedure.
				//Should probably be more global
				wndProcOriginal = (WNDPROC)SetWindowLongPtr(g_hWnd,
					GWLP_WNDPROC, (LONG_PTR)(WNDPROC)FilterProc); //FilterProc is some replacement value
				//In this case, we set a new address for the window procedure (???)

				DrawMenuBar(g_hWnd);

				GetDebugPrivs();
			}
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default: break;
	}
	return true;
}

