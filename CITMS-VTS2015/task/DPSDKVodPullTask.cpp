// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "DPSDKVodPullTask.h"
#include "MiscTool.h"
#include "TimeConsuming.h"

using namespace ScheduleServer;

int32_t CDPSDKVodPullTask::_sdk_handler = -1;
bool CDPSDKVodPullTask::_sdk_initialized = false;
CSSMutex CDPSDKVodPullTask::_sdk_init_mutex;

CDPSDKVodPullTask::CDPSDKVodPullTask(SDK_RECV_TASK_INFO& task_info):
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
}

int CDPSDKVodPullTask::init_sdk()
{
	CSSLocker lock(&_sdk_init_mutex);

	if(true == _sdk_initialized)
		return 0;

	if(DPSDK_RET_SUCCESS != DPSDK_Create(DPSDK_CORE_SDK_SERVER, _sdk_handler))
		return -1;

	_sdk_initialized = true;

	return 0;
}

#include <Poco/URI.h>
#include <Poco/NumberFormatter.h>
#include <Poco/String.h>

int CDPSDKVodPullTask::connect_device()
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
		//1000010 是设备 ID，该设备 ID 可以在平台 web 管理员端“基本配置-设备管理”页面的“编码”列查询。
		//$1 代表的是单元类型。 1 是编码单元、 2 是解码单元、 3 是报警输入单元、 4是报警输出单元。实时视频都是$1。
		//$0 默认全写 0，代表的是单元类型的序号。
		//$0 这个代表的是通道序号。从 0 开始计算。比如一个设备有 8 个通道，则通道序号是 0-7.

		//dsssdk://fym:88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0&begintime=1482474300&endtime=1482474397&fps=25
#if 0
		url = url.substr(url.find("://") + 3, url.length() - strlen("dsssdk://"));
		//fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0
		_username = url.substr(0, url.find("/"));
		_password = url.substr(url.find("/") + 1, url.find("@") - url.find("/") - 1);
		_ip = url.substr(url.find("@") + 1, url.find(":") - url.find("@") - 1);
		_port = MiscTools::parse_string_to_type<unsigned short>(url.substr(url.find(":") + 1, url.find_last_of("/") - url.find(":") - 1));
		_camera_id = url.substr(url.find("cameraid=") + 9, url.find("&channel") - url.find("cameraid=") - 9);
		_channel = url.substr(url.find("&channel=") + 9, url.find("&begintime") - url.find("&channel=") - 9);
		_camera_id += "$1$0$" + _channel;

		_begin_time = MiscTools::parse_string_to_type<unsigned long long>(url.substr(url.find("begintime=") + 10, url.find("&endtime") - url.find("begintime=") - 10));
		_end_time = MiscTools::parse_string_to_type<unsigned long long>(url.substr(url.find("endtime=") + 8, url.find("&fps") - url.find("endtime=") - 8));
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

			if ("cameraid" == key)
				_camera_id = val;
			else if ("channel" == key)
				_channel = val;
			else if ("begintime" == key)
				_begin_time = MiscTools::parse_string_to_type<unsigned long long>(val);
			else if ("endtime" == key)
				_end_time = MiscTools::parse_string_to_type<unsigned long long>(val);
			else if ("fps" == key)
				_fps = val;
		}

		_camera_id += "$1$0$" + _channel;
#endif

		_seq = -1;

		//string ip = "192.168.0.190";
		//unsigned short port = 9000;
		//string username = "fym";
		//string password = "88888888";

		Login_Info_t info = {0};
		memset(& info, 0, sizeof(Login_Info_t));
		
		memcpy(info.szIp, _ip.c_str(), _ip.length());
		info.nPort = _port;
		memcpy(info.szUsername, _username.c_str(), _username.length());
		memcpy(info.szPassword, _password.c_str(), _password.length());
		info.nProtocol = DPSDK_PROTOCOL_VERSION_II;
		info.iType = 1;

		ret = DPSDK_Login(_sdk_handler, &info);
		if(DPSDK_RET_SUCCESS != ret && DPSDK_RET_ALREADY_LOGIN != 4)
		{
			return ret;
		}
		
		/*typedef struct tagGetRecordStreamTimeInfo
		{
		char								szCameraId[DPSDK_CORE_CHL_ID_LEN];			// 通道ID
		dpsdk_check_right_e					nRight;										// 是否检测权限
		dpsdk_pb_mode_e						nMode;										// 录像流请求类型

		dpsdk_recsource_type_e				nSource;									// 录像源类型                  
		uint64_t							uBeginTime;									// 开始时间 
		uint64_t							uEndTime;									// 结束时间
		}Get_RecordStream_Time_Info_t;*/
		Get_RecordStream_Time_Info_t vod_info = {0};
		memset(&vod_info, 0, sizeof(Get_RecordStream_Time_Info_t));
		memcpy(vod_info.szCameraId, _camera_id.c_str(), _camera_id.length());
		vod_info.nRight = DPSDK_CORE_NOT_CHECK_RIGHT;
		vod_info.nMode = DPSDK_CORE_PB_MODE_NORMAL;
		vod_info.nSource = DPSDK_CORE_PB_RECSOURCE_DEVICE;//DPSDK_CORE_PB_RECSOURCE_PLATFORM;
		vod_info.uBeginTime = _begin_time;
		vod_info.uEndTime = _end_time;

