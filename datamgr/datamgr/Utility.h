#pragma once

enum {
	MaxUrlLength = 0x1000,
};

// forward deceleration
struct TAR_ARCHIVE;

class Utility
{
public:
    static BOOL ConstructRecycleFolder(TAR_ARCHIVE * pArchive, RFS_FIND_DATA & recycleFolder);
    static BOOL ConstructSearchFolder(TAR_ARCHIVE * pArchive, RFS_FIND_DATA & searchFolder);
	static bool RfsComparation(const RFS_FIND_DATA & left, const RFS_FIND_DATA & right);
	static BOOL HttpRequest(const wchar_t * requestUrl, std::wstring & response);
	static BOOL JsonRequest(const wchar_t * reqJson, std::wstring & response);
	static BOOL LoadLocalizedName(const wchar_t * localeName, const wchar_t * key, wchar_t * retVaule, int cchMax);
	static unsigned char ToHex(unsigned char x);
	static unsigned char FromHex(unsigned char x);
	static std::string UrlEncode(const std::string& str);
	static std::string UrlDecode(const std::string& str);
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
