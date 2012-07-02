// OfflineInstallDlg.h : header file
//

#pragma once

#include "Functions_OS.h"
#include "Functions_Users.h"

#include "afxcmn.h"
#include "afxwin.h"

#define MAX_OS_COUNT 4

// COfflineInstallDlg dialog
class COfflineInstallDlg : public CDialog
{
// Construction
public:
	COfflineInstallDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_OFFLINEINSTALL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMCustomdrawProgress1(NMHDR *pNMHDR, LRESULT *pResult);

private:
	os_struct_t *os_list_head;
	users_struct_t *users_list_head;
	HCURSOR m_waitcursor;
	HCURSOR m_stdcursor;
	HBITMAP bit_supp[MAX_OS_COUNT], bit_unsupp[MAX_OS_COUNT];
	WCHAR *m_rcs_path;
	rcs_struct_t m_rcs_info;

public:
	afx_msg void OnBnClickedButton3();
	CComboBoxEx m_oslist;
	afx_msg void OnCbnSelchangeComboboxex3();
	CString m_osinfo;
	afx_msg void OnBnClickedOk();
	CListCtrl m_user_list;
	afx_msg void OnBnClickedCancel();
	afx_msg void OnNMRClickList3(NMHDR *pNMHDR, LRESULT *pResult);
	CButton m_install_button;
	CButton m_uninstall_button;
	afx_msg void OnBnClickedButton1();
	CStatic m_bitmap;
	afx_msg void OnBnClickedButton2();
	BOOL m_install_kernel;
	afx_msg void OnBnClickedButton4();
	CButton m_export_button;
	CButton m_install_kernel_button;
	afx_msg void OnBnClickedButton5();
};
