// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "HIKSDKVodPullTask.h"
#include "MiscTool.h"
#include "TimeConsuming.h"

using namespace ScheduleServer;

//int32_t CHIKSDKLivePullTask::_sdk_handler = -1;
bool CHIKSDKVodPullTask::_sdk_initialized = false;
CSSMutex CHIKSDKVodPullTask::_sdk_init_mutex;

CHIKSDKVodPullTask::CHIKSDKVodPullTask(SDK_RECV_TASK_INFO& task_info):
CSDKRecvTask(task_info)
{
	_connect_ret = connect_device();
	if (!_connect_ret)
	{
		_status = SDKRecvTask_Connected;
		_initialized = true;
	}
	else
	{
		_status = SDKRecvTask_Done;
		_initialized = false;
	}

#ifdef PUSH_HK_VIDEO_FILE
	_hk_video_file = fopen(HK_VIDEO_FILE, "rb");

	if (NULL != _hk_video_file)
		fseek(_hk_video_file, 0, SEEK_SET);

	_start_write = false;

	_next_fetch_video_frame_timestamp = 0;
#endif

	_got_sps_pps = false;

	_sdk_user_id = -1;
}

int CHIKSDKVodPullTask::init_sdk()
{
	CSSLocker lock(&_sdk_init_mutex);

	if (true == CSDKRecvTask::_hik_net_sdk_initialized)
		return 0;

	if (true == _sdk_initialized)
		return 0;

	if (FALSE == NET_DVR_Init()) return -1;

	CSDKRecvTask::_hik_net_sdk_initialized = true;

	_sdk_initialized = true;

	return 0;
}

#include <Poco/URI.h>
#include <Poco/NumberFormatter.h>
#include <Poco/String.h>

int CHIKSDKVodPullTask::connect_device()
{
	_packet_buf_pos = 0;

	SINGLETON(CScheduleServer).reg_ua(_task_info.ua_id, UA_MobilePhone);
	_ua = SINGLETON(CScheduleServer).fetch_ua(_task_info.ua_id);

	if (NULL == _ua)
		return 3;

	if (0 != init_sdk())
		return 2;

	memset(_video_frame, 0, sizeof(_video_frame));
	_video_frame_length = 0;
	_last_video_type = NAL_INVALID;

	memset(_frame, 0, sizeof(_frame));
	_last_frame_type = NAL_INVALID;
	_frame_pos = 0;

	string url = _task_info.sdk_url;
	TRACE("\nSDK: %s", url.c_str());

	int ret = 1;
	{
		//http://127.0.0.1:80/api/startvodplay?connectstring=hiksdk://admin:citms@b7@192.168.0.237:8000/params?channel=2&begintime=1482474300&endtime=1482474397&fps=25
		//hiksdk://admin:citms@b7@192.168.0.237:8000/params?channel=2&begintime=1482474300&endtime=1482474397&fps=25
#if 0		
		url = url.substr(url.find("://") + 3, url.length() - strlen("ivms8600://"));
		//fym/88888888@192.168.0.190:9000/params?cameraid=1000004
		_username = url.substr(0, url.find("/"));
		_password = url.substr(url.find("/") + 1, url.find("@") - url.find("/") - 1);
		_ip = url.substr(url.find("@") + 1, url.find(":") - url.find("@") - 1);
		_port = url.substr(url.find(":") + 1, url.find_last_of("/") - url.find(":") - 1);

		_username = "admin";
		_password = "citms@b7";
		_ip = "192.168.0.237";
		_port = "8000";
		_channel = "2";
#else
		Poco::URI uri(url);

		std::string userInfo = uri.getUserInfo();
		_username = userInfo.substr(0, userInfo.find(':'));

		if (userInfo.length() > _username.length())
		{
			_password = userInfo.substr(_username.length() + 1);
	}

		_ip = uri.getHost();
		_port = MiscTools::parse_string_to_type<unsigned short>(Poco::NumberFormatter::format(uri.getPort()));

		auto paramsVector = uri.getQueryParameters();
		for (auto i = 0; i < paramsVector.size(); ++i)
		{
			std::string key = Poco::toLower(paramsVector[i].first);
			std::string val = paramsVector[i].second;

			if ("channel" == key)
				_channel = val;
			else if ("begintime" == key)
				_begin_time = MiscTools::parse_string_to_type<unsigned long long>(val);
			else if ("endtime" == key)
				_end_time = MiscTools::parse_string_to_type<unsigned long long>(val);
			else if ("fps" == key)
				_fps = val;
		}
#endif

		_vod_stream_handler = -1;

#ifdef PUSH_HK_VIDEO_FILE
		_vod_stream_handler = 1;
		return 0;
#endif

		{
			/*CSSLocker lock(&_hk_sdk_handlers_mutex);

			string key = _ip + _username + _password + _port;

			if (_hk_sdk_handlers.end() == _hk_sdk_handlers.find(key))
			_sdk_handler = -1;
			else
			_sdk_handler = _hk_sdk_handlers[key];

			if (0 >= _user_id)
			{
			_sdk_handler = Plat_LoginCMS(_ip.c_str(), _username.c_str(), _password.c_str(), _port.c_str());

			_hk_sdk_handlers[key] = _sdk_handler;
			}*/

			NET_DVR_DEVICEINFO_V30 devInfo;
			_sdk_user_id = NET_DVR_Login_V30(const_cast<char*>(_ip.c_str()), MiscTools::parse_string_to_type<WORD>(_port), const_cast<char*>(_username.c_str()), const_cast<char*>(_password.c_str()), &devInfo);
		}

		if (0 > _sdk_user_id)
			return 5;

		NET_DVR_VOD_PARA struVodPara = { 0 };
		struVodPara.dwSize = sizeof(struVodPara);
		struVodPara.struIDInfo.dwChannel = 2; //通道号
		struVodPara.hWnd = NULL; //回放窗口
		struVodPara.struBeginTime.dwYear = 2017; //开始时间
		struVodPara.struBeginTime.dwMonth = 6;
		struVodPara.struBeginTime.dwDay = 14;
		struVodPara.struBeginTime.dwHour = 9;
		struVodPara.struBeginTime.dwMinute = 0;
		struVodPara.struBeginTime.dwSecond = 0;
		struVodPara.struEndTime.dwYear = 2017; //结束时间
		struVodPara.struEndTime.dwMonth = 6;
		struVodPara.struEndTime.dwDay = 30;
		struVodPara.struEndTime.dwHour = 10;
		struVodPara.struEndTime.dwMinute = 7;
		struVodPara.struEndTime.dwSecond = 0;
		//---------------------------------------
		//按时间回放
		_vod_stream_handler = NET_DVR_PlayBackByTime_V40(_sdk_user_id, &struVodPara);
		if (0 > _vod_stream_handler)
		{
			return 6;
		}

		if (FALSE == NET_DVR_SetPlayDataCallBack_V40(_vod_stream_handler, CHIKSDKVodPullTask::StreamCallback, (void*)this))
		{
			return 7;
		}

		if (FALSE == NET_DVR_PlayBackControl_V40(_vod_stream_handler, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL))
		{
			ret = 8;
		}
		else
			ret = 0;
	}

	return ret;
}

