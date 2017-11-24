// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _TASK_COUNTER_H_
#define _TASK_COUNTER_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "Locker.h"

namespace ScheduleServer
{
	class CTaskCounter
	{
	public:
		CTaskCounter() : _counter(0) {};
		~CTaskCounter() {};

	public:
		void increase()
		{
			CSSLocker lock(&_mutex);

			++_counter;
		}

		void decrease()
		{
			CSSLocker lock(&_mutex);

			--_counter;
		}

		long get()
		{
			CSSLocker lock(&_mutex);

			return _counter;
		}

	private:
		volatile long _counter;
		CSSMutex _mutex;
	};
}

#endif//_TASK_COUNTER_H_