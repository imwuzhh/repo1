#pragma warning(disable:4996)
#include <Windows.h>
#include <ShlObj.h>
#include <atlbase.h>
#include <string>
#include <list>
#include <iostream>
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
#include <tinystr.h>
#include <tinyxml.h>
#include <json/json.h>
#include "Utility.h"

/*
// Json request
{
	Server: "127.0.0.1",
	Port: 60684,
	Version: "1.0.0.1", 
	Method: "SampleRequest",
	Param: {param1: value1, param2: value2, ...}
}

// Json response
{
	Ack: "SampleRequest",
	statusCode: 0,
	Status: "Code is 0, so everything is ok.",
	Return: {ret1: value1, ret2: value2, ...}
}
*/

///////////////////////////////////////////////////////////////////////////////
// Debug helper functions
static std::string WideStringToAnsi(const wchar_t * wstr){
	if (!wstr) return "";
	char szAnsi [MAX_PATH] = "";
	WideCharToMultiByte(CP_ACP, 0, wstr, wcslen(wstr), szAnsi, lengthof(szAnsi), NULL, NULL);
	return szAnsi;
}
static void OutputLog(const char * format, ...){
	va_list va;
	va_start(va, format);
	char szMsg [0x400] = "";
	_vsnprintf_s(szMsg, lengthof(szMsg), format, va);
	OutputDebugStringA(szMsg); OutputDebugStringA("\n");
}
#define WSTR2ASTR(w) (WideStringToAnsi(w).c_str())
#define OUTPUTLOG OutputLog

///////////////////////////////////////////////////////////////////////////////
// cache directory
#define VDRIVE_LOCAL_CACHE_ROOT ((pArchive && pArchive->context) \
								? (pArchive->context->cachedir) \
								: _T(""))

///////////////////////////////////////////////////////////////////////////////
// Global Context
static Edoc2Context * gspEdoc2Context = NULL;

///////////////////////////////////////////////////////////////////////////////
// TAR archive manipulation

/**
* Initialize 
*/
HRESULT DMInit(){
	HRESULT hr = E_FAIL;
	if (gspEdoc2Context) return S_OK;

	gspEdoc2Context = new Edoc2Context();
	memset(gspEdoc2Context, 0, sizeof(Edoc2Context));

	struct Edoc2Context & context = *gspEdoc2Context;

	// Setup locale.
	GetUserDefaultLocaleName(context.localeName, lengthof(context.localeName));

	// Setup local cache directory
	if (GetTempPathW(lengthof(context.cachedir), context.cachedir) <= 0){
		hr = AtlHresultFromLastError();
		goto bail;
	}
	wcscat_s(context.cachedir, lengthof(context.cachedir), _T("\\vdrivecache\\"));
	if (!PathFileExists(context.cachedir)){
		SHCreateDirectory(GetActiveWindow(), context.cachedir);
	}

	// Setup Service address
	wcscpy_s(context.service, lengthof(context.service), _T("http://192.168.253.242"));
	// Setup Username & passworld
	wcscpy_s(context.username, lengthof(context.username), _T("admin"));
	wcscpy_s(context.password, lengthof(context.password), _T("edoc2"));
	// Cleanup AccessToken
	context.AccessToken [0] = _T('\0');

	hr = S_OK;
bail:
	return hr;
}

HRESULT DMCleanup()
{
	if (gspEdoc2Context){
		delete gspEdoc2Context;
		gspEdoc2Context = NULL;
	}
	return S_OK;
}

/**
 * Opens an existing .tar archive.
 * Here we basically memorize the filename of the .tar achive, and we only
 * open the file later on when it is actually needed the first time.
 */
