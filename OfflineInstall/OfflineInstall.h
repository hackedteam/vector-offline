// OfflineInstall.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// COfflineInstallApp:
// See OfflineInstall.cpp for the implementation of this class
//

class COfflineInstallApp : public CWinApp
{
public:
	COfflineInstallApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern COfflineInstallApp theApp;