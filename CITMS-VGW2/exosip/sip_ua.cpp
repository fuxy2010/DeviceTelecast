#if defined(__arc__)
#define LOG_PERROR 1
#include <includes_api.h>
#include <os_cfg_pub.h>
#endif

#if !defined(WIN32) && !defined(_WIN32_WCE)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>
#ifdef OSIP_MT
#include <pthread.h>
#endif
#endif

#include <osip_mt.h>
#include <eXosip.h>
#include <eXosip2.h>

#include "sip_ua.h"

using namespace SIP_UA;

EventCallBackFunc SIP_UA::_event_callback_func = NULL;

char* SIP_UA::_proxy = NULL;
char* SIP_UA::_contact = NULL;
char* SIP_UA::_fromuser = NULL;

char* SIP_UA::_local_ip = NULL;
unsigned short SIP_UA::_local_port = 0;
char* SIP_UA::_server_ip = NULL;
unsigned short SIP_UA::_server_port = 0;

char* SIP_UA::_username = NULL;
char*SIP_UA:: _password = NULL;

int SIP_UA::_regid = 0;
int SIP_UA::_expiry = 0;
int SIP_UA::_auth = 0;

bool SIP_UA::_stop_event_thread = false;
struct osip_thread* SIP_UA::_event_thread = NULL;
char* SIP_UA::_peer_uri = NULL;
bool SIP_UA::_auto_answer = false;
int SIP_UA::_call_id = 0;
int SIP_UA::_dialog_id = 0;
int SIP_UA::_transaction_id = 0;

