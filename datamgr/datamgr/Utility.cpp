#pragma warning(disable:4996)
#include <Windows.h>
#include <ShlObj.h>
#include <atlbase.h>
#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <tinystr.h>
#include <tinyxml.h>
#include <json/json.h>
#include <curl/curl.h>
#include "Edoc2Context.h"
#include "datamgr.h"
#include "Utility.h"
#if defined(_MSC_VER)
#pragma comment(lib, "ws2_32.lib")
#endif

///////////////////////////////////////////////////////////////////////////////
// Debug helper functions
std::string WideStringToAnsi(const wchar_t * wstr){
	if (!wstr) return "";
	char szAnsi [MAX_PATH] = "";
	WideCharToMultiByte(CP_ACP, 0, wstr, wcslen(wstr), szAnsi, lengthof(szAnsi), NULL, NULL);
	return szAnsi;
}
void OutputLog(const char * format, ...){
	va_list va;
	va_start(va, format);
	char * szMsg = (char *) malloc(0x100000);
	vsnprintf_s(szMsg, 0x100000, 0xffffe, format, va);
	OutputDebugStringA(szMsg); OutputDebugStringA("\n");
	free(szMsg);
}

//////////////////////////////////////////////////////////////////////////
// Helpers
BOOL Utility::ConstructRecycleFolder(TAR_ARCHIVE * pArchive, VFS_FIND_DATA & recycleFolder)
{
    memset(&recycleFolder, 0, sizeof(recycleFolder));
    recycleFolder.dwId.category = RecycleCat;
    recycleFolder.dwId.id = RecycleId;
    LoadLocalizedName(pArchive->context->configfile, pArchive->context->localeName, _T("RecycleBin"), recycleFolder.cFileName, lengthof(recycleFolder.cFileName));
    recycleFolder.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    SYSTEMTIME stime; GetSystemTime(&stime);
    FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
    recycleFolder.ftLastAccessTime = recycleFolder.ftLastWriteTime = recycleFolder.ftCreationTime = ftime;
    return TRUE;
}

BOOL Utility::ConstructSearchFolder(TAR_ARCHIVE * pArchive, VFS_FIND_DATA & searchFolder)
{
    memset(&searchFolder, 0, sizeof(searchFolder));
    searchFolder.dwId.category = SearchCat;
    searchFolder.dwId.id = SearchId;
    LoadLocalizedName(pArchive->context->configfile, pArchive->context->localeName, _T("SearchBin"), searchFolder.cFileName, lengthof(searchFolder.cFileName));
    searchFolder.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    SYSTEMTIME stime; GetSystemTime(&stime);
    FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
    searchFolder.ftLastAccessTime = searchFolder.ftLastWriteTime = searchFolder.ftCreationTime = ftime;
    return TRUE;
}

bool Utility::RfsComparation(const VFS_FIND_DATA & left, const VFS_FIND_DATA & right)
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

BOOL Utility::HttpRequest(const wchar_t * requestUrl, std::wstring & response, unsigned int timeoutMs)
{
    std::stringstream bytestream;
    if (!HttpRequestWithCookie(requestUrl, _T(""), bytestream, timeoutMs))
        return FALSE;
    response = (const wchar_t *)CA2WEX<>(bytestream.str().c_str(), CP_UTF8);
    return TRUE;
}

BOOL Utility::HttpRequestWithCookie(const wchar_t * requestUrl, const std::wstring & cookie, std::stringstream & response, unsigned int timeoutMs)
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
		curl_easy_setopt(curl, CURLOPT_URL, (char *)CW2AEX<128>(requestUrl, CP_ACP));

#ifdef UTILITY_SKIP_PEER_VERIFICATION
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

#ifdef UTILITY_SKIP_HOSTNAME_VERIFICATION
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
		// curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeoutMs);


		// HarryWu, 2014.2.20
		// Progress support.
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, XferProgress);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, 0L);

        // Setup cookie, if present
        if (!cookie.empty()){
            curl_easy_setopt(curl, CURLOPT_COOKIE, (const char *)CW2AEX<>(cookie.c_str(), CP_UTF8));
        }

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
            if (chunk.size) response.write(chunk.memory, chunk.size);
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

