#pragma once

struct EnumParam {
	char * className;
	char * title;
	DWORD userdata;
	DWORD returnValue;
};

class Hunter
{
public:
	static HWND s_ChildWindow;
	static HWND s_ParentWindow;
public:
	Hunter(void);
	~Hunter(void);
public:

	static void ResizeChild(HWND hParent, HWND hView, HWND hChild);

	static void EmbedWindow(HWND hParent, HWND hView, HWND hChild);

	static void ReleaseChild(HWND hParent, HWND hChild);

	static HWND FindChildWindow(const char * classname, const char * title, DWORD userdata);

	static BOOL CALLBACK ChildWindowEnumProc(HWND hWnd, LPARAM lParam);

	static BOOL CALLBACK TopWindowEnumProc(HWND hWnd, LPARAM lParam);

	static void CALLBACK TimerProc(HWND hWnd, UINT, UINT_PTR, DWORD);
};
