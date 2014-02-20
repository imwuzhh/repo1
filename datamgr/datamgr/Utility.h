#pragma once

enum {
	MaxUrlLength = 0x1000,
};

// forward deceleration
struct TAR_ARCHIVE;

class Utility
{
public:
	Utility(void);
	virtual ~Utility(void);
public:
	static BOOL Login(TAR_ARCHIVE * pArchive);
	static BOOL GetTopPublic(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPublic);
	static BOOL GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPersonal);
	static BOOL GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFolders);
	static BOOL GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFiles);
	static BOOL ConstructRecycleFolder(RFS_FIND_DATA & recycleFolder);
	static BOOL ConstructSearchFolder(RFS_FIND_DATA & searchFolder);
public:
	static bool RfsComparation(const RFS_FIND_DATA & left, const RFS_FIND_DATA & right);
public:
	static BOOL HttpRequest(const wchar_t * requestUrl, std::wstring & response);
	static BOOL JsonRequest(const wchar_t * reqJson, std::wstring & response);
};
