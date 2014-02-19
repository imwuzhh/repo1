#pragma once

enum {
	MaxUrlLength = 0x1000,
};

class Utility
{
public:
	Utility(void);
	virtual ~Utility(void);
public:
	static BOOL Login(const wchar_t * user, const wchar_t * pass, wchar_t * accessToken);
	static BOOL GetTopPublic(const wchar_t * accessToken, std::list<RFS_FIND_DATA> & topPublic);
	static BOOL GetTopPersonal(const wchar_t * accessToken, std::list<RFS_FIND_DATA> & topPersonal);
	static BOOL GetChildFolders(const wchar_t * accessToken, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFolders);
	static BOOL GetChildFiles(const wchar_t * accessToken, const RemoteId & folderId, std::list<RFS_FIND_DATA> & childFiles);
	static BOOL ConstructRecycleFolder(RFS_FIND_DATA & recycleFolder);
	static BOOL ConstructSearchFolder(RFS_FIND_DATA & searchFolder);
public:
	static bool RfsComparation(const RFS_FIND_DATA & left, const RFS_FIND_DATA & right);
public:
	static BOOL HttpRequest(const wchar_t * requestUrl, std::wstring & response);
};
