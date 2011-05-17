#include "stdafx.h"
#include "commons.h"
#include <Windows.h>
#include <AclAPI.h>

void ClearAttributes(WCHAR *fname)
{
	SetFileAttributes(fname, FILE_ATTRIBUTE_NORMAL);
}

BOOL DeleteDirectory(WCHAR *dir_path)
{
	WCHAR temp_path[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA file_info;

	// Cancella il contenuto ricorsivamente
	swprintf_s(temp_path, MAX_PATH, L"%s\\*", dir_path);
	hFind = FindFirstFile(temp_path, &file_info);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// Salta le directory (es: ".", ".." etc...)
			if (!wcscmp(file_info.cFileName, L".") || !wcscmp(file_info.cFileName, L".."))
				continue;

			swprintf_s(temp_path, MAX_PATH, L"%s\\%s", dir_path, file_info.cFileName);

			if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DeleteDirectory(temp_path);
				continue;
			}
			
			ClearAttributes(temp_path);
			DeleteFile(temp_path);
		} while (FindNextFile(hFind, &file_info));
		FindClose(hFind);
	}
	
	// Cancella la directory
	ClearAttributes(dir_path);
	return RemoveDirectory(dir_path);
}

WCHAR *SlashToBackSlash(WCHAR *string)
{
	static WCHAR path[MAX_PATH];
	WCHAR *ptr;

	if (!string)
		return NULL;

	wcscpy_s(path, MAX_PATH, string);

	ptr = path;
	do {
		if (*ptr == L'/')
			*ptr = L'\\';
	} while(*(++ptr));

	return path;
}

char *SkipUnprintable(char *start_ptr)
{
	for(; *start_ptr<=0x20; start_ptr++);
	return start_ptr;
}

WCHAR *GetValueForKey(WCHAR *file_path, char *key_name, DWORD n_entry)
{
	HANDLE hfile, hmap;
	char key_tag[128], *xml_ptr, *search_ptr, *close_array_ptr;
	WCHAR *key_value = NULL, *end_ptr;
	DWORD file_size;

	if (n_entry == 0)
		return NULL;

	sprintf_s(key_tag, sizeof(key_tag), "<key>%s</key>", key_name);
	hfile = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) 
		return NULL;

	file_size = GetFileSize(hfile, NULL);
	if (file_size == INVALID_FILE_SIZE) {
		CloseHandle(hfile);
		return NULL;
	}

	hmap = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, file_size, NULL);
	if (!hmap) {
		CloseHandle(hfile);
		return NULL;
	}

	xml_ptr = (char *)MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
	if (!xml_ptr) {
		CloseHandle(hmap);
		CloseHandle(hfile);
		return NULL;
	}

	if (search_ptr = strstr(xml_ptr, key_tag)) {
		search_ptr+=strlen(key_tag);
		search_ptr = SkipUnprintable(search_ptr);
		if (!strncmp(search_ptr, "<array>", strlen("<array>"))) {
			close_array_ptr = strstr(search_ptr, "</array>");
			if (close_array_ptr) {
				for(; n_entry && (search_ptr=strstr(++search_ptr, "<string>")); n_entry--);
				if (search_ptr && search_ptr<close_array_ptr) {
					search_ptr+=strlen("<string>");
					key_value = (WCHAR *)calloc(256, sizeof(WCHAR));
					if (key_value) {
						_snwprintf_s(key_value, 256, _TRUNCATE, L"%S", search_ptr);
						end_ptr = wcswcs(key_value, L"</string>");
						if (end_ptr)
							end_ptr[0] = 0;
					}
				}
			}
		} else if (!strncmp(search_ptr, "<string>", strlen("<string>"))) {
			if (n_entry == 1) {
				search_ptr+=strlen("<string>");
				key_value = (WCHAR *)calloc(256, sizeof(WCHAR));
				if (key_value) {
					_snwprintf_s(key_value, 256, _TRUNCATE, L"%S", search_ptr);
					end_ptr = wcswcs(key_value, L"</string>");
					if (end_ptr)
						end_ptr[0] = 0;
				}
			}
		}
	}

	UnmapViewOfFile(xml_ptr);
	CloseHandle(hmap);
	CloseHandle(hfile);
	return key_value;
}

