#pragma warning(disable:4996)
#include <Windows.h>
#include <ShlObj.h>
#include <atlbase.h>
#include <string>
#include <list>
#include <iostream>
#include <sys/stat.h>
#include <tinystr.h>
#include <tinyxml.h>
#include <json/json.h>
#include "Edoc2Context.h"
#include "datamgr.h"
#include "Proto.h"
#include "HttpImpl.h"
#include "JsonImpl.h"
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
// cache directory
#define VDRIVE_LOCAL_CACHE_ROOT ((pArchive && pArchive->context) \
								? (pArchive->context->cachedir) \
								: _T(""))

#define ENABLE_HTTP(pArchive)  (pArchive && pArchive->context && pArchive->context->enableHttp)

///////////////////////////////////////////////////////////////////////////////
// Global Context
static Edoc2Context * gspEdoc2Context = NULL;
static __inline Proto * GetProto(TAR_ARCHIVE * pArchive){ return (pArchive->context->proto);}
///////////////////////////////////////////////////////////////////////////////
// TAR archive manipulation

/**
* Initialize from configuration file.
* e.g: libdatamgr.ini, libdatamgr.xml
*/
static HRESULT DMInit(HINSTANCE hInst){
	HRESULT hr = E_FAIL;
	if (gspEdoc2Context) return S_OK;

	gspEdoc2Context = new Edoc2Context();
	memset(gspEdoc2Context, 0, sizeof(Edoc2Context));

	struct Edoc2Context & context = *gspEdoc2Context;

    // Setup Instance handle
    context.hInst = hInst;

    // Setup module path
    GetModuleFileName(hInst, context.modulepath, lengthof(context.modulepath));
    wcsrchr(context.modulepath, _T('\\'))[1] = _T('\0');

    // Setup configuration path
    wcscpy_s(context.configfile, lengthof(context.configfile), context.modulepath);
    wcscat_s(context.configfile, lengthof(context.configfile), _T("libdatamgr.xml"));

    // Setup locale.
	GetUserDefaultLocaleName(context.localeName, lengthof(context.localeName));

    // Setup Proxy Type
    // TODO: Load this configuration from file.
    // Default to False, means that, use Json proxy to access server.
    // otherwise, use the internal http impl.
    context.enableHttp = Utility::CheckHttpEnable(context.configfile);

    // Setup Http Timeout in milliseconds.
    context.HttpTimeoutMs = Utility::GetHttpTimeoutMs(context.configfile);

    if (context.enableHttp){
        context.proto = new HttpImpl();
    }else{
        context.proto = new JsonImpl();
    }

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
    Utility::GetServiceBase(context.configfile, context.service, lengthof(context.service));

	// Setup Username & passworld
	Utility::GetServiceUser(context.configfile, context.username, lengthof(context.username));
	Utility::GetServicePass(context.configfile, context.password, lengthof(context.password));

	// Cleanup AccessToken
	context.AccessToken [0] = _T('\0');

	hr = S_OK;
bail:
	return hr;
}

static HRESULT DMCleanup()
{
	if (gspEdoc2Context){
        delete gspEdoc2Context->proto; gspEdoc2Context->proto = NULL;
		delete gspEdoc2Context; gspEdoc2Context = NULL;
	}
	return S_OK;
}

