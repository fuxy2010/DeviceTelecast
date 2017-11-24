// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2009-01 ~ 2009-03
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _GENERAL_DEF_H_
#define _GENERAL_DEF_H_

//当编译器为VC且版本不低于VC6.0时，本文件在编译时只被包含一次
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "stdafx.h"

#ifdef _DEBUG
//#include "..\\utility\\VLD1.9H\\vld.h"
#endif

#include <string>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>

//#ifndef	PERFORMANCE_ANALYSIS
//#define	PERFORMANCE_ANALYSIS
//#endif

#ifndef _SENDING_DATA_BLOCK_SIZE_
#define _SENDING_DATA_BLOCK_SIZE_	1400
#endif

//#define PUSH_VIDEO_FILE
//#define VIDEO_FILE "hk.264"
//#define FILE_VIDEO_FRAME_INTERVAL 80


namespace ScheduleServer
{
	//参数上下文类型定义
	typedef std::map<std::string, std::string> SS_CTX;

	//设备类型定义
	typedef enum
	{
		SIPStack = 0,	//SIP Server
		Unknown			//未知
	}Device_Type;

	typedef struct tagTIME_INFO
	{
		unsigned long year;		//[1900,--]
		unsigned long month;	//[1, 12]
		unsigned long day;		//[1, 31]
		unsigned long hour;		//[0, 23]
		unsigned long minute;	//[0, 59]
		unsigned long second;	//[0, 59]

		tagTIME_INFO() : year(0), month(0), day(0), hour(0), minute(0), second(0)
		{
		}
	}
	TIME_INFO;

    //返回值类型定义
    typedef enum
    {
        SS_NoErr = 0,						//函数调用成功
		SS_RecvData,

		//服务器启动
		SS_ServerStartFail,					//服务器启动失败

		//UA操作
		SS_InsertUAFail,					//插入UA失败
		SS_InsertMediaDataFail,				//插入媒体数据到队列失败

		//SIPStack接入
		SS_ConnectSIPStackFail,				//连接SIPStack失败
		SS_DisconnectSIPStackFail,				//断开SIPStack失败

		//系统会议线程
		SS_StartConferenceThreadFail,		//启动会议线程失败

		//任务线程
		SS_StartTaskThreadFail,			//启动任务线程失败
		SS_AddTaskFail,					//添加任务失败

		//任务队列
		SS_InvalidTask,					//错误任务

		//查询任务执行结果
		SS_QueryTaskResultFail,			//查询任务执行结果失败

		//更新任务
		SS_SyncTaskFail,					//更新任务失败

		//RTP传输
		SS_RTPSendSessionUnAvailable,		//RTP发送Session不可用
		SS_RTPRecvSessionUnAvailable,		//RTP接收Session不可用
		SS_AddRTPHeaderFail,				//添加RTP包头失败
		SS_SendPacketFail,					//发送RTP包失败

        SS_Unknown_Error					//未知错误    
    }SS_Error;

    /*typedef struct 
    {
        SS_Error ret_code;
		std::string message;
    }SS_Error_Info;

	const std::string DEFAULT_SS_ERROR_MESSAGE = "未知错误";

    const SS_Error_Info global_ds_error_info_list[] = {
        {SS_NoErr,						"调用成功"},
        {SS_InvalidDBConnection,			"用户名或密码错误"},
        {SS_ExecFail,					"执行SQL语句失败"},
		{SS_QueryFail,					"查询记录失败"},

        {SS_Unknown_Error,				"未知错误"}
    };*/
}

#endif // _GENERAL_DEF_H_
