// LogExport.cpp : implementation file
//

#include "stdafx.h"
#include "OfflineInstall.h"
#include "LogExport.h"
#define PBCLOSE_WINDOW WM_USER+10


// LogExport dialog

IMPLEMENT_DYNAMIC(LogExport, CDialog)

LogExport::LogExport(CWnd* pParent /*=NULL*/)
	: CDialog(LogExport::IDD, pParent),
	m_user_name(NULL),
	m_user_hash(NULL),
	m_computer_name(NULL),
	m_src_path(NULL),
	m_dest_drive(NULL)
	, m_progress_text(_T(""))
{

}

LogExport::~LogExport()
{
	SAFE_FREE(m_user_name);
	SAFE_FREE(m_user_hash);
	SAFE_FREE(m_computer_name);
	SAFE_FREE(m_src_path);
	SAFE_FREE(m_dest_drive);
}

DWORD ExportThread(LogExport *export_class)
{
	// Lancia la funzione di retrieve
	export_class->m_success = export_class->OfflineRetrieve();
	// Distrugge la dialog
	export_class->PostMessageW(PBCLOSE_WINDOW);
	return 0;
}

// Effettua lo scrambling e il descrimbling di una stringa
// Ricordarsi di liberare la memoria allocata
// E' Thread SAFE
#define ALPHABET_LEN 64
char *LogExport::LOG_ScrambleName(char *string, BYTE scramble, BOOL crypt)
{
	char alphabet[ALPHABET_LEN]={'_','B','q','w','H','a','F','8','T','k','K','D','M',
		                         'f','O','z','Q','A','S','x','4','V','u','X','d','Z',
		                         'i','b','U','I','e','y','l','J','W','h','j','0','m',
                                 '5','o','2','E','r','L','t','6','v','G','R','N','9',
					             's','Y','1','n','3','P','p','c','7','g','-','C'};                  
	char *ret_string;
	DWORD i,j;

	if ( !(ret_string = _strdup(string)) )
		return NULL;

	// Evita di lasciare i nomi originali anche se il byte e' 0
	scramble%=ALPHABET_LEN;
	if (scramble == 0)
		scramble = 1;

	for (i=0; ret_string[i]; i++) {
		for (j=0; j<ALPHABET_LEN; j++)
			if (ret_string[i] == alphabet[j]) {
				// Se crypt e' TRUE cifra, altrimenti decifra
				if (crypt)
					ret_string[i] = alphabet[(j+scramble)%ALPHABET_LEN];
				else
					ret_string[i] = alphabet[(j+ALPHABET_LEN-scramble)%ALPHABET_LEN];
				break;
			}
	}
	return ret_string;
}

void LogExport::PrepareIniFile(WCHAR *fname)
{
	WORD wBOM = 0xFEFF;
	DWORD NumberOfBytesWritten;
	HANDLE hFile = CreateFileW(fname, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile!=INVALID_HANDLE_VALUE) {
		WriteFile(hFile, &wBOM, sizeof(WORD), &NumberOfBytesWritten, NULL);
		CloseHandle(hFile);
	}
}

