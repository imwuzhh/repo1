#pragma warning(disable:4996)
#include <Windows.h>
#include <ShlObj.h>
#include <atlbase.h>
#include <string>
#include <list>
#include <sys/stat.h>

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

#include "datamgr.h"

///////////////////////////////////////////////////////////////////////////////
// Debug helper functions
std::string WideStringToAnsi(const wchar_t * wstr){
	char szAnsi [MAX_PATH] = "";
	WideCharToMultiByte(CP_ACP, 0, wstr, wcslen(wstr), szAnsi, lengthof(szAnsi), NULL, NULL);
	return szAnsi;
}

#define WSTR2ASTR(w) (WideStringToAnsi(w).c_str())

static void OutputLog(const char * format, ...){
	va_list va;
	va_start(va, format);
	char szMsg [0x400] = "";
	_vsnprintf_s(szMsg, lengthof(szMsg), format, va);
	OutputDebugStringA(szMsg);
	OutputDebugStringA("\n");
}

#define OUTPUTLOG OutputLog

#define VDRIVE_LOCAL_CACHE_ROOT _T("d:\\workspace\\temp\\vdrivecache\\")

///////////////////////////////////////////////////////////////////////////////
// TAR archive manipulation

/**
 * Opens an existing .tar archive.
 * Here we basically memorize the filename of the .tar achive, and we only
 * open the file later on when it is actually needed the first time.
 */
HRESULT DMOpen(LPCWSTR pwstrFilename, TAR_ARCHIVE** ppArchive)
{
   TAR_ARCHIVE* pArchive = new TAR_ARCHIVE();
   if( pArchive == NULL ) return E_OUTOFMEMORY;
   pArchive->hFile = INVALID_HANDLE_VALUE;
   pArchive->ftLastWrite.dwLowDateTime = 0;
   pArchive->ftLastWrite.dwHighDateTime = 0;
   wcscpy_s(pArchive->wszFilename, lengthof(pArchive->wszFilename), pwstrFilename);
   *ppArchive = pArchive;
   return S_OK;
}

/**
 * Close the archive.
 */
HRESULT DMClose(TAR_ARCHIVE* pArchive)
{
   if( pArchive == NULL ) return E_INVALIDARG;
   delete pArchive;
   return S_OK;
}

/**
 * Return file information.
 * Convert the archive file information to a Windows WIN32_FIND_DATA structure, which
 * contains the basic information we must know about a virtual file/folder.
 */
HRESULT DMGetFileAttr(TAR_ARCHIVE* pArchive, LPCWSTR pstrFilename, WIN32_FIND_DATA* pData)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), pwstrPath=%s", __FUNCTION__, (pstrFilename && wcslen(pstrFilename)) ? WSTR2ASTR(pstrFilename) : "<ROOT>");

   std::wstring dir_prefix  = VDRIVE_LOCAL_CACHE_ROOT;
   dir_prefix += pstrFilename;
   if (!PathFileExists(dir_prefix.c_str())){
	   return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   }
   HANDLE hFind = FindFirstFile(dir_prefix.c_str(), pData);
   if (INVALID_HANDLE_VALUE  == hFind)
	   return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);

   FindClose(hFind); hFind = INVALID_HANDLE_VALUE;

   return S_OK;
}

/**
 * Return the list of children of a sub-folder.
 */
HRESULT DMGetChildrenList(TAR_ARCHIVE* pArchive, LPCWSTR pwstrPath, WIN32_FIND_DATA ** retList, int * nListCount)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), pwstrPath=%s", __FUNCTION__, (pwstrPath && wcslen(pwstrPath)) ? WSTR2ASTR(pwstrPath) : "<ROOT>");

   std::list<WIN32_FIND_DATA> retWin32FindData;

   std::wstring dir_prefix = VDRIVE_LOCAL_CACHE_ROOT;
   dir_prefix += pwstrPath ? pwstrPath : _T("");
   dir_prefix += _T("\\*");

   HANDLE hFind = INVALID_HANDLE_VALUE;
   WIN32_FIND_DATA wfd;
   if (INVALID_HANDLE_VALUE == (hFind = FindFirstFile(dir_prefix.c_str(), &wfd)))
	   return AtlHresultFromLastError();
   while (FindNextFile(hFind, &wfd)){
	   if (wfd.cFileName[0] != _T('.'))
			retWin32FindData.push_back(wfd);
   }
   FindClose(hFind);

   if (retWin32FindData.size() == 0){
	   *retList = NULL;
	   *nListCount = 0;
	   return S_OK;
   }

   *nListCount = retWin32FindData.size();
   *retList = new WIN32_FIND_DATA[*nListCount];
   WIN32_FIND_DATA *aList = *retList;

   int index = 0;
   for(std::list<WIN32_FIND_DATA>::iterator it = retWin32FindData.begin(); 
	   it != retWin32FindData.end();
	   it ++){
		aList [index] = *it;
		index ++;
   }

   return S_OK;
}

/**
 * Rename a file or folder in the archive.
 * Notice that we do not support rename of a folder, which contains files, yet.
 */
