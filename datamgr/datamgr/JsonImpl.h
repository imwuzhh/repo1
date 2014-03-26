#pragma once
#include "Proto.h"

class JsonImpl : public Proto
{
public:
    JsonImpl();
    ~JsonImpl();
public:
    BOOL Login(TAR_ARCHIVE * pArchive);

    BOOL GetTopPublic(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPublic);

    BOOL GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPersonal);

    BOOL GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFolders);

    BOOL GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFiles);

    BOOL GetChildFolderAndFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount);

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
};