BOOL WINAPI DllMain(_In_  HINSTANCE hinstDLL,
                    _In_  DWORD fdwReason,
                    _In_  LPVOID lpvReserved
                    )
{
    if (DLL_PROCESS_ATTACH){
        DMInit(hinstDLL);
    }
    if (DLL_PROCESS_DETACH){
        DMCleanup();
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
BOOL DMHttpIsEnable()
{
    return gspEdoc2Context && gspEdoc2Context->enableHttp;
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

   if (dwId.category == VdriveCat && dwId.id == VdriveId){
	   std::list<RFS_FIND_DATA> publicList;
       GetProto(pArchive)->GetTopPublic(pArchive, publicList);
	   tmpList.merge(publicList, Utility::RfsComparation);

	   std::list<RFS_FIND_DATA> personalList;
       GetProto(pArchive)->GetTopPersonal(pArchive, personalList);
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
       GetProto(pArchive)->GetChildFolders(pArchive, dwId, childFolders);
	   tmpList.merge(childFolders, Utility::RfsComparation);

	   std::list<RFS_FIND_DATA> childFiles;
       GetProto(pArchive)->GetChildFiles(pArchive, dwId, childFiles);
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
HRESULT DMRename(TAR_ARCHIVE* pArchive, RemoteId itemId, LPCWSTR pwstrNewName, BOOL isFolder)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);

   OUTPUTLOG("%s(), RemoteId={%d,%d}, NewName=%s", __FUNCTION__, itemId.category, itemId.id, (const char *)CW2A(pwstrNewName));

   if (!GetProto(pArchive)->RenameItem(pArchive, itemId, pwstrNewName, isFolder)){
	   OUTPUTLOG("%s(), Failed to rename item on server.", __FUNCTION__);
	   return E_FAIL;
   }
   return S_OK;
}

/**
 * Delete a file or folder.
 */
HRESULT DMDelete(TAR_ARCHIVE* pArchive, RemoteId itemId, BOOL isFolder)
{
   CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);

   OUTPUTLOG("%s(), RemoteId={%d,%d}", __FUNCTION__, itemId.category, itemId.id);

   if (!GetProto(pArchive)->DeleteItem(pArchive, itemId, isFolder))
   {
	   return E_FAIL;
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

   OUTPUTLOG("%s(), ParentId={%d,%d}, pwstrFilename=[%s]", __FUNCTION__, parentId.category, parentId.id, (const char *)CW2A(pwstrFilename));

   if (wcsrchr(pwstrFilename, _T('\\'))){
	   pwstrFilename = wcsrchr(pwstrFilename, _T('\\')) + 1;
   }

   RemoteId folderId = {PublicCat, 0};
   if (GetProto(pArchive)->CreateFolder(pArchive, parentId, pwstrFilename, &folderId)){
	   pWfd->dwId = folderId;
	   OUTPUTLOG("%s(`%s\') return Id=[%d:%d]", __FUNCTION__, (const char *)CW2A(pwstrFilename), folderId.category, folderId.id);
	   return S_OK;
   }

   return E_FAIL;
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

    OUTPUTLOG("%s(),ParentID={%d,%d}, pwstrFilename=[%s], dwFileSize=[%d]", __FUNCTION__, parentId.category, parentId.id, (const char *)CW2A(pwstrFilename), dwFileSize);

    // HarryWu, 2014.2.15
    // TODO: Post file content to server
    // pbBuffer, dwFileSize
    std::wstring basename = wcsrchr(pwstrFilename, _T('\\')) ? (wcsrchr(pwstrFilename, _T('\\')) + 1): (pwstrFilename);
    wchar_t szTempFile [MAX_PATH] = _T("");
    if (!Utility::GenerateTempFilePath(szTempFile, lengthof(szTempFile), basename.c_str()))
        return S_FALSE;

    // Write content to temp file, 
    // and then upload this temp file.
    OUTPUTLOG("%s(), [Stream]: Generate temporary file to `%s\'", __FUNCTION__, (const char *)CW2A(szTempFile));

    FILE * fout = _wfopen(szTempFile, _T("wb"));
    int byteswritten = fwrite(pbBuffer, 1, dwFileSize, fout);
    if (byteswritten != dwFileSize){
        fclose(fout); return S_FALSE;
    }
    fclose(fout);

    OUTPUTLOG("%s(), write [%s] with %d bytes", __FUNCTION__, (const char *)CW2A(szTempFile), dwFileSize);

    // Upload Temporary file to server.
    if (!GetProto(pArchive)->UploadFile(pArchive, parentId, szTempFile, basename.c_str()))
        return S_FALSE;

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
    std::wstring basename = wcsrchr(pwstrFilename, _T('\\')) ? (wcsrchr(pwstrFilename, _T('\\')) + 1): (pwstrFilename);
    wchar_t szTempFile [MAX_PATH] = _T("");
    if (!Utility::GenerateTempFilePath(szTempFile, lengthof(szTempFile), basename.c_str()))
        return S_FALSE;

	// Download remote file to local temp file,
	// and then read content from this file.
	GetProto(pArchive)->DownloadFile(pArchive, itemId, szTempFile);
    OUTPUTLOG("%s(), [Stream]: Generate temporary file to `%s\'", __FUNCTION__, (const char *)CW2A(szTempFile));

	// Write content to temp file, 
	// and then upload this temp file.
	FILE * fin = _wfopen(szTempFile, _T("rb"));
	if (fin != NULL){
		fseek(fin, 0, SEEK_END);
		*pdwFileSize = ftell(fin);
        fseek(fin, 0, SEEK_SET);
	}

	OUTPUTLOG("%s(), read [%s] with %d bytes", __FUNCTION__, (const char *)CW2A(szTempFile), *pdwFileSize);
	DMMalloc((LPBYTE*)ppbBuffer, *pdwFileSize);
	if (*ppbBuffer && fin){
		int readbytes = fread(*ppbBuffer, 1, *pdwFileSize, fin);
        if (readbytes == *pdwFileSize) {
            fclose(fin); return S_OK;
        }
	}

	if (fin) fclose(fin);
	return S_FALSE;
}

HRESULT DMDownload(TAR_ARCHIVE * pArchive, LPCWSTR pwstrLocalDir, RemoteId itemId, BOOL isFolder, BOOL removeSource)
{
	CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);

	OUTPUTLOG("%s(), local=`%s\' remote=[%d:%d], flag[`%s\'], [%s]"
		, __FUNCTION__
		, (const char *)CW2A(pwstrLocalDir)
		, itemId.category
		, itemId.id
		, removeSource ? "RemoveSource" : "KeepSource"
        , isFolder ? "Folder" : "File");

    if (isFolder){
        if (!GetProto(pArchive)->DownloadFolder(pArchive, itemId, pwstrLocalDir))
            return S_FALSE;
    }else{
        if (!GetProto(pArchive)->DownloadFile(pArchive, itemId, pwstrLocalDir))
            return S_FALSE;
    }

	return S_OK;
}

HRESULT DMUpload(TAR_ARCHIVE * pArchive, LPCWSTR pwstrLocalPath, RemoteId viewId, BOOL removeSource)
{
	CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);

	OUTPUTLOG("%s(), local=`%s\' remote=[%d:%d], flag[`%s\']"
		, __FUNCTION__
		, (const char *)CW2A(pwstrLocalPath)
		, viewId.category
		, viewId.id
		, removeSource ? "RemoveSource" : "KeepSource");

    if (PathIsDirectory(pwstrLocalPath)){
        if (!GetProto(pArchive)->UploadFolder(pArchive, viewId, pwstrLocalPath))
            return S_FALSE;
    }else{
        if (!GetProto(pArchive)->UploadFile(pArchive, viewId, pwstrLocalPath, NULL))
            return S_FALSE;
    }

	return S_OK;
}

HRESULT DMSelect(TAR_ARCHIVE * pArchive, RemoteId itemId, BOOL selected, BOOL isFolder)
{
    CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);

    OUTPUTLOG("%s() [%s] [%d:%d]", __FUNCTION__, selected ? "Select" : "CancelSelect", itemId.category, itemId.id);

    if (!GetProto(pArchive)->Select(pArchive, itemId, selected, isFolder))
        return S_FALSE;

    return S_OK;
}

HRESULT DMGetCustomColumns(TAR_ARCHIVE * pArchive, RemoteId viewId, LPWSTR pwstrColumnList, int maxcch)
{
    CComCritSecLock<CComCriticalSection> lock(pArchive->csLock);
    
    OUTPUTLOG("%s() ViewId=[%d:%d]", __FUNCTION__, viewId.category, viewId.id);

    if (!GetProto(pArchive)->GetColumnInfo(pArchive, viewId, pwstrColumnList, maxcch))
        return S_FALSE;

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