HRESULT DMRename(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename, LPCWSTR pwstrNewName)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), pwstrFilename=%s, pwstrNewName=%s", __FUNCTION__, pwstrFilename ? WSTR2ASTR(pwstrFilename) : "", pwstrNewName ? WSTR2ASTR(pwstrNewName) : "");
   std::wstring dir_prefix = VDRIVE_LOCAL_CACHE_ROOT;
   std::wstring origin = dir_prefix;
   origin += pwstrFilename;
   
   wchar_t szNew [MAX_PATH] = _T(""); wcscpy_s(szNew, lengthof(szNew), origin.c_str());
   wcsrchr(szNew, _T('\\'))[1] = 0;
   wcscat_s(szNew, lengthof(szNew), pwstrNewName);

   MoveFile(origin.c_str(), szNew);

   return S_OK;
}

/**
 * Delete a file or folder.
 */
HRESULT DMDelete(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), pwstrFilename=%s", __FUNCTION__, pwstrFilename ? WSTR2ASTR(pwstrFilename) : "");
   std::wstring fullpath = VDRIVE_LOCAL_CACHE_ROOT;
   fullpath += pwstrFilename;
   
   // HarryWu, 2014.2.1
   // Notice, we must construct a double-null-ed string to specify the full path 
   // to delete.
   wchar_t szFullPath [MAX_PATH] = _T(""); memset(szFullPath, 0, sizeof(szFullPath));
   wcscpy_s(szFullPath, lengthof(szFullPath) - 1, fullpath.c_str());

#if 0
   std::wstring cmdstr = _T("/c \"rd /s /q %s\"");
   wchar_t szCmd [MAX_PATH] = _T("");
   _swprintf(szCmd, cmdstr.c_str(), szFullPath);
   ShellExecute(GetActiveWindow(), _T("Open"), _T("c:\\windows\\system32\\cmd.exe"), szCmd, NULL, SW_HIDE);
#else
   SHFILEOPSTRUCT shfop; memset(&shfop, 0, sizeof(shfop));
   shfop.hwnd = ::GetActiveWindow();
   shfop.wFunc = FO_DELETE;
   shfop.fFlags = FOF_NO_UI;
   shfop.pFrom = szFullPath;
   shfop.pTo = NULL;
   SHFileOperation(&shfop);
#endif
   return S_OK;
}

/**
 * Create a new sub-folder in the archive.
 */
HRESULT DMCreateFolder(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), pwstrFilename=%s", __FUNCTION__, pwstrFilename ? WSTR2ASTR(pwstrFilename) : "");
   std::wstring dir_prefix = VDRIVE_LOCAL_CACHE_ROOT;
   dir_prefix += pwstrFilename;
   if (!PathFileExists(dir_prefix.c_str())){
		SHCreateDirectory(GetActiveWindow(), dir_prefix.c_str());
   }
   return S_OK;
}

/**
 * Change the file-attributes of a file.
 */
HRESULT DMSetFileAttr(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename, DWORD dwAttributes)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), pwstrFilename=%s", __FUNCTION__, pwstrFilename ? WSTR2ASTR(pwstrFilename) : "");
   return S_OK;
}

/**
 * Create a new file in the archive.
 * This method expects a memory buffer containing the entire file contents.
 * HarryWu, 2014.2.2
 * Notice! an empty file can NOT be created here!
 */
HRESULT DMWriteFile(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename, const LPBYTE pbBuffer, DWORD dwFileSize, DWORD dwAttributes)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), pwstrFilename=%s, dwFileSize=%d", __FUNCTION__, pwstrFilename ? WSTR2ASTR(pwstrFilename) : ""
	   , dwFileSize);
   std::wstring fullpath = VDRIVE_LOCAL_CACHE_ROOT;
   fullpath += pwstrFilename;
   FILE * fout = _wfopen(fullpath.c_str(), _T("wb"));
   if (fout){
	   if (dwFileSize != fwrite(pbBuffer, 1, dwFileSize, fout)){
		   fclose(fout);
		   return E_FAIL;
	   }
	   fclose(fout);
   }else{
	   return AtlHresultFromLastError();
   }
   return S_OK;
}

/**
 * Reads a file from the archive.
 * The method returns the contents of the entire file in a memory buffer.
 */
HRESULT DMReadFile(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename, LPBYTE* ppbBuffer, DWORD& dwFileSize)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), pwstrFilename=%s", __FUNCTION__, pwstrFilename ? WSTR2ASTR(pwstrFilename) : "");

   std::wstring fullpath = VDRIVE_LOCAL_CACHE_ROOT;
   fullpath += pwstrFilename;

   struct _stat _st = {0}; _wstat(fullpath.c_str(), &_st);

   dwFileSize = _st.st_size;
   *ppbBuffer = (LPBYTE)malloc(dwFileSize);
   memset(*ppbBuffer, 'E', dwFileSize);

   FILE * fin = _wfopen(fullpath.c_str(), _T("rb"));
   if (fin){
		dwFileSize = fread(*ppbBuffer, 1, dwFileSize, fin);
		if (dwFileSize < _st.st_size)
		{
			fclose(fin); free(*ppbBuffer); dwFileSize = 0;
			return E_FAIL;
		}
		fclose(fin);
   }else{
	   free(*ppbBuffer); dwFileSize = 0;
	   return AtlHresultFromLastError();
   }

   return S_OK;
}