int CHIKSDKVodPullTask::disconnect_device()
{
	BOOL ret1 = NET_DVR_StopPlayBack(_vod_stream_handler);
	BOOL ret2 = NET_DVR_Logout_V30(_sdk_user_id);

	return 0;
}

SS_Error CHIKSDKVodPullTask::run()
{
	if (SS_RecvData != CSDKRecvTask::run()) return SS_NoErr;

#ifdef PUSH_HK_VIDEO_FILE
	if (!_next_fetch_video_frame_timestamp)
		_next_fetch_video_frame_timestamp = timeGetTime();

	if (_next_fetch_video_frame_timestamp > timeGetTime())
	{
		//TRACE("\n<Wait %d -> %d", _next_fetch_video_frame_timestamp, timeGetTime());
		return SS_NoErr;
	}

	int length = 16384;
	if(length != fread(_packet, sizeof(unsigned char), length, _hk_video_file))
	{
		fseek(_hk_video_file, 0, SEEK_SET);
		return SS_NoErr;
	}

	packet_combine(_packet, length);

#endif

	return SS_NoErr;
}

void CHIKSDKVodPullTask::packet_combine(unsigned char* data, int len)
{
	//TRACE("\n-------------------------- %d\n", len);
	//for(int i = 0; i < len; ++i) TRACE("%x ", data[i]);
	//return;

	if(false)
	{
		FILE* f = fopen("d:\\hkv0.dat", "ab+");

		//char temp[] = { 0xFF, 0xFF, 0xFF, 0xFF };
		//fwrite(temp, 1, 4, f);

		//fwrite(&len, sizeof(int), 4, f);

		fwrite(data, 1, len, f);
		fclose(f);
	}

	int pos = 0;
	int last_heaer = 0;

	while (pos < len - 3)
	{
		if (0 == data[pos] && 0 == data[pos + 1] && 1 == data[pos + 2])
		{
			if (!pos)
			{
				last_heaer = pos;
				pos += 4;
			}
			else
			{
				if (0xe0 == data[pos + 3] || 0xba == data[pos + 3] || 0xbc == data[pos + 3] || 0xc0 == data[pos + 3] || 0xbd == data[pos + 3])
				{
					memcpy(_packet_buf + _packet_buf_pos, data + last_heaer, pos - last_heaer);
					_packet_buf_pos += (pos - last_heaer);

					on_recv_packet(_packet_buf, _packet_buf_pos);
					_packet_buf_pos = 0;
					
					last_heaer = pos;
					pos += 4;

				}
				else//video frame
				{
					++pos;
				}

			}
		}
		else
		{
			++pos;
		}
	}

	memcpy(_packet_buf + _packet_buf_pos, data + last_heaer, len - last_heaer);
	_packet_buf_pos += (len - last_heaer);
}

