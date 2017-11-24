#include "StdAfx.h"
#include "HttpService.h"

using namespace HttpServer;

#define HTTP_SERVICE_STATUS_UNKNOW 0		// 未知状态
#define HTTP_SERVICE_STATUS_STARTING 1		// 连接中
#define HTTP_SERVICE_STATUS_RUNNING 2		// 已连接

class CCitmsErrorHandler : public Poco::ErrorHandler
{
public:
	CCitmsErrorHandler() {}
	/// Creates the ErrorHandler.

	virtual ~CCitmsErrorHandler() {}
	/// Destroys the ErrorHandler.

	virtual void exception(const Poco::Exception& exc) {
		// TODO
	}

	virtual void exception(const std::exception& exc) {
		// TODO
	}
};

#include "MiscTool.h"
#include "resource.h"
#include "Locker.h"
#include "TypeDef.h"
#include "json.h"
#include "scheduleserver.h"
#include "RTSPRecvTask.h"
#include "RTMPPushTask.h"
#include "DPSDKLivePullTask.h"
#include "DPSDKVodPullTask.h"
#include "HKPSDKLivePullTask.h"
#include "HKPSDKVodPullTask.h"

using namespace ScheduleServer;

static CCitmsErrorHandler * sErrorHandler = new CCitmsErrorHandler;

class CCitmsHttpRequestHandler : public Poco::Net::AbstractHTTPRequestHandler
{
public:
	CCitmsHttpRequestHandler() :
	_status_edit(NULL)
	{
		_status_edit = (CEdit*)(AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_EDIT_HTTP_SERVICE));//->SetWindowText(decodedURI.c_str());
	}
	virtual ~CCitmsHttpRequestHandler() {}

private:
	CEdit* _status_edit;
	static string _status;
	static CSSMutex _status_mutex;

	static CSSMutex _livecast_mutex;
	static CSSMutex _vod_mutex;

	//static map<unsigned long, CTask*> _pull_task_map;
	//static map<unsigned long, CTask*> _push_task_map;

private:
	string return_response(int ErrCode, string ErrorDesc, string Data, unsigned long delayms = 0)
	{
		Json::Value json_value;
		json_value["ErrCode"] = Json::Value(ErrCode);
		json_value["ErrorDesc"] = Json::Value(ErrorDesc);
		json_value["Data"] = Json::Value(Data);

		if(!ErrCode)
			Sleep(delayms);

		Json::FastWriter fast_writer;
		return fast_writer.write(json_value);
	}

