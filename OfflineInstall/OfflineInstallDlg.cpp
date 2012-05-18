// OfflineInstallDlg.cpp : implementation file
//

#include "stdafx.h"
#include "OfflineInstall.h"
#include "OfflineInstallDlg.h"
#include "Functions_OS.h"
#include "Functions_Users.h"
#include "Functions_RCS.h"
#include "commons.h"
#include "LogExport.h"
#include <string.h>
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// COfflineInstallDlg dialog




COfflineInstallDlg::COfflineInstallDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COfflineInstallDlg::IDD, pParent),
	os_list_head(NULL),
	users_list_head(NULL)
	, m_osinfo(_T(""))
	, m_install_kernel(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COfflineInstallDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX3, m_oslist);
	DDX_Text(pDX, IDC_EDIT1, m_osinfo);
	DDX_Control(pDX, IDC_LIST3, m_user_list);
	DDX_Control(pDX, IDOK, m_install_button);
	DDX_Control(pDX, IDCANCEL, m_uninstall_button);
	DDX_Control(pDX, IDC_BITMAP, m_bitmap);
	DDX_Check(pDX, IDC_CHECK1, m_install_kernel);
	DDX_Control(pDX, IDC_BUTTON4, m_export_button);
	DDX_Control(pDX, IDC_CHECK1, m_install_kernel_button);
}

BEGIN_MESSAGE_MAP(COfflineInstallDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON3, &COfflineInstallDlg::OnBnClickedButton3)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX3, &COfflineInstallDlg::OnCbnSelchangeComboboxex3)
	ON_BN_CLICKED(IDOK, &COfflineInstallDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &COfflineInstallDlg::OnBnClickedCancel)
	ON_NOTIFY(NM_RCLICK, IDC_LIST3, &COfflineInstallDlg::OnNMRClickList3)
	ON_BN_CLICKED(IDC_BUTTON1, &COfflineInstallDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &COfflineInstallDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON4, &COfflineInstallDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &COfflineInstallDlg::OnBnClickedButton5)
END_MESSAGE_MAP()


// COfflineInstallDlg message handlers

BOOL COfflineInstallDlg::OnInitDialog()
{
	HANDLE hfile;
	DWORD dummy, counter;
	WCHAR title_string[256];
	BYTE mac_drive_a[100] = {0x93, 0x95, 0xA1, 0x5C, 0xF3, 0x0E, 0x95, 0x40, 0x83, 0xE1, 0x31, 0xCF, 0xE9, 0x37, 0xBA, 0xBB,
						     0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x03, 0xF0,
						     0x6D, 0xAB, 0x2F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						     0x00, 0x00, 0x00, 0x00};


	CDialog::OnInitDialog();

	for (counter=0; (hfile = CreateFile(L"\\\\.\\MacDrive", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL))==INVALID_HANDLE_VALUE && counter<5; counter++) 
		Sleep(1000);

	if (hfile != INVALID_HANDLE_VALUE) {
		DeviceIoControl(hfile, 0x222118, mac_drive_a, 100, 0, 0, &dummy, NULL);
		CloseHandle(hfile);
		Sleep(2000);
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	RECT rect;
	m_user_list.GetClientRect(&rect);
	DWORD width = rect.right - rect.left;
#define NAME_SIZE 150
#define FULL_NAME_SIZE 150
	m_user_list.InsertColumn(0, L"Name", LVCFMT_LEFT, NAME_SIZE);
	m_user_list.InsertColumn(1, L"Full Name", LVCFMT_LEFT, FULL_NAME_SIZE);
	m_user_list.InsertColumn(2, L"Description", LVCFMT_LEFT, width-NAME_SIZE-FULL_NAME_SIZE);

	m_waitcursor = LoadCursor(NULL, IDC_WAIT);
	m_stdcursor = LoadCursor(NULL, IDC_ARROW);
	// Icone windows
	bit_supp[0] = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BITMAP2));
	bit_unsupp[0] = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BITMAP1));
	// Icone mac
	bit_supp[1] = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BITMAP4));
	bit_unsupp[1] = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BITMAP3));


	m_install_kernel = TRUE;
	UpdateData(FALSE);

	ShowWindow(SW_SHOWNORMAL);

	CImageList *m_pImageList = new CImageList();
	m_pImageList->Create(16, 16, ILC_MASK | ILC_COLOR32, 7, 7);
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON3));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON4));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON5));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON6));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON1));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON2));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON7));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON8));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON9));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON10));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON11));
	m_pImageList->Add((AfxGetApp())->LoadIcon(IDI_ICON12));

	m_user_list.SetImageList(m_pImageList, LVSIL_SMALL);
	m_user_list.SetImageList(m_pImageList, LVSIL_STATE);
	m_oslist.SetImageList(m_pImageList);

