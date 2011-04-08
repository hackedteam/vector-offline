extern BOOL MAC_RCSInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info);
extern BOOL MAC_RCSUnInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info);
extern BOOL MAC_DriverInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *user_list);
extern BOOL MAC_DriverUnInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *user_list, DWORD installation_count);
extern void MAC_GetSourceFileDirectory(users_struct_t *curr_user, rcs_struct_t *rcs_info, os_struct_t *os_info, WCHAR *src_path);
