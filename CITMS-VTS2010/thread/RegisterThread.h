// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _REGISTER_THREAD_H_      
#define _REGISTER_THREAD_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "BaseThread.h"
#include "Task.h"

namespace ScheduleServer
{
	//会议线程////////////////////////////////////////////////////////////////////////
	class CRegisterThread : public CBaseThread
	{
	public:
		CRegisterThread() :
		_latest_register_timestamp(0), _latest_show_statistics_timestamp(0), _latest_ua_statistics_timestamp(0)
		{

		};

		virtual ~CRegisterThread() {};

	protected:
		// 线程入口函数
		virtual void entry();

		virtual void on_start();

		virtual void on_close();

	public:
		void restart()
		{
			_latest_register_timestamp = 0;
		}

	private:
		unsigned long _latest_register_timestamp;
		unsigned long _latest_ua_statistics_timestamp;
		unsigned long _latest_show_statistics_timestamp;

	};
}
#endif  // _REGISTER_THREAD_H_   