#define MAX_TRY 10
#define TRY_SLEEP 3000
	DWORD i;
	do {
		for (i=0; i<MAX_TRY; i++) {
			if(ReadRCSInfo(&m_rcs_info))
				 break;
			 Sleep(TRY_SLEEP);
		}
		if (i>=MAX_TRY) {
			if (MessageBox(L"Can't find RCS configuration files", L"Error", MB_RETRYCANCEL) == IDCANCEL) {
				OnBnClickedButton1(); // Spegne la macchina
				return TRUE;
			}
		} else
			break;
	} while(1);

	// Setta il titolo della finestra
	_snwprintf_s(title_string, 256, _TRUNCATE, L"RCS Offline Installation    v%s", m_rcs_info.version);
	SetWindowText(title_string);

	// Acquisisce la lista dei sistemi operativo
	GeneralInit();
	OnBnClickedButton3(); // <- Rescan

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void COfflineInstallDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR COfflineInstallDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Tasto Rescan
void COfflineInstallDlg::OnBnClickedButton3()
{
	int i;
	os_struct_t *curr_elem;
	COMBOBOXEXITEM cmbitem;
	cmbitem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	cmbitem.iItem = 0;

	do {
		SetCursor(m_waitcursor);
		os_list_head = GetOSList();
		if (!os_list_head) {
			SetCursor(m_stdcursor);
			if (MessageBox(L"No OS found", L"Error", MB_RETRYCANCEL) == IDCANCEL) {
				OnBnClickedButton1(); // Spegne la macchina
				return;
			}
		}
	} while(!os_list_head);

	if ( (i = m_oslist.GetCount()) == CB_ERR ) {
		SetCursor(m_stdcursor);
		return;
	}
	while(i--)
		m_oslist.DeleteItem(i);

	for(curr_elem=os_list_head; curr_elem; curr_elem=curr_elem->next) {
		WCHAR os_info[1024];
		
		swprintf_s(os_info, sizeof(os_info)/sizeof(os_info[0]), L"%s (%s)", curr_elem->computer_name, curr_elem->product_name);
		cmbitem.pszText = os_info;
		cmbitem.iImage = GetOsImage(curr_elem, FALSE);
		cmbitem.iSelectedImage = GetOsImage(curr_elem, FALSE);
		m_oslist.InsertItem(&cmbitem);

		curr_elem->list_index = cmbitem.iItem;
		cmbitem.iItem++;
	}
	m_oslist.SetCurSel(0);
	// Ci pensa questa funzione a rimettere il cursore normale
	OnCbnSelchangeComboboxex3();
}

