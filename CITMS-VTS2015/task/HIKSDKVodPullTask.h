// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _HIK_SDK_VOD_PULL_TASK_H_
#define _HIK_SDK_VOD_PULL_TASK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "SDKRecvTask.h"
#include "time.h"
#include "ScheduleServer.h"

#include "HCNetSDK.h"

//海康平台SDK
#pragma comment(lib, "../dev/hik/HCNetSDK.lib")
#pragma comment(lib, "../dev/hik/PlayCtrl.lib")
#pragma comment(lib, "WS2_32.lib")

//#define PUSH_HK_VIDEO_FILE
#define HK_VIDEO_FILE "hkvod_p.dat"
#define HK_VIDEO_FRAME_INTERVAL 40

namespace ScheduleServer
{
	class CHIKSDKVodPullTask : public CSDKRecvTask
	{
	public:
		CHIKSDKVodPullTask(SDK_RECV_TASK_INFO& task_info);

		virtual SS_Error run();

	protected:
		virtual int connect_device();
		virtual int disconnect_device();
		virtual SS_Error on_done();

	private:
		LONG _sdk_user_id;//static int32_t _sdk_handler;
		LONG _vod_stream_handler;
		static bool _sdk_initialized;
		static CSSMutex _sdk_init_mutex;

		static int init_sdk();

		unsigned char _frame[65536 * 10];
		NAL_TYPE _last_frame_type;
		int _frame_pos;

	private:
		void on_recv_packet(unsigned char* data, int len);
		void packet_combine(unsigned char* data, int len);
		//void pakcet_segment(unsigned char* data, int len);
		static void CALLBACK StreamCallback(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser);

	private:
		string _ip;
		string _port;
		string _username;
		string _password;
		string _channel;
		unsigned long long _begin_time;
		unsigned long long _end_time;
		string _fps;

		int _seq;

	private:
#ifdef PUSH_HK_VIDEO_FILE	
		FILE* _hk_video_file;
		bool _start_write;
		unsigned long _next_fetch_video_frame_timestamp;
		unsigned char _packet[65535];
#endif
		bool _got_sps_pps;

	private:
		unsigned char _packet_buf[512000];
		int _packet_buf_pos;

	};
}

#endif  //_HIK_SDK_VOD_PULL_TASK_H_      
