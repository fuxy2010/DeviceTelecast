// **********************************************************************
// ����: ������
// �汾: 1.0
// ����: 2011-01 ~ 2011-11
// �޸���ʷ��¼: 
// ����, ����, �������
// **********************************************************************
#ifndef _HKP_SDK_VOD_PULL_TASK_H_
#define _HKP_SDK_VOD_PULL_TASK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "SDKRecvTask.h"
#include "time.h"
#include "ScheduleServer.h"

#include "tmcp_sdk_defines.h"
#include "tmcp_interface_sdk.h"

//����ƽ̨SDK
#pragma comment(lib, "../dev/hktmcp/tmcp_interface_sdk.lib")
#pragma comment(lib, "WS2_32.lib")

//#define PUSH_HK_VIDEO_FILE
//#define HK_VIDEO_FILE "ivms_vod.dat"
//#define HK_VIDEO_FILE "ivms_vod2.dat"
//#define HK_VIDEO_FILE "dj.dat"
//#define HK_VIDEO_FILE "vod.mp4"
#define HK_VIDEO_FILE "vod2.mp4"
//#define HK_VIDEO_FILE "20170719104742.mp4"
//#define HK_VIDEO_FILE "kk.dat"
#define HK_VIDEO_FRAME_INTERVAL 80

#define GET_VOD_BY_DOWNLOADING

#define VOD_BUFFER_LEN	(65536 * 100)

namespace ScheduleServer
{
	class CHKPSDKVodPullTask : public CSDKRecvTask
	{
	public:
		CHKPSDKVodPullTask(SDK_RECV_TASK_INFO& task_info);
		virtual ~CHKPSDKVodPullTask();

		virtual SS_Error run();

	protected:
		virtual int connect_device();
		virtual int disconnect_device();
		virtual SS_Error on_done();

	private:
		int32_t _sdk_handler;//static int32_t _sdk_handler;
		static bool _sdk_initialized;
		static CSSMutex _sdk_init_mutex;

		static int init_sdk();

		unsigned char* _frame;//unsigned char _frame[65536 * 100];
		NAL_TYPE _last_frame_type;
		int _frame_pos;

		unsigned char* _pts;//unsigned char _pts[65536 * 100];
		int _pts_pos;//д���ǩ

	private:
		CSSMutex _on_recv_packet_mutext;
		void on_recv_packet(unsigned char* data, int len);

		void on_recv_packet_dh(unsigned char* data, int len);

		void on_recv_packet_hk(unsigned char* data, int len);
		void packet_combine(unsigned char* data, int len);
		void pakcet_segment(unsigned char* data, int len);
		//static int32_t DPSDK_CALLTYPE MediaDataCallback(int32_t nPDLLHandle, int32_t nSeq, int32_t nMediaType, const char* szNodeId, int32_t nParamVal, char* szData, int32_t nDataLen, void* pUserParam);
		static void __stdcall StreamCallback(int handle, const char* data, int size, void *pUser);

		void add_recv_packet(unsigned char* data, int len);

	private:
		string _ip;
		string _port;
		string _username;
		string _password;
		string _camera_id;
		string _condition;
		string _channel;
		unsigned long long _begin_time;
		unsigned long long _end_time;

		string _fps;

		int _seq;

	private:
#ifdef PUSH_HK_VIDEO_FILE	
		FILE* _hk_video_file;
		bool _start_write;		
#endif
		unsigned char _packet[65535];
		unsigned long _next_fetch_video_frame_timestamp;

		bool _got_sps_pps;
		bool _got_first_video_packet;

	private:
		//static CHKPSDKVodPullTask* _global_hk_vod_pull_task;

	private:
#ifdef GET_VOD_BY_DOWNLOADING
		FILE* _vod_file;
		string _vod_file_path;
		int _vod_file_offset;
		long _download_percent;
#endif

	};
}

#endif  //_HKP_SDK_VOD_PULL_TASK_H_      