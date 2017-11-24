// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "GeneralDef.h"
#include "ScheduleServer.h"
#include "RegisterThread.h"
#include "TimeConsuming.h"
#include "resource.h"

using namespace ScheduleServer;

void CRegisterThread::entry()
{
	//unsigned long start = timeGetTime();//本次线程循环执行起始时间

	//15秒记录一次UA统计信息
	if(15000 <= (::timeGetTime() - _latest_ua_statistics_timestamp))
	{
		_latest_ua_statistics_timestamp = ::timeGetTime();

		SINGLETON(CScheduleServer).ua_statistics();
	}

	//5秒更新一次音视频统计显示
	/*if(5000 <= (::timeGetTime() - _latest_show_statistics_timestamp))
	{
		_latest_show_statistics_timestamp = ::timeGetTime();
		
		CUserAgent* ua = SINGLETON(CScheduleServer).fetch_ua(SINGLETON(CScheduleServer).get_playing_ua_id());

		if(NULL != ua)
		{
			//音频统计
			string statistics = "音频丢包率: " + MiscTools::parse_type_to_string<double>(ua->_audio_statistics->get_packet_lost_rate());
			statistics += "%, 间隔: " + MiscTools::parse_type_to_string<double>(ua->_audio_statistics->get_averate_packet_timestamp_interval());
			statistics += "ms, 相对延时: " + MiscTools::parse_type_to_string<double>(ua->_audio_statistics->get_packet_delay());
			statistics += "ms, 码率: " + MiscTools::parse_type_to_string<double>(ua->_audio_statistics->get_bitrate());
			statistics += "kbps";

			//AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_AUDIO_STATISTICS)->SetWindowText(statistics.c_str());

			//视频统计
			statistics += "\n\r视频丢包率: " + MiscTools::parse_type_to_string<double>(ua->_video_statistics->get_packet_lost_rate());
			statistics += "%, 间隔: " + MiscTools::parse_type_to_string<double>(ua->_video_statistics->get_averate_packet_timestamp_interval());
			statistics += "ms, 相对延迟: " + MiscTools::parse_type_to_string<double>(ua->_video_statistics->get_packet_delay());
			statistics += "ms, 码率: " + MiscTools::parse_type_to_string<double>(ua->_video_statistics->get_bitrate());
			statistics += "kbps";

			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_SIP_STATISTICS)->SetWindowText(statistics.c_str());			
		}
	}*/

	Sleep(3000);
}

void CRegisterThread::on_start()
{
}

void CRegisterThread::on_close()
{
}
