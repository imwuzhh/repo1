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

#include "datamgr.h"
#include "Utility.h"
#include <json/json.h>
#include <curl/curl.h>

#define SKIP_PEER_VERIFICATION
#define SKIP_HOSTNAME_VERIFICATION

Utility::Utility(void)
{
}

Utility::~Utility(void)
{
}

BOOL Utility::Login(TAR_ARCHIVE * pArchive)
{
	// HarryWu, 2014.2.28
	// Json Format request, not in use now, 
	{
		Json::StyledWriter writer;
		Json::Value  root;
		root ["Server"] = (const char *)CW2A(pArchive->context->service);
		root ["Port"]   = 60684;
		root ["Version"]= "1.0.0.1";
		root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
		root ["Method"] = "Login";
		// Now, you use this pair to login in remote server for me, and
		// return the AccessToken to me.
		Json::Value parameters; 
		parameters["username"] = (const char * )CW2A(pArchive->context->username);
		parameters["password"] = (const char * )CW2A(pArchive->context->password);
		root ["Params"] = parameters;
		std::string jsonString = writer.write(root);

		// Do Json Request
		std::wstring jsonResp;
		JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp);
	}
	// HarryWu, 2014.2.28
	// Here begin of HttpRequest, directly to remote server
	const wchar_t * svcaddr = pArchive->context->service;
	const wchar_t * user = pArchive->context->username;
	const wchar_t * pass = pArchive->context->password;
	wchar_t * accessToken= pArchive->context->AccessToken;
	int maxTokenLength   = lengthof(pArchive->context->AccessToken);

	// "http://192.168.253.242/EDoc2WebApi/api/Org/UserRead/UserLogin?userLoginName=admin&password=edoc2"
	wchar_t url [MaxUrlLength] = _T("");
	wsprintf(url
		, _T("%s/EDoc2WebApi/api/Org/UserRead/UserLogin?userLoginName=%s&password=%s")
		, svcaddr, user, pass);

	std::wstring response;
	if (!HttpRequest(url, response))
		return FALSE;

	if (response.empty())
		return FALSE;

	// HarryWu, 2014.2.21
	// if token have ", TopPublic will be OK,
	// but, TopPersonal will fail.
	int position = response.find(_T("\""));
	while ( position != std::wstring::npos ){
		response.replace( position, 1, _T(""));
		position = response.find(_T("\""), position + 1 );
	}

	wcscpy_s(accessToken, maxTokenLength, response.c_str());
	return TRUE;
}

BOOL Utility::GetTopPublic(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPublic)
{
	// HarryWu, 2014.2.28
	// Json Format request, not in use now, 
	{
		Json::StyledWriter writer;
		Json::Value  root;
		root ["Server"] = (const char *)CW2A(pArchive->context->service);
		root ["Port"]   = 60684;
		root ["Version"]= "1.0.0.1";
		root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
		root ["Method"] = "GetTopPublic";
		Json::Value parameters; parameters["userParam"] = (int)0;// just for test, useless.
		root ["Params"] = parameters;
		std::string jsonString = writer.write(root);

		// Do Json Request
		std::wstring jsonResp;
		JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp);
	}
	// HarryWu, 2014.2.28
	// Here begin of HttpRequest, directly to remote server
	if (pArchive->context->AccessToken[0] == _T('\0')){
		if (!Login(pArchive)){
			OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
			return FALSE;
		}
	}

	// "http://192.168.253.242/EDoc2WebApi/api/Doc/FolderRead/GetTopPublicFolder?token=2ef01717-6f61-4592-a606-7292f3cb5a57"
	wchar_t url [MaxUrlLength] = _T("");
	wsprintf(url
		, _T("%s/EDoc2WebApi/api/Doc/FolderRead/GetTopPublicFolder?token=%s")
		, pArchive->context->service, pArchive->context->AccessToken);

	// {"$id":"1","FolderId":1,"AreaId":1,"ParentFolderId":1,"FolderCode":"[PublicRoot]","FolderSortOrder":0,"FolderName":"PublicRoot","FolderPath":"1","FolderNamePath":"PublicRoot","FolderSize":0,"FolderMaxFolderSize":0,"FolderAlertSize":0,"FolderMaxFileSize":0,"FolderForbiddenFileExtensions":null,"FolderChildFoldersCount":0,"FolderChildFilesCount":0,"SecurityLevelId":0,"FolderState":0,"FolderLockCount":0,"FolderCreateTime":"2009-05-20T20:45:39.937","FolderCreateOperator":0,"FolderModifyTime":"2009-05-20T20:45:39.937","FolderModifyOperator":0,"FolderArchiveTime":"0001-01-01T00:00:00","FolderArchiveOperator":0,"FolderCurVerId":0,"FolderNewestVerId":0,"FolderType":1,"FolderGuid":"97af3663-8793-45f5-a2c5-4f5ad537353f","FolderOwnerId":0,"IsDeleted":false,"FolderDeleteTime":"0001-01-01T00:00:00","FolderDeleteOperator":0,"FolderIsCascadeDelete":false,"RelativePath":"PublicRoot","IsArcFolder":false,"HasBoundStorageArea":false,"Children":null}
	std::wstring response;
    if (!HttpRequest(url, response))
		return FALSE;

	if (response.empty())
		return FALSE;

	Json::Value root;
	Json::Reader reader; 
	if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
		OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));
		RFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

		Json::Value folderId = root.get("FolderId", 0);
		rfd.dwId.id = folderId.asInt();
		rfd.dwId.category = PublicCat;
		if (rfd.dwId.id == 0) return FALSE;

		Json::Value folderName = root.get("FolderName", "");
		std::string test = folderName.asString();
		if (test.empty()) return FALSE;
		wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));
		
		rfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		SYSTEMTIME stime; GetSystemTime(&stime);
		FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
		rfd.ftLastAccessTime = rfd.ftLastWriteTime = rfd.ftCreationTime = ftime;

		topPublic.push_back(rfd);
	}
	return TRUE;
}

