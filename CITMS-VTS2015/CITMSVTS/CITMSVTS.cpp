
// CITMSVTS.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "CITMSVTS.h"
#include "CITMSVTSDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCITMSVTSApp

BEGIN_MESSAGE_MAP(CCITMSVTSApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CCITMSVTSApp construction

CCITMSVTSApp::CCITMSVTSApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CCITMSVTSApp object

CCITMSVTSApp theApp;

//////////////////
LONG MyUnhandledExceptionFilter(_EXCEPTION_POINTERS *ExceptionInfo)
{
	CHAR strDumpFile[MAX_PATH] = { 0 };
	SYSTEMTIME tm;

	HANDLE hFile = NULL;

	GetSystemTime(&tm);
	sprintf(strDumpFile, "CITMSVTS%04d%02d%02d%02d%02d%02d.dmp", tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond);

	hFile = CreateFileA(strDumpFile,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	
	if(hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION ExInfo;
		ExInfo.ThreadId = GetCurrentThreadId();
		ExInfo.ExceptionPointers = ExceptionInfo;
		ExInfo.ClientPointers = NULL;
		BOOL bOK = MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			MiniDumpNormal,
			&ExInfo,
			NULL,
			NULL);

		CloseHandle(hFile);
	}
	
	return EXCEPTION_CONTINUE_SEARCH;
}

void CreateDumpFile(EXCEPTION_POINTERS *pException)
{
	// 创建Dump文件 
	CHAR strDumpFile[MAX_PATH] = { 0 };
	SYSTEMTIME tm;

	GetSystemTime(&tm);
	sprintf(strDumpFile, "CITMSVTS%04d%02d%02d%02d%02d%02d.dmp", tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond);

	HANDLE hDumpFile = CreateFileA(strDumpFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// Dump信息
	MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
	dumpInfo.ExceptionPointers = pException;
	dumpInfo.ThreadId = GetCurrentThreadId();
	dumpInfo.ClientPointers = TRUE;
	
	// 写入Dump文件内容
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
	
	CloseHandle(hDumpFile);
}

#include <string>
using std::string;
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{
	// 这里弹出一个错误对话框并退出程序  
	char szPath[512];
	GetModuleFileNameA(NULL, szPath, 512);
	char *pChar = strrchr(szPath, '\\');
	*(pChar + 1) = 0;
	string strPath = szPath;
	SYSTEMTIME syst;
	GetLocalTime(&syst);
	char strCount[100];
	sprintf_s(strCount, 100, "%d.%.2d.%.2d.%.2d.%.2d.%.2d.%.3d.dmp", syst.wYear -
		2000, syst.wMonth, syst.wDay, syst.wHour, syst.wMinute, syst.wSecond, syst.wMilliseconds);

	strPath += string(strCount);
	MakeSureDirectoryPathExists(strPath.c_str());
	CreateDumpFile(pException);
	FatalAppExitA(0, "*** Unhandled Exception! ***");

	return EXCEPTION_EXECUTE_HANDLER;
}

//////////////////

// CCITMSVTSApp initialization

BOOL CCITMSVTSApp::InitInstance()
{
	::SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)MyUnhandledExceptionFilter);
	//SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CCITMSVTSDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