static void syslog_wrapper(int a, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

#define LOG_INFO 0
#define LOG_ERR 0
#define LOG_WARNING 0
#define LOG_DEBUG 0

static void* sip_ua_thread_func(void *arg)
{
	osip_message_t *reg = NULL;
	osip_message_t *ack = NULL;
	osip_message_t *invite = NULL;
	osip_message_t *answer = NULL;
	sdp_message_t *remote_sdp = NULL;
	int i;

	while(false == _stop_event_thread)
	{
		eXosip_event_t* ev;

		//事件轮询，最多0.1s返回
		ev = eXosip_event_wait(0, 100);

		//自动处理一些401,407 超时释放什么的
		eXosip_lock();
		eXosip_default_action(ev);
		eXosip_automatic_refresh();//其中包含了自动注册的内容
		eXosip_automatic_action();
		eXosip_unlock();

		if(NULL == ev)
			continue;
		
		syslog_wrapper(LOG_INFO, "[%s] event %s\n", _username, ev->textinfo);

		switch (ev->type)
		{
		case EXOSIP_REGISTRATION_NEW:
			syslog_wrapper(LOG_INFO, "received new registration\n");
			break;

		case EXOSIP_REGISTRATION_SUCCESS:
			//注册成功
			syslog_wrapper(LOG_INFO, "registrered successfully\n");
			break;

		case EXOSIP_REGISTRATION_FAILURE:
			//注册失败
			syslog_wrapper(LOG_INFO,"Registration failed\n");

			if(NULL != _event_callback_func)
			{
				_event_callback_func(osip_message_get_from(ev->request)->url->username,
									osip_message_get_to(ev->request)->url->username,
									SIP_REG_FAIL, "");
			}
			break;

		case EXOSIP_REGISTRATION_TERMINATED:
			syslog_wrapper(LOG_INFO,"Registration terminated\n");
			break;

		case EXOSIP_CALL_INVITE:
			{
				//收到呼叫请求
				syslog_wrapper(LOG_INFO, "EXOSIP_CALL_INVIT Received a INVITE msg from %s:%s, UserName is %s, password is %s\n",
					ev->request->req_uri->host,
					ev->request->req_uri->port,
					ev->request->req_uri->username,
					ev->request->req_uri->password);

				//得到消息体,认为该消息就是SDP格式.
				remote_sdp = eXosip_get_remote_sdp (ev->did);

				_call_id = ev->cid;
				_dialog_id = ev->did;
				_transaction_id = ev->tid;

				//回复180 ringning
				eXosip_lock();
				eXosip_call_send_answer(ev->tid, 180, NULL);
				eXosip_unlock();

				osip_body_t* body;
				osip_message_get_body (ev->request, 0, &body);

				if(NULL != _event_callback_func)
				{
					_event_callback_func(osip_message_get_from(ev->request)->url->username,
										osip_message_get_to(ev->request)->url->username,
										SIP_INTIVE, body->body);
				}

				if(true == _auto_answer)
				{
					//自动应答
					SIP_UA::answer();
				}
			}
			break;

		case EXOSIP_CALL_ACK:
			//收到呼叫会话建立确认
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_ACK\n");
			break;

		case EXOSIP_CALL_CANCELLED:
			//呼叫被取消
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_CANCELLED\n");
			break;

		case EXOSIP_CALL_TIMEOUT:
			//呼叫超时
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_TIMEOUT\n");
			break;

		case EXOSIP_CALL_PROCEEDING:
			//收到1xx
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_PROCEEDING\n");
			_call_id = ev->cid;
			_dialog_id = ev->did;
			_transaction_id = ev->tid;
			break;

		case EXOSIP_CALL_RINGING:
			//收到180
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_RINGING\n");
			_call_id = ev->cid;
			_dialog_id = ev->did;
			_transaction_id = ev->tid;
			break;

		case EXOSIP_CALL_ANSWERED:
			{
				//对方接听
				syslog_wrapper(LOG_INFO, "EXOSIP_CALL_ANSWERED\n");
				_call_id = ev->cid;
				_dialog_id = ev->did;
				_transaction_id = ev->tid;

				eXosip_lock();

				//构造消息
				i = eXosip_call_build_ack(_dialog_id, &ack);
				if (i != 0)
				{
					syslog_wrapper(LOG_ERR, "build ack  failed!\n");
					eXosip_unlock();
					continue;
				}

				//解析和获取sdp
				osip_body_t* body;
				osip_message_get_body(ev->response, 0, &body);
				if(_event_callback_func)
				{
					_event_callback_func(osip_message_get_from(ev->request)->url->username,
										osip_message_get_to(ev->request)->url->username,
										SIP_ANSWERED, body->body);
				}

				//发送消息				
				i = eXosip_call_send_ack(_dialog_id, ack);
				eXosip_unlock();
			}
			break;

		case EXOSIP_CALL_MESSAGE_NEW:
			//会话内收到消息
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_MESSAGE_NEW:%s\n", ev->request->sip_method);
			if(!strcmp(ev->request->sip_method, "BYE"))
			{
				if(_event_callback_func)
				{
					_event_callback_func(osip_message_get_from(ev->request)->url->username,
										osip_message_get_to(ev->request)->url->username,
										SIP_HANGUP, "");
				}
			}
			break;

		case EXOSIP_CALL_CLOSED:
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_CLOSED\n");
			break;

		case EXOSIP_CALL_RELEASED:
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_RELEASED\n");
			break;

		case EXOSIP_CALL_REQUESTFAILURE:
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_REQUESTFAILURE\n");
			break;

		case EXOSIP_MESSAGE_NEW:
			//收到message消息
			if(MSG_IS_MESSAGE(ev->request))
			{
				osip_body_t *body;
				osip_message_t *message = NULL;
				char * from;
				char * to;
				osip_uri_to_str(osip_message_get_from(ev->request)->url, &from);
				osip_uri_to_str(osip_message_get_to(ev->request)->url, &to);
				osip_message_get_body(ev->request, 0, &body);
				syslog_wrapper(LOG_INFO, "EXOSIP_MESSAGE_NEW:%s %s\n", ev->request->sip_method, body->body);

				if(_event_callback_func)
				{
					_event_callback_func(osip_message_get_from(ev->request)->url->username,
										osip_message_get_to(ev->request)->url->username,
										SIP_MSG, body->body);
				}

				//按照规则，需要回复200 OK信息
				eXosip_message_build_answer (ev->tid, 200, &answer);
				eXosip_message_send_answer (ev->tid, 200, answer);

				osip_free(from);
				osip_free(to);
			}
			else if (MSG_IS_NOTIFY(ev->request))
			{
				//按照规则，需要回复200 OK信息
				osip_body_t *body;
				osip_message_get_body (ev->request, 0, &body);
				if(body!=NULL)
					arg=body->body;
				//收到notify
				if(_event_callback_func)
				{
					_event_callback_func(osip_message_get_from(ev->request)->url->username,
										osip_message_get_to(ev->request)->url->username,
										SIP_NOTIFY, "");
				}
				eXosip_message_build_answer(ev->tid, 200,&answer);
				eXosip_message_send_answer(ev->tid, 200,answer);
			}
			else if (MSG_IS_REGISTER(ev->request))
			{


				osip_authorization_t *auth;
				osip_from_t *from = NULL;
				char* fromstr;
				char contact[256];
				osip_message_get_authorization(ev->request, 0, &auth);
				osip_uri_to_str(osip_message_get_to(ev->request)->url, &fromstr);
				_snprintf(contact,sizeof(contact),"%s;expires=100",fromstr);

				if(auth == NULL)
				{
					osip_message_t *answer401;
					eXosip_message_build_answer(ev->tid, 401, &answer401);

					char rand_str[64];
					memset(rand_str, 0, 64);
					eXosip_generate_random(rand_str, 64);
					char www_auth_body[1024];
					memset(www_auth_body, 0, 1024);
					snprintf(www_auth_body, sizeof(www_auth_body),"Digest realm=\"%s\",algorithm=md5,nonce=\"%s\",stale=false", "user",rand_str);
					osip_message_set_header(answer401, "WWW-Authenticate", www_auth_body);
					eXosip_message_send_answer(ev->tid, 401, answer401);
				}
				else
				{
					osip_message_t *ok200;
					eXosip_message_build_answer(ev->tid, 200, &ok200);
					osip_message_set_contact(ok200,contact);
					eXosip_message_send_answer(ev->tid, 200, ok200);
				}
				osip_free(fromstr);
			}
			break;

		case EXOSIP_CALL_MESSAGE_ANSWERED:
			syslog_wrapper(LOG_INFO, "EXOSIP_CALL_MESSAGE_ANSWERED\n");
			break;

		case EXOSIP_MESSAGE_ANSWERED:
			syslog_wrapper(LOG_INFO, "EXOSIP_MESSAGE_ANSWERED\n");
			break;

		/*fym case EXOSIP_IN_SUBSCRIPTION_NEW:
			_sub_id = ev->sid;
			_sub_dialog_id = ev->did;
			eXosip_insubscription_automatic(ev);
			break;

		case EXOSIP_SUBSCRIPTION_PROCEEDING:
			//收到sub 的1xx回应
			break;

		case EXOSIP_SUBSCRIPTION_ANSWERED:
			//收到sub 的2xx回应
			_sub_id = ev->sid;
			_sub_dialog_id = ev->did;
			break;

		case EXOSIP_SUBSCRIPTION_NOTIFY:
			{
				osip_body_t *body;
				osip_message_get_body(ev->request, 0, &body);

				if(body!=NULL)
					arg=body->body;
				//收到notify
				if(_event_callback_func)
				{
					_event_callback_func(osip_message_get_from(ev->request)->url->username,
										osip_message_get_to(ev->request)->url->username,
										SIP_NOTIFY, "");
				}
			}
			break;*/

		case EXOSIP_NOTIFICATION_ANSWERED:
			//收到notify 的2xx回应
			break;

		default:
			syslog_wrapper(LOG_DEBUG,
				"recieved unknown eXosip event (type, did, cid) = (%d, %d, %d)\n",
				ev->type, ev->did, ev->cid);
		}

		eXosip_event_free(ev);
	}

	return NULL;
}

bool SIP_UA::start(const char* username, const char* password,
		const char* local_ip, const unsigned short local_port,
		const char* server_ip, const unsigned short server_port,
		bool by_tcp)
{
	int usersname_length = strlen(username);

	if(NULL == username || !usersname_length)
		return false;

	if(NULL == local_ip || !strlen(local_ip) || !local_port)
		return false;

	if(NULL == server_ip || !strlen(server_ip) || !server_port)
		return false;

	int ret = eXosip_init();
	if(ret)
	{
		syslog_wrapper(LOG_ERR, "eXosip_init failed %d\n", ret);
		return false;
	}

	//proxy = osip_strdup("sip:220.249.112.22");
	//fromuser = osip_strdup("sip:111111@20.249.112.22");
	//contact = osip_strdup("sip:111111@192.168.1.6:10000;transport=tcp");
	
	_proxy = (char*)osip_malloc(32);//osip_malloc已memset
	sprintf(_proxy, "sip:%s", server_ip);

	_fromuser = (char*)osip_malloc(strlen(username) + 32);
	sprintf(_fromuser, "sip:%s@%s", username, server_ip);

	_contact = (char*)osip_malloc(strlen(username) + 64);
	sprintf(_contact, "sip:%s@%s:%d;transport=%s", username, local_ip, local_port, ((true == by_tcp) ? "tcp" : "udp"));

	_username = (char*)osip_malloc(usersname_length + 1);
	osip_strncpy(_username, username, usersname_length);

	_local_ip = (char*)osip_malloc(strlen(local_ip) + 1);
	osip_strncpy(_local_ip, local_ip, strlen(local_ip));

	_local_port = local_port;

	_server_ip = (char*)osip_malloc(strlen(server_ip) + 1);
	osip_strncpy(_server_ip, server_ip, strlen(server_ip));

	_server_port = server_port;

	if(eXosip_listen_addr(((true == by_tcp) ? IPPROTO_TCP : IPPROTO_UDP), NULL, _local_port, AF_INET, 0))
	{
		syslog_wrapper(LOG_ERR, "eXosip_listen_addr failed");
		return false;
	}

	eXosip_masquerade_contact(local_ip, local_port);

	eXosip_set_user_agent(username);

	if(eXosip_add_authentication_info(username, username, password, NULL, NULL))
	{
		syslog_wrapper(LOG_ERR, "eXosip_add_authentication_info failed");
		return false;
	}

    _stop_event_thread = false;

    _event_thread = osip_thread_create(20000, sip_ua_thread_func, NULL);
    if(NULL == _event_thread)
    {
        syslog_wrapper(LOG_ERR," Cannot start thread!\n");
        return false;
    }

    syslog_wrapper(LOG_INFO,"[%s] evnet thread started!\n", username);

	return true;
}

void SIP_UA::shutdown()
{
    int i;

    //关闭事件线程
    _stop_event_thread = true;

    if(NULL != _event_thread)
    {
        i = osip_thread_join(_event_thread);

        if (i != 0)
        {
            syslog_wrapper(LOG_ERR," can't terminate thread!\n");
        }

        osip_free(_event_thread);

        syslog_wrapper(LOG_INFO,"[%s] evnet thread stopped!\n", _username);
    }

    eXosip_quit();

	osip_free(_contact);
	osip_free(_fromuser);
	osip_free(_local_ip);
	osip_free(_proxy);
	osip_free(_username);
	osip_free(_password);
	osip_free(_peer_uri);
}

void SIP_UA::set_callback_func(EventCallBackFunc func)
{
	_event_callback_func = func;
}

void SIP_UA::sip_register(int expires)
{
    osip_message_t *reg = NULL;

    int i;

	//syslog_wrapper(LOG_INFO, _fromuser);
	//syslog_wrapper(LOG_INFO, _proxy);
	//syslog_wrapper(LOG_INFO, _contact);

	eXosip_lock();
    
	//构造消息
    _regid = eXosip_register_build_initial_register(_fromuser, _proxy, _contact, expires, &reg);	
    
	if(_regid < 1)
    {
        syslog_wrapper(LOG_ERR,"eXosip_register_build_initial_register failed");
		eXosip_unlock();
        return;
    }

    //发送消息    
    i = eXosip_register_send_register(_regid, reg);
    eXosip_unlock();

    if (i != 0)
    {
        syslog_wrapper(LOG_ERR, "eXosip_register_send_register failed");
        return;
    }
}

void SIP_UA::call(const char* ua_id)
{
    int i;
	char sdp[] = "v=0\r\n"
		"s=sipreg\r\n"
		"o=- 12973748354127722 1 IN IP4 127.0.0.1\r\n"
		"c=IN IP4 127.0.0.1\r\n"
		"t=0 0\r\n"
		"m=audio 40000 RTP/AVP 107 0 8 101\r\n"
		"a=rtpmap:107 BV32/16000\r\n"
		"a=rtpmap:101 telephone-event/8000\r\n"
		"a=fmtp:101 0-15\r\n"
		"a=sendrecv\r\n";

    osip_message_t *invite = NULL;
	char* to_uri = NULL;

	if(NULL == ua_id || !strlen(ua_id))
		return;

	eXosip_lock();

	to_uri = (char*)osip_malloc(strlen(ua_id) + 32);
	sprintf(to_uri, "sip:%s@%s", ua_id, _server_ip);

    syslog_wrapper(LOG_INFO, "call %s\n", to_uri);

    //构造消息
    i = eXosip_call_build_initial_invite(&invite, to_uri, _fromuser, NULL, "This si a call for a conversation");
    if (i != 0)
    {
        syslog_wrapper(LOG_ERR, "Intial INVITE failed!\n");
		eXosip_unlock();
        return ;
    }

    osip_message_set_body (invite, sdp, strlen(sdp));
    osip_message_set_content_type (invite, "application/sdp");

    //发送消息    
    i = eXosip_call_send_initial_invite(invite);

    eXosip_unlock();
}

void SIP_UA::answer()
{
	int i;
	char sdp[] = "v=0\r\n"
		"s=sipreg\r\n"
		"o=- 12973748354127722 1 IN IP4 127.0.0.1\r\n"
		"c=IN IP4 127.0.0.1\r\n"
		"t=0 0\r\n"
		"m=audio 40000 RTP/AVP 107 0 8 101\r\n"
		"a=rtpmap:107 BV32/16000\r\n"
		"a=rtpmap:101 telephone-event/8000\r\n"
		"a=fmtp:101 0-15\r\n"
		"a=sendrecv\r\n";
    osip_message_t *answer = NULL;

	eXosip_lock();

    //构造消息
    i = eXosip_call_build_answer(_transaction_id, 200, &answer);
    if (i != 0)
    {
        syslog_wrapper(LOG_INFO, "This request msg is invalid!Cann't response!\n");
        eXosip_call_send_answer(_transaction_id, 400, NULL);
    }
    else
    {
		osip_message_set_body(answer, sdp, strlen(sdp));
        osip_message_set_content_type(answer, "application/sdp");
		
		//发送消息
		eXosip_call_send_answer(_transaction_id, 200, answer);
        
	}

	eXosip_unlock();
}


void SIP_UA::hangup()
{
    eXosip_call_terminate(_call_id, _dialog_id);
}

void SIP_UA::auto_answer(bool enable)
{
	_auto_answer = enable;
}

void SIP_UA::message(const char* ua_id, const char* content)
{
    int i;
    osip_message_t *message = NULL;
    char* to_uri = NULL;

	if(NULL == ua_id || !strlen(ua_id) || NULL == content || !strlen(content))
		return;

	eXosip_lock();

	to_uri = (char*)osip_malloc(strlen(ua_id) + 32);
	sprintf(to_uri, "sip:%s@%s", ua_id, _server_ip);

    syslog_wrapper(LOG_INFO, "message %s %s\n", to_uri, content);

    i = eXosip_message_build_request(&message, "MESSAGE", to_uri, _fromuser, NULL);
    if (i != 0)
    {
		eXosip_unlock();
		return;
    }

    osip_message_set_expires(message, "120");
    osip_message_set_body(message, content, strlen(content));
    osip_message_set_content_type (message, "text/plain");

    
    i = eXosip_message_send_request(message);

    eXosip_unlock ();
}

char nat_msg[128];
LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	printf("\nreceive message %d!", message);

	switch (message)
	{
	case WM_PAINT:
		break;

	case WM_CREATE:
		break;

	case WM_HOTKEY:
		break;

	case WM_TIMER:
		break;

	case WM_CLOSE:
		exit(1);
		break;
	
	case WM_DESTROY: //窗口销毁消息
		PostQuitMessage(0);
		break;

	case SEND_NAT_PROBE_MSG:
		{
			memset(nat_msg, 0, sizeof(nat_msg));
			sprintf(nat_msg,
				"{\"t\":\"REG\",\"c\":\"0\",\"n\":\"2\",\"s\":\"2\",\"b\":\"UA\",\"m\":\"VUA\",\"p\":\"111\",\"i\":\"%s\"}",
				_username);

			SIP_UA::message("VUA", nat_msg);
		}

	default://必须有此项，否则无法创建窗口
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int create_window(char* window_name)
{
	HWND hwnd = NULL;
	ATOM wnd_atom;
	WNDCLASSEX wnd_class;
	MSG msg;
	DWORD error = 0;

	if(NULL == window_name)
		return -1;

	wnd_class.cbSize = sizeof(WNDCLASSEX);
	wnd_class.style = 0;
	wnd_class.lpfnWndProc = (WNDPROC)(wnd_proc);//&wnd_proc;
	wnd_class.cbClsExtra = 0;
	wnd_class.cbWndExtra = 0;
	wnd_class.hInstance = NULL;
	wnd_class.hIcon = NULL;
	wnd_class.hCursor = NULL;
	wnd_class.hbrBackground = NULL;
	wnd_class.lpszMenuName = NULL;//菜单名
	wnd_class.lpszClassName = window_name;//窗口类名
	wnd_class.hIconSm = NULL;

	wnd_atom = RegisterClassEx(&wnd_class);	

	if(NULL == wnd_atom)
	{
		error = GetLastError();
		exit(-1);		
	}

	hwnd = CreateWindow(window_name,//窗口类名
		window_name,//窗口标题
		WS_POPUP,//Window style ( a popup doesn't need a parent )
		0,//x pos
		0,//y pos
		CW_USEDEFAULT,//default width
		CW_USEDEFAULT,//default height
		NULL,//No parent
		NULL,//No menu handle
		NULL,// Ignored on WinNT
		NULL);// data for message proc. Who cares?

	if(NULL == hwnd)
	{
		error = GetLastError();
		exit(-1);
	}

	while (GetMessage(&msg, hwnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

char callback_data[1024];
void event_callback(const char* from, const char* to, const SIP_EVENT_TYPE& type, const char* attachment)
{
	printf("\ncallback: %s => %s, %d, %s", from, to, type, attachment);

	HWND hwnd = FindWindow(NULL, "大杀器2.0");//虚拟UA对话框标题栏名称
	
	if(NULL != hwnd) 
	{
		memset(callback_data, 0, sizeof(callback_data));
		int callback_data_length = strlen(from) + strlen(to) + strlen(attachment) + 1;
		sprintf(callback_data, "%s#%s#%s", from, to, attachment);

		COPYDATASTRUCT cds;
		cds.dwData = type;//自定义
		cds.cbData = callback_data_length;//数据长度
		cds.lpData = callback_data;//数据指针

		SendMessage(hwnd, WM_COPYDATA, (WPARAM)(type), (LPARAM)&cds);
	}
}

int main_test(int argc,char* argv[])
{
	int i;
	char* username = NULL;
	char* password = NULL;
	char* local_ip = NULL;
	unsigned short local_port = 0;
	char* server_ip = NULL;
	unsigned short server_port = 0;	
	bool by_tcp = true;
	char* window_name = NULL;

#if 1
	if(8 != argc)
		exit(1);

	//sip_ua.exe username password 10.10.10.15 5060 10.10.10.169 10000 tcp
	for(i = 0; i < argc; ++i)
	{
		printf("arg: %s\n", argv[i]);
	}

	username = argv[1];
	password = argv[2];

	local_ip = argv[3];
	local_port = atoi(argv[4]);

	server_ip = argv[5];
	server_port = atoi(argv[6]);
	
	by_tcp = (!strcmp("tcp", argv[7])) ? true : false;
#endif

	SIP_UA::auto_answer(true);
	SIP_UA::set_callback_func(event_callback);
	
	//SIP_UA::start("3489660928", "3489660928", "192.168.1.5", 6000, "220.249.112.22", 5060, true);
	//SIP_UA::start("111111", "111111", "10.10.10.169", 20000, "10.10.10.15", 5060, true);	
	SIP_UA::start(username, password, local_ip, local_port, server_ip, server_port, by_tcp);

	SIP_UA::sip_register(90);
	//SIP_UA::call("607887303");//对sipdroid有效，对xLite无效
	//SIP_UA::message("222222", "hahaha");

	window_name = (char*)osip_malloc(32);//osip_malloc已memset
	sprintf(window_name, "sipua%s", _username);

	create_window(window_name);

	return 0;
}
