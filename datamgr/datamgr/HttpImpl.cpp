#pragma warning(disable:4996)
#include <Windows.h>
#include <ShlObj.h>
#include <atlbase.h>
#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
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
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
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


BOOL HttpImpl::GetTopPublic(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPublic)
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
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
        VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

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

BOOL HttpImpl::GetTopPersonal(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & topPersonal)
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
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
        VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

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

BOOL HttpImpl::GetChildFolders(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<VFS_FIND_DATA> & childFolders)
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
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
        for (size_t index = 0; index < root.size(); index ++){
            VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

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

BOOL HttpImpl::GetChildFiles(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<VFS_FIND_DATA> & childFiles)
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
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
        return FALSE;

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
        for (size_t index = 0; index < root.size(); index ++){
            VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

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

BOOL HttpImpl::GetChildFolderAndFiles(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount)
{
    // HarryWu, 2014.2.28
    // Here begin of HttpRequest, directly to remote server
    if (pArchive->context->AccessToken[0] == _T('\0')){
        if (!Login(pArchive)){
            OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
            return FALSE;
        }
    }
    // "http://192.168.253.242/EDoc2WebApi/api/Doc/FolderRead/GetDocListInfo?token=%s&folderId=%d&pageNum=%d&pageSize=%d"
    wchar_t url [MaxUrlLength] = _T("");
    wsprintf(url
        , _T("%s/EDoc2WebApi/api/Doc/FolderRead/GetDocListInfo?token=%s&folderId=%d&docViewId=0&pageNum=%d&pageSize=%d")
        , pArchive->context->service, pArchive->context->AccessToken, remoteId.id, PageNo, PageSize);

    std::wstring response;
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
        return FALSE;

    // Sample Response:
    // {"$id":"1","_infoItems":[{"$id":"2","_name":"basic:name","_title":"name","_remark":null,"_dataType":"docName","<Width>k__BackingField":0},{"$id":"3","_name":"basic:size","_title":"size","_remark":null,"_dataType":"fileSize","<Width>k__BackingField":0},{"$id":"4","_name":"basic:creator","_title":"creator","_remark":null,"_dataType":"string","<Width>k__BackingField":0},{"$id":"5","_name":"basic:editor","_title":"editor","_remark":null,"_dataType":"string","<Width>k__BackingField":0},{"$id":"6","_name":"basic:modifyTime","_title":"modifyTime","_remark":null,"_dataType":"datetime","<Width>k__BackingField":0},{"$id":"7","_name":"basic:version","_title":"version","_remark":null,"_dataType":"version","<Width>k__BackingField":0}],"_foldersInfo":[{"$id":"8","id":122,"name":"新建文件夹","code":"0","path":"2\\8\\122","parentFolderId":8,"childFolderCount":0,"childFileCount":0,"size":0,"maxFolderSize":0,"createTime":"2014-03-26T13:18:52.007","modifyTime":"2014-03-26T13:18:52.007","creatorId":2,"creatorName":"Administrator","editorId":2,"editorName":"Administrator","state":0,"folderType":1,"remark":"NAN","isDeleted":false,"uploadType":0,"favoriteId":"","favoriteType":"","isfavorite":false,"permission":-1}],"_filesInfo":[{"$id":"9","id":81,"name":"api.doc","code":"","path":"2\\8\\","parentFolderId":8,"size":81920,"curSize":81920,"createTime":"2014-03-24T16:21:47.08","modifyTime":"2014-03-24T16:21:47.08","creatorId":2,"creatorName":"Administrator","editorId":2,"editorName":"Administrator","state":0,"remark":"","curVerId":80,"curVerNumStr":"1.0","fileType":2,"currentOperatorId":0,"currentOperator":null,"lastVerId":80,"lastVerNumStr":"1.0","isDeleted":false,"securityLevelId":0,"effectiveTime":"","expirationTime":"","FileCipherText":false,"securityLevelName":"","isoState":0,"isfavorite":false,"favoriteId":"","favoriteType":"","permission":-1}],"_settings":{"$id":"10","pageNum":1,"pageSize":2,"totalCount":12,"viewMode":"List","docViewId":0},"_mustOnline":true,"_enabledOutSend":false,"_securityEnable":false,"_processStrategy":[],"_archiveStrategy":{"$id":"11"},"_isArchive":false,"_ciphertextOutwardPolicy":0}

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
        // Parse Folder array
        Json::Value _fodersInfo = root.get("_foldersInfo", "");
        if (!_fodersInfo.empty()){
            for (size_t index = 0; index < _fodersInfo.size(); index ++){
                VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

                // Parse id
                Json::Value folderId = _fodersInfo[index].get("id", 0);
                rfd.dwId.id = folderId.asInt();
                rfd.dwId.category = remoteId.category;
                if (rfd.dwId.id == 0) return FALSE;

                // Parse Name
                Json::Value folderName = _fodersInfo[index].get("name", "");
                std::string test = folderName.asString();
                if (test.empty()) return FALSE;
                wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

                // Parse FileCurSize
                Json::Value fileSize = _fodersInfo[index].get("size", 0);
                rfd.nFileSizeLow = fileSize.asInt();

                rfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                SYSTEMTIME stime; GetSystemTime(&stime);
                FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
                rfd.ftLastAccessTime = rfd.ftLastWriteTime = rfd.ftCreationTime = ftime;

                children.push_back(rfd);
            }
        }
        // Parse Files array
        Json::Value _filesInfo = root.get("_filesInfo", "");
        if (!_filesInfo.empty()){
            for (size_t index = 0; index < _filesInfo.size(); index ++){
                VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

                // Parse id
                Json::Value folderId = _filesInfo[index].get("id", 0);
                rfd.dwId.id = folderId.asInt();
                rfd.dwId.category = remoteId.category;
                if (rfd.dwId.id == 0) return FALSE;

                // Parse Name
                Json::Value folderName = _filesInfo[index].get("name", "");
                std::string test = folderName.asString();
                if (test.empty()) return FALSE;
                wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

                // Parse FileCurSize
                Json::Value fileSize = _filesInfo[index].get("size", 0);
                rfd.nFileSizeLow = fileSize.asInt();

                rfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                SYSTEMTIME stime; GetSystemTime(&stime);
                FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
                rfd.ftLastAccessTime = rfd.ftLastWriteTime = rfd.ftCreationTime = ftime;

                children.push_back(rfd);
            }
        }
        // Parse _settings.
        Json::Value _settings = root.get("_settings", "");
        if (!_settings.empty()){
            Json::Value JsonTotalCount = _settings.get("totalCount", 0);
            int itemCount = 0;
            if (!JsonTotalCount.empty()){
                itemCount = JsonTotalCount.asInt();
            }
            if (itemCount == 0)
                return FALSE;

            *PageCount = ( itemCount % PageSize ) ? (itemCount/PageSize + 1): (itemCount/PageSize);
        }
    }
    return TRUE;
}

BOOL HttpImpl::GetDocInfo(TAR_ARCHIVE * pArchive, const RemoteId & remoteId, std::list<std::wstring> & columns, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount)
{
    // HarryWu, 2014.2.28
    // Here begin of HttpRequest, directly to remote server
    if (pArchive->context->AccessToken[0] == _T('\0')){
        if (!Login(pArchive)){
            OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
            return FALSE;
        }
    }
    // "http://192.168.253.242/EDoc2WebApi/api/Doc/FolderRead/GetDocListInfo?token=%s&folderId=%d&pageNum=%d&pageSize=%d"
    wchar_t url [MaxUrlLength] = _T("");
    wsprintf(url
        , _T("%s/EDoc2WebApi/api/Doc/FolderRead/GetDocListInfo?token=%s&folderId=%d&docViewId=0&pageNum=%d&pageSize=%d")
        , pArchive->context->service, pArchive->context->AccessToken, remoteId.id, PageNo, PageSize);

    std::wstring response;
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
        return FALSE;

    // Sample Response:
    // {"$id":"1","_infoItems":[{"$id":"2","_name":"basic:name","_title":"name","_remark":null,"_dataType":"docName","<Width>k__BackingField":0},{"$id":"3","_name":"basic:size","_title":"size","_remark":null,"_dataType":"fileSize","<Width>k__BackingField":0},{"$id":"4","_name":"basic:creator","_title":"creator","_remark":null,"_dataType":"string","<Width>k__BackingField":0},{"$id":"5","_name":"basic:editor","_title":"editor","_remark":null,"_dataType":"string","<Width>k__BackingField":0},{"$id":"6","_name":"basic:modifyTime","_title":"modifyTime","_remark":null,"_dataType":"datetime","<Width>k__BackingField":0},{"$id":"7","_name":"basic:version","_title":"version","_remark":null,"_dataType":"version","<Width>k__BackingField":0}],"_foldersInfo":[{"$id":"8","id":122,"name":"新建文件夹","code":"0","path":"2\\8\\122","parentFolderId":8,"childFolderCount":0,"childFileCount":0,"size":0,"maxFolderSize":0,"createTime":"2014-03-26T13:18:52.007","modifyTime":"2014-03-26T13:18:52.007","creatorId":2,"creatorName":"Administrator","editorId":2,"editorName":"Administrator","state":0,"folderType":1,"remark":"NAN","isDeleted":false,"uploadType":0,"favoriteId":"","favoriteType":"","isfavorite":false,"permission":-1}],"_filesInfo":[{"$id":"9","id":81,"name":"api.doc","code":"","path":"2\\8\\","parentFolderId":8,"size":81920,"curSize":81920,"createTime":"2014-03-24T16:21:47.08","modifyTime":"2014-03-24T16:21:47.08","creatorId":2,"creatorName":"Administrator","editorId":2,"editorName":"Administrator","state":0,"remark":"","curVerId":80,"curVerNumStr":"1.0","fileType":2,"currentOperatorId":0,"currentOperator":null,"lastVerId":80,"lastVerNumStr":"1.0","isDeleted":false,"securityLevelId":0,"effectiveTime":"","expirationTime":"","FileCipherText":false,"securityLevelName":"","isoState":0,"isfavorite":false,"favoriteId":"","favoriteType":"","permission":-1}],"_settings":{"$id":"10","pageNum":1,"pageSize":2,"totalCount":12,"viewMode":"List","docViewId":0},"_mustOnline":true,"_enabledOutSend":false,"_securityEnable":false,"_processStrategy":[],"_archiveStrategy":{"$id":"11"},"_isArchive":false,"_ciphertextOutwardPolicy":0}

    if (response.empty())
        return FALSE;

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
        // Parse Folder array
        Json::Value _fodersInfo = root.get("_foldersInfo", "");
        if (!_fodersInfo.empty()){
            for (size_t index = 0; index < _fodersInfo.size(); index ++){
                VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

                // Parse id
                Json::Value folderId = _fodersInfo[index].get("id", 0);
                rfd.dwId.id = folderId.asInt();
                rfd.dwId.category = remoteId.category;
                if (rfd.dwId.id == 0) return FALSE;

                // Parse Name
                Json::Value folderName = _fodersInfo[index].get("name", "");
                std::string test = folderName.asString();
                if (test.empty()) return FALSE;
                wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

                // Parse FileCurSize
                Json::Value fileSize = _fodersInfo[index].get("size", 0);
                rfd.nFileSizeLow = fileSize.asInt();

                rfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

                SYSTEMTIME stime; GetSystemTime(&stime);
                Json::Value modifyTime = _fodersInfo[index].get("modifyTime", "0000-00-00T00:00:00.000");
                if (!modifyTime.empty()){
                    Utility::ParseTime((const wchar_t *)CA2WEX<>(modifyTime.asString().c_str(), CP_UTF8), &stime);
                }
                FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
                rfd.ftLastAccessTime = rfd.ftLastWriteTime = ftime;

                GetSystemTime(&stime);
                modifyTime = _fodersInfo[index].get("createTime", "0000-00-00T00:00:00.000");
                if (!modifyTime.empty()){
                    Utility::ParseTime((const wchar_t *)CA2WEX<>(modifyTime.asString().c_str(), CP_UTF8), &stime);
                }
                SystemTimeToFileTime(&stime, &ftime);
                rfd.ftCreationTime = ftime;

                std::string creatorName = _fodersInfo[index].get("creatorName", "").asString();
                wcscpy_s(rfd.creatorName, lengthof(rfd.creatorName), (const wchar_t *)CA2WEX<>(creatorName.c_str(), CP_UTF8));

                children.push_back(rfd);
            }
        }
        // Parse Files array
        Json::Value _filesInfo = root.get("_filesInfo", "");
        if (!_filesInfo.empty()){
            for (size_t index = 0; index < _filesInfo.size(); index ++){
                VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

                // Parse id
                Json::Value folderId = _filesInfo[index].get("id", 0);
                rfd.dwId.id = folderId.asInt();
                rfd.dwId.category = remoteId.category;
                if (rfd.dwId.id == 0) return FALSE;

                // Parse Name
                Json::Value folderName = _filesInfo[index].get("name", "");
                std::string test = folderName.asString();
                if (test.empty()) return FALSE;
                wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

                // Parse FileCurSize
                Json::Value fileSize = _filesInfo[index].get("size", 0);
                rfd.nFileSizeLow = fileSize.asInt();

                rfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                
                SYSTEMTIME stime; GetSystemTime(&stime);
                Json::Value modifyTime = _filesInfo[index].get("modifyTime", "0000-00-00T00:00:00.000");
                if (!modifyTime.empty()){
                    Utility::ParseTime((const wchar_t *)CA2WEX<>(modifyTime.asString().c_str(), CP_UTF8), &stime);
                }
                FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
                rfd.ftLastAccessTime = rfd.ftLastWriteTime = rfd.ftCreationTime = ftime;

                GetSystemTime(&stime);
                modifyTime = _filesInfo[index].get("createTime", "0000-00-00T00:00:00.000");
                if (!modifyTime.empty()){
                    Utility::ParseTime((const wchar_t *)CA2WEX<>(modifyTime.asString().c_str(), CP_UTF8), &stime);
                }
                SystemTimeToFileTime(&stime, &ftime);
                rfd.ftCreationTime = ftime;

                std::string creatorName = _filesInfo[index].get("creatorName", "").asString();
                wcscpy_s(rfd.creatorName, lengthof(rfd.creatorName), (const wchar_t *)CA2WEX<>(creatorName.c_str(), CP_UTF8));

                std::string lastVerNumStr = _filesInfo[index].get("lastVerNumStr", "").asString();
                wcscpy_s(rfd.versionStr, lengthof(rfd.versionStr), (const wchar_t *)CA2WEX<>(lastVerNumStr.c_str(), CP_UTF8));

                children.push_back(rfd);
            }
        }
        // Parse _settings.
        Json::Value _settings = root.get("_settings", "");
        if (!_settings.empty()){
            Json::Value JsonTotalCount = _settings.get("totalCount", 0);
            int itemCount = 0;
            if (!JsonTotalCount.empty()){
                itemCount = JsonTotalCount.asInt();
            }
            if (itemCount == 0)
                return FALSE;

            *PageCount = ( itemCount % PageSize ) ? (itemCount/PageSize + 1): (itemCount/PageSize);
        }

        // Parse _infoItems
        Json::Value _infoItems = root.get("_infoItems", "");
        if (!_infoItems.empty() && _infoItems.isArray()){
            for (int i = 0; i < _infoItems.size(); i++)
            {
                Json::Value item = _infoItems[i].get("_title", "");
                if (!item.empty()){
                    columns.push_back((const wchar_t *)CA2WEX<>(item.asString().c_str(), CP_UTF8));
                }
            }
        }
    }
    return TRUE;
}

