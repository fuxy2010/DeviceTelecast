// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _MEDIA_DATA_H_
#define _MEDIA_DATA_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

//定义USE_BOOST_POOL则使用boost内存池，否则不使用但在Debug版中增加内存泄漏检测
#ifndef	USE_BOOST_POOL
//#define USE_BOOST_POOL
#endif

#ifdef	USE_BOOST_POOL
#include <boost/pool/singleton_pool.hpp>
#endif

//每个音频RTP包所含语音帧个数
#define AUDIO_FRAME_PER_PACKET			3
//语音帧长度(单位short）
#define AUDIO_FRAME_LENGTH				160

//RTP包头长度
#define RTP_HEADER_LENGTH				12

//音频数据编码后RTP包最大长度(含RTP包头长度12）,单位字节
//#define MAX_AUDIO_PACKET_SIZE_BYTE		(13 * AUDIO_FRAME_PER_PACKET + RTP_HEADER_LENGTH)
//为兼容手台及固话语音RTP包(该类型原始语音包编码后长度不变）
//#define MAX_AUDIO_PACKET_SIZE_BYTE		(AUDIO_FRAME_LENGTH * AUDIO_FRAME_PER_PACKET * sizeof(WORD) + RTP_HEADER_LENGTH)
//一次分配到位
#define MAX_AUDIO_PACKET_SIZE_BYTE		1024

//音频RTP包解码后原始语音帧(共AUDIO_FRAME_PER_PACKET个)数据包长度,单位字
//#define MAX_RAW_AUDIO_FRAME_SIZE_WORD	(AUDIO_FRAME_LENGTH * AUDIO_FRAME_PER_PACKET)
//一次分配到位
#define MAX_RAW_AUDIO_FRAME_SIZE_WORD	512

//视频RTP数据包最大长度(含RTP包头长度12）,单位字节
#define MAX_VIDEO_PACKET_SIZE_BYTE		(65536 * 10)

//SIP消息数据包最大长度单位字节
#define MAX_SIP_MESSAGE_SIZE_BYTE		1400

namespace ScheduleServer
{
	//////////////////////////////////////////////////////////////////////////
	//音频RTP数据包
	typedef struct tagAUDIO_PACKET
    {
		unsigned long	ua_id;//发送源UA ID
		unsigned short	payload_size;//数据包实际长度（含RTP包头）
		unsigned long	sequence;//RTP包序列号
		unsigned long	timestamp;//编码打包时戳
		int				energy;//语音能量

		//音频RTP数据包（含RTP包头）
		unsigned char	payload[MAX_AUDIO_PACKET_SIZE_BYTE];

		tagAUDIO_PACKET() : ua_id(0), payload_size(0), sequence(0), timestamp(0), energy(0)
		{
			//如用boost内存池方式，该方法不被调用，已在malloc中初始化
			::memset(payload, 0, sizeof(payload));
		}

		~tagAUDIO_PACKET()
		{
		}

		bool operator < (tagAUDIO_PACKET const& _packet) const
		{
			if(timestamp < _packet.timestamp)
				return true;
			else if(timestamp == _packet.timestamp)
				return sequence < _packet.sequence;

			return false;
		}
    }
	AUDIO_PACKET;
	
	typedef struct tagAUDIO_PACKET_PTR
	{
		AUDIO_PACKET* packet;

		tagAUDIO_PACKET_PTR() : packet(NULL)
		{
			//万不可将创建packet_ptr放在此中 cout << "\ncreate AUDIO_PACKET_PTR";
		}

		~tagAUDIO_PACKET_PTR()
		{
			//cout << "\ndelete AUDIO_PACKET_PTR";
		}

		bool operator < (tagAUDIO_PACKET_PTR const& _packet_ptr) const
		{
			if(NULL == packet)
				return true;

			if(NULL == _packet_ptr.packet)
				return false;

			return (*packet < *(_packet_ptr.packet));
		}
	}
	AUDIO_PACKET_PTR;

