#include "stdafx.h"
#include "shfrwkres.h"

#include "ShellFolder.h"
#include "EnumIDList.h"
#include "QueryInfo.h"
#include "DataObject.h"
#include "FileStream.h"
#include "DropTarget.h"
#include "IdentityName.h"
#include "TransferItem.h"
#include "PropertyStore.h"
#include "CommandProvider.h"
#include "CategoryProvider.h"

#pragma warning( disable : 4510 4610 4996)


//////////////////////////////////////////////////////////////////////
// CShellFolder

HRESULT WINAPI CShellFolder::UpdateRegistry(BOOL bRegister) throw()
{
   // Prepare lookup table for different placements
   CComBSTR bstrExtension;
   bstrExtension.LoadString(IDS_NSE_FILEEXTENSION);
   struct { 
      UINT nRegistryResource;
      CComBSTR bstrName;
      REFKNOWNFOLDERID RefreshFolderId;
   } aLocations[] = 
   {
      /* VFS_LOCATION_JUNCTION */   { IDR_SHELL_LOC_JUNCTION,   bstrExtension,           FOLDERID_Desktop },
      /* VFS_LOCATION_DESKTOP */    { IDR_SHELL_LOC_ROOTED,     L"Desktop",              FOLDERID_Desktop },
      /* VFS_LOCATION_MYCOMPUTER */ { IDR_SHELL_LOC_ROOTED,     L"MyComputer",           FOLDERID_ComputerFolder },
      /* VFS_LOCATION_USERFILES */  { IDR_SHELL_LOC_ROOTED,     L"UserFiles",            FOLDERID_UsersFiles },      
      /* VFS_LOCATION_NETHOOD */    { IDR_SHELL_LOC_ROOTED,     L"NetworkNeighborhood",  FOLDERID_NetworkFolder },
      /* VFS_LOCATION_REMOTE */     { IDR_SHELL_LOC_ROOTED,     L"RemoteComputer",       FOLDERID_NetworkFolder },
      /* VFS_LOCATION_PRINTERS */   { IDR_SHELL_LOC_ROOTED,     L"PrintersAndFaxes",     FOLDERID_PrintersFolder },
   };
   int iLocationIndex = _ShellModule.GetConfigInt(VFS_INT_LOCATION);
   ATLASSERT(iLocationIndex<lengthof(aLocations));
   // We define a couple of meta-tokens for the registry script
   // so it is more generic.
   WCHAR wszTemp[50];
   CComBSTR bstrCLSID = CLSID_ShellFolder;
   CComBSTR bstrSendToCLSID = CLSID_SendTo;
   CComBSTR bstrPreviewCLSID = CLSID_Preview;
   CComBSTR bstrDropTargetCLSID = CLSID_DropTarget;
   CComBSTR bstrContextMenuCLSID = CLSID_ContextMenu;   
   CComBSTR bstrPropertySheetCLSID = CLSID_PropertySheet;
   CComBSTR bstrLocation = aLocations[iLocationIndex].bstrName;
   CComBSTR bstrProjectName, bstrDisplayName, bstrDescription, bstrUrlProtocol, bstrAttribs, bstrPreviewHost;
   bstrProjectName.LoadString(IDS_NSE_PROJNAME);
   bstrDisplayName.LoadString(IDS_NSE_DISPLAYNAME);
   bstrDescription.LoadString(IDS_NSE_DESCRIPTION);
   bstrUrlProtocol.LoadString(IDS_NSE_URLPROTOCOL);
   bstrPreviewHost = L"{6D2B5079-2F0B-48DD-AB7F-97CEC514D30B}";
   // 32bit PreviewHost under 64bit Windows
   BOOL bIsWOW64 = FALSE; ::IsWow64Process(::GetCurrentProcess(), &bIsWOW64);
   if( bIsWOW64 ) bstrPreviewHost = L"{534A1E02-D58F-44F0-B58B-36CBED287C7C}";
   // Format Root attributes
   ::wsprintf(wszTemp, L"&H%08X", _ShellModule.GetConfigInt(VFS_INT_SHELLROOT_SFGAO)); 
   bstrAttribs = wszTemp;
   // Format Location and find its associated registry script
   _ATL_REGMAP_ENTRY regMapEntries[] =
   {
      { L"CLSID", bstrCLSID },
      { L"PROJECTNAME", bstrProjectName },
      { L"DISPLAYNAME", bstrDisplayName },
      { L"DESCRIPTION", bstrDescription },
      { L"FOLDERATTRIBS", bstrAttribs },
      { L"LOCATION", bstrLocation },
      { L"URLPROTOCOL", bstrUrlProtocol },
      { L"PREVIEWHOST", bstrPreviewHost },
      { L"SENDTOCLSID", bstrSendToCLSID },
      { L"PREVIEWCLSID", bstrPreviewCLSID },
      { L"DROPTARGETCLSID", bstrDropTargetCLSID },
      { L"CONTEXTMENUCLSID", bstrContextMenuCLSID },
      { L"PROPERTYSHEETCLSID", bstrPropertySheetCLSID },
      { NULL, NULL }
   };
   HRESULT Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELLFOLDER, bRegister, regMapEntries);
   if( SUCCEEDED(Hr) ) {
      Hr = _AtlModule.UpdateRegistryFromResource(aLocations[iLocationIndex].nRegistryResource, bRegister, regMapEntries);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_SENDTO) ) {
      Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELL_ROOT_SENDTO, bRegister, regMapEntries);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_PREVIEW) ) {
      Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELL_ROOT_PREVIEW, bRegister, regMapEntries);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_SHELLNEW) ) {
      Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELL_ROOT_SHELLNEW, bRegister, regMapEntries);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_PROPSHEET) ) {
      Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELL_ROOT_PROPSHEET, bRegister, regMapEntries);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_DROPTARGET) ) {
      Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELL_ROOT_DROPTARGET, bRegister, regMapEntries);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_CONTEXTMENU) ) {
      Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELL_ROOT_CONTEXTMENU, bRegister, regMapEntries);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_CUSTOMSCRIPT) ) {
      Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELL_ROOT_CUSTOMSCRIPT, bRegister, regMapEntries);
   }   
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_PROPERTIES) ) {
      Hr = UpdatePropertySchemaFromResource(IDR_SHELL_ROOT_PROPERTIES, bRegister);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_URLPROTOCOL) ) {
      ATLASSERT(bstrUrlProtocol.Length()>0);
      Hr = _AtlModule.UpdateRegistryFromResource(IDR_SHELL_ROOT_URLPROTOCOL, bRegister, regMapEntries);
   }   
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_SENDTO) ) {
      ATLASSERT(_ShellModule.GetConfigInt(VFS_INT_LOCATION)!=VFS_LOCATION_JUNCTION);
      Hr = UpdateSendToFromProject(bstrDisplayName, bstrProjectName, bRegister);
   }
   if( SUCCEEDED(Hr) && _ShellModule.GetConfigBool(VFS_CAN_ROOT_STARTMENU_LINK) ) {
      ATLASSERT(_ShellModule.GetConfigInt(VFS_INT_LOCATION)!=VFS_LOCATION_JUNCTION);
      Hr = UpdateStartMenuLink(bstrDisplayName, bstrDescription, aLocations[iLocationIndex].RefreshFolderId, bRegister);
   }
   // Refresh shell...
   CPidl pidlKnownFolder;
   pidlKnownFolder.CreateFromKnownFolder(aLocations[iLocationIndex].RefreshFolderId);
   ::SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlKnownFolder);
   ::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
   return Hr;
}

HRESULT CShellFolder::FinalConstruct()
{
   ATLTRACE(L"CShellFolder::FinalConstruct\n");
   m_hwndOwner = NULL;
   m_hShellDefView = NULL;
   m_hMenu = m_hContextMenu = NULL;
   m_pShellView = NULL;
   m_pSortKey = PKEY_Null;
   m_iSortDirection = 0;
   return S_OK;
}

void CShellFolder::FinalRelease()
{
   ATLTRACE(L"CShellFolder::FinalRelease\n");
   if( ::IsMenu(m_hMenu) ) ::DestroyMenu(m_hMenu);
   if (m_pShellView) {m_pShellView->Release(); m_pShellView = NULL; }
}

// IPersist

STDMETHODIMP CShellFolder::GetClassID(CLSID* pclsid)
{
   ATLTRACE(L"CShellFolder::GetClassID\n");
   *pclsid = GetObjectCLSID();
   return S_OK;
}

// IPersistFolder

STDMETHODIMP CShellFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
   ATLTRACE(L"CShellFolder::Initialize\n", this);
   // Spawn a file-system
   CRefPtr<CNseFileSystem> spFS;
   HR( _ShellModule.CreateFileSystem(pidl, &spFS) );
   m_spFS = spFS;
   // Clear data members
   m_pidlRoot = pidl;
   m_pidlParent.Delete();
   m_pidlFolder.Delete();
   m_pidlMonitor = m_pidlRoot;
   // Generate an item for the root folder
   m_spFolderItem = m_spFS->GenerateRoot(this);
   return S_OK;
}

// IPersistFolder2

STDMETHODIMP CShellFolder::GetCurFolder(PIDLIST_ABSOLUTE* ppidl)
{
   ATLTRACE(L"CShellFolder::GetCurFolder\n");
   return m_pidlMonitor.CopyTo(ppidl);
}

// IPersistFolder3

STDMETHODIMP CShellFolder::InitializeEx(IBindCtx* pbc, PCIDLIST_ABSOLUTE pidlRoot, const PERSIST_FOLDER_TARGET_INFO* pPFTI)
{
   ATLTRACE(L"CShellFolder::InitializeEx\n");
   return Initialize(pidlRoot);
}

STDMETHODIMP CShellFolder::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO* pPFTI)
{
   ::ZeroMemory(pPFTI, sizeof(PERSIST_FOLDER_TARGET_INFO));
   ATLTRACENOTIMPL(L"CShellFolder::GetFolderTargetInfo");
}

// IPersistIDlist

STDMETHODIMP CShellFolder::SetIDList(PCIDLIST_ABSOLUTE pidl)
{
   ATLTRACENOTIMPL(L"CShellFolder::SetIDList");
}

STDMETHODIMP CShellFolder::GetIDList(PIDLIST_ABSOLUTE* ppidl)
{
   ATLTRACE(L"CShellFolder::GetIDList\n");
   return m_pidlMonitor.CopyTo(ppidl);
}

// IShellFolder

STDMETHODIMP CShellFolder::ParseDisplayName(HWND hwnd, IBindCtx* pbc, PWSTR pszDisplayName, ULONG* pchEaten, PIDLIST_RELATIVE* ppidl, ULONG* pdwAttributes)
{
   ATLTRACE(L"CShellFolder::ParseDisplayName  name='%ws' pbc=%p\n", pszDisplayName, pbc);
   ATLTRACENOTIMPL(_T("CShellFolder::ParseDisplayName"));
}

STDMETHODIMP CShellFolder::EnumObjects(HWND hwnd, SHCONTF grfFlags, IEnumIDList** ppEnumIDList)
{
   ATLTRACE(L"CShellFolder::EnumObjects  hwnd=%X flags=0x%X [SHCONT_%s]\n", hwnd, grfFlags, DbgGetSHCONTF(grfFlags));
   *ppEnumIDList = NULL;
   CComObject<CEnumIDList>* pEnum = NULL;
   HR( CComObject<CEnumIDList>::CreateInstance(&pEnum) );
   CComPtr<IUnknown> spKeepAlive = pEnum->GetUnknown();
   HR( pEnum->Init(this, hwnd, grfFlags) );
   return pEnum->QueryInterface(ppEnumIDList);
}