// La selezione sulla combo box OS è cambiata
void COfflineInstallDlg::OnCbnSelchangeComboboxex3()
{
	os_struct_t *curr_elem;
	users_struct_t *curr_user;
	WCHAR os_info_str[2048];
	DWORD i;
	DWORD curr_sel = m_oslist.GetCurSel();

	SetCursor(m_waitcursor);

	// Cerca l'elemento selezionato
	for(curr_elem=os_list_head; curr_elem; curr_elem=curr_elem->next) 
		if (curr_elem->list_index == curr_sel)
			break;

	if (!curr_elem) {
		SetCursor(m_stdcursor);
		return;
	}

	// Aggiorna le OS Info
	swprintf_s(os_info_str, sizeof(os_info_str)/sizeof(os_info_str[0]), 
		L"Computer Name: %s\r\nOS Version: %s %s (%d-bit)\r\nRegistered To: %s%s%s%s\r\nProduct ID: %s\r\n"
		L"Install Date: %04d-%02d-%02d", curr_elem->computer_name, curr_elem->product_name,
		curr_elem->csd_version ? curr_elem->csd_version :  L"", curr_elem->arch, curr_elem->reg_owner, 
		(curr_elem->reg_org && wcslen(curr_elem->reg_org)) ? L" (" : L"", curr_elem->reg_org ? curr_elem->reg_org : L"", (curr_elem->reg_org && wcslen(curr_elem->reg_org)) ? L")" : L"", curr_elem->product_id, 
		curr_elem->install_date.wYear, curr_elem->install_date.wMonth, curr_elem->install_date.wDay);
	m_osinfo = os_info_str;
	UpdateData(FALSE);

	// Su MacOSX il driver deve essere installato per forza
	if (curr_elem->os == MAC_OS) {
		m_install_kernel=TRUE;	
		m_install_kernel_button.EnableWindow(FALSE);
	} else
		m_install_kernel_button.EnableWindow(TRUE);

	// Aggiorna l'immagine del sistema e i tasti di install/uninstall
	if (curr_elem->is_supported) {
		m_install_button.EnableWindow(TRUE);
		m_uninstall_button.EnableWindow(TRUE);
		m_export_button.EnableWindow(TRUE);
		m_bitmap.SetBitmap(bit_supp[GetOsImage(curr_elem, TRUE)]);
	} else {
		m_install_button.EnableWindow(FALSE);
		m_uninstall_button.EnableWindow(FALSE);
		m_export_button.EnableWindow(FALSE);
		m_bitmap.SetBitmap(bit_unsupp[GetOsImage(curr_elem, TRUE)]);
	}

	// Crea la lista degli utenti per quel OS
	if (users_list_head)
		FreeUsersList(users_list_head);
	users_list_head = GetUserList(curr_elem, &m_rcs_info);
	m_user_list.DeleteAllItems();

	// Riempi le colonne con le info degli utenti	
	for(curr_user=users_list_head, i=0; curr_user; curr_user=curr_user->next) {
		if (curr_user->user_name)
			curr_user->list_index = m_user_list.InsertItem(i, curr_user->user_name, GetUserPrivImg(curr_user));
		else
			curr_user->list_index = -1;

		if (curr_user->list_index == -1)
			continue;
		if (curr_user->full_name)
			m_user_list.SetItemText(curr_user->list_index, 1, curr_user->full_name);
		if (curr_user->desc)
			m_user_list.SetItemText(curr_user->list_index, 2, curr_user->desc);

		// Setta l'icona se è presente RCS
		LVITEM lvit;
		lvit.stateMask = LVIS_STATEIMAGEMASK;
		lvit.mask = LVIF_STATE;
		lvit.state = INDEXTOSTATEIMAGEMASK(GetUserRCSStateImg(curr_user));
		lvit.iItem = curr_user->list_index;
		lvit.iSubItem = 0;
		m_user_list.SetItem(&lvit);

		i++;
	}

	SetCursor(m_stdcursor);
	UpdateData(FALSE);
}

