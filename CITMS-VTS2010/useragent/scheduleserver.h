// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _SCHEDULE_SERVER_H_       
#define _SCHEDULE_SERVER_H_

#include "stdafx.h"
#include "GeneralDef.h"
#include <boost/pool/detail/singleton.hpp>//#include "singleton.h"
#include "Logger.h"
#include "MiscTool.h"
#include "ModuleManager.h"
#include "ConfigBox.h"
//#include "IDeviceAccess.h"
#include "TaskThread.h"
#include "UserAgent.h"
#include "JRTPSession.h"
#include "RegisterThread.h"

#include "TypeDef.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

//#define VIDEO_SAMPLE_INTERVAL	50
//#define VIDEO_WIDTH				640
//#define VIDEO_HEIGHT				480

namespace ScheduleServer
{
	class CScheduleServer
	{
	public:
		CScheduleServer();

		~CScheduleServer();

		SS_Error start(std::string path);

		SS_Error shutdown();

		void write_log(std::string& log, int level, bool show_on_screen);

		void wait_for_shutdown();

		void set_cur_path(const std::string& path) { _cur_path = path; };

		std::string get_cur_path() { return _cur_path; };

		//RTSPServerLib::CRTSPServer* get_rtsp_server() { return _rtsp_server; };

		void singleton_test();
		
		static void print_contex(const SS_CTX& ctx, const bool to_send);

		//开启服务
		void enable() { _enalble = true; }

		//关闭服务
		void disable()
		{
			_enalble = false;
		}

		std::string get_server_ip()
		{
			return _server_ip;
		}

		unsigned short get_server_port(unsigned long reference)
		{
			if(true == _server_recv_port_vec.empty())
				return 0;

			return _server_recv_port_vec[reference % _server_recv_port_vec.size()];
		}

	public:
		std::map<unsigned long, bool> _is_ua_idle;

	private:
		std::string _server_ip;
		std::vector<unsigned short> _server_recv_port_vec;

	private:
		volatile bool _enalble;//服务是否可用

		//程序所在文件夹绝对路径
		std::string _cur_path;

		//ICommunicator* _communicator;//ICE通讯器

		std::map<unsigned long, CUserAgent*> _ua_map;
		CSSMutex _ua_map_mutex;

		CRegisterThread* _register_thread;

	public:
		CRegisterThread* get_register_thread() { return _register_thread; }

	public:
		//增加任务
		//index为0则是随机选择任务线程，否则选择index模任务线程数取余的任务线程
		SS_Error add_task(CTask* task, unsigned long index);

		//UA登记
		SS_Error reg_ua(const unsigned long& id, USER_AGENT_TYPE type)
		{
			if(!id)
				return SS_NoErr;

			CSSLocker lock(&_ua_map_mutex);

			if(_ua_map.end() == _ua_map.find(id))
			{
				//第一次注册
				USER_AGENT_INFO info;
				info.id = id;
				info.type = type;

				CUserAgent* ua = new CUserAgent(info);
				//ua->update_alive_timestamp();
				_ua_map[ua->_info.id] = ua;
			}
			else
			{
				//已注册过
				CUserAgent* ua = _ua_map[id];

				if(NULL == ua)
					return SS_NoErr;

				ua->_info.type = type;
			}

			return SS_NoErr;
		}

		//获取UA指针
		inline CUserAgent* fetch_ua(const unsigned long& id)
		{
			CSSLocker lock(&_ua_map_mutex);

			if(!id || _ua_map.end() == _ua_map.find(id))
				return NULL;

			return _ua_map[id];
		}

		//获取UA指针
		CUserAgent* fetch_ua(const string& id)
		{
			return fetch_ua(MiscTools::parse_string_to_type<unsigned long>(id));
		}

		//统计并记录UA媒体数据
		void ua_statistics()
		{
		}

		//根据ID从_ua_map中删除UA同时销毁UA
		void remove_ua(const unsigned long& id)
		{
			CUserAgent* ua = NULL;

			{
				CSSLocker lock(&_ua_map_mutex);

				ua = _ua_map[id];
				_ua_map.erase(id);
			}
			
			delete ua;
			ua = NULL;
		}