STDMETHODIMP CShellFolder::BindToObject(PCUIDLIST_RELATIVE pidl, IBindCtx* pbc, REFIID riid, LPVOID* ppRetVal)
{
   ATLTRACE(L"CShellFolder::BindToObject  riid=%s\n", DbgGetIID(riid));
   if( !::ILIsChild(pidl) )
   {
      CComPtr<IShellFolder> spShellFolder;
      PCUITEMID_CHILD pidlChild = NULL;
      HR( ::SHBindToFolderIDListParent(this, pidl, IID_PPV_ARGS(&spShellFolder), &pidlChild) );
      return spShellFolder->BindToObject(pidlChild, pbc, riid, ppRetVal);
   }
   // At this point, we are left with a child PIDL only
   ATLASSERT(::ILIsChild(pidl));
   PCUITEMID_CHILD pidlItem = static_cast<PCUITEMID_CHILD>(pidl);
   // Ask NSE Item first...
   if( _ShellModule.GetConfigBool(VFS_HAVE_OBJECTOF) ) {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      HRESULT Hr = spItem->GetObjectOf(VFS_OBJECTOF_BIND, NULL, riid, ppRetVal);
      if( SUCCEEDED(Hr) ) return Hr;
   }
   // Attach to child folder?
   if( riid == IID_IShellFolder || riid == IID_IShellFolder2 || riid == IID_IStorage || riid == IID_IObjectProvider )
   {
      CRefPtr<CShellFolder> spFolder;
      HR( BindToFolder(pidlItem, spFolder) );
      return spFolder->QueryInterface(riid, ppRetVal);
   }
   // Process standard objects...
   if( riid == IID_IStream )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      if( IsBitSet(spItem->GetSFGAOF(SFGAO_FOLDER), SFGAO_FOLDER) ) return E_NOINTERFACE;
      if( !IsBitSet(spItem->GetSFGAOF(SFGAO_STREAM), SFGAO_STREAM) ) return E_NOINTERFACE;
      CComObject<CFileStream>* pStream = NULL;
      HR( CComObject<CFileStream>::CreateInstance(&pStream) );
      CComPtr<IUnknown> spKeepAlive = pStream->GetUnknown();
      HR( pStream->Init(this, pidlItem) );
      return pStream->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_IMoniker )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      if( IsBitSet(spItem->GetSFGAOF(SFGAO_FOLDER), SFGAO_FOLDER) ) return E_NOINTERFACE;
      if( !IsBitSet(spItem->GetSFGAOF(SFGAO_CANMONIKER), SFGAO_CANMONIKER) ) return E_NOINTERFACE;
      return spItem->GetMoniker(reinterpret_cast<IMoniker**>(ppRetVal));
   }
   if( riid == IID_IPropertyStoreFactory )
   {      
      CComObject<CPropertyStoreFactory>* pPsFactory = NULL;
      HR( CComObject<CPropertyStoreFactory>::CreateInstance(&pPsFactory) );
      CComPtr<IUnknown> spKeepAlive = pPsFactory->GetUnknown();
      HR( pPsFactory->Init(this, pidlItem) );
      return pPsFactory->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_IPropertyStore )
   {      
      CComObject<CPropertyStoreFactory>* pPsFactory = NULL;
      HR( CComObject<CPropertyStoreFactory>::CreateInstance(&pPsFactory) );
      CComPtr<IUnknown> spKeepAlive = pPsFactory->GetUnknown();
      HR( pPsFactory->Init(this, pidlItem) );
      return pPsFactory->GetPropertyStore(GPS_DEFAULT, NULL, riid, ppRetVal);
   }
   if( riid == IID_IPropertySetStorage )
   {
      CComObject<CPropertyStoreFactory>* pPsFactory = NULL;
      HR( CComObject<CPropertyStoreFactory>::CreateInstance(&pPsFactory) );
      CComPtr<IUnknown> spKeepAlive = pPsFactory->GetUnknown();
      HR( pPsFactory->Init(this, pidlItem) );
      CComPtr<IPropertyStore> spStore;
      HR( pPsFactory->GetPropertyStore(GPS_DELAYCREATION, NULL, IID_PPV_ARGS(&spStore)) );
      return ::PSCreateAdapterFromPropertyStore(spStore, riid, ppRetVal);
   }
   if( riid == IID_IExtractImage )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      return spItem->GetExtractIcon(riid, ppRetVal);
   }
   if( riid == IID_IThumbnailProvider )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      return spItem->GetThumbnail(riid, ppRetVal);
   }
   if( riid == IID_ITransferMediumItem )
   {
      CComObject<CTransferMediumItem>* pTransferMediumItem = NULL;
      HR( CComObject<CTransferMediumItem>::CreateInstance(&pTransferMediumItem) );
      CComPtr<IUnknown> spKeepAlive = pTransferMediumItem->GetUnknown();
      HR( pTransferMediumItem->Init(this, pidlItem) );
      return pTransferMediumItem->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_IIdentityName )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      CComObject<CIdentityName>* pIdentityName = NULL;
      HR( CComObject<CIdentityName>::CreateInstance(&pIdentityName) );
      CComPtr<IUnknown> spKeepAlive = pIdentityName->GetUnknown();
      if( FAILED( pIdentityName->Init(this, pidlItem) ) ) return E_FAIL;
      return pIdentityName->QueryInterface(riid, ppRetVal);
   }
   ATLTRACE(L"CShellFolder::BindToObject - failed\n");
   return E_NOINTERFACE;
}

STDMETHODIMP CShellFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, IBindCtx* pbc, REFIID riid, LPVOID* ppRetVal)
{
   ATLTRACE(L"CShellFolder::BindToStorage\n", this);
   // Ask NSE Item first...
   if( _ShellModule.GetConfigBool(VFS_HAVE_OBJECTOF) ) {
      PCUITEMID_CHILD pidlItem = static_cast<PCUITEMID_CHILD>(pidl);
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      HRESULT Hr = spItem->GetObjectOf(VFS_OBJECTOF_STORAGE, NULL, riid, ppRetVal);
      if( SUCCEEDED(Hr) ) return Hr;
   }
   // Fall back to BindToObject call...
   return BindToObject(pidl, pbc, riid, ppRetVal);
}

STDMETHODIMP CShellFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
   ATLTRACE(L"CShellFolder::CompareIDs  column=%u flags=0x%X\n", LOWORD(lParam), HIWORD(lParam));
   CRefPtr<CShellFolder> spFolder = this;
   BOOL bHaveUniqueNames = _ShellModule.GetConfigBool(VFS_HAVE_UNIQUE_NAMES);
   UINT iSortColumn = LOWORD(lParam);
   UINT uColumnFlags = HIWORD(lParam);
   VFS_COLUMNINFO Column = { 0 };
   HR( m_spFolderItem->GetColumnInfo(iSortColumn, Column) );
   UINT nMaxSpanColumns = 3; 
   PROPERTYKEY SortKey = Column.pkey;
   if( IsBitSet(uColumnFlags, SHCIDS_ALLFIELDS) ) nMaxSpanColumns = 6, bHaveUniqueNames = FALSE;
   if( IsBitSet(uColumnFlags, SHCIDS_CANONICALONLY) && bHaveUniqueNames ) SortKey = PKEY_ParsingName;
   while( !::ILIsEmpty(pidl1) || !::ILIsEmpty(pidl2) ) 
   {
      if( ::ILIsEmpty(pidl1) ) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(-1));
      if( ::ILIsEmpty(pidl2) ) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(1));
      CNseItemPtr spItem1 = spFolder->GenerateChildItem(static_cast<PCUITEMID_CHILD>(pidl1));
      CNseItemPtr spItem2 = spFolder->GenerateChildItem(static_cast<PCUITEMID_CHILD>(pidl2));
      if( spItem1 == NULL || spItem2 == NULL ) return E_FAIL;
      // HarryWu, 2014.09.25
      // custom sorting in root space.
      // always in this order, public->personal->recycle bin->search bin
      if (::ILIsEmpty(m_pidlFolder)){
          VFS_FIND_DATA wfd1 = spItem1->GetFindData(); VFS_FIND_DATA wfd2 = spItem2->GetFindData();
          return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(wfd1.dwId.category - wfd2.dwId.category));
      }
      // Items may be sorted by DIRECTORY attribute first
      if( !IsBitSet(Column.dwFlags, SHCOLSTATE_NOSORTBYFOLDERNESS) ) {
         CComPropVariant vfd1, vfd2;
         spItem1->GetProperty(PKEY_FileAttributes, vfd1);
         spItem2->GetProperty(PKEY_FileAttributes, vfd2);
         vfd1.ulVal &= FILE_ATTRIBUTE_DIRECTORY;
         vfd2.ulVal &= FILE_ATTRIBUTE_DIRECTORY;
         int iRes = vfd2.CompareTo(vfd1);
         if( iRes != 0 ) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(iRes));
      }
      // Items are sorted by chosen column
      CComPropVariant v1, v2;
      spItem1->GetProperty(SortKey, v1);
      spItem2->GetProperty(SortKey, v2);
      int iRes = v1.CompareTo(v2);
      if( iRes != 0 ) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(iRes));
      // Finally, we sort items by the next few remaining columns in order.
      // We do this especially if the primary sort-column is not the 1st column (assumed
      // to be the items' Title), so a sort on Type will use the Title as its second
      // sorting field.
      if( !bHaveUniqueNames || iSortColumn != 0 ) {
         for( UINT iColumn = 0; iColumn < nMaxSpanColumns; iColumn++ ) {
            if( iSortColumn == iColumn ) continue;
            VFS_COLUMNINFO Column2 = { 0 };
            if( FAILED( m_spFolderItem->GetColumnInfo(iColumn, Column2) ) ) continue;
            if( !IsBitSet(Column2.dwFlags, SHCOLSTATE_ONBYDEFAULT) ) continue;
            CComPropVariant vc1, vc2;
            spItem1->GetProperty(Column2.pkey, vc1);
            spItem2->GetProperty(Column2.pkey, vc2);
            iRes = vc1.CompareTo(vc2);
            if( iRes != 0 ) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(iRes));
         }
      }
      // If this is a relative PIDL, traverse next sub-folder
      if( !::ILIsChild(pidl1) ) {
         CRefPtr<CShellFolder> spChildFolder;
         CPidl pidlChild( (PCUITEMID_CHILD) pidl1 );
         HR( spFolder->BindToFolder(pidlChild.GetItem(0), spChildFolder) );
         spFolder = spChildFolder;
      }
      // Ready for next PIDL level
      pidl1 = ::ILGetNext(pidl1);
      pidl2 = ::ILGetNext(pidl2);
   }
   return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

STDMETHODIMP CShellFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID* ppRetVal)
{
   ATLTRACE(L"CShellFolder::CreateViewObject  riid=%s\n", DbgGetIID(riid));
   ATLASSERT(m_spFolderItem);
   if( m_spFolderItem == NULL ) return E_UNEXPECTED;
   // Ask Folder NSE Item first...
   if( _ShellModule.GetConfigBool(VFS_HAVE_OBJECTOF) ) {
      HRESULT Hr = m_spFolderItem->GetObjectOf(VFS_OBJECTOF_VIEW, hwndOwner, riid, ppRetVal);
      if( SUCCEEDED(Hr) ) return Hr;
   }
   // Process standard objects...
   if( riid == IID_IShellView )
   {
      m_hwndOwner = hwndOwner;
      SFV_CREATE sfvData = { sizeof(sfvData), 0 };
      CComQIPtr<IShellFolder> spFolder = GetUnknown();
      sfvData.psfvcb = this;
      sfvData.pshf = spFolder;
      HRESULT Hr = ::SHCreateShellFolderView(&sfvData, (IShellView**)ppRetVal);
      if (Hr == S_OK){
          if (m_pShellView){m_pShellView->Release(); m_pShellView = NULL;}
          m_pShellView = (IShellView *)*ppRetVal;
          m_pShellView->AddRef();
      }

      if (!DMSearchBarIsEnable()){
          // And remove default system search bar
          _RemoveSystemSearchBar(hwndOwner);
      }

      return Hr;
   }
   if( riid == IID_IDropTarget ) 
   {
      CComObject<CDropTarget>* pDropTarget = NULL;
      HR( CComObject<CDropTarget>::CreateInstance(&pDropTarget) );
      CComPtr<IUnknown> spKeepAlive = pDropTarget->GetUnknown();
      HR( pDropTarget->Init(this, NULL, hwndOwner) );
      return pDropTarget->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_IContextMenu )
   {
      if( ::IsMenu(m_hMenu) ) ::DestroyMenu(m_hMenu);
      m_hMenu = m_spFolderItem->GetMenu();
      m_hContextMenu = ::GetSubMenu(m_hMenu, _T("ViewMenu"));
      if (m_pidlFolder.IsEmpty()) return E_FAIL;

      PCUITEMID_CHILD pidlCurrentFolder = (PCUITEMID_CHILD)::ILClone(m_pidlFolder.GetLastItem());
      if (pidlCurrentFolder){
          HR (_RefineUserMenuItems(m_hContextMenu, 1, &pidlCurrentFolder));
          ::ILFree((PIDLIST_RELATIVE)pidlCurrentFolder);
      }

      DEFCONTEXTMENU dcm = { hwndOwner, static_cast<IContextMenuCB*>(this), m_pidlMonitor, static_cast<IShellFolder*>(this), 0, NULL, NULL, 0, NULL };
      return ::SHCreateDefaultContextMenu(&dcm, riid, ppRetVal);
   }
   if( riid == IID_IExplorerCommandProvider )
   {
      CComObject<CExplorerCommandProvider>* pCommandProvider = NULL;
      HR( CComObject<CExplorerCommandProvider>::CreateInstance(&pCommandProvider) );
      CComPtr<IUnknown> spKeepAlive = pCommandProvider->GetUnknown();     
      HRESULT Hr = pCommandProvider->Init(this, hwndOwner, m_spFolderItem->GetMenu());
      if( FAILED(Hr) ) return Hr;
      return pCommandProvider->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_ICategoryProvider )
   {
      CComObject<CCategoryProvider>* pCategoryProvider = NULL;
      HR( CComObject<CCategoryProvider>::CreateInstance(&pCategoryProvider) );
      CComPtr<IUnknown> spKeepAlive = pCategoryProvider->GetUnknown();
      HR( pCategoryProvider->Init(this) );
      return pCategoryProvider->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_ITransferSource )
   {
      CComObject<CTransferSource>* pTransferSource = NULL;
      HR( CComObject<CTransferSource>::CreateInstance(&pTransferSource) );
      CComPtr<IUnknown> spKeepAlive = pTransferSource->GetUnknown();
      HR( pTransferSource->Init(this) );
      return pTransferSource->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_ITransferDestination )
   {
      CComObject<CTransferDestination>* pTransferDestination = NULL;
      HR( CComObject<CTransferDestination>::CreateInstance(&pTransferDestination) );
      CComPtr<IUnknown> spKeepAlive = pTransferDestination->GetUnknown();
      HR( pTransferDestination->Init(this) );
      return pTransferDestination->QueryInterface(riid, ppRetVal);
   }
   if( riid == CLSID_ShellFolder )
   {
      // Allows internal objects to query the C++ implementation of this IShellFolder instance.
      // This violates basic COM sense, but we need this as a programming shortcut. It restricts
      // our Shell Extension to only work as an in-proc component.
      AddRef(); 
      *ppRetVal = this;
      return S_OK;
   }
   return QueryInterface(riid, ppRetVal);
}

