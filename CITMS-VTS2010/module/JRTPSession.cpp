// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "stdafx.h"
//#include <boost/pool/detail/singleton.hpp>//#include "singleton.h"
//#include "Logger.h"
//#include "ScheduleServer.h"

//#include "rtpsession.h"
//#include "rtpudpv4transmitter.h"
//#include "rtpipv4address.h"
//#include "rtpsessionparams.h"
//#include "rtperrors.h"
//#include "rtpsourcedata.h"
#include "JRTPSession.h"
//#include "rtpstructs.h"

using namespace ScheduleServer;

CRTPRecvSession::CRTPRecvSession(unsigned short port) :
_available(false)
{
	_available = true;
}

CRTPRecvSession::~CRTPRecvSession()
{
}

//////////////////////////////////////////////////////////////////////////
CRTPNATSession::CRTPNATSession(unsigned short port) :
_available(false)
{
	_available = true;

}

CRTPNATSession::~CRTPNATSession()
{
}

SS_Error CRTPNATSession::add_rtp_header(unsigned char* data, const unsigned long& length,
										const unsigned char& payload_type, const bool& mark,
										const unsigned short& sequence, const unsigned long& timestamp, const unsigned long& ssrc)
{
	return SS_NoErr;
}

SS_Error CRTPNATSession::send_rtp_packet(const struct sockaddr_in& dest_addr, const unsigned char* packet, const unsigned long& length)
{
	return SS_NoErr;
}

SS_Error CRTPNATSession::send_rtp_packet(const vector<struct sockaddr_in>& dest_addr_vec,
									const unsigned char* packet, const unsigned long& length)
{
	return SS_NoErr;
}
