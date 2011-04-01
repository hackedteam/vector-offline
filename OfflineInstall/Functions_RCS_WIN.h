extern BOOL WIN_RCSInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info);
extern BOOL WIN_RCSUnInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info);
extern BOOL WIN_DriverInstall(os_struct_t *os_info, rcs_struct_t *rcs_info);
extern BOOL WIN_DriverUnInstall(os_struct_t *os_info, rcs_struct_t *rcs_info);
extern void WIN_GetSourceFileDirectory(users_struct_t *curr_user, rcs_struct_t *rcs_info, WCHAR *src_path);