BOOL Utility::GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPersonal)
{
	// HarryWu, 2014.2.28
	// Json Format request, not in use now, 
	{
		Json::StyledWriter writer;
		Json::Value  root;
		root ["Server"] = (const char *)CW2A(pArchive->context->service);
		root ["Port"]   = 60684;
		root ["Version"]= "1.0.0.1";
		root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
		root ["Method"] = "GetTopPersonal";
		Json::Value parameters; parameters["userParam"] = (int)0;// just for test, useless.
		root ["Params"] = parameters;
		std::string jsonString = writer.write(root);

		// Do Json Request
		std::wstring jsonResp;
		JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp);
	}
	// HarryWu, 2014.2.28
	// Here begin of HttpRequest, directly to remote server
	if (pArchive->context->AccessToken[0] == _T('\0')){
		if (!Login(pArchive)){
			OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
			return FALSE;
		}
	}
	// "http://192.168.253.242/EDoc2WebApi/api/Doc/FolderRead/GetTopPersonalFolder?token=2ef01717-6f61-4592-a606-7292f3cb5a57"
	wchar_t url [MaxUrlLength] = _T("");
	wsprintf(url
		, _T("%s/EDoc2WebApi/api/Doc/FolderRead/GetTopPersonalFolder?token=%s")
		, pArchive->context->service, pArchive->context->AccessToken);

	// {"$id":"1","FolderId":1,"AreaId":1,"ParentFolderId":1,"FolderCode":"[PublicRoot]","FolderSortOrder":0,"FolderName":"PublicRoot","FolderPath":"1","FolderNamePath":"PublicRoot","FolderSize":0,"FolderMaxFolderSize":0,"FolderAlertSize":0,"FolderMaxFileSize":0,"FolderForbiddenFileExtensions":null,"FolderChildFoldersCount":0,"FolderChildFilesCount":0,"SecurityLevelId":0,"FolderState":0,"FolderLockCount":0,"FolderCreateTime":"2009-05-20T20:45:39.937","FolderCreateOperator":0,"FolderModifyTime":"2009-05-20T20:45:39.937","FolderModifyOperator":0,"FolderArchiveTime":"0001-01-01T00:00:00","FolderArchiveOperator":0,"FolderCurVerId":0,"FolderNewestVerId":0,"FolderType":1,"FolderGuid":"97af3663-8793-45f5-a2c5-4f5ad537353f","FolderOwnerId":0,"IsDeleted":false,"FolderDeleteTime":"0001-01-01T00:00:00","FolderDeleteOperator":0,"FolderIsCascadeDelete":false,"RelativePath":"PublicRoot","IsArcFolder":false,"HasBoundStorageArea":false,"Children":null}
	std::wstring response;
	if (!HttpRequest(url, response))
		return FALSE;

	if (response.empty())
		return FALSE;

	Json::Value root;
	Json::Reader reader; 
	if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
		OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));
		RFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

		Json::Value folderId = root.get("FolderId", 0);
		rfd.dwId.id = folderId.asInt();
		rfd.dwId.category = PersonCat;
		if (rfd.dwId.id == 0) return FALSE;

		Json::Value folderName = root.get("FolderName", "");
		std::string test = folderName.asString();
		if (test.empty()) return FALSE;
		wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

		rfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		SYSTEMTIME stime; GetSystemTime(&stime);
		FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
		rfd.ftLastAccessTime = rfd.ftLastWriteTime = rfd.ftCreationTime = ftime;

		topPersonal.push_back(rfd);
	}
	return TRUE;
}

