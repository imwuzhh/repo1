#pragma once

class Proto
{
public:
     virtual BOOL Login(TAR_ARCHIVE * pArchive) = 0;

     virtual BOOL GetTopPublic(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPublic) = 0;

     virtual BOOL GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPersonal) = 0;
     
     virtual BOOL GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<VFS_FIND_DATA> & childFolders) = 0;
     
     virtual BOOL GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<VFS_FIND_DATA> & childFiles) = 0;
     
     virtual BOOL GetDocInfo(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<std::wstring> & columns, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount) = 0;

     virtual BOOL GetPagedRecycleItems(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount) = 0;

     virtual BOOL GetPagedSearchResults(TAR_ARCHIVE * pArchive, const std::wstring & query, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount) = 0;
     
     virtual BOOL DeleteItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL isFolder) = 0;
     
     virtual BOOL RenameItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * newName, BOOL isFolder) = 0;
     
     virtual BOOL CreateFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * folderName, RemoteId * retId) = 0;
     
     /**
     * Download remote item to local disk.
     * Parameters:
     * 1) pArchive, the session context info.
     * 2) itemId, item's id.
     * 3) localPath, if this is a directory, use the item's name for full path, otherwise, use this value as the destination full path.
     * Return:
     * True for successful, False for failure.
     */
     virtual BOOL DownloadFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath) = 0;
     
     /**
     * Upload a local file to remote server.
     * Parameters:
     * 1) pArchive, the session context info.
     * 2) parentFolderId, remote folder's id, in which local file will be uploaded.
     * 3) localPath, the file's full path in local disk;
     * 4) faceName, the display name, this is a chance for user to setup the display name of this uploaded item.
     * Return:
     * True for successful, False for failure.
     */
     virtual BOOL UploadFile(TAR_ARCHIVE * pArchive, const RemoteId & parentFolderId, const wchar_t * localPath, const wchar_t * faceName) = 0;
     
     /**
     * Download remote item to local disk.
     * Parameters:
     * 1) pArchive, the session context info.
     * 2) itemId, item's id.
     * 3) localPath, this is a directory, and will be used as the parent of downloaded item.
     * Return:
     * True for successful, False for failure.
     */
     virtual BOOL DownloadFolder(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath) = 0;
     
     /**
     * Upload a local file to remote server.
     * Parameters:
     * 1) pArchive, the session context info.
     * 2) parentFolderId, remote folder's id, in which local file will be uploaded.
     * 3) localPath, the directory's full path in local disk;
     * Return:
     * True for successful, False for failure.
     */
     virtual BOOL UploadFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentFolderId, const wchar_t * localPath) = 0;
     
     virtual BOOL Select(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL selected, BOOL isFolder) = 0;
     
     virtual BOOL GetColumnInfo(TAR_ARCHIVE * pArchive, const RemoteId & viewId, wchar_t * pColumnInfo, int maxcch) = 0;

     virtual BOOL PreviewFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId) = 0;

     virtual BOOL OnShellViewCreated(TAR_ARCHIVE * pArchive, HWND shellViewWnd) = 0;

     virtual BOOL OnShellViewSized(TAR_ARCHIVE * pArchive, HWND shellViewWnd) = 0;

     virtual BOOL OnShellViewRefreshed(TAR_ARCHIVE  * pArchive, HWND shellViewWnd) = 0;

     virtual BOOL OnShellViewClosing(TAR_ARCHIVE  * pArchive, HWND shellViewWnd) = 0;

     virtual BOOL FileExists(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * childName, VFS_FIND_DATA & childInfo) = 0;

     virtual BOOL FolderExists(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * childName, VFS_FIND_DATA & childInfo) = 0;

     /**
     * Find child folder/file with parent id and child's name.
     * Parameters:
     * 1) pArchive: context.
     * 2) parentId: parent's id.
     * 3) childName: child file/folder's name.
     * 4) childInfo: return info of found child.
     * Notice:
     * if this api works, then the above two FileExists()/FolderExists() should be deprecated.
     */
     virtual BOOL FindChild(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * childName, VFS_FIND_DATA & childInfo) = 0;

     virtual BOOL CheckMenu(TAR_ARCHIVE * pArchive, std::wstring & idlist, MenuType * selectedMenuItems) = 0;

     virtual BOOL LockFile(TAR_ARCHIVE * pArchive, RemoteId id, BOOL toLock) = 0;

     virtual BOOL InternalLink(TAR_ARCHIVE * pArchive, LPCWSTR idlist) = 0;

     virtual BOOL ShareFile(TAR_ARCHIVE * pArchive, LPCWSTR idlist) = 0;

     virtual BOOL ExtEditFile(TAR_ARCHIVE * pArchive, RemoteId id) = 0;

     virtual BOOL DistributeFile(TAR_ARCHIVE * pArchive, LPCWSTR idlist) = 0;

     virtual BOOL ViewLog(TAR_ARCHIVE * pArchive, LPCWSTR idlist) = 0;

     virtual BOOL HistoryVersion(TAR_ARCHIVE * pArchive, RemoteId id) = 0;

	 /**
	 * Notify json proxy about shell items is selected or selection cancelled.
	 * Parameters:
	 * 1) pArchive: context
	 * 2) itemIds: selected items' id, with format `id1:flag1,id2:flag2,...', if directory selected, flag is 1, otherwise flag is 0.
	 */
     virtual BOOL SelectItems(TAR_ARCHIVE * pArchive, LPCWSTR itemIds) = 0;

     /**
     * Function: check available of old token. 
     * Parameters:
     * 1) pArchive, session context.
     * 2) token, old token.
     * Return:
     * if available return TRUE, otherwise return FALSE.
     */
     virtual BOOL CheckToken(TAR_ARCHIVE * pArchive, LPCWSTR token) = 0;

     virtual BOOL Recover(TAR_ARCHIVE * pArchive, LPCWSTR itemIds) = 0;

     virtual BOOL ClearRecycleBin(TAR_ARCHIVE * pArchive) = 0;

     virtual BOOL Move(TAR_ARCHIVE * pArchive, const RemoteId & srcId, const RemoteId & destId, BOOL fRemoveSource) = 0;
};
