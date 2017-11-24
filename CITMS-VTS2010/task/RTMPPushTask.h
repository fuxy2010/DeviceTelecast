// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _RTMP_PUSH_TASK_H_
#define _RTMP_PUSH_TASK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "Task.h"
#include "time.h"
#include "ScheduleServer.h"

namespace ScheduleServer
{
	typedef enum
	{
		RTMPPushTask_Begin = 0,
		RTMPPushTask_Connected,
		RTMPPushTask_PUSH,
		RTMPPushTask_Done
	}
	RTMP_PUSH_TASK_STATUS;

	typedef struct tagRTMP_PUSH_TASK_INFO
	{
		unsigned long	task_id;//任务ID
		unsigned long	ua_id;
		unsigned long	create_timestamp;//创建该任务的时戳
		string			rtmp_url;

		tagRTMP_PUSH_TASK_INFO() : task_id(0), ua_id(0), create_timestamp(::time(NULL))
		{
			rtmp_url = "";
		}

		~tagRTMP_PUSH_TASK_INFO()
		{
		}

		void clear()
		{

		}
	}
	RTMP_PUSH_TASK_INFO;

	class CRTMPPushTask : public CTask
	{
	public:
		CRTMPPushTask(RTMP_PUSH_TASK_INFO& task_info);
		virtual ~CRTMPPushTask();

		virtual SS_Error run();

	protected:
		virtual SS_Error on_done();

		virtual SS_Error on_exception();

	public:
		//int init();

	protected:
		/*volatile */RTMP_PUSH_TASK_STATUS _status;

		RTMP_PUSH_TASK_INFO _task_info;

	private:
		CRTMPSession _rtmp_session;
		
		CUserAgent* _ua;

	private:
		int init_rtmp();
		void push();

	private:
		bool _initialized;

	public:
		bool is_initialized() { return _initialized; }

		virtual void shutdown() { _status = RTMPPushTask_Done; }
	};
}

#endif  //_RTMP_PUSH_TASK_H_      