BOOL Utility::GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<RFS_FIND_DATA> & childFolders)
{
	// HarryWu, 2014.2.28
	// Json Format request, not in use now, 
	{
		Json::StyledWriter writer;
		Json::Value  root;
		root ["Server"] = (const char *)CW2A(pArchive->context->service);
		root ["Port"]   = 60684;
		root ["Version"]= "1.0.0.1";
		root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
		root ["Method"] = "GetChildFolders";
		Json::Value parameters; parameters["ParentId"] = (int)remoteId.id;
		root ["Params"] = parameters;
		std::string jsonString = writer.write(root);

		// Do Json Request
		std::wstring jsonResp;
		JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp);
	}
	// HarryWu, 2014.2.28
	// Here begin of HttpRequest, directly to remote server
	if (pArchive->context->AccessToken[0] == _T('\0')){
		if (!Login(pArchive)){
			OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
			return FALSE;
		}
	}
	// "http://192.168.253.242/EDoc2WebApi/api/Doc/FolderRead/GetChildFolderListByFolderId?token=2ef01717-6f61-4592-a606-7292f3cb5a57&folderId=16086"
	wchar_t url [MaxUrlLength] = _T("");
	wsprintf(url
		, _T("%s/EDoc2WebApi/api/Doc/FolderRead/GetChildFolderListByFolderId?token=%s&folderId=%d")
		, pArchive->context->service, pArchive->context->AccessToken, remoteId.id);

	// {"$id":"1","FolderId":1,"AreaId":1,"ParentFolderId":1,"FolderCode":"[PublicRoot]","FolderSortOrder":0,"FolderName":"PublicRoot","FolderPath":"1","FolderNamePath":"PublicRoot","FolderSize":0,"FolderMaxFolderSize":0,"FolderAlertSize":0,"FolderMaxFileSize":0,"FolderForbiddenFileExtensions":null,"FolderChildFoldersCount":0,"FolderChildFilesCount":0,"SecurityLevelId":0,"FolderState":0,"FolderLockCount":0,"FolderCreateTime":"2009-05-20T20:45:39.937","FolderCreateOperator":0,"FolderModifyTime":"2009-05-20T20:45:39.937","FolderModifyOperator":0,"FolderArchiveTime":"0001-01-01T00:00:00","FolderArchiveOperator":0,"FolderCurVerId":0,"FolderNewestVerId":0,"FolderType":1,"FolderGuid":"97af3663-8793-45f5-a2c5-4f5ad537353f","FolderOwnerId":0,"IsDeleted":false,"FolderDeleteTime":"0001-01-01T00:00:00","FolderDeleteOperator":0,"FolderIsCascadeDelete":false,"RelativePath":"PublicRoot","IsArcFolder":false,"HasBoundStorageArea":false,"Children":null}
	std::wstring response;
	if (!HttpRequest(url, response))
		return FALSE;

	if (response.empty())
		return FALSE;

	Json::Value root;
	Json::Reader reader; 
	if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
		OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));
		for (size_t index = 0; index < root.size(); index ++){
			RFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

			Json::Value folderId = root[index].get("FolderId", 0);
			rfd.dwId.id = folderId.asInt();
			rfd.dwId.category = remoteId.category;
			if (rfd.dwId.id == 0) return FALSE;

			Json::Value folderName = root[index].get("FolderName", "");
			std::string test = folderName.asString();
			if (test.empty()) return FALSE;
			wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

			rfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			SYSTEMTIME stime; GetSystemTime(&stime);
			FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
			rfd.ftLastAccessTime = rfd.ftLastWriteTime = rfd.ftCreationTime = ftime;

			childFolders.push_back(rfd);
		}
	}
	return TRUE;
}