#if 0
#else
	string on_livecast_start(string uri)
	{
		CSSLocker lock(&_livecast_mutex);
		//http://127.0.0.1:80/api/startrealplay?connectstring=rtsp://192.168.0.190:9090/dss/monitor/params?cameraid=1000004%240&substream=1

		size_t pos = uri.find("connectstring=") + 14;
		string url = uri.substr(pos, uri.length() - pos);

		TRACE("\n<LIVE START %s", url.c_str());

		if(1024 < url.length())
		{
			return return_response(1004, "Too long request url.", "");
		}

		if(NULL == strstr(url.c_str(), "RTSP") && NULL == strstr(url.c_str(), "rtsp")) return return_response(1004, "No RTSP url.", "");

		LiveCastRequest req;
		req.url = url;

		LiveCastResponse res = SINGLETON(CScheduleServer).query_livecast_response(req);

		if(false == res.num.empty())
		{
			Json::Value json_value;
			json_value["StreamNo"] = Json::Value(res.num);
			json_value["StreamUrl"] = Json::Value(res.url);
			json_value["ExpireTime"] = Json::Value(res.expire);

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
		}
		else
		{
			unsigned long id = timeGetTime() & 0xffffff;
			string hls_url = "";
			string rtmp_url = "";
			CRTMPPushTask* rtmp_push_task = NULL;
			CRTSPRecvTask* rtsp_recv_task = NULL;

			//RTMP
			{
				RTMP_PUSH_TASK_INFO task_info;

				task_info.task_id = id;
				task_info.ua_id = id;

				task_info.rtmp_url = "rtmp://";
				task_info.rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				task_info.rtmp_url += ":" + SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
				task_info.rtmp_url += "/hls/";
				task_info.rtmp_url += MiscTools::parse_type_to_string<unsigned long>(task_info.ua_id);
				//task_info.rtmp_url = "rtmp://121.41.15.6/hls/123";
				//task_info.rtmp_url = "rtmp://192.168.0.123/hls/123";

				//创建模拟UA发送音视频任务
				rtmp_push_task = new CRTMPPushTask(task_info);

				if(false == rtmp_push_task->is_initialized())
				{
					return return_response(2001, "Failed in rtmp push.", "");
				}

				if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtmp_push_task, task_info.task_id))
				{
					delete rtmp_push_task;
					rtmp_push_task = NULL;

					return return_response(3001, "Failed in rtmp push 2.", "");
				}
			}

			//RTSP
			{
				RTSP_RECV_TASK_INFO task_info;

				task_info.task_id = id + 1;
				task_info.ua_id = id;

				task_info.rtsp_url = req.url;

				//创建模拟UA发送音视频任务
				rtsp_recv_task = new CRTSPRecvTask(task_info);

				if(false == rtsp_recv_task->is_initialized())
				{
					rtmp_push_task->shutdown();
					return return_response(2001, "Failed in creating rtsp session.", "");
				}

				if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtsp_recv_task, task_info.task_id))
				{
					delete rtsp_recv_task;
					rtsp_recv_task = NULL;

					rtmp_push_task->shutdown();

					return return_response(3001, "Failed in creating rtsp session 2.", "");
				}

				hls_url = "http://";
				hls_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				hls_url += ":";
				hls_url += SINGLETON(CConfigBox).get_property("HLSServerPort", "8080");
				hls_url += "/hls/";
				hls_url += MiscTools::parse_type_to_string<unsigned long>(id);
				hls_url += ".m3u8";

				rtmp_url = "rtmp://";
				rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				rtmp_url += ":";
				rtmp_url += SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
				rtmp_url += "/hls/";
				rtmp_url += MiscTools::parse_type_to_string<unsigned long>(id);
			}

			//success////////////////////////////////////////////////////////////////////////
			SINGLETON(CScheduleServer)._push_task_map[id] = rtmp_push_task;
			SINGLETON(CScheduleServer)._pull_task_map[id] = rtsp_recv_task;

			res.num = MiscTools::parse_type_to_string<unsigned long>(id);
			res.url = hls_url;
			res.url2 = rtmp_url;
			res.expire = "300";
			SINGLETON(CScheduleServer).insert_livecast_map(req, res);

			Json::Value json_value;
			json_value["StreamNo"] = Json::Value(MiscTools::parse_type_to_string<unsigned long>(id));
			json_value["StreamUrl"] = Json::Value(hls_url);
			json_value["StreamUrl2"] = Json::Value(rtmp_url);
			json_value["ExpireTime"] = Json::Value("300");

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value), 6000);//return fast_writer.write(json_value);
		}
		
		return "";
	}

	string on_livecast_keep(string uri)
	{
		CSSLocker lock(&_livecast_mutex);

		//http://127.0.0.1:80/api/keeprealplay?streamno=[StreamNo]&expiretime=[ExpireTime]

		size_t pos = uri.find("streamno=") + 9;
		string streamno = uri.substr(pos, uri.find("&expiretime=") - pos);

		pos = uri.find("expiretime=") + 11;
		string expiretime = uri.substr(pos, uri.length() - pos);

		TRACE("\n<LIVE KEEP %s", expiretime.c_str());
		
		unsigned long stream_no = MiscTools::parse_string_to_type<unsigned long>(streamno);
		unsigned long expire = MiscTools::parse_string_to_type<unsigned long>(expiretime);

		if(!stream_no || !expire)
		{
			return return_response(1002, "Invalid request.", "");
		}

		if(true == SINGLETON(CScheduleServer).query_livecast_request(streamno).url.empty())
		{
			return return_response(1002, "Invalid StreamNo.", "");
		}

		SINGLETON(CScheduleServer).update_expire(stream_no, expire);
		
		Json::Value json_value;
		json_value["StreamNo"] = Json::Value(MiscTools::parse_type_to_string<unsigned long>(stream_no));
		json_value["ExpireTime"] = Json::Value(MiscTools::parse_type_to_string<unsigned long>(expire));

		Json::FastWriter fast_writer;
		return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
	}

	string on_livecast_stop(string uri)
	{
		CSSLocker lock(&_livecast_mutex);
		//http://127.0.0.1:80/api/stoprealplay?streamno=[StreamNo]

		size_t pos = uri.find("streamno=") + 9;
		string streamno = uri.substr(pos, uri.length() - pos);

		TRACE("\n<LIVE STOP %s", streamno.c_str());

		unsigned long stream_no = MiscTools::parse_string_to_type<unsigned long>(streamno);

		if(!stream_no)
		{
			return return_response(1002, "Invalid request.", "");
		}

		if(true == SINGLETON(CScheduleServer).query_livecast_request(streamno).url.empty())
		{
			return return_response(1002, "Invalid StreamNo.", "");
		}

		SINGLETON(CScheduleServer).shutdown_cast(stream_no);

		Json::Value json_value;
		json_value["StreamStatus"] = Json::Value("CLOSED");

		Json::FastWriter fast_writer;
		return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
	}
