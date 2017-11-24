// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _RTSP_RECV_TASK_H_
#define _RTSP_RECV_TASK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "Task.h"
#include "time.h"
#include "ScheduleServer.h"

#define __STDC_CONSTANT_MACROS
#define	__STDC_LIMIT_MACROS
#define UINT_LEAST64_MAX UINT64_MAX
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus 
}
#endif

namespace ScheduleServer
{
	typedef enum
	{
		RTSPRecvTaskBegin = 0,
		RTSPRecvTaskRTSP_Connected,
		RTSPRecvTaskRTSP_RECV,
		RTSPRecvTask_Done,
		RTSPRecvTask_Pause
	}
	RTSP_RECV_TASK_STATUS;

	typedef struct tagRTSP_RECV_TASK_INFO
	{
		unsigned long	task_id;//任务ID
		unsigned long	ua_id;
		unsigned long	create_timestamp;//创建该任务的时戳
		string			rtsp_url;

		tagRTSP_RECV_TASK_INFO() : task_id(0), ua_id(0), create_timestamp(::time(NULL))
		{
			rtsp_url = "";
		}

		~tagRTSP_RECV_TASK_INFO()
		{
		}

		void clear()
		{

		}
	}
	RTSP_RECV_TASK_INFO;

	class CRTSPRecvTask : public CTask
	{
	public:
		CRTSPRecvTask(RTSP_RECV_TASK_INFO& task_info);
		virtual ~CRTSPRecvTask();

		virtual SS_Error run();

	protected:
		virtual SS_Error on_done();

		virtual SS_Error on_exception();

	public:
		//int init();

	protected:
		/*volatile */RTSP_RECV_TASK_STATUS _status;

		RTSP_RECV_TASK_INFO _task_info;

	private:
		bool _start_push;
		bool _start_recv;
		//unsigned long _last_frame_timestamp;

		unsigned long _last_packet_timestamp;

		AVFormatContext	*pFormatCtx;
		AVCodecContext	*pCodecCtx;
		AVCodec			*pCodec;
		AVPacket		*packet;
		int				video_index;
		//int ret, got_picture;

		unsigned char _dh_video_frame[65536 * 10];
		NAL_TYPE _last_dh_video_type;
		int _dh_video_frame_length;

		void add_dh_video_frame(unsigned char* frame, int length, unsigned long timestamp)
		{
			if(length && NULL != _ua)
			{
				int offset = 0;
				NAL_TYPE type = CRTMPSession::get_video_packet_type(frame, length, offset);
				if(NAL_INVALID == type)
					return;

				VIDEO_PACKET_PTR packet_ptr;

				CMemPool::malloc_video_packet(packet_ptr, length);

				packet_ptr.packet->ua_id = _task_info.ua_id;
				packet_ptr.packet->sequence = 0;
				packet_ptr.packet->timestamp = timestamp;
				packet_ptr.packet->payload_size = length;//净荷长度
				packet_ptr.packet->mark = true;
				memcpy(packet_ptr.packet->payload, frame, length);

				//TRACE("\nP1 %d", _video_frame_length);
				if(SS_NoErr != _ua->add_sample_video_packet(packet_ptr))//for sps and pps
				{
					CMemPool::free_video_packet(packet_ptr);
				}
			}
		}

		CUserAgent* _ua;

	private:
		int init_rtsp();
		void recv_frame();

	private:
		bool _initialized;

	public:
		bool is_initialized() { return _initialized; }

		virtual void shutdown() { _status = RTSPRecvTask_Done; }

		void pause();

		void restart(string& url);

	private:
		HANDLE _wait_for_pause_handle;

	private:
		static map<DWORD, unsigned long> _latest_packet_timestamp;
		static int rtsp_interrupt_callback(void* ctx);
	};
}

#endif  //_RTSP_RECV_TASK_H_      