		//删除全部UA
		void remove_all_ua()
		{
			CSSLocker lock(&_ua_map_mutex);

			for(auto iter = _ua_map.begin(); iter != _ua_map.end(); ++iter)
			{
				delete iter->second;
				iter->second = NULL;
			}

			_ua_map.clear();
		}

		//所有虚拟UA向SIP服务器注册并发送NAT探测包
		void ua_register_and_nat()
		{
			CSSLocker lock(&_ua_map_mutex);

			for(auto iter = _ua_map.begin(); iter != _ua_map.end(); ++iter)
			{
				//NAT探测
				//iter->second->nat_probe(SINGLETON(CConfigBox).get_property("ScheduleServerIP", "127.0.0.1"));

				//SIP注册
				//到期前会自动重新注册 iter->second->sip_register();
			}
		}

		int start_rtmp_push(unsigned long uuid);

		int stop_rtmp_push(unsigned long uuid);

	public:
		void dh_test();
		int rtsp_test();
		int rtsp_test2();
		int h264_test();
		int http_test();

	private:
		std::map<LiveCastRequest, LiveCastResponse> _livecast_map;
		CSSMutex _livecast_map_mutex;

		std::map<VODRequest, VODResponse> _vod_map;
		CSSMutex _vod_map_mutex;

	public:
		LiveCastResponse query_livecast_response(LiveCastRequest request)
		{
			CSSLocker lock(&_livecast_map_mutex);

			if(_livecast_map.end() == _livecast_map.find(request))
			{
				LiveCastResponse response;
				return response;
			}

			return _livecast_map[request];
		}

		LiveCastRequest query_livecast_request(string num)
		{
			CSSLocker lock(&_livecast_map_mutex);

			for(auto iter = _livecast_map.begin(); iter != _livecast_map.end(); ++iter)
			{
				if(iter->second.num == num)
					return iter->first;
			}

			LiveCastRequest request;
			return request;
		}

		LiveCastRequest query_livecast_request(unsigned long number)
		{
			CSSLocker lock(&_livecast_map_mutex);

			string num = MiscTools::parse_type_to_string<unsigned long>(number);

			for(auto iter = _livecast_map.begin(); iter != _livecast_map.end(); ++iter)
			{
				if(iter->second.num == num)
					return iter->first;
			}

			LiveCastRequest request;
			return request;
		}

		void insert_livecast_map(LiveCastRequest request, LiveCastResponse response)
		{
			CSSLocker lock(&_livecast_map_mutex);

			_livecast_map[request] = response;
		}

		void remove_livecast_map(LiveCastRequest request)
		{
			CSSLocker lock(&_livecast_map_mutex);

			_livecast_map.erase(request);
		}

		//////////////////////////////////////////////////////////////////////////
		VODResponse query_vod_response(VODRequest request)
		{
			CSSLocker lock(&_vod_map_mutex);

			if(_vod_map.end() == _vod_map.find(request))
			{
				VODResponse response;
				return response;
			}

			return _vod_map[request];
		}

		VODRequest query_vod_request(string num)
		{
			CSSLocker lock(&_vod_map_mutex);

			for(auto iter = _vod_map.begin(); iter != _vod_map.end(); ++iter)
			{
				if(iter->second.num == num)
					return iter->first;
			}

			VODRequest request;
			return request;
		}

		VODRequest query_vod_request(unsigned long number)
		{
			CSSLocker lock(&_vod_map_mutex);

			string num = MiscTools::parse_type_to_string<unsigned long>(number);

			for(auto iter = _vod_map.begin(); iter != _vod_map.end(); ++iter)
			{
				if(iter->second.num == num)
					return iter->first;
			}

			VODRequest request;
			return request;
		}

		void insert_vod_map(VODRequest request, VODResponse response)
		{
			CSSLocker lock(&_vod_map_mutex);

			_vod_map[request] = response;
		}

		void remove_vod_map(VODRequest request)
		{
			CSSLocker lock(&_vod_map_mutex);

			_vod_map.erase(request);
		}

	public:
		map<unsigned long, CTask*> _pull_task_map;
		map<unsigned long, CTask*> _push_task_map;

		CSSMutex _shutdown_cast_mutex;
		void shutdown_cast(unsigned long id);
		void update_expire(unsigned long id, unsigned long expire);

	};
}

#endif//_SCHEDULE_SERVER_H_