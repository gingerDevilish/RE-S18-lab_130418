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
int numCPU;

BOOL CALLBACK findTabs(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
)
{
	TCHAR Class[40];
	GetClassName(hwnd, Class, 40);
	if (!_tcscmp(Class, L"SysTabControl32"))
	{
		HWND* handle = (HWND*)lParam;
		*handle = hwnd;
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK findTabChild(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
)
{
	TCHAR Class[40];
	GetClassName(hwnd, Class, 40);
	TCHAR Title[100];
	GetWindowText(hwnd, Title, 100);
	TCHAR msg[256];
	swprintf(msg, L"Class: %s\nTitle: %s", Class, Title);
	//MessageBox(nullptr, msg, L"TABCHILD", MB_OK);
	if (!_tcscmp(Class, L"SysListView32"))
	{
		HWND* handle = (HWND*)lParam;
		if (*handle)
		{
			handle[1] = hwnd;
		}
		else
		{
			handle[0] = hwnd;
		}
		return FALSE;
	}
	return TRUE;
}


BOOL CALLBACK findHeader(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
)
{
	TCHAR Class[40];
	GetClassName(hwnd, Class, 40);
	TCHAR Title[100];
	GetWindowText(hwnd, Title, 100);
	TCHAR msg[256];
	swprintf(msg, L"Class: %s\nTitle: %s", Class, Title);
	//MessageBox(nullptr, msg, L"TABCHILD", MB_OK);
	if (!_tcscmp(Class, L"SysHeader32"))
	{
		HWND* handle = (HWND*)lParam;
		*handle = hwnd;
		
		return FALSE;
	}
	return TRUE;
}

BOOL InsertColumn(HWND hWnd, int index, LPARAM lParam)
{
	SendMessage(hWnd, LVM_INSERTCOLUMN, index, lParam);

	return TRUE;
}


DWORD WINAPI ThreadProc(
	_In_ LPVOID lpParameter
)
{
	Sleep(3000);
	//EnumWindows(EnumProc, GetCurrentProcessId());
	TCHAR s[120];
	swprintf(s, L"Finished ENUMWINDOWS, handle=%d", g_hWnd);
	//MessageBox(nullptr, s, L"MyDll.dll", MB_OK);
	g_hWnd = FindWindow(NULL, L"Диспетчер задач");

	MessageBox(nullptr, s, L"MyDll.dll", MB_OK);
	if (g_hWnd)
	{
		LPWSTR sz = NULL;

		SetWindowText(g_hWnd, L"Hooked Task Manager");
		HWND tabcontrol=NULL;
		EnumChildWindows(g_hWnd, findTabs,(LPARAM) &tabcontrol);
		TCITEM tie = {0};
		tie.mask = TCIF_TEXT;
		tie.pszText = L"HAHAHA";
		if (TabCtrl_SetItem(tabcontrol, 5, &tie) == -1)
		{
			MessageBox(NULL, L"Could Not Set Tab Name!", L"Error", MB_OK);

		}

		HWND tabchild[2] = {0};
		EnumChildWindows(g_hWnd, findTabChild, (LPARAM)tabchild);
		if (!tabchild)
		{
			MessageBox(NULL, L"Null", L"Tabchild", MB_OK);
		}


		HWND header = (HWND)::SendMessage(tabchild[0], LVM_GETHEADER, 0, 0);
		int column_count = SendMessage(header, HDM_GETITEMCOUNT, 0, 0);

		int pid_column_index;
		HDITEM hditem = { 0 };
		hditem.mask = HDI_TEXT;
		for (int i = 0; i < column_count; i++)
		{
			SendMessage(header, HDM_GETITEM, i, (LPARAM)&hditem);
			if (!_tcscmp(hditem.pszText, L"ИД процесса"))
			{
				pid_column_index = i;
			}
		}

		pid_column_index = 1;
		
		Sleep(5000);
		int entries_count = SendMessage(tabchild[0], LVM_GETITEMCOUNT, 0, 0L);

		TCHAR msg[256] = { 0 };
		swprintf(msg, L"Column count: %d\nEntries count: %d", column_count, entries_count);
		MessageBox(NULL, msg, L"Error", MB_OK);

		LVCOLUMN columninfo;
		columninfo.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		columninfo.cx = 150;
		columninfo.pszText = L"Affinity";
		columninfo.iSubItem = column_count;
		InsertColumn(tabchild[0], 7, (LPARAM)&columninfo);

		MessageBox(NULL, L"Inserted Column", L"Error", MB_OK);

		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		numCPU = sysinfo.dwNumberOfProcessors;

		//DWORD* affinity_map = new DWORD[entries_count];
		std::vector<DWORD>* affinity_map = new std::vector<DWORD>();
		TCHAR* affinity_repr = new TCHAR[numCPU + 1];
		LVITEM lvitem = { 0 };
		WCHAR* buffer = new WCHAR[80];
		DWORD_PTR sysaffinity;
		DWORD_PTR affinity;
		while (1) {
			lvitem.mask = LVIF_TEXT;
			lvitem.iSubItem = pid_column_index;
			for (int i = 0; i < entries_count; i++)
			{
				memset(affinity_repr, '\0', (numCPU + 1) * sizeof(TCHAR));
				memset(buffer, '\0', 80);

				lvitem.iSubItem = pid_column_index;
				lvitem.pszText = buffer;
				ListView_GetItemText(tabchild[0], i, pid_column_index, buffer, 256);
				int pid = _ttoi(lvitem.pszText);

				GetProcessAffinityMask(OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid),
					(PDWORD_PTR)&affinity,
					(PDWORD_PTR)&sysaffinity);
				affinity_map->push_back(affinity);

				lvitem.iSubItem = 7;
				for (int j = 0; j < numCPU; j++)
				{
					BOOL flag = ((*affinity_map)[i] >> j) & 1;
					affinity_repr[j] = flag ? '+' : '-';
				}
				lvitem.pszText = affinity_repr;
				SendMessage(tabchild[0], LVM_SETITEMTEXT, i, (LPARAM)&lvitem);
			}

			affinity_map->clear();
		}


		// somewhere here move to filterproc
		//retrieve PID +
		//find affinities +
		//make affinities matrix +
		//print to the column +
		//move somewhere where it can be feasible
		//ensure regular & adequate updates




		/*
		{

			//Subclass the window with our own window procedure.
			//Should probably be more global
			wndProcOriginal = (WNDPROC)SetWindowLongPtr(g_hWnd,
				GWLP_WNDPROC, (LONG_PTR)(WNDPROC)FilterProc); //FilterProc is some replacement value
															  //In this case, we set a new address for the window procedure (???)


			GetDebugPrivs();
		}*/
	}

	return 0;
}

bool APIENTRY DllMain(HMODULE hModule, const DWORD ulReasonForCall, LPVOID lpReserved)
{
	switch (ulReasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		//MessageBox(nullptr, L"... is loading", L"MyDll.dll", MB_OK);
		CreateThread(NULL, 0, ThreadProc, nullptr, 0, NULL);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default: break;
	}
	return true;
}

