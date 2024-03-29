// MyDll.cpp : Defines the exported functions for the DLL application.
//! as for now, functions here are unused
#pragma warning(disable:4996)
#include "stdafx.h"

extern HWND	g_hWnd;
extern WNDPROC wndProcOriginal;
extern int numCPU;

#define IDC_EDIT1                       1001
#define IDD_EX_DLG                      101

void GetDebugPrivs()
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tp;

	if (::OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		if (!::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))
		{
			::CloseHandle(hToken);
		}

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = sedebugnameValue;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!::AdjustTokenPrivileges(hToken,
			FALSE, &tp, sizeof(tp), NULL, NULL))
		{
			::CloseHandle(hToken);
		}

		::CloseHandle(hToken);
	}
}

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM p)
{

	DWORD pid;
	GetWindowThreadProcessId(hWnd, &pid);
	TCHAR tmp[80];
	GetWindowText(hWnd, tmp, 79);
	TCHAR s[250];
	swprintf(s, L"p=%d\npid=%d\nhWnd=%p\nname: %s", p, pid, hWnd, tmp);
	//MessageBox(nullptr, s, L"EnumProc", MB_OK);

	if ((p == pid)/* || _tcsstr(tmp, L"Notepad")*/)
	{
		g_hWnd = hWnd;
		MessageBox(nullptr, L"Found the needed process:D", L"EnumProc", MB_OK);
		return FALSE;
	}

	return TRUE;
}



INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		return TRUE;
	}

	case WM_COMMAND:
	{
		if (wParam == IDCANCEL)
		{
			EndDialog(hDlg, IDCANCEL);
		}
	}
	}

	return FALSE;
}

LRESULT APIENTRY FilterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		if (LOWORD(wParam) == 2112)
		{
			DialogBox(GetModuleHandle(L"InstallTaskHook.dll"), MAKEINTRESOURCEW(IDD_EX_DLG), g_hWnd, DlgProc);
			int err = GetLastError();
		}
	}

	if (uMsg == LVM_SETITEMW)
	{
		LVITEMW* item = (LVITEMW*)lParam;
		//if (item->iSubItem == 5)
		//{
			CallWindowProc(wndProcOriginal, hwnd, uMsg, wParam, lParam);
		//	item->iSubItem = 1;
		//}
	}

	return CallWindowProc(wndProcOriginal, hwnd, uMsg, wParam, lParam);
}