// MyDll.cpp : Defines the exported functions for the DLL application.
//
#pragma warning(disable:4996)
#include "stdafx.h"

extern HWND	g_hWnd;
extern WNDPROC wndProcOriginal;


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

	if (p == pid)
	{
		g_hWnd = hWnd;
		return FALSE;
	}

	return TRUE;
}

void ShowStats(HWND hDlg)
{
	HWND hChild = GetWindow(g_hWnd, GW_CHILD);
	hChild = GetWindow(hChild, GW_CHILD);
	int ic = SendMessage(hChild, LVM_GETITEMCOUNT, 0, 0);

	if (ic != 0)
	{
		int pidColumn = -1;

		for (int i = 0; i<20; i++)
		{
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT;
			lvc.pszText = new WCHAR[255];
			lvc.cchTextMax = 255;

			wcscpy(lvc.pszText, L"");

			SendMessage(hChild, LVM_GETCOLUMN, i, (LPARAM)&lvc);

			if (wcscmp(lvc.pszText, L"PID") == 0)
			{
				pidColumn = i;
			}

			delete[] lvc.pszText;

			if (pidColumn != -1)
				break;
		}

		if (pidColumn != -1)
		{
			int sel = SendMessage(hChild, LVM_GETSELECTIONMARK, 0, 0);

			if (sel == -1)
			{
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), L"Nothing Selected.");
			}
			else
			{
				LVITEM lvi;
				lvi.mask = LVIF_TEXT;
				lvi.pszText = new WCHAR[16];
				lvi.cchTextMax = 16;
				lvi.iItem = sel;
				lvi.iSubItem = pidColumn;

				SendMessage(hChild, LVM_GETITEMTEXT, sel, (LPARAM)&lvi);

				int pid = _wtoi(lvi.pszText);

				delete[] lvi.pszText;

				//std::string str1 = GetCmdLineData(pid);

				//std::string str = GetModuleInfo(pid);
				//SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), std::string(str1 + "\r\n\r\n" + str).c_str());
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), L"Zero fucks given");
			}
		}
		else
		{
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), L"PID Column now shown");
		}
	}
	else
	{
		SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), L"Process List not shown.");
	}
}


INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ShowStats(hDlg);

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

	return CallWindowProc(wndProcOriginal, hwnd, uMsg, wParam, lParam);
}