STDMETHODIMP CShellFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY rgpidl, SFGAOF* rgfInOut)
{
   ATLTRACE(L"CShellFolder::GetAttributesOf  count=%u mask=0x%X [SFGAO_%s]\n", cidl, *rgfInOut, DbgGetSFGAOF(*rgfInOut));
   DWORD dwOrigAttributes = *rgfInOut;
   if( *rgfInOut == 0 ) *rgfInOut = (SFGAOF)(~SFGAO_VALIDATE);
   // Gather attribute information for child items...
   for( UINT i = 0; i < cidl; i++ ) {
      CNseItemPtr spItem = GenerateChildItem(rgpidl[i]);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      *rgfInOut &= spItem->GetSFGAOF(dwOrigAttributes);
      // Must honor SFGAO_VALIDATE flag: we need to trigger an actual lookup rather
      // than possibly just fetching information out of the PIDL structure.
      if( IsBitSet(dwOrigAttributes, SFGAO_VALIDATE) ) {
         CNseItemPtr spTemp;
         const VFS_FIND_DATA wfd = spItem->GetFindData();
         HRESULT Hr = m_spFolderItem->GetChild(wfd.cFileName, SHGDN_FORPARSING, &spTemp);
         if( FAILED(Hr) ) return Hr;
      }
   }
   return S_OK;
}

STDMETHODIMP CShellFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY rgpidl, REFIID riid, UINT* rgfReserved, LPVOID* ppRetVal)
{
   ATLTRACE(L"CShellFolder::GetUIObjectOf  count=%u riid=%s\n", cidl, DbgGetIID(riid));
   *ppRetVal = NULL;
   if( cidl == 0 ) return E_INVALIDARG;
   if( rgpidl == NULL ) return E_INVALIDARG;
   PCUITEMID_CHILD pidlItem = static_cast<PCUITEMID_CHILD>(rgpidl[0]);
   // Ask NSE Item first...
   if( _ShellModule.GetConfigBool(VFS_HAVE_OBJECTOF) ) {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      HRESULT Hr = spItem->GetObjectOf(VFS_OBJECTOF_UI, hwndOwner, riid, ppRetVal);
      if( SUCCEEDED(Hr) ) return Hr;
   }
   // Process standard objects...
   if( riid == IID_IDataObject )
   {
      CComObject<CDataObject>* pDataObject = NULL;
      HR( CComObject<CDataObject>::CreateInstance(&pDataObject) );
      CComPtr<IUnknown> spKeepAlive = pDataObject->GetUnknown();
      CComPtr<IDataObject> spMaster;
      HR( ::SHCreateDataObject(m_pidlMonitor, cidl, rgpidl, pDataObject, IID_PPV_ARGS(&spMaster)) );
      HR( pDataObject->Init(this, hwndOwner, spMaster, cidl, rgpidl) );
      HRESULT Hr = m_spFolderItem->SetDataObject(spMaster);
      if( FAILED(Hr) && Hr != E_NOTIMPL ) return Hr;
      return spMaster->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_IExtractIconA || riid == IID_IExtractIconW || riid == IID_IExtractImage )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      return spItem->GetExtractIcon(riid, ppRetVal);
   }
   if( riid == IID_IDropTarget )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      if( !IsBitSet(spItem->GetSFGAOF(SFGAO_DROPTARGET), SFGAO_DROPTARGET) ) return E_FAIL;
      CComObject<CDropTarget>* pDropTarget = NULL;
      HR( CComObject<CDropTarget>::CreateInstance(&pDropTarget) );
      CComPtr<IUnknown> spKeepAlive = pDropTarget->GetUnknown();
      HR( pDropTarget->Init(this, pidlItem, hwndOwner) );
      return pDropTarget->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_IContextMenu )
   {
      // BUG: We don't support merging of menus for multiple items, but the menu
      //      can disable its menuitems if needed.
	  if( ::IsMenu(m_hMenu) ) ::DestroyMenu(m_hMenu);
	  CNseItemPtr spItem = GenerateChildItem(pidlItem);
	  if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
	  m_hMenu = spItem->GetMenu();
	  m_hContextMenu = ::GetSubMenu(m_hMenu, _T("ContextMenu"));
	  
	  if (cidl > 1){ // HarryWu, refine menu for multiple selections.
		  ::RemoveMenu(m_hContextMenu, ID_FILE_VIEWLOG, MF_BYCOMMAND);
		  ::RemoveMenu(m_hContextMenu, ID_FILE_INNERLINK, MF_BYCOMMAND);
		  if (!spItem->IsFolder()){
			  ::RemoveMenu(m_hContextMenu, ID_FILE_PREVIEW, MF_BYCOMMAND);
			  ::RemoveMenu(m_hContextMenu, ID_FILE_EXTEDIT, MF_BYCOMMAND);
			  ::RemoveMenu(m_hContextMenu, ID_FILE_LOCK, MF_BYCOMMAND);
			  ::RemoveMenu(m_hContextMenu, ID_FILE_UNLOCK, MF_BYCOMMAND);
			  ::RemoveMenu(m_hContextMenu, ID_FILE_OLDVERSION, MF_BYCOMMAND);
		  }
	  }

      HR (_RefineUserMenuItems(m_hContextMenu, cidl, rgpidl));
      
      DEFCONTEXTMENU dcm = { hwndOwner, static_cast<IContextMenuCB*>(this), m_pidlMonitor, static_cast<IShellFolder*>(this), cidl, rgpidl, NULL, 0, NULL };
      return ::SHCreateDefaultContextMenu(&dcm, riid, ppRetVal);
   }
   if( riid == IID_IQueryInfo && _ShellModule.GetConfigBool(VFS_HAVE_INFOTIPS) )
   {
      CComObject<CQueryInfo>* pQueryInfo = NULL;
      HR( CComObject<CQueryInfo>::CreateInstance(&pQueryInfo) );
      CComPtr<IUnknown> spKeepAlive = pQueryInfo->GetUnknown();
      HR( pQueryInfo->Init(this, pidlItem) );
      return pQueryInfo->QueryInterface(riid, ppRetVal);
   }
   if( riid == IID_IPreviewHandler )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      return spItem->GetPreview(riid, ppRetVal);
   }
   if( riid == IID_IQueryAssociations )
   {
      CNseItemPtr spItem = GenerateChildItem(pidlItem);
      if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
      CComBSTR bstrCLSID = GetObjectCLSID();
      ASSOCIATIONELEMENT rgAssoc[] = { { ASSOCCLASS_CLSID_STR, NULL, bstrCLSID }, { ASSOCCLASS_FOLDER, NULL, NULL } };
      return ::AssocCreateForClasses(rgAssoc, spItem->IsFolder() ? 2U : 1U, riid, ppRetVal);
   }
   ATLTRACE(L"CShellFolder::GetUIObjectOf - failed\n");
   return E_FAIL;
}

STDMETHODIMP CShellFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET* psrName)
{
   ATLTRACE(L"CShellFolder::GetDisplayNameOf  flags=0x%X [SHGDN_%s]\n", uFlags, DbgGetSHGDNF(uFlags));
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   // Ask NSE Item first...
   if( _ShellModule.GetConfigBool(VFS_HAVE_NAMEOF) ) {
      if( spItem->GetNameOf(uFlags, &(psrName->pOleStr)) == S_OK ) {
         psrName->uType = STRRET_WSTR;
         return S_OK;
      }
   }
   WCHAR wszName[300] = { 0 };
   const VFS_FIND_DATA wfd = spItem->GetFindData();
   // This is part of the hack to get the SaveAs dialog working.
   // We have redirected the file to a temporary file in the %TEMP% folder.
   if( IsBitSet(uFlags, SHGDN_FORPARSING) && wfd.cAlternateFileName[1] == VFS_HACK_SAVEAS_JUNCTION ) {
      return StrToSTRRET(IsBitSet(uFlags, SHGDN_INFOLDER) ? ::PathFindFileName(m_spFS->m_wszOpenSaveAsFilename) : m_spFS->m_wszOpenSaveAsFilename, psrName);
   }
   // Needs parsing- or display-name?
   bool bNeedsParsingName = IsBitSet(uFlags, SHGDN_FORPARSING);
   if( uFlags == (SHGDN_FORADDRESSBAR|SHGDN_FORPARSING) ) bNeedsParsingName = false;
   // Asking for the full pathname?
   if( IsBitSet(uFlags, SHGDN_FORPARSING) && !IsBitSet(uFlags, SHGDN_INFOLDER) ) {
      CCoTaskString strFolder;
      HR( ::SHGetNameFromIDList(m_pidlMonitor, bNeedsParsingName ? SIGDN_DESKTOPABSOLUTEPARSING : SIGDN_DESKTOPABSOLUTEEDITING, &strFolder) );
      wcscat_s(wszName, lengthof(wszName), strFolder);
      wcscat_s(wszName, lengthof(wszName), L"\\");
   }
   // Append item's name too
   REFPROPERTYKEY pkey = bNeedsParsingName ? PKEY_ParsingName : PKEY_ItemNameDisplay;
   CComPropVariant v;
   HR( spItem->GetProperty(pkey, v) );
   ATLASSERT(v.vt==VT_LPWSTR);
   wcscat_s(wszName, lengthof(wszName), v.pwszVal);
   return StrToSTRRET(wszName, psrName);
}

