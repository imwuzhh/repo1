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


void Hunter::ResizeChild(HWND hParent, HWND hChild){
	RECT ParentRect;
	GetClientRect(hParent, &ParentRect);
	SetWindowPos(hChild, HWND_TOP, (ParentRect.right - ParentRect.left)*0.75, 0, (ParentRect.right - ParentRect.left)/4, (ParentRect.bottom - ParentRect.top), 0);
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

void Hunter::EmbedWindow(HWND hParent, HWND hChild){
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

	ResizeChild(hParent, hChild);
}

void CALLBACK Hunter::TimerProc(HWND hWnd, UINT, UINT_PTR, DWORD){
	// HarryWu, 2014.2.27
	// This child window is a demo.
	// You should always Know where it is.
	// And, this child window is under your full control.
	// so that, you can ignore WM_DESTROY Message.
	HWND hDlg = FindChildWindow("Chrome_WidgetWin_0", "Эјвз", 0x00000000);
	if (hDlg == NULL) return ;
	s_ChildWindow = hDlg;

	HWND hParent = FindChildWindow("SHELLDLL_DefView", "ShellView", 0x00000000 /*SHOULDBE 0xED0CED0C*/);
	hParent = GetParent(GetParent(hParent));

	if (s_ParentWindow == hParent && s_ParentWindow != NULL
		&& GetParent(s_ChildWindow) == s_ParentWindow){
		// If Already embedded, resize it, 
		ResizeChild(s_ParentWindow, s_ChildWindow);
		return ;
	}

	s_ParentWindow = hParent;
	EmbedWindow(hParent, hDlg);
}

BOOL CALLBACK Hunter::ChildWindowEnumProc(HWND hWnd, LPARAM lParam)
{
	EnumParam * ep = (EnumParam *)lParam;

	char szClassName [100] = "";
	char szTitle [100] = "";
	GetClassNameA(hWnd, szClassName, 100);
	GetWindowTextA(hWnd, szTitle, 100);
	DWORD userdata = GetWindowLongA(hWnd, GWL_USERDATA);
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
	DWORD userdata = GetWindowLongA(hWnd, GWL_USERDATA);
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