BOOL Utility::HttpPostFile(const wchar_t * url, int parentId, const wchar_t * tempFile, const wchar_t * faceName, std::stringstream & response, int timeoutMs)
{
    /*
    const wchar_t * formxml = 
        _T("<form>")
            _T("<input type='file' name='Filedata'  value='' filename=''></input>")
            _T("<input type='text' name='FILE_INFO' value=''></input>")
            _T("<input type='text' name='FILE_MODE' value=''></input>")
        _T("</form>");
    HttpPostForm(url, formxml, _T(""), response, timeoutMs);
    */

    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;    /* no data at this point */ 

    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    struct curl_slist *headerlist=NULL;
    static const char buf[] = "Expect:";
    char szTemp [100] = ""; 

    curl_global_init(CURL_GLOBAL_ALL);

    std::string fileName = "";

    if (faceName == NULL)
        fileName = (const char *)CW2AEX<>(wcsrchr(tempFile, _T('\\')) + 1, CP_UTF8);
    else
        fileName = (const char *)CW2AEX<>(faceName, CP_UTF8);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "Filename",
        CURLFORM_COPYCONTENTS,fileName.c_str() ,
        CURLFORM_END);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "FILE_MODE",
        CURLFORM_COPYCONTENTS, "UPLOAD",
        CURLFORM_END);

    char szFileInfo [MAX_PATH] = "";
    sprintf_s(szFileInfo, lengthof(szFileInfo)
        , "%d\01%s\01\01%s\01\01\01"
        , parentId
        , fileName.c_str()
        , fileName.c_str());

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "FILE_INFO",
        CURLFORM_COPYCONTENTS, szFileInfo,
        CURLFORM_END);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "Filedata",
        CURLFORM_FILE, (const char *)CW2AEX<>(tempFile, CP_ACP),
        CURLFORM_FILENAME, fileName.c_str(), 
        CURLFORM_END);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "upload",
        CURLFORM_COPYCONTENTS, "Submit Query",
        CURLFORM_END);

    curl = curl_easy_init();
    /* initalize custom header list (stating that Expect: 100-continue is not wanted */
    headerlist = curl_slist_append(headerlist, buf);
    if(curl) {
        /* what URL that receives this POST */
        curl_easy_setopt(curl, CURLOPT_URL, (const char *)CW2AEX<>(url, CP_ACP));

        /* only disable 100-continue header if explicitly requested */
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        /* send all data to this function  */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */ 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeoutMs);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

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

            // HarryWu, 2014.2.21
            // I guess that, you are using utf-8.
            response.write(chunk.memory, chunk.size);

            OUTPUTLOG("url=[%s] [%lu] bytes retrieved\n", (const char *)CW2A(url), (long)chunk.size);
        }
        /* always cleanup */
        curl_easy_cleanup(curl);

        /* then cleanup the formpost chain */
        curl_formfree(formpost);

        /* free slist */
        curl_slist_free_all (headerlist);
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
	return FALSE;
}

BOOL Utility::LoadLocalizedName(const wchar_t * xmlconfigfile, const wchar_t * localeName, const wchar_t * key, wchar_t * retVaule, int cchMax)
{
	if (key && retVaule) wcscpy_s(retVaule, cchMax, key);
	TiXmlDocument * xdoc = new TiXmlDocument();
	if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("Locale");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild((const char *)CW2AEX<>(key, CP_UTF8));
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * value = target->FirstChild((const char *)CW2AEX<>(localeName, CP_UTF8));
    if (!value){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    wcscpy_s(retVaule, cchMax, (const wchar_t *)CA2WEX<>(value->ToElement()->GetText(), CP_UTF8));
    
    delete xdoc; xdoc = NULL;
	return TRUE;
}

BOOL Utility::CheckHttpEnable(const wchar_t * xmlconfigfile)
{
    TiXmlDocument * xdoc = new TiXmlDocument();
    if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("Net");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild("HttpEnable");
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    BOOL enablehttp = !strnicmp(target->ToElement()->GetText(), "Yes", 3);

    delete xdoc; xdoc = NULL;

    return enablehttp;
}

DWORD Utility::GetHttpTimeoutMs(const wchar_t * xmlconfigfile)
{
    TiXmlDocument * xdoc = new TiXmlDocument();
    if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("Net");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild("HttpConnectioTimeoutMilliSeconds");
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    DWORD httpTimeoutMs = 2000;
    if (target->ToElement() && target->ToElement()->GetText())
        httpTimeoutMs = atoi(target->ToElement()->GetText());

    delete xdoc; xdoc = NULL;

    return httpTimeoutMs;
}

BOOL Utility::GetServiceBase(const wchar_t* xmlconfigfile, wchar_t * pszSvcBase, int maxcch)
{
    TiXmlDocument * xdoc = new TiXmlDocument();
    if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("Net");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild("Service");
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    if (target->ToElement() && target->ToElement()->GetText())
       wcscpy_s(pszSvcBase, maxcch, (const wchar_t *)CA2WEX<>(target->ToElement()->GetText(), CP_UTF8));

    delete xdoc; xdoc = NULL;

    return TRUE;
}

BOOL Utility::GetServiceUser(const wchar_t* xmlconfigfile, wchar_t * pszSvcUser, int maxcch)
{
    TiXmlDocument * xdoc = new TiXmlDocument();
    if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("Net");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild("User");
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    if (target->ToElement() && target->ToElement()->GetText())
        wcscpy_s(pszSvcUser, maxcch, (const wchar_t *)CA2WEX<>(target->ToElement()->GetText(), CP_UTF8));

    delete xdoc; xdoc = NULL;

    return TRUE;
}

BOOL Utility::GetServicePass(const wchar_t* xmlconfigfile, wchar_t * pszSvcPass, int maxcch)
{
    TiXmlDocument * xdoc = new TiXmlDocument();
    if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("Net");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild("Pass");
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    if (target->ToElement() && target->ToElement()->GetText())
        wcscpy_s(pszSvcPass, maxcch, (const wchar_t *)CA2WEX<>(target->ToElement()->GetText(), CP_UTF8));

    delete xdoc; xdoc = NULL;

    return TRUE;
}

DWORD Utility::GetViewPort(const wchar_t * xmlconfigfile)
{
    TiXmlDocument * xdoc = new TiXmlDocument();
    if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("Net");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild("ViewPort");
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    DWORD viewPort = 60688;
    if (target->ToElement() && target->ToElement()->GetText())
        viewPort = atoi(target->ToElement()->GetText());

    delete xdoc; xdoc = NULL;

    return viewPort;
}

DWORD Utility::GetJsonPort(const wchar_t * xmlconfigfile)
{
    TiXmlDocument * xdoc = new TiXmlDocument();
    if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("Net");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild("ViewPort");
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    DWORD jsonPort = 60684;
    if (target->ToElement() && target->ToElement()->GetText())
        jsonPort = atoi(target->ToElement()->GetText());

    delete xdoc; xdoc = NULL;

    return jsonPort;
}

unsigned char Utility::ToHex(unsigned char x) 
{ 
	return  x > 9 ? x + 55 : x + 48; 
}

unsigned char Utility::FromHex(unsigned char x) 
{ 
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string Utility::UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)str[i]) || 
			(str[i] == '-') ||
			(str[i] == '_') || 
			(str[i] == '.') || 
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] % 16);
		}
	}
	return strTemp;
}

