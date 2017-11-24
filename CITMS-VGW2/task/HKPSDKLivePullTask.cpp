// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "HKPSDKLivePullTask.h"
#include "MiscTool.h"
#include "TimeConsuming.h"

using namespace ScheduleServer;
using namespace Platform;//HK Platform

int32_t CHKPSDKLivePullTask::_sdk_handler = -1;
bool CHKPSDKLivePullTask::_sdk_initialized = false;
CSSMutex CHKPSDKLivePullTask::_sdk_init_mutex;

CHKPSDKLivePullTask::CHKPSDKLivePullTask(SDK_RECV_TASK_INFO& task_info):
CSDKRecvTask(task_info)
{
	if(!connect_device())
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

	if(NULL != _hk_video_file)
		fseek(_hk_video_file, 0, SEEK_SET);

	_start_write = false;

	_next_fetch_video_frame_timestamp = 0;
#endif

	_got_sps_pps = false;
}

int CHKPSDKLivePullTask::init_sdk()
{
	CSSLocker lock(&_sdk_init_mutex);

	if(true == CSDKRecvTask::_hik_platform_sdk_initialized)
		return 0;

	if(true == _sdk_initialized)
		return 0;

	if(0 != Plat_Init()) return -1;

	CSDKRecvTask::_hik_platform_sdk_initialized = true;

	_sdk_initialized = true;

	return 0;
}

int CHKPSDKLivePullTask::connect_device()
{
	SINGLETON(CScheduleServer).reg_ua(_task_info.ua_id, UA_MobilePhone);
	_ua = SINGLETON(CScheduleServer).fetch_ua(_task_info.ua_id);

	if(NULL == _ua)
		return -3;

	if(0 != init_sdk())
		return -2;

	memset(_video_frame, 0, sizeof(_video_frame));
	_video_frame_length = 0;
	_last_video_type = NAL_INVALID;

	memset(_frame, 0, sizeof(_frame));
	_last_frame_type = NAL_INVALID;
	_frame_pos = 0;

	string url = _task_info.sdk_url;
	TRACE("\nSDK: %s", url.c_str());

	int ret = -1;
	{
		//cmssdk://[user]/[passwd]@<ip>:<port>/params?cameraid=xxxxxx
		url = url.substr(url.find("://") + 3, url.length() - strlen("cmssdk://"));
		//fym/88888888@192.168.0.190:9000/params?cameraid=1000004
		_username = url.substr(0, url.find("/"));
		_password = url.substr(url.find("/") + 1, url.find("@") - url.find("/") - 1);
		_ip = url.substr(url.find("@") + 1, url.find(":") - url.find("@") - 1);
		_port = url.substr(url.find(":") + 1, url.find_last_of("/") - url.find(":") - 1);
		_camera_id = url.substr(url.find("cameraid=") + 9, url.find("&channel") - url.find("cameraid=") - 9);

		_seq = -1;

#ifdef PUSH_HK_VIDEO_FILE
		_seq = 1;
		return 0;
#endif

		//if(0 >= _sdk_handler)
		{
			_sdk_handler = Plat_LoginCMS(_ip.c_str(), _username.c_str(), _password.c_str(), _port.c_str());
		}

		if(0 >= _sdk_handler)
			return -5;

		const char* real_url = Plat_QueryRealStreamURL(_camera_id.c_str(), _sdk_handler);

		if(NULL == real_url)
			return -6;

		_seq = Plat_PlayVideo(real_url, NULL, _sdk_handler, CHKPSDKLivePullTask::StreamCallback, (void*)this);

		if(0 <= _seq)
			ret = 0;
		else
			ret = -7;
	}
	
	return ret;
}

int CHKPSDKLivePullTask::disconnect_device()
{
	int ret1 = Plat_StopVideo(_seq);
	int ret2 = Plat_LogoutCMS(_sdk_handler);

	return 0;
}

SS_Error CHKPSDKLivePullTask::run()
{
	CSDKRecvTask::run();

#ifdef PUSH_HK_VIDEO_FILE
	if(!_next_fetch_video_frame_timestamp)
		_next_fetch_video_frame_timestamp = timeGetTime();

	if(_next_fetch_video_frame_timestamp > timeGetTime())
	{
		//TRACE("\n<Wait %d -> %d", _next_fetch_video_frame_timestamp, timeGetTime());
		return SS_NoErr;
	}

	if(!feof(_hk_video_file))
	{
		unsigned long flag = 0;
		fread(&flag, sizeof(unsigned long), 1, _hk_video_file);

		int length = 0;
		fread(&length, sizeof(int), 1, _hk_video_file);

		if(length != fread(_packet, sizeof(unsigned char), length, _hk_video_file))
		{
			fseek(_hk_video_file, 0, SEEK_SET);
			return SS_NoErr;
		}

		//TRACE("\n<HK0 %x %d %x %x %x %x", flag, length, temp[0], temp[1], temp[2], temp[3]);
		//TRACE("\n<HK1 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK2 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK0 %x %d >> %x %x %x %x <-> %x %x %x %x", flag, length, _packet[0], _packet[1], _packet[2], _packet[3], _packet[length - 4], _packet[length - 3], _packet[length - 2], _packet[length - 1]);
		//TRACE("\n<HKF: %x %d", flag, length);

		on_recv_packet(_packet, length);

		if(false)
		{
			FILE* f = fopen("temp.dat", "ab+");
			fwrite(_packet, 1, length, f);
			fclose(f);
		}
	}
	else
	{
		fseek(_hk_video_file, 0, SEEK_SET);

		TRACE("\n<File End");
	}
#endif

	return SS_NoErr;
}

//unsigned long fl = 0;
void CHKPSDKLivePullTask::on_recv_packet(unsigned char* data, int len)
{
	//fl += len;
	//TRACE("\n<Packetlength: %d / %d", len, fl);
	

	_last_packet_timestamp = timeGetTime();

	long index = 0;

	//假设不存在NAL间隔横跨两个packet的情况
	while(index < len - 5)
	{
		if(0 == data[index] && 0 == data[index + 1] && 0 == data[index + 2] && 1 == data[index + 3])
		{
			if(_frame_pos)
			{
				NAL_TYPE type = (NAL_TYPE)(_frame[4] & 0x1f);
				if(NAL_SPS == type)

					_got_sps_pps = true;

				if(true == _got_sps_pps)
				{
					on_recv_frame(_frame, _frame_pos, false);

#ifdef PUSH_HK_VIDEO_FILE
					if(NAL_SLICE_IDR == type || NAL_SLICE == type)
					{
						//TRACE("\n<INC %d", _next_fetch_video_frame_timestamp);
						_next_fetch_video_frame_timestamp += HK_VIDEO_FRAME_INTERVAL;
						//TRACE(" -> %d>", _next_fetch_video_frame_timestamp);
					}
					//fl = 0;
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
			if(NAL_INVALID != _last_frame_type)
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

void __stdcall CHKPSDKLivePullTask::StreamCallback(int handle,const char* data,int size,void *pUser)
{
	CHKPSDKLivePullTask* task = (CHKPSDKLivePullTask*)pUser;

	task->on_recv_packet((unsigned char*)data, size);
}

SS_Error CHKPSDKLivePullTask::on_done()
{
	_task_info.clear();

	_is_done = true;

	int ret1 = Plat_StopVideo(_seq);
	int ret2 = Plat_LogoutCMS(_sdk_handler);

	return SS_NoErr;
}

