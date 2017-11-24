// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _HKP_SDK_LIVE_PULL_TASK_H_
#define _HKP_SDK_LIVE_PULL_TASK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "SDKRecvTask.h"
#include "time.h"
#include "ScheduleServer.h"

#include "tmcp_sdk_defines.h"
#include "tmcp_interface_sdk.h"

//海康平台SDK
#pragma comment(lib, "../dev/hktmcp/tmcp_interface_sdk.lib")
#pragma comment(lib, "WS2_32.lib")

//#define PUSH_HK_VIDEO_FILE
#define HK_VIDEO_FILE "hk.dat"
#define HK_VIDEO_FRAME_INTERVAL 80

namespace ScheduleServer
{
	class CHKPSDKLivePullTask : public CSDKRecvTask
	{
	public:
		CHKPSDKLivePullTask(SDK_RECV_TASK_INFO& task_info);

		virtual SS_Error run();

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
		//static int32_t DPSDK_CALLTYPE MediaDataCallback(int32_t nPDLLHandle, int32_t nSeq, int32_t nMediaType, const char* szNodeId, int32_t nParamVal, char* szData, int32_t nDataLen, void* pUserParam);
		static void __stdcall StreamCallback(int handle,const char* data,int size,void *pUser);

	private:
		string _ip;
		string _port;
		string _username;
		string _password;
		string _camera_id;
		string _channel;

		int _seq;

	private:
#ifdef PUSH_HK_VIDEO_FILE	
		FILE* _hk_video_file;
		bool _start_write;
		unsigned long _next_fetch_video_frame_timestamp;
		unsigned char _packet[65535];
#endif
		bool _got_sps_pps;
	};
}

#endif  //_HKP_SDK_LIVE_PULL_TASK_H_      