std::string Utility::UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (str[i] == '+') strTemp += ' ';
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high*16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

BOOL Utility::GenerateTempFilePath(wchar_t * pwszTempFilePath, int cchMax, const wchar_t * templatename)
{
    GetTempPath(cchMax, pwszTempFilePath);

    srand(GetTickCount());
    wchar_t szTempFileName [MAX_PATH] = _T("");
    _stprintf_s(szTempFileName, lengthof(szTempFileName)
        , _T("vdrivecache-[%u.%u.%d].%s")
        , (unsigned int)time(NULL), GetTickCount(), rand(), templatename);

    wcscat_s(pwszTempFilePath, cchMax, szTempFileName);
    return TRUE;
}

BOOL Utility::HttpPostForm(const wchar_t * url, const wchar_t * httpform, const std::wstring & cookie, std::stringstream & response, int timeoutMs)
{
    bool useMulitPartPost = false;
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;    /* no data at this point */ 

    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    struct curl_slist *headerlist=NULL;
    static const char buf[] = "Expect:";
    
    curl_global_init(CURL_GLOBAL_ALL);

    //TODO: parse http form, and setup parameters for http post.
    std::string formdataurlencoded = "";
    {
        TiXmlDocument * xdoc = new TiXmlDocument();
        xdoc->Parse((const char *)CW2AEX<>(httpform, CP_UTF8));

        TiXmlElement * xroot = xdoc->RootElement();

        for (TiXmlElement * child = xroot->FirstChildElement("input"); child; child = child->NextSiblingElement("input"))
        {
            const char * typestr = child->Attribute("type");
            if (typestr == NULL) continue ;

            const char *namestr = NULL; const char * valuestr = NULL;
            if (!stricmp(typestr, "file")){
                namestr  = child->Attribute("name");
                valuestr = child->Attribute("value");
            }

            if (!stricmp(typestr, "text") || !stricmp(typestr, "hidden") || !stricmp(typestr, "button")){
                namestr  = child->Attribute("name");             
                valuestr = child->Attribute("value");

                if (useMulitPartPost){
                    curl_formadd(&formpost,
                        &lastptr,
                        CURLFORM_COPYNAME, namestr,
                        CURLFORM_COPYCONTENTS, valuestr,
                        CURLFORM_END);
                }else{
                    formdataurlencoded += namestr; formdataurlencoded += "="; formdataurlencoded += valuestr; formdataurlencoded += "&"; 
                }
            }

            if (namestr && valuestr) OUTPUTLOG("KEY[`%s\']->Value[`%s\']", namestr, valuestr);
        }

        delete xdoc; xdoc = NULL;
        //return FALSE;
    }  
    
    curl = curl_easy_init();
    /* initalize custom header list (stating that Expect: 100-continue is not wanted */
    headerlist = curl_slist_append(headerlist, buf);
    if(curl) {
        /* what URL that receives this POST */
        curl_easy_setopt(curl, CURLOPT_URL, (const char *)CW2AEX<>(url, CP_ACP));

        /* only disable 100-continue header if explicitly requested */
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, formdataurlencoded.c_str());

        /* send all data to this function  */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */ 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeoutMs);

        // Setup cookie, if present
        if (!cookie.empty()){
            curl_easy_setopt(curl, CURLOPT_COOKIE, (const char *)CW2AEX<>(cookie.c_str(), CP_UTF8));
        }

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

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

            // HarryWu, 2014.2.21
            // I guess that, you are using utf-8.
            response.write(chunk.memory, chunk.size);

            OUTPUTLOG("url=[%s] [%lu] bytes retrieved\n", (const char *)CW2A(url), (long)chunk.size);
        }
        /* always cleanup */
        curl_easy_cleanup(curl);

        /* then cleanup the formpost chain */
        curl_formfree(formpost);

        /* free slist */
        curl_slist_free_all (headerlist);
    }

    if(chunk.memory){
        free(chunk.memory);
    }

    curl_global_cleanup();

    return TRUE;
}

