#include <Windows.h>
#include <tchar.h>
#include "linkTree.h"
#include <stdio.h>
#include <iostream>

linkTree::linkTree(void)
{
}

linkTree::~linkTree(void)
{
}

BOOL linkTree::ReadLink(const wchar_t * fullpath, Link * pLink, BOOL isFolder){
	std::wstring linkpath = fullpath;
	if (isFolder){
		linkpath += _T("\\"); linkpath += SELFFILENAME;
	}
	FILE * fin = _wfopen(linkpath.c_str(), _T("rb"));
	if (!fin) return FALSE;
	fread(pLink, 1, sizeof(*pLink), fin);
	fclose(fin);
	return TRUE;
}

BOOL linkTree::WriteLink(const wchar_t * fullpath, const Link * pLink, BOOL isFolder){
	std::wstring linkpath = fullpath;
	if (isFolder) {
		linkpath += _T("\\"); linkpath += SELFFILENAME;
	}
	FILE * fout = _wfopen(linkpath.c_str(), _T("wb"));
	if (!fout) return FALSE;
	fwrite(pLink, 1, sizeof(*pLink), fout);
	fclose(fout);
	return TRUE;
}
