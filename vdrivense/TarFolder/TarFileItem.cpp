
#include "stdafx.h"

#include "TarFileSystem.h"
#include "ShellFolder.h"
#include "LaunchFile.h"
#include "EnumIDList.h"


///////////////////////////////////////////////////////////////////////////////
// CTarFileItem

CTarFileItem::CTarFileItem(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem) :
   CNseFileItem(pFolder, pidlFolder, pidlItem, bReleaseItem)
{
}

/**
 * Return SFGAOF flags for this item.
 * These flags tell the Shell about the capabilities of this item. Here we
 * can toggle the ability to copy/delete/rename an item.
 */
SFGAOF CTarFileItem::GetSFGAOF(SFGAOF dwMask)
{
   return CNseFileItem::GetSFGAOF(dwMask)
          | SFGAO_CANCOPY
          | SFGAO_CANMOVE
          | SFGAO_CANDELETE
          | SFGAO_CANRENAME
          | SFGAO_HASPROPSHEET;
}

/**
 * Return item information.
 */
HRESULT CTarFileItem::GetProperty(REFPROPERTYKEY pkey, CComPropVariant& v)
{
   if( pkey == PKEY_Volume_FileSystem ) {
      return ::InitPropVariantFromString(L"TarFS", &v);
   }
   return CNseFileItem::GetProperty(pkey, v);
}

/**
 * Set item information.
 */
HRESULT CTarFileItem::SetProperty(REFPROPERTYKEY pkey, const CComPropVariant& v)
{
   // In the Property Page, the user is allowed to
   // modify the file-attributes.
   if( pkey == PKEY_FileAttributes ) {
      ULONG uFileAttribs = 0;
      HR( ::PropVariantToUInt32(v, &uFileAttribs) );
      WCHAR wszFilename[MAX_PATH] = { 0 };
      HR( _GetPathnameQuick(m_pidlFolder, m_pidlItem, wszFilename) );
      HR( DMSetFileAttr(_GetTarArchivePtr(), wszFilename, uFileAttribs) );
      // Update properties of our NSE Item
      m_pWfd->dwFileAttributes = uFileAttribs;
      return S_OK;
   }
   return CNseFileItem::SetProperty(pkey, v);
}

/**
 * Create an NSE Item instance from a child PIDL.
 */
CNseItem* CTarFileItem::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem)
{
   // Validate that the child PIDL really is ours
   if( !CPidlMemPtr<NSEFILEPIDLDATA>(pidlItem).IsType(TARFILE_MAGIC_ID) ) return NULL;
   // Spawn new NSE Item instance
   return new CTarFileItem(pFolder, pidlFolder, pidlItem, bReleaseItem);
}

/**
 * Create an NSE Item instance from static data.
 *
 * HarryWu, 2014.2.2
 * pass parameter <WIN32_FIND_DATA:wfd> by ref should be more effective.
 * and values in <WIN32_FIND_DATA:wfd> present in debug mode is not correct,
 * if pass <WIN32_FIND_DATA:wfd> by value.
 */
CNseItem* CTarFileItem::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, const VFS_FIND_DATA & wfd)
{
   NSEFILEPIDLDATA data = { sizeof(NSEFILEPIDLDATA), TARFILE_MAGIC_ID, 1, wfd };
   return new CTarFileItem(pFolder, pidlFolder, GenerateITEMID(&data, sizeof(data)), TRUE);
}

/**
 * Look up a single item by filename.
 * Validate its existance outside any cache.
 */
HRESULT CTarFileItem::GetChild(LPCWSTR pwstrName, SHGNO ParseType, CNseItem** pItem)
{
   WCHAR wszFilename[MAX_PATH] = { 0 };
   HR( _GetPathnameQuick(m_pidlFolder, m_pidlItem, wszFilename) );
   ::PathAppend(wszFilename, pwstrName);
   
   LocalId parentId;
   HR( _GetIdQuick(m_pidlItem, &parentId));
   VFS_FIND_DATA wfd = { 0 };
   RFS_FIND_DATA * childList = NULL; int childCount = 0;
   DMGetChildrenList(_GetTarArchivePtr(), *(RemoteId *)&parentId, &childList, &childCount);
   for (int i = 0; i < childCount; i++)
   {
	   if (!wcsicmp(pwstrName, childList[i].cFileName)){
		   wfd = *(VFS_FIND_DATA *)&childList[i];
		   *pItem = GenerateChild(m_pFolder, m_pFolder->m_pidlFolder, wfd);
		   DMFree((LPBYTE)childList);
		   return *pItem != NULL ? S_OK : E_OUTOFMEMORY;
	   }
   }
   DMFree((LPBYTE)childList);
   return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
}

