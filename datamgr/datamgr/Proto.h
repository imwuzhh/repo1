#pragma once

class Proto
{
public:
     virtual BOOL Login(TAR_ARCHIVE * pArchive) = 0;
     virtual BOOL GetTopPublic(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPublic) = 0;
     virtual BOOL GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPersonal) = 0;
     virtual BOOL GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFolders) = 0;
     virtual BOOL GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFiles) = 0;
     virtual BOOL DeleteItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL isFolder) = 0;
     virtual BOOL RenameItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * newName, BOOL isFolder) = 0;
     virtual BOOL CreateFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * folderName, RemoteId * retId) = 0;
     virtual BOOL Download(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * tempFile) = 0;
     virtual BOOL Upload(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * tempFile) = 0;
     virtual BOOL Select(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL selected, BOOL isFolder) = 0;
     virtual BOOL GetColumnInfo(TAR_ARCHIVE * pArchive, const RemoteId & viewId, wchar_t * pColumnInfo, int maxcch) = 0;
};
