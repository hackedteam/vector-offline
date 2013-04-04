#include "stdafx.h"
#include "Functions_OS.h"
#include "commons.h"


DWORD MAC_GetArch(os_struct_t *os_info)
{
	// XXX
	return 32;
}

BOOL MAC_IsSupported(os_struct_t *os_info)
{
	if (!os_info->csd_version)
		return FALSE;

	if (wcsncmp(os_info->csd_version, L"10.5", 4) && wcsncmp(os_info->csd_version, L"10.6", 4) && wcsncmp(os_info->csd_version, L"10.7", 4) && wcsncmp(os_info->csd_version, L"10.8", 4)) 
		return FALSE;

	return TRUE;
}

BOOL RecognizeMacOS(WCHAR *drive_letter, os_struct_t *os_struct)
{
	DWORD dummy;
	WCHAR file_path[256];
	char install_date_string[24];
	HANDLE hfile;

	wsprintf(file_path, L"%smach_kernel", drive_letter);
	hfile = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;
	CloseHandle(hfile);

	// -> OSType
	os_struct->os = MAC_OS;

	// -> SystemRoot
	wcscpy_s(os_struct->system_root, MAX_PATH, drive_letter);

	// -> ProductName
	// -> Version
	wsprintf(file_path, L"%s\\System\\Library\\CoreServices\\SystemVersion.plist", drive_letter);
	os_struct->product_name = GetValueForKey(file_path, "ProductName", 1); 
	os_struct->csd_version = GetValueForKey(file_path, "ProductVersion", 1); 

	// -> Computer Name
	wsprintf(file_path, L"%sLibrary\\Preferences\\SystemConfiguration\\preferences.plist", drive_letter);
	os_struct->computer_name = GetValueForKey(file_path, "HostName", 1);
	if (!os_struct->computer_name)
		os_struct->computer_name = GetValueForKey(file_path, "LocalHostName", 1);

	// -> Install date
	wsprintf(file_path, L"%sprivate\\var\\log\\OSInstall.custom", drive_letter);
	memset(&(os_struct->install_date), 0, sizeof(SYSTEMTIME));
	if ( (hfile = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL)) != INVALID_HANDLE_VALUE ) {
		ReadFile(hfile, install_date_string, 24, &dummy, NULL);
		ZeroMemory(install_date_string, sizeof(install_date_string));
		ReadFile(hfile, install_date_string, 19, &dummy, NULL);
		CloseHandle(hfile);
		sscanf_s(install_date_string, "%d-%d-%d %d:%d:%d", 
			&os_struct->install_date.wYear,
			&os_struct->install_date.wMonth,
			&os_struct->install_date.wDay,
			&os_struct->install_date.wHour,
			&os_struct->install_date.wMinute,
			&os_struct->install_date.wSecond);
	}

	// -> arch
	// -> IsSupported
	// -> owner
	// -> product id
	os_struct->reg_owner  = _wcsdup(L"N.A.");
	os_struct->product_id = _wcsdup(L"N.A.");
	os_struct->arch = MAC_GetArch(os_struct);
	os_struct->is_blacklisted = FALSE;
	os_struct->is_supported = MAC_IsSupported(os_struct); // Va chiamata per ultima...

	return TRUE;
}

