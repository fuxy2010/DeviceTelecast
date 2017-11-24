#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "Config.h"

namespace ScheduleServer
{
    // 媒体接入插件设备类型定义
    // 需要与平台数据库设备类型定义保存一致！
    // 如果设备类型修改，需要同时重新编译相应的插件！
    typedef INT DEVICE_TYPE;
    typedef enum
    {
        COMMONCAMERATYPE = 0,                           
        HKCAMERATYPE = 1,                                           // 流媒体1.0兼容设备类型定义(海康)
        
        DHCAMERATYPE = 3,                                           // 大华
        
        HKCARCAMERATYPE = 10,                                       // 海康车载
        HKPCCAMERATYPE = 11,                                        // 海康PC
        
    };

    // 媒体数据类型定义
    typedef enum
    {
        SYSHEAD_DATA = 0,                                           // 系统头数据
        REAL_VIDEO_DATA,                                            // 实时视频数据
        REAL_AUDIO_DATA,                                            // 实时音频数据
        REAL_VIDEO_IFRAME_DATA = 10,                                // 实时视频I帧数据
        TEXT_DATA,                                                  // 文本类型数据
        IMAGE_DATA,                                                 // 图像类型数据
        FAKE_DATA,                                                  // 伪数据包    
        UNKNOWN_DATA                                                // 未知类型数据
    } DATA_TYPE;

    // 流媒体功能请求操作类型定义
    typedef enum
    {
        REAL_START = 0,                                             // 开始实时视频接入
        REAL_STOP,                                                  // 停止实时视频接入(流媒体内部使用)
        REAL_FASTUPDATE,                                            // 请求I帧
        REAL_SNAPSHOT,                                              // 获取快照

        VOICE_START = 10,                                           // 开始语音对讲
        VOICE_STOP,                                                 // 停止语音对讲
        VOICE_CLIENT_DATA,                                          // 发送客户端语音数据

        RECORD_DOWNLOAD_START = 20,                                 // 开始录像下载
        RECORD_DOWNLOAD_STOP,                                       // 停止录像下载
        RECORD_REVIEW_START,                                        // 开始录像回放
        RECORD_REVIEW_STOP,                                         // 停止录像回放
        RECORD_REVIEW_PLAY_PAUSE,                                   // 暂停录像回放 
        RECORD_REVIEW_PLAY_RESUME,                                  // 恢复录像回放

        DONOTHING                                                   // 缺省流媒体功能请求                                       
    } REQUEST_OPERATION;

    // 客户端信息结构定义
    typedef struct _CLIENT_INFO 
    {
        STRING uuid;                                                // 客户端唯一标识
        STRING ip;                                                  // 客户端请求使用IP地址
        STRING mac;                                                 // 客户端请求使用MAC地址
        INT port;                                                   // 端口号
        STRINGLIST ip_list;                                         // 客户端IP地址列表   
        STRING proxy;                                               // 服务器代理

        // 缺省构造函数
        _CLIENT_INFO() : uuid(""), ip(""), mac(""), port(0), proxy("")
        {
            ip_list.clear();
        }
    } CLIENT_INFO, *CLIENT_INFO_PTR;

    // 客户端信息列表类型定义
    typedef std::list<CLIENT_INFO> CLIENT_INFO_LIST;

    // 设备描述信息结构定义
    typedef struct _DEVICE_INFO  
    {
        STRING uuid;                                                // 设备唯一标识
        INT type;                                                   // 设备类型 
        STRING version;                                             // 版本号
        STRING ip;                                                  // 设备IP地址
        UINT port;                                                  // 设备端口号
        STRING mac;                                                 // 设备MAC地址
        STRING monitor_proxy;                                       // 设备关联前端服务器代理
        STRING alarm_proxy;                                         // 设备关联报警服务器代理
        UINT channel_start;                                         // 设备通道起始值(兼容流媒体1.0数据库从1开始编号)
        UINT channel_count;                                         // 设备通道数    
        STRING user;                                                // 用户名
        STRING password;                                            // 密码 
        BOOL reconnect_flag;                                        // 设备断线重连标识

        // 缺省构造函数
        _DEVICE_INFO() : uuid(""), 
            type(0), version(""), 
            ip(""), port(0), mac(""),
            monitor_proxy(""), alarm_proxy(""),
            channel_start(1), channel_count(0),
            user(""), password(""), reconnect_flag(TRUE)
        {
        }
    } DEVICE_INFO, *DEVICE_INFO_PTR;

