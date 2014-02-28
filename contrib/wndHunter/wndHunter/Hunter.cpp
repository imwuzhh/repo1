#include "StdAfx.h"
#include "Hunter.h"
#include <string>
#pragma warning(disable: 4996 4244)

HWND Hunter::s_ChildWindow = NULL;
HWND Hunter::s_ParentWindow = NULL;

Hunter::Hunter(void)
{
}

Hunter::~Hunter(void)
{
}

HWND Hunter::FindChildWindow(const char * classname, const char * title, DWORD userdata)
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


void Hunter::ResizeChild(HWND hParent, HWND hView, HWND hChild){
	RECT ViewRect, wvRect;
	GetClientRect(hView, &ViewRect);
	GetWindowRect(hView, &wvRect);
	RECT ParentRect, wpRect;
	GetClientRect(hParent, &ParentRect);
	GetWindowRect(hParent, &wpRect);
	SetWindowPos(hChild, HWND_TOP
		, (ParentRect.right - ParentRect.left)*0.75
		, (wvRect.top - wpRect.top)
		, (ParentRect.right - ParentRect.left)/4
		, (ViewRect.bottom - ViewRect.top), 0);
}

void Hunter::ReleaseChild(HWND hParent, HWND hChild){

	SetWindowLongA(hChild, GWL_STYLE, WS_CAPTION
		| WS_POPUP
		| WS_VISIBLE
		| WS_CLIPSIBLINGS
		| WS_CLIPCHILDREN
		| WS_SYSMENU
		| WS_THICKFRAME
		| WS_MINIMIZEBOX | WS_MAXIMIZEBOX
		);

	SetWindowLongA(hChild, GWL_EXSTYLE, WS_EX_LEFT
		| WS_EX_LTRREADING
		| WS_EX_RIGHTSCROLLBAR
		| WS_EX_WINDOWEDGE);

	SetParent(hChild, NULL);
}

void Hunter::EmbedWindow(HWND hParent, HWND hView, HWND hChild){
	SetWindowLongA(hChild, GWL_STYLE, 0
		| WS_CHILDWINDOW 
		| WS_CLIPSIBLINGS 
		| WS_VISIBLE
		//| WS_BORDER 
		//| WS_THICKFRAME
		| WS_CLIPCHILDREN
		);

	SetWindowLongA(hChild, GWL_EXSTYLE, 0
		| WS_EX_LEFT
		| WS_EX_LTRREADING
		| WS_EX_RIGHTSCROLLBAR);

	SetParent(hChild, hParent);

	DWORD parentStyle = GetWindowLongA(hParent, GWL_STYLE);
	parentStyle |= WS_CLIPCHILDREN;
	SetWindowLongA(hParent, GWL_STYLE, parentStyle);

	ResizeChild(hParent, hView, hChild);
}

void CALLBACK Hunter::TimerProc(HWND hWnd, UINT, UINT_PTR, DWORD){
	// HarryWu, 2014.2.27
	// This child window is a demo.
	// You should always Know where it is.
	// And, this child window is under your full control.
	// so that, you can ignore WM_DESTROY Message.
	// ...
	// A story, i used chrome to test embed, 
	// but, 2014.2.28, chrome auto-updated to v33 from v27.
	// and on v33, these is not title for Chrome_WidgetWin_0,
	// it is very very accident event!
	//HWND hDlg = FindChildWindow("Chrome_WidgetWin_0", "Эјвз", 0x00000000);
	// And aslo, Windows ie7 diffs with ie11 on titile,
	// "Internet Explorer", "Windows Internet Explorer"
	// :(
	HWND hDlg = FindChildWindow("TabWindowClass", "Эјвз - Windows Internet Explorer", 0x00000000);
	if (hDlg == NULL) return ;
	s_ChildWindow = hDlg;

	HWND hView = FindChildWindow("SHELLDLL_DefView", "ShellView", 0x00000000 /*SHOULDBE 0xED0CED0C*/);
	if (hView == NULL) return ;

	s_ParentWindow = GetParent(GetParent(hView));

	if (GetParent(s_ChildWindow) == s_ParentWindow){
		// If Already embedded, resize it, 
		ResizeChild(s_ParentWindow, hView, s_ChildWindow);
		return ;
	}

	EmbedWindow(s_ParentWindow, hView, hDlg);
}

BOOL CALLBACK Hunter::ChildWindowEnumProc(HWND hWnd, LPARAM lParam)
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

BOOL CALLBACK Hunter::TopWindowEnumProc(HWND hWnd, LPARAM lParam)
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
