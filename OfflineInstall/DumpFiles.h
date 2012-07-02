extern WCHAR **PopulateMasks(WCHAR *patterns);
extern void FreeMasks(WCHAR **masks);
extern void ExploreDirectoryAndCapture(WCHAR *start_path, DWORD depth, WCHAR **masks, WCHAR *dest_dir);