STDMETHODIMP CShellFolder::SetNameOf(HWND hwnd, PCUITEMID_CHILD pidl, LPCWSTR pszName, SHGDNF uFlags, PITEMID_CHILD* ppidlOut)
{
   ATLTRACE(L"CShellFolder::SetNameOf  flags=0x%X [SHGDN_%s]\n", uFlags, DbgGetSHGDNF(uFlags));
   if( pszName == NULL ) return E_INVALIDARG;
   if( wcslen(pszName) == 0 ) return AtlHresultFromWin32(ERROR_FILENAME_EXCED_RANGE);
   // Do the rename of the child item
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   TCHAR wszOutputName[MAX_PATH] = { 0 };
   wcscpy_s(wszOutputName, lengthof(wszOutputName), pszName);
   HR( spItem->Rename(pszName, wszOutputName) );
   // Lookup the new item and return it as the result
   if( ppidlOut != NULL ) {
      CNseItemPtr spNewItem;
      HRESULT hr = ( m_spFolderItem->GetChild(wszOutputName, SHGDN_FORPARSING, &spNewItem) );
      if (hr != S_OK) return hr;
      *ppidlOut = ::ILCloneChild( spNewItem->GetITEMID() );
      // Notify Shell directly about the rename operation...
      CPidl pidlOld = m_pidlMonitor + pidl;
      CPidl pidlNew = m_pidlMonitor + spNewItem->GetITEMID();
      ::SHChangeNotify(spNewItem->IsFolder() ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM, SHCNF_IDLIST | SHCNF_FLUSH, pidlOld, pidlNew);
   }
   ::SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, m_pidlMonitor);
   return S_OK;
}

// IShellFolder2

STDMETHODIMP CShellFolder::GetDefaultSearchGUID(GUID* pguid)
{
   ATLTRACENOTIMPL(L"CShellFolder::GetDefaultSearchGUID\n");
}

STDMETHODIMP CShellFolder::EnumSearches(IEnumExtraSearch** ppenum)
{
   ATLTRACENOTIMPL(L"CShellFolder::EnumSearches\n");
}

STDMETHODIMP CShellFolder::GetDefaultColumn(DWORD /*dwRes*/, ULONG* pSort, ULONG* pDisplay)
{
   ATLTRACE(L"CShellFolder::GetDefaultColumn\n");
   const VFS_FOLDERSETTINGS Settings = m_spFolderItem->GetFolderSettings();
   if( pSort != NULL ) *pSort = Settings.uDefaultSortColumn;
   if( pDisplay != NULL ) *pDisplay = Settings.uDefaultDisplayColumn;
   return S_OK;
}

STDMETHODIMP CShellFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF* pcsFlags)
{
   ATLTRACE(L"CShellFolder::GetDefaultColumnState  column=%u\n", iColumn);
   VFS_COLUMNINFO Column = { 0 };
   HR( m_spFolderItem->GetColumnInfo(iColumn, Column) );
   *pcsFlags = Column.dwFlags;
   return S_OK;
}

STDMETHODIMP CShellFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
   ATLTRACE(L"CShellFolder::GetDetailsEx  scid=%s\n", DbgGetPKEY(*pscid));
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   CComPropVariant var;
   if( SUCCEEDED( spItem->GetProperty(*pscid, var) ) ) return var.CopyTo(pv);
   ATLTRACE(L"CShellFolder::GetDetailsEx - failed\n");
   return E_FAIL;
}

STDMETHODIMP CShellFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS* psd)
{
   //OUTPUTLOG("%s(), Column: %d", __FUNCTION__, iColumn);
   ATLTRACE(L"CShellFolder::GetDetailsOf  column=%u\n", iColumn);
   ATLASSERT(psd);
   SHCOLUMNID scid = { 0 };
   HRESULT Hr = MapColumnToSCID(iColumn, &scid);
   if( FAILED(Hr) ) return Hr;
   // If it's for the column, return column information
   if( pidl == NULL ) return _GetColumnDetailsOf(&scid, psd);
   // Return member information for this item...
   CComVariant v;
   HR( GetDetailsEx(pidl, &scid, &v) );
   HR( v.ChangeType(VT_BSTR) );
   return StrToSTRRET(v.bstrVal, &psd->str);
}

STDMETHODIMP CShellFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
{
   ATLTRACE(L"CShellFolder::MapColumnToSCID  column=%u\n", iColumn);
   VFS_COLUMNINFO Info = { 0 };
   HRESULT Hr = m_spFolderItem->GetColumnInfo(iColumn, Info);
   if( FAILED(Hr) ) return Hr;
   // HarryWu, 2014.3.18
   // if not column, notify the caller to stop calling next time.
   if( IsBitSet(Info.dwAttributes, VFS_COLF_NOTCOLUMN) ) return E_FAIL;
   *pscid = Info.pkey;
   return S_OK;
}       

// IShellIcon

STDMETHODIMP CShellFolder::GetIconOf(PCUITEMID_CHILD pidl, UINT uIconFlags, int* pIconIndex)
{
   ATLTRACE(L"CShellFolder::GetIconOf\n");
   if( !_ShellModule.GetConfigBool(VFS_HAVE_SYSICONS) ) return S_FALSE;
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return S_FALSE;
   return spItem->GetSysIcon(uIconFlags, pIconIndex);
}

// IShellIconOverlay

STDMETHODIMP CShellFolder::GetOverlayIndex(PCUITEMID_CHILD pidl, int* pIndex)
{
   ATLTRACE(L"CShellFolder::GetOverlayIndex\n");
   if( !_ShellModule.GetConfigBool(VFS_HAVE_ICONOVERLAYS) ) return S_FALSE;
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return S_FALSE;
   return spItem->GetIconOverlay(pIndex);
}

STDMETHODIMP CShellFolder::GetOverlayIconIndex(PCUITEMID_CHILD pidl, int* pIconIndex)
{
   ATLTRACE(L"CShellFolder::GetOverlayIconIndex\n");
   if( !_ShellModule.GetConfigBool(VFS_HAVE_ICONOVERLAYS) ) return S_FALSE;
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return S_FALSE;
   int iIndex = 0;
   HRESULT Hr = spItem->GetIconOverlay(&iIndex);
   if( Hr == S_OK ) *pIconIndex = INDEXTOOVERLAYMASK(iIndex);
   return Hr;
}

// IFolderViewSettings

STDMETHODIMP CShellFolder::GetColumnPropertyList(REFIID riid, LPVOID* ppv)
{
   ATLTRACE(L"CShellFolder::GetColumnPropertyList\n");
   return E_NOTIMPL;
}

STDMETHODIMP CShellFolder::GetGroupByProperty(PROPERTYKEY* pkey, BOOL* pfGroupAscending)
{
   ATLTRACE(L"CShellFolder::GetGroupByProperty\n");
   const VFS_FOLDERSETTINGS Settings = m_spFolderItem->GetFolderSettings();
   if( Settings.GroupByKey == PKEY_Null ) return E_NOTIMPL;
   *pkey = Settings.GroupByKey;
   *pfGroupAscending = Settings.bGroupByAsc;
   return S_OK;
}

STDMETHODIMP CShellFolder::GetViewMode(FOLDERLOGICALVIEWMODE* plvm)
{
   ATLTRACE(L"CShellFolder::GetViewMode\n");
   const VFS_FOLDERSETTINGS Settings = m_spFolderItem->GetFolderSettings();
   if( Settings.ViewMode == 0 ) return E_NOTIMPL;
   *plvm = Settings.ViewMode;
   return S_OK;
}

STDMETHODIMP CShellFolder::GetIconSize(UINT* puIconSize)
{
   ATLTRACE(L"CShellFolder::GetIconSize\n");
   const VFS_FOLDERSETTINGS Settings = m_spFolderItem->GetFolderSettings();
   if( Settings.cxyIcon == 0 ) return E_NOTIMPL;
   *puIconSize = Settings.cxyIcon;
   return S_OK;
}

STDMETHODIMP CShellFolder::GetFolderFlags(FOLDERFLAGS* pfolderMask, FOLDERFLAGS* pfolderFlags)
{
   ATLTRACE(L"CShellFolder::GetFolderFlags\n");
   const VFS_FOLDERSETTINGS Settings = m_spFolderItem->GetFolderSettings();
   if( Settings.FlagsMask == 0 ) return E_NOTIMPL;
   *pfolderMask = Settings.FlagsMask;
   *pfolderFlags = Settings.FlagsValue;
   return S_OK;
}

STDMETHODIMP CShellFolder::GetSortColumns(SORTCOLUMN* rgSortColumns, UINT cColumnsIn, UINT* pcColumnsOut)
{
   ATLTRACE(L"CShellFolder::GetSortColumns\n");
   return E_NOTIMPL;
}

STDMETHODIMP CShellFolder::GetGroupSubsetCount(UINT* pcVisibleRows)
{
   ATLTRACE(L"CShellFolder::GetGroupSubsetCount\n");
   const VFS_FOLDERSETTINGS Settings = m_spFolderItem->GetFolderSettings();
   if( Settings.nGroupVisible == 0 ) return E_NOTIMPL;
   *pcVisibleRows = Settings.nGroupVisible;
   return S_OK;
}

// IExplorerPaneVisibility

STDMETHODIMP CShellFolder::GetPaneState(REFEXPLORERPANE ep, EXPLORERPANESTATE* peps)
{
   ATLTRACE(L"CShellFolder::GetPaneState  ep=%s\n", DbgGetIID(ep));
   static struct { 
      REFEXPLORERPANE ep; 
      VFS_CONFIG config; 
      EXPLORERPANESTATE peps;
   } aList[] = {
      { EP_NavPane,     VFS_HIDE_NAVTREE_PANEL, EPS_DEFAULT_OFF | EPS_INITIALSTATE | EPS_FORCE },
      { EP_DetailsPane, VFS_HIDE_DETAILS_PANEL, EPS_DEFAULT_OFF | EPS_INITIALSTATE | EPS_FORCE },
      { EP_PreviewPane, VFS_HIDE_PREVIEW_PANEL, EPS_DEFAULT_OFF | EPS_FORCE },
   };
   *peps = EPS_DONTCARE;
   for( int i = 0; i < lengthof(aList); i++ ) {
      if( ep == aList[i].ep && _ShellModule.GetConfigBool(aList[i].config) ) *peps = aList[i].peps;
   }
   return m_spFolderItem->GetPaneState(ep, peps);
}

// IThumbnailHandlerFactory

STDMETHODIMP CShellFolder::GetThumbnailHandler(PCUITEMID_CHILD pidlItem, IBindCtx *pbc, REFIID riid, LPVOID* ppv)
{
   ATLTRACE(L"CShellFolder::GetThumbnailHandler  riid=%s\n", DbgGetIID(riid));
   CNseItemPtr spItem = GenerateChildItem(pidlItem);
   if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   return spItem->GetThumbnail(riid, ppv);
}

// IStorage

STDMETHODIMP CShellFolder::CreateStream(LPCWSTR pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream** ppstm)
{
   ATLTRACENOTIMPL(L"CShellFolder::CreateStream");
}

STDMETHODIMP CShellFolder::OpenStream(LPCWSTR pwcsName, LPVOID reserved1, DWORD grfMode, DWORD reserved2, IStream** ppstm)
{
   ATLTRACE(L"CShellFolder::OpenStream  name=%ls\n", pwcsName);
   CNseItemPtr spItem;
   HR( m_spFolderItem->GetChild(pwcsName, SHGDN_FORPARSING, &spItem) );
   CComObject<CFileStream>* pStream = NULL;
   HR( CComObject<CFileStream>::CreateInstance(&pStream) );
   CComPtr<IUnknown> spKeepAlive = pStream->GetUnknown();
   HR( pStream->Init(this, spItem->GetITEMID()) );
   return pStream->QueryInterface(IID_PPV_ARGS(ppstm));
}

STDMETHODIMP CShellFolder::CreateStorage(LPCWSTR pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage** ppstg)
{
   ATLTRACENOTIMPL(L"CShellFolder::CreateStorage");
}

STDMETHODIMP CShellFolder::OpenStorage(LPCWSTR pwcsName, IStorage* pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage** ppstg)
{
   ATLTRACENOTIMPL(L"CShellFolder::OpenStorage");
}

STDMETHODIMP CShellFolder::CopyTo(DWORD ciidExclude, const IID* rgiidExclude, SNB snbExclude, IStorage* pstgDest)
{
   ATLTRACENOTIMPL(L"CShellFolder::CopyTo");
}

STDMETHODIMP CShellFolder::MoveElementTo(LPCWSTR pwcsName, IStorage* pstgDest, LPCWSTR pwcsNewName, DWORD grfFlags)
{
   ATLTRACENOTIMPL(L"CShellFolder::MoveElementTo");
}

STDMETHODIMP CShellFolder::Commit(DWORD grfCommitFlags)
{
   ATLTRACE(L"CShellFolder::Commit\n");
   return S_OK;
}

STDMETHODIMP CShellFolder::Revert()
{
   ATLTRACENOTIMPL(L"CShellFolder::Revert");
}

