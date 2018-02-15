#include <Windows.h>

int main(int argc, char* argv[]) 
{
	HANDLE hModule = LoadLibrary(argv[1]);
	return 0;
}

