/*
# wine gcc getpidbyname.c -lpsapi
*/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>

int main (int argc, char **argv)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		return 1;
	}

	cProcesses = cbNeeded / sizeof(DWORD);

	for ( i = 0; i < cProcesses; i++ )
	{
		if( aProcesses[i] != 0 )
		{
			DWORD processID = aProcesses[i];
			TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

			HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
					PROCESS_VM_READ,
					FALSE, processID );

			if (NULL != hProcess )
			{
				HMODULE hMod;
				DWORD cbNeeded;

				if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
							&cbNeeded) )
				{
					GetModuleBaseName( hProcess, hMod, szProcessName, 
							sizeof(szProcessName)/sizeof(TCHAR) );
				}
			}


			if (strcmp(szProcessName,argv[1]) == 0)
			{
				printf("PID: %u\n", processID);
				break;
			}

			CloseHandle( hProcess );
		}
	}

	return 0;
}

