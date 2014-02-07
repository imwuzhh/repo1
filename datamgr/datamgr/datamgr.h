/////////////////////////////////////////////////////////////////////////////
// TAR Folder Shell Extension
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2009 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// source file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

#ifndef __LIB_DATAMGR_H__
#define __LIB_DATAMGR_H__

struct Edoc2Context {
	wchar_t username      [32];
	wchar_t password      [32];
	wchar_t AccessToken   [128];
	wchar_t cachedir      [1024];
};

typedef struct tagTAR_ARCHIVE
{
   CComAutoCriticalSection csLock;               // Thread lock
   struct Edoc2Context context;
} TAR_ARCHIVE;

#ifdef __cplusplus
extern "C" {
#endif

HRESULT DMOpen(LPCWSTR pstrFilename, TAR_ARCHIVE** ppArchive);
HRESULT DMClose(TAR_ARCHIVE* pArchive);

HRESULT DMGetChildrenList(TAR_ARCHIVE* pArchive, LPCWSTR pwstrPath, WIN32_FIND_DATA ** aList, int * nListCount);

HRESULT DMRename(TAR_ARCHIVE* pArchive, LPCWSTR pstrFilename, LPCWSTR pstrNewName);
HRESULT DMDelete(TAR_ARCHIVE* pArchive, LPCWSTR pstrFilename);
HRESULT DMCreateFolder(TAR_ARCHIVE* pArchive, LPCWSTR pstrFilename);

HRESULT DMSetFileAttr(TAR_ARCHIVE* pArchive, LPCWSTR pstrFilename, DWORD dwAttributes);

HRESULT DMGetFileAttr(TAR_ARCHIVE* pArchive, LPCWSTR pstrFilename, WIN32_FIND_DATA* pData);
HRESULT DMReadFile(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename, LPBYTE* ppbBuffer, DWORD * dwFileSize);
HRESULT DMWriteFile(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename, const LPBYTE pbBuffer, DWORD dwFileSize, DWORD dwAttributes);

HRESULT DMMalloc(LPBYTE * ppBuffer, DWORD dwBufSize);
HRESULT DMRealloc(LPBYTE * ppBuffer, DWORD dwBufSize);
HRESULT DMFree(LPBYTE pBuffer);

#ifdef __cplusplus
}
#endif

#endif