BOOL Utility::GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<RFS_FIND_DATA> & childFiles)
{
	// HarryWu, 2014.2.28
	// Json Format request, not in use now, 
	{
		Json::StyledWriter writer;
		Json::Value  root;
		root ["Server"] = (const char *)CW2A(pArchive->context->service);
		root ["Port"]   = 60684;
		root ["Version"]= "1.0.0.1";
		root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
		root ["Method"] = "GetChildFiles";
		Json::Value parameters; parameters["ParentId"] = (int)remoteId.id;
		root ["Params"] = parameters;
		std::string jsonString = writer.write(root);

		// Do Json Request
		std::wstring jsonResp;
		JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp);
	}
	// HarryWu, 2014.2.28
	// Here begin of HttpRequest, directly to remote server
	if (pArchive->context->AccessToken[0] == _T('\0')){
		if (!Login(pArchive)){
			OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
			return FALSE;
		}
	}
	// "http://192.168.253.242/EDoc2WebApi/api/Doc/FileRead/GetChildFileListByFolderId?token=c8132990-f885-440f-9c5b-88fa685d2482&folderId=16415"
	wchar_t url [MaxUrlLength] = _T("");
	wsprintf(url
		, _T("%s/EDoc2WebApi/api/Doc/FileRead/GetChildFileListByFolderId?token=%s&folderId=%d")
		, pArchive->context->service, pArchive->context->AccessToken, remoteId.id);

	// {"$id":"1","FolderId":1,"AreaId":1,"ParentFolderId":1,"FolderCode":"[PublicRoot]","FolderSortOrder":0,"FolderName":"PublicRoot","FolderPath":"1","FolderNamePath":"PublicRoot","FolderSize":0,"FolderMaxFolderSize":0,"FolderAlertSize":0,"FolderMaxFileSize":0,"FolderForbiddenFileExtensions":null,"FolderChildFoldersCount":0,"FolderChildFilesCount":0,"SecurityLevelId":0,"FolderState":0,"FolderLockCount":0,"FolderCreateTime":"2009-05-20T20:45:39.937","FolderCreateOperator":0,"FolderModifyTime":"2009-05-20T20:45:39.937","FolderModifyOperator":0,"FolderArchiveTime":"0001-01-01T00:00:00","FolderArchiveOperator":0,"FolderCurVerId":0,"FolderNewestVerId":0,"FolderType":1,"FolderGuid":"97af3663-8793-45f5-a2c5-4f5ad537353f","FolderOwnerId":0,"IsDeleted":false,"FolderDeleteTime":"0001-01-01T00:00:00","FolderDeleteOperator":0,"FolderIsCascadeDelete":false,"RelativePath":"PublicRoot","IsArcFolder":false,"HasBoundStorageArea":false,"Children":null}
	std::wstring response;
	if (!HttpRequest(url, response))
		return FALSE;

	if (response.empty())
		return FALSE;

	Json::Value root;
	Json::Reader reader; 
	if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
		OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));
		for (size_t index = 0; index < root.size(); index ++){
			RFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

			// Parse id
			Json::Value folderId = root[index].get("FileId", 0);
			rfd.dwId.id = folderId.asInt();
			rfd.dwId.category = remoteId.category;
			if (rfd.dwId.id == 0) return FALSE;

			// Parse Name
			Json::Value folderName = root[index].get("FileName", "");
			std::string test = folderName.asString();
			if (test.empty()) return FALSE;
			wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

			// Parse FileCurSize
			Json::Value fileSize = root[index].get("FileCurSize", 0);
			rfd.nFileSizeLow = fileSize.asInt();

			rfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			SYSTEMTIME stime; GetSystemTime(&stime);
			FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
			rfd.ftLastAccessTime = rfd.ftLastWriteTime = rfd.ftCreationTime = ftime;

			childFiles.push_back(rfd);
		}
	}
	return TRUE;
}