void CHIKSDKVodPullTask::on_recv_packet(unsigned char* data, int len)
{
	//TRACE("\n<FRAME %x %x %x %x %x %d > ", data[0], data[1], data[2], data[3], data[4], len);
	//return;

	if (false)
	//if (0 == data[0] && 0 == data[1] && 1 == data[2])
	{
		FILE* f = fopen("d:\\hkv1.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);

		return;
	}

	if (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xe0 == data[3])
	{
		if (false)
		{
			FILE* f = fopen("d:\\hkv2.dat", "ab+");
			fwrite(data, 1, len, f);
			fclose(f);
		}

		len -= 9 + data[8];
		data += 9 + data[8];

		//TRACE("\n<FRAME %d %x %x %x %x %x> ", len, data[0], data[1], data[2], data[3], data[4]);
	}
	/*else if (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xbc == data[3])
	{
	//TRACE("\n<BC %d %x %x %x %x %x", len, data[0], data[1], data[2], data[3], data[4]);
	return;
	}*/
	else// if (0x49 == data[0] && 0x4d == data[1] && 0x4b == data[2] && 0x48 == data[3])//IMKH
	{
		//TRACE("\n<BA %d %x %x %x %x %x", len, data[0], data[1], data[2], data[3], data[4]);
		return;
	}

	if (false)
	{
		FILE* f = fopen("d:\\hkv3.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);

		return;
	}

	_last_packet_timestamp = timeGetTime();

	//TRACE("\n<PS %d %x %x %x %x %x> ", len, data[0], data[1], data[2], data[3], data[4]);

	if(false)
	{
		FILE* f = fopen("d:\\hkv7.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);
	}

	long index = 0;

	//假设不存在NAL间隔横跨两个packet的情况
	while (index < len - 5)
	{
		if (0 == data[index] && 0 == data[index + 1] && 0 == data[index + 2] && 1 == data[index + 3])
		{
			if (_frame_pos)
			{
				NAL_TYPE type = (NAL_TYPE)(_frame[4] & 0x1f);
				if (NAL_SPS == type)
					_got_sps_pps = true;

				if (true == _got_sps_pps)
				{
					on_recv_frame(_frame, _frame_pos, false);
#ifdef PUSH_HK_VIDEO_FILE
					if (NAL_SLICE_IDR == type || NAL_SLICE == type)
					{
						//TRACE("\n<INC %d", _next_fetch_video_frame_timestamp);
						_next_fetch_video_frame_timestamp += HK_VIDEO_FRAME_INTERVAL;
						//TRACE(" -> %d>", _next_fetch_video_frame_timestamp);
					}
#endif
				}

				//TRACE("\n------------------ <Frame %d length: %d @ %d", data[index + 4] & 0x1f, _frame_pos, _last_packet_timestamp);
				//TRACE("\n------------------ <Frame %d length: %d @ %d", _frame[4] & 0x1f, _frame_pos, _last_packet_timestamp);
			}

			_frame_pos = 0;

			memcpy(_frame + _frame_pos, data + index, 4);
			_frame_pos += 4;

			index += 4;

			_last_frame_type = (NAL_TYPE)(data[index + 4] & 0x1f);
		}
		else
		{
			if (NAL_INVALID != _last_frame_type)
			{
				memcpy(_frame + _frame_pos, data + index, 1);
				++_frame_pos;
			}

			++index;
		}
	}

	memcpy(_frame + _frame_pos, data + index, len - index);
	_frame_pos += (len - index);

}

void CALLBACK CHIKSDKVodPullTask::StreamCallback(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{
	//TRACE("\n<F %d %d %x %x %x %x %x", dwDataType, dwBufSize, pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4]);

	if (NET_DVR_STREAMDATA != dwDataType) return;

	CHIKSDKVodPullTask* task = (CHIKSDKVodPullTask*)pUser;
	task->packet_combine(pBuffer, dwBufSize);

	return;
}

SS_Error CHIKSDKVodPullTask::on_done()
{
	CSDKRecvTask::on_done();

	_task_info.clear();

	BOOL ret1 = NET_DVR_StopPlayBack(_vod_stream_handler);
	BOOL ret2 = NET_DVR_Logout_V30(_sdk_user_id);

	_is_done = true;

	return SS_NoErr;
}