STDMETHODIMP CShellFolder::EnumElements(DWORD reserved1, LPVOID reserved2, DWORD reserved3, IEnumSTATSTG** ppenum)
{
   ATLTRACE(L"CShellFolder::EnumElements\n");
   *ppenum = NULL;
   if( !_ShellModule.GetConfigBool(VFS_HAVE_VIRTUAL_FILES) ) return E_NOTIMPL;
   CNseItemArray aChildren;
   HR( m_spFolderItem->EnumChildren(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_STORAGE, aChildren) );
   CSimpleArray<STATSTG> aList;
   for( int i = 0; i < aChildren.GetSize(); i++ ) {
      const VFS_FIND_DATA wfd = aChildren[i]->GetFindData();
      STATSTG statstg = { 0 };
      statstg.pwcsName = ::StrDup(wfd.cFileName);
      statstg.cbSize.LowPart = wfd.nFileSizeLow;
      statstg.cbSize.HighPart = wfd.nFileSizeHigh;
      statstg.ctime = wfd.ftCreationTime;
      statstg.mtime = wfd.ftLastWriteTime;
      statstg.atime = wfd.ftLastAccessTime;
	  // HarryWu, 2014.2.10
	  // How to setup folder's type.
      statstg.type = STGTY_STREAM;
      aList.Add(statstg);
   }
   typedef CComEnum< IEnumSTATSTG, &IID_IEnumSTATSTG, STATSTG, _Copy<STATSTG> > CEnumSTATSTG;
   CComObject<CEnumSTATSTG>* pEnumSTATSTG = NULL;
   HR( CComObject<CEnumSTATSTG>::CreateInstance(&pEnumSTATSTG) );
   HR( pEnumSTATSTG->Init(aList.GetData(), aList.GetData() + aList.GetSize(), NULL, AtlFlagCopy) );
   for( int i = 0; i < aList.GetSize(); i++ ) ::CoTaskMemFree(aList[i].pwcsName);
   return pEnumSTATSTG->QueryInterface(IID_PPV_ARGS(ppenum));
}

STDMETHODIMP CShellFolder::DestroyElement(LPCWSTR pwcsName)
{
   ATLTRACE(L"CShellFolder::DestroyElement  name=%ls\n", pwcsName);
   CNseItemPtr spItem;
   HR( m_spFolderItem->GetChild(pwcsName, SHGDN_FORPARSING, &spItem) );
   HR( spItem->Delete() );
   return S_OK;
}

STDMETHODIMP CShellFolder::RenameElement(LPCWSTR pwcsOldName, LPCWSTR pwcsNewName)
{
   ATLTRACE(L"CShellFolder::RenameElement  name=%ls\n", pwcsOldName);
   CNseItemPtr spItem;
   HR( m_spFolderItem->GetChild(pwcsOldName, SHGDN_FORPARSING, &spItem) );
   WCHAR wszOutputName[MAX_PATH] = { 0 };
   wcscpy_s(wszOutputName, lengthof(wszOutputName), pwcsNewName);
   HR( spItem->Rename(pwcsNewName, wszOutputName) );
   return S_OK;
}

STDMETHODIMP CShellFolder::SetElementTimes(LPCWSTR pwcsName, const FILETIME* pctime, const FILETIME* patime, const FILETIME* pmtime)
{
   ATLTRACENOTIMPL(L"CShellFolder::SetElementTimes");
}

STDMETHODIMP CShellFolder::SetClass(REFCLSID clsid)
{
   ATLTRACENOTIMPL(L"CShellFolder::SetClass");
}

STDMETHODIMP CShellFolder::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
   ATLTRACENOTIMPL(L"CShellFolder::SetStateBits");
}

STDMETHODIMP CShellFolder::Stat(STATSTG* pStatstg, DWORD grfStatFlag)
{
   ATLTRACE(L"CShellFolder::Stat\n");
   const VFS_FIND_DATA wfd = m_spFolderItem->GetFindData();
   ::ZeroMemory(pStatstg, sizeof(STATSTG));
   if( !IsBitSet(grfStatFlag, STATFLAG_NONAME) ) pStatstg->pwcsName = ::StrDup(wfd.cFileName);
   pStatstg->grfMode = STGM_READWRITE;
   pStatstg->type = STGTY_STORAGE;
   return S_OK;
}

// IItemNameLimits

STDMETHODIMP CShellFolder::GetValidCharacters(LPWSTR* ppwszValidChars, LPWSTR* ppwszInvalidChars)
{
   ATLTRACE(L"CShellFolder::GetValidCharacters\n");
   CCoTaskString strAllowed = _ShellModule.GetConfigStr(VFS_STR_FILENAME_CHARS_ALLOWED);
   CCoTaskString strNotAllowed = _ShellModule.GetConfigStr(VFS_STR_FILENAME_CHARS_NOTALLOWED);
   if( ppwszValidChars != NULL ) *ppwszValidChars = strAllowed.Detach();
   if( ppwszInvalidChars != NULL ) *ppwszInvalidChars = strNotAllowed.Detach();
   return S_OK;
}

STDMETHODIMP CShellFolder::GetMaxLength(LPCWSTR pszName, int* piMaxNameLen)
{
   ATLTRACE(L"CShellFolder::GetMaxLength  name='%ls'\n", pszName);
   bool bHasPath = (wcschr(pszName, '\\') != NULL);
   int cchMax = _ShellModule.GetConfigInt(bHasPath ? VFS_INT_MAX_PATHNAME_LENGTH : VFS_INT_MAX_FILENAME_LENGTH);
   if( cchMax <= 0 ) cchMax = MAX_PATH;
   ATLASSERT(cchMax<=MAX_PATH);  // Our framework sets this limit!
   int cchName = pszName != NULL ? (int) ::lstrlenW(pszName) : 0;
   *piMaxNameLen = cchMax > cchName ? cchMax - cchName : 0;
   return S_OK;
}

// IObjectProvider

STDMETHODIMP CShellFolder::QueryObject(REFGUID guidObject, REFIID riid, LPVOID* ppv)
{
   ATLTRACE(L"CShellFolder::QueryObject  obj=%s riid=%s\n", DbgGetIID(guidObject), DbgGetIID(riid));
   ATLTRACE(L"CShellFolder::QueryObject - failed\n");
   *ppv = NULL;
   return E_NOTIMPL;
}

// IContextMenuCB

STDMETHODIMP CShellFolder::CallBack(IShellFolder* psf, HWND hwndOwner, IDataObject* pDataObject, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   ATLTRACE(L"CShellFolder::MenuCallback  msg=%ld wp=0x%X lp=0x%X\n", uMsg, wParam, lParam);
   HRESULT Hr = E_NOTIMPL;
   switch( uMsg ) {
   case DFM_INVOKECOMMANDEX:
      {
         const DFMICS* pDFMICS = reinterpret_cast<DFMICS*>(lParam);
         ATLASSERT(pDFMICS->cbSize==sizeof(DFMICS));
         CComPtr<IShellItemArray> spItems;
         ::SHCreateShellItemArrayFromDataObject(pDataObject, IID_PPV_ARGS(&spItems));
         VFS_MENUCOMMAND Cmd = { hwndOwner, (UINT) wParam, HIWORD(pDFMICS->pici->lpVerb) != 0 ? pDFMICS->pici->lpVerb : VFS_MNUCMD_NOVERB, DROPEFFECT_COPY, NULL, spItems, NULL, pDFMICS->punkSite, NULL };
         return ExecuteMenuCommand(Cmd);
      }
   case DFM_MERGECONTEXTMENU:
      {
         QCMINFO* pqcmi = reinterpret_cast<QCMINFO*>(lParam);
         if( !::IsMenu(m_hContextMenu) ) return S_OK;
		 if (!DMHttpTransferIsEnable()){
			_RefineShellMenuItems(pqcmi->hmenu, pDataObject);
		 }
         _SetMenuState(m_hContextMenu, pDataObject);
         UINT uCmdFirst = pqcmi->idCmdFirst;
         pqcmi->idCmdFirst = ::Shell_MergeMenus(pqcmi->hmenu, m_hContextMenu, pqcmi->indexMenu, pqcmi->idCmdFirst, pqcmi->idCmdLast, 0);
         if( !IsBitSet(wParam, CMF_NODEFAULT) ) ::SetMenuDefaultItem(pqcmi->hmenu, uCmdFirst + ::GetMenuItemID(m_hContextMenu, 0), MF_BYCOMMAND);
         return S_OK;
      }
   case DFM_GETVERBA:
   case DFM_GETVERBW:
      {
         CResString<300> sCmd(LOWORD(wParam));
         if( sCmd.IsEmpty() ) return S_FALSE;
         LPCTSTR pstrSep = _tcschr(sCmd, '\n');
         if( pstrSep == NULL ) return S_FALSE;
         if( uMsg == DFM_GETVERBA ) return strncpy(reinterpret_cast<LPSTR>(lParam), CT2CA(pstrSep + 1), HIWORD(wParam)) > 0 ? S_OK : E_FAIL;
         else return wcsncpy(reinterpret_cast<LPWSTR>(lParam), CT2CW(pstrSep + 1), HIWORD(wParam)) > 0 ? S_OK : E_FAIL;
      }
   case DFM_GETHELPTEXT:
   case DFM_GETHELPTEXTW:
      {
         CResString<300> sCmd((UINT) wParam);
         if( sCmd.IsEmpty() ) return S_FALSE;
         LPTSTR pstrSep = const_cast<LPTSTR>(_tcschr(sCmd, '\n'));
         if( pstrSep != NULL ) *pstrSep = '\0';
         if( uMsg == DFM_GETHELPTEXT ) return strncpy(reinterpret_cast<LPSTR>(lParam), CT2CA(sCmd), HIWORD(wParam)) > 0 ? S_OK : E_FAIL;
         else return wcsncpy(reinterpret_cast<LPWSTR>(lParam), CT2CW(sCmd), HIWORD(wParam)) > 0 ? S_OK : E_FAIL;
      }
   case DFM_MERGECONTEXTMENU_TOP:
   case DFM_MERGECONTEXTMENU_BOTTOM:
      return S_OK;
   case DFM_GETDEFSTATICID:
   case DFM_MAPCOMMANDNAME:
   case DFM_VALIDATECMD:
      Hr = S_FALSE;
      break;
   }
   return Hr;
}

// TODO: R/W protected in multi threads.
static CRITICAL_SECTION * s_lock = NULL;
static std::map<HWND, CShellFolder *> s_ShellViewObjects;
static CShellFolder * GetShellFolder(HWND hWnd)
{
    EnterCriticalSection(s_lock);
    if (s_ShellViewObjects.find(hWnd) == s_ShellViewObjects.end()){
        LeaveCriticalSection(s_lock);
        return NULL;
    }
    CShellFolder * pFolder = s_ShellViewObjects.find(hWnd)->second;
    LeaveCriticalSection(s_lock);  
    return pFolder;
}
static void AddShellFolder(HWND hWnd, CShellFolder * pFolder)
{
    EnterCriticalSection(s_lock);
    pFolder->AddRef(); s_ShellViewObjects.insert(std::make_pair(hWnd, pFolder));
    LeaveCriticalSection(s_lock);
}
static void RemoveShellFolder(HWND hWnd)
{
    EnterCriticalSection(s_lock);
    if (s_ShellViewObjects.find(hWnd) == s_ShellViewObjects.end()){
        LeaveCriticalSection(s_lock);
        return ;
    }
    CShellFolder * pFolder = s_ShellViewObjects.find(hWnd)->second;
    s_ShellViewObjects.erase(hWnd); pFolder->Release();
    LeaveCriticalSection(s_lock);  
}

