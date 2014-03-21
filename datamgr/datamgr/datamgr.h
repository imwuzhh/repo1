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

struct Edoc2Context;

struct TAR_ARCHIVE
{
   CComAutoCriticalSection csLock;               // Thread lock
   struct Edoc2Context * context;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
* Initialize workground, allocate resources.
* Parameters:
* None
*/
HRESULT DMInit();

/**
* Cleanup resources
* Parameters:
* None
*/
HRESULT DMCleanup();

/**
* Check that Use Http directly or Json proxy to communicate with remote.
* Parameters:
* None
*/
BOOL DMHttpIsEnable();

/**
* Create an context for consequent api invoke.
* Parameters:
* [pstrFileName] reserved
* [ppArchive] pointer to hold the conext.   
*/
HRESULT DMOpen(LPCWSTR pstrFilename, TAR_ARCHIVE** ppArchive);

/**
* Destroy the context.
* Parameters:
* [pArchive] the context handle.
*/
HRESULT DMClose(TAR_ARCHIVE* pArchive);

/**
* Enumerate the children of specified id.
* Parameters:
* [pArchive] the context.
* [dwId] the folder's id used to enumerate its childeren.
* [aList] pointer to hold the array of children, it's DMAllocat-ed internally, so free it with DMFree().
* [nListCount] pointer to get number of the total children.
*/
HRESULT DMGetChildrenList(TAR_ARCHIVE* pArchive, RemoteId dwId, RFS_FIND_DATA ** aList, int * nListCount);

/**
* Rename an item
* Parameters:
* [pArchive] the context.
* [itemId] the id of the item, whose name will be renamed.
* [pstrNewName] the new name.
* [isFolder] flag, to specify if the item is a folder, as, file and folder use different api.
*/
HRESULT DMRename(TAR_ARCHIVE* pArchive, RemoteId itemId, LPCWSTR pstrNewName, BOOL isFolder);

/**
* Delete an item
* Parameters:
* [pArchive] the context.
* [itemId] id of item, which will be deleted.
* [isFolder] flag to specify if item is a folder, as file and folder use different api.
*/
HRESULT DMDelete(TAR_ARCHIVE* pArchive, RemoteId itemId, BOOL isFolder);

/**
* Create new folder
* Parameters:
* [pArchive] the context.
* [parentId] id of folder, in which, the new folder will be created.
* [pstrFilename] the folder's name.
* [pWfd] structure used to hold the brand-new id of the newly created folder.
*/
HRESULT DMCreateFolder(TAR_ARCHIVE* pArchive, RemoteId parentId, LPCWSTR pstrFilename, RFS_FIND_DATA * pWfd);

/**
* Download a file from remote, use curl and save it to local position specified by pwstrFilename.
* Parameters:
* [pArchive] the context.
* [itemId] file id to download.
* [pwstrFilename] local position to save file.
* [ppbBuffer] reserved.
* [dwFileSize] reserved.
*/
HRESULT DMReadFile(TAR_ARCHIVE* pArchive, RemoteId itemId, LPCWSTR pwstrFilename, LPBYTE* ppbBuffer, DWORD * dwFileSize);

/**
* Upload a file 
* Parameters:
* [pArchive] the context.
* [parentId] folder's id, in which local file will be uploaded.
* [pwstrFilename] local file path.
* [pbBuffer] reserved.
* [dwFileSize] reserved.
* [dwAttributes] reserved.
*/
HRESULT DMWriteFile(TAR_ARCHIVE* pArchive, RemoteId parentId, LPCWSTR pwstrFilename, const LPBYTE pbBuffer, DWORD dwFileSize, DWORD dwAttributes);

/**
* Post a download task to external tool, by specifying the source path and the destination id.
* Parameters:
* [pArchive] the context.
* [pwstrLocalDir] local full path, a directory in local disk.
* [itemId] the remote id which will be downloaed, folder/file will be both OK, if a folder, tool will enumerate children by itself.
* [removeSource] flag, True when cut, False when copy.
*/
HRESULT DMDownload(TAR_ARCHIVE * pArchive, LPCWSTR pwstrLocalDir, RemoteId itemId, BOOL removeSource);

/**
* Upload a file or a folder to remote folder.
* Parameters:
* [pArchive] the context.
* [pwstrLocalPath] fullpath on local disk of file or folder, if it's folder, tool will enumerate children by itself.
* [viewId] folder's id, in which file/folder will be uploaded.
* [removeSource] flag, True when cut, False when copy.
*/
HRESULT DMUpload(TAR_ARCHIVE * pArchive, LPCWSTR pwstrLocalPath, RemoteId viewId, BOOL removeSource);

/**
* Notify the tool when an item in shell is being seleced.
* Parameters:
* [pArchive] the context.
* [itemId] the selected item's id.
* [selected] flag, True, selected; False, Cancel selection.
* [isFolder] flag, True, the item is a folder; False, the item is a file.
*/
HRESULT DMSelect(TAR_ARCHIVE * pArchive, RemoteId itemId, BOOL selected, BOOL isFolder);

/**
* Retrieve the custom column info from remote.
* Parameters:
* [pArchive] the context.
* [viewId] the folder's id, which has custome columns.
* [pwstrColumnList] string, used to hold the custom columns, colon separated, e.g.: "Col1;Col2;Col3;"
* [maxcch] buffer's length of pwstrColumnList
*/
HRESULT DMGetCustomColumns(TAR_ARCHIVE * pArchive, RemoteId viewId, LPWSTR pwstrColumnList, int maxcch);

/**
* Malloc memory from libdatamgr, as we use /MT to build, if you get memory from this module, free it with DMFree().
* Parameters:
* [ppBuffer] out pointer to hold the buffer position allated.
* [dwBufSize] request buffer size.
*/
HRESULT DMMalloc(LPBYTE * ppBuffer, DWORD dwBufSize);

/**
* Reallocate memory with previous memory.
* Parameters:
* [ppBufer] out pointer to hold the reallocated buffer.
* [dwBufSize] new request size.
*/
HRESULT DMRealloc(LPBYTE * ppBuffer, DWORD dwBufSize);

/**
* Free memory
* Parameters:
* [pBuffer] buffer pointer to be freed.
*/
HRESULT DMFree(LPBYTE pBuffer);

#ifdef __cplusplus
}
#endif

#endif