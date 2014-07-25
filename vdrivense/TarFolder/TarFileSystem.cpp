
#include "stdafx.h"

#include "TarFileSystem.h"


///////////////////////////////////////////////////////////////////////////////
// CTarShellModule

/**
 * Return capabilities of this Shell Extension.
 * The GetConfigXXX methods are called by the NSE to query what functionality our
 * file-system supports.
 */
BOOL CTarShellModule::GetConfigBool(VFS_CONFIG Item)
{
   switch( Item ) {
   case VFS_CAN_ROOT_URLPROTOCOL:
	   return TRUE;

   case VFS_CAN_SLOW_COPY:
	   return TRUE;

   case VFS_CAN_PROGRESSUI:
   case VFS_CAN_ROOT_PREVIEW:
   case VFS_CAN_ROOT_SHELLNEW:
   case VFS_CAN_ROOT_DROPTARGET:
   case VFS_CAN_ROOT_CONTEXTMENU:
      return TRUE;

   case VFS_HAVE_ICONOVERLAYS:
   case VFS_HAVE_SYSICONS:
   case VFS_HAVE_UNIQUE_NAMES:
   case VFS_HAVE_VIRTUAL_FILES:
      return TRUE;

   case VFS_HIDE_PREVIEW_PANEL:
      return TRUE;

   }
   return FALSE;
}

/**
 * Return capabilities of this Shell Extension.
 * The GetConfigXXX methods are called by the NSE to query what functionality our
 * file-system supports.
 */
LONG CTarShellModule::GetConfigInt(VFS_CONFIG Item)
{
   switch( Item ) {

   case VFS_INT_LOCATION:
      return VFS_LOCATION_MYCOMPUTER;

   case VFS_INT_MAX_FILENAME_LENGTH:
      return MAX_PATH;

   case VFS_INT_MAX_PATHNAME_LENGTH:
      return MAX_PATH;

   case VFS_INT_SHELLROOT_SFGAO:
       return   SFGAO_FOLDER 
			  | SFGAO_CANCOPY 
              | SFGAO_CANMOVE 
              | SFGAO_CANRENAME 
              | SFGAO_DROPTARGET
              //| SFGAO_STREAM
              | SFGAO_BROWSABLE 
              | SFGAO_HASSUBFOLDER 
              | SFGAO_FILESYSANCESTOR
              | SFGAO_STORAGEANCESTOR;

   }
   return 0;
}

/**
 * Return capabilities of this Shell Extension.
 * The GetConfigXXX methods are called by the NSE to query what functionality our
 * file-system supports.
 */
LPCWSTR CTarShellModule::GetConfigStr(VFS_CONFIG Item)
{
   switch( Item ) {

   case VFS_STR_FILENAME_CHARS_NOTALLOWED:
      return L":<>\\/|\"'*?[]";

   }
   return NULL;
}

/**
 * Called during installation (dll registration)
 * Use this to do extra work during the module registration. Remember that this
 * method is run as Admin and only called during installation.
 */
HRESULT CTarShellModule::DllInstall()
{
   return S_OK;
}

/**
 * Called during uninstall (dll de-registration)
 */
HRESULT CTarShellModule::DllUninstall()
{
   return S_OK;
}


/**
 * Called on ShellNew integration.
 * Invoked by user through Desktop ContextMenu -> New -> Tar Folder.
 */
HRESULT CTarShellModule::ShellAction(LPCWSTR pstrType, LPCWSTR pstrCmdLine)
{
   if( wcscmp(pstrType, L"ShellNew") == 0 ) E_FAIL;
   return S_OK;
}

/**
 * Called at process/thread startup/shutdown.
 */
BOOL CTarShellModule::DllMain(DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH){
        if (FAILED(CheckCallingProcess())) return FALSE;
		LoadLangResource();
	}
    return TRUE;
}

/**
 * Create a FileSystem instance from a IDLIST (Windows file).
 */
HRESULT CTarShellModule::CreateFileSystem(PCIDLIST_ABSOLUTE pidlRoot, CNseFileSystem** ppFS)
{
   // Create a new file-system instance with the source .tar filename.
   CTarFileSystem* pFS = new CTarFileSystem();
   HRESULT Hr = pFS->Init(pidlRoot);
   if( SUCCEEDED(Hr) ) *ppFS = pFS;
   else delete pFS; 
   return Hr;
}

HRESULT CTarShellModule::LoadLangResource()
{
	wchar_t szLangId [MAX_PATH] = _T("");
	GetUserDefaultLocaleName(szLangId, lengthof(szLangId));
	if (!wcschr(szLangId, _T('-'))) return S_OK;

	wcschr(szLangId, _T('-'))[0] = _T('_');
	OutputDebugString(szLangId);

	wchar_t szFullResPath [MAX_PATH] = _T("");
	GetModuleFileName(_pModule->get_m_hInst(), szFullResPath, lengthof(szFullResPath));
	wchar_t * x64 = wcsstr(szFullResPath, _T("64.dll"));
	wchar_t * x86 = wcsstr(szFullResPath, _T("32.dll"));
	if (!x86 && !x64) return S_OK;

	if (x64) *x64 = 0; if (x86) *x86 = 0;
	wcscat_s(szFullResPath, lengthof(szFullResPath), _T("."));
	wcscat_s(szFullResPath, lengthof(szFullResPath), szLangId);
	wcscat_s(szFullResPath, lengthof(szFullResPath), x64 ? _T("64.dll") : _T("32.dll"));

	HINSTANCE hResInst = LoadLibrary(szFullResPath);
	if (hResInst)
	{
		_pModule->SetResourceInstance(hResInst);
		OutputDebugString(szFullResPath);
	}
	return S_OK;
}

HRESULT CTarShellModule::CheckCallingProcess()
{
    char szExePath [MAX_PATH] = "";
    char szExeName [MAX_PATH] = "";
    GetModuleFileNameA(NULL, szExePath, MAX_PATH);
    ::_splitpath(szExePath, NULL, NULL, szExeName, NULL);
    if (!stricmp(szExeName, "Explorer") || !stricmp(szExeName, "Regsvr32"))
        return S_OK;
    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
// CTarFileSystem

CTarFileSystem::CTarFileSystem() : m_cRef(1), m_pArchive(NULL)
{
}

CTarFileSystem::~CTarFileSystem()
{
   DMClose(m_pArchive);
}

HRESULT CTarFileSystem::Init(PCIDLIST_ABSOLUTE pidlRoot)
{
   // Get the filename of the Windows source file.
   WCHAR wszTarFilename[MAX_PATH] = L""; 
   // Initialize the .tar archive; this operation doesn't actually touch
   // the file because a file-system is spawned for many operations
   // that doesn't need the physical files.
   HR( DMOpen(wszTarFilename, &m_pArchive) );
   return S_OK;
}

VOID CTarFileSystem::AddRef()
{
   ::InterlockedIncrement(&m_cRef);
}

VOID CTarFileSystem::Release()
{
   if( ::InterlockedDecrement(&m_cRef) == 0 ) delete this;
}

/**
 * Create the root NSE Item instance.
 */
CNseItem* CTarFileSystem::GenerateRoot(CShellFolder* pFolder)
{
   return new CTarFileItem(pFolder, NULL, NULL, FALSE);
}