BOOL HttpImpl::GetPagedRecycleItems(TAR_ARCHIVE * pArchive, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount)
{
    // HarryWu, 2014.2.28
    // Here begin of HttpRequest, directly to remote server
    if (pArchive->context->AccessToken[0] == _T('\0')){
        if (!Login(pArchive)){
            OUTPUTLOG("Failed to login, user=%s, pass=%s", (char *)CW2A(pArchive->context->username), (char *)CW2A(pArchive->context->password));
            return FALSE;
        }
    }
    // "http://7.25.25.112/edoc2v4/?token=97cc65e4-9e70-49e4-91cc-ac8453936a8b"
    // [PostData]
    // jueAction:postBack
    // jueUid:webClient
    // jueEvt:LoadPagedDocRecycle
    // pageNum:1
    // pageSize:0
    wchar_t url [MaxUrlLength] = _T("");
    wsprintf(url
        , _T("%s/edoc2v4/?token=%s")
        , pArchive->context->service, pArchive->context->AccessToken);

    wchar_t formxml [MaxUrlLength] = _T("");
    wsprintf(formxml
        , _T("<form>")
        _T("<input type='text' name='jueAction' value='postBack'></input>")
        _T("<input type='text' name='jueUid' value='webClient'></input>")
        _T("<input type='text' name='jueEvt' value='LoadPagedDocRecycle'></input>")
        _T("<input type='text' name='pageNum' value='%d'></input>")
        _T("<input type='text' name='pageSize' value='%d'></input>")
        _T("</form>")
        , PageNo, PageSize);

    // Prepare cookie.
    std::wstring cookie = _T("");
    cookie += _T("account=");  cookie += pArchive->context->username;    cookie += _T(";");
    cookie += _T("tkn=");      cookie += pArchive->context->AccessToken; cookie += _T(";");
    cookie += _T("token=");    cookie += pArchive->context->AccessToken; cookie += _T(";");
    cookie += _T("remeberAccount=true");cookie += _T(";");

    std::stringstream respstream;
    if (!Utility::HttpPostForm(url, formxml, cookie, respstream, pArchive->context->HttpTimeoutMs))
        return FALSE;

    std::string respstr = respstream.str();

    std::wstring response = (const wchar_t *)CA2WEX<>(respstr.c_str(), CP_UTF8);

    // Sample Response:
    // {"result":0,"pageNum":1,"pageSize":30,"totalCount":69,"folders":[{"id":113,"name":"新建文件夹 (1)","size":16826340,"deleteTime":new Date(1395646476007),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (1)","folderType":1},{"id":117,"name":"新建文件夹","size":4206585,"deleteTime":new Date(1395646475817),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹","folderType":1},{"id":112,"name":"1","size":8438763,"deleteTime":new Date(1395646475597),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\1","folderType":1},{"id":96,"name":"pdb","size":227897,"deleteTime":new Date(1395564222297),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\pdb","folderType":1},{"id":40,"name":"321简体","size":132103,"deleteTime":new Date(1395564221717),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\321简体","folderType":1},{"id":72,"name":"123是3","size":959981,"deleteTime":new Date(1395564220387),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\123是3","folderType":1},{"id":99,"name":"几","size":0,"deleteTime":new Date(1395396857870),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\123中文\\几","folderType":1},{"id":95,"name":"haoderen","size":0,"deleteTime":new Date(1395324669897),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\haoderen","folderType":1},{"id":92,"name":"新建文件夹 (1)","size":0,"deleteTime":new Date(1395208297690),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\新建文件夹 (1)","folderType":1},{"id":66,"name":"pdb","size":0,"deleteTime":new Date(1395121656737),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\pdb","folderType":1},{"id":93,"name":"新建文件夹 (2)","size":0,"deleteTime":new Date(1395046902390),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\新建文件夹 (2)","folderType":1},{"id":46,"name":"testfolderrename","size":0,"deleteTime":new Date(1395043644743),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\testfolderrename","folderType":1},{"id":94,"name":"新建文件夹 (3)","size":0,"deleteTime":new Date(1395037542607),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\新建文件夹 (3)","folderType":1},{"id":88,"name":"新建文件夹 (2)","size":0,"deleteTime":new Date(1395036910120),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\新建文件夹 (2)","folderType":1},{"id":86,"name":"新建文件sdd","size":0,"deleteTime":new Date(1395036895037),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\新建文件sdd","folderType":1},{"id":87,"name":"新建文件夹 (1)","size":0,"deleteTime":new Date(1395036589640),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\新建文件夹 (1)","folderType":1},{"id":89,"name":"新建文件夹 (3)","size":0,"deleteTime":new Date(1395036475940),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\新建文件夹 (3)","folderType":1},{"id":90,"name":"新建文件夹 (4)","size":0,"deleteTime":new Date(1395036386410),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\新建文件夹 (4)","folderType":1},{"id":56,"name":"新建文件夹 (10)","size":0,"deleteTime":new Date(1394701252897),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (10)","folderType":1},{"id":55,"name":"新建文件夹 (9)","size":0,"deleteTime":new Date(1394701252627),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (9)","folderType":1},{"id":54,"name":"新建文件夹 (8)","size":0,"deleteTime":new Date(1394701252533),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (8)","folderType":1},{"id":53,"name":"新建文件夹 (7)","size":0,"deleteTime":new Date(1394701252377),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (7)","folderType":1},{"id":52,"name":"新建文件夹 (6)","size":0,"deleteTime":new Date(1394701252303),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (6)","folderType":1},{"id":51,"name":"新建文件夹 (5)","size":0,"deleteTime":new Date(1394701252183),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (5)","folderType":1},{"id":50,"name":"新建文件夹 (4)","size":0,"deleteTime":new Date(1394701252063),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (4)","folderType":1},{"id":49,"name":"新建文件夹 (3)","size":0,"deleteTime":new Date(1394701252003),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (3)","folderType":1},{"id":48,"name":"新建文件夹 (2)","size":0,"deleteTime":new Date(1394701251867),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (2)","folderType":1},{"id":47,"name":"新建文件夹 (1)","size":0,"deleteTime":new Date(1394701251717),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"个人内容库\\Administrator\\新建文件夹 (1)","folderType":1},{"id":42,"name":"看io扩","size":0,"deleteTime":new Date(1394520798527),"deletedBy":"Administrator","creatorName":"Administrator","namePath":"企业内容库\\123中文\\看io扩","folderType":1}],"files":[{"id":95,"name":"雾里看花.jpg","size":16093,"deleteTime":new Date(1395897960107),"deletedBy":"test1","creatorName":"Administrator","namePath":"企业内容库\\新丼建文件夹","fileType":2}]}

    if (response.empty())
        return FALSE;

    while(true){
        size_t pBegin = response.find(_T("new Date("));
        if (pBegin != std::wstring::npos) {
            size_t pEnd = response.find(_T(")"), pBegin);
            if (pEnd != std::wstring::npos){
                response.replace(pBegin, pEnd - pBegin + 1, _T("\"2104-04-04T12:12:12.000\""));
            }
        } else break;
    }

    OUTPUTLOG("Json response: %s", (const char *)CW2A(response.c_str()));

    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(response.c_str()), root, false)){
        // Parse Folder array
        Json::Value _fodersInfo = root.get("folders", "");
        if (!_fodersInfo.empty()){
            for (size_t index = 0; index < _fodersInfo.size(); index ++){
                VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

                // Parse id
                Json::Value folderId = _fodersInfo[index].get("id", 0);
                rfd.dwId.id = folderId.asInt();
                rfd.dwId.category = RecycleCat;
                if (rfd.dwId.id == 0) return FALSE;

                // Parse Name
                Json::Value folderName = _fodersInfo[index].get("name", "");
                std::string test = folderName.asString();
                if (test.empty()) return FALSE;
                wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

                // Parse FileCurSize
                Json::Value fileSize = _fodersInfo[index].get("size", 0);
                rfd.nFileSizeLow = fileSize.asInt();

                rfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

                SYSTEMTIME stime; GetSystemTime(&stime);
                Json::Value modifyTime = _fodersInfo[index].get("modifyTime", "0000-00-00T00:00:00.000");
                if (!modifyTime.empty()){
                    Utility::ParseTime((const wchar_t *)CA2WEX<>(modifyTime.asString().c_str(), CP_UTF8), &stime);
                }
                FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
                rfd.ftLastAccessTime = rfd.ftLastWriteTime = ftime;

                GetSystemTime(&stime);
                modifyTime = _fodersInfo[index].get("createTime", "0000-00-00T00:00:00.000");
                if (!modifyTime.empty()){
                    Utility::ParseTime((const wchar_t *)CA2WEX<>(modifyTime.asString().c_str(), CP_UTF8), &stime);
                }
                SystemTimeToFileTime(&stime, &ftime);
                rfd.ftCreationTime = ftime;

                std::string creatorName = _fodersInfo[index].get("creatorName", "").asString();
                wcscpy_s(rfd.creatorName, lengthof(rfd.creatorName), (const wchar_t *)CA2WEX<>(creatorName.c_str(), CP_UTF8));

                children.push_back(rfd);
            }
        }
        // Parse Files array
        Json::Value _filesInfo = root.get("files", "");
        if (!_filesInfo.empty()){
            for (size_t index = 0; index < _filesInfo.size(); index ++){
                VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));

                // Parse id
                Json::Value folderId = _filesInfo[index].get("id", 0);
                rfd.dwId.id = folderId.asInt();
                rfd.dwId.category = RecycleCat;
                if (rfd.dwId.id == 0) return FALSE;

                // Parse Name
                Json::Value folderName = _filesInfo[index].get("name", "");
                std::string test = folderName.asString();
                if (test.empty()) return FALSE;
                wcscpy_s(rfd.cFileName, lengthof(rfd.cFileName), (const wchar_t *)CA2W(test.c_str()));

                // Parse FileCurSize
                Json::Value fileSize = _filesInfo[index].get("size", 0);
                rfd.nFileSizeLow = fileSize.asInt();

                rfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

                SYSTEMTIME stime; GetSystemTime(&stime);
                Json::Value modifyTime = _filesInfo[index].get("modifyTime", "0000-00-00T00:00:00.000");
                if (!modifyTime.empty()){
                    Utility::ParseTime((const wchar_t *)CA2WEX<>(modifyTime.asString().c_str(), CP_UTF8), &stime);
                }
                FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
                rfd.ftLastAccessTime = rfd.ftLastWriteTime = rfd.ftCreationTime = ftime;

                GetSystemTime(&stime);
                modifyTime = _filesInfo[index].get("createTime", "0000-00-00T00:00:00.000");
                if (!modifyTime.empty()){
                    Utility::ParseTime((const wchar_t *)CA2WEX<>(modifyTime.asString().c_str(), CP_UTF8), &stime);
                }
                SystemTimeToFileTime(&stime, &ftime);
                rfd.ftCreationTime = ftime;

                std::string creatorName = _filesInfo[index].get("creatorName", "").asString();
                wcscpy_s(rfd.creatorName, lengthof(rfd.creatorName), (const wchar_t *)CA2WEX<>(creatorName.c_str(), CP_UTF8));

                std::string lastVerNumStr = _filesInfo[index].get("lastVerNumStr", "").asString();
                wcscpy_s(rfd.versionStr, lengthof(rfd.versionStr), (const wchar_t *)CA2WEX<>(lastVerNumStr.c_str(), CP_UTF8));

                children.push_back(rfd);
            }
        }
        // Parse _settings.
        {
            Json::Value JsonTotalCount = root.get("totalCount", 0);
            int itemCount = 0;
            if (!JsonTotalCount.empty()){
                itemCount = JsonTotalCount.asInt();
            }
            if (itemCount == 0)
                return FALSE;

            *PageCount = ( itemCount % PageSize ) ? (itemCount/PageSize + 1): (itemCount/PageSize);
        }
    }
    return TRUE;
}

