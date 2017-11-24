// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _TASK_THREAD_H_      
#define _TASK_THREAD_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "BaseThread.h"
#include "Task.h"
#include "JRTPSession.h"

namespace ScheduleServer
{
	//任务处理线程
	class CTaskThread : public CBaseThread
	{
	public:
		CTaskThread() : _parallel_task_num(10) {};//最多并发执行10项任务
		virtual ~CTaskThread() {};
		
		SS_Error add_task(CTask* task);

		SS_Error remove_all_tasks();

		unsigned long get_task_num();

		static void sleep_ms(unsigned long interval);

	protected:
		// 线程入口函数
		virtual void entry();

		virtual void on_start();

		virtual void on_close();

	protected:
		std::list<CTask*> _task_queue;

		const unsigned long _parallel_task_num;//并发执行任务数上限

	public:
		//CRTPSendSession _rtp_send_session;//RTP发送session
	};

	class CTaskThreadPool
	{
	public:
		static SS_Error add_threads(unsigned long task_thread_num, void* parent);

		static void remove_threads();

		static CTaskThread* select_thread(unsigned long index = 0);

		//获取任务线程数
		static unsigned long get_task_thread_num() { return _task_thread_num; };

		//增加常驻任务计数
		static void increase_resident_task_count(unsigned long num) { _resident_task_cout += num; };

		//减少常驻任务计数
		static void decrease_resident_task_count(unsigned long num) { _resident_task_cout = (num > _resident_task_cout) ? 0 : (_resident_task_cout - num); };

		//获取常驻任务计数
		static unsigned long get_resident_task_count() { return _resident_task_cout; };

	private:

		static CTaskThread** _task_thread_array;//任务线程指针集合

		static unsigned long _task_thread_num;//任务线程总数

		static bool _is_firstly_select;//是否首次调用select_thread

		static HANDLE _create_task_thread_handle;//任务线程是否创建标志句柄

		static unsigned long _resident_task_cout;//常驻任务计数
	};
}

#endif  // _TASK_THREAD_H_   
