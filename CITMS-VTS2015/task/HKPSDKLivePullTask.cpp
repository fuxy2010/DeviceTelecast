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

//int32_t CHKPSDKLivePullTask::_sdk_handler = -1;
bool CHKPSDKLivePullTask::_sdk_initialized = false;
CSSMutex CHKPSDKLivePullTask::_sdk_init_mutex;

#include "sps_decode.h"

CHKPSDKLivePullTask::CHKPSDKLivePullTask(SDK_RECV_TASK_INFO& task_info):
CSDKRecvTask(task_info)
{
	_frame = new unsigned char[65536 * 100];

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
	_got_first_video_packet = false;

	_sdk_handler = -1;
}

CHKPSDKLivePullTask::~CHKPSDKLivePullTask()
{
	delete _frame;
	_frame = NULL;
}

int CHKPSDKLivePullTask::init_sdk()
{
	CSSLocker lock(&_sdk_init_mutex);

	//delete _frame;
	//_frame = NULL;

	if(true == CSDKRecvTask::_hik_platform_sdk_initialized)
		return 0;

	if(true == _sdk_initialized)
		return 0;

	if(0 != Plat_Init()) return -1;

	CSDKRecvTask::_hik_platform_sdk_initialized = true;

	_sdk_initialized = true;

	return 0;
}

#include <Poco/URI.h>
#include <Poco/NumberFormatter.h>
#include <Poco/String.h>