/**
 * Retrieve the list of children of the current folder item.
 */
HRESULT CTarFileItem::EnumChildren(HWND hwndOwner, SHCONTF grfFlags, CSimpleValArray<CNseItem*>& aItems)
{
   // Only directories have sub-items
   if( !IsFolder() ) return E_HANDLE;

   // Get actual path and retrieve a list of sub-items
   WCHAR wszPath[MAX_PATH] = { 0 };
   HR( _GetPathnameQuick(m_pidlFolder, m_pidlItem, wszPath) );

   // HarryWu, 2014.2.18
   // Get the actual id to retrieve a list of sub-items
   LocalId dwId = {0,0};
   HR( _GetIdQuick(m_pidlItem, &dwId));

   // HarryWu, 2014.1.29
   // Note!, it is NOT safe to pass c++ objects array between modules.
   // use /MD to genereate these modules.
   RFS_FIND_DATA * aList = NULL; int nListCount = 0;
   HR( DMGetChildrenList(_GetTarArchivePtr(), *(RemoteId*)&dwId, &aList, &nListCount) );
   for( int i = 0; i < nListCount; i++ ) {
      // Filter item according to the 'grfFlags' argument
      if( SHFilterEnumItem(grfFlags, *(WIN32_FIND_DATA *)(&aList[i])) != S_OK ) continue;
      // Create an NSE Item from the file-info data
      aItems.Add( GenerateChild(m_pFolder, m_pFolder->m_pidlFolder, *(VFS_FIND_DATA *)(&aList[i])) );
   }
   DMFree((LPBYTE)aList);
   return S_OK;
}

/**
 * Produce a file-stream instance.
 */
HRESULT CTarFileItem::GetStream(const VFS_STREAM_REASON& Reason, CNseFileStream** ppFile)
{
   WCHAR wszFilename[MAX_PATH] = { 0 };
   HR( _GetPathnameQuick(m_pidlFolder, m_pidlItem, wszFilename) );
   
   LocalId parentId;
   HR( _GetViewIdQuick(m_pidlFolder, &parentId));
   
   LocalId itemId;
   HR(_GetIdQuick(m_pidlItem, &itemId));

   *ppFile = new CTarFileStream(static_cast<CTarFileSystem*>(m_pFolder->m_spFS.m_p), parentId, itemId, wszFilename, Reason.uAccess);
   return *ppFile != NULL ? S_OK : E_OUTOFMEMORY;
}

/**
 * Create a new directory.
 */
HRESULT CTarFileItem::CreateFolder()
{
   LocalId parentId = {0,0};
   HR( _GetViewIdQuick(m_pidlFolder, &parentId));

   // Create new directory in archive
   WCHAR wszFilename[MAX_PATH] = { 0 };
   HR( _GetPathnameQuick(m_pidlFolder, m_pidlItem, wszFilename) );

   OUTPUTLOG("%s(), pidlFodler=%p, pidlItem=%p", __FUNCTION__, m_pidlFolder, m_pidlItem);

   HR( DMCreateFolder(_GetTarArchivePtr(), *(RemoteId *)&parentId, wszFilename, (RFS_FIND_DATA *)m_pWfd) );
   return S_OK;
}

/**
 * Rename this item.
 */
HRESULT CTarFileItem::Rename(LPCWSTR pstrNewName, LPWSTR pstrOutputName)
{
   // Rename the item in archive
   HR( DMRename(_GetTarArchivePtr(), *(RemoteId *)&m_pWfd->dwId, pstrOutputName) );
   return S_OK;
}

/**
 * Delete this item.
 */
HRESULT CTarFileItem::Delete()
{
   HR( DMDelete(_GetTarArchivePtr(), *(RemoteId *)(&m_pWfd->dwId)) );
   return S_OK;
}

/**
 * Returns the menu-items for an item.
 */
