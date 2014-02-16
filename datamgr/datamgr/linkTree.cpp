#include <Windows.h>
#include <tchar.h>
#include "linkTree.h"
#include <stdio.h>

linkTree::linkTree(void)
{
}

linkTree::~linkTree(void)
{
}

BOOL linkTree::ReadLink(const wchar_t * fullpath, Link * pLink){
	FILE * fin = _wfopen(fullpath, _T("rb"));
	if (!fin) return FALSE;
	fread(pLink, 1, sizeof(*pLink), fin);
	return TRUE;
}

BOOL linkTree::WriteLink(const wchar_t * fullpath, const Link * pLink){
	FILE * fout = _wfopen(fullpath, _T("wb"));
	if (!fout) return FALSE;
	fwrite(pLink, 1, sizeof(*pLink), fout);
	fclose(fout);
	return TRUE;
}