    // 设备描述信息列表类型定义
    typedef std::list<DEVICE_INFO> DEVICE_INFO_LIST;

    // 录像请求参数信息结构定义
    typedef struct _RECORD_INFO 
    {
        STRING uuid;                                                // 录像请求标识
        STRING user_uuid;                                           // 录像文件使用者ID
        STRINGLIST file_list;                                       // 录像文件列表
        STRING file_ex;                                             // 扩展录像文件名(录像备份时使用)
        INT file_type;                                              // 录像文件类型(根据不同设备SDK，录像文件类型的定义不同)
        BOOL by_time;                                               // 按时间回放/下载标志
        STRING begin_time;                                          // DVR录像回放/下载起始时间(DVR录像回放/下载使用)
        STRING end_time;                                            // DVR录像回放/下载结束时间(DVR录像回放/下载使用)
        BOOL loop;                                                  // 录像循环播放标志
        BOOL download;                                              // 录像下载标志
        INT offset;                                                 // 录像回放起始位置(百分比)

        // 缺省构造函数
        _RECORD_INFO() : uuid(""), user_uuid(""), file_ex(""), file_type(0),
            by_time(FALSE), begin_time(""), end_time(""),
            loop(FALSE), download(FALSE), offset(0)
        {
            file_list.clear();
        }
    } RECORD_INFO, *RECORD_INFO_PTR;

    // 媒体源信息结构定义
    typedef struct _STREAMSOURCE_INFO 
    {
        INT uuid;                                                   // 媒体源标识
        INT channel;                                                // 请求通道(语音对讲、录像回放/下载使用虚拟通道)
        DEVICE_INFO device_info;                                    // 设备信息
        //RECORD_INFO record_info;                                    // 录像请求参数信息
        
        // 缺省构造函数
        _STREAMSOURCE_INFO() : uuid(INVALID_ID), channel(0)
        {
        }
    } STREAMSOURCE_INFO, *STREAMSOURCE_INFO_PTR;

    // 媒体源信息列表类型定义
    typedef std::vector<STREAMSOURCE_INFO> STREAMSOURCE_INFO_LIST;

    // 流媒体功能请求结构定义
    typedef struct _REQUEST_INFO 
    {
		REQUEST_OPERATION operation;                                // 流媒体功能请求操作类型
		STREAMSOURCE_INFO source_info;                              // 媒体源信息

        // 缺省构造函数
        _REQUEST_INFO() : operation(DONOTHING)//, is_relay(FALSE), is_storage(FALSE)
        {
            //proxy_list.clear();
            //context.clear();
        }
    } REQUEST_INFO, *REQUEST_INFO_PTR;

    // 流媒体功能请求列表类型定义
    typedef std::list<REQUEST_INFO> REQUEST_INFO_LIST;

	//////////////////////////////////////////////////////////////////////////
	typedef struct tagLiveCastRequest
	{
		//string band;
		//string type;
		string url;

		tagLiveCastRequest() : /*band(""), type(""),*/ url("")
		{
		}

		~tagLiveCastRequest()
		{
		}

		bool operator < (tagLiveCastRequest const& _request) const
		{
			if(url < _request.url)
				return true;

			return false;
		}
	}
	LiveCastRequest;

	typedef struct tagLiveCastResponse
	{
		string num;
		string url;//hls
		string url2;//rtmp
		string expire;

		tagLiveCastResponse() : num(""), url(""), url2(""), expire("")
		{
		}

		~tagLiveCastResponse()
		{
		}

		bool operator < (tagLiveCastResponse const& _response) const
		{
			if(url < _response.url)
				return true;

			return false;
		}
	}
	LiveCastResponse;

	typedef struct tagVODRequest
	{
		string url;

		tagVODRequest() : url("")
		{
		}

		~tagVODRequest()
		{
		}

		bool operator < (tagVODRequest const& _request) const
		{
			if(url < _request.url)
				return true;

			return false;
		}
	}
	VODRequest;

	typedef struct tagVODResponse
	{
		string num;
		string url;//hls
		string url2;//rtmp
		string duration;

		tagVODResponse() : num(""), url(""), url2(""), duration("")
		{
		}

		~tagVODResponse()
		{
		}

		bool operator < (tagVODResponse const& _response) const
		{
			if(url < _response.url)
				return true;

			return false;
		}
	}
	VODResponse;
}

#endif // _TYPEDEF_H_
