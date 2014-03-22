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
BOOL Utility::ConstructRecycleFolder(TAR_ARCHIVE * pArchive, RFS_FIND_DATA & recycleFolder)
{
    memset(&recycleFolder, 0, sizeof(recycleFolder));
    recycleFolder.dwId.category = RecycleCat;
    recycleFolder.dwId.id = 0;
    LoadLocalizedName(pArchive->context->configfile, pArchive->context->localeName, _T("RecycleBin"), recycleFolder.cFileName, lengthof(recycleFolder.cFileName));
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
    LoadLocalizedName(pArchive->context->configfile, pArchive->context->localeName, _T("SearchBin"), searchFolder.cFileName, lengthof(searchFolder.cFileName));
    searchFolder.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    SYSTEMTIME stime; GetSystemTime(&stime);
    FILETIME   ftime; SystemTimeToFileTime(&stime, &ftime);
    searchFolder.ftLastAccessTime = searchFolder.ftLastWriteTime = searchFolder.ftCreationTime = ftime;
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

BOOL Utility::HttpRequest(const wchar_t * requestUrl, std::wstring & response, unsigned int timeoutMs)
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeoutMs);


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

BOOL Utility::HttpPost(const wchar_t * accessToken, int parentId, const wchar_t * tempFile, std::wstring & response)
{
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

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "StartUploadFile",
        CURLFORM_COPYCONTENTS, (const char *)CW2A(accessToken),
        CURLFORM_END);

    sprintf_s(szTemp, "%X%x", rand(), rand());
    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "UploadId",
        CURLFORM_COPYCONTENTS, szTemp,
        CURLFORM_END);

    sprintf_s(szTemp, lengthof(szTemp), "%d", parentId);
    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "ParentId",
        CURLFORM_COPYCONTENTS, szTemp,
        CURLFORM_END);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "Name",
        CURLFORM_COPYCONTENTS, (const char *)CW2A(wcsrchr(tempFile, _T('\\')) + 1),
        CURLFORM_END);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "Remark",
        CURLFORM_COPYCONTENTS, "",
        CURLFORM_END);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "FileCode",
        CURLFORM_COPYCONTENTS, "0",
        CURLFORM_END);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "DefaultSecurityLevel",
        CURLFORM_COPYCONTENTS, "0",
        CURLFORM_END);

    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "FileSize",
        CURLFORM_COPYCONTENTS, "0",
        CURLFORM_END);


    curl = curl_easy_init();
    /* initalize custom header list (stating that Expect: 100-continue is not wanted */
    headerlist = curl_slist_append(headerlist, buf);
    if(curl) {
        /* what URL that receives this POST */
        curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.253.242/mobile/api/Transfer/UploadFile");

        /* only disable 100-continue header if explicitly requested */
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        /* send all data to this function  */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */ 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

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
            OUTPUTLOG("%lu bytes retrieved\n", (long)chunk.size);
            // HarryWu, 2014.2.21
            // I guess that, you are using utf-8.
            response = (wchar_t *)CA2WEX<128>(chunk.memory, CP_UTF8);
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