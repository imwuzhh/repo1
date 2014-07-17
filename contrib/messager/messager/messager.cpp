// messager.cpp : 定义控制台应用程序的入口点。
//

#pragma warning(disable: 4996)
#include <Windows.h>
#include <ShlObj.h>
#include <string>

#define ID_FILE_PREV                    16
#define ID_FILE_NEXT                    17
#define ID_FILE_SEARCH                  18
#define ID_FILE_GOTO                    19

struct EnumParam {
    char * className;
    char * title;
    DWORD userdata;
    DWORD returnValue;
};


BOOL CALLBACK ChildWindowEnumProc(HWND hWnd, LPARAM lParam)
{
    EnumParam * ep = (EnumParam *)lParam;

    char szClassName [100] = "";
    char szTitle [100] = "";
    GetClassNameA(hWnd, szClassName, 100);
    GetWindowTextA(hWnd, szTitle, 100);
    DWORD userdata = GetWindowLongPtrA(hWnd, GWLP_USERDATA);

    if (0){
        std::string debugstr = szClassName;
        debugstr += szTitle;
        debugstr += "\n";
        OutputDebugStringA(debugstr.c_str());
    }

    if (!stricmp(szTitle, ep->title) && !stricmp(szClassName, ep->className)
        && (ep->userdata && ep->userdata == userdata || ep->userdata == 0)){
            ep->returnValue = (DWORD)hWnd;
            return FALSE; // Found, quit.
    }
    return TRUE;
}

BOOL CALLBACK TopWindowEnumProc(HWND hWnd, LPARAM lParam)
{
    EnumParam * ep = (EnumParam *)lParam;

    char szClassName [100] = "";
    char szTitle [100] = "";
    GetClassNameA(hWnd, szClassName, 100);
    GetWindowTextA(hWnd, szTitle, 100);
    DWORD userdata = GetWindowLongPtrA(hWnd, GWLP_USERDATA);

    if (0){
        std::string debugstr = szClassName;
        debugstr += szTitle;
        debugstr += "\n";
        OutputDebugStringA(debugstr.c_str());
    }

    if (!stricmp(szTitle, ep->title) && !stricmp(szClassName, ep->className)
        && (ep->userdata && ep->userdata == userdata || ep->userdata == 0)){
            ep->returnValue = (DWORD)hWnd;
            return FALSE; // Found, quit.
    }

    // Go on to search its children.
    EnumChildWindows(hWnd, ChildWindowEnumProc, lParam);
    if (ep->returnValue){
        return FALSE;
    }

    return TRUE; // to continue enumerate
}

HWND FindChildWindow(const char * classname, const char * title, DWORD userdata)
{
    EnumParam ep = {0};
    ep.className = strdup(classname);
    ep.title = strdup(title);
    ep.userdata = userdata;
    ep.returnValue = 0ul;
    EnumWindows(TopWindowEnumProc, (LPARAM)&ep);
    free(ep.className); free(ep.title);
    return (HWND) ep.returnValue;
}

#define CountOf(arr) (sizeof(arr)/sizeof(arr[0]))

int main(int argc, char * argv[])
{
    char szBuffer [0x20] = "";
    COPYDATASTRUCT cds;
    cds.dwData = 0xED0CED0C;
    cds.cbData = sizeof(szBuffer);
    cds.lpData = szBuffer;

    printf("a->Prev, b->Next, c->Search, g->GotoPage\n");
    while (TRUE)
    {
        printf(">"); char c = getchar(); if (c == '\r' || c == '\n') continue;
        UINT cmd = ID_FILE_NEXT;

        HWND hMsgWnd = FindChildWindow("SHELLDLL_DefView", "ShellView", 0xED0CED0C);
        if (hMsgWnd == NULL) return 0;

        switch (c)
        {
            case 'a': cmd = ID_FILE_PREV; strcpy_s(szBuffer, CountOf(szBuffer), "PrevPage"); break;
			case 'b': cmd = ID_FILE_NEXT; strcpy_s(szBuffer, CountOf(szBuffer), "NextPage"); break;
			case 'g': cmd = ID_FILE_GOTO; strcpy_s(szBuffer, CountOf(szBuffer), "GotoPage://2"); break;
            case 'c': cmd = ID_FILE_SEARCH;    strcpy_s(szBuffer, CountOf(szBuffer), "Search://hello"); break;
            default: break;
        }
        SendMessage(hMsgWnd, WM_COPYDATA, (WPARAM)GetConsoleWindow(), (LPARAM)&cds);
    }
	return 0;
}