HRESULT DMOpen(LPCWSTR pwstrFilename, TAR_ARCHIVE** ppArchive)
{
   HRESULT hr = E_FAIL;
   TAR_ARCHIVE* pArchive = new TAR_ARCHIVE();
   if( pArchive == NULL ) return E_OUTOFMEMORY;
   // Retrieve the session context ptr.
   pArchive->context = gspEdoc2Context;
   *ppArchive = pArchive;
   return S_OK;
bail:
   if (pArchive != NULL) delete pArchive;
   *ppArchive = NULL;
   return hr;
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
 * Return the list of children of a sub-folder.
 */
HRESULT DMGetChildrenList(TAR_ARCHIVE* pArchive, RemoteId dwId, RFS_FIND_DATA ** retList, int * nListCount)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   OUTPUTLOG("%s(), RemoteId={%d, %d}", __FUNCTION__, dwId.category, dwId.id);
   *retList = NULL; *nListCount = 0;
   std::list<RFS_FIND_DATA> tmpList;

   // HarryWu, 2014.2.18
   // Test code.
   // 1) get sub folders for Enterprise Level
   // "http://192.168.253.242/EDoc2WebApi/api/Doc/FolderRead/GetTopPublicFolder?token=2ef01717-6f61-4592-a606-7292f3cb5a57"
   // 2) get sub foders for Personal Level
   // "http://192.168.253.242/EDoc2WebApi/api/Doc/FolderRead/GetTopPersonalFolder?token=2ef01717-6f61-4592-a606-7292f3cb5a57"
   // 3) get sub folder by id.
   // "http://192.168.253.242/EDoc2WebApi/api/Doc/FolderRead/GetChildFolderListByFolderId?token=2ef01717-6f61-4592-a606-7292f3cb5a57&folderId=16086"
   // 4) get sub files by id
   // "http://192.168.253.242/EDoc2WebApi/api/Doc/FileRead/GetChildFileListByFolderId?token=c8132990-f885-440f-9c5b-88fa685d2482&folderId=16415"

   if (dwId.category == VdriveCat && dwId.id == VdriveId){
	   std::list<RFS_FIND_DATA> publicList;
	   Utility::GetTopPublic(pArchive, publicList);
	   tmpList.merge(publicList, Utility::RfsComparation);
	   std::list<RFS_FIND_DATA> personalList;
	   Utility::GetTopPersonal(pArchive, personalList);
	   tmpList.merge(personalList, Utility::RfsComparation);
	   RFS_FIND_DATA recycleBin;
	   Utility::ConstructRecycleFolder(pArchive, recycleBin);
	   tmpList.push_back(recycleBin);
	   RFS_FIND_DATA searchBin;
	   Utility::ConstructSearchFolder(pArchive, searchBin);
	   tmpList.push_back(searchBin);
   }else if (dwId.category == RecycleCat){

   }else if (dwId.category == SearchCat){

   }else if (dwId.category == PublicCat || dwId.category == PersonCat){
	   std::list<RFS_FIND_DATA> childFolders;
	   Utility::GetChildFolders(pArchive, dwId, childFolders);
	   tmpList.merge(childFolders, Utility::RfsComparation);
	   std::list<RFS_FIND_DATA> childFiles;
	   Utility::GetChildFiles(pArchive, dwId, childFiles);
	   tmpList.merge(childFiles, Utility::RfsComparation);
   }else{
	   OUTPUTLOG("Invalid RemoteId{%d,%d}", dwId.category, dwId.id);
	   return E_INVALIDARG;
   }

   if (tmpList.size() == 0) return S_OK;

   if (S_OK != DMMalloc((LPBYTE *)retList, tmpList.size() * sizeof(RFS_FIND_DATA))){
	   return E_OUTOFMEMORY;
   }
   RFS_FIND_DATA *aList = (RFS_FIND_DATA *)(*retList);

   int index = 0;
   for(std::list<RFS_FIND_DATA>::iterator it = tmpList.begin(); 
	   it != tmpList.end(); it ++){ aList [index] = *it;
		// refine the attributes.
		RFS_FIND_DATA * pData = &aList[index];
		pData->dwFileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;   
		pData->dwFileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
		if (!IsBitSet(pData->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
			pData->dwFileAttributes |= FILE_ATTRIBUTE_VIRTUAL;
		index ++;
   }
   *nListCount = tmpList.size();
   return S_OK;
}

/**
 * Rename a file or folder in the archive.
 * Notice that we do not support rename of a folder, which contains files, yet.
 */
HRESULT DMRename(TAR_ARCHIVE* pArchive, RemoteId itemId, LPCWSTR pwstrNewName)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);

   OUTPUTLOG("%s(), RemoteId={%d,%d}, NewName=%s", __FUNCTION__, itemId.category, itemId.id, (const char *)CW2A(pwstrNewName));

   { // Rename File/Folder on remote
	   Utility::RenameItem(pArchive, itemId, pwstrNewName);
   }
   return S_OK;
}

/**
 * Delete a file or folder.
 */
HRESULT DMDelete(TAR_ARCHIVE* pArchive, RemoteId itemId)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);

   OUTPUTLOG("%s(), RemoteId={%d,%d}", __FUNCTION__, itemId.category, itemId.id);

   {// Delete remote File/Folder
	   Utility::DeleteItem(pArchive, itemId);
   }
   return S_OK;
}

/**
 * Create a new sub-folder in the archive.
 */
HRESULT DMCreateFolder(TAR_ARCHIVE* pArchive, RemoteId parentId, LPCWSTR pwstrFilename, RFS_FIND_DATA * pWfd)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   
   if (NULL == pwstrFilename ) return E_INVALIDARG;

   OUTPUTLOG("%s(), ParentId={%d,%d}, pwstrFilename=[%s]", __FUNCTION__, parentId.category, parentId.id, WSTR2ASTR(pwstrFilename));

   {// Create Folder On remote
	   RemoteId folderId = {PublicCat, 0};
	   Utility::CreateFolder(pArchive, parentId, pwstrFilename, &folderId);
	   pWfd->dwId = folderId;
   }

   return S_OK;
}

/**
 * Change the file-attributes of a file.
 */