// HarryWu, 2014.4.17
// sub class window procedure.
static WNDPROC s_OldShellViewWndProc = NULL;
static LRESULT CALLBACK s_ShellViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!s_OldShellViewWndProc) return S_OK;
    COPYDATASTRUCT * pCDS = (COPYDATASTRUCT *)lParam;
    if ( uMsg != WM_COPYDATA || !pCDS || pCDS->dwData != 0xED0CED0C){
        return CallWindowProcA(s_OldShellViewWndProc, hWnd, uMsg, wParam, lParam);
    }

    VFS_MENUCOMMAND cmd; memset(&cmd, 0, sizeof(cmd));

    if (!strnicmp((const char *)pCDS->lpData, "PrevPage", 8))
        cmd.wMenuID = ID_FILE_PREV;
    else if (!strnicmp((const char *)pCDS->lpData, "NextPage", 8))
        cmd.wMenuID = ID_FILE_NEXT;
	else if (!strnicmp((const char *)pCDS->lpData, "GotoPage://", 11)){
		cmd.wMenuID = ID_FILE_GOTO;
		cmd.pUserData = malloc(sizeof(DWORD));
		// assume that pCDS->lpData is null-terminated string.
		*(DWORD *)cmd.pUserData = atoi((const char *)pCDS->lpData + 11);
	}
    else if (!strnicmp((const char *)pCDS->lpData, "Search://", 9))
    {
        cmd.wMenuID = ID_FILE_SEARCH;
        cmd.pUserData = malloc(0x200);
        //TODO: Bounds check of pCDS->lpData
		// assume that pCDS->lpData is null-terminated string.
        strcpy_s((char *)cmd.pUserData, 0x200, (const char *)(pCDS->lpData) + 9);
    }else{
        cmd.wMenuID = 0;
        OUTPUTLOG("%s() undefined message.", __FUNCTION__);
    }

    CShellFolder * pFolder = GetShellFolder(hWnd);
    if (cmd.wMenuID && pFolder) pFolder->ExecuteMenuCommand(cmd);
    return S_OK;
}

// IShellFolderViewCB messages
LRESULT CShellFolder::OnWindowCreated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hWnd = (HWND)wParam;

    m_hShellDefView = hWnd;

	// Tag this window.
	SetWindowLongPtr(hWnd, GWLP_USERDATA, 0xED0CED0C);

	// HarryWu, 2014.2.12
	// To dock a window pane on DefView, 
	// Set the following style & Ex Style
	// 	   , WS_CHILDWINDOW 
	//     | WS_CLIPSIBLINGS 
    //     | WS_VISIBLE
	//     | WS_BORDER | WS_THICKFRAME
	//     | WS_CLIPCHILDREN
	//     | WS_EX_LEFT
	//     | WS_EX_LTRREADING
	//     | WS_EX_RIGHTSCROLLBAR
	// And Also setup Z-order to HWND_TOP
	//     SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	// ...
	// And Process WM_SIZE message.
	//
	// Spy++ confuse GetParent() and GetAncestor(GA_PARENT)

    m_spFolderItem->InitCustomColumns();

    m_spFolderItem->OnShellViewCreated(m_hwndOwner, hWnd, m_spFolderItem->GetFindData().dwId.category, m_spFolderItem->GetFindData().dwId.id);

    if (s_lock == NULL) {
        s_lock = new CRITICAL_SECTION; InitializeCriticalSection(s_lock);
    }

    if ((WNDPROC)GetWindowLongPtrA(hWnd, GWLP_WNDPROC) != s_ShellViewWndProc){
        s_OldShellViewWndProc = (WNDPROC)SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)s_ShellViewWndProc);
    }

    AddShellFolder(hWnd, this);

    return S_OK;
}

LRESULT CShellFolder::OnWindowClosing(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hWnd = (HWND)wParam;
    m_spFolderItem->OnShellViewClosing(m_hwndOwner, m_hShellDefView);
    RemoveShellFolder(m_hShellDefView);
    return S_OK;
}

LRESULT CShellFolder::OnColumnClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return S_OK;
}

LRESULT CShellFolder::OnBackGroudEnumDone(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	PROPERTYKEY pKey = m_pSortKey;
	SORTDIRECTION iDirection = m_iSortDirection;
	HR( _GetCurrentSortColumn(&pKey, &iDirection));
	if (pKey == m_pSortKey && iDirection == m_iSortDirection){
        OUTPUTLOG("%s(), key=%s, direction=%d, no sort", __FUNCTION__, GetPropertyKeyNameA(pKey).c_str(), iDirection);
        return S_OK;
    }

	OUTPUTLOG("%s(), SortColumn=%s, direction=%d", __FUNCTION__, GetPropertyKeyNameA(pKey).c_str(), iDirection);
	if (m_pSortKey == PKEY_Null){
		m_pSortKey = pKey;
		m_iSortDirection = iDirection;
		return S_OK;
	}

	m_pSortKey = pKey;
	m_iSortDirection = iDirection;
    m_spFolderItem->Resort(m_hwndOwner, m_hShellDefView,  GetPropertyKeyNameW(pKey).c_str(), iDirection);
    return S_OK;
}

LRESULT CShellFolder::OnGetNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // Tell the Shell we want to receive Change Notifications for our folder.
   // Note the nasty non-owned referenced IDLIST to the Shell here!
   * reinterpret_cast<PIDLIST_ABSOLUTE*>(wParam) = m_pidlMonitor;
   * reinterpret_cast<LONG*>(lParam) = SHCNE_DISKEVENTS;
   return S_OK;
}

LRESULT CShellFolder::OnListRefreshed(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // This is an undocumented feature, but it appears that when the user
   // forces a refresh (ie. through F5) then the wParam is non-zero.
   OUTPUTLOG("%s() wParam=0x%08x", __FUNCTION__, wParam);
   m_spFolderItem->OnShellViewRefreshed(m_hwndOwner, m_hShellDefView);
   return 0;
}

LRESULT CShellFolder::OnSelectionChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	struct SFVCB_SELECTINFO{
		UINT uOldState; // 0
		UINT uNewState; //LVIS_SELECTED, LVIS_FOCUSED,...
		LPITEMIDLIST pidl;
	};
	SFVCB_SELECTINFO * pSelectInfo = (SFVCB_SELECTINFO *)lParam;

    // HarryWu, 2014.5.13
    // Ok to get the selected object.
    PIDLIST_ABSOLUTE pidlSelected = NULL;
    if (m_pShellView){
        IDataObject * pDataObject = NULL;
        m_pShellView->GetItemObject(SVGIO_SELECTION, IID_IDataObject, (void **)&pDataObject);
        if (pDataObject){
            IShellItemArray * pItemArray = NULL;
            SHCreateShellItemArrayFromDataObject(pDataObject, IID_IShellItemArray, (void **)&pItemArray);
            if (pItemArray){
                DWORD count = 0;
                pItemArray->GetCount(&count);
                count = count;
                //OUTPUTLOG("Selected [%d] items", count);
                std::wstring sSelectedItemIds;
                for (size_t i = 0; i < count; i ++){
                    IShellItem * pShellItem = NULL;
                    pItemArray->GetItemAt(i, &pShellItem);
                    if (pShellItem){
                        PIDLIST_ABSOLUTE pidlAbs = NULL;
                        SHGetIDListFromObject(pShellItem, &pidlAbs);
                        CPidl oPidlAbs = pidlAbs;
                        int itemCount = oPidlAbs.GetItemCount();
                        //OUTPUTLOG("IDList Count [%d]\n", itemCount);
                        PUITEMID_CHILD childPidl = oPidlAbs.GetLastItem();
                        if (childPidl){
                            NSEFILEPIDLDATA * pNSEInfo = (NSEFILEPIDLDATA *)childPidl;
                            wchar_t buf [100] = _T("");
							wsprintf(buf, _T("%d:%d,"), pNSEInfo->wfd.dwId.id, (pNSEInfo->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0);
                            sSelectedItemIds += buf;
                        }
                        pShellItem->Release();
                    }
                }
                m_spFolderItem->SelectItems(m_hwndOwner, m_hShellDefView, sSelectedItemIds.c_str());
                pItemArray->Release();
            }
            pDataObject->Release();
        }else{
            m_spFolderItem->SelectItems(m_hwndOwner, m_hShellDefView, NULL);
        }
    }
	return 0;
}

LRESULT CShellFolder::OnUpdateObject(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CShellFolder::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_spFolderItem->OnShellViewSized(m_hwndOwner, m_hShellDefView);
	return S_FALSE;
}

// Operations

/**
 * Bind to a child folder.
 * Helper function that binds to a folder item in the children list of the current folder.
 */
HRESULT CShellFolder::BindToFolder(PCUITEMID_CHILD pidl, CRefPtr<CShellFolder>& spFolder)
{
   ATLASSERT(::ILIsChild(pidl));
   // Ensure this child is browsable...
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) {
      ATLTRACE(L"CShellFolder::BindToFolder - failed (not found)\n");
      return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   }
   if( !spItem->IsFolder() ) {
      ATLTRACE(L"CShellFolder::BindToFolder - failed (SFGAO_FOLDER)\n");
      return E_INVALIDARG;
   }
   VFS_FIND_DATA wfd = spItem->GetFindData();
   if ( wfd.dwId.category == RecycleCat && wfd.dwId.id != RecycleId || wfd.dwId.category == SearchCat && wfd.dwId.id != SearchId)
   {
       ATLTRACE(L"CShellFolder::BindToFolder - failed (Invalid Category)\n");
       return E_INVALIDARG;
   }
   // Initialize new folder object
   CComObject<CShellFolder>* pFolder = NULL;
   HR( CComObject<CShellFolder>::CreateInstance(&pFolder) );
   CComPtr<IUnknown> spKeepAlive = pFolder->GetUnknown();
   pFolder->m_spFS = m_spFS;
   pFolder->m_pidlRoot = m_pidlRoot;
   pFolder->m_pidlParent = m_pidlFolder;
   pFolder->m_pidlFolder = m_pidlFolder + pidl;
   pFolder->m_pidlMonitor = CPidl(m_pidlRoot, m_pidlFolder, pidl);
   // Finally create a NSE Item for the folder.
   // NOTE: The delicate choice of arguments here is because we depend on the memory
   //       to be scoped by the new instance.
   pFolder->m_spFolderItem = m_spFolderItem->GenerateChild(pFolder, pFolder->m_pidlParent, ILCloneChild(pidl), TRUE);
   ATLASSERT(pFolder->m_spFolderItem);
   if( pFolder->m_spFolderItem == NULL ) return E_UNEXPECTED;
   spFolder = pFolder;
   return S_OK;
}

/**
 * Set a property on a child item.
 * Helper function to set a property on a child item.
 */
HRESULT CShellFolder::SetItemProperty(PCUITEMID_CHILD pidl, REFPROPERTYKEY pkey, PROPVARIANT* pv)
{
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   return spItem->SetProperty(pkey, *pv);
}

/**
 * Get a property from a child item.
 * Helper function to read a property from a child item.
 * When asking an item for multiple properties, use the GetItemProperties() method instead.
 */
HRESULT CShellFolder::GetItemProperty(PCUITEMID_CHILD pidl, REFPROPERTYKEY pkey, PROPVARIANT* pv)
{
   ATLASSERT(pv->vt==VT_EMPTY);
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   CComPropVariant var;
   HRESULT Hr = spItem->GetProperty(pkey, var);
   if( FAILED(Hr) ) return Hr;
   return var.Detach(pv);
}

/**
 * Get the property state for a property of a child item.
 */
HRESULT CShellFolder::GetItemPropertyState(PCUITEMID_CHILD pidl, REFPROPERTYKEY pkey, VFS_PROPSTATE& State)
{
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   State = spItem->GetPropertyState(pkey);
   return S_OK;
}

/**
 * Get one or more properties from a child item.
 * Helper function to read a bundle of properties quickly.
 * If a property does not exist, it is returned as VT_EMPTY.
 */
HRESULT CShellFolder::GetItemProperties(PCUITEMID_CHILD pidl, UINT cKeys, const PROPERTYKEY* pkey, PROPVARIANT* pv)
{
   CNseItemPtr spItem = GenerateChildItem(pidl);
   if( spItem == NULL ) return AtlHresultFromWin32(ERROR_FILE_NOT_FOUND);
   for( UINT i = 0; i < cKeys; i++ ) {
      CComPropVariant var;
      spItem->GetProperty(pkey[i], var);
      var.Detach(pv + i);
   }
   return S_OK;
}

/**
 * Create a NSE Item.
 * Helper function which returns a child item of this folder. This method blindly
 * creates the child PIDL as a member of this folder, it does not validate if it
 * is actually an existing item under it.
 */
