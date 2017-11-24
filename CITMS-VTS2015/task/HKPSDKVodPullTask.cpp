// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "HKPSDKVodPullTask.h"
#include "MiscTool.h"
#include "TimeConsuming.h"

using namespace ScheduleServer;
using namespace Platform;//HK Platform

//int32_t CHKPSDKVodPullTask::_sdk_handler = -1;
bool CHKPSDKVodPullTask::_sdk_initialized = false;
CSSMutex CHKPSDKVodPullTask::_sdk_init_mutex;

CHKPSDKVodPullTask::CHKPSDKVodPullTask(SDK_RECV_TASK_INFO& task_info) :
	CSDKRecvTask(task_info)
{
	_frame = new unsigned char[65536 * 100];
	_pts = new unsigned char[65536 * 100];

	if (!connect_device())
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
	_hk_video_file = fopen(HK_VIDEO_FILE, "rb+");

	if (NULL != _hk_video_file)
		fseek(_hk_video_file, 0, SEEK_SET);

	string log = HK_VIDEO_FILE;
	log = MiscTools::parse_now_to_string() + " Play: " + log;
	SINGLETON(CScheduleServer).add_log(log);

	_start_write = false;

	_next_fetch_video_frame_timestamp = 0;
#endif

#ifdef GET_VOD_BY_DOWNLOADING
	_vod_file = NULL;
	_vod_file_offset = 0;
	_download_percent = 0;
#endif

	_got_sps_pps = false;
	_got_first_video_packet = false;

	_sdk_handler = -1;

	//_global_hk_vod_pull_task = this;
}

CHKPSDKVodPullTask::~CHKPSDKVodPullTask()
{
	delete _frame;
	_frame = NULL;

	delete _pts;
	_pts = NULL;
}

int CHKPSDKVodPullTask::init_sdk()
{
	CSSLocker lock(&_sdk_init_mutex);

	if (true == CSDKRecvTask::_hik_platform_sdk_initialized)
		return 0;

	if (true == _sdk_initialized)
		return 0;

	if (0 != Plat_Init()) return -1;

	CSDKRecvTask::_hik_platform_sdk_initialized = true;

	_sdk_initialized = true;

	return 0;
}

#include <Poco/URI.h>
#include <Poco/NumberFormatter.h>
#include <Poco/String.h>

