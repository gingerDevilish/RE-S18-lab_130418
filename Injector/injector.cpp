// Injector.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "injector.h"
#include <string>
#include <iostream>
#include <fstream>


#ifdef _WIN64
UCHAR g_shellcode[] =
{
	/*0x00:*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	 //pLoadLibrary pointer, RUNTIME
	/*0x08:*/ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,    // 8x nops to fix disassembly of VS
	/*0x10:*/ 0x48, 0x83, 0xEC, 0x28,							 //sub         rsp,28h
	/*0x14:*/ 0x48, 0x8D, 0x0D, 0x1D, 0x00, 0x00, 0x00,			 //lea         rcx,[RIP+(38h-1Bh)]
	/*0x1B:*/ 0xFF, 0x15, 0xDF, 0xFF, 0xFF, 0xFF,				 //call        qword ptr[RIP-(21h-0)]
	/*0x21:*/ 0x31, 0xC9,										 //xor         ecx, ecx
	/*0x23:*/ 0x83, 0xCA, 0xFF,	    							 //or          edx, 0FFFFFFFFh
	/*0x26:*/ 0x48, 0x85, 0xC0,									 //test        rax, rax
	/*0x29:*/ 0x0F, 0x44, 0xCA,									 //cmove       ecx, edx
	/*0x2C:*/ 0x89, 0xC8,										 //mov         eax, ecx
	/*0x2E:*/ 0x48, 0x83, 0xC4, 0x28,							 //add         rsp, 28h
	/*0x32:*/ 0xC3,											     //ret
	/*0x33:*/ 0x90, 0x90, 0x90, 0x90, 0x90						 // 5x nop for alignment
	/*0x38:*/												     // String: "MyDll.dll"
};

#else
UCHAR g_shellcode[] =
{
	/*0x00:*/ 0x00, 0x00, 0x00, 0x00, 			   //pLoadLibrary pointer, RUNTIME
	/*0x04:*/ 0x90, 0x90, 0x90, 0x90,			   // 8x nops to fix disassembly of VS
	/*0x0B:*/ 0x83, 0xEC, 0x28,				       // sub    esp,0x28
	/*0x10:*/ 0xE8, 0x00, 0x00, 0x00, 0x00,        // call   8 <.text + 0x8>
	/*0x11:*/ 0x5B,								   // pop    ebx
	/*0x14:*/ 0x83, 0xC3, 0x04,					   // add    ebx,0x4
	/*0x17:*/ 0x8D, 0x4B, 0x1D,					   // lea    ecx,[ebx + (40h - 23h)]
	/*0x1C:*/ 0xE8, 0x00, 0x00, 0x00, 0x00,        // call   14 <.text + 0x14>
	/*0x1D:*/ 0x5B,								   // pop    ebx
	/*0x20:*/ 0x83, 0xC3, 0x04,					   // add    ebx,0x4
	/*0x23:*/ 0xFF, 0x53, 0xDB,					   // call   DWORD PTR[ebx-25h]
	/*0x25:*/ 0x31, 0xC9,						   // xor    ecx,ecx
	/*0x2B:*/ 0x81, 0xCA, 0xFF, 0xFF, 0xFF, 0xFF,  // or	 edx,0xffffffff
	/*0x2D:*/ 0x85, 0xC0,						   // test   eax,eax
	/*0x30:*/ 0x0F, 0x44, 0xCA,					   // cmove  ecx,edx
	/*0x32:*/ 0x89, 0xC8,						   // mov    eax,ecx
	/*0x35:*/ 0x83, 0xC4, 0x28,					   // add    esp,0x28
	/*0x36:*/ 0xC3,								   // ret
	/*0x3B:*/ 0x90, 0x90, 0x90, 0x90, 0x90	       // 5x nop for alignment
	/*0x40:*/									   // String: "MyDll.dll"
};
#endif