CNseItem* CShellFolder::GenerateChildItem(PCUITEMID_CHILD pidlItem, BOOL bReleaseItem /*= FALSE*/)
{
   ATLASSERT(pidlItem);
   ATLASSERT(m_spFolderItem);
   //ATLASSERT(::ILIsChild(pidlItem));   // Optimized away because of CShellFolder::CompareIDs
   if( pidlItem == NULL ) return NULL;
   if( m_spFolderItem == NULL ) return NULL;
#if defined(_M_X64) || defined(_M_IA64)
   if( !bReleaseItem && !::ILIsAligned(pidlItem) ) {
      pidlItem = ILCloneChild(pidlItem);
      bReleaseItem = TRUE;
   }
   ATLASSERT(ILIsAligned(pidlItem));
   return m_spFolderItem->GenerateChild(this, m_pidlFolder, static_cast<PCITEMID_CHILD>(pidlItem), bReleaseItem);
#else
   return m_spFolderItem->GenerateChild(this, m_pidlFolder, pidlItem, bReleaseItem);
#endif  // _M_IX86
}

/**
 * Create a NSE Item from a IShellItem.
 * Even if the argument is a IShellItem, this method still assumes that the
 * item is actually a child of this folder.
 */
CNseItem* CShellFolder::GenerateChildItemFromShellItem(IShellItem* pShellItem)
{
   ATLASSERT(pShellItem);
   ATLASSERT(m_spFolderItem);
   CPidl pidlFull;
   if( FAILED( pidlFull.CreateFromObject(pShellItem) ) ) return NULL;
   ATLASSERT(m_pidlMonitor.IsParent(pidlFull, TRUE));
   ATLASSERT(m_pidlMonitor.GetItemCount()==pidlFull.GetItemCount()-1);
   if( !m_pidlMonitor.IsParent(pidlFull, TRUE) ) return NULL;
   return m_spFolderItem->GenerateChild(this, m_pidlFolder, ::ILCloneChild( pidlFull.GetLastItem() ), TRUE);
}

/**
 * Execute File-Operation commands on separate thread.
 * This solves a problem with any slow file-copy that would block since the Shell often uses the
 * UI thread to perform any menu-command execution.
 */
static DWORD WINAPI FileOperationThread(LPVOID pData)
{
   // BUG: This violates fundamental COM rules by moving an interface to a different apartment. We detached
   //      ownership to the FileOperation component, so we cross our fingers. Let's hope the FO component
   //      doesn't have any real thread-affinity (ie. uses TLS).
   IFileOperation* pFO = reinterpret_cast<IFileOperation*>(pData);
   pFO->PerformOperations();
   pFO->Release();
   return 0;
}

/**
 * Execute a menuitem command on one or more shell items.
 * This is a helper function that executes a menu command on the item selection.
 */
HRESULT CShellFolder::ExecuteMenuCommand(VFS_MENUCOMMAND& Cmd)
{
   // Translate some verbs for easy accessibility.
   // Shell sometimes calls us only with the verb and not the ID of the verb.
   static struct {
      LPCSTR pszVerb;
      UINT uVerb;
   } aVerbs[] = {
      { CMDSTR_COPYA,        DFM_CMD_COPY },
      { CMDSTR_PASTEA,       DFM_CMD_PASTE },
      { CMDSTR_DELETEA,      DFM_CMD_DELETE },
      { CMDSTR_RENAMEA,      DFM_CMD_RENAME },
      { CMDSTR_NEWFOLDERA,   DFM_CMD_NEWFOLDER },
      { CMDSTR_PROPERTIESA,  DFM_CMD_PROPERTIES },
   };
   for( int i = 0; Cmd.wMenuID == 0 && Cmd.pstrVerb != NULL && i < lengthof(aVerbs); i++ ) {
      if( strcmp(Cmd.pstrVerb, aVerbs[i].pszVerb) == 0 ) Cmd.wMenuID = aVerbs[i].uVerb;
   }
   // Do we have a DataObject available, or try getting it now...
   CComPtr<IDataObject> spDataObject;
   if( Cmd.pDataObject == NULL ) 
   {
      ::OleGetClipboard(&spDataObject);
      if( spDataObject != NULL ) {
         Cmd.pDataObject = spDataObject;
         DataObj_GetData(Cmd.pDataObject, CFSTR_PREFERREDDROPEFFECT, &Cmd.dwDropEffect, sizeof(Cmd.dwDropEffect));
      }
   }
   // Prepare command execution; it may either be a command on the folder item
   // or on one or more selected items.
   HRESULT HrRes = S_FALSE;
   if( Cmd.pShellItems == NULL ) 
   {
      // No ShellItemArray; execute directly on view
      HRESULT Hr = m_spFolderItem->ExecuteMenuCommand(Cmd);
      if( Hr != E_NOTIMPL ) HrRes = Hr;
   }
   else
   {
      // Execute on every item in selection...
      DWORD dwItems = 0;
      HR( Cmd.pShellItems->GetCount(&dwItems) );
      for( DWORD i = 0; i < dwItems; i++ ) {
         CComPtr<IShellItem> spShellItem;
         HR( Cmd.pShellItems->GetItemAt(i, &spShellItem) );
         CNseItemPtr spItem = GenerateChildItemFromShellItem(spShellItem);
         if( spItem == NULL ) continue;
         HRESULT Hr = spItem->ExecuteMenuCommand(Cmd);
         if( Hr != E_NOTIMPL ) HrRes = Hr;
         if( HrRes != S_OK ) break;
         // HarryWu, 20140729
         // Some operations(GUI present) should be batched.
         if (Cmd.wMenuID == ID_FILE_SHARE) break;
         if (Cmd.wMenuID == ID_FILE_VIEWLOG) break;
         if (Cmd.wMenuID == ID_FILE_DISTRIBUTE) break;
         if (Cmd.wMenuID == ID_FILE_INNERLINK) break;
         if (Cmd.wMenuID == ID_FILE_OLDVERSION) break;
      }
   }
   // Any item can abort if they performed the entire operation alone
   if( HrRes == NSE_S_ALL_DONE ) HrRes = S_OK;
   // Perform any file-operation requested.
   // Execute commands on separate thread if slow copying is expected.
   // TODO: Make this COM safe! Also right now, we can only do this on a COPY operation, and not a MOVE operation, 
   //       because of the need to notify the data-object in CDropTarget::Drop().
   if( Cmd.pFO != NULL ) 
   {
      if( SUCCEEDED(HrRes) ) {
         if( _ShellModule.GetConfigBool(VFS_CAN_SLOW_COPY) && (Cmd.dwDropEffect & DROPEFFECT_COPY) ) {
            ::SHCreateThread(FileOperationThread, Cmd.pFO, CTF_COINIT_STA | CTF_PROCESS_REF | CTF_THREAD_REF | CTF_INSIST, NULL);
            Cmd.pFO = NULL;
         }
         else {
            HrRes = Cmd.pFO->PerformOperations();
            BOOL bAborted = FALSE;
            if( SUCCEEDED(HrRes) && SUCCEEDED(Cmd.pFO->GetAnyOperationsAborted(&bAborted)) && bAborted ) HrRes = E_ABORT;
         }
      }
      if( Cmd.pFO != NULL ) Cmd.pFO->Release();
      Cmd.pFO = NULL;
   }
   // Free any remaining user-data
   free(Cmd.pUserData);
   Cmd.pUserData = NULL;
   return HrRes;
}

/**
 * Show Context Menu for NSE folder item.
 */
HRESULT CShellFolder::CreateMenu(HWND hwndOwner, LPCTSTR pstrMenuType, IContextMenu3** ppRetVal)
{
   ATLASSERT(pstrMenuType);
   ATLASSERT(ppRetVal);
   if( ::IsMenu(m_hMenu) ) ::DestroyMenu(m_hMenu);
   m_hMenu = m_spFolderItem->GetMenu();
   m_hContextMenu = ::GetSubMenu(m_hMenu, pstrMenuType);
   DEFCONTEXTMENU dcm = { hwndOwner, static_cast<IContextMenuCB*>(this), m_pidlMonitor, static_cast<IShellFolder*>(this), 0, NULL, NULL, 0, NULL };
   return ::SHCreateDefaultContextMenu(&dcm, IID_PPV_ARGS(ppRetVal));
}

// Implementation

/**
 * Get information about a column.
 */
HRESULT CShellFolder::_GetColumnDetailsOf(const SHCOLUMNID* pscid, SHELLDETAILS* psd) const
{
   ATLASSERT(pscid);
   ATLASSERT(psd);
   // HarryWu, 2014.3.18
   // Here is a chance to customize the column name.
   psd->str.pOleStr = NULL;
   psd->str.uType = STRRET_WSTR;
   CComPtr<IPropertyDescription> spDescription;
   HR( ::PSGetPropertyDescription(*pscid, IID_PPV_ARGS(&spDescription)) );
   PROPDESC_VIEW_FLAGS pvf = PDVF_DEFAULT;
   HR( spDescription->GetViewFlags(&pvf) );
   if( IsBitSet(pvf, PDVF_CANWRAP) ) psd->fmt |= LVCFMT_WRAP;
   if( IsBitSet(pvf, PDVF_FILLAREA) ) psd->fmt |= LVCFMT_FILL;
   if( IsBitSet(pvf, PDVF_RIGHTALIGN) ) psd->fmt |= LVCFMT_RIGHT;
   if( IsBitSet(pvf, PDVF_HIDELABEL) ) psd->fmt |= LVCFMT_NO_TITLE;
   if( IsBitSet(pvf, PDVF_CENTERALIGN) ) psd->fmt |= LVCFMT_CENTER;
   if( IsBitSet(pvf, PDVF_BEGINNEWGROUP) ) psd->fmt |= LVCFMT_LINE_BREAK;  
   HR( spDescription->GetDefaultColumnWidth((UINT*)(&psd->cxChar)) );
   HRESULT Hr = spDescription->GetDisplayName(&psd->str.pOleStr);
   if( FAILED(Hr) ) Hr = StrToSTRRET(L"", &psd->str);
   return Hr;
}

/**
 * Set the menu state from a selection of items.
 */
HRESULT CShellFolder::_SetMenuState(HMENU hMenu, IDataObject* pDataObject)
{
   CComPtr<IShellItemArray> spShellItems;
   ::SHCreateShellItemArrayFromDataObject(pDataObject, IID_PPV_ARGS(&spShellItems));
   VFS_MENUSTATE State = { hMenu, spShellItems };
   if( spShellItems == NULL ) 
   {
      m_spFolderItem->SetMenuState(State);
   }
   else
   {
      DWORD dwItems = 0;
      HR( spShellItems->GetCount(&dwItems) );
      for( DWORD i = 0; i < dwItems; i++ ) {
         CComPtr<IShellItem> spShellItem;
         HR( spShellItems->GetItemAt(i, &spShellItem) );
         CNseItemPtr spItem = GenerateChildItemFromShellItem(spShellItem);
         if( spItem != NULL ) spItem->SetMenuState(State);
      }
   }
   return S_OK;
}

