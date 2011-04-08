extern BOOL ReadRCSInfo(rcs_struct_t *rcs_info);
extern BOOL RCSInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info);
extern BOOL GetSourceFileDirectory(users_struct_t *curr_user, os_struct_t *curr_elem, rcs_struct_t *rcs_info, WCHAR *src_path);
extern BOOL RCSUnInstall(rcs_struct_t *rcs_info, users_struct_t *user_info, os_struct_t *os_info);
extern BOOL DriverUnInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *user_list, DWORD installation_count);
extern BOOL DriverInstall(os_struct_t *os_info, rcs_struct_t *rcs_info, users_struct_t *user_list);