BOOL HttpImpl::GetPagedSearchResults(TAR_ARCHIVE * pArchive, const std::wstring & query, std::list<VFS_FIND_DATA> & children, int PageSize, int PageNo, int* PageCount)
{
    for (int i = 0; i < PageSize; i ++){
        VFS_FIND_DATA rfd; memset(&rfd, 0, sizeof(rfd));
        rfd.dwId.category = VdriveCat;
        rfd.dwId.id = 0;
        swprintf_s(rfd.cFileName, lengthof(rfd.cFileName), _T("%s-%04d-%04d")
            , query.c_str(), PageNo, i);
        rfd.dwFileAttributes = (i % 2) ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_DIRECTORY;
        children.push_back(rfd);
    }
    *PageCount = 3;
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
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
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
    std::string strNewName = Utility::UrlEncode((const char *)CW2AEX<>(newName, CP_UTF8));
    wsprintf(url
        , isFolder 
        ?  _T("%s/EDoc2WebApi/api/Doc/FolderRead/ChangeFolderName?token=%s&folderId=%d&newName=%s")
        :  _T("%s/EDoc2WebApi/api/Doc/FileRead/ChanageFileName?token=%s&fileId=%d&newFileName=%s&newRemark=NAN")
        , pArchive->context->service, pArchive->context->AccessToken, itemId.id, (const wchar_t *)CA2W(strNewName.c_str()));

    // {"$id":"1","FolderId":1,"AreaId":1,"ParentFolderId":1,"FolderCode":"[PublicRoot]","FolderSortOrder":0,"FolderName":"PublicRoot","FolderPath":"1","FolderNamePath":"PublicRoot","FolderSize":0,"FolderMaxFolderSize":0,"FolderAlertSize":0,"FolderMaxFileSize":0,"FolderForbiddenFileExtensions":null,"FolderChildFoldersCount":0,"FolderChildFilesCount":0,"SecurityLevelId":0,"FolderState":0,"FolderLockCount":0,"FolderCreateTime":"2009-05-20T20:45:39.937","FolderCreateOperator":0,"FolderModifyTime":"2009-05-20T20:45:39.937","FolderModifyOperator":0,"FolderArchiveTime":"0001-01-01T00:00:00","FolderArchiveOperator":0,"FolderCurVerId":0,"FolderNewestVerId":0,"FolderType":1,"FolderGuid":"97af3663-8793-45f5-a2c5-4f5ad537353f","FolderOwnerId":0,"IsDeleted":false,"FolderDeleteTime":"0001-01-01T00:00:00","FolderDeleteOperator":0,"FolderIsCascadeDelete":false,"RelativePath":"PublicRoot","IsArcFolder":false,"HasBoundStorageArea":false,"Children":null}
    std::wstring response;
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
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
    if (!Utility::HttpRequest(url, response, pArchive->context->HttpTimeoutMs))
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
BOOL HttpImpl::UploadFile(TAR_ARCHIVE * pArchive, const RemoteId & parentId, const wchar_t * tempFile, const wchar_t * faceName)
{
    // Prepare upload id, used for query progress.
    wchar_t uploadidstring [100] = _T("");
    _stprintf_s(uploadidstring, lengthof(uploadidstring), _T("EDoc2_SWFUpload_%0u_0"), (int)time(NULL));

    // Prepare url.
    wchar_t url [MaxUrlLength] = _T("");
    _stprintf_s(url, lengthof(url)
        , _T("%s/edoc2v4/Document/File_Upload.aspx?tkn=%s&upload=%s")
        , pArchive->context->service
        , pArchive->context->AccessToken
        , uploadidstring);

    std::stringstream response;
    if (!Utility::HttpPostFile(url, parentId.id, tempFile, faceName, response, pArchive->context->HttpTimeoutMs))
        return FALSE;

    return TRUE;

    std::wstring uploadid = (const wchar_t *)CA2WEX<>(response.str().c_str(), CP_UTF8);
    _stprintf_s(url, lengthof(url)
        , _T("%s/edoc2v4/UploadProgress.ashx?upload=%s")
        , pArchive->context->service
        , uploadidstring);

    std::wstring strresponse;
    if (!Utility::HttpRequest(url, strresponse, pArchive->context->HttpTimeoutMs))
        return FALSE;

    // Sample Response string.
    //{"uploadId":"EDoc2_SWFUpload_1395551254_0","filename":"webapixxx.doc","percent":100,"speed":0,"remaining":-1,"status":"End","message":null,"stack":null,"tag":"103|50|webapixxxx.doc|2601"}
    Json::Value root;
    Json::Reader reader; 
    if (reader.parse((const char *)CW2A(strresponse.c_str()), root, false)){
        Json::Value tag = root.get("tag", "");
        if (tag.empty()) return FALSE;
        std::string tagstr = tag.asString();
        int fileId = atoi(strchr(tagstr.c_str(), '|') + 1);
    }

    return TRUE;
}

/**
* Integrated method into shell to Download file.
* NOTE: reserved for harry's prototype test.
*       you must not use this method.
*/
BOOL HttpImpl::DownloadFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * tempFile)
{
    // Prepare url.
    wchar_t url [MaxUrlLength] = _T("");
    _stprintf_s(url, lengthof(url)
        , _T("%s/edoc2v4/Document/File_Download.aspx?file_id=%d&checkout=false&regionID=1")
        , pArchive->context->service
        , itemId.id);

    // Prepare cookie.
    std::wstring cookie = _T("");
    cookie += _T("account=");  cookie += pArchive->context->username;    cookie += _T(";");
    cookie += _T("tkn=");      cookie += pArchive->context->AccessToken; cookie += _T(";");
    cookie += _T("token=");    cookie += pArchive->context->AccessToken; cookie += _T(";");
    
    std::stringstream response;
    if (!Utility::HttpRequestWithCookie(url, cookie, response, pArchive->context->HttpTimeoutMs)){
        return FALSE;
    }

    std::ofstream ofs; ofs.open((const char *)CW2A(tempFile), std::ofstream::binary | std::ofstream::trunc);
    if (ofs.is_open())
    {
        ofs<<response.rdbuf();
        ofs.close();
    }

    return TRUE;
}

