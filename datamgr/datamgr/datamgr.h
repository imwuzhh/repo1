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
	PublicCat = (0x00000001),
	PublicId  = (0),
	PersonCat = (0x00000002),
	PersonId  = (0),
	RecycleCat= (0x00000004),
	RecycleId = (~3),
	SearchCat = (0x00000008),
	SearchId  = (~4),
};

enum {
	MaxPageSize = 0x7ffff,
};

enum {
	SYSTEM_CMDID_PROPERTIES = 30996,
	SYSTEM_CMDID_DELETE    =  30994,
	SYSTEM_CMDID_RENAME    =  30995,
	SYSTEM_CMDID_CUT       =  31001,
};

struct RemoteId {
	DWORD category;
	DWORD id;
};

#if defined(_WIN64)
typedef unsigned long long MenuType;
#elif defined(_WIN32)
typedef unsigned __int64   MenuType;
#endif

#define MenuDef_OpenFile     (0x0000000000000001)
#define MenuDef_DownloadFile (0x0000000000000002)
#define MenuDef_NewFolder    (0x0000000000000004)
#define MenuDef_Properties   (0x0000000000000008)
#define MenuDef_Share        (0x0000000000000010)
#define MenuDef_Upload       (0x0000000000000020)
#define MenuDef_Preview      (0x0000000000000040)
#define MenuDef_Innerlink    (0x0000000000000080)
#define MenuDef_Distribute   (0x0000000000000100)
#define MenuDef_Lock         (0x0000000000000200)
#define MenuDef_Unlock       (0x0000000000000400)
#define MenuDef_OldVersion   (0x0000000000000800)
#define MenuDef_Viewlog      (0x0000000000001000)
#define MenuDef_ExtEdit      (0x0000000000002000)
#define MenuDef_Cut          (0x0000000000004000)
#define MenuDef_Delete       (0x0000000000008000)
#define MenuDef_Rename       (0x0000000000010000)
#define MenuDef_TrashClear	 (0x0000000000020000)
#define MenuDef_TrashRestore (0x0000000000040000)

#define MenuDef_AllRemoved   (0x0000000000000000)
#define MenuDef_AllSelected  (0xffffFFFFffffFFFF)

typedef struct _VFS_FIND_DATAW {
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
//////////////////////////////////////////////////
    RemoteId dwId;
    WCHAR versionStr [16];
    WCHAR creatorName[32];
} VFS_FIND_DATA, *PVFS_FIND_DATA, *LPVFS_FIND_DATA;

struct ColumnDef {
    wchar_t colName [100];
};

struct ViewSettings{
    int colCount;
    struct ColumnDef aColumns [100];
};

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
* Check that Use Http directly or Json proxy to communicate with remote.
* Parameters:
* None
*/
BOOL DMHttpIsEnable();

BOOL DMHttpTransferIsEnable();

BOOL DMFastCheckIsEnable();

BOOL DMHasRootChild(DWORD dwCat);

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
HRESULT DMGetRootChildren(TAR_ARCHIVE* pArchive, RemoteId dwId, VFS_FIND_DATA ** aList, int * nListCount);

HRESULT DMGetDocInfo(TAR_ARCHIVE* pArchive, RemoteId dwId, int PageSize, int PageNo, int * totalPage, ViewSettings * pVS, VFS_FIND_DATA ** aList, int * nListCount);

HRESULT DMSetupQuery(TAR_ARCHIVE * pArchive, const wchar_t * query, VFS_FIND_DATA * wfd);

/**
* Get Page size
* Parameters:
* 
*/
HRESULT DMGetPageSize(TAR_ARCHIVE * pArchive, DWORD * pdwPageSize);


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
HRESULT DMCreateFolder(TAR_ARCHIVE* pArchive, RemoteId parentId, LPCWSTR pstrFilename, VFS_FIND_DATA * pWfd);

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
HRESULT DMDownload(TAR_ARCHIVE * pArchive, LPCWSTR pwstrLocalDir, RemoteId itemId, BOOL isFolder, BOOL removeSource);

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
* Notify external assistance tool to preivew a file.
* Parameters:
* [pArchive] context handle.
* [itemId] file's id.
*/
HRESULT DMPreviewFile(TAR_ARCHIVE * pArchive, RemoteId itemId);