#endif
	string on_vod_start(string uri)
	{
		CSSLocker lock(&_vod_mutex);
		//http://127.0.0.1:80/api/startvodplay?connectstring=rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004%247%26begintime=1482474300%26endtime=1482474397%26substream=1
		//rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004%247%26begintime=1482474300%26endtime=1482474397%26substream=1
		//http://127.0.0.1:80/api/startvodplay?connectstring=rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004%241%26begintime=1483411300%26endtime=1483411600%26substream=1
		//rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004%241%26begintime=1483411300%26endtime=1483411600%26substream=1		

		size_t pos = uri.find("connectstring=") + 14;
		string url = uri.substr(pos, uri.length() - pos);

		TRACE("\n<VOD START %s", url.c_str());

		if(1024 < url.length())
		{
			return return_response(1004, "Too long request url.", "");
		}

		if(NULL == strstr(url.c_str(), "RTSP") && NULL == strstr(url.c_str(), "rtsp")) return return_response(1004, "No RTSP url.", "");

		VODRequest req;
		req.url = url;

		VODResponse res = SINGLETON(CScheduleServer).query_vod_response(req);

		if(false == res.num.empty())
		{
			Json::Value json_value;
			json_value["StreamNo"] = Json::Value(res.num);
			json_value["StreamUrl"] = Json::Value(res.url);
			json_value["Duration"] = Json::Value(res.duration);

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
		}
		else
		{
			unsigned long id = timeGetTime() & 0xffffff;
			string hls_url = "";
			string rtmp_url = "";
			CRTMPPushTask* rtmp_push_task = NULL;
			CRTSPRecvTask* rtsp_recv_task = NULL;

			//RTMP
			{
				RTMP_PUSH_TASK_INFO task_info;

				task_info.task_id = id;
				task_info.ua_id = id;

				task_info.rtmp_url = "rtmp://";
				task_info.rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				task_info.rtmp_url += ":" + SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
				task_info.rtmp_url += "/hls/";
				task_info.rtmp_url += MiscTools::parse_type_to_string<unsigned long>(task_info.ua_id);
				//task_info.rtmp_url = "rtmp://121.41.15.6/hls/123";
				//task_info.rtmp_url = "rtmp://192.168.0.123/hls/123";

				//创建模拟UA发送音视频任务
				rtmp_push_task = new CRTMPPushTask(task_info);

				if(false == rtmp_push_task->is_initialized())
				{
					return return_response(2001, "Failed in rtmp push.", "");
				}

				if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtmp_push_task, task_info.task_id))
				{
					delete rtmp_push_task;
					rtmp_push_task = NULL;

					return return_response(3001, "Failed in rtmp push 2.", "");
				}
			}

			//RTSP
			{
				RTSP_RECV_TASK_INFO task_info;

				task_info.task_id = id + 1;
				task_info.ua_id = id;

				task_info.rtsp_url = req.url;

				//创建模拟UA发送音视频任务
				rtsp_recv_task = new CRTSPRecvTask(task_info);

				if(false == rtsp_recv_task->is_initialized())
				{
					rtmp_push_task->shutdown();
					return return_response(2001, "Failed in creating rtsp session.", "");
				}

				if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtsp_recv_task, task_info.task_id))
				{
					delete rtsp_recv_task;
					rtsp_recv_task = NULL;

					rtmp_push_task->shutdown();

					return return_response(3001, "Failed in creating rtsp session 2.", "");
				}

				hls_url = "http://";
				hls_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				hls_url += ":";
				hls_url += SINGLETON(CConfigBox).get_property("HLSServerPort", "8080");
				hls_url += "/hls/";
				hls_url += MiscTools::parse_type_to_string<unsigned long>(id);
				hls_url += ".m3u8";

				rtmp_url = "rtmp://";
				rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				rtmp_url += ":";
				rtmp_url += SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
				rtmp_url += "/hls/";
				rtmp_url += MiscTools::parse_type_to_string<unsigned long>(id);
			}

			//success////////////////////////////////////////////////////////////////////////
			SINGLETON(CScheduleServer)._push_task_map[id] = rtmp_push_task;
			SINGLETON(CScheduleServer)._pull_task_map[id] = rtsp_recv_task;

			unsigned long begintime = MiscTools::parse_string_to_type<unsigned long>(url.substr(url.find("begintime=") + 10, url.length() - url.find("begintime=") - 10 - url.find("%26endtime")));
			unsigned long endtime = MiscTools::parse_string_to_type<unsigned long>(url.substr(url.find("endtime=") + 8, url.length() - url.find("endtime=") - 8 - url.find("%26substream")));

			res.num = MiscTools::parse_type_to_string<unsigned long>(id);
			res.url = hls_url;
			res.url2 = rtmp_url;
			res.duration = MiscTools::parse_type_to_string<unsigned long>(endtime - begintime);
			SINGLETON(CScheduleServer).insert_vod_map(req, res);

			rtmp_push_task->update_expire(endtime - begintime + 5);
			rtsp_recv_task->update_expire(endtime - begintime + 5);

			Json::Value json_value;
			json_value["StreamNo"] = Json::Value(MiscTools::parse_type_to_string<unsigned long>(id));
			json_value["StreamUrl"] = Json::Value(hls_url);
			json_value["StreamUrl2"] = Json::Value(rtmp_url);
			json_value["Duration"] = Json::Value(res.duration);

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value), 6000);//return fast_writer.write(json_value);
		}

		return "";
	}

	string on_vod_stop(string uri)
	{
		CSSLocker lock(&_vod_mutex);
		//http://127.0.0.1:80/api/stopvodplay?streamno=[StreamNo]

		size_t pos = uri.find("streamno=") + 9;
		string streamno = uri.substr(pos, uri.length() - pos);

		TRACE("\n<VOD STOP %s", streamno.c_str());

		unsigned long stream_no = MiscTools::parse_string_to_type<unsigned long>(streamno);

		if(!stream_no)
		{
			return return_response(1002, "Invalid request.", "");
		}

		SINGLETON(CScheduleServer).shutdown_cast(stream_no);

		Json::Value json_value;
		json_value["StreamStatus"] = Json::Value("CLOSED");

		Json::FastWriter fast_writer;
		return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
	}

	string on_vod_seek(string uri)
	{
		CSSLocker lock(&_vod_mutex);
		//http://127.0.0.1:80/api/seekvodplay?streamno=[StreamNo]&seektime=[SeekTime]
		//rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004%247%26begintime=1482474300%26endtime=1482474397%26substream=1
		//dsssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0&begintime=1486569600&endtime=1486655999

		string streamno = uri.substr(uri.find("streamno=") + 9, uri.find("&seektime") - uri.find("streamno=") - 9);
		string seektime = uri.substr(uri.find("seektime=") + 9, uri.length() - uri.find("seektime=") - 9);

		string former_url = SINGLETON(CScheduleServer).query_vod_request(streamno).url;
		if(true == former_url.empty())
		{
			return return_response(1001, "Invalid StreamNo.", "");
		}

		if(string::npos != former_url.find("rtsp"))
		{
			//rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004%247%26begintime=1482474300%26endtime=1482474397%26substream=1

			unsigned long begintime = MiscTools::parse_string_to_type<unsigned long>(former_url.substr(former_url.find("begintime=") + 10, 10));//former_url.length() - former_url.find("begintime=") - 10 - former_url.find("%26endtime")));
			unsigned long endtime = MiscTools::parse_string_to_type<unsigned long>(former_url.substr(former_url.find("endtime=") + 8, 10));//former_url.length() - former_url.find("endtime=") - 8 - former_url.find("%26substream")));
			unsigned long offset = MiscTools::parse_string_to_type<unsigned long>(seektime);
			begintime += offset;

			if(endtime <= begintime)
			{
				return return_response(1001, "Invalid SeekTime.", "");
			}

			string new_url = former_url.substr(0, former_url.find("begintime=") + 10);
			new_url += MiscTools::parse_type_to_string<unsigned long>(begintime);
			//new_url += former_url.substr(former_url.find("%26endtime="), former_url.length() - former_url.find("%26endtime="));
			new_url += former_url.substr(former_url.find("begintime=") + 10 + 10, former_url.length() - former_url.find("begintime=") - 10 - 10);

			CRTSPRecvTask* rtsp_task = dynamic_cast<CRTSPRecvTask*>(SINGLETON(CScheduleServer)._pull_task_map[MiscTools::parse_string_to_type<unsigned long>(streamno)]);
			if(NULL != rtsp_task)
			{
				rtsp_task->pause();
				rtsp_task->restart(new_url);
			}

			Json::Value json_value;
			json_value["StreamStatus"] = Json::Value("PUSHING");
			json_value["StreamNo"] = Json::Value(streamno);

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
		}
		else if(string::npos != former_url.find("dsssdk"))
		{
			//dsssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0&begintime=1486569600&endtime=1486655999

			unsigned long begintime = MiscTools::parse_string_to_type<unsigned long>(former_url.substr(former_url.find("begintime=") + 10, 10));//former_url.length() - former_url.find("begintime=") - 10 - former_url.find("%26endtime")));
			unsigned long endtime = MiscTools::parse_string_to_type<unsigned long>(former_url.substr(former_url.find("endtime=") + 8, 10));//former_url.length() - former_url.find("endtime=") - 8 - former_url.find("%26substream")));
			unsigned long offset = MiscTools::parse_string_to_type<unsigned long>(seektime);
			begintime += offset;

			if(endtime <= begintime)
			{
				return return_response(1001, "Invalid SeekTime.", "");
			}

			string new_url = former_url.substr(0, former_url.find("begintime=") + 10);
			new_url += MiscTools::parse_type_to_string<unsigned long>(begintime);
			//new_url += former_url.substr(former_url.find("&endtime="), former_url.length() - former_url.find("&endtime="));
			new_url += former_url.substr(former_url.find("begintime=") + 10 + 10, former_url.length() - former_url.find("begintime=") - 10 - 10);

			CDPSDKVodPullTask* task = dynamic_cast<CDPSDKVodPullTask*>(SINGLETON(CScheduleServer)._pull_task_map[MiscTools::parse_string_to_type<unsigned long>(streamno)]);
			if(NULL != task)
			{
				task->pause();
				task->restart(new_url);
			}

			Json::Value json_value;
			json_value["StreamStatus"] = Json::Value("PUSHING");
			json_value["StreamNo"] = Json::Value(streamno);

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
		}

		return return_response(1001, "Invalid Request URL.", "");
	}

	//////////////////////////////////////////////////////////////////////////
	string on_livecast_start_sdk(string uri)
	{
		CSSLocker lock(&_livecast_mutex);
		//http://127.0.0.1:80/api/startrealplay?connectstring=dsssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0
		//dsssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0

		//http://127.0.0.1:80/api/startrealplay?connectstring=cmssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004
		//cmssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004

		size_t pos = uri.find("connectstring=") + 14;
		string url = uri.substr(pos, uri.length() - pos);

		TRACE("\n<LIVE SDK START %s", url.c_str());

		if(1024 < url.length())
		{
			return return_response(1004, "Too long request url.", "");
		}

		if(NULL == strstr(url.c_str(), "dsssdk")
			&& NULL == strstr(url.c_str(), "DSSSDK")
			&& NULL == strstr(url.c_str(), "cmssdk")
			&& NULL == strstr(url.c_str(), "CMSSDK"))
			return return_response(1004, "No SDK url.", "");

		LiveCastRequest req;
		req.url = url;

		LiveCastResponse res = SINGLETON(CScheduleServer).query_livecast_response(req);

		if(false == res.num.empty())
		{
			Json::Value json_value;
			json_value["StreamNo"] = Json::Value(res.num);
			json_value["StreamUrl"] = Json::Value(res.url);
			json_value["ExpireTime"] = Json::Value(res.expire);

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
		}
		else
		{
			unsigned long id = timeGetTime() & 0xffffff;
			string hls_url = "";
			string rtmp_url = "";
			CRTMPPushTask* rtmp_push_task = NULL;
			CSDKRecvTask* sdk_recv_task = NULL;//CDPSDKLivePullTask* sdk_recv_task = NULL;

			//RTMP
			{
				RTMP_PUSH_TASK_INFO task_info;

				task_info.task_id = id;
				task_info.ua_id = id;

				task_info.rtmp_url = "rtmp://";
				task_info.rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				task_info.rtmp_url += ":" + SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
				task_info.rtmp_url += "/hls/";
				task_info.rtmp_url += MiscTools::parse_type_to_string<unsigned long>(task_info.ua_id);
				//task_info.rtmp_url = "rtmp://121.41.15.6/hls/123";
				//task_info.rtmp_url = "rtmp://192.168.0.123/hls/123";

				//创建模拟UA发送音视频任务
				rtmp_push_task = new CRTMPPushTask(task_info);

				if(false == rtmp_push_task->is_initialized())
				{
					return return_response(2001, "Failed in rtmp push.", "");
				}

				if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtmp_push_task, task_info.task_id))
				{
					delete rtmp_push_task;
					rtmp_push_task = NULL;

					return return_response(3001, "Failed in rtmp push 2.", "");
				}
			}

			//SDK
			{
				SDK_RECV_TASK_INFO task_info;

				task_info.task_id = id + 1;
				task_info.ua_id = id;

				task_info.sdk_url = req.url;

				//创建模拟UA发送音视频任务
				//sdk_recv_task = new CDPSDKLivePullTask(task_info);
				if(NULL != strstr(url.c_str(), "dsssdk") || NULL != strstr(url.c_str(), "DSSSDK"))
					sdk_recv_task = new CDPSDKLivePullTask(task_info);
				else if(NULL != strstr(url.c_str(), "cmssdk") || NULL != strstr(url.c_str(), "CMSSDK"))
					sdk_recv_task = new CHKPSDKLivePullTask(task_info);

				if(false == sdk_recv_task->is_initialized())
				{
					rtmp_push_task->shutdown();
					return return_response(2001, "Failed in creating sdk session.", "");
				}

				if(SS_NoErr != SINGLETON(CScheduleServer).add_task(sdk_recv_task, task_info.task_id))
				{
					delete sdk_recv_task;
					sdk_recv_task = NULL;

					rtmp_push_task->shutdown();

					return return_response(3001, "Failed in creating rtsp session 2.", "");
				}

				hls_url = "http://";
				hls_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				hls_url += ":";
				hls_url += SINGLETON(CConfigBox).get_property("HLSServerPort", "8080");
				hls_url += "/hls/";
				hls_url += MiscTools::parse_type_to_string<unsigned long>(id);
				hls_url += ".m3u8";

				rtmp_url = "rtmp://";
				rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				rtmp_url += ":";
				rtmp_url += SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
				rtmp_url += "/hls/";
				rtmp_url += MiscTools::parse_type_to_string<unsigned long>(id);
			}

			//success////////////////////////////////////////////////////////////////////////
			SINGLETON(CScheduleServer)._push_task_map[id] = rtmp_push_task;
			SINGLETON(CScheduleServer)._pull_task_map[id] = sdk_recv_task;

			res.num = MiscTools::parse_type_to_string<unsigned long>(id);
			res.url = hls_url;
			res.url2 = rtmp_url;
			res.expire = "300";
			SINGLETON(CScheduleServer).insert_livecast_map(req, res);

			Json::Value json_value;
			json_value["StreamNo"] = Json::Value(MiscTools::parse_type_to_string<unsigned long>(id));
			json_value["StreamUrl"] = Json::Value(hls_url);
			json_value["StreamUrl2"] = Json::Value(rtmp_url);
			json_value["ExpireTime"] = Json::Value("300");

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value), 6000);//return fast_writer.write(json_value);
		}
		
		return "";
	}

	string on_vod_start_sdk(string uri)
	{
		CSSLocker lock(&_vod_mutex);		
		//http://127.0.0.1:80/api/startvodplay?connectstring=dsssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0&begintime=1486569600&endtime=1486655999
		//dsssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&channel=0&begintime=1486569600&endtime=1486655999

		//http://127.0.0.1:80/api/startvodplay?connectstring=cmssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&condition=1&begintime=1482474300&endtime=1482474397
		//cmssdk://fym/88888888@192.168.0.190:9000/params?cameraid=1000004&condition=1&begintime=1482474300&endtime=1482474397

		size_t pos = uri.find("connectstring=") + 14;
		string url = uri.substr(pos, uri.length() - pos);

		TRACE("\n<VOD START %s", url.c_str());

		if(1024 < url.length())
		{
			return return_response(1004, "Too long request url.", "");
		}

		if(NULL == strstr(url.c_str(), "dsssdk")
			&& NULL == strstr(url.c_str(), "DSSSDK")
			&& NULL == strstr(url.c_str(), "cmssdk")
			&& NULL == strstr(url.c_str(), "CMSSDK"))
			return return_response(1004, "No SDK url.", "");

		VODRequest req;
		req.url = url;

		VODResponse res = SINGLETON(CScheduleServer).query_vod_response(req);

		if(false == res.num.empty())
		{
			Json::Value json_value;
			json_value["StreamNo"] = Json::Value(res.num);
			json_value["StreamUrl"] = Json::Value(res.url);
			json_value["Duration"] = Json::Value(res.duration);

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value));//return fast_writer.write(json_value);
		}
		else
		{
			unsigned long id = timeGetTime() & 0xffffff;
			string hls_url = "";
			string rtmp_url = "";
			CRTMPPushTask* rtmp_push_task = NULL;
			CSDKRecvTask* sdk_recv_task = NULL;//CDPSDKVodPullTask* sdk_recv_task = NULL;

			//RTMP
			{
				RTMP_PUSH_TASK_INFO task_info;

				task_info.task_id = id;
				task_info.ua_id = id;

				task_info.rtmp_url = "rtmp://";
				task_info.rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				task_info.rtmp_url += ":" + SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
				task_info.rtmp_url += "/hls/";
				task_info.rtmp_url += MiscTools::parse_type_to_string<unsigned long>(task_info.ua_id);
				//task_info.rtmp_url = "rtmp://121.41.15.6/hls/123";
				//task_info.rtmp_url = "rtmp://192.168.0.123/hls/123";

				//创建模拟UA发送音视频任务
				rtmp_push_task = new CRTMPPushTask(task_info);

				if(false == rtmp_push_task->is_initialized())
				{
					return return_response(2001, "Failed in rtmp push.", "");
				}

				if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtmp_push_task, task_info.task_id))
				{
					delete rtmp_push_task;
					rtmp_push_task = NULL;

					return return_response(3001, "Failed in rtmp push 2.", "");
				}
			}

			//SDK
			{
				SDK_RECV_TASK_INFO task_info;

				task_info.task_id = id + 1;
				task_info.ua_id = id;

				task_info.sdk_url = req.url;

				//创建模拟UA发送音视频任务
				//sdk_recv_task = new CDPSDKVodPullTask(task_info);
				if(NULL != strstr(url.c_str(), "dsssdk") || NULL != strstr(url.c_str(), "DSSSDK"))
					sdk_recv_task = new CDPSDKVodPullTask(task_info);
				else if(NULL != strstr(url.c_str(), "cmssdk") || NULL != strstr(url.c_str(), "CMSSDK"))
					sdk_recv_task = new CHKPSDKVodPullTask(task_info);

				if(false == sdk_recv_task->is_initialized())
				{
					rtmp_push_task->shutdown();
					return return_response(2001, "Failed in creating sdk session.", "");
				}

				if(SS_NoErr != SINGLETON(CScheduleServer).add_task(sdk_recv_task, task_info.task_id))
				{
					delete sdk_recv_task;
					sdk_recv_task = NULL;

					rtmp_push_task->shutdown();

					return return_response(3001, "Failed in creating rtsp session 2.", "");
				}

				hls_url = "http://";
				hls_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				hls_url += ":";
				hls_url += SINGLETON(CConfigBox).get_property("HLSServerPort", "8080");
				hls_url += "/hls/";
				hls_url += MiscTools::parse_type_to_string<unsigned long>(id);
				hls_url += ".m3u8";

				rtmp_url = "rtmp://";
				rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
				rtmp_url += ":";
				rtmp_url += SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
				rtmp_url += "/hls/";
				rtmp_url += MiscTools::parse_type_to_string<unsigned long>(id);
			}

			//success////////////////////////////////////////////////////////////////////////
			SINGLETON(CScheduleServer)._push_task_map[id] = rtmp_push_task;
			SINGLETON(CScheduleServer)._pull_task_map[id] = sdk_recv_task;

			unsigned long begintime = MiscTools::parse_string_to_type<unsigned long>(url.substr(url.find("begintime=") + 10, url.length() - url.find("begintime=") - 10 - url.find("%26endtime")));
			unsigned long endtime = MiscTools::parse_string_to_type<unsigned long>(url.substr(url.find("endtime=") + 8, url.length() - url.find("endtime=") - 8 - url.find("%26substream")));

			res.num = MiscTools::parse_type_to_string<unsigned long>(id);
			res.url = hls_url;
			res.url2 = rtmp_url;
			res.duration = MiscTools::parse_type_to_string<unsigned long>(endtime - begintime);
			SINGLETON(CScheduleServer).insert_vod_map(req, res);

			rtmp_push_task->update_expire(endtime - begintime + 5);
			sdk_recv_task->update_expire(endtime - begintime + 5);

			Json::Value json_value;
			json_value["StreamNo"] = Json::Value(MiscTools::parse_type_to_string<unsigned long>(id));
			json_value["StreamUrl"] = Json::Value(hls_url);
			json_value["StreamUrl2"] = Json::Value(rtmp_url);
			json_value["Duration"] = Json::Value(res.duration);

			Json::FastWriter fast_writer;
			return return_response(0, "success", fast_writer.write(json_value), 6000);//return fast_writer.write(json_value);
		}

		return "";
	}

