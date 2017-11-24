// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _DP_SDK_LIVE_PULL_TASK_H_
#define _DP_SDK_LIVE_PULL_TASK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "SDKRecvTask.h"
#include "time.h"
#include "ScheduleServer.h"

#include "./dpsdk/DPSDK_Core.h"
#include "./dpsdk/DPSDK_Ext.h"
#include "./dpsdk/DPSDK_Core_Error.h"

// 大华平台SDK
#pragma comment(lib, "../dev/dp/dpsdk/DPSDK_Core.lib")
#pragma comment(lib, "../dev/dp/dpsdk/DPSDK_Ext.lib")
#pragma comment(lib, "WS2_32.lib")

namespace ScheduleServer
{
	class CDPSDKLivePullTask : public CSDKRecvTask
	{
	public:
		CDPSDKLivePullTask(SDK_RECV_TASK_INFO& task_info);

	protected:
		virtual int connect_device();
		virtual int disconnect_device();
		virtual SS_Error on_done();

	private:
		static int32_t _sdk_handler;
		static bool _sdk_initialized;
		static CSSMutex _sdk_init_mutex;

		static int init_sdk();

		unsigned char _frame[65536 * 10];
		NAL_TYPE _last_frame_type;
		int _frame_pos;

	private:
		void on_recv_packet(unsigned char* data, int len);
		static int32_t DPSDK_CALLTYPE MediaDataCallback(int32_t nPDLLHandle, int32_t nSeq, int32_t nMediaType, const char* szNodeId, int32_t nParamVal, char* szData, int32_t nDataLen, void* pUserParam);

	private:
		string _ip;
		unsigned short _port;
		string _username;
		string _password;
		string _camera_id;
		string _channel;

		int _seq;

#ifdef PUSH_HK_VIDEO_FILE	
		FILE* _dh_video_file;
		long _file_index;
		long _last_frame_index;
		bool _start_write;
		unsigned long _next_fetch_video_frame_timestamp;
#endif

	};
}

#endif  //_DP_SDK_LIVE_PULL_TASK_H_      
