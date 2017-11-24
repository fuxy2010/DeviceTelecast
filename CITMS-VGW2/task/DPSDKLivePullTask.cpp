// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "DPSDKLivePullTask.h"
#include "MiscTool.h"
#include "TimeConsuming.h"

using namespace ScheduleServer;

int32_t CDPSDKLivePullTask::_sdk_handler = -1;
bool CDPSDKLivePullTask::_sdk_initialized = false;
CSSMutex CDPSDKLivePullTask::_sdk_init_mutex;

CDPSDKLivePullTask::CDPSDKLivePullTask(SDK_RECV_TASK_INFO& task_info):
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

int CDPSDKLivePullTask::init_sdk()
{
	CSSLocker lock(&_sdk_init_mutex);

	if(true == _sdk_initialized)
		return 0;

	if(DPSDK_RET_SUCCESS != DPSDK_Create(DPSDK_CORE_SDK_SERVER, _sdk_handler))
		return -1;

	_sdk_initialized = true;

	return 0;
}

int CDPSDKLivePullTask::connect_device()
{
#ifdef PUSH_VIDEO_FILE
	return 0;
#endif

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

		//dsssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0
		url = url.substr(url.find("://") + 3, url.length() - strlen("dsssdk://"));
		//fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0
		_username = url.substr(0, url.find("/"));
		_password = url.substr(url.find("/") + 1, url.find("@") - url.find("/") - 1);
		_ip = url.substr(url.find("@") + 1, url.find(":") - url.find("@") - 1);
		_port = MiscTools::parse_string_to_type<unsigned short>(url.substr(url.find(":") + 1, url.find_last_of("/") - url.find(":") - 1));
		_camera_id = url.substr(url.find("cameraid=") + 9, url.find("&channel") - url.find("cameraid=") - 9);
		_channel = url.substr(url.find("&channel=") + 9, url.length() - url.find("&channel=") - 9);
		_camera_id += "$1$0$" + _channel;

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

		/*typedef struct tagGetRealStreamInfo
		{
		char								szCameraId[DPSDK_CORE_CHL_ID_LEN];			// 通道ID
		dpsdk_check_right_e					nRight;										// 是否检测权限
		dpsdk_stream_type_e					nStreamType;								// 码流类型
		dpsdk_media_type_e					nMediaType;									// 媒体类型
		dpsdk_trans_type_e					nTransType;									// 传输类型
		}Get_RealStream_Info_t;*/
		Get_RealStream_Info_t stream_info = {0};
		memset(&stream_info, 0, sizeof(Get_RealStream_Info_t));
		memcpy(stream_info.szCameraId, _camera_id.c_str(), _camera_id.length());//memcpy(stream_info.szCameraId, "1000004$1$0$0", strlen("1000004$1$0$0"));
		stream_info.nRight = DPSDK_CORE_NOT_CHECK_RIGHT;
		stream_info.nStreamType = DPSDK_CORE_STREAMTYPE_MAIN;
		stream_info.nMediaType = DPSDK_CORE_MEDIATYPE_VIDEO;
		stream_info.nTransType = DPSDK_CORE_TRANSTYPE_TCP;

		ret = DPSDK_GetRealStream( _sdk_handler, _seq, &stream_info, CDPSDKLivePullTask::MediaDataCallback, (void*)this);
	}
	
	return ret;
}

int CDPSDKLivePullTask::disconnect_device()
{
	int ret1 = DPSDK_StopRealplayBySeq(_sdk_handler, _seq);
	int ret2 = DPSDK_Logout(_sdk_handler);

	return 0;
}

void CDPSDKLivePullTask::on_recv_packet(unsigned char* data, int len)
{
	_last_packet_timestamp = timeGetTime();

	if(40 >= len)
	{
		memcpy(_frame + _frame_pos, data, len);
		_frame_pos += len;

		return;
	}

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

int32_t DPSDK_CALLTYPE CDPSDKLivePullTask::MediaDataCallback(int32_t nPDLLHandle, int32_t nSeq, int32_t nMediaType, const char* szNodeId, int32_t nParamVal, char* szData, int32_t nDataLen, void* pUserParam)
{
	if(DPSDK_CORE_MEDIATYPE_VIDEO != nMediaType) return 0;

	unsigned long id = (unsigned long)(pUserParam);

	//TRACE("\n%d %d %s %d %d", nMediaType, nSeq, szNodeId, (unsigned long)(pUserParam), nDataLen);

	//TRACE("\n%d %x %x %x %x %x", nMediaType, szData[40], szData[41], szData[42], szData[43], szData[44]);

	if(40 >= nDataLen)
	{
		TRACE("\n<2SHORT %d %x %x %x %x %x", nMediaType, szData[0], szData[1], szData[2], szData[3], szData[4]);
		//return 0;
	}

	CDPSDKLivePullTask* task = (CDPSDKLivePullTask*)pUserParam;

	task->on_recv_packet((unsigned char*)szData, nDataLen);

	/*{
		unsigned char* dat = (unsigned char*)(szData + 40);
		int length = nDataLen - 40;

		int offset = 0;
		NAL_TYPE type =  CRTMPSession::get_video_packet_type(dat, length, offset);
		unsigned long timestamp = timeGetTime();

		//TRACE("\n<T %d>", type);

		if(NAL_INVALID == type && NAL_INVALID != _last_frame_type)
		{
			//TRACE("\n<I %d>", type);
			dat = (unsigned char*)(szData);
			length = nDataLen;

			memcpy(_frame + _frame_pos, dat, length);
			_frame_pos += length;
		}
		else if(NAL_SLICE == type || NAL_SPS == type)
		{
			//TRACE("\n<P %d>", type);

			if(_frame_pos) on_recv_packet(_frame, _frame_pos, id);

			_frame_pos = 0;
			memcpy(_frame + _frame_pos, dat, length);
			_frame_pos += length;

			_last_frame_type = type;
		}
		else
		{
			TRACE("\nError");
		}
	}*/

	return 0;
}

SS_Error CDPSDKLivePullTask::on_done()
{
	_task_info.clear();

	_is_done = true;

	int ret1 = DPSDK_StopRealplayBySeq(_sdk_handler, _seq);
	int ret2 = DPSDK_Logout(_sdk_handler);

	return SS_NoErr;
}