protected:
	virtual void run()
	{
		string uri = "";
		Poco::URI::decode(request().getURI(), uri, false);

		show_request(request().clientAddress().toString(),request().getMethod(), uri);

		if(request().getMethod() != Poco::Net::HTTPServerRequest::HTTP_GET)
		{
			sendErrorResponse(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, "Please use GET method");
			return;
		}
		
		string res = "";
		
		if(string::npos != uri.find("startrealplay"))
		{
			if(string::npos != uri.find("rtsp://"))
				res = on_livecast_start(uri);
			else if(string::npos != uri.find("dsssdk://"))
				res = on_livecast_start_sdk(uri);
			else if(string::npos != uri.find("cmssdk://"))
				res = on_livecast_start_sdk(uri);
		}
		else if(string::npos != uri.find("keeprealplay"))
		{
			res = on_livecast_keep(uri);
		}
		else if(string::npos != uri.find("stoprealplay"))
		{
			res = on_livecast_stop(uri);
		}
		else if(string::npos != uri.find("startvodplay"))
		{
			if(string::npos != uri.find("rtsp://"))
				res = on_vod_start(uri);
			else if(string::npos != uri.find("dsssdk://"))
				res = on_vod_start_sdk(uri);
			else if(string::npos != uri.find("cmssdk://"))
				res = on_vod_start_sdk(uri);
		}
		else if(string::npos != uri.find("stopvodplay"))
		{
			res = on_vod_stop(uri);
		}
		else if(string::npos != uri.find("seekvodplay"))
		{
			res = on_vod_seek(uri);
		}
		else
		{
			res = return_response(1001, "Invalid request.", "");
		}

		response().setStatusAndReason(Poco::Net::HTTPResponse::HTTP_OK);
		//Access-Control-Allow-Origin
		response().setAccessControl();
		response().setContentType("text/plain");		

		show_response(res.data());

		response().sendBuffer(res.data(), res.size());
	}

