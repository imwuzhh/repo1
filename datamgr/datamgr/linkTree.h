#pragma once

#define SELFFILENAME _T(".self")

enum LinkType{
	FolderLink = _T('D'),
	FileLink   = _T('F')
};

struct Link {
	DWORD dwId;
	LinkType dwType;
	DWORD dwVersion;
	DWORD dwFileSize;
};

class linkTree
{
public:
	linkTree(void);
	~linkTree(void);
	static BOOL ReadLink(const wchar_t * fullpath, Link * pLink, BOOL isFolder);
	static BOOL WriteLink(const wchar_t * fullpath,const Link * pLink, BOOL isFolder);
};
