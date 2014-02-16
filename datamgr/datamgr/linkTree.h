#pragma once

struct Link {
	DWORD dwId;
	DWORD dwVersion;
	DWORD dwFileSize;
};

class linkTree
{
public:
	linkTree(void);
	~linkTree(void);
	static BOOL ReadLink(const wchar_t * fullpath, Link * pLink);
	static BOOL WriteLink(const wchar_t * fullpath,const Link * pLink);
};
