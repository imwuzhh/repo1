#pragma once

class Utility
{
public:
	Utility(void);
	virtual ~Utility(void);
	BOOL ConstructTopFolders(std::list<RFS_FIND_DATA> & topFolderList);
	BOOL Login(const wchar_t * user, const wchar_t * pass, wchar_t * accessToken);
};