BOOL HttpImpl::UploadFolder(TAR_ARCHIVE * pArchive, const RemoteId & parentFolderId, const wchar_t * localPath)
{
    return S_FALSE;
}

BOOL HttpImpl::DownloadFolder(TAR_ARCHIVE * pArchive, const RemoteId & itemId, const wchar_t * localPath)
{
    return S_FALSE;
}

BOOL HttpImpl::Select(TAR_ARCHIVE * pArchive, const RemoteId & itemId, BOOL selected, BOOL isFolder)
{
    return TRUE;
}

BOOL HttpImpl::GetColumnInfo(TAR_ARCHIVE * pArchive, const RemoteId & viewId, wchar_t * pColumnInfo, int maxcch)
{
    wcscpy_s(pColumnInfo, maxcch, _T("Attr1;Attr2;Attr3;Attr4"));
    return TRUE;
}

BOOL HttpImpl::PreviewFile(TAR_ARCHIVE * pArchive, const RemoteId & itemId)
{
    // Dummy Implementation
    char szMsg [100]; sprintf_s(szMsg, lengthof(szMsg), "%s(ID=[%d:%d])", __FUNCTION__, itemId.category, itemId.id); MessageBoxA(GetActiveWindow(), szMsg, __FUNCTION__, MB_OKCANCEL | MB_ICONINFORMATION);
    return TRUE;
}

