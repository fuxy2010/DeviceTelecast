// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _JRTP_SESSION_H_
#define _JRTP_SESSION_H_

#include <iostream>
#include <string>
#include <vector>

#include "GeneralDef.h"
//#include "rtpsession.h"

typedef struct tagRTPHeader//fym struct RTPHeader
{
#ifdef RTP_BIG_ENDIAN
	unsigned char version:2;
	unsigned char padding:1;
	unsigned char extension:1;
	unsigned char csrccount:4;

	unsigned char marker:1;
	unsigned char payloadtype:7;
#else // little endian
	unsigned char csrccount:4;
	unsigned char extension:1;
	unsigned char padding:1;
	unsigned char version:2;

	unsigned char payloadtype:7;
	unsigned char marker:1;
#endif // RTP_BIG_ENDIAN

	unsigned short sequencenumber;
	unsigned int timestamp;
	unsigned int ssrc;
}
RTPHeader, *RTPHeaderPtr;//fym

namespace ScheduleServer
{
	//RTP包类型
	typedef enum
	{
		AMRNBRTPPacket = 0,//音频包
		PCMRTPPacket,//音频包
		H264RTPPacket,//视频包
		UnknownRTPPacket//未知
	}
	RTP_PACKET_TYPE;

	class CRTPRecvSession//用于接收音视频数据及收发SIP消息
	{
	public:
		CRTPRecvSession(unsigned short port);//port为监听接收数据包的端口
		virtual ~CRTPRecvSession();

	public:
		/*void set_rtp_callback(RTPCallBackFunc func)
		{
		}

		void set_udp_callback(UDPCallBackFunc func)
		{
		}*/

		//用于模拟UA发送SIP消息
		SS_Error send_udp_packet(const string& dest_ip, const unsigned short dest_port, const unsigned char* packet, const unsigned long& length)
		{
			return SS_NoErr;
		}

	private:
		bool _available;
		//RTPSession _rtp_session;

	};

	//////////////////////////////////////////////////////////////////////////
	class CRTPNATSession//用于向处于内网中的UA发送音视频数据
	{
	public:
		CRTPNATSession(unsigned short port);//port为socket绑定端口
		virtual ~CRTPNATSession();

	public:
		/*void set_udp_callback(UDPCallBackFunc func)
		{
		}*/

		//在data的开头生成RTP包头
		//data必须在开头预留12个字节用于存放RTP包头
		//length为data的长度（含包头）
		static SS_Error add_rtp_header(unsigned char* data, const unsigned long& length,
			const unsigned char& payload_type, const bool& mark,
			const unsigned short& sequence, const unsigned long& timestamp, const unsigned long& ssrc);

		//发送RTP包到单一地址
		//packet已包含RTP包头，len包括包头长度
		//发送前len必须校验长度，不能超过1612字节
		SS_Error send_rtp_packet(const struct sockaddr_in& dest_addr, const unsigned char* packet, const unsigned long& length);

		//发送RTP包到多个地址
		//packet已包含RTP包头，len包括包头长度
		//发送前len必须校验长度，不能超过1612字节
		SS_Error send_rtp_packet(const vector<struct sockaddr_in>& dest_addr_vec,
			const unsigned char* packet, const unsigned long& length);

	private:
		bool _available;
		//RTPSession _rtp_session;

	};
}

#endif//_JRTP_SESSION_H_