	//////////////////////////////////////////////////////////////////////////
	//原始语音帧
	typedef struct tagRAW_AUDIO_FRAME
	{
		unsigned long	ua_id;//发送源UA ID
		int				energy;//语音帧能量
		short			payload[MAX_RAW_AUDIO_FRAME_SIZE_WORD];//语音帧存储区
		bool			available;
		unsigned long	sequence;//RTP包序列号
		unsigned long	timestamp;//编码打包时戳

		tagRAW_AUDIO_FRAME() : ua_id(0), energy(0), available(false)
		{
			//如用boost内存池方式，该方法不被调用，已在malloc中初始化
			::memset(payload, 0, sizeof(payload));
		}

		~tagRAW_AUDIO_FRAME()
		{
		}

		bool operator < (tagRAW_AUDIO_FRAME const& _frame) const
		{
			if(timestamp < _frame.timestamp)
				return true;
			else if(timestamp == _frame.timestamp)
				return sequence < _frame.sequence;

			return false;
		}
	}
	RAW_AUDIO_FRAME;//, *RAW_AUDIO_FRAME_PTR;

	typedef struct tagRAW_AUDIO_FRAME_PTR
	{
		RAW_AUDIO_FRAME* frame;

		tagRAW_AUDIO_FRAME_PTR() : frame(NULL)
		{
		}

		~tagRAW_AUDIO_FRAME_PTR()
		{
		}

		bool operator < (tagRAW_AUDIO_FRAME_PTR const& _frame_ptr) const
		{
			if(NULL == frame)
				return true;

			if(NULL == _frame_ptr.frame)
				return false;

			return (*frame < *(_frame_ptr.frame));
		}
	}
	RAW_AUDIO_FRAME_PTR;

	//////////////////////////////////////////////////////////////////////////
	//视频RTP数据包信息
	typedef struct tagVIDEO_PACKET
	{
		DWORD			ua_id;//发送源UA ID
		//WORD			payload_size;//数据包实际大小（含RTP包头)
		DWORD			payload_size;//数据包实际大小（含RTP包头)
		DWORD			sequence;//RTP包序列号
		unsigned long	timestamp;//RTP包时戳
		bool			mark;
		unsigned short	frame;//帧类型0-未知, 1-I帧，2-非I帧

		//视频RTP数据包（含RTP包头）
		BYTE payload[MAX_VIDEO_PACKET_SIZE_BYTE];

		tagVIDEO_PACKET() : ua_id(0), payload_size(0), sequence(0), timestamp(0), mark(false), frame(0)
		{
			//如用boost内存池方式，该方法不被调用，已在malloc中初始化
			::memset(payload, 0, sizeof(payload));
		}

		~tagVIDEO_PACKET()
		{
		}

		bool operator < (tagVIDEO_PACKET const& _packet) const
		{
			if(timestamp < _packet.timestamp)
				return true;
			else if(timestamp == _packet.timestamp)
			{
				return sequence < _packet.sequence;
			}

			return false;
		}
	}
	VIDEO_PACKET;

	//视频RTP数据包
	typedef struct tagVIDEO_PACKET_PTR
	{
		VIDEO_PACKET* packet;

		tagVIDEO_PACKET_PTR() : packet(NULL)
		{
		}

		~tagVIDEO_PACKET_PTR()
		{
		}

		bool operator < (tagVIDEO_PACKET_PTR const& _packet_ptr) const
		{
			if(NULL == packet)
				return true;

			if(NULL == _packet_ptr.packet)
				return false;

			return (*packet < *(_packet_ptr.packet));
		}
	}
	VIDEO_PACKET_PTR;

	//////////////////////////////////////////////////////////////////////////
	//SIP消息数据包信息
	typedef struct tagSIP_MESSAGE
	{
		unsigned long	message_size;//数据包实际大小
		unsigned long	timestamp;//RTP包时戳
		unsigned long	interval;//如为待发送的SIP消息则为发送后Sleep的间隔（毫秒），如为收到的SIP消息则为0

		//SIP消息数据包
		BYTE payload[MAX_SIP_MESSAGE_SIZE_BYTE];

		tagSIP_MESSAGE() : message_size(0), timestamp(0), interval(0)
		{
			//如用boost内存池方式，该方法不被调用，已在malloc中初始化
			::memset(payload, 0, sizeof(payload));
		}

		~tagSIP_MESSAGE()
		{
		}

		bool operator < (tagSIP_MESSAGE const& _message) const
		{
			return (timestamp < _message.timestamp);
		}
	}
	SIP_MESSAGE;

