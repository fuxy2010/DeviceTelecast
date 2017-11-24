// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "GeneralDef.h"
#include "BaseThread.h"

using namespace ScheduleServer;

//void*   CBaseThread::_main_thread_data_ptr = NULL;

//DWORD   CBaseThread::_thread_storage_index = 0;

CBaseThread::CBaseThread() :
_thread_handle(NULL),
_parent_ptr(NULL),
_joined(false)
//_thread_data_ptr(NULL)
{
#ifdef _THREAD_STATISTIC_
	//获得计数器的时钟频率
	QueryPerformanceFrequency(&_litmp);
	_clock_freq = (double)_litmp.QuadPart;

	_enter_timestamp = 0;
	_leave_timestamp = 0;
	_now = 0;
	_should_printf_statistic_info = false;
	_log = NULL;
#endif
}

CBaseThread::~CBaseThread()
{
#ifdef _THREAD_STATISTIC_
	if(NULL != _log)
		fclose(_log);
#endif
}

void CBaseThread::initialize()
{
	//_thread_storage_index = ::TlsAlloc();
}

SS_Error CBaseThread::start(void* parent)
{
	unsigned int id(0);

	on_start();//on_start要在最开始调用

	_thread_handle = (HANDLE)_beginthreadex( NULL,   // Inherit security
										0,      // Inherit stack size
										_entry, // Entry function
										(void*)this,    // Entry arg
										0,      // Begin executing immediately
										&id );

	SuspendThread(_thread_handle);//暂停(挂起)线程 
	SetThreadPriority(_thread_handle, THREAD_PRIORITY_HIGHEST);//设置线程优先级
	ResumeThread(_thread_handle);//恢复线程运行

	_parent_ptr = parent;

	_joined = false;

	return SS_NoErr;
}

SS_Error CBaseThread::shutdown()
{
	_joined = true;

	join();

	on_close();//on_start要在最终调用

	return SS_NoErr;
}

void CBaseThread::join()
{
    DWORD theErr = ::WaitForSingleObject(_thread_handle, INFINITE);
}

unsigned int WINAPI CBaseThread::_entry(LPVOID inThread)
{
    CBaseThread* theThread = (CBaseThread*)inThread;

	if(NULL == theThread)
		return -1;

	/*if(FALSE == ::TlsSetValue(_thread_storage_index, theThread))
		return -1;*/

	while(false == theThread->_joined)
	{
		theThread->entry();
	}

    return 0;
}

/*CBaseThread* CBaseThread::get_current()
{
	return (CBaseThread *)::TlsGetValue(_thread_storage_index);
}*/
