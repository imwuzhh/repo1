#pragma warning(disable:4996)
#include <Windows.h>
#include <ShlObj.h>
#include <atlbase.h>
#include <string>
#include <list>
#include <iostream>
#include <sys/stat.h>

#include "datamgr.h"
#include "Utility.h"

Utility::Utility(void)
{
}

Utility::~Utility(void)
{
}

BOOL Utility::ConstructTopFolders(std::list<RFS_FIND_DATA> & topFolderList)
{

	return TRUE;
}

BOOL Utility::Login(const wchar_t * user, const wchar_t * pass, wchar_t * accessToken)
{
	// "http://kb.edoc2.com/EDoc2WebApi/api/Org/UserRead/UserLogin?userLoginName=admin&password=edoc2"
	return TRUE;
}