//Tasto Install
void COfflineInstallDlg::OnBnClickedOk()
{
	DWORD nSelected;
	users_struct_t *curr_user;
	os_struct_t *curr_elem;
	DWORD curr_sel;
	DWORD count, selected;
	WCHAR toprint[1024];
	DWORD i;
	
	selected = m_user_list.GetSelectedCount();
	if (selected == 0) {
		MessageBox(L"Select users first", L"Error", MB_ICONWARNING);
		return;
	}
	if (selected == 1)
		swprintf_s(toprint, 1024, L"Are you sure you want to install RCS for this user?");
	else
		swprintf_s(toprint, 1024, L"Are you sure you want to install RCS for %d users?", selected);

	if (MessageBox(toprint, L"Confirmation", MB_YESNO) != IDYES)
		return;

	SetCursor(m_waitcursor);
	UpdateData(TRUE);
	POSITION p = m_user_list.GetFirstSelectedItemPosition();
	count = 0;

	// Cerca l'os selezionato
	curr_sel = m_oslist.GetCurSel();
	for(curr_elem=os_list_head; curr_elem; curr_elem=curr_elem->next) 
		if (curr_elem->list_index == curr_sel)
			break;

	while (p) {
		nSelected = m_user_list.GetNextSelectedItem(p);
		if ( (curr_user = FindUser(nSelected, users_list_head)) ) {
			if (RCSInstall(&m_rcs_info, curr_user, curr_elem))
				count++;
			else
				RCSUnInstall(&m_rcs_info, curr_user, curr_elem);
		}
	}

	// Aggiorna la lista degli utenti.
	// Ci pensa lui a togliere la clessidra...
	OnCbnSelchangeComboboxex3();

	// Check sull'ibernazione del sistema (per ora solo su windows)
	if (count > 0) {
		ModifyHybernationPermissions(curr_elem);
		if (IsHybernated(curr_elem)) {
			if (MessageBox(L"This system is hibernated. Do you want to dehibernate it?", L"Warning", MB_YESNO) == IDYES)
				InvalidateHybernated(curr_elem);
		}
		RestoreHybernationPermissions(curr_elem);
	}

	// Se c'è almeno un utente con RCS installato allora installa il driver
	if (m_install_kernel) {
		for(curr_user=users_list_head, i=0; curr_user; curr_user=curr_user->next) {
			if (curr_user->rcs_status == RCS_INSTALLED) {
				i++;
				break;
			}
		}
		if (i>0 && !DriverInstall(curr_elem ,&m_rcs_info, users_list_head))
			MessageBox(L"Kernel components installation failed", L"Error", MB_ICONWARNING);
	}

	if (count<selected) {
		swprintf_s(toprint, 1024, L"Installation failed for %d user(s)", selected-count);
		MessageBox(toprint, L"Error", MB_ICONWARNING);
	} else
		MessageBox(L"Installation successful!", L"RCS installation", MB_OK);

}


// Uninstall
void COfflineInstallDlg::OnBnClickedCancel()
{
	DWORD nSelected;
	users_struct_t *curr_user;
	os_struct_t *curr_elem;
	DWORD curr_sel;
	DWORD count, selected;
	WCHAR toprint[1024];
	DWORD i;

	selected = m_user_list.GetSelectedCount();
	if (selected == 0) {
		MessageBox(L"Select users first", L"Error", MB_ICONWARNING);
		return;
	}
	if (selected == 1)
		swprintf_s(toprint, 1024, L"Are you sure you want to uninstall RCS for this user?");
	else
		swprintf_s(toprint, 1024, L"Are you sure you want to uninstall RCS for %d users?", selected);

	if (MessageBox(toprint, L"Confirmation", MB_YESNO) != IDYES)
		return;

	SetCursor(m_waitcursor);
	POSITION p = m_user_list.GetFirstSelectedItemPosition();
	count = 0;

	// Cerca l'os selezionato
	curr_sel = m_oslist.GetCurSel();
	for(curr_elem=os_list_head; curr_elem; curr_elem=curr_elem->next) 
		if (curr_elem->list_index == curr_sel)
			break;

	while (p) {
		nSelected = m_user_list.GetNextSelectedItem(p);
		if ( (curr_user = FindUser(nSelected, users_list_head)) ) {
			if (curr_user->rcs_status==RCS_CLEAN ||
				RCSUnInstall(&m_rcs_info, curr_user, curr_elem) || 
				curr_user->rcs_status!=RCS_INSTALLED)
				count++;
		}
	}

	// Aggiorna la lista utenti.
	// Ci pensa lui a togliere la clessidra...
	OnCbnSelchangeComboboxex3();

	// Conta quanti utenti ci sono con RCS installato
	for(curr_user=users_list_head, i=0; curr_user; curr_user=curr_user->next) {
		if (curr_user->rcs_status == RCS_INSTALLED) {
			i++;
			break;
		}
	} 
	// Ora sta alla funzione decidere se rimuovere o meno i componenti unici
	// Quella windows rimuovera' il driver solo se i==0
	DriverUnInstall(curr_elem ,&m_rcs_info, users_list_head, i);

	if (count<selected) {
		swprintf_s(toprint, 1024, L"Uninstallation failed for %d user(s)", selected-count);
		MessageBox(toprint, L"Error", MB_ICONWARNING);
	} else
		MessageBox(L"Uninstallation successful!", L"RCS uninstallation", MB_OK);


}


