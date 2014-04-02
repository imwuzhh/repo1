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
#include <curl/curl.h>
#include "Edoc2Context.h"
#include "datamgr.h"
#include "Utility.h"
#include "JsonImpl.h"

JsonImpl::JsonImpl()
{

}

JsonImpl::~JsonImpl()
{

}

BOOL JsonImpl::Login(TAR_ARCHIVE * pArchive)
{
    // HarryWu, 2014.2.28
    // Json Format request, not in use now, 
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
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp))
        return FALSE;

    //TODO: Retrieve the accessToken.

    return TRUE;
}

BOOL JsonImpl::GetTopPublic(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPublic)
{
    // HarryWu, 2014.2.28
    // Json Format request, not in use now, 
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
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp))
        return FALSE;

    // TODO: Json result parsing.

    return TRUE;
}

BOOL JsonImpl::GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPersonal)
{
    // HarryWu, 2014.2.28
    // Json Format request, not in use now, 
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
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp))
        return FALSE;

    // TODO: Json result parsing.

    return TRUE;
}

BOOL JsonImpl::GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<VFS_FIND_DATA> & childFolders)
{
    // HarryWu, 2014.2.28
    // Json Format request, not in use now, 
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
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp))
        return FALSE;

    // TODO: Json Result parsing.

    return TRUE;    
}


BOOL JsonImpl::GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<VFS_FIND_DATA> & childFiles)
{
    // HarryWu, 2014.2.28
    // Json Format request, not in use now, 
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
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp))
        return FALSE;

    // TODO: Json result parsing.

    return TRUE;
}

BOOL JsonImpl::GetChildFolderAndFiles(TAR_ARCHIVE *pArchive, const RemoteId &folderId, std::list<VFS_FIND_DATA> &children, int PageSize, int PageNo, int *PageCount)
{
    // HarryWu, 2014.2.28
    // Json Format request, not in use now, 
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "GetChildFolderAndFiles";

    Json::Value parameters; 
    parameters["ParentId"] = (int)folderId.id;
    parameters["PageSize"] = (int)PageSize;
    parameters["PageNo"]   = (int)PageNo;

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    // Do Json Request
    std::wstring jsonResp;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp))
        return FALSE;

    // TODO: Json result parsing.
    return TRUE;
}

BOOL JsonImpl::GetDocInfo(TAR_ARCHIVE *pArchive, const RemoteId &folderId, std::list<std::wstring> & columns, std::list<VFS_FIND_DATA> &children, int PageSize, int PageNo, int *PageCount)
{
    // HarryWu, 2014.2.28
    // Json Format request, not in use now, 
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "GetDocInfo";

    Json::Value parameters; 
    parameters["ParentId"] = (int)folderId.id;
    parameters["PageSize"] = (int)PageSize;
    parameters["PageNo"]   = (int)PageNo;

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    // Do Json Request
    std::wstring jsonResp;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), jsonResp))
        return FALSE;

    // TODO: Json result parsing.
    return TRUE;
}

BOOL JsonImpl::DeleteItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL isFolder)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "DeleteItem";
    Json::Value idlist; idlist.append((int)itemId.id);
    Json::Value parameters; 
    parameters["idlist"] = idlist;
    parameters["isFolder"] = isFolder ? "True" : "False";

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}


BOOL JsonImpl::RenameItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * newName, BOOL isFolder)
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
    parameters["isFolder"] = isFolder ? "True" : "False";

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::CreateFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * folderName, RemoteId * retId)
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
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    // TODO: Json result parsing.
    retId->id = 0; // Parse it from response.

    return TRUE;
}

/**
* Call this route to inovke external tool to upload.
*/
BOOL JsonImpl::UploadFile(TAR_ARCHIVE * pArchive, const RemoteId & viewId, const wchar_t * localPath, const wchar_t * faceName)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "UploadFile";

    Json::Value parameters; 
    parameters["Destination"] = (int)viewId.id;
    parameters["Source"] = (const char *)CW2A(localPath);

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

