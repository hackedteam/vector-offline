#include "stdafx.h"
#include "Functions_Users.h"
#include "Functions_Users_WIN.h"
#include "Functions_Users_MAC.h"
#include "commons.h"

users_struct_t *GetUserList(os_struct_t *os_info, rcs_struct_t *rcs_info)
{
	if (os_info->os == WIN_OS)
		return WIN_GetUserList(os_info, rcs_info);
	else if (os_info->os == MAC_OS)
		return MAC_GetUserList(os_info, rcs_info);
	else
		return NULL;
}

DWORD GetUserRCSStateImg(users_struct_t *user_info)
{
	if (user_info->rcs_status == RCS_INSTALLED)
		return 8;

	if (user_info->rcs_status == RCS_BROKEN)
		return 9;

	//if (user_info->rcs_status == RCS_CLEAN)
		return 10;
}


DWORD GetUserPrivImg(users_struct_t *user_info)
{
	if (!user_info->is_local)
		return 6;

	if (user_info->is_admin) {
		if (user_info->is_disabled)
			return 0;
		else
			return 1;
	} else {
		if (user_info->is_disabled)
			return 2;
		else
			return 3;
	}
}

users_struct_t *FindUser(DWORD index, users_struct_t *list_head)
{
	for(; list_head; list_head=list_head->next)
		if (list_head->list_index == index)
			break;
	return list_head;
}

void FreeUsersList(users_struct_t *list_head)
{
	users_struct_t *next;

	while(list_head) {
		next = list_head->next;

		SAFE_FREE(list_head->user_name);
		SAFE_FREE(list_head->full_name);
		SAFE_FREE(list_head->desc);
		SAFE_FREE(list_head->user_temp);
		SAFE_FREE(list_head->user_local_settings);
		SAFE_FREE(list_head->user_home);
		SAFE_FREE(list_head->user_hash);

		SAFE_FREE(list_head);
		list_head = next;
	}
}

