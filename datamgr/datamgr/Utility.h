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
	static BOOL ConstructRecycleFolder(TAR_ARCHIVE * pArchive, RFS_FIND_DATA & recycleFolder);
	static BOOL ConstructSearchFolder(TAR_ARCHIVE * pArchive, RFS_FIND_DATA & searchFolder);
	static BOOL DeleteItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL isFolder);
	static BOOL RenameItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * newName, BOOL isFolder);
	static BOOL CreateFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * folderName, RemoteId * retId);
	static BOOL WriteFile(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * tempFile);
	static BOOL ReadFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * tempFile);
    static BOOL BatchUpload(TAR_ARCHIVE * pArchive, const RemoteId & viewId, const wchar_t * localPath);
    static BOOL BatchDownload(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath);
public:
	static bool RfsComparation(const RFS_FIND_DATA & left, const RFS_FIND_DATA & right);
public:
	static BOOL HttpRequest(const wchar_t * requestUrl, std::wstring & response);
	static BOOL JsonRequest(const wchar_t * reqJson, std::wstring & response);
	static BOOL LoadLocalizedName(const wchar_t * localeName, const wchar_t * key, wchar_t * retVaule, int cchMax);

private:
	static unsigned char ToHex(unsigned char x);
	static unsigned char FromHex(unsigned char x);
	static std::string UrlEncode(const std::string& str);
	static std::string UrlDecode(const std::string& str);
};

#ifndef WSTR2ASTR
	#define WSTR2ASTR(w) (WideStringToAnsi(w).c_str())
#endif

#ifndef OUTPUTLOG
	#define OUTPUTLOG OutputLog
#endif

std::string WideStringToAnsi(const wchar_t * wstr);
void OutputLog(const char * format, ...);