/**
* Call this routine to invoke external tool to download.
*/
BOOL JsonImpl::DownloadFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "DownloadFile";

    Json::Value parameters; 
    parameters["Source"] = (int)itemId.id;
    parameters["Destination"] = (const char *)CW2A(localPath);

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::UploadFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentFolderId, const wchar_t * localPath)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "UploadFolder";

    Json::Value parameters; 
    parameters["Destination"] = (int)parentFolderId.id;
    parameters["Source"] = (const char *)CW2A(localPath);

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::DownloadFolder(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "DownloadFolder";

    Json::Value parameters; 
    parameters["Source"] = (int)itemId.id;
    parameters["Destination"] = (const char *)CW2A(localPath);

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE; 
}

BOOL JsonImpl::Select(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL selected, BOOL isFolder)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "Select";
    Json::Value parameters; 
    parameters["id"] = (int)itemId.id;
    parameters["selected"] = selected ? "True" : "False";
    parameters["isFolder"] = isFolder ? "True" : "False";
    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::GetColumnInfo(TAR_ARCHIVE * pArchive, const RemoteId & viewId, wchar_t * pColumnInfo, int maxcch)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "GetColumnInfo";
    Json::Value parameters; 
    parameters["id"] = (int)viewId.id;
    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    wcscpy_s(pColumnInfo, maxcch, _T("Attr1;Attr2;Attr3;Attr4"));
    return TRUE;
}

BOOL JsonImpl::PreviewFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "PreviewFile";

    Json::Value parameters; 
    parameters["id"] = (int)itemId.id;

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::OnShellViewCreated(TAR_ARCHIVE * pArchive, HWND shellViewWnd)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "OnShellViewCreated";

    Json::Value parameters; 
    // In windows, window handle is global handle of x-processes.
    parameters["WindowHandle"] = (int)shellViewWnd;

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::OnShellViewSized(TAR_ARCHIVE * pArchive, HWND shellViewWnd)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "OnShellViweSized";

    Json::Value parameters; 
    // In windows, window handle is global handle of x-processes.
    parameters["WindowHandle"] = (int)shellViewWnd;

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::OnShellViewRefreshed(TAR_ARCHIVE  * pArchive, HWND shellViewWnd)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "OnShellViewRefreshed";

    Json::Value parameters; 
    // In windows, window handle is global handle of x-processes.
    parameters["WindowHandle"] = (int)shellViewWnd;

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::OnShellViewClosing(TAR_ARCHIVE  * pArchive, HWND shellViewWnd)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "OnShellViewClosing";

    Json::Value parameters; 
    // In windows, window handle is global handle of x-processes.
    parameters["WindowHandle"] = (int)shellViewWnd;

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    return TRUE;
}

BOOL JsonImpl::FileExists(TAR_ARCHIVE * pArchive, const RemoteId & parentId, wchar_t * childName, VFS_FIND_DATA & childInfo)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "FileExists";

    Json::Value parameters; 
    // In windows, window handle is global handle of x-processes.
    parameters["parentId"] = (int)parentId.id;
    parameters["childName"]= (const char *)CW2AEX<>(childName, CP_UTF8);

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    // TODO: Parse result and fill in contents of <childInfo>
    return TRUE;
}

BOOL JsonImpl::FolderExists(TAR_ARCHIVE * pArchive, const RemoteId & parentId, wchar_t * childName, VFS_FIND_DATA & childInfo)
{
    Json::StyledWriter writer;
    Json::Value  root;
    root ["Server"] = (const char *)CW2A(pArchive->context->service);
    root ["Port"]   = 60684;
    root ["Version"]= "1.0.0.1";
    root ["Token" ] = (const char *)CW2A(pArchive->context->AccessToken);
    root ["Method"] = "FolderExists";

    Json::Value parameters; 
    // In windows, window handle is global handle of x-processes.
    parameters["parentId"] = (int)parentId.id;
    parameters["childName"]= (const char *)CW2AEX<>(childName, CP_UTF8);

    root ["Params"] = parameters;
    std::string jsonString = writer.write(root);

    std::wstring response;
    if (!Utility::JsonRequest((const wchar_t *)CA2W(jsonString.c_str()), response))
        return FALSE;

    // TODO: Parse result and fill in contents of <childInfo>
    return TRUE;
}