#if 0
		struct tm {
			int tm_sec;     /* seconds after the minute - [0,59] */
			int tm_min;     /* minutes after the hour - [0,59] */
			int tm_hour;    /* hours since midnight - [0,23] */
			int tm_mday;    /* day of the month - [1,31] */
			int tm_mon;     /* months since January - [0,11] */
			int tm_year;    /* years since 1900 */
			int tm_wday;    /* days since Sunday - [0,6] */
			int tm_yday;    /* days since January 1 - [0,365] */
			int tm_isdst;   /* daylight savings time flag */
		};
#endif
#if 0
		struct tm beginT = {0, 0, 9, 9, 1, 117, 0, 0, 0};//2017-2-9 09:00:00
		struct tm endT = {59, 9, 9, 9, 1, 117, 0, 0, 0};//2017-2-9 09:09:59
		uint64_t uBeginTime = mktime(&beginT);
		uint64_t uEndTime = mktime(&endT);
		vod_info.uBeginTime = uBeginTime;//_begin_time;
		vod_info.uEndTime = uEndTime;//_end_time;
#endif
		
		ret = DPSDK_GetRecordStreamByTime(_sdk_handler, _seq, &vod_info, CDPSDKVodPullTask::MediaDataCallback, (void*)this);

		//int ret2 = DPSDK_SetPlaybackSpeed(_sdk_handler, _seq, DPSDK_CORE_PB_FAST8);
		//TRACE("\nSpeed: %d", ret2);
	}
	
	return ret;
}

int CDPSDKVodPullTask::disconnect_device()
{
	int ret1 = DPSDK_StopPlaybackBySeq(_sdk_handler, _seq);
	int ret2 = DPSDK_Logout(_sdk_handler);

	return 0;
}

void CDPSDKVodPullTask::on_recv_packet(unsigned char* data, int len)
{
	_last_packet_timestamp = timeGetTime();

	unsigned char* dat = (unsigned char*)(data + 40);
	int length = len - 40;

	int offset = 0;
	NAL_TYPE type =  CRTMPSession::get_video_packet_type(dat, length, offset);
	unsigned long timestamp = timeGetTime();

	//TRACE("\n<T %d>", type);

	if(NAL_INVALID == type && NAL_INVALID != _last_frame_type)
	{
		//TRACE("\n<I %d>", type);
		dat = (unsigned char*)(data);
		length = len;

		memcpy(_frame + _frame_pos, dat, length);
		_frame_pos += length;
	}
	else if(NAL_SLICE == type || NAL_SPS == type)
	{
		//TRACE("\n<P %d>", type);

		if(_frame_pos) on_recv_frame(_frame, _frame_pos, true);

		_frame_pos = 0;
		memcpy(_frame + _frame_pos, dat, length);
		_frame_pos += length;

		_last_frame_type = type;
	}
	else
	{
		TRACE("\nError");
	}
}

int32_t DPSDK_CALLTYPE CDPSDKVodPullTask::MediaDataCallback(int32_t nPDLLHandle, int32_t nSeq, int32_t nMediaType, const char* szNodeId, int32_t nParamVal, char* szData, int32_t nDataLen, void* pUserParam)
{
	if(2 != nMediaType) return 0;

	unsigned long id = (unsigned long)(pUserParam);

	//TRACE("\n%d %d %s %d %d", nMediaType, nSeq, szNodeId, (unsigned long)(pUserParam), nDataLen);

	//TRACE("\n%d %x %x %x %x %x", nMediaType, szData[40], szData[41], szData[42], szData[43], szData[44]);

	if(40 >= nDataLen)
	{
		TRACE("\n<3SHORT %d %d %x %x %x %x %x", nMediaType, nDataLen, szData[0], szData[1], szData[2], szData[3], szData[4]);
		return 0;
	}

	CDPSDKVodPullTask* task = (CDPSDKVodPullTask*)pUserParam;

	task->on_recv_packet((unsigned char*)szData, nDataLen);

	return 0;
}

SS_Error CDPSDKVodPullTask::on_done()
{
	CSDKRecvTask::on_done();

	_task_info.clear();

	int ret1 = DPSDK_StopPlaybackBySeq(_sdk_handler, _seq);
	int ret2 = DPSDK_Logout(_sdk_handler);

	_is_done = true;

	return SS_NoErr;
}

