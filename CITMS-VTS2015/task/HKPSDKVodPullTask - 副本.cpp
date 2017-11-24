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
	_hk_video_file = fopen(HK_VIDEO_FILE, "rb");

	if (NULL != _hk_video_file)
		fseek(_hk_video_file, 0, SEEK_SET);

	_start_write = false;

	_next_fetch_video_frame_timestamp = 0;
#endif

	_got_sps_pps = false;

	_sdk_handler = -1;

	_global_hk_vod_pull_task = this;
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
	_packet_buf_pos = 0;

	SINGLETON(CScheduleServer).reg_ua(_task_info.ua_id, UA_MobilePhone);
	_ua = SINGLETON(CScheduleServer).fetch_ua(_task_info.ua_id);

	if (NULL == _ua)
		return -3;

	if (0 != init_sdk())
		return -2;

	memset(_video_frame, 0, 65536 * 100);//memset(_video_frame, 0, sizeof(_video_frame));
	_video_frame_length = 0;
	_last_video_type = NAL_INVALID;

	memset(_frame, 0, 65536 * 100);//memset(_frame, 0, sizeof(_frame));
	_last_frame_type = NAL_INVALID;
	_frame_pos = 0;

	memset(_pts, 0, 65536 * 100);//memset(_pts, 0, sizeof(_pts));
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

		_seq = Plat_PlayVideo(real_url, NULL, _sdk_handler, CHKPSDKVodPullTask::StreamCallback, (void*)this);

		LOG_WRITE("CHKPSDKVodPullTask Plat_PlayVideo url: " + string(real_url) + ", ret: " + MiscTools::parse_type_to_string<int>(_seq), 1, false);

		if (0 <= _seq)
			ret = 0;
		else
			ret = -7;
	}

	return ret;
}

int CHKPSDKVodPullTask::disconnect_device()
{
	int ret1 = Plat_StopVideo(_seq);
	int ret2 = Plat_LogoutCMS(_sdk_handler);

	return 0;
}