// Menu contestuale per select ALL
void COfflineInstallDlg::OnNMRClickList3(NMHDR *pNMHDR, LRESULT *pResult)
{
	int i;
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CPoint pt(pNMListView->ptAction);
	CMenu menu;
	menu.LoadMenu(IDR_MENU1); 
	CMenu* pContextMenu = menu.GetSubMenu(0);
	POINT p;
	p.x = pt.x;
	p.y = pt.y;
	::ClientToScreen(pNMHDR->hwndFrom, &p);
	int nID = pContextMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, p.x, p.y, this);

	if (nID == ID_SELECTALL_UNSELECTALL) {
		for (i=0; i<m_user_list.GetItemCount(); i++) {
			m_user_list.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED | LVIS_FOCUSED);
			m_user_list.EnsureVisible(i, FALSE);
		}
	} else if (nID == ID_SELECTITEMS_UNSELECTALL) {
		for (i=0; i<m_user_list.GetItemCount(); i++) {
			m_user_list.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
			m_user_list.EnsureVisible(i, FALSE);
		}
	}
	*pResult = 0;
}

// Halt
void COfflineInstallDlg::OnBnClickedButton1()
{
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&si, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	CreateProcess(L"X:\\windows\\system32\\wpeutil.exe", L"X:\\windows\\system32\\wpeutil.exe Shutdown", NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi);
}

// Reboot
void COfflineInstallDlg::OnBnClickedButton2()
{
	OnCancel();
}

// Export Log
void COfflineInstallDlg::OnBnClickedButton4()
{
	DWORD selected;
	WCHAR toprint[1024];
	int nSelected, curr_sel;
	users_struct_t *curr_user;
	os_struct_t *curr_elem;
	LogExport export_object;
	
	selected = m_user_list.GetSelectedCount();
	if (selected == 0) {
		MessageBox(L"Select users first", L"Error", MB_ICONWARNING);
		return;
	}
	if (selected == 1)
		swprintf_s(toprint, 1024, L"Are you sure you want to export logs for this user?");
	else
		swprintf_s(toprint, 1024, L"Are you sure you want to export logs for %d users?", selected);

	if (MessageBox(toprint, L"Confirmation", MB_YESNO) != IDYES)
		return;
	
	BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"Select destination directory";
    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );
	
    if ( pidl != 0 ) {
        // get the name of the folder
        WCHAR dest_path[MAX_PATH];
		WCHAR src_path[MAX_PATH];
		BOOL failed=FALSE;
        if ( SHGetPathFromIDList ( pidl, dest_path ) ) {

			// Cerca l'os selezionato
			curr_sel = m_oslist.GetCurSel();
			for(curr_elem=os_list_head; curr_elem; curr_elem=curr_elem->next) 
				if (curr_elem->list_index == curr_sel)
					break;
			// Retrieve per tutti gli utenti
			SetCursor(m_waitcursor);
			POSITION p = m_user_list.GetFirstSelectedItemPosition();
			while (p) {
				nSelected = m_user_list.GetNextSelectedItem(p);
				if ( (curr_user = FindUser(nSelected, users_list_head)) ) {
					if (!GetSourceFileDirectory(curr_user, curr_elem, &m_rcs_info, src_path) ||
						!export_object.Export(&m_rcs_info, curr_elem->time_bias, curr_user->user_name, curr_user->user_hash, curr_elem->computer_name, src_path, dest_path, curr_elem->os, curr_elem->arch))
						failed=TRUE;
				}
			}
			SetCursor(m_stdcursor);
			if (failed)
				MessageBox(L"Export failed", L"Error", MB_ICONWARNING);
			else {
				// Stampa anche la data UTC del client
				LARGE_INTEGER li_time, li_bias;
				FILETIME ft;
				SYSTEMTIME st;
				WCHAR result_string[256];
				
				GetLocalTime(&st);
				SystemTimeToFileTime(&st, &ft);
				li_time.HighPart = ft.dwHighDateTime;
				li_time.LowPart = ft.dwLowDateTime;
				li_bias.QuadPart = (int)curr_elem->time_bias;
				li_bias.QuadPart *= (60*1000*1000*10);
				li_time.QuadPart += li_bias.QuadPart;
				ft.dwHighDateTime = li_time.HighPart;
				ft.dwLowDateTime  = li_time.LowPart;
				FileTimeToSystemTime(&ft, &st);				
				swprintf_s(result_string, 256, L"- Export successful!\r\n\r\n- UTC time: %4d/%02d/%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond); 
				MessageBox(result_string, L"Log export", MB_OK);
			}
        }

        // free memory used
        IMalloc * imalloc = 0;
        if ( SUCCEEDED( SHGetMalloc ( &imalloc )) ) {
            imalloc->Free ( pidl );
            imalloc->Release ( );
        }
    }
}

