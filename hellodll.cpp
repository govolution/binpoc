// http://www.adp-gmbh.ch/win/programming/rundll32.html
// wine g++ -o hellodll.dll hellodll.cpp -shared -mwindows
// test: rundll32 c:\path\to\dll\rdl.dll,rdl hello


#include <windows.h>

extern "C" __declspec (dllexport) void __cdecl rdl (
   HWND hwnd,        // handle to owner window
   HINSTANCE hinst,  // instance handle for the DLL
   LPTSTR lpCmdLine, // string the DLL will parse
   int nCmdShow      // show state
)
{
  ::MessageBox(0,lpCmdLine,0,0);
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved)
{
    return TRUE;
}