BOOL LogExport::OfflineRetrieve()
{
	char *scrambled_searchA;
	WCHAR scrambled_path[MAX_PATH];
	char scrambled_nameA[MAX_PATH];
	char *clear_nameA;
	char *tmp_ptr;
	WCHAR clear_path[MAX_PATH];
	WCHAR dest_path[MAX_PATH];
	SYSTEMTIME system_time;
	FILETIME file_time;
	DWORD curr_file=1, tot_file=1;
	WCHAR progress_text[1024];

	HANDLE hFind;
	WIN32_FIND_DATAW find_data;

	// Setta la caption della finestra
	SetWindowText(m_user_name);

	// Crea una directory per i log di questo utente
	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);	
	swprintf_s(dest_path, sizeof(dest_path)/sizeof(dest_path[0]), L"%s\\%s_EXP_%.8X%.8X000000000000000000000000", m_dest_drive, m_rcs_info.rcs_name, file_time.dwHighDateTime, file_time.dwLowDateTime);
	if (!CreateDirectory(dest_path, NULL))
		return FALSE;

	// Crea il file con le info dell'utente
	swprintf_s(clear_path, sizeof(clear_path)/sizeof(clear_path[0]), L"%s\\offline.ini", dest_path);
	PrepareIniFile(clear_path);
	if (!WritePrivateProfileStringW(L"OFFLINE", L"USERID", m_user_name, clear_path))
		return FALSE;
	if (!WritePrivateProfileStringW(L"OFFLINE", L"DEVICEID", m_computer_name, clear_path))
		return FALSE;
	if (!WritePrivateProfileStringW(L"OFFLINE", L"FACTORY", m_rcs_info.rcs_name, clear_path))
		return FALSE;
	if (!WritePrivateProfileStringW(L"OFFLINE", L"INSTANCE", m_user_hash, clear_path))
		return FALSE;

	if (m_os_type == WIN_OS) {
		if (!WritePrivateProfileStringW(L"OFFLINE", L"PLATFORM", L"WINDOWS", clear_path))
			return FALSE;
	} else if (m_os_type == MAC_OS) {
		if (!WritePrivateProfileStringW(L"OFFLINE", L"PLATFORM", L"MACOS", clear_path))
			return FALSE;
	} else {
		if (!WritePrivateProfileStringW(L"OFFLINE", L"PLATFORM", L"UNKNOWN", clear_path))
			return FALSE;
	}

	// Setta la data dell'export
	LARGE_INTEGER li_time, li_bias;
	FILETIME ft;
	SYSTEMTIME st;
	WCHAR wchar_date[128];
	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &ft);
	li_time.HighPart = ft.dwHighDateTime;
	li_time.LowPart = ft.dwLowDateTime;
	li_bias.QuadPart = (int)m_time_bias;
	li_bias.QuadPart *= (60*1000*1000*10);
	li_time.QuadPart += li_bias.QuadPart;
	
	swprintf_s(wchar_date, sizeof(wchar_date)/sizeof(wchar_date[0]), L"%.8X.%.8X", li_time.HighPart, li_time.LowPart);
	if (!WritePrivateProfileStringW(L"OFFLINE", L"SYNCTIME", wchar_date, clear_path))
		return FALSE;

	// Enumera i log dell'utente
	if (m_os_type == WIN_OS) {
		if ( !(scrambled_searchA = LOG_ScrambleName("?LOG*.???", (BYTE)(m_rcs_info.hscramb), TRUE)) )
			return FALSE;
	} else if (m_os_type == MAC_OS) {
		if ( !(scrambled_searchA = LOG_ScrambleName("LOG*.???", (BYTE)(m_rcs_info.hscramb), TRUE)) )
			return FALSE;
	} else
		return FALSE;
	swprintf_s(scrambled_path, sizeof(scrambled_path)/sizeof(scrambled_path[0]), L"%s\\%S", m_src_path, scrambled_searchA);
	SAFE_FREE(scrambled_searchA);

	// Conta il numero di file e setta il limite della progressbar
	if ( (hFind = FindFirstFileW(scrambled_path, &find_data)) == INVALID_HANDLE_VALUE )
		return TRUE;
	for(; FindNextFileW(hFind, &find_data); tot_file++);
	FindClose(hFind);
	m_progress.SetRange32(0, tot_file);
	m_progress.SetStep(1);

	// Enumera realmente i file
	if ( (hFind = FindFirstFileW(scrambled_path, &find_data)) == INVALID_HANDLE_VALUE )
		return TRUE;

	do {
		swprintf_s(progress_text, sizeof(progress_text)/sizeof(WCHAR), L"Copying file %d of %d", curr_file++, tot_file);
		m_progress_text.SetString(progress_text);
		UpdateData(FALSE);

		sprintf_s(scrambled_nameA, sizeof(scrambled_nameA), "%S", find_data.cFileName);
		clear_nameA = LOG_ScrambleName(scrambled_nameA, (BYTE)(m_rcs_info.hscramb), FALSE);
		if (clear_nameA) {
			swprintf_s(scrambled_path, sizeof(scrambled_path)/sizeof(scrambled_path[0]), L"%s\\%s", m_src_path, find_data.cFileName);
			// Se e' un file di tipo LOG_ (nel formato .bin) aggiunge il timestamp per compatibilita' con il formato
			// in cui salva i file ASP
			if (!strncmp(clear_nameA+1, "LOG_", strlen("LOG_")) && (tmp_ptr = strstr(clear_nameA, ".bin"))) {
				*tmp_ptr = 0x00;
				GetSystemTime(&system_time);
				SystemTimeToFileTime(&system_time, &file_time);
				swprintf_s(clear_path, sizeof(clear_path)/sizeof(clear_path[0]), L"%s\\%S_%.8X%.8X.bin", dest_path, clear_nameA, file_time.dwHighDateTime, file_time.dwLowDateTime);
			} else {
				swprintf_s(clear_path, sizeof(clear_path)/sizeof(clear_path[0]), L"%s\\%S", dest_path, clear_nameA);
			}
			SAFE_FREE(clear_nameA);

			// Copia i log con il loro nome in chiaro e cancella la copia originale
			if (find_data.nFileSizeLow==0 || CopyFileW(scrambled_path, clear_path, FALSE))
				DeleteFileW(scrambled_path);
		}
		m_progress.StepIt();
	} while (FindNextFileW(hFind, &find_data));
	FindClose(hFind);		
	return TRUE;
}

BOOL LogExport::Export(rcs_struct_t *rcs_info, DWORD time_bias, WCHAR *user_name, WCHAR *user_hash, WCHAR *computer_name, WCHAR *src_path, WCHAR *dest_drive, DWORD os_type, DWORD arch_type)
{
	m_success = FALSE;
	// Libera eventuali variabili allocate 
	// precedentemente
	SAFE_FREE(m_user_name);
	SAFE_FREE(m_user_hash);
	SAFE_FREE(m_computer_name);
	SAFE_FREE(m_src_path);
	SAFE_FREE(m_dest_drive);
	// Setta i valori attuali
	memcpy(&m_rcs_info, rcs_info, sizeof(rcs_struct_t));
	m_os_type = os_type;
	m_arch_type = arch_type;
	m_time_bias = time_bias;
	m_user_name = _wcsdup(user_name);
	m_user_hash = _wcsdup(user_hash);
	m_computer_name = _wcsdup(computer_name);
	m_src_path = _wcsdup(src_path);
	m_dest_drive = _wcsdup(dest_drive);
	// Apre la dialog che lancera' il thread di retrieve
	DoModal();
	return m_success;
}

void LogExport::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
	DDX_Text(pDX, IDC_EDIT1, m_progress_text);
}


BEGIN_MESSAGE_MAP(LogExport, CDialog)
	ON_MESSAGE(PBCLOSE_WINDOW, OnThreadEnd)
END_MESSAGE_MAP()


// LogExport message handlers
BOOL LogExport::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Lancia il thread che fara' il retrieve
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ExportThread, this, NULL, NULL);

	return TRUE;  
}

afx_msg LRESULT LogExport::OnThreadEnd(WPARAM wParam, LPARAM lParam)
{
	OnOK();
	return 0;
}