#pragma once

#define WIN_OS 1
#define MAC_OS 2

typedef struct os_struct {
	WCHAR drive[4];
	WCHAR system_root[MAX_PATH];
	WCHAR *product_type;		// HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\ProductOptions\ProductType
	WCHAR *computer_name;		// HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\ComputerName\ComputerName\ComputerName
	WCHAR *product_name;		// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProductName
	WCHAR *csd_version;			// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\CSDVersion
	WCHAR *reg_owner;			// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\RegisteredOwner
	WCHAR *reg_org;				// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\RegisteredOrganization
	WCHAR *curr_ver;			// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\CurrentVersion
	SYSTEMTIME install_date;	// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\InstallDate
	WCHAR *product_id;			// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProductId
	DWORD os;
	DWORD arch;
	BOOL  is_supported;
	BOOL  is_blacklisted;
	WCHAR bl_software[256];
	DWORD time_bias;			// HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\TimeZoneInformation\ActiveTimeBias

	DWORD list_index;

	struct os_struct *next;
} os_struct_t;

extern os_struct_t *GetOSList();
extern DWORD GetOsImage(os_struct_t *os_info, BOOL is_large_icon);