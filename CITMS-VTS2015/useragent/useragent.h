// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _USER_AGENT_H_
#define _USER_AGENT_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#define SEND_NAT_PROBE_MSG WM_USER + 100

#include "GeneralDef.h"
#include "Locker.h"
#include "MediaData.h"
#include "WUACodec.h"
#include "JRTPSession.h"
#include "RTMPSession.h"

namespace ScheduleServer
{
	//UA类型
	typedef enum
	{
		UA_MobilePhone = 0,//手机
		UA_Radio,//350M手台
		UA_FixedPhone,//固话
		UA_Unknown//未知
	}
	USER_AGENT_TYPE;

	//UA信息
	typedef struct tagUSER_AGENT_INFO
	{
		unsigned long		id;//UA的ID，如UA为手机则为SIM卡的IMSI号
		USER_AGENT_TYPE		type;//类型
		unsigned long		video_frame_interval;

		tagUSER_AGENT_INFO() :
		id(0), type(UA_MobilePhone), video_frame_interval(0)
		{
		}

		~tagUSER_AGENT_INFO()
		{
		}

	}
	USER_AGENT_INFO, *USER_AGENT_INFO_PTR;

	class CUserAgent
	{
	public:
		CUserAgent(USER_AGENT_INFO& info);
		~CUserAgent();

	private:
		CAudioCodec* _audio_codec;
	
		unsigned char _aac_packet[1024];

	public:
		//采集媒体数据////////////////////////////////////////////////////////////////////////
		//插入本地采样的音频帧
		SS_Error add_sample_audio_packet(const char* frame, const unsigned long& length);		

		//取最早的音频帧
		AUDIO_PACKET_PTR fetch_sample_audio_packet();

		//插入本地采样视频数据包
		int input_video_packet(unsigned char* packet, unsigned long length);
		
		SS_Error add_sample_video_packet(VIDEO_PACKET_PTR packet);

		SS_Error add_dev_video_frame(VIDEO_FRAME_PTR frame);
		
		VIDEO_PACKET_PTR fetch_sample_video_packet();

		VIDEO_FRAME_PTR fetch_dev_video_frame();

		SS_Error add_dev_video_packet(unsigned char* packet, unsigned long length);
		VIDEO_PACKET_PTR fetch_dev_video_packet();

	private:
		//删除所有音频数据包
		SS_Error remove_all_audio_packet();
		//删除所有视频数据包
		SS_Error remove_all_video_packet();

	private:
		//采样语音帧队列
		std::list<AUDIO_PACKET_PTR> _sample_audio_packet_list;
		CSSMutex _sample_audio_packet_list_mutex;

		//视频数据包队列
		std::list<VIDEO_PACKET_PTR> _sample_video_packet_list;
		CSSMutex _sample_video_packet_list_mutex;

		std::list<VIDEO_PACKET_PTR> _dev_video_packet_list;
		CSSMutex _dev_video_packet_list_mutex;

		std::list<VIDEO_FRAME_PTR> _dev_video_frame_list;
		CSSMutex _dev_video_frame_list_mutex;

	public:
		//UA信息
		USER_AGENT_INFO _info;
		//取消对_info访问加锁 CSSMutex _info_mutex;

		volatile bool _start_hls;

	protected:
		static const unsigned long _max_audio_packet_num;//音频数据包队列最大长度
		static const unsigned long _max_video_packet_num;//视频数据包队列最大长度

	//////////////////////////////////////////////////////////////////////////
	private:
		bool _first_video_packet;
		unsigned long _first_video_timestamp;

		bool _first_audio_packet;
		unsigned long _first_audio_timestamp;

	public:

		int send_audio();
		int send_video();

	private:
		static CSSMutex _rtmp_send_mutex;

	private://video
		//编码器相关设置
		//long _x264_handle;  //定义一路编码器
		//unsigned char _stream_buf[655360];  //编码后一帧码流的缓存,注意数组的尺寸不能太小
		//int _nal_len[1000]; //定义数据包尺寸存储数组,注意数组的尺寸不能太小

	public:
		static int _VIDEO_WIDTH;
		static int _VIDEO_HEIGHT;
		static int _VIDEO_SAMPLE_INTERVAL;

	private:
#ifdef PUSH_VIDEO_FILE	
		FILE* _push_video_file;
#endif
		unsigned long _next_fetch_video_frame_timestamp;
	};
}

#endif//_EVENT_DATA_H_