int CHKPSDKVodPullTask::connect_device()
{
	SINGLETON(CScheduleServer).reg_ua(_task_info.ua_id, UA_MobilePhone);
	_ua = SINGLETON(CScheduleServer).fetch_ua(_task_info.ua_id);

	if (NULL == _ua)
		return -3;

	if (0 != init_sdk())
		return -2;

	memset(_video_frame, 0, VOD_BUFFER_LEN);//memset(_video_frame, 0, sizeof(_video_frame));
	_video_frame_length = 0;
	_last_video_type = NAL_INVALID;

	memset(_frame, 0, VOD_BUFFER_LEN);//memset(_frame, 0, sizeof(_frame));
	_last_frame_type = NAL_INVALID;
	_frame_pos = 0;

	memset(_pts, 0, VOD_BUFFER_LEN);//memset(_pts, 0, sizeof(_pts));
	_pts_pos = 0;

	string url = _task_info.sdk_url;
	TRACE("\nSDK: %s", url.c_str());

	int ret = -1;
	{
		//ivms8600://[user]/[passwd]@<ip>:<port>/params?cameraid=1000004&condition=x&begintime=1482474300&endtime=1482474397&fps=25
		//注：condition为录像查询条件，0为IPSAN  1为设备  2为PCNVR
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
			else if ("condition" == key)
				_condition = val;
			else if ("begintime" == key)
				_begin_time = MiscTools::parse_string_to_type<unsigned long long>(val);
			else if ("endtime" == key)
				_end_time = MiscTools::parse_string_to_type<unsigned long long>(val);
			else if ("fps" == key)
				_fps = val;
			else if ("dev" == key)
			{
				std::string tmpType = Poco::toLower(val);
				if ("dh" == tmpType)
					_dev_type = IVMS8600_DH;
				else if ("hk" == tmpType)
					_dev_type = IVMS8600_HK;
			}
		}
#if 0
		url = url.substr(url.find("://") + 3, url.length() - strlen("dsssdk://"));
		//fym/88888888@192.168.0.190:9000/params?cameraid=1000004&condition=1&begintime=1482474300&endtime=1482474397
		_username = url.substr(0, url.find("/"));
		_password = url.substr(url.find("/") + 1, url.find("@") - url.find("/") - 1);
		_ip = url.substr(url.find("@") + 1, url.find(":") - url.find("@") - 1);
		_port = url.substr(url.find(":") + 1, url.find_last_of("/") - url.find(":") - 1);
		_camera_id = url.substr(url.find("cameraid=") + 9, url.find("&condition") - url.find("cameraid=") - 9);
		_condition = url.substr(url.find("condition=") + 10, url.find("&begintime") - url.find("condition=") - 10);

		_begin_time = MiscTools::parse_string_to_type<unsigned long long>(url.substr(url.find("begintime=") + 10, url.find("&endtime") - url.find("begintime=") - 10));
		_end_time = MiscTools::parse_string_to_type<unsigned long long>(url.substr(url.find("endtime=") + 8, url.find("&fps") - url.find("endtime=") - 8));

		if (string::npos != url.find("dev=hk"))
			_dev_type = IVMS8600_HK;
		else
			_dev_type = IVMS8600_DH;
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

		if (0 >= _sdk_handler)
		{
			LOG_WRITE("CHKPSDKVodPullTask Plat_LoginCMS fail " + _sdk_handler, 1, false);
			return -5;
		}

		//////////////////////////////////////////////////////////////////////////
		/*int	Plat_QueryRecordFile(
		const char*	csCameraID,
		const long	    lStartTime,
		const long	    lEndtime,
		const char*	csQueryCondition
		int			iUserHandle)*/

		// TODO added by 谭军胜
		// 执行SDK接口建议采用加锁操作，否则存在并发风险

		LOG_WRITE("CHKPSDKVodPullTask Plat_QueryRecordFile: " + MiscTools::parse_type_to_string<unsigned long>(_begin_time) + " -> " + MiscTools::parse_type_to_string<unsigned long>(_end_time), 1, false);

		if (-1 == Plat_QueryRecordFile(_camera_id.c_str(), _begin_time, _end_time, _condition.c_str(), _sdk_handler))
		{
			int ec = Plat_GetLastError();
			LOG_WRITE("CHKPSDKVodPullTask Plat_QueryRecordFile fail: " + ec, 1, false);
			if (-10302 == ec)
			{
				//AfxMessageBox("该用户在此监控点无录像查询权限");
				return -8;
			}

			return -6;
		}

		const char* real_url = Plat_GetPlayBackUrl(_sdk_handler);

		//////////////////////////////////////////////////////////////////////////

		if (NULL == real_url)
			return -6;

#ifndef GET_VOD_BY_DOWNLOADING
		_seq = Plat_PlayVideo(real_url, NULL, _sdk_handler, CHKPSDKVodPullTask::StreamCallback, (void*)this);
		LOG_WRITE("CHKPSDKVodPullTask Plat_PlayVideo url: " + string(real_url) + ", ret: " + MiscTools::parse_type_to_string<int>(_seq), 1, false);
#else

		//启动下载
		//CString strFileName = “testdownload.mp4”;
		_vod_file_path = "VOD" + MiscTools::parse_type_to_string<unsigned long>(timeGetTime() & 0xffff) + ".mp4";
		_seq = Plat_StartDownLoad(real_url, _vod_file_path.c_str(), 0, 0, 512 * 1024 * 1024, _sdk_handler);
#endif

		if (0 <= _seq)
			ret = 0;
		else
			ret = -7;
	}

	return ret;
}