HRESULT DMSetFileAttr(TAR_ARCHIVE* pArchive, LPCWSTR pwstrFilename, DWORD dwAttributes)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   
   if (NULL == pwstrFilename) return E_INVALIDARG;

   OUTPUTLOG("%s(), pwstrFilename=[%s]", __FUNCTION__, WSTR2ASTR(pwstrFilename));

   return S_OK;
}

/**
 * Create a new file in the archive.
 * This method expects a memory buffer containing the entire file contents.
 * HarryWu, 2014.2.2
 * Notice! an empty file can NOT be created here!
 */
HRESULT DMWriteFile(TAR_ARCHIVE* pArchive, RemoteId parentId, LPCWSTR pwstrFilename, const LPBYTE pbBuffer, DWORD dwFileSize, DWORD dwAttributes)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
   
   if (NULL == pwstrFilename || !pbBuffer || !dwFileSize) return E_INVALIDARG;

   OUTPUTLOG("%s(),ParentID={%d,%d}, pwstrFilename=[%s], dwFileSize=[%d]", __FUNCTION__, parentId.category, parentId.id, WSTR2ASTR(pwstrFilename), dwFileSize);

   // HarryWu, 2014.2.15
   // TODO: Post file content to server
   // pbBuffer, dwFileSize
   {
		wchar_t szTempFile [MAX_PATH] = _T("");
		GetTempPath(lengthof(szTempFile), szTempFile);
		wcscat_s(szTempFile, lengthof(szTempFile), _T("\\"));
		wcscat_s(szTempFile, lengthof(szTempFile), wcsrchr(pwstrFilename, _T('\\')) ? wcsrchr(pwstrFilename, _T('\\')) + 1: pwstrFilename);
		
		// Write content to temp file, 
		// and then upload this temp file.
		FILE * fout = _wfopen(szTempFile, _T("wb"));
		fwrite(pbBuffer, 1, dwFileSize, fout);
		fclose(fout);

		OUTPUTLOG("%s(), write [%s] with %d bytes", __FUNCTION__, (const char *)CW2A(szTempFile), dwFileSize);

		// Upload Temporary file to server.
		Utility::UploadFile(pArchive, parentId, szTempFile);
   }

   return S_OK;
}

/**
 * Reads a file from the archive.
 * The method returns the contents of the entire file in a memory buffer.
 */
HRESULT DMReadFile(TAR_ARCHIVE* pArchive, RemoteId itemId, LPCWSTR pwstrFilename, LPBYTE* ppbBuffer, DWORD* pdwFileSize)
{
	CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);

	if (NULL == pwstrFilename ) return E_INVALIDARG;

	OUTPUTLOG("%s(), ItemId={%d,%d}, pwstrFilename=[%s]", __FUNCTION__, itemId.category, itemId.id, WSTR2ASTR(pwstrFilename));

	*ppbBuffer = NULL; *pdwFileSize = 0;

	// HarryWu, 2014.2.15
	// TODO: Read file contents from remote.
	// ...
	{
		wchar_t szTempFile [MAX_PATH] = _T("");
		GetTempPath(lengthof(szTempFile), szTempFile);
		wcscat_s(szTempFile, lengthof(szTempFile), _T("\\"));
		wcscat_s(szTempFile, lengthof(szTempFile), wcsrchr(pwstrFilename, _T('\\')) ? wcsrchr(pwstrFilename, _T('\\')) + 1: pwstrFilename);

		// Download remote file to local temp file,
		// and then read content from this file.
		Utility::DownloadFile(pArchive, itemId, szTempFile);

		// Write content to temp file, 
		// and then upload this temp file.
		FILE * fin = _wfopen(szTempFile, _T("rb"));
		if (fin != NULL){
			fseek(fin, 0, SEEK_END);
			*pdwFileSize = ftell(fin);
		}
		OUTPUTLOG("%s(), read [%s] with %d bytes", __FUNCTION__, (const char *)CW2A(szTempFile), *pdwFileSize);
		DMMalloc((LPBYTE*)ppbBuffer, *pdwFileSize);
		if (*ppbBuffer && fin){
			fread(*ppbBuffer, 1, *pdwFileSize, fin);
		}
		if (fin) fclose(fin);
	}
	return S_OK;
}

HRESULT DMMalloc(LPBYTE * ppBuffer, DWORD dwBufSize)
{
	*ppBuffer = (LPBYTE)malloc(dwBufSize);
	if (*ppBuffer != NULL)
		return S_OK;
	else 
		return E_OUTOFMEMORY;
}

HRESULT DMRealloc(LPBYTE * ppBuffer, DWORD dwBufSize)
{
	*ppBuffer = (LPBYTE)realloc(*ppBuffer, dwBufSize);
	if (*ppBuffer != NULL)
		return S_OK;
	else 
		return E_OUTOFMEMORY;
}

HRESULT DMFree(LPBYTE pBuffer)
{
	free(pBuffer);
	return S_OK;
}