SS_Error CHKPSDKVodPullTask::run()
{
	CSDKRecvTask::run();

#ifdef PUSH_HK_VIDEO_FILE
	if (!_next_fetch_video_frame_timestamp)
		_next_fetch_video_frame_timestamp = timeGetTime();

	if (_next_fetch_video_frame_timestamp > timeGetTime())
	{
		//TRACE("\n<Wait %d -> %d", _next_fetch_video_frame_timestamp, timeGetTime());
		return SS_NoErr;
	}

	if (!feof(_hk_video_file))
	{
		unsigned long flag = 0;
		fread(&flag, sizeof(unsigned long), 1, _hk_video_file);

		int length = 0;
		fread(&length, sizeof(int), 1, _hk_video_file);

		if (length != fread(_packet, sizeof(unsigned char), length, _hk_video_file))
		{
			fseek(_hk_video_file, 0, SEEK_SET);
			return SS_NoErr;
		}

		//TRACE("\n<HK0 %x %d %x %x %x %x", flag, length, temp[0], temp[1], temp[2], temp[3]);
		//TRACE("\n<HK1 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK2 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK0 %x %d >> %x %x %x %x <-> %x %x %x %x", flag, length, _packet[0], _packet[1], _packet[2], _packet[3], _packet[length - 4], _packet[length - 3], _packet[length - 2], _packet[length - 1]);
		//TRACE("\n<HKF: %x %d", flag, length);
		//return SS_NoErr;

		on_recv_packet(_packet, length);

		if (false)
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
#if 1
						_frame_interval = 80;
#else
						int width = 0, height = 0, fps = 0;
						h264_decode_sps(_frame, _frame_pos, width, height, fps);

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

#ifdef PUSH_HK_VIDEO_FILE
					if (NAL_SLICE_IDR == type || NAL_SLICE == type)
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

	TRACE("\n\n<HK PACKET %d > ", len);

	if (false)
	{
		FILE* f = fopen("hkvod_orig_0717.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);

		return;
	}

	if (0x49 == data[0] && 0x4d == data[1] && 0x4b == data[2] && 0x48 == data[3]) return;

	//TRACE("\n\n<HK PACKET %x %x %x %x %x %d > ", data[0], data[1], data[2], data[3], data[4], len);
	//return;

	/*if (0 == data[0] && 0 == data[1] && 1 == data[2])
	{
	if (_packet_buf_pos)
	{
	pakcet_segment(_packet_buf, _packet_buf_pos);

	_packet_buf_pos = 0;
	}
	}

	memcpy(_packet_buf + _packet_buf_pos, data, len);
	_packet_buf_pos += len;*/

	if (65536 * 100 < len + _pts_pos)//if (sizeof(_pts) < len + _pts_pos)
	{
		TRACE("\nPTS OVERFLOW!");
		return;
	}

	//TRACE("\n<POS1 %d > ", _pts_pos);
	memcpy(_pts + _pts_pos, data, len);
	_pts_pos += len;
	//TRACE("\n<POS2 %d > ", _pts_pos);

	int index = 0;
	int p1 = -1;
	int p2 = -1;
	while (index < _pts_pos - 3)
	{
		if (true == is_pts(_pts + index))
		{
			if (-1 == p1)
			{
				p1 = index;
				//TRACE("\nP1: %d ", p1);
			}
			else
			{
				p2 = index;
				//TRACE("\nP2: %d ", p2);
				{
					pakcet_segment(_pts + p1, p2 - p1);
				}

				p1 = p2;
			}

			index += 4;
		}
		else
		{
			++index;
		}
	}



	if (0 < p1)
	{
		memcpy(_pts, _pts + p1, _pts_pos - p1);
		_pts_pos -= p1;
	}

	//TRACE("\n<POS3 %d > ", _pts_pos);

	return;





	while (index < len)
	{
		if (0 == data[index] && 0 == data[index + 1] && 1 == data[index + 2] && (0xe0 == data[index + 3] || 0xba == data[index + 3] || 0xbc == data[index + 3] || 0xc0 == data[index + 3] || 0xbd == data[index + 3]))
		{
			TRACE("\n<PTS FLAG ------------------ %x %x %x %x %x %d %d> ", data[index], data[index + 1], data[index + 2], data[index + 3], data[index + 4], index);

			if (_pts_pos)
			{
				//TRACE("\n<PTS %x %x %x %x %x %d > ", _pts[0], _pts[1], _pts[2], _pts[3], _pts[4], _pts_pos);
				//TRACE("\n<PTS FLAG ------------------ %x %x %x %x %x %d > ", data[index], data[index + 1], data[index + 2], data[index + 3], data[index + 4], index);
				pakcet_segment(_pts, _pts_pos);
			}

			_pts_pos = 0;

			memcpy(_pts + _pts_pos, data + index, 4);

			_pts_pos += 4;
			index += 4;
		}
		else
		{
			memcpy(_pts + _pts_pos, data + index, 1);

			++_pts_pos;
			++index;
		}
	}
}

void CHKPSDKVodPullTask::pakcet_segment(unsigned char* data, int len)
{
	//TRACE("\n<PTS %x %x %x %x %x %d > ", data[0], data[1], data[2], data[3], data[4], len);
	//return;

	if (true == is_pts(data))
	{
		//TRACE("\n<PTS %x %x %x %x %x -> %d > ", data[0], data[1], data[2], data[3], data[4], len);

		if (true == is_video_pts(data))
		{
			//int offset = 9 + data[8];
			//TRACE("\n<OFFSET %d -> %x %x %x %x %x> ", offset, data[offset + 0], data[offset + 1], data[offset + 2], data[offset + 3], data[offset + 4] & 0x1f);
			//len -= 9 + data[8];
			//data += 9 + data[8];
		};
	}
	else
	{
		TRACE("\n<UNKNOWN %x %x %x %x %x -> %d > ", data[0], data[1], data[2], data[3], data[4], len);
	}

	on_recv_packet_hk(data, len);
	return;

	//int type = 0xba & 0x1f;
	//type = 0xe0 & 0x1f;
	//type = 0xbc & 0x1f;
	//type = 0xc0 & 0x1f;
	//type = 0xbd & 0x1f;

	int pos = 0;
	int last_heaer = 0;

	if (false)//ok
	{
		FILE* f = fopen("pts.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);

		return;
	}

	//假设不存在NAL间隔横跨两个packet的情况
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
					on_recv_packet_hk(data + last_heaer, pos - last_heaer);
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

	on_recv_packet_hk(data + last_heaer, len - last_heaer);
}

void CHKPSDKVodPullTask::on_recv_packet_hk(unsigned char* data, int len)
{
	//TRACE("\n<FRAME %x %x %x %x %x %d > ---------", data[0], data[1], data[2], data[3], data[4], len);
	//return;

	if (false)
	{
		FILE* f = fopen("pts.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);
	}

	if(true == is_video_pts(data))
	{
		if (false)
		{
			FILE* f = fopen("ptse0.dat", "ab+");
			fwrite(data, 1, len, f);
			fclose(f);

			return;
		}

		//int offset = 9 + data[8];
		//TRACE("\n<OFFSET %d -> %x %x %x %x %x, %d> ", offset, data[offset + 0], data[offset + 1], data[offset + 2], data[offset + 3], data[offset + 4] & 0x1f, len - offset);
		len -= 9 + data[8];
		data += 9 + data[8];

		//TRACE("\n<FRAME %x %x %x %x %x, %d> ", data[0], data[1], data[2], data[3], data[4] & 0x1f, len);
	}
	/*else if (0 == data[0] && 0 == data[1] && 1 == data[2] && 0xbc == data[3])
	{
	//TRACE("\n<BC %d %x %x %x %x %x", len, data[0], data[1], data[2], data[3], data[4]);
	return;
	}*/
	else if(true == is_pts(data))// if (0x49 == data[0] && 0x4d == data[1] && 0x4b == data[2] && 0x48 == data[3])//IMKH
	{
		//TRACE("\n<BA %d %x %x %x %x %x", len, data[0], data[1], data[2], data[3], data[4]);
		return;
	}
	else
	{
		return;
	}

	if (false)
	{
		FILE* f = fopen("ptsv.dat", "ab+");
		fwrite(data, 1, len, f);
		fclose(f);

		return;
	}

	_last_packet_timestamp = timeGetTime();

	long index = 0;

	//假设不存在NAL间隔横跨两个packet的情况
	while (index < len)
	{
		if (0 == data[index] && 0 == data[index + 1] && 0 == data[index + 2] && 1 == data[index + 3])
		{
			//TRACE("\n------------------ <Frame %d %x length: %ld +++++++++", data[index + 4] & 0x1f, data[index + 4], index);
			if (_frame_pos)
			{
				NAL_TYPE type = (NAL_TYPE)(_frame[4] & 0x1f);
				if (NAL_SPS == type)
				{
					_got_sps_pps = true;
					{
#if 1
						_frame_interval = 80;
#else
						int width = 0, height = 0, fps = 0;
						h264_decode_sps(_frame, _frame_pos, width, height, fps);

						if (!fps) _frame_interval = 40;//默认25fps
						else _frame_interval = (int)(1000 / (double)fps);
#endif
					}
				}

				if (true == _got_sps_pps)
				{
					//TRACE("\n------------------ <Frame %d length: %ld @ %d", _frame[4] & 0x1f, _frame_pos, _last_packet_timestamp);

					if (false)
					{
						FILE* f = fopen("pure.dat", "ab+");
						fwrite(data, 1, len, f);
						fclose(f);
					}

					on_recv_frame(_frame, _frame_pos, false);

#ifdef PUSH_HK_VIDEO_FILE
					if (NAL_SLICE_IDR == type || NAL_SLICE == type)
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
			if (NAL_INVALID != _last_frame_type)
			{
				memcpy(_frame + _frame_pos, data + index, 1);
				++_frame_pos;
			}

			++index;
		}
	}

	//memcpy(_frame + _frame_pos, data + index, len - index);
	//_frame_pos += (len - index);
}

CHKPSDKVodPullTask* CHKPSDKVodPullTask::_global_hk_vod_pull_task = NULL;
void __stdcall CHKPSDKVodPullTask::StreamCallback(int handle, const char* data, int size, void *pUser)
{
	if (false)
	{
		FILE* f = fopen("ivms_vod.dat", "ab+");

		char temp[] = { 0xFF, 0xFF, 0xFF, 0xFF };
		fwrite(temp, 1, 4, f);

		fwrite(&size, sizeof(int), 1, f);

		fwrite(data, 1, size, f);

		fclose(f);

		return;
	}

	CHKPSDKVodPullTask* task = (CHKPSDKVodPullTask*)pUser;

	if (NULL != task)
	{
		task->on_recv_packet((unsigned char*)data, size);
	}
	else if (NULL != _global_hk_vod_pull_task)
	{
		_global_hk_vod_pull_task->on_recv_packet((unsigned char*)data, size);
	}
}

SS_Error CHKPSDKVodPullTask::on_done()
{
	_task_info.clear();

	_is_done = true;

	int ret1 = Plat_StopVideo(_seq);
	int ret2 = Plat_LogoutCMS(_sdk_handler);

	return SS_NoErr;
}