// Dump Files
void COfflineInstallDlg::OnBnClickedButton5()
{
	DWORD selected;
	WCHAR toprint[1024];
	int nSelected, curr_sel;
	users_struct_t *curr_user;
	os_struct_t *curr_elem;
	LogExport export_object;
	
	selected = m_user_list.GetSelectedCount();
	if (selected == 0) {
		MessageBox(L"Select users first", L"Error", MB_ICONWARNING);
		return;
	}
	if (selected == 1)
		swprintf_s(toprint, 1024, L"Are you sure you want to dump files for this user?");
	else
		swprintf_s(toprint, 1024, L"Are you sure you want to dump files for %d users?", selected);

	if (MessageBox(toprint, L"Confirmation", MB_YESNO) != IDYES)
		return;
	
	BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"Select destination directory";
    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );
	
    if ( pidl != 0 ) {
        // get the name of the folder
        WCHAR dest_path[MAX_PATH];
		BOOL failed=FALSE;
        if ( SHGetPathFromIDList ( pidl, dest_path ) ) {

			// Cerca l'os selezionato
			curr_sel = m_oslist.GetCurSel();
			for(curr_elem=os_list_head; curr_elem; curr_elem=curr_elem->next) 
				if (curr_elem->list_index == curr_sel)
					break;
			// Retrieve per tutti gli utenti
			SetCursor(m_waitcursor);
			POSITION p = m_user_list.GetFirstSelectedItemPosition();
			while (p) {
				nSelected = m_user_list.GetNextSelectedItem(p);
				if ( (curr_user = FindUser(nSelected, users_list_head)) ) {
					WCHAR dump_dir[MAX_PATH];
					if (curr_elem->os == MAC_OS) 
						_snwprintf_s(dump_dir, MAX_PATH, _TRUNCATE, L"%s\\%s", curr_elem->drive, SlashToBackSlash(curr_user->user_home));
					else
						_snwprintf_s(dump_dir, MAX_PATH, _TRUNCATE, L"%s", curr_user->user_home);

					if (!export_object.Dump(&m_rcs_info, curr_elem->time_bias, curr_user->user_name, curr_user->user_hash, curr_elem->computer_name, dump_dir, dest_path, curr_elem->os, curr_elem->arch))
						failed=TRUE;
				}
			}
			SetCursor(m_stdcursor);
			if (failed)
				MessageBox(L"Dump failed", L"Error", MB_ICONWARNING);
			else {
				// Stampa anche la data UTC del client
				LARGE_INTEGER li_time, li_bias;
				FILETIME ft;
				SYSTEMTIME st;
				WCHAR result_string[256];
				
				GetLocalTime(&st);
				SystemTimeToFileTime(&st, &ft);
				li_time.HighPart = ft.dwHighDateTime;
				li_time.LowPart = ft.dwLowDateTime;
				li_bias.QuadPart = (int)curr_elem->time_bias;
				li_bias.QuadPart *= (60*1000*1000*10);
				li_time.QuadPart += li_bias.QuadPart;
				ft.dwHighDateTime = li_time.HighPart;
				ft.dwLowDateTime  = li_time.LowPart;
				FileTimeToSystemTime(&ft, &st);				
				swprintf_s(result_string, 256, L"- Dump successful!\r\n\r\n- UTC time: %4d/%02d/%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond); 
				MessageBox(result_string, L"Dump Files", MB_OK);
			}
        }

        // free memory used
        IMalloc * imalloc = 0;
        if ( SUCCEEDED( SHGetMalloc ( &imalloc )) ) {
            imalloc->Free ( pidl );
            imalloc->Release ( );
        }
    }

}