BOOL HttpImpl::OnShellViewCreated(TAR_ARCHIVE * pArchive, HWND shellViewWnd)
{
    return FALSE;
}

BOOL HttpImpl::OnShellViewSized(TAR_ARCHIVE * pArchive, HWND shellViewWnd)
{
    return FALSE;
}

BOOL HttpImpl::OnShellViewRefreshed(TAR_ARCHIVE  * pArchive, HWND shellViewWnd)
{
    return FALSE;
}

BOOL HttpImpl::OnShellViewClosing(TAR_ARCHIVE  * pArchive, HWND shellViewWnd)
{
    return FALSE;
}

BOOL HttpImpl::FileExists(TAR_ARCHIVE * pArchive, const RemoteId & parentId, wchar_t * childName, VFS_FIND_DATA & childInfo)
{
    return FALSE;
}

BOOL HttpImpl::FolderExists(TAR_ARCHIVE * pArchive, const RemoteId & parentId, wchar_t * childName, VFS_FIND_DATA & childInfo)
{
    return FALSE;
}

BOOL HttpImpl::CheckMenu(TAR_ARCHIVE * pArchive, std::wstring & idlist, MenuType * selectedMenuItems)
{
    *selectedMenuItems=MenuDef_AllSelected;
    return TRUE;
}