BOOL Utility::GetShellViewPageSize(const wchar_t* xmlconfigfile, DWORD * pageSize)
{
    TiXmlDocument * xdoc = new TiXmlDocument();
    if (!xdoc) return FALSE;

    if (!xdoc->LoadFile((const char *)CW2A(xmlconfigfile))){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlElement * xroot = xdoc->RootElement();
    if (!xroot){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * local = xroot->FirstChild("View");
    if (!local) {
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    TiXmlNode * target= local->FirstChild("PageSize");
    if (!target){
        delete xdoc; xdoc = NULL;
        return FALSE;
    }

    if (target->ToElement() && target->ToElement()->GetText())
        *pageSize = atoi(target->ToElement()->GetText());

    delete xdoc; xdoc = NULL;

    return TRUE;
}

BOOL Utility::ParseTime(const std::wstring & timestr, SYSTEMTIME * retTime2)
{
    // "2014-03-26T16:15:38.223"
    SYSTEMTIME sTime, *retTime ; retTime = &sTime;
    _stscanf_s(timestr.c_str(), _T("%04hd-%02hd-%02hdT%02hd:%02hd:%02hd.%03hd")
        , &retTime->wYear
        , &retTime->wMonth
        , &retTime->wDay
        , &retTime->wHour
        , &retTime->wMinute
        , &retTime->wSecond
        , &retTime->wMilliseconds);
    TIME_ZONE_INFORMATION tz; GetTimeZoneInformation(&tz);
    SystemTimeToTzSpecificLocalTime(&tz, retTime, retTime2);
    return TRUE;
}

BOOL Utility::ParseVersion(const std::wstring & verString, DWORD * dwVersion)
{
    return TRUE;
}

BOOL Utility::SocketRequest(const wchar_t * ipv4, unsigned short port, const wchar_t * _wreq)
{
    // HarryWu, 2014.5.30
    // WSAStartup() should be called already.

    sockaddr_in saddr;
    saddr.sin_addr.s_addr = inet_addr((const char *)CW2AEX<>(ipv4, CP_ACP));
    saddr.sin_port = htons(port);
    saddr.sin_family = AF_INET;

    OUTPUTLOG("try to connect [%s:%hu]", (const char *)CW2AEX<>(ipv4, CP_ACP), port);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return FALSE;

    unsigned long ul = 1;
    if (ioctlsocket(sock, FIONBIO, (unsigned long*)&ul))
    {
        closesocket(sock); sock = INVALID_SOCKET;
        return FALSE;
    }

    if (connect(sock, (sockaddr *)&saddr, sizeof(saddr))) {
        //closesocket(sock);
        //return FALSE;
    }

    fd_set r; FD_ZERO(&r); FD_SET(sock, &r);
    struct timeval timeout = {0, 50000};
    if (select(0, 0, &r, 0, &timeout) <= 0){
        closesocket(sock); sock = NULL;
        OUTPUTLOG("Connect timeout [%d] ms", timeout.tv_usec/1000);
        return FALSE;
    }

    unsigned long ul1= 0 ;
    if (ioctlsocket(sock, FIONBIO, (unsigned long*)&ul1))
    {
        closesocket(sock); sock = INVALID_SOCKET;
        return FALSE;
    }

    std::string req = (const char *)CW2AEX<>(_wreq, CP_ACP);
    OUTPUTLOG("try to send reqeust: [%s]", req.c_str());
    int reqlen = req.length();
    if (reqlen != send(sock, req.c_str(), reqlen, 0))
    {
        closesocket(sock); sock = INVALID_SOCKET;
        return FALSE;
    }

    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    OUTPUTLOG("successfully sent request.");
    return TRUE;
}