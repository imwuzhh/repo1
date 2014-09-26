    std::string sValue = "D:\\workspace\\github\\repo1\\Debug\\VDriveNSE64.dll,-101";
    HKEY hOpenedKey = NULL;
    LONG nResult = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID\\{2C3256E4-49AA-11D3-8229-0050AE509054}\\DefaultIcon", 0, KEY_ALL_ACCESS, &hOpenedKey);
    BOOL fSuc = RegSetValueExA(hOpenedKey, NULL, 0, REG_SZ, (BYTE *)sValue.c_str(), sValue.length());
    fSuc = RegCloseKey(hOpenedKey);
    PIDLIST_ABSOLUTE pidl;
    SHGetSpecialFolderLocation(GetActiveWindow(), CSIDL_DRIVES, &pidl);
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST | SHCNF_FLUSH, pidl, NULL);
    return 0;