private:
	void show_request(string from, string method, string req)
	{
		CSSLocker lock(&_status_mutex);

		string log = "";
		log += "REQUEST: " + MiscTools::parse_now_to_string() + "\r\n";
		log += "FROM: " + from + "\r\n";
		log += "METHOD: " + method + "\r\n";
		log += req + "\r\n\r\n";

		if(100 < _status_edit->GetLineCount()) _status = "";

		_status = log + _status;
		_status_edit->SetWindowText(_status.c_str());
	}

	void show_response(string res)
	{
		CSSLocker lock(&_status_mutex);

		string log = "";
		log += "RESPONSE: " + MiscTools::parse_now_to_string() + "\r\n";
		log += res + "\r\n\r\n";

		_status = log + _status;
		_status_edit->SetWindowText(_status.c_str());
	}
};

CSSMutex CCitmsHttpRequestHandler::_livecast_mutex;
CSSMutex CCitmsHttpRequestHandler::_vod_mutex;

//map<unsigned long, CTask*> CCitmsHttpRequestHandler::_pull_task_map;
//map<unsigned long, CTask*> CCitmsHttpRequestHandler::_push_task_map;

string CCitmsHttpRequestHandler::_status = "";
CSSMutex CCitmsHttpRequestHandler::_status_mutex;