/**
* Notify about create of shell view window
* Parameters:
* [pArchive] context handle
* [shellViewWnd] window handle of shell view window
*/
HRESULT DMOnShellViewCreated(TAR_ARCHIVE * pArchive, HWND shellViewWnd);

/**
* Notify about refresh of shell view window
* Parameters:
* [pArchive] context handle
* [shellViewWnd] window handle of shell view window
*/
HRESULT DMOnShellViewRefreshed(TAR_ARCHIVE * pArchive, HWND shellViewWnd);

/**
* Notify about size change of shell view window
* Parameters:
* [pArchive] context handle
* [shellViewWnd] window handle of shell view window
*/
HRESULT DMOnShellViewSized(TAR_ARCHIVE * pArchive, HWND shellViewWnd);

/**
* Notify about size change of shell view window
* Parameters:
* [pArchive] context handle
* [shellViewWnd] window handle of shell view window
*/
HRESULT DMOnShellViewClosing(TAR_ARCHIVE * pArchive, HWND shellViewWnd);

/**
* Get Current Page Number
* Parameters:
* [pArchive] context handle
* [id] folder's id
* [retPageNo] pointer to retrieve current page number.
*/
HRESULT DMGetCurrentPageNumber(TAR_ARCHIVE * pArchive, RemoteId id, DWORD * retPageNo);

HRESULT DMSetPageNumber(TAR_ARCHIVE * pArchive, RemoteId id, DWORD dwNewPageNo);

HRESULT DMIncCurrentPageNumber(TAR_ARCHIVE * pArchive, RemoteId id);

HRESULT DMDecCurrentPageNumber(TAR_ARCHIVE * pArchive, RemoteId id);

HRESULT DMSetTotalPageNumber(TAR_ARCHIVE * pArchive, RemoteId id, DWORD totalPage);

HRESULT DMAddItemToDB(TAR_ARCHIVE * pArchive, RemoteId id, wchar_t * pwstrName, BOOL isFolder);

HRESULT DMLockFile(TAR_ARCHIVE * pArchive, RemoteId id, BOOL toLock);

HRESULT DMIsFileLocked(TAR_ARCHIVE * pArchive, RemoteId id, BOOL * isLocked);

HRESULT DMInternalLink(TAR_ARCHIVE * pArchive, LPCWSTR idlist);

HRESULT DMShareFile(TAR_ARCHIVE * pArchive, LPCWSTR idlist);

HRESULT DMExtEditFile(TAR_ARCHIVE * pArchive, RemoteId id);

HRESULT DMDistributeFile(TAR_ARCHIVE * pArchive, LPCWSTR idlist);

HRESULT DMViewLog(TAR_ARCHIVE * pArchive, LPCWSTR idlist);

HRESULT DMHistoryVersion(TAR_ARCHIVE * pArchive, RemoteId id);

HRESULT DMSelectItems(TAR_ARCHIVE * pArchive, LPCWSTR itemIds);

HRESULT DMFindChild(TAR_ARCHIVE * pArchive, RemoteId parentId, LPCWSTR childName, VFS_FIND_DATA * pInfo);

/**
* Select proper menus for given items specified by `idlist' parameter.
* Parameters:
* [pArchive] Context handle.
* [idlist]   items' id list, in format "type:id", such as "0:1998;1:123;", 0 for file type, 1 for folder type.
* [menudef]  selected value of menu items.
*/
HRESULT DMCheckMenu(TAR_ARCHIVE * pArchive, const wchar_t * idlist, MenuType * menudef);

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