bool InjectDll(const HANDLE hProcess, const LPCTSTR lpFileName, PVOID pfnLoadLibrary)
{
	auto ret = false;
	PVOID lp_shellcode_remote = nullptr;
	HANDLE h_remote_thread = nullptr;

	for (;;)
	{
		//allocate remote storage
		const DWORD lp_file_name_size = (wcslen(lpFileName) + 1) * sizeof(wchar_t);
		const DWORD lp_shellcode_size = sizeof(g_shellcode) + lp_file_name_size;
		lp_shellcode_remote = VirtualAllocEx(hProcess, nullptr,
			lp_shellcode_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (nullptr == lp_shellcode_remote)
		{
			printf("VirtualAllocEx returns NULL \n");
			break;
		}

		//fill remote storage with actual shellcode
		SIZE_T bytes_written;
		auto res = WriteProcessMemory(hProcess, lp_shellcode_remote,
			g_shellcode, sizeof(g_shellcode), &bytes_written);
		if (false == res)
		{
			printf("WriteProcessMemory failed with %d \n", GetLastError());
			break;
		}

		//fill remote storage with string
		res = WriteProcessMemory(hProcess, RVA_TO_VA(PVOID, lp_shellcode_remote, sizeof(g_shellcode)),
			lpFileName, lp_file_name_size, &bytes_written);
		if (false == res)
		{
			printf("WriteProcessMemory failed with %d \n", GetLastError());
			break;
		}

		//adjust pfnLoadLibrary
		const DWORD patched_pointer_rva = 0x00;
		auto patched_pointer_value = ULONG_PTR(pfnLoadLibrary);
		WriteRemoteDataType<ULONG_PTR>(hProcess,
			RVA_TO_VA(ULONG_PTR, lp_shellcode_remote, patched_pointer_rva),
			&patched_pointer_value);

		DWORD tid;
		//in case of problems try MyLoadLibrary if this is actually current process
		h_remote_thread = CreateRemoteThread(hProcess,
			nullptr, 0, LPTHREAD_START_ROUTINE(RVA_TO_VA(ULONG_PTR, lp_shellcode_remote, 0x10)),
			lp_shellcode_remote,
			0, &tid);
		if (nullptr == h_remote_thread)
		{
			printf("CreateRemoteThread failed with %d \n", GetLastError());
			break;
		}

		//wait for MyDll initialization
		WaitForSingleObject(h_remote_thread, INFINITE);

		DWORD exit_code = 0xDEADFACE;
		GetExitCodeThread(h_remote_thread, &exit_code);
		printf("GetExitCodeThread returns %x", exit_code);

		ret = true;
		break;
	}

	if (!ret)
	{
		if (lp_shellcode_remote) SIC_MARK;
		//TODO call VirtualFree(...)
	}

	if (h_remote_thread) CloseHandle(h_remote_thread);
	return ret;
}

//returns entry point in remote process
ULONG_PTR GetEnryPoint(const HANDLE hProcess)
{
	const auto h_process = hProcess;

	process_basic_information pbi;
	memset(&pbi, 0, sizeof(pbi));
	DWORD retlen = 0;

	auto status = ZwQueryInformationProcess(
		h_process,
		0,
		&pbi,
		sizeof(pbi),
		&retlen);

	const auto p_local_peb = REMOTE(peb, ULONG_PTR(pbi.peb_base_address));

	printf("\n");
	printf("from PEB: %p and %p\n", p_local_peb->reserved3[0], p_local_peb->reserved3[1]);

	const auto peb_remote_image_base = ULONG_PTR(p_local_peb->reserved3[1]); // TODO x64 POC only
	const auto p_remote_entry_point = FindEntryPoint2(h_process, peb_remote_image_base);
	return p_remote_entry_point;
}

//returns address of LoadLibraryA in remote process
ULONG_PTR GetRemoteLoadLibraryA(const HANDLE hProcess)
{
	const auto h_process = hProcess;
	DWORD n_modules;
	const auto ph_modules = GetRemoteModules(h_process, &n_modules);
	export_context my_context;
	my_context.module_name = "KERNEL32.dll";
	my_context.function_name = "LoadLibraryW";
	my_context.remote_function_address = 0;
	RemoteModuleWorker(h_process, ph_modules, n_modules, FindExport, &my_context);
	printf("kernel32!LoadLibraryA is at %llu \n", my_context.remote_function_address);

	return my_context.remote_function_address;
}

#if 0
bool FindEntryPoint(REMOTE_ARGS_DEFS, const PVOID context)
{
	bool is64;
	const auto p_local_pe_header = GetLocalPeHeader(REMOTE_ARGS_CALL, &is64);
	ULONG_PTR p_remote_entry_point;
	const auto my_context = pentry_point_context(context);

	if (is64)
	{
		const auto p_local_pe_header2 = PIMAGE_NT_HEADERS64(p_local_pe_header);
		p_remote_entry_point = RVA_TO_REMOTE_VA(PVOID,
			p_local_pe_header2->OptionalHeader.AddressOfEntryPoint);
	}
	else
	{
		const auto p_local_pe_header2 = PIMAGE_NT_HEADERS32(p_local_pe_header);
		p_remote_entry_point = RVA_TO_REMOTE_VA(PVOID,
			p_local_pe_header2->OptionalHeader.AddressOfEntryPoint);
	}
	free(p_local_pe_header);

	my_context->remote_remote_entry_point = p_remote_entry_point;

	return false;
}
#endif

ULONG_PTR FindEntryPoint2(REMOTE_ARGS_DEFS)
{
	bool is64;
	const auto p_local_pe_header = GetLocalPeHeader(REMOTE_ARGS_CALL, &is64);
	ULONG_PTR p_remote_entry_point;

	if (is64)
	{
		const auto p_local_pe_header2 = PIMAGE_NT_HEADERS64(p_local_pe_header);
		p_remote_entry_point = RVA_TO_REMOTE_VA(
			PVOID,
			p_local_pe_header2->OptionalHeader.AddressOfEntryPoint);
	}
	else
	{
		const auto p_local_pe_header2 = PIMAGE_NT_HEADERS32(p_local_pe_header);
		p_remote_entry_point = RVA_TO_REMOTE_VA(
			PVOID,
			p_local_pe_header2->OptionalHeader.AddressOfEntryPoint);
	}
	free(p_local_pe_header);

	return p_remote_entry_point;
}

void spawnSuspendedProcess(const LPCWSTR path, STARTUPINFO* startup_info, PROCESS_INFORMATION* proc_info) {
	if(!PathFileExistsW(path)) {
		throw "Failed to open binary file: file don't exist";
	}

	int status = CreateProcess(path, nullptr, nullptr, nullptr, true, CREATE_SUSPENDED, nullptr, nullptr, startup_info, proc_info);
	if (status != 1) {
		throw "Failed to spawn suspended process";
	}

	Sleep(1000);
}

int _tmain(int argc, _TCHAR* argv[])
{
	const auto BINARY_PATH = L"C:\\Windows\\system32\\taskmgr.exe";
	const auto DLL_PATH = L"Z:\\work\\RE-S18-lab_130418\\x64\\Debug\\MyDll.dll";

	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION process_info;
	try {
		spawnSuspendedProcess(BINARY_PATH, &info, &process_info);
	}
	catch(const char* e) {
		std::cerr << e << std::endl;
		return 1;
	}
	
	//-------
	const auto h_process = process_info.hProcess;
	const auto p_remote_entry_point = GetEnryPoint(h_process);

	WORD orig_word;
	WORD pathed_word = 0xFEEB;
	ReadRemoteDataType<WORD>(h_process, p_remote_entry_point, &orig_word);
	WriteRemoteDataType<WORD>(h_process, p_remote_entry_point, &pathed_word);

	ResumeThread(process_info.hThread); //resumed pathed process
	Sleep(1000);

	const auto remote_load_library_address = GetRemoteLoadLibraryA(h_process);

	InjectDll(h_process, L"Z:\\work\\RE-S18-lab_130418\\x64\\DebugMyDll.dll", 
		PVOID(remote_load_library_address));
	Sleep(2000);

	auto status = ZwSuspendProcess(h_process);
	WriteRemoteDataType<WORD>(h_process, p_remote_entry_point, &orig_word);
	status = ZwResumeProcess(h_process);

	
	WaitForSingleObject(process_info.hProcess, INFINITE);
	CloseHandle(process_info.hProcess);
	CloseHandle(process_info.hThread);
	
	return 0;
}