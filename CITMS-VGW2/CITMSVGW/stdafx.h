
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars









#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

// C 运行时头文件
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <sys\timeb.h>

using namespace std;

#ifdef _DEBUG
#define ASSERT(x) {if(!(x)) _asm{int 0x03}}
#define VERIFY(x) {if(!(x)) _asm{int 0x03}}
#else
#define ASSERT(x)
#define VERIFY(x) x
#endif

#if defined(_WIN32)
#include <iphlpapi.h>
#include <Nb30.h>
#include <Winsock2.h>
#include <mmsystem.h>
#pragma comment(lib, "IPHlpApi.Lib")
#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#endif

#include "MediaData.h"

#pragma comment(lib, "../utility/ffmpeg/lib/avcodec.lib")
#pragma comment(lib, "../utility/ffmpeg/lib/avformat.lib")
#pragma comment(lib, "../utility/ffmpeg/lib/avutil.lib")
#pragma comment(lib, "../utility/ffmpeg/lib/avdevice.lib")
#pragma comment(lib, "../utility/ffmpeg/lib/avfilter.lib")
#pragma comment(lib, "../utility/ffmpeg/lib/postproc.lib")
#pragma comment(lib, "../utility/ffmpeg/lib/swresample.lib")
#pragma comment(lib, "../utility/ffmpeg/lib/swscale.lib")

#ifndef HIGHEST_LOG_LEVEL
#define HIGHEST_LOG_LEVEL 10
#endif

#ifdef _DEBUG
#define LOG_WRITE(log_str, log_level, show_on_screen) if (SINGLETON(CLogger).can_trace(log_level)) {std::ostringstream _oss; _oss << log_str << "\n{" << __FUNCTION__ << "(" << __LINE__ <<")}";\
	SINGLETON(CScheduleServer).write_log(_oss.str(), log_level, show_on_screen);}
#else
#define LOG_WRITE(log_str, log_level, show_on_screen) if (SINGLETON(CLogger).can_trace(log_level)) {std::ostringstream _oss; _oss << log_str << "\n{" << __FUNCTION__ << "(" << __LINE__ <<")}";\
	SINGLETON(CScheduleServer).write_log(_oss.str(), log_level, show_on_screen);}
#endif

#define LOG_SET_LEVEL(level) SINGLETON(CLogger).set_trace_level(level);
#define LOG_SET_ONSCREEN(level) SINGLETON(CLogger).set_on_screen(level);


#define __STDC_CONSTANT_MACROS

//#pragma comment(lib, "../rtmp/lib/librtmp.lib")
#pragma comment(lib, "../rtmp/lib/libeay32.lib")
#pragma comment(lib, "../rtmp/lib/polarssl.lib")
#pragma comment(lib, "../rtmp/lib/ssleay32.lib")
#pragma comment(lib, "../rtmp/lib/zdll.lib")