BOOL Utility::ConstructRecycleFolder(TAR_ARCHIVE * pArchive, RFS_FIND_DATA & recycleFolder)
{
	memset(&recycleFolder, 0, sizeof(recycleFolder));
	recycleFolder.dwId.category = RecycleCat;
	recycleFolder.dwId.id = 0;
	LoadLocalizedName(pArchive->context->localeName, _T("RecycleBin"), recycleFolder.cFileName, lengthof(recycleFolder.cFileName));
	recycleFolder.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	SYSTEMTIME stime; GetSystemTime(&stime);
	FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
	recycleFolder.ftLastAccessTime = recycleFolder.ftLastWriteTime = recycleFolder.ftCreationTime = ftime;
	return TRUE;
}

BOOL Utility::ConstructSearchFolder(TAR_ARCHIVE * pArchive, RFS_FIND_DATA & searchFolder)
{
	memset(&searchFolder, 0, sizeof(searchFolder));
	searchFolder.dwId.category = SearchCat;
	searchFolder.dwId.id = 0;
	LoadLocalizedName(pArchive->context->localeName, _T("SearchBin"), searchFolder.cFileName, lengthof(searchFolder.cFileName));
	searchFolder.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	SYSTEMTIME stime; GetSystemTime(&stime);
	FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
	searchFolder.ftLastAccessTime = searchFolder.ftLastWriteTime = searchFolder.ftCreationTime = ftime;
	return TRUE;
}

BOOL Utility::DeleteItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId)
{
	Json::StyledWriter writer;
	Json::Value  root;
	root ["Server"] = (const char *)CW2A(pArchive->context->service);
	root ["Port"]   = 60684;
	root ["Version"]= "1.0.0.1";
	root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
	root ["Method"] = "DeleteItem";
	Json::Value idlist; idlist.append((int)itemId.id);
	Json::Value parameters; parameters["idlist"] = idlist;
	root ["Params"] = parameters;
	std::string jsonString = writer.write(root);

	std::wstring response;
	Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response);
	return TRUE;
}

BOOL Utility::RenameItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * newName)
{
	Json::StyledWriter writer;
	Json::Value  root;
	root ["Server"] = (const char *)CW2A(pArchive->context->service);
	root ["Port"]   = 60684;
	root ["Version"]= "1.0.0.1";
	root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
	root ["Method"] = "Rename";
	Json::Value parameters; 
	parameters["id"] = (int)itemId.id;
	parameters["newName"] = (const char *)CW2A(newName);
	root ["Params"] = parameters;
	std::string jsonString = writer.write(root);

	std::wstring response;
	Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response);
	return TRUE;
}

BOOL Utility::CreateFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * folderName, RemoteId * retId)
{
	Json::StyledWriter writer;
	Json::Value  root;
	root ["Server"] = (const char *)CW2A(pArchive->context->service);
	root ["Port"]   = 60684;
	root ["Version"]= "1.0.0.1";
	root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
	root ["Method"] = "CreateFolder";
	Json::Value parameters; 
	parameters["ParentId"] = (int)parentId.id;
	parameters["FolderName"] = (const char *)CW2A(folderName);
	root ["Params"] = parameters;
	std::string jsonString = writer.write(root);

	std::wstring response;
	Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response);

	retId->id = rand(); // Parse it from response.

	return TRUE;
}

BOOL Utility::UploadFile(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * tempFile)
{
	Json::StyledWriter writer;
	Json::Value  root;
	root ["Server"] = (const char *)CW2A(pArchive->context->service);
	root ["Port"]   = 60684;
	root ["Version"]= "1.0.0.1";
	root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
	root ["Method"] = "UploadFile";
	Json::Value parameters; 
	parameters["ParentId"] = (int)parentId.id;
	parameters["FileName"] = (const char *)CW2A(tempFile);
	root ["Params"] = parameters;
	std::string jsonString = writer.write(root);

	std::wstring response;
	Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response);

	return TRUE;
}

