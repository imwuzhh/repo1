#pragma once
#include "Proto.h"

class JsonImpl : public Proto
{
public:
    JsonImpl();
    ~JsonImpl();
public:
    BOOL Login(TAR_ARCHIVE * pArchive);

    BOOL GetTopPublic(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPublic);

    BOOL GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPersonal);

    BOOL GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<VFS_FIND_DATA> & childFolders);

    BOOL GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<VFS_FIND_DATA> & childFiles);

    BOOL GetDocInfo(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<std::wstring> & columns, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount);

    BOOL GetPagedRecycleItems(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount);

    BOOL GetPagedSearchResults(TAR_ARCHIVE * pArchive, const std::wstring & query, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount);

    BOOL DeleteItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL isFolder);

    BOOL RenameItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * newName, BOOL isFolder);

    BOOL CreateFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * folderName, RemoteId * retId);

    BOOL DownloadFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath);

    BOOL UploadFile(TAR_ARCHIVE * pArchive, const RemoteId & parentFolderId, const wchar_t * localPath, const wchar_t * faceName);

    BOOL DownloadFolder(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath);

    BOOL UploadFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentFolderId, const wchar_t * localPath);

    BOOL Select(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL selected, BOOL isFolder);

    BOOL GetColumnInfo(TAR_ARCHIVE * pArchive, const RemoteId & viewId, wchar_t * pColumnInfo, int maxcch);

    BOOL PreviewFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId);

    BOOL OnShellViewCreated(TAR_ARCHIVE * pArchive, HWND shellViewWnd);

    BOOL OnShellViewSized(TAR_ARCHIVE * pArchive, HWND shellViewWnd);

    BOOL OnShellViewRefreshed(TAR_ARCHIVE  * pArchive, HWND shellViewWnd);

    BOOL OnShellViewClosing(TAR_ARCHIVE  * pArchive, HWND shellViewWnd);

    BOOL FileExists(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * childName, VFS_FIND_DATA & childInfo);

    BOOL FolderExists(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * childName, VFS_FIND_DATA & childInfo);

    BOOL FindChild(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * childName, VFS_FIND_DATA & childInfo);

    BOOL CheckMenu(TAR_ARCHIVE * pArchive, std::wstring & idlist, MenuType * selectedMenuItems);

    BOOL LockFile(TAR_ARCHIVE * pArchive, RemoteId id, BOOL toLock);

    BOOL InternalLink(TAR_ARCHIVE * pArchive, LPCWSTR idlist);

    BOOL ShareFile(TAR_ARCHIVE * pArchive, LPCWSTR idlist);

    BOOL ExtEditFile(TAR_ARCHIVE * pArchive, RemoteId id);

    BOOL DistributeFile(TAR_ARCHIVE * pArchive, LPCWSTR idlist);

    BOOL ViewLog(TAR_ARCHIVE * pArchive, LPCWSTR idlist);

    BOOL HistoryVersion(TAR_ARCHIVE * pArchive, RemoteId id);

    BOOL SelectItems(TAR_ARCHIVE * pArchive, LPCWSTR itemIds);

    BOOL CheckToken(TAR_ARCHIVE * pArchive, LPCWSTR token);
};