	//SIP消息数据包
	typedef struct tagSIP_MESSAGE_PTR
	{
		SIP_MESSAGE* message;

		tagSIP_MESSAGE_PTR() : message(NULL)
		{
		}

		~tagSIP_MESSAGE_PTR()
		{
		}

		bool operator < (tagSIP_MESSAGE_PTR const& _message_ptr) const
		{
			if(NULL == message)
				return true;

			if(NULL == _message_ptr.message)
				return false;

			return (*message < *(_message_ptr.message));
		}
	}
	SIP_MESSAGE_PTR;

#ifdef USE_BOOST_POOL

	//////////////////////////////////////////////////////////////////////////
	//音频RTP数据包内存池
	typedef struct tagSINGLETON_AUDIO_PACKET_POOL {} SINGLETON_AUDIO_PACKET_POOL;
	typedef boost::singleton_pool<SINGLETON_AUDIO_PACKET_POOL, sizeof(AUDIO_PACKET)> audio_packet_pool;

	//原始语音帧内存池
	typedef struct tagSINGLETON_RAW_AUDIO_FRAME_POOL {} SINGLETON_RAW_AUDIO_FRAME_POOL;
	typedef boost::singleton_pool<SINGLETON_RAW_AUDIO_FRAME_POOL, sizeof(RAW_AUDIO_FRAME)> raw_audio_frame_pool;

	//视频RTP数据包内存池
	typedef struct tagSINGLETON_VIDEO_PACKET_POOL {} SINGLETON_VIDEO_PACKET_POOL;
	typedef boost::singleton_pool<SINGLETON_VIDEO_PACKET_POOL, sizeof(VIDEO_PACKET)> video_packet_pool;

	//SIP消息数据包内存池
	typedef struct tagSINGLETON_SIP_MESSAGE_POOL {} SINGLETON_SIP_MESSAGE_POOL;
	typedef boost::singleton_pool<SINGLETON_SIP_MESSAGE_POOL, sizeof(SIP_MESSAGE)> sip_message_pool;

	class CMemPool
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		static void malloc_audio_packet(AUDIO_PACKET_PTR& packet_ptr)
		{
			if(NULL != packet_ptr.packet)
				audio_packet_pool::free(packet_ptr.packet);

			packet_ptr.packet = (AUDIO_PACKET*)audio_packet_pool::malloc();

			::memset(packet_ptr.packet, 0, sizeof(AUDIO_PACKET));
		}

		static void free_audio_packet(AUDIO_PACKET_PTR& packet_ptr)
		{
			if(NULL == packet_ptr.packet)
				return;

			audio_packet_pool::free(packet_ptr.packet);
			packet_ptr.packet = NULL;
		}

		//////////////////////////////////////////////////////////////////////////
		static void malloc_raw_audio_frame(RAW_AUDIO_FRAME_PTR& frame_ptr)
		{
			if(NULL != frame_ptr.frame)
				raw_audio_frame_pool::free(frame_ptr.frame);

			frame_ptr.frame = (RAW_AUDIO_FRAME*)raw_audio_frame_pool::malloc();

			::memset(frame_ptr.frame, 0, sizeof(RAW_AUDIO_FRAME));

			frame_ptr.frame->available = false;
		}

		static void free_raw_audio_frame(RAW_AUDIO_FRAME_PTR& frame_ptr)
		{
			if(NULL == frame_ptr.frame)
				return;

			raw_audio_frame_pool::free(frame_ptr.frame);
			frame_ptr.frame = NULL;
		}

		//////////////////////////////////////////////////////////////////////////
		static void malloc_video_packet(VIDEO_PACKET_PTR& packet_ptr)
		{
			if(NULL != packet_ptr.packet)
				video_packet_pool::free(packet_ptr.packet);

			packet_ptr.packet = (VIDEO_PACKET*)video_packet_pool::malloc();

			::memset(packet_ptr.packet, 0, sizeof(VIDEO_PACKET));
		}

		static void free_video_packet(VIDEO_PACKET_PTR& packet_ptr)
		{
			if(NULL == packet_ptr.packet)
				return;

			video_packet_pool::free(packet_ptr.packet);
			packet_ptr.packet = NULL;
		}

