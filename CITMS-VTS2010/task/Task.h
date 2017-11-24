// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _TASK_H_
#define _TASK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"

namespace ScheduleServer
{
	//任务基类
	class CTask
	{

	public:
		CTask() : _is_done(false), _owner(NULL), _expire(300), _start_time(timeGetTime()) {};
		virtual ~CTask() {};

		virtual SS_Error run() = 0;
		bool is_done() { return _is_done; };

		void set_owner(void* owner) { _owner = owner; };

		virtual void shutdown() = 0;

	protected:
		virtual SS_Error on_done() = 0;

		virtual SS_Error on_exception() = 0;

	protected:
		bool _is_done;
		
		void* _owner;

	protected:
		unsigned long _expire;
		unsigned long _start_time;

	public:
		void update_expire(unsigned expire)
		{
			_expire = expire;
			_start_time = timeGetTime();
		}

	};
}

#endif  // _TASK_H_      