class _CCitms_RequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
	_CCitms_RequestHandlerFactory()
	{
	}

	Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request)
	{
		/*CStdString msg("Request from "
			+ request.clientAddress().toString()
			+ ": "
			+ request.getMethod()
			+ " "
			+ request.getURI()
			+ " "
			+ request.getVersion());
		*/
		return new CCitmsHttpRequestHandler;
	}

	virtual void handleRequest(Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp)
	{
		resp.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
		resp.setContentType("text/html");
		
		ostream & out = resp.send();
		out << "<h1>Hello World!</h1>";
	}
};

CHttpService & CHttpService::GetInstance()
{
	static CHttpService instance;
	return instance;
}

CHttpService::~CHttpService()
{
	if (m_server) {
		m_server->stopAll(true);
		delete m_server;
		m_server = NULL;
	}
	if (m_timer) {
		m_timer->cancel(true);
		delete m_timer;
		m_timer = nullptr;
	}
}

void CHttpService::Initialize()
{
	Poco::ErrorHandler::set(sErrorHandler);
	m_state = HTTP_SERVICE_STATUS_UNKNOW;
	if (!m_timer)
		m_timer = new Poco::Util::Timer;

	Poco::Util::TimerTask::Ptr pTask = new Poco::Util::TimerTaskAdapter<CHttpService>(*this, &CHttpService::DoStart);
	m_timer->schedule(pTask, 2000, 5000);	// 5秒钟尝试一次
}