		//////////////////////////////////////////////////////////////////////////
		static void malloc_sip_message(SIP_MESSAGE_PTR& message_ptr)
		{
			if(NULL != message_ptr.message)
				sip_message_pool::free(message_ptr.message);

			message_ptr.message = (SIP_MESSAGE*)sip_message_pool::malloc();

			::memset(message_ptr.message, 0, sizeof(SIP_MESSAGE));
		}

		static void free_sip_message(SIP_MESSAGE_PTR& message_ptr)
		{
			if(NULL == message_ptr.message)
				return;

			sip_message_pool::free(message_ptr.message);
			message_ptr.message = NULL;
		}
	};

#else//#ifdef USE_BOOST_POOL

#ifdef _DEBUG
	/*for audio sampler
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>

	#ifndef DBG_NEW
		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
		#define new DBG_NEW
	#endif*/
#endif //_DEBUG

	class CMemPool
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		static void malloc_audio_packet(AUDIO_PACKET_PTR& packet_ptr)
		{
			if(NULL != packet_ptr.packet)
				delete packet_ptr.packet;

			packet_ptr.packet = new AUDIO_PACKET;

			::memset(packet_ptr.packet, 0, sizeof(AUDIO_PACKET));
		}

		static void free_audio_packet(AUDIO_PACKET_PTR& packet_ptr)
		{
			if(NULL == packet_ptr.packet)
				return;

			delete packet_ptr.packet;
			packet_ptr.packet = NULL;
		}

		//////////////////////////////////////////////////////////////////////////
		static void malloc_raw_audio_frame(RAW_AUDIO_FRAME_PTR& frame_ptr)
		{
			if(NULL != frame_ptr.frame)
				delete frame_ptr.frame;

			frame_ptr.frame = new RAW_AUDIO_FRAME;

			::memset(frame_ptr.frame, 0, sizeof(RAW_AUDIO_FRAME));
		}

		static void free_raw_audio_frame(RAW_AUDIO_FRAME_PTR& frame_ptr)
		{
			if(NULL == frame_ptr.frame)
				return;

			delete frame_ptr.frame;
			frame_ptr.frame = NULL;
		}

		//////////////////////////////////////////////////////////////////////////
		static void malloc_video_packet(VIDEO_PACKET_PTR& packet_ptr)
		{
			if(NULL != packet_ptr.packet)
				delete packet_ptr.packet;

			packet_ptr.packet = new VIDEO_PACKET;

			::memset(packet_ptr.packet, 0, sizeof(VIDEO_PACKET));
		}

		static void free_video_packet(VIDEO_PACKET_PTR& packet_ptr)
		{
			if(NULL == packet_ptr.packet)
				return;

			delete packet_ptr.packet;
			packet_ptr.packet = NULL;
		}

		//////////////////////////////////////////////////////////////////////////
		static void malloc_sip_message(SIP_MESSAGE_PTR& message_ptr)
		{
			if(NULL != message_ptr.message)
				delete message_ptr.message;

			message_ptr.message = new SIP_MESSAGE;

			::memset(message_ptr.message, 0, sizeof(SIP_MESSAGE));
		}

		static void free_sip_message(SIP_MESSAGE_PTR& message_ptr)
		{
			if(NULL == message_ptr.message)
				return;

			delete message_ptr.message;
			message_ptr.message = NULL;
		}
	};
#endif
}

//语音数据采样间隔（毫秒）
#ifndef	AUDIO_SAMPLING_RATE
#define	AUDIO_SAMPLING_RATE	60
#endif

#endif//_MEDIA_DATA_H_

/*
在RTPRecvSession的回调中创建并malloc AUDIO_PACKET_PTR或AUDIO_PACKET_PTR并复制RTP数据
再将AUDIO_PACKET_PTR或VIDEO_PACKET_PTR复制到对应的CUserAgent的存储队列时，如队列超长，则弹出最早的数据包并free掉

会议中，从每个CUserAgent的音频数据队列中取出一个AUDIO_PACKET_PTR，调用input_audio_packet函数复制给PARTICIPANT的audio_packet_ptr
注意input_audio_packet中会首先free一次audio_packet_ptr，即上次复制给audio_packet_ptr的AUDIO_PACKET_PTR
故在会议控制任务和混音任务无需关注free从CUserAgent取出的数据包
*/