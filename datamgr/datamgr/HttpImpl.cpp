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
#include "HttpImpl.h"

HttpImpl::HttpImpl()
{

}

HttpImpl::~HttpImpl()
{

}

BOOL HttpImpl::Login(TAR_ARCHIVE * pArchive)
{
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
    if (!Utility::HttpRequest(url, response))
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


BOOL HttpImpl::GetTopPublic(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPublic)
{
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
    if (!Utility::HttpRequest(url, response))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
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

BOOL HttpImpl::GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<RFS_FIND_DATA> & topPersonal)
{
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
    if (!Utility::HttpRequest(url, response))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
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

BOOL HttpImpl::GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<RFS_FIND_DATA> & childFolders)
{
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
    if (!Utility::HttpRequest(url, response))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
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

BOOL HttpImpl::GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<RFS_FIND_DATA> & childFiles)
{
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
    if (!Utility::HttpRequest(url, response))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
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

BOOL HttpImpl::DeleteItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL isFolder)
{
    // HarryWu, 2014.2.28
    // Here begin of HttpRequest, directly to remote server
    if (pArchive->context->AccessToken[0] == _T('\0')){
        if (!Login(pArchive)){
            OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
            return FALSE;
        }
    }
    // "http://localhost/EDoc2WebApi/api/Doc/FolderRead/DeleteFolderList?token=7c370032-078b-41df-bd1e-0ba5cdeb9976&intoRecycleBin=true&folderIds=562296484"
    // "http://localhost/EDoc2WebApi/api/Doc/FileRead/DeleteFileList?token=7c370032-078b-41df-bd1e-0ba5cdeb9976&intoRecycleBin=true&fileIds=562296484"
    wchar_t url [MaxUrlLength] = _T("");
    wsprintf(url
        , isFolder 
        ?  _T("%s/EDoc2WebApi/api/Doc/FolderRead/DeleteFolderList?token=%s&intoRecycleBin=true&folderIds=%d")
        :  _T("%s/EDoc2WebApi/api/Doc/FileRead/DeleteFileList?token=%s&intoRecycleBin=true&fileIds=%d")
        , pArchive->context->service, pArchive->context->AccessToken, itemId.id);

    // {"$id":"1","FolderId":1,"AreaId":1,"ParentFolderId":1,"FolderCode":"[PublicRoot]","FolderSortOrder":0,"FolderName":"PublicRoot","FolderPath":"1","FolderNamePath":"PublicRoot","FolderSize":0,"FolderMaxFolderSize":0,"FolderAlertSize":0,"FolderMaxFileSize":0,"FolderForbiddenFileExtensions":null,"FolderChildFoldersCount":0,"FolderChildFilesCount":0,"SecurityLevelId":0,"FolderState":0,"FolderLockCount":0,"FolderCreateTime":"2009-05-20T20:45:39.937","FolderCreateOperator":0,"FolderModifyTime":"2009-05-20T20:45:39.937","FolderModifyOperator":0,"FolderArchiveTime":"0001-01-01T00:00:00","FolderArchiveOperator":0,"FolderCurVerId":0,"FolderNewestVerId":0,"FolderType":1,"FolderGuid":"97af3663-8793-45f5-a2c5-4f5ad537353f","FolderOwnerId":0,"IsDeleted":false,"FolderDeleteTime":"0001-01-01T00:00:00","FolderDeleteOperator":0,"FolderIsCascadeDelete":false,"RelativePath":"PublicRoot","IsArcFolder":false,"HasBoundStorageArea":false,"Children":null}
    std::wstring response;
    if (!Utility::HttpRequest(url, response))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    if (response.compare(_T("0")) == 0)
        return TRUE;

    return FALSE;
}

BOOL HttpImpl::RenameItem(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * newName, BOOL isFolder)
{
    // HarryWu, 2014.2.28
    // Here begin of HttpRequest, directly to remote server
    if (pArchive->context->AccessToken[0] == _T('\0')){
        if (!Login(pArchive)){
            OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
            return FALSE;
        }
    }
    // "http://localhost/EDoc2WebApi/api/Doc/FolderRead/ChangeFolderName?token=7c370032-078b-41df-bd1e-0ba5cdeb9976&folderId=562296484&newName=newFolderName"
    // "http://localhost/EDoc2WebApi/api/Doc/FileRead/ChanageFileName?token=7c370032-078b-41df-bd1e-0ba5cdeb9976&fileId=1023596&newFileName=newFilename&newRemark="
    wchar_t url [MaxUrlLength] = _T("");
    std::string strNewName = Utility::UrlEncode((const char *)CW2A(newName));
    wsprintf(url
        , isFolder 
        ?  _T("%s/EDoc2WebApi/api/Doc/FolderRead/ChangeFolderName?token=%s&folderId=%d&newName=%s")
        :  _T("%s/EDoc2WebApi/api/Doc/FileRead/ChanageFileName?token=%s&fileId=%d&newFileName=%s&newRemark=NAN")
        , pArchive->context->service, pArchive->context->AccessToken, itemId.id, (const wchar_t *)CA2W(strNewName.c_str()));

    // {"$id":"1","FolderId":1,"AreaId":1,"ParentFolderId":1,"FolderCode":"[PublicRoot]","FolderSortOrder":0,"FolderName":"PublicRoot","FolderPath":"1","FolderNamePath":"PublicRoot","FolderSize":0,"FolderMaxFolderSize":0,"FolderAlertSize":0,"FolderMaxFileSize":0,"FolderForbiddenFileExtensions":null,"FolderChildFoldersCount":0,"FolderChildFilesCount":0,"SecurityLevelId":0,"FolderState":0,"FolderLockCount":0,"FolderCreateTime":"2009-05-20T20:45:39.937","FolderCreateOperator":0,"FolderModifyTime":"2009-05-20T20:45:39.937","FolderModifyOperator":0,"FolderArchiveTime":"0001-01-01T00:00:00","FolderArchiveOperator":0,"FolderCurVerId":0,"FolderNewestVerId":0,"FolderType":1,"FolderGuid":"97af3663-8793-45f5-a2c5-4f5ad537353f","FolderOwnerId":0,"IsDeleted":false,"FolderDeleteTime":"0001-01-01T00:00:00","FolderDeleteOperator":0,"FolderIsCascadeDelete":false,"RelativePath":"PublicRoot","IsArcFolder":false,"HasBoundStorageArea":false,"Children":null}
    std::wstring response;
    if (!Utility::HttpRequest(url, response))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    if (response.compare(_T("0")) == 0)
        return TRUE;

    return FALSE;
}

BOOL HttpImpl::CreateFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * folderName, RemoteId * retId)
{

    // HarryWu, 2014.2.28
    // Here begin of HttpRequest, directly to remote server
    if (pArchive->context->AccessToken[0] == _T('\0')){
        if (!Login(pArchive)){
            OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
            return FALSE;
        }
    }
    // "http://kb.edoc2.com/EDoc2WebApi/api/Doc/FolderRead/CreateFolder?token=52d98bfa-94df-4283-b930-ff84e10bb4e8&folderName={}&parentFolderId={}&folderCode={}&folderRemark={}"
    wchar_t url [MaxUrlLength] = _T("");
    std::string strFolderName = Utility::UrlEncode((const char *)CW2AEX<>(folderName, CP_UTF8));// UTF-8 and then URL encoded.
    wsprintf(url
        , _T("%s/EDoc2WebApi/api/Doc/FolderRead/CreateFolder?token=%s&folderName=%s&parentFolderId=%d&folderCode=0&folderRemark=NAN")
        , pArchive->context->service, pArchive->context->AccessToken, (const wchar_t *)CA2W(strFolderName.c_str()), parentId.id);

    // {"$id":"1","FolderId":1,"AreaId":1,"ParentFolderId":1,"FolderCode":"[PublicRoot]","FolderSortOrder":0,"FolderName":"PublicRoot","FolderPath":"1","FolderNamePath":"PublicRoot","FolderSize":0,"FolderMaxFolderSize":0,"FolderAlertSize":0,"FolderMaxFileSize":0,"FolderForbiddenFileExtensions":null,"FolderChildFoldersCount":0,"FolderChildFilesCount":0,"SecurityLevelId":0,"FolderState":0,"FolderLockCount":0,"FolderCreateTime":"2009-05-20T20:45:39.937","FolderCreateOperator":0,"FolderModifyTime":"2009-05-20T20:45:39.937","FolderModifyOperator":0,"FolderArchiveTime":"0001-01-01T00:00:00","FolderArchiveOperator":0,"FolderCurVerId":0,"FolderNewestVerId":0,"FolderType":1,"FolderGuid":"97af3663-8793-45f5-a2c5-4f5ad537353f","FolderOwnerId":0,"IsDeleted":false,"FolderDeleteTime":"0001-01-01T00:00:00","FolderDeleteOperator":0,"FolderIsCascadeDelete":false,"RelativePath":"PublicRoot","IsArcFolder":false,"HasBoundStorageArea":false,"Children":null}
    std::wstring response;
    if (!Utility::HttpRequest(url, response))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
        retId->category = parentId.category;
        int index = 0;
        Json::Value newFolderId = root[index].get("FolderId", -1);
        if (newFolderId.empty()) return FALSE;
        retId->id = newFolderId.asInt();
        if (retId->id != -1) return TRUE;
    }

    return FALSE;
}


/**
* Integrated method to upload file.
* NOTE: reserved for harry's prototype test.
*       you must not use this method.
*/
BOOL HttpImpl::Upload(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * tempFile)
{
    return TRUE;
}

/**
* Integrated method into shell to Download file.
* NOTE: reserved for harry's prototype test.
*       you must not use this method.
*/
BOOL HttpImpl::Download(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * tempFile)
{
    return TRUE;
}

BOOL HttpImpl::Select(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL selected, BOOL isFolder)
{
    return TRUE;
}

BOOL HttpImpl::GetColumnInfo(TAR_ARCHIVE * pArchive, wchar_t * pColumnInfo, int maxcch)
{
    return TRUE;
}