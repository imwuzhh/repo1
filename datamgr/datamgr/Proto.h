#pragma once

class Proto
{
public:
     virtual BOOL Login(TAR_ARCHIVE * pArchive) = 0;

     virtual BOOL GetTopPublic(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPublic) = 0;

     virtual BOOL GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPersonal) = 0;
     
     virtual BOOL GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<VFS_FIND_DATA> & childFolders) = 0;
     
     virtual BOOL GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<VFS_FIND_DATA> & childFiles) = 0;
     
     virtual BOOL GetChildFolderAndFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount) = 0;
     
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

     virtual BOOL OnShellViweSized(TAR_ARCHIVE * pArchive, HWND shellViewWnd) = 0;

     virtual BOOL OnShellViewRefreshed(TAR_ARCHIVE  * pArchive, HWND shellViewWnd) = 0;
};
