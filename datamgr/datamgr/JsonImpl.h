#pragma once
#include "Proto.h"

class JsonImpl
{
public:
    JsonImpl();
    ~JsonImpl();
public:
    static BOOL Login(TAR_ARCHIVE * pArchive);
    static BOOL GetTopPublic(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPublic);
    static BOOL GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPersonal);
    static BOOL GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFolders);
    static BOOL GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFiles);
    static BOOL DeleteItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL isFolder);
    static BOOL RenameItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * newName, BOOL isFolder);
    static BOOL CreateFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * folderName, RemoteId * retId);
    static BOOL Upload(TAR_ARCHIVE * pArchive, const RemoteId & viewId, const wchar_t * localPath);
    static BOOL Download(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath);
    static BOOL Select(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL selected, BOOL isFolder);
    static BOOL GetColumnInfo(TAR_ARCHIVE * pArchive, wchar_t * pColumnInfo, int maxcch);
};