int CHKPSDKVodPullTask::disconnect_device()
{
#ifndef GET_VOD_BY_DOWNLOADING
	int ret1 = Plat_StopVideo(_seq);
#else
	int ret1 = Plat_StopDownLoad(_seq);
#endif
	int ret2 = Plat_LogoutCMS(_sdk_handler);

	fclose(_vod_file);
	_vod_file = NULL;

	return 0;
}

SS_Error CHKPSDKVodPullTask::run()
{
	if (SS_RecvData != CSDKRecvTask::run()) return SS_NoErr;

#ifndef PUSH_HK_VIDEO_FILE

#ifdef GET_VOD_BY_DOWNLOADING
	if(100 > _download_percent)
	{
		if (0 > _seq) return SS_NoErr;

		if (0 != Plat_GetFilePercent(_seq, &_download_percent)) return SS_NoErr;

		if (100 == _download_percent)
		{
			int ret = Plat_StopDownLoad(_seq);

			{
				string log = "";
				log += "Download completed! " + MiscTools::parse_now_to_string() + "\r\n";
				log += _vod_file_path + "\r\n";

				SINGLETON(CScheduleServer).add_log(log);
			}

			_vod_file = fopen(_vod_file_path.c_str(), "rb+");

			if (NULL != _vod_file) fseek(_vod_file, 0, SEEK_SET);

			SINGLETON(CScheduleServer).add_log(MiscTools::parse_now_to_string() + " Play: " + _vod_file_path);

			_next_fetch_video_frame_timestamp = 0;
		}
	}
	else if (100 == _download_percent)//download completed
	{
		if (!_next_fetch_video_frame_timestamp)
			_next_fetch_video_frame_timestamp = timeGetTime();

		if (_next_fetch_video_frame_timestamp > timeGetTime())
		{
			//TRACE("\n<Wait %d -> %d", _next_fetch_video_frame_timestamp, timeGetTime());
			return SS_NoErr;
		}

		if (NULL == _vod_file)
		{
			_vod_file = fopen(_vod_file_path.c_str(), "rb+");

			return SS_NoErr;
		}

		if(true)//if (!feof(_vod_file))
		{
			int length = 5120;

			if (length != fread(_packet, sizeof(unsigned char), length, _vod_file))
			{
				//fseek(_vod_file, 0, SEEK_SET);
				TRACE("\n<File End");

				_download_percent = 101;//此后不再读取该文件

				return SS_NoErr;
			}

			on_recv_packet(_packet, length);

			_next_fetch_video_frame_timestamp += 10;
		}
		else
		{
			//fseek(_vod_file, 0, SEEK_SET);

			TRACE("\n<File End");
		}
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
#endif//GET_VOD_BY_DOWNLOADING

#else
	if (!_next_fetch_video_frame_timestamp)
		_next_fetch_video_frame_timestamp = timeGetTime();

	if (_next_fetch_video_frame_timestamp > timeGetTime())
	{
		//TRACE("\n<Wait %d -> %d", _next_fetch_video_frame_timestamp, timeGetTime());
		return SS_NoErr;
	}

	if(!feof(_hk_video_file))
	{
		int length = 5120;

		if (length != fread(_packet, sizeof(unsigned char), length, _hk_video_file))
		{
			fseek(_hk_video_file, 0, SEEK_SET);
			return SS_NoErr;
		}
		
		on_recv_packet(_packet, length);

		_next_fetch_video_frame_timestamp += 10;
	}
	else
	{
		fseek(_hk_video_file, 0, SEEK_SET);

		TRACE("\n<File End");
	}
#endif

	return SS_NoErr;
}

void CHKPSDKVodPullTask::add_recv_packet(unsigned char* data, int len)
{
	if (NULL == _ua) return;

	CSSLocker lock(&_on_recv_packet_mutext);

	_ua->add_dev_video_packet(data, len);
}

void CHKPSDKVodPullTask::on_recv_packet(unsigned char* data, int len)
{
	CSSLocker lock(&_on_recv_packet_mutext);

	if (IVMS8600_DH == _dev_type)
		on_recv_packet_dh(data, len);
	else if (IVMS8600_HK == _dev_type)
		packet_combine(data, len);
}

void CHKPSDKVodPullTask::on_recv_packet_dh(unsigned char* data, int len)
{
	if (!len) return;
	//TRACE("\n<DH FRAME %x %x %x %x %x %d > ", data[0], data[1], data[2], data[3], data[4], len);
	//return;

	if (false)
	{
		FILE* f = fopen("d:\\ivms_vod_pure0.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);
	}

	int index = 0;

	while (index < len)
	{
		if (0 == data[index] && 0 == data[index + 1] && 0 == data[index + 2] && 1 == data[index + 3])
		{
			//TRACE("\n------------------ <Frame %d length: %d @ %d", _frame[4] & 0x1f, _frame_pos, _last_packet_timestamp);

			if (_frame_pos)
			{
				NAL_TYPE type = (NAL_TYPE)(_frame[4] & 0x1f);
				if (NAL_SPS == type)
				{
					_got_sps_pps = true;
					{
#if 0
						_frame_interval = 80;
#else
						int width = 0, height = 0, fps = 0;
						h264_decode_sps(_frame + 4, _frame_pos - 4, width, height, fps);

						if (!fps) _frame_interval = 40;//默认25fps
						else _frame_interval = (int)(1000 / (double)fps);
#endif
					}
				}

				if (true == _got_sps_pps)
				{
					//TRACE("\n------------------ <Frame %d length: %d @ %d", _frame[4] & 0x1f, _frame_pos, _last_packet_timestamp);

					if (false)
					{
						FILE* f = fopen("ivms_vod_pure.dat", "ab+");
						fwrite(_frame, 1, _frame_pos, f);
						fclose(f);
					}

					//TRACE("\n<DH FRAME %x %x %x %x %x %d > ", _frame[0], _frame[1], _frame[2], _frame[3], _frame[4] & 0x1f, _frame_pos);
					on_recv_frame(_frame, _frame_pos, false);

//#ifdef PUSH_HK_VIDEO_FILE
					if (NAL_SLICE_IDR == type || NAL_SLICE == type)
					{
						//TRACE("\n<INC %d", _next_fetch_video_frame_timestamp);
						_next_fetch_video_frame_timestamp += HK_VIDEO_FRAME_INTERVAL;
						//TRACE(" -> %d>", _next_fetch_video_frame_timestamp);
					}
					//fl = 0;
//#endif
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
}

void CHKPSDKVodPullTask::packet_combine(unsigned char* data, int len)
{
	if (!len) return;

	if (VOD_BUFFER_LEN <= _pts_pos + len) return;

	memcpy(_pts + _pts_pos, data, len);
	_pts_pos += len;

	int index = 0;
	while(index < _pts_pos - 4)
	{
		unsigned char* p = _pts + index;

		if(true == is_ba(p))
		{
			int length = 14 + (p[13] & 0x07);
			//TRACE("\n<BA, %d> ------", length);
			
			if (index + length > _pts_pos)
			{
				if (index > _pts_pos) return;
				memcpy(_pts, p, _pts_pos - index);
				_pts_pos -= index;
				index = 0;
				break;
			}

			index += length;
		}
		else if(true == is_bb(p) || true == is_bc(p) || true == is_bd(p))
		{
			int length = 6 + p[5] + (p[4] * 0x100);
			//TRACE("\n<%x, %d> ------", p[3], length);

			if (index + length > _pts_pos)
			{
				if (index > _pts_pos) return;
				memcpy(_pts, p, _pts_pos - index);
				_pts_pos -= index;
				index = 0;
				break;
			}

			index += length;
		}
		else if(true == is_e0(p))
		{
			int length = 6 + p[5] + (p[4] * 0x100);
			//TRACE("\n<E0, %d> ------", length);

			if(index + length > _pts_pos)
			{
				if (index > _pts_pos) return;
				memcpy(_pts, p, _pts_pos - index);
				_pts_pos -= index;
				index = 0;
				break;
			}

			//fetch packet
			{
				//int additional_length = p[8];

				unsigned char* packet = p + 9 + p[8];
				int packet_length = length - 9 - p[8];

				pakcet_segment(packet, packet_length);
				if(false)
				{
					FILE* f = fopen("video.dat", "ab+");
					fwrite(packet, 1, packet_length, f);
					fclose(f);
				}
			}

			index += length;
		}
		else
		{
			++index;
		}
	}

	return;
}

void CHKPSDKVodPullTask::pakcet_segment(unsigned char* data, int len)
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

void CHKPSDKVodPullTask::on_recv_packet_hk(unsigned char* data, int len)
{
	if (!len) return;

	//TRACE("\n<PACKET %x %x %x %x %x, %d > ---------", data[0], data[1], data[2], data[3], data[4], len);

	if (true == is_video_slice(data))
	{
		if (_frame_pos)
		{
			//TRACE("\n<FRAME %x %x %x %x %x, %d > ---------", _frame[0], _frame[1], _frame[2], _frame[3], _frame[4], _frame_pos);
			if(false)
			{
				FILE* f = fopen("video3.dat", "ab+");
				fwrite(_frame, 1, _frame_pos, f);
				fclose(f);
			}

			on_recv_frame(_frame, _frame_pos, false);
		}

		if(VOD_BUFFER_LEN < len) return;
		memcpy(_frame, data, len);
		_frame_pos = len;
	}
	else
	{
		if (VOD_BUFFER_LEN <= len + _frame_pos) return;
		memcpy(_frame + _frame_pos, data, len);
		_frame_pos += len;
	}

	return;
}

//CHKPSDKVodPullTask* CHKPSDKVodPullTask::_global_hk_vod_pull_task = NULL;
void __stdcall CHKPSDKVodPullTask::StreamCallback(int handle, const char* data, int size, void *pUser)
{
	if (false)
	{
		FILE* f = fopen("sdk_vod.dat", "ab+");

		//char temp[] = { 0xFF, 0xFF, 0xFF, 0xFF };
		//fwrite(temp, 1, 4, f);
		//fwrite(&size, sizeof(int), 1, f);

		fwrite(data, 1, size, f);
		fclose(f);

		//return;
	}

	CHKPSDKVodPullTask* task = (CHKPSDKVodPullTask*)pUser;

	if (NULL != task)
	{
		//task->on_recv_packet((unsigned char*)data, size);
		task->add_recv_packet((unsigned char*)data, size);
	}
	/*else if (NULL != _global_hk_vod_pull_task)
	{
		_global_hk_vod_pull_task->add_recv_packet((unsigned char*)data, size);
	}*/
}

//#include <windows.h>
SS_Error CHKPSDKVodPullTask::on_done()
{
	CSDKRecvTask::on_done();

	_task_info.clear();

#ifndef GET_VOD_BY_DOWNLOADING
	int ret1 = Plat_StopVideo(_seq);
#else
	int ret1 = Plat_StopDownLoad(_seq);
#endif
	int ret2 = Plat_LogoutCMS(_sdk_handler);

#ifdef GET_VOD_BY_DOWNLOADING
	string cmd = "del " + _vod_file_path;
	system(cmd.c_str());
	//remove(_vod_file_path.c_str());
	//DeleteFile(_vod_file_path.c_str());
#endif

	_is_done = true;

	return SS_NoErr;
}
