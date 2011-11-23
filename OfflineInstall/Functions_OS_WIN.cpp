#include "stdafx.h"
#include "Functions_OS.h"
#include "commons.h"

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
		 !wcscmp(os_info->curr_ver, L"6.2"))   // Windows 8
		return TRUE;

	return FALSE;
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

	return TRUE;
}
