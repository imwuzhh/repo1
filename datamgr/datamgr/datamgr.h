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

enum {
	VdriveCat = (0),
	VdriveId  = (0),
	PublicCat = (1),
	PublicId  = (0),
	PersonCat = (2),
	PersonId  = (0),
	RecycleCat= (3),
	RecycleId = (0),
	SearchCat = (4),
	SearchId  = (0),
};

struct RemoteId {
	DWORD category;
	DWORD id;
};

typedef struct _RFS_FIND_DATAA {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	CHAR   cFileName[ MAX_PATH ];
	CHAR   cAlternateFileName[ 14 ];
#ifdef _MAC
	DWORD dwFileType;
	DWORD dwCreatorType;
	WORD  wFinderFlags;
#endif
	RemoteId dwId;
	DWORD dwVersion;
	DWORD dwAttributes;
	DWORD dwPage;
	DWORD dwTotalPage;
	unsigned char md5 [16];
} RFS_FIND_DATAA, *PRFS_FIND_DATAA, *LPRFS_FIND_DATAA;
typedef struct _RFS_FIND_DATAW {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	WCHAR  cFileName[ MAX_PATH ];
	WCHAR  cAlternateFileName[ 14 ];
#ifdef _MAC
	DWORD dwFileType;
	DWORD dwCreatorType;
	WORD  wFinderFlags;
#endif
	RemoteId dwId;
	DWORD dwVersion;
	DWORD dwAttributes;
	DWORD dwPage;
	DWORD dwTotalPage;
	unsigned char md5 [16];
} RFS_FIND_DATAW, *PRFS_FIND_DATAW, *LPRFS_FIND_DATAW;
#ifdef UNICODE
typedef RFS_FIND_DATAW RFS_FIND_DATA;
typedef PRFS_FIND_DATAW PRFS_FIND_DATA;
typedef LPRFS_FIND_DATAW LPRFS_FIND_DATA;
#else
typedef RFS_FIND_DATAA RFS_FIND_DATA;
typedef PRFS_FIND_DATAA PRFS_FIND_DATA;
typedef LPRFS_FIND_DATAA LPRFS_FIND_DATA;
#endif // UNICODE

struct Edoc2Context {
	DWORD   dwUserId;
	wchar_t localeName    [32];
	wchar_t service       [128];
	wchar_t username      [32];
	wchar_t password      [32];
	wchar_t AccessToken   [128];
	wchar_t cachedir      [1024];
};

struct TAR_ARCHIVE
{
   CComAutoCriticalSection csLock;               // Thread lock
   struct Edoc2Context * context;
};

#ifdef __cplusplus
extern "C" {
#endif

HRESULT DMInit();
HRESULT DMCleanup();

HRESULT DMOpen(LPCWSTR pstrFilename, TAR_ARCHIVE** ppArchive);
HRESULT DMClose(TAR_ARCHIVE* pArchive);

HRESULT DMGetChildrenList(TAR_ARCHIVE* pArchive, RemoteId dwId, RFS_FIND_DATA ** aList, int * nListCount);

HRESULT DMRename(TAR_ARCHIVE* pArchive, RemoteId itemId, LPCWSTR pstrNewName);
HRESULT DMDelete(TAR_ARCHIVE* pArchive, RemoteId itemId);
HRESULT DMCreateFolder(TAR_ARCHIVE* pArchive, RemoteId parentId, LPCWSTR pstrFilename, RFS_FIND_DATA * pWfd);

HRESULT DMUpload(TAR_ARCHIVE* pArchive, RemoteId parentId, LPCWSTR localRes);
HRESULT DMDownload(TAR_ARCHIVE* pArchive, RemoteId itemId, LPCWSTR localTarget);

HRESULT DMSetFileAttr(TAR_ARCHIVE* pArchive, LPCWSTR pstrFilename, DWORD dwAttributes);

HRESULT DMReadFile(TAR_ARCHIVE* pArchive, RemoteId itemId, LPCWSTR pwstrFilename, LPBYTE* ppbBuffer, DWORD * dwFileSize);
HRESULT DMWriteFile(TAR_ARCHIVE* pArchive, RemoteId parentId, LPCWSTR pwstrFilename, const LPBYTE pbBuffer, DWORD dwFileSize, DWORD dwAttributes);

HRESULT DMMalloc(LPBYTE * ppBuffer, DWORD dwBufSize);
HRESULT DMRealloc(LPBYTE * ppBuffer, DWORD dwBufSize);
HRESULT DMFree(LPBYTE pBuffer);

#ifdef __cplusplus
}
#endif

#endif