BOOL Utility::DownloadFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * tempFile)
{
	Json::StyledWriter writer;
	Json::Value  root;
	root ["Server"] = (const char *)CW2A(pArchive->context->service);
	root ["Port"]   = 60684;
	root ["Version"]= "1.0.0.1";
	root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
	root ["Method"] = "DownloadFile";
	Json::Value parameters; 
	parameters["ItemId"] = (int)itemId.id;
	parameters["FileName"] = (const char *)CW2A(tempFile);
	root ["Params"] = parameters;
	std::string jsonString = writer.write(root);

	std::wstring response;
	Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response);

	return TRUE;
}

bool Utility::RfsComparation(const RFS_FIND_DATA & left, const RFS_FIND_DATA & right)
{
	return (left.dwId.category - right.dwId.category)<0;
	//return wcscmp(left.cFileName, right.cFileName);
}

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

int XferProgress(void *clientp,
				curl_off_t dltotal,
				curl_off_t dlnow,
				curl_off_t ultotal,
				curl_off_t ulnow)
{
	return 0;
	OUTPUTLOG("%s(%.2f), %d/%d Bytes", __FUNCTION__
		, 100.0f * dlnow / (dltotal ? dltotal : 1)
		, (unsigned int)dlnow
		, (unsigned int)dltotal);
	return 0; // non-zero will abort xfer.
}

BOOL Utility::HttpRequest(const wchar_t * requestUrl, std::wstring & response)
{
	CURL *curl;
	CURLcode res;

	DWORD dwBeginTicks = 0;

	struct MemoryStruct chunk;
	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if(curl) {
		// HarryWu, 2014.2.20
		// Carefully use CW2A() utility, type convertion.
		curl_easy_setopt(curl, CURLOPT_URL, (char *)CW2A(requestUrl));

#ifdef SKIP_PEER_VERIFICATION
		/*
		* If you want to connect to a site who isn't using a certificate that is
		* signed by one of the certs in the CA bundle you have, you can skip the
		* verification of the server's certificate. This makes the connection
		* A LOT LESS SECURE.
		*
		* If you have a CA cert for the server stored someplace else than in the
		* default bundle, then the CURLOPT_CAPATH option might come handy for
		* you.
		*/
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
		/*
		* If the site you're connecting to uses a different host name that what
		* they have mentioned in their server certificate's commonName (or
		* subjectAltName) fields, libcurl will refuse to connect. You can skip
		* this check, but this will make the connection less secure.
		*/
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

		/* send all data to this function  */ 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

		/* we pass our 'chunk' struct to the callback function */ 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

		// HarryWu, 2014.2.20
		// Setup timeout for connection & read/write, not for dns
		// Async Dns with timeout require c-ares utility.
		/*
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000L);
		*/
		/*		*/
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 500L);


		// HarryWu, 2014.2.20
		// Progress support.
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, XferProgress);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, 0L);

		/* Perform the request, res will get the return code */
		dwBeginTicks = GetTickCount();
		res = curl_easy_perform(curl);
		// HarryWu, 2014.2.20
		// printf() does not check type of var of CW2A().
		OUTPUTLOG("curl: request [%s] Cost [%d] ms", (char *)CW2A(requestUrl), GetTickCount() - dwBeginTicks);

		/* Check for errors */
		if(res != CURLE_OK){
			OUTPUTLOG("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}else{
			/*
			* Now, our chunk.memory points to a memory block that is chunk.size
			* bytes big and contains the remote file.
			*
			* Do something nice with it!
			*/ 
			OUTPUTLOG("%lu bytes retrieved\n", (long)chunk.size);
			// HarryWu, 2014.2.21
			// I guess that, you are using utf-8.
			response = (wchar_t *)CA2WEX<128>(chunk.memory, CP_UTF8);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	if(chunk.memory){
		free(chunk.memory);
	}

	curl_global_cleanup();

	return TRUE;
}

BOOL Utility::JsonRequest(const wchar_t * reqJson, std::wstring & response)
{
	OUTPUTLOG("%s(), JsonRequest: %s", __FUNCTION__, (const char *)CW2A(reqJson));
	return TRUE;
}

BOOL Utility::LoadLocalizedName(const wchar_t * localeName, const wchar_t * key, wchar_t * retVaule, int cchMax)
{
	if (key && retVaule) wcscpy_s(retVaule, cchMax, key);
	TiXmlDocument * xdoc = new TiXmlDocument();
	if (!xdoc) return FALSE;
	delete xdoc;
	return TRUE;
}