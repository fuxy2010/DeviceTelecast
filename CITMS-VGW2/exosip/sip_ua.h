#ifndef _SIP_UA_H_
#define _SIP_UA_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#define SEND_NAT_PROBE_MSG WM_USER + 100

#include <osip_mt.h>

namespace SIP_UA
{
	//事件类型，在UserAgent.h和sip_ua.h中应保持一致
	typedef enum
	{
		SIP_INTIVE = 0,//收到其他UA呼叫
		SIP_HANGUP,//主呼中被其他UA拒接或通话中（无论主被叫）对方UA主动挂断
		SIP_MSG,//收到其他UA的Message消息
		SIP_ANSWERED,//呼叫其他UA后对方接听
		SIP_NOTIFY,//呼叫其他UA NOTIFY消息
		SIP_REG_FAIL,//注册失败
		SIP_Unknown//未知
	}
	SIP_EVENT_TYPE;

	//错误码
	typedef enum
	{
		SIP_ERROR_NOERROR = 0//无错误

	}
	SIP_ERROR_CODE;	

	//SIP通信事件回调
	//ua_id为对方UA的ID
	//type为时间类型
	//当type为SIP_MESSAGE时attachment为消息内容,否则attachment为空字符串
	typedef void (*EventCallBackFunc)(const char* from, const char* to, const SIP_EVENT_TYPE& type, const char* attachment);

	extern EventCallBackFunc _event_callback_func;

	extern char* _proxy;
	extern char* _contact;
	extern char* _fromuser;

	extern char* _local_ip;
	extern unsigned short _local_port;
	extern char* _server_ip;
	extern unsigned short _server_port;
	
	extern char* _username;
	extern char* _password;

	extern int _regid;
	extern int _expiry;
	extern int _auth;

	extern bool _stop_event_thread;
	extern struct osip_thread* _event_thread;

	extern char* _peer_uri;//对方UA

	extern bool _auto_answer;

	extern int _call_id;
	extern int _dialog_id;
	extern int _transaction_id;

	//启动SIPUA(开始监听及消息处理)
	extern bool start(const char* username, const char* password,
					const char* local_ip, const unsigned short local_port,
					const char* server_ip, const unsigned short server_port,
					bool by_tcp);

	extern void shutdown();

	//设置SIP事件回调函数
	extern void set_callback_func(EventCallBackFunc func);

	//向SIP服务器注册
	extern void sip_register(int expires);

	//呼叫UA
	extern void call(const char* ua_id);

	//是否允许自动接听,enable为允许
	extern void auto_answer(bool enable);

	//呼叫应答
	//仅在非自动应答情况下调用
	extern void answer();

	//挂断通话,包括如下情形
	//为主叫,被叫应答后挂断
	//为主叫,被叫未应答即主动挂断
	//为被叫,应答后挂断
	//为被叫,拒绝接听
	extern void hangup();

	//向UA发送Message消息,content为消息内容
	extern void message(const char* ua_id, const char* content);

}
#endif//_SIP_UA_H_