void SetPrivilege(LPCWSTR privilege)
{
	HANDLE hProc = 0, hProcToken = 0;
	TOKEN_PRIVILEGES tp;
	LUID luid;
	
	do {	
		if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hProcToken) ) 
			break;
		if (!LookupPrivilegeValue (NULL, privilege , &luid))
			break;

		ZeroMemory (&tp, sizeof (tp));
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges (hProcToken, FALSE, (TOKEN_PRIVILEGES *)&tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	} while (FALSE);

	if (hProcToken)	
		CloseHandle (hProcToken);
	if (hProc) 
		CloseHandle(hProc);
}

void ReadRegValue(WCHAR *subkey, WCHAR *value, DWORD *type, WCHAR **buffer)
{
	DWORD size = NULL;
	HKEY hreg;

	if (type)
		*type = 0;

	if (buffer)
		*buffer = NULL;

	if (RegOpenKey(HKEY_LOCAL_MACHINE, subkey, &hreg) != ERROR_SUCCESS)
		return;

	if (RegQueryValueEx(hreg, value, NULL, type, NULL, &size) != ERROR_SUCCESS) {
		RegCloseKey(hreg);
		return;
	}

	if (!buffer) {
		RegCloseKey(hreg);
		return;
	}
	
	*buffer = (WCHAR *)calloc(size+2, 1);
	if (!(*buffer)) {
		RegCloseKey(hreg);
		return;
	}

	if (RegQueryValueEx(hreg, value, NULL, type, (LPBYTE)(*buffer), &size) != ERROR_SUCCESS) {
		RegCloseKey(hreg);
		SAFE_FREE((*buffer));
		return;
	}

	RegCloseKey(hreg);
}

BOOL RegEnumSubKey(WCHAR *subkey, DWORD index, WCHAR **buffer) 
{
	BOOL ret_val = FALSE;
	WCHAR temp_buff[1024];
	DWORD size = NULL;
	*buffer = NULL;
	HKEY hreg = NULL;

	do {
		if (RegOpenKey(HKEY_LOCAL_MACHINE, subkey, &hreg) != ERROR_SUCCESS)
			break;

		memset(temp_buff, 0, sizeof(temp_buff));
		if (RegEnumKey(hreg, index, temp_buff, (sizeof(temp_buff)/sizeof(temp_buff[0]))-1) != ERROR_SUCCESS)
			break;

		if ( ! ( (*buffer) = (WCHAR *)calloc(wcslen(temp_buff)*2+2, sizeof(WCHAR)) ) )
			break;

		swprintf_s((*buffer), wcslen(temp_buff)+1, L"%s", temp_buff);
		ret_val = TRUE;
	} while(0);

	if (hreg)
		RegCloseKey(hreg);

	return ret_val;
}

DWORD AddAceToObjectsSecurityDescriptor (
    LPTSTR pszObjName,          // name of object
    SE_OBJECT_TYPE ObjectType,  // type of object
    LPTSTR pszTrustee,          // trustee for new ACE
    TRUSTEE_FORM TrusteeForm,   // format of trustee structure
    DWORD dwAccessRights,       // access mask for new ACE
    ACCESS_MODE AccessMode,     // type of ACE
    DWORD dwInheritance         // inheritance flags for new ACE
) 
{
	char errorMsg[256];
    DWORD dwRes = 0;
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;

    if (NULL == pszObjName) 
        return ERROR_INVALID_PARAMETER;

    dwRes = GetNamedSecurityInfo(pszObjName, ObjectType, 
          DACL_SECURITY_INFORMATION,
          NULL, NULL, &pOldDACL, NULL, &pSD);
    if (ERROR_SUCCESS != dwRes)
        goto Cleanup; 

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = dwAccessRights;
    ea.grfAccessMode = AccessMode;
    ea.grfInheritance= dwInheritance;
    ea.Trustee.TrusteeForm = TrusteeForm;
    ea.Trustee.ptstrName = pszTrustee;

    dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    if (ERROR_SUCCESS != dwRes)
        goto Cleanup; 
    
    dwRes = SetNamedSecurityInfo(pszObjName, ObjectType, 
          DACL_SECURITY_INFORMATION,
          NULL, NULL, pNewDACL, NULL);
    if (ERROR_SUCCESS != dwRes)
        goto Cleanup; 

    Cleanup:
        if(pSD != NULL) 
            LocalFree((HLOCAL) pSD); 
        if(pNewDACL != NULL) 
            LocalFree((HLOCAL) pNewDACL); 

        return dwRes;
}

void GeneralInit()
{
	SetPrivilege(SE_RESTORE_NAME);
	SetPrivilege(SE_BACKUP_NAME);
	SetPrivilege(SE_SYSTEM_PROFILE_NAME);
	SetPrivilege(SE_DEBUG_NAME);
}