int CHKPSDKLivePullTask::connect_device()
{
	SINGLETON(CScheduleServer).reg_ua(_task_info.ua_id, UA_MobilePhone);
	_ua = SINGLETON(CScheduleServer).fetch_ua(_task_info.ua_id);

	if(NULL == _ua)
		return -3;

	if(0 != init_sdk())
		return -2;

	memset(_video_frame, 0, 65536 * 100);//memset(_video_frame, 0, sizeof(_video_frame));
	_video_frame_length = 0;
	_last_video_type = NAL_INVALID;

	memset(_frame, 0, 65536 * 100);//memset(_frame, 0, sizeof(_frame));
	_last_frame_type = NAL_INVALID;
	_frame_pos = 0;

	string url = _task_info.sdk_url;
	TRACE("\nSDK: < %s >", url.c_str());

	int ret = -1;
	{
		//ivms8600://[user]:[passwd]@<ip>:<port>/params?cameraid=xxxxxx&fps=25
		//ivms8600://[ [user]:[passwd]@]<ip>:<port>/params?dev=xx&cameraid=xxxxxx&fps=x
#if 0
		url = url.substr(url.find("://") + 3, url.length() - strlen("ivms8600://"));
		//fym/88888888@192.168.0.190:9000/params?cameraid=1000004
		_username = url.substr(0, url.find("/"));
		_password = url.substr(url.find("/") + 1, url.find("@") - url.find("/") - 1);
		_ip = url.substr(url.find("@") + 1, url.find(":") - url.find("@") - 1);
		_port = url.substr(url.find(":") + 1, url.find_last_of("/") - url.find(":") - 1);
		_camera_id = url.substr(url.find("cameraid=") + 9, url.find("&fps") - url.find("cameraid=") - 9);

		if (string::npos != url.find("dev=hk"))
			_dev_type = IVMS8600_HK;
		else
			_dev_type = IVMS8600_DH;
#else
		Poco::URI uri(url);

		std::string userInfo = uri.getUserInfo();
		_username = userInfo.substr(0, userInfo.find(':'));

		if (userInfo.length() > _username.length())
		{
			_password = userInfo.substr(_username.length() + 1);
		}

		_ip = uri.getHost();
		_port = Poco::NumberFormatter::format(uri.getPort());

		auto paramsVector = uri.getQueryParameters();
		for (auto i = 0; i < paramsVector.size(); ++i)
		{
			std::string key = Poco::toLower(paramsVector[i].first);
			std::string val = paramsVector[i].second;

			if ("cameraid" == key)
				_camera_id = val;
			else if ("fps" == key)
				_fps = val;
			else if("dev" == key)
			{
				std::string tmpType = Poco::toLower(val);
				if ("dh" == tmpType)
					_dev_type = IVMS8600_DH;
				else if ("hk" == tmpType)
					_dev_type = IVMS8600_HK;
			}
		}
#endif

		_seq = -1;

#ifdef PUSH_HK_VIDEO_FILE
		_seq = 1;
		return 0;
#endif

		//if(0 >= _sdk_handler)
		{
			CSSLocker lock(&_hk_sdk_handlers_mutex);

			string key = _ip + _username + _password + _port;

			if (_hk_sdk_handlers.end() == _hk_sdk_handlers.find(key))
				_sdk_handler = -1;
			else
				_sdk_handler = _hk_sdk_handlers[key];

			if (0 >= _sdk_handler)
			{
				_sdk_handler = Plat_LoginCMS(_ip.c_str(), _username.c_str(), _password.c_str(), _port.c_str());

				_hk_sdk_handlers[key] = _sdk_handler;
			}
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
	if (SS_RecvData != CSDKRecvTask::run()) return SS_NoErr;

	//int i = 0;
	//int j = 1 / i;

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

		char temp[] = "ivms_hk.dat";
		if(!strcmp(temp, HK_VIDEO_FILE)) fseek(_hk_video_file, sizeof(int) * 3, SEEK_CUR);

		if(length != fread(_packet, sizeof(unsigned char), length, _hk_video_file))
		{
			fseek(_hk_video_file, 0, SEEK_SET);
			return SS_NoErr;
		}

		//TRACE("\n<HK0 %x %d %x %x %x %x", flag, length, temp[0], temp[1], temp[2], temp[3]);
		//TRACE("\n<HK1 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK2 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK0 %x %d >> %x %x %x %x <-> %x %x %x %x ---------", flag, length, _packet[0], _packet[1], _packet[2], _packet[3], _packet[length - 4], _packet[length - 3], _packet[length - 2], _packet[length - 1]);
		//TRACE("\n<HKFrame: %x %d", flag, length);

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
#else
	if (NULL != _ua)
	{
		VIDEO_PACKET_PTR packet_ptr = _ua->fetch_dev_video_packet();

		if (NULL != packet_ptr.packet && 0 < packet_ptr.packet->payload_size)
		{
			on_recv_packet(packet_ptr.packet->payload, packet_ptr.packet->payload_size);
		}

		CMemPool::free_video_packet(packet_ptr);
	}
#endif

	return SS_NoErr;
}

void CHKPSDKLivePullTask::add_recv_packet(unsigned char* data, int len)
{
	if (NULL == _ua) return;

	CSSLocker lock(&_on_recv_packet_mutext);

	_ua->add_dev_video_packet(data, len);
}

//unsigned long fl = 0;
void CHKPSDKLivePullTask::on_recv_packet(unsigned char* data, int len)
{
	//CSSLocker lock(&_on_recv_packet_mutext);

	//TRACE("\n<------ HKPacket %d >> %x %x %x %x <-> %x %x %x %x ---------", len, data[0], data[1], data[2], data[3], data[len - 4], data[len - 3], data[len - 2], data[len - 1]);
	//fl += len;
	//TRACE("\n<Packetlength: %d / %d", len, fl);
	//int length = len;
	if (!len) return;

	//if(65536 * 100 <= _frame_pos + len) return;//if (sizeof(_frame) <= _frame_pos + len) return;

	if(IVMS8600_HK == _dev_type)
	{
		unsigned char* p = data;
		if (true == is_ba(p))
		{
			int length = 14 + (p[13] & 0x07);
			//TRACE("\n<BA, %d> ------", length);
		}
		else if (true == is_bb(p) || true == is_bc(p) || true == is_bd(p))
		{
			int length = 6 + p[5] + (p[4] * 0x100);
			//TRACE("\n<%x, %d> ------", p[3], length);
		}
		else if (true == is_e0(p))
		{
			int length = 6 + p[5] + (p[4] * 0x100);
			//TRACE("\n<E0, %d> ------", length);

			//fetch packet
			{
				//int additional_length = p[8];

				unsigned char* packet = p + 9 + p[8];
				int packet_length = length - 9 - p[8];

				check_video_packet(packet, packet_length);
			}
		}
		else
		{
			return;
		}

		return;
	}

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
				if (NAL_SPS == type)
				{
					_got_sps_pps = true;

					{
#if 0
						_frame_interval = 66;
#else
						int width = 0, height = 0, fps = 0;
						h264_decode_sps(_frame + 4, _frame_pos - 4, width, height, fps);

						if (!fps) _frame_interval = 40;//默认25fps
						else _frame_interval = (int)(1000 / (double)fps);
#endif
					}
				}

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

void CHKPSDKLivePullTask::check_video_packet(unsigned char* data, int len)
{
	//TRACE("\n<PACKET %x %x %x %x %x, %d > ---------", data[0], data[1], data[2], data[3], data[4], len);

	/*if(false)
	{
	FILE* f = fopen("video2.dat", "ab+");
	fwrite(data, 1, len, f);
	fclose(f);
	return;
	}*/
	if (true == is_video_slice(data))
	{
		_got_first_video_packet = true;

		if (NAL_SPS == (NAL_TYPE)(data[4] & 0x1f))
		{
			_got_sps_pps = true;

			{
#if 0
				_frame_interval = 80;
#else
				int width = 0, height = 0, fps = 0;
				h264_decode_sps(data + 4, len - 4, width, height, fps);

				if (!fps) _frame_interval = 40;//默认25fps
				else _frame_interval = (int)(1000 / (double)fps);
#endif
			}

		}
	}

	if (false == _got_first_video_packet) return;
	if (false == _got_sps_pps) return;

	if (false)
	{
		FILE* f = fopen("video21.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);
		//return;
	}

	on_recv_packet_hk(data, len);
}

void CHKPSDKLivePullTask::on_recv_packet_hk(unsigned char* data, int len)
{
	if (!len) return;

	//TRACE("\n<PACKET %x %x %x %x %x, %d > ---------", data[0], data[1], data[2], data[3], data[4], len);

	if (true == is_video_slice(data))
	{
		if (_frame_pos)
		{
			//TRACE("\n<FRAME %x %x %x %x %x, %d > ---------", _frame[0], _frame[1], _frame[2], _frame[3], _frame[4], _frame_pos);
			if (false)
			{
				FILE* f = fopen("video3.dat", "ab+");
				fwrite(_frame, 1, _frame_pos, f);
				fclose(f);
			}

			on_recv_frame(_frame, _frame_pos, false);
		}

		memcpy(_frame, data, len);
		_frame_pos = len;
	}
	else
	{
		memcpy(_frame + _frame_pos, data, len);
		_frame_pos += len;
	}

	return;
}

void __stdcall CHKPSDKLivePullTask::StreamCallback(int handle,const char* data,int size,void *pUser)
{
	CHKPSDKLivePullTask* task = (CHKPSDKLivePullTask*)pUser;

	//task->on_recv_packet((unsigned char*)data, size);
	task->add_recv_packet((unsigned char*)data, size);
}

SS_Error CHKPSDKLivePullTask::on_done()
{
	CSDKRecvTask::on_done();

	_task_info.clear();

	int ret1 = Plat_StopVideo(_seq);
	int ret2 = Plat_LogoutCMS(_sdk_handler);

	_is_done = true;

	return SS_NoErr;
}
