#pragma once
#include "afxcmn.h"
#include "Functions_Users.h"
#include "commons.h"

// LogExport dialog

class LogExport : public CDialog
{
	DECLARE_DYNAMIC(LogExport)

public:
	LogExport(CWnd* pParent = NULL);   // standard constructor
	BOOL Export(rcs_struct_t *rcs_info, DWORD time_bias, WCHAR *user_name, WCHAR *user_hash, WCHAR *computer_name, WCHAR *src_path, WCHAR *dest_drive, DWORD os_type, DWORD arch_type);
	BOOL OfflineRetrieve();
	BOOL m_success;
	virtual ~LogExport();

// Dialog Data
	enum { IDD = IDD_DIALOGBAR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	void PrepareIniFile(WCHAR *fname);
	char *LOG_ScrambleName(char *string, BYTE scramble, BOOL crypt);
	afx_msg LRESULT OnThreadEnd(WPARAM wParam, LPARAM lParam);
	rcs_struct_t m_rcs_info;
	
	DWORD m_time_bias;
	DWORD m_os_type;
	DWORD m_arch_type; 

	WCHAR *m_user_name;
	WCHAR *m_computer_name;
	WCHAR *m_src_path;
	WCHAR *m_dest_drive;
	WCHAR *m_user_hash;
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CProgressCtrl m_progress;
protected:
	CString m_progress_text;
};