HMENU CTarFileItem::GetMenu()
{
   UINT uMenuRes = IDM_FILE;
   if( IsFolder() ) uMenuRes = IDM_FOLDER;
   return ::LoadMenu(_pModule->GetResourceInstance(), MAKEINTRESOURCE(uMenuRes));
}

/**
 * Execute a menucommand.
 */
HRESULT CTarFileItem::ExecuteMenuCommand(VFS_MENUCOMMAND& Cmd)
{
   switch( Cmd.wMenuID ) {
   case ID_FILE_PREVIEW:     return _PreviewFile(GetITEMID());
   case ID_FILE_OPEN:        return LoadAndLaunchFile(Cmd.hWnd, m_pFolder, GetITEMID());
   case ID_FILE_EXTRACT:     return _ExtractToFolder(Cmd);
   case ID_COMMAND_EXTRACT:  return _ExtractToFolder(Cmd);
   case DFM_CMD_PASTE:       return _DoPasteFiles(Cmd);
   case DFM_CMD_NEWFOLDER:   return _DoNewFolder(Cmd, IDS_NEWFOLDER);
   case ID_FILE_NEWFOLDER:   return _DoNewFolder(Cmd, IDS_NEWFOLDER);
   case ID_FILE_PROPERTIES:  return _DoShowProperties(Cmd);
   }
   return E_NOTIMPL;
}

// Implementation

TAR_ARCHIVE* CTarFileItem::_GetTarArchivePtr() const
{
   // NOTE: There is a nasty downcast going on here. It is c++ safe since we have
   //       single inheritance only.
   return static_cast<CTarFileSystem*>(m_pFolder->m_spFS.m_p)->m_pArchive;
}

/**
 * Extract a file from tar archive to file-system.
 * This method is invoked from one of the extra menuitems or command buttons
 * we added and prompts the user to extract the selected item(s) to a particular
 * disk folder.
 */
HRESULT CTarFileItem::_ExtractToFolder(VFS_MENUCOMMAND& Cmd)
{
   // Prompt the user to choose the target path first...
   if( Cmd.pUserData == NULL ) {
      CPidl pidlFolder;
      CComBSTR bstrTitle;
      bstrTitle.LoadString(IDS_EXTRACTTITLE);
      BROWSEINFO bi = { Cmd.hWnd, NULL, NULL, bstrTitle, BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_VALIDATE, NULL, 0, 0 };
      pidlFolder.Attach( ::SHBrowseForFolder(&bi) );
      if( pidlFolder.IsNull() ) return E_ABORT;      
      LPTSTR pstrTarget = (LPTSTR) malloc(MAX_PATH * sizeof(TCHAR));
      HR( ::SHGetPathFromIDList(pidlFolder, pstrTarget) );
      Cmd.pUserData = pstrTarget;
   }
   // Extract this item to target folder.
   // Let the Windows CopyEngine do the dirty work simply by copying the
   // file/folder out of this item.
   if( Cmd.pFO == NULL ) {
      HR( ::SHCreateFileOperation(Cmd.hWnd, FOF_NOCOPYSECURITYATTRIBS | FOFX_NOSKIPJUNCTIONS | FOF_NOCONFIRMMKDIR, &Cmd.pFO) );
   }
   if( IsRoot() ) {
      // Item is the archive itself; copy its immediate children
      CNseItemArray aList;
      HR( EnumChildren(Cmd.hWnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, aList) );
      for( int i = 0; i < aList.GetSize(); i++ ) HR( aList[i]->ExecuteMenuCommand(Cmd) );
   }
   else {
      // Item is a file/folder inside the archive
      CComPtr<IShellItem> spSourceFile, spTargetFolder;
      CPidl pidlFile(m_pFolder->m_pidlRoot, m_pidlFolder, m_pidlItem);
      HR( ::SHCreateItemFromIDList(pidlFile, IID_PPV_ARGS(&spSourceFile)) );
      HR( ::SHCreateItemFromParsingName(static_cast<LPCTSTR>(Cmd.pUserData), NULL, IID_PPV_ARGS(&spTargetFolder)) );
      HR( Cmd.pFO->CopyItem(spSourceFile, spTargetFolder, m_pWfd->cFileName, NULL) );
   }
   return S_OK;
}

HRESULT CTarFileItem::_PreviewFile( PCITEMID_CHILD pidl)
{
	MessageBox(GetActiveWindow(), _T("Preview"), _T("VDrive"), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}