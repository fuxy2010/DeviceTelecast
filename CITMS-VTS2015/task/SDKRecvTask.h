// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _SDK_RECV_TASK_H_
#define _SDK_RECV_TASK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "Task.h"
#include "time.h"
#include "ScheduleServer.h"
#include "sps_decode.h"

namespace ScheduleServer
{
	typedef enum
	{
		DSSSDK_DH = 1,
		IVMS8600_HK = 2,
		IVMS8600_DH = 3,
		UNKNOWN = -1
	}
	DEV_TYPE;

	typedef enum
	{
		SDKRecvTaskBegin = 0,
		SDKRecvTask_Connected,
		SDKRecvTask_RECV,
		SDKRecvTask_Done,
		SDKRecvTask_Pause
	}
	SDK_RECV_TASK_STATUS;

	typedef struct tagSDK_RECV_TASK_INFO
	{
		unsigned long	task_id;//任务ID
		unsigned long	ua_id;
		unsigned long	create_timestamp;//创建该任务的时戳
		string			sdk_url;//dsssdk://[user]/[passwd]@<ip>:<port>/params?cameraid=1000004&channel=0
		std::map<string, string> context;

		tagSDK_RECV_TASK_INFO() : task_id(0), ua_id(0), create_timestamp(::time(NULL))
		{
			sdk_url = "";
			context.clear();
		}

		~tagSDK_RECV_TASK_INFO()
		{
		}

		void clear()
		{

		}
	}
	SDK_RECV_TASK_INFO;

	class CSDKRecvTask : public CTask
	{
	public:
		CSDKRecvTask(SDK_RECV_TASK_INFO& task_info);
		virtual ~CSDKRecvTask();

		virtual SS_Error run();

	protected:
		virtual SS_Error on_done();

		virtual SS_Error on_exception();

	public:
		//int init();

	protected:
		/*volatile */SDK_RECV_TASK_STATUS _status;

		SDK_RECV_TASK_INFO _task_info;
		
	protected:
		bool _start_push;
		bool _start_recv;
		//unsigned long _last_frame_timestamp;

		unsigned long _last_packet_timestamp;

		unsigned char* _video_frame;//unsigned char _video_frame[65536 * 100];
		NAL_TYPE _last_video_type;
		int _video_frame_length;

		void add_video_frame(unsigned char* frame, int length, unsigned long timestamp)
		{
			if(length && NULL != _ua)
			{
				int offset = 0;
				NAL_TYPE type = CRTMPSession::get_video_packet_type(frame, length, offset);
				if(NAL_INVALID == type)
					return;

				/*VIDEO_PACKET_PTR packet_ptr;

				CMemPool::malloc_video_packet(packet_ptr);

				packet_ptr.packet->ua_id = _task_info.ua_id;
				packet_ptr.packet->sequence = 0;
				packet_ptr.packet->timestamp = timestamp;
				packet_ptr.packet->payload_size = length;//净荷长度
				packet_ptr.packet->mark = true;
				memcpy(packet_ptr.packet->payload, frame, length);

				//TRACE("\n++++++++++++++++++ <AF %d %d>", type, length);

				//TRACE("\nP1 %d", _video_frame_length);
				if(SS_NoErr != _ua->add_sample_video_packet(packet_ptr))//for sps and pps
				{
					CMemPool::free_video_packet(packet_ptr);
				}*/

				VIDEO_FRAME_PTR frame_ptr;

				CMemPool::malloc_video_frame(frame_ptr, length);

				frame_ptr.frame->ua_id = _task_info.ua_id;
				frame_ptr.frame->sequence = 0;
				frame_ptr.frame->timestamp = timestamp;
				frame_ptr.frame->payload_size = length;//净荷长度
				frame_ptr.frame->type = type;
				memcpy(frame_ptr.frame->payload, frame, length);

				//TRACE("\n++++++++++++++++++ <AF %d %d>", type, length);

				//TRACE("\nP1 %d", _video_frame_length);
				_ua->_info.video_frame_interval = _frame_interval;
				if (SS_NoErr != _ua->add_dev_video_frame(frame_ptr))//for sps and pps
				{
					CMemPool::free_video_frame(frame_ptr);
				}
			}
		}

		CUserAgent* _ua;

	protected:
		virtual int connect_device() = 0;
		virtual int disconnect_device() = 0;
		//virtual void recv_frame() = 0;

	protected:
		bool _initialized;

	public:
		bool is_initialized() { return _initialized; }

		void shutdown() { _status = SDKRecvTask_Done; }

		void pause();

		void restart(string& url);

		void on_recv_frame(unsigned char* data, int length, bool sps_pps_sei_idr);

	private:
		HANDLE _wait_for_pause_handle;

	private:
		//static map<DWORD, unsigned long> _latest_packet_timestamp;
		//static int rtsp_interrupt_callback(void* ctx);

	public:
		static bool _hik_platform_sdk_initialized;
		static bool _hik_net_sdk_initialized;

	protected:
		//key: _ip + _username + _password + _port
		static std::map<string, int32_t> _hk_sdk_handlers;
		static CSSMutex _hk_sdk_handlers_mutex;

	protected:
		DEV_TYPE _dev_type;
		unsigned long _first_frame_timestamp;
		unsigned long _last_frame_timestamp;
		unsigned long _frame_interval;

		unsigned long t1;
		unsigned long t2;

	protected:
		bool is_pts(unsigned char* data)
		{
			if (0 == data[0] && 0 == data[1] && 1 == data[2])
			{
				if (0xe0 == data[3] || 0xba == data[3] || 0xbc == data[3] || 0xc0 == data[3] || 0xbd == data[3])
					return true;
				else
					return false;
			}

			return false;
		}

		bool is_video_pts(unsigned char* data)
		{
			return (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xe0 == data[3]);
		}

		bool is_ba(unsigned char* data)
		{
			return (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xba == data[3]);
		}

		bool is_bb(unsigned char* data)
		{
			return (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xbb == data[3]);
		}

		bool is_bc(unsigned char* data)
		{
			return (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xbc == data[3]);
		}

		bool is_bd(unsigned char* data)
		{
			return (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xbd == data[3]);
		}

		bool is_e0(unsigned char* data)
		{
			return (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xe0 == data[3]);
		}

		bool is_video_slice(unsigned char* data)
		{
			return (0 == data[0] && 0 == data[1] && 0 == data[2] && 1 == data[3]);
		}

	protected:
		int _connect_ret;

	public:
		int get_connect_ret() { return _connect_ret; }

	};
}

#endif  //_SDK_RECV_TASK_H_      
