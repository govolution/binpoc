#include <windows.h>
#include <stdio.h>

/*
 * cl hidewindow.c /link user32.lib
 * code from somewhere
 */

int main(int argc, char* argv[])
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	printf("Peek A Boo!\n");
	Sleep(2000);
	ShowWindow(GetConsoleWindow(), SW_SHOW);
	return 0;
}


