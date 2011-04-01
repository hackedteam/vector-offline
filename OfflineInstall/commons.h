#define SAFE_FREE(x) if(x) {free(x); x=NULL;}

extern void GeneralInit();
extern void ReadRegValue(WCHAR *subkey, WCHAR *value, DWORD *type, WCHAR **buffer);
extern BOOL RegEnumSubKey(WCHAR *subkey, DWORD index, WCHAR **buffer);
extern void SetPrivilege(LPCWSTR privilege);
extern WCHAR *FindRCSPath(void);
extern WCHAR *GetValueForKey(WCHAR *file_path, char *key_name, DWORD n_entry);
extern WCHAR *SlashToBackSlash(WCHAR *string);
extern void ClearAttributes(WCHAR *fname);
extern BOOL DeleteDirectory(WCHAR *dir_path);