void CHttpService::Uninitialize()
{
	if (m_timer) {
		m_timer->cancel(true);
		delete m_timer;
		m_timer = nullptr;
	}
}

void CHttpService::Start(const std::string & ipAddr, int port)
{
	CSSLocker lock(&m_srvMutex);
	if ((m_ipAddr == ipAddr) && (m_port == port) || (m_server == NULL)) {
		// 参数完全一致，正在连接或已经连接上是不做重新开启
		if (m_state != HTTP_SERVICE_STATUS_UNKNOW) {
			return;
		}
	}
	m_ipAddr = ipAddr;
	m_port = port;

	// TODO log "正在尝试启动HTTP服务……"
	// 关闭现有的服务
	if (m_server) {
		m_server->stopAll(true);
		delete m_server;
		m_server = NULL;
	}
	m_state = HTTP_SERVICE_STATUS_STARTING;
}

void CHttpService::Stop()
{
	CSSLocker lock(&m_srvMutex);
	if (m_server) {
		m_server->stopAll(true);
		delete m_server;
		m_server = NULL;
	}
	// TODO log "已停止HTTP服务"
	m_state = HTTP_SERVICE_STATUS_UNKNOW;
}

CHttpService::CHttpService()
{

}

void CHttpService::DoStart(Poco::Util::TimerTask& task)
{
	CSSLocker lock(&m_srvMutex);
	if (m_state == HTTP_SERVICE_STATUS_STARTING) {
		try
		{
			Poco::Net::SocketAddress saddr(m_ipAddr, m_port);
			Poco::Net::ServerSocket svs(saddr);
			Poco::Net::HTTPServerParams * params_ptr = new Poco::Net::HTTPServerParams;
			params_ptr->setMaxQueued(500);
			params_ptr->setMaxThreads(10);
			m_server = new Poco::Net::HTTPServer(new _CCitms_RequestHandlerFactory, svs, params_ptr);
			m_server->start();
			m_state = HTTP_SERVICE_STATUS_RUNNING;
			//TODO log "成功启动HTTP服务"
		}
		catch (const Poco::Exception& e) {
			//TODO log "HTTP服务启动失败: "
			if (m_server) {
				m_server->stopAll(true);
				delete m_server;
				m_server = NULL;
			}
		}
		catch (...) {
			if (m_server) {
				m_server->stopAll(true);
				delete m_server;
				m_server = NULL;
			}
		}
	}
}
