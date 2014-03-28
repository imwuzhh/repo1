#pragma once

enum {
	MaxUrlLength = 0x1000,
};

// forward deceleration
struct TAR_ARCHIVE;

class Utility
{
public:
    static BOOL ConstructRecycleFolder(TAR_ARCHIVE * pArchive, VFS_FIND_DATA & recycleFolder);
    static BOOL ConstructSearchFolder(TAR_ARCHIVE * pArchive, VFS_FIND_DATA & searchFolder);
	static bool RfsComparation(const VFS_FIND_DATA & left, const VFS_FIND_DATA & right);
    static BOOL HttpRequest(const wchar_t * requestUrl, std::wstring & response, unsigned int timeoutMs);
    static BOOL HttpRequestWithCookie(const wchar_t * requestUrl, const std::wstring & cookie, std::stringstream & response, unsigned int timeoutMs);
    static BOOL HttpPostFile(const wchar_t * service, int parentId, const wchar_t * tempFile, const wchar_t * faceName, std::stringstream & response, int timeoutMs);
	static BOOL JsonRequest(const wchar_t * reqJson, std::wstring & response);
	static BOOL LoadLocalizedName(const wchar_t * xmlconfigfile, const wchar_t * localeName, const wchar_t * key, wchar_t * retVaule, int cchMax);
    static BOOL CheckHttpEnable(const wchar_t * xmlconfigfile);
    static DWORD GetHttpTimeoutMs(const WCHAR * xmlconfigfile);
    static BOOL GetServiceBase(const wchar_t* xmlconfigfile, wchar_t * pszSvcBase, int maxcch);
    static BOOL GetServiceUser(const wchar_t* xmlconfigfile, wchar_t * pszSvcUser, int maxcch);
    static BOOL GetServicePass(const wchar_t* xmlconfigfile, wchar_t * pszSvcPass, int maxcch);
	static unsigned char ToHex(unsigned char x);
	static unsigned char FromHex(unsigned char x);
	static std::string UrlEncode(const std::string& str);
	static std::string UrlDecode(const std::string& str);
    static BOOL GenerateTempFilePath(wchar_t * pwszTempFilePath, int cchMax, const wchar_t * templatename);
    static BOOL HttpPostForm(const wchar_t * url, const wchar_t * httpform, const wchar_t * cookie, std::stringstream & response, int timeoutMs);
};

///////////////////////////////////////////////////////////////////////////////
// Macros

#ifndef lengthof
#define lengthof(x)  (sizeof(x)/sizeof(x[0]))
#endif  // lengthof

#ifndef offsetof
#define offsetof(type, field)  ((int)&((type*)0)->field)
#endif  // offsetof

#ifndef IsBitSet
#define IsBitSet(val, bit)  (((val)&(bit))!=0)
#endif // IsBitSet


#ifdef _DEBUG
#define HR(expr)  { HRESULT _hr; if(FAILED(_hr=(expr))) { _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, #expr); _CrtDbgBreak(); return _hr; } }  
#else
#define HR(expr)  { HRESULT _hr; if(FAILED(_hr=(expr))) return _hr; }
#endif // _DEBUG

#define UTILITY_SKIP_PEER_VERIFICATION
#define UTILITY_SKIP_HOSTNAME_VERIFICATION

#ifndef WSTR2ASTR
	#define WSTR2ASTR(w) (WideStringToAnsi(w).c_str())
#endif

#ifndef OUTPUTLOG
	#define OUTPUTLOG OutputLog
#endif

std::string WideStringToAnsi(const wchar_t * wstr);
void OutputLog(const char * format, ...);