HRESULT CShellFolder::_RefineUserMenuItems(HMENU hMenu, int cidl, PCUITEMID_CHILD_ARRAY rgpidl)
{
    // HarryWu, 2014.2.20
    // Disable menu items here.
    std::wstring idstring = _T("");
    for (int i = 0; i < cidl; i ++){
        const NSEFILEPIDLDATA * pNseDataPtr = (const NSEFILEPIDLDATA *)(rgpidl [i]);
        if (!pNseDataPtr) continue ;
        wchar_t buffer[128] = _T("");
        swprintf_s(buffer, lengthof(buffer), _T("%d:%d.%d"), static_cast<int>(IsBitSet(pNseDataPtr->wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)), pNseDataPtr->wfd.dwId.category, pNseDataPtr->wfd.dwId.id);
        idstring += buffer; idstring += _T(";");
    }

    MenuType selectedMenus = MenuDef_AllRemoved;
    if (!idstring.empty()){
        m_spFolderItem->SelectMenuItems(m_hShellDefView, idstring.c_str(), &selectedMenus);
    }

	if (!IsBitSet(selectedMenus, MenuDef_OpenFile)) ::RemoveMenu(hMenu, ID_FILE_OPEN, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_DownloadFile)) ::RemoveMenu(hMenu, ID_FILE_EXTRACT, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_NewFolder)) ::RemoveMenu(hMenu, ID_FILE_NEWFOLDER, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Properties)) ::RemoveMenu(hMenu, ID_FILE_PROPERTIES, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Share)) ::RemoveMenu(hMenu, ID_FILE_SHARE, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Upload)) ::RemoveMenu(hMenu, ID_FILE_UPLOAD, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Preview)) ::RemoveMenu(hMenu, ID_FILE_PREVIEW, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Innerlink)) ::RemoveMenu(hMenu, ID_FILE_INNERLINK, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Distribute)) ::RemoveMenu(hMenu, ID_FILE_DISTRIBUTE, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Lock)) ::RemoveMenu(hMenu, ID_FILE_LOCK, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Unlock)) ::RemoveMenu(hMenu, ID_FILE_UNLOCK, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_OldVersion)) ::RemoveMenu(hMenu, ID_FILE_OLDVERSION, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_Viewlog)) ::RemoveMenu(hMenu, ID_FILE_VIEWLOG, MF_BYCOMMAND);
	if (!IsBitSet(selectedMenus, MenuDef_ExtEdit)) ::RemoveMenu(hMenu, ID_FILE_EXTEDIT, MF_BYCOMMAND); 
    if (!IsBitSet(selectedMenus, MenuDef_Properties)) ::RemoveMenu(hMenu, ID_FILE_PROPERTIES, MF_BYCOMMAND); 
    if (!IsBitSet(selectedMenus, MenuDef_TrashRestore)) ::RemoveMenu(hMenu, ID_FILE_RECOVER, MF_BYCOMMAND); 
    if (!IsBitSet(selectedMenus, MenuDef_TrashClear)) ::RemoveMenu(hMenu, ID_FILE_CLEAR_ALL, MF_BYCOMMAND); 

	if (!DMHttpTransferIsEnable()){
		::RemoveMenu(hMenu, ID_FILE_PREV, MF_BYCOMMAND); 
		::RemoveMenu(hMenu, ID_FILE_NEXT, MF_BYCOMMAND); 
		::RemoveMenu(hMenu, ID_FILE_SEARCH, MF_BYCOMMAND); 
		::RemoveMenu(hMenu, ID_FILE_UPLOAD, MF_BYCOMMAND); 
	}

	return S_OK;
}

HRESULT CShellFolder::_RefineShellMenuItems(HMENU hMenu, IDataObject * pDataObject)
{
	// HarryWu, 2014.2.20
	// Disable menu items here.
	CComPtr<IShellItemArray> spShellItems;
	if (::SHCreateShellItemArrayFromDataObject(pDataObject, IID_PPV_ARGS(&spShellItems)))
	{
		return S_OK;
	}

	std::wstring idstring = _T("");
	DWORD ic = 0; spShellItems->GetCount(&ic);
	for (int i = 0; i < ic; i++){
		CComPtr<IShellItem> spShellItem;
		spShellItems->GetItemAt(i, &spShellItem);
		CNseItemPtr spItem = GenerateChildItemFromShellItem(spShellItem);
		if( spItem == NULL ) continue;
		VFS_FIND_DATA wfd = spItem->GetFindData();
		wchar_t buffer[128] = _T("");
		swprintf_s(buffer, lengthof(buffer), _T("%d:%d.%d"), static_cast<int>(IsBitSet(wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)), wfd.dwId.category, wfd.dwId.id);
		idstring += buffer; idstring += _T(";");
	}

	MenuType selectedMenus = MenuDef_AllRemoved;
	if (!idstring.empty()){
		m_spFolderItem->SelectMenuItems(m_hShellDefView, idstring.c_str(), &selectedMenus);
	}

	if (!IsBitSet(selectedMenus, MenuDef_Properties)) ::RemoveMenu(hMenu, SYSTEM_CMDID_PROPERTIES, MF_BYCOMMAND); 
    if (!IsBitSet(selectedMenus, MenuDef_Copy)) ::RemoveMenu(hMenu, SYSTEM_CMDID_COPY, MF_BYCOMMAND); 
    if (!IsBitSet(selectedMenus, MenuDef_Cut)) ::RemoveMenu(hMenu, SYSTEM_CMDID_CUT, MF_BYCOMMAND); 
	if (!IsBitSet(selectedMenus, MenuDef_Delete)) ::RemoveMenu(hMenu, SYSTEM_CMDID_DELETE, MF_BYCOMMAND); 
	if (!IsBitSet(selectedMenus, MenuDef_Rename)) ::RemoveMenu(hMenu, SYSTEM_CMDID_RENAME, MF_BYCOMMAND); 
	return S_OK;
}

/**
 * Advanced parsing from display-name when bind-options are available.
 */
HRESULT CShellFolder::_ParseDisplayNameWithBind(CNseItemPtr& spItem, PWSTR pszDisplayName, IBindCtx* pbc, const BIND_OPTS& Opts)
{
   ATLASSERT(pbc);
   ATLASSERT(pszDisplayName);
   if( spItem == NULL && Opts.grfMode == STGM_CREATE ) {
      // The Shell wants to create a new file. For convenience it may provide a WIN32_FIND_DATA structure
      // with prefilled entries. We'll mark the item with the VFS_HACK_SAVEAS_JUNCTION flag so the physical object
      // can be requested.
      VFS_FIND_DATA wfd = { 0 };
      CComPtr<IFileSystemBindData> spFSBD;
      ::SHGetBindCtxParam(pbc, STR_FILE_SYS_BIND_DATA, IID_PPV_ARGS(&spFSBD));
      if( spFSBD != NULL ) spFSBD->GetFindData((WIN32_FIND_DATA *)&wfd);
      if( wfd.cFileName[0] == '\0' ) wcscpy_s(wfd.cFileName, lengthof(wfd.cFileName), pszDisplayName);
      if( wfd.dwFileAttributes == 0 ) wfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
      wfd.cAlternateFileName[0] = '\0';
      wfd.cAlternateFileName[1] = VFS_HACK_SAVEAS_JUNCTION;
      spItem = m_spFolderItem->GenerateChild(this, m_pidlFolder, wfd);
   }         
   if( spItem != NULL && Opts.grfMode == STGM_FAILIFTHERE ) {
      // FIX: During a Vista Search Folder operation the Shell will call us
      //      with STGM_FAILIFTHERE and refuse to show our items. Try to
      //      detect this and fool the Shell. On Windows 7 the Search Folder
      //      is working beautifully.
      CComPtr<IUnknown> spPOI;
      if( SUCCEEDED( ::SHGetBindCtxParam(pbc, L"ParseOriginalItem", IID_PPV_ARGS(&spPOI)) ) ) return S_OK;
      // Normally we obey the STGM_FAILIFTHERE flag
      return AtlHresultFromWin32(ERROR_FILE_EXISTS);            
   }
   if( spItem != NULL && Opts.grfMode == STGM_READWRITE ) {
      // During the FileSave dialog, the Shell may want to overwrite an existing file. Let's
      // mark the file so the physical object can be requested.
      VFS_FIND_DATA wfd = spItem->GetFindData();
      wfd.cAlternateFileName[1] = VFS_HACK_SAVEAS_JUNCTION;
      spItem = m_spFolderItem->GenerateChild(this, m_pidlFolder, wfd);
   }
   return S_OK;
}

HRESULT CShellFolder::_NaviageTo(PCUITEMID_CHILD pidl)
{
    if (!m_spUnkSite) return E_NOINTERFACE;
    CComQIPtr<IServiceProvider> spService = m_spUnkSite;
    CComPtr<IShellBrowser> spBrowser;
    HR( spService->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&spBrowser)));
    spBrowser->BrowseObject(pidl, SBSP_SAMEBROWSER);
    return S_OK;
}

HRESULT CShellFolder::_GetCurrentSortColumn(PROPERTYKEY * ppkey, SORTDIRECTION * iDirection)
{
    if (!m_spUnkSite) return E_NOINTERFACE;
    CComQIPtr<IServiceProvider> spService = m_spUnkSite;
    CComPtr<IFolderView2> spFV2;
    HR( spService->QueryService(SID_SFolderView, IID_PPV_ARGS(&spFV2)) );
    SORTCOLUMN sc;
    HR( spFV2->GetSortColumns(&sc, 1));
    *ppkey = sc.propkey;
	*iDirection = sc.direction;
    return S_OK;
}

HRESULT CShellFolder::_RemoveSystemSearchBar(HWND hWndOwner)
{
    // HarryWu, 2014.8.21
    // Here we assume that spy++ display the correct structure of windows.
    // the find algorithm could be modified.

    DWORD dwVersion = GetVersion();
    // Get the Windows version.
    DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    DWORD dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    if (dwMajorVersion == 6 && dwMinorVersion == 1)// Win7
    {
        if (m_hwndOwner == NULL) return S_OK;
        HWND hWnd = GetWindow(m_hwndOwner, GW_HWNDPREV);
        if (hWnd == NULL) return S_OK;
        hWnd = GetWindow(hWnd, GW_CHILD);
        if (hWnd == NULL) return S_OK;
        hWnd = GetWindow(hWnd, GW_CHILD);
        if (hWnd == NULL) return S_OK;
        hWnd = GetWindow(hWnd, GW_HWNDLAST);
        if (hWnd == NULL) return S_OK;
        ShowWindow(hWnd, SW_HIDE);
    }

    if (dwMajorVersion == 6 && dwMinorVersion >= 2) // Win8
    {
        if (m_hwndOwner == NULL) return S_OK;
        HWND hWnd = GetWindow(m_hwndOwner, GW_HWNDPREV);
        if (hWnd == NULL) return S_OK;
        hWnd = GetWindow(hWnd, GW_CHILD);
        if (hWnd == NULL) return S_OK;
        hWnd = GetWindow(hWnd, GW_CHILD);
        if (hWnd == NULL) return S_OK;
        hWnd = GetWindow(hWnd, GW_HWNDFIRST);
        if (hWnd == NULL) return S_OK;
        while(hWnd)
        {
            TCHAR tcClassName[MAX_PATH]=_T("");
            GetClassName(hWnd, tcClassName, MAX_PATH);
            if(!_tcscmp(tcClassName,_T("UniversalSearchBand")))
                break;
            hWnd = GetWindow(hWnd,GW_HWNDNEXT);
        }
        if (hWnd) ShowWindow(hWnd, SW_HIDE);
    }
    return S_OK;
}

HRESULT CShellFolder::_Refresh()
{
    return m_pShellView ? m_pShellView->Refresh() : S_OK;
}

HRESULT CShellFolder::StartOperations( void) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::FinishOperations(HRESULT hrResult) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PreRenameItem( DWORD dwFlags, IShellItem *psiItem, LPCWSTR pszNewName) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PostRenameItem( DWORD dwFlags,IShellItem *psiItem,LPCWSTR pszNewName,HRESULT hrRename,IShellItem *psiNewlyCreated) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PreMoveItem( DWORD dwFlags,IShellItem *psiItem,IShellItem *psiDestinationFolder, LPCWSTR pszNewName) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PostMoveItem( DWORD dwFlags,IShellItem *psiItem,IShellItem *psiDestinationFolder, LPCWSTR pszNewName,HRESULT hrMove,IShellItem *psiNewlyCreated) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PreCopyItem( DWORD dwFlags,IShellItem *psiItem,IShellItem *psiDestinationFolder,LPCWSTR pszNewName) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PostCopyItem( DWORD dwFlags,IShellItem *psiItem,IShellItem *psiDestinationFolder, LPCWSTR pszNewName,HRESULT hrCopy,IShellItem *psiNewlyCreated) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PreDeleteItem( DWORD dwFlags, IShellItem *psiItem) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PostDeleteItem( DWORD dwFlags, IShellItem *psiItem,HRESULT hrDelete,IShellItem *psiNewlyCreated) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PreNewItem( DWORD dwFlags,IShellItem *psiDestinationFolder, LPCWSTR pszNewName) 
{
    return S_OK;
}
HRESULT CShellFolder::PostNewItem( DWORD dwFlags, IShellItem *psiDestinationFolder, LPCWSTR pszNewName, LPCWSTR pszTemplateName, DWORD dwFileAttributes,HRESULT hrNew,IShellItem *psiNewItem) 
{
    static_cast<CNseBaseItem *>((CNseItem *)m_spFolderItem)->_AddSelectEdit(m_spUnkSite, pszNewName, SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE | SVSI_SELECT | SVSI_EDIT);
    return S_OK;
}
HRESULT CShellFolder::UpdateProgress(  UINT iWorkTotal,UINT iWorkSoFar) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::ResetTimer( void) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::PauseTimer( void) 
{
    return E_NOTIMPL;
}
HRESULT CShellFolder::ResumeTimer( void) 
{
    return E_NOTIMPL;
}

OBJECT_ENTRY_AUTO(CLSID_ShellFolder, CShellFolder)

