#include "stdafx.h"
#include "Functions_OS.h"
#include "Functions_OS_MAC.h"
#include "Functions_OS_WIN.h"
#include "commons.h"

DWORD GetOsImage(os_struct_t *os_info, BOOL is_large_icon)
{
	if (is_large_icon) {
		if (os_info->os == WIN_OS)
			return 0;
		else if (os_info->os == MAC_OS)
			return 1;
		else // Per future espansioni (es: linux)
			return 0;
	} else {
		if (os_info->os == WIN_OS) {
			if (os_info->is_supported)
				return 4;
			else
				return 5;
		} else if (os_info->os == MAC_OS) {
			if (os_info->is_supported)
				return 10;
			else
				return 11;
		} else // Per future espansioni (es: linux)
			return 5;
	} 
}

void FreeOSList(os_struct_t *list_head)
{
	os_struct_t *next;

	while(list_head) {
		next = list_head->next;

		SAFE_FREE(list_head->product_type);
		SAFE_FREE(list_head->computer_name);
		SAFE_FREE(list_head->product_name);
		SAFE_FREE(list_head->csd_version);
		SAFE_FREE(list_head->reg_owner);
		SAFE_FREE(list_head->reg_org);
		SAFE_FREE(list_head->curr_ver);
		SAFE_FREE(list_head->product_id);

		SAFE_FREE(list_head);
		list_head = next;
	}
}

os_struct_t *GetOSList() 
{
	os_struct_t *list_head = NULL, temp_os_struct;
	os_struct_t **list_curr = NULL;
	WCHAR driver_letters[512];
	DWORD driver_len, i;

	if (!(driver_len = GetLogicalDriveStrings(sizeof(driver_letters)/sizeof(driver_letters[0]), driver_letters)))
		return NULL;

	list_curr = &list_head;
	// Cicla tutti i volumi montati
	for (i=0; i<driver_len; i+=4) {

		memset(&temp_os_struct, 0, sizeof(os_struct_t));
		memcpy(temp_os_struct.drive, &driver_letters[i], 4*sizeof(WCHAR));

		if(!RecognizeWindowsOS(&driver_letters[i], &temp_os_struct) &&
		   !RecognizeMacOS(&driver_letters[i], &temp_os_struct))
			continue;

		// Crea la struttura
		if ( !(*list_curr = (os_struct_t *)malloc(sizeof(os_struct_t))) )  {
			FreeOSList(&temp_os_struct);
			return list_head;
		}

		// Valorizza la struttura
		memcpy((*list_curr), &temp_os_struct, sizeof(os_struct_t));
		list_curr = &((*list_curr)->next);
	}

	return list_head;
}


