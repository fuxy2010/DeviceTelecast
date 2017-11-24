
// CITMSVTS.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CCITMSVTSApp:
// See CITMSVTS.cpp for the implementation of this class
//

class CCITMSVTSApp : public CWinApp
{
public:
	CCITMSVTSApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CCITMSVTSApp theApp;
