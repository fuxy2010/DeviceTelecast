/*
 * SIP Registration Agent -- by ww@styx.org
 * 
 * This program is Free Software, released under the GNU General
 * Public License v2.0 http://www.gnu.org/licenses/gpl
 *
 * This program will register to a SIP proxy using the contact
 * supplied on the command line. This is useful if, for some 
 * reason your SIP client cannot register to the proxy itself.
 * For example, if your SIP client registers to Proxy A, but
 * you want to be able to recieve calls that arrive at Proxy B,
 * you can use this program to register the client's contact
 * information to Proxy B.
 *
 * This program requires the eXosip library. To compile,
 * assuming your eXosip installation is in /usr/local,
 * use something like:
 *
 * gcc -O2 -I/usr/local/include -L/usr/local/lib sipreg.c \
 *         -o sipreg \
 *         -leXosip2 -losip2 -losipparser2 -lpthread
 *
 * It should compile and run on any POSIX compliant system
 * that supports pthreads.
 *
 */


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

//fym #ifdef _WIN32_WCE
/* #include <syslog.h> */
//#include <windows.h>
//#include <winsock2.h>
//fym #endif

#include <osip_mt.h>
#include <eXosip.h>
#include <eXosip2.h>

#if !defined(WIN32) && !defined(_WIN32_WCE) && !defined(__arc__)
#define _GNU_SOURCE
#include <getopt.h>
#endif

#define PROG_NAME "sipreg"
#define PROG_VER  "1.0"
#define UA_STRING "SipReg v" PROG_VER
#define SYSLOG_FACILITY LOG_DAEMON

#if defined(WIN32) || defined(_WIN32_WCE)
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

#elif defined(LOG_PERROR)
/* If we can, we use syslog() to emit the debugging messages to stderr. */
#define syslog_wrapper    syslog
#else
#define syslog_wrapper(a,b...) fprintf(stderr,b);fprintf(stderr,"\n")
#endif


static void usage(void);
#ifdef OSIP_MT
static void *register_proc(void *arg);
#endif

static void usage(void)
{
	printf("Usage: " PROG_NAME " [required_options] [optional_options]\n"
		   "\n\t[required_options]\n"
		   "\t-r --proxy\tsip:proxyhost[:port]\n"
		   "\t-u --from\tsip:user@host[:port]\n"
		   "\n\t[optional_options]\n"
		   "\t-c --contact\tsip:user@host[:port]\n"
		   "\t-d --debug (log to stderr and do not fork)\n"
		   "\t-e --expiry\tnumber (default 3600)\n"
		   "\t-f --firewallip\tN.N.N.N\n"
		   "\t-h --help\n"
		   "\t-l --localip\tN.N.N.N (force local IP address)\n"
		   "\t-p --port\tnumber (default 5060)\n"
		   "\t-U --username\tauthentication username\n"
		   "\t-P --password\tauthentication password\n");
}

typedef struct regparam_t {
	int regid;
	int expiry;
	int auth;
} regparam_t;

#if 0
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int APIENTRY WinMain(HINSTANCE hInstance/*应用程序的实例句柄*/, HINSTANCE hPrevInstance, LPSTR lpCmdLine/*命令行*/, int nCmdShow/*显示方式*/)
{
	MSG msg;
	HWND hWnd;
	char szTitle[]="系统信息"; // The title bar text
	char szWindowClass[]="popMsg"; // The title bar text

	WNDCLASSEX wcex={0};
	wcex.cbSize = sizeof(WNDCLASSEX); //WNDCLASSEX结构体大小
	wcex.style = CS_HREDRAW | CS_VREDRAW; //位置改变时重绘
	wcex.lpfnWndProc = (WNDPROC)WndProc; //消息处理函数
	wcex.hInstance = 0; //当前实例句柄
	wcex.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME; //背景色
	wcex.lpszClassName = szWindowClass; //参窗口类名
	wcex.hIcon =0; //图标
	wcex.hCursor =LoadCursor(NULL, IDC_ARROW); //光标
	wcex.lpszMenuName =0; //菜单名称
	wcex.hIconSm =0; //最小化图标

	RegisterClassEx(&wcex); //注册窗口类

	hWnd = CreateWindow(szWindowClass, szTitle, WS_SYSMENU|WS_VISIBLE, CW_USEDEFAULT,CW_USEDEFAULT, 400, 300, NULL, NULL, 0, NULL);
	
	if(!hWnd)
	{
		return FALSE;
	}
	
	ShowWindow(hWnd, nCmdShow);	
	UpdateWindow(hWnd);
	
	while (GetMessage(&msg, NULL, 0, 0)) // 消息循环:
	{
		TranslateMessage(&msg); //转化虚拟按键到字符消息
		DispatchMessage(&msg); //分派消息调用回调函数
	}
	
	return msg.wParam;
}
#endif

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		break;

	case WM_CREATE:
		RegisterHotKey(hWnd ,1, 0 ,0x41 );
		break;
	
	case WM_HOTKEY:
		MessageBox(0,0,0,0);
		break;
	
	case WM_TIMER:
		break;

	case WM_DESTROY: //窗口销毁消息
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	
	return 0;
}

#if 1
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
#endif

#ifdef OSIP_MT
static void *register_proc(void *arg)
{
	struct regparam_t *regparam = arg;
	int reg;

	for (;;) {
#ifdef _WIN32_WCE
		Sleep((regparam->expiry / 2) * 1000);
#else
		Sleep(regparam->expiry / 2);
#endif
		eXosip_lock();
		reg = eXosip_register_send_register(regparam->regid, NULL);
		if (0 > reg) {
#ifdef _WIN32_WCE
			fprintf(stdout, "eXosip_register: error while registring");
#else
			perror("eXosip_register");
#endif
			exit(1);
		}
		regparam->auth = 0;
		eXosip_unlock();
	}
	return NULL;
}
#endif

/*
#ifdef _WIN32_WCE
int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
	int c;
	int port = 5060;
	char *contact = NULL;
	char *fromuser = NULL;
	const char *localip = NULL;
	const char *firewallip = NULL;
	char *proxy = NULL;
#if !defined(__arc__)
	struct servent *service;
#endif
	char *username = NULL;
	char *password = NULL;
	struct regparam_t regparam = { 0, 3600, 0 };
#ifdef OSIP_MT
	struct osip_thread *register_thread;
#endif
	int debug = 0;
	int nofork = 0;


#ifdef _WIN32_WCE
	proxy = osip_strdup("sip:sip.antisip.com");
	fromuser = osip_strdup("sip:jack@sip.antisip.com");

#else
	for (;;) {
#define short_options "c:de:f:hl:p:r:u:U:P:"
#ifdef _GNU_SOURCE
		int option_index = 0;
		static struct option long_options[] = {
			{"contact", required_argument, NULL, 'c'},
			{"debug", no_argument, NULL, 'd'},
			{"expiry", required_argument, NULL, 'e'},
			{"firewallip", required_argument, NULL, 'f'},
			{"from", required_argument, NULL, 'u'},
			{"help", no_argument, NULL, 'h'},
			{"localip", required_argument, NULL, 'l'},
			{"port", required_argument, NULL, 'p'},
			{"proxy", required_argument, NULL, 'r'},
			{"username", required_argument, NULL, 'U'},
			{"password", required_argument, NULL, 'P'},
			{NULL, 0, NULL, 0}
		};

		c = getopt_long(argc, argv, short_options, long_options, &option_index);
#else
		c = getopt(argc, argv, short_options);
#endif
		if (c == -1)
			break;

		switch (c) {
		case 'c':
			contact = optarg;
			break;
		case 'd':
			nofork = 1;
#ifdef LOG_PERROR
			debug = LOG_PERROR;
#endif
			break;
		case 'e':
			regparam.expiry = atoi(optarg);
			break;
		case 'f':
			firewallip = optarg;
			break;
		case 'h':
			usage();
			exit(0);
		case 'l':
			localip = optarg;
			break;
		case 'p':
#if !defined(__arc__)
			service = getservbyname(optarg, "udp");
			if (service)
				port = ntohs(service->s_port);
			else
				port = atoi(optarg);
#else
			port = atoi(optarg);
#endif
			break;
		case 'r':
			proxy = optarg;
			break;
		case 'u':
			fromuser = optarg;
			break;
		case 'U':
			username = optarg;
			break;
		case 'P':
			password = optarg;
			break;
		default:
			break;
		}
	}
#endif

	if (!proxy || !fromuser) {
		usage();
		exit(1);
	}
#ifndef _WIN32_WCE
	if (!nofork) {
		int cpid = fork();

		if (cpid)
			exit(0);
		close(0);
		close(1);
		close(2);
	}
#endif

#if 0
	openlog(PROG_NAME, LOG_PID | debug, SYSLOG_FACILITY);
#endif

	syslog_wrapper(LOG_INFO, UA_STRING " up and running");
	syslog_wrapper(LOG_INFO, "proxy: %s", proxy);
	syslog_wrapper(LOG_INFO, "fromuser: %s", fromuser);
	syslog_wrapper(LOG_INFO, "contact: %s", contact);
	syslog_wrapper(LOG_INFO, "expiry: %d", regparam.expiry);
	syslog_wrapper(LOG_INFO, "local port: %d", port);

	if (debug > 0)
		TRACE_INITIALIZE(6, NULL);

	if (eXosip_init(eXosip)) {
		syslog_wrapper(LOG_ERR, "eXosip_init failed");
		exit(1);
	}
	if (eXosip_listen_addr(eXosip, IPPROTO_UDP, NULL, port, AF_INET, 0)) {
		syslog_wrapper(LOG_ERR, "eXosip_listen_addr failed");
		exit(1);
	}

	if (localip) {
		syslog_wrapper(LOG_INFO, "local address: %s", localip);
		eXosip_masquerade_contact(localip, port);
	}

	if (firewallip) {
		syslog_wrapper(LOG_INFO, "firewall address: %s:%i", firewallip, port);
		eXosip_masquerade_contact(firewallip, port);
	}

	eXosip_set_user_agent(eXosip, UA_STRING);

	if (username && password) {
		syslog_wrapper(LOG_INFO, "username: %s", username);
		syslog_wrapper(LOG_INFO, "password: [removed]");
		if (eXosip_add_authentication_info
			(username, username, password, NULL, NULL)) {
			syslog_wrapper(LOG_ERR, "eXosip_add_authentication_info failed");
			exit(1);
		}
	}

	{
		osip_message_t *reg = NULL;
		int i;

		regparam.regid =
			eXosip_register_build_initial_register(eXosip, fromuser, proxy, contact,
												   regparam.expiry * 2, &reg);
		if (regparam.regid < 1) {
			syslog_wrapper(LOG_ERR,
						   "eXosip_register_build_initial_register failed");
			exit(1);
		}
		i = eXosip_register_send_register(eXosip, regparam.regid, reg);
		if (i != 0) {
			syslog_wrapper(LOG_ERR, "eXosip_register_send_register failed");
			exit(1);
		}
	}

#ifdef OSIP_MT
	register_thread = osip_thread_create(20000, register_proc, &regparam);
	if (register_thread == NULL) {
		syslog_wrapper(LOG_ERR, "pthread_create failed");
		exit(1);
	}
#endif

	for (;;) {
		eXosip_event_t *event;

		if (!(event = eXosip_event_wait(0, 1))) {
#ifndef OSIP_MT
			eXosip_execute(eXosip);
			eXosip_automatic_action(eXosip);
#endif
			osip_usleep(10000);
			continue;
		}
#ifndef OSIP_MT
		eXosip_execute(eXosip);
#endif

		eXosip_automatic_action(eXosip);
		switch (event->type) {
		case EXOSIP_REGISTRATION_NEW:
			syslog_wrapper(LOG_INFO, "received new registration");
			break;
		case EXOSIP_REGISTRATION_SUCCESS:
			syslog_wrapper(LOG_INFO, "registrered successfully");
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			regparam.auth = 1;
			break;
		case EXOSIP_REGISTRATION_TERMINATED:
			printf("Registration terminated\n");
			break;
		default:
			syslog_wrapper(LOG_DEBUG,
						   "recieved unknown eXosip event (type, did, cid) = (%d, %d, %d)",
						   event->type, event->did, event->cid);

		}
		eXosip_event_free(event);
	}

return 0;
}*/

#include <crtdbg.h>
int test(int argc,char* argv[])
{
	int c;
	int port = 10000;
	char *contact = NULL;
	char *fromuser = NULL;
	const char *localip = NULL;
	const char *firewallip = NULL;
	char *proxy = NULL;
	char *username = NULL;
	char *password = NULL;
	struct regparam_t regparam = { 0, 3600, 0 };
#ifdef OSIP_MT
	struct osip_thread *register_thread;
#endif
	int debug = 0;
	int nofork = 0;
	int i;

	for(i = 0; i < argc; ++i)
	{
		printf("arg: %s\n", argv[i]);
	}

	//create_window("sipua");

	if(eXosip_init())
	{
		syslog_wrapper(LOG_ERR, "eXosip_init failed");
		exit(1);
	}

	proxy = osip_strdup("sip:220.249.112.22");
	fromuser = osip_strdup("sip:111111@220.249.112.22");
	contact = osip_strdup("sip:111111@192.168.1.6:10000;transport=tcp");

	//if(eXosip_listen_addr(&eXosip, IPPROTO_UDP, NULL, port, AF_INET, 0))
	if(eXosip_listen_addr(IPPROTO_TCP, NULL, port, AF_INET, 0))
	{
		syslog_wrapper(LOG_ERR, "eXosip_listen_addr failed");
		exit(1);
	}

	eXosip_masquerade_contact("127.0.0.1", port);

	/*fym if(firewallip)
	{
		syslog_wrapper(LOG_INFO, "firewall address: %s:%i", firewallip, port);
		eXosip_masquerade_contact(firewallip, port);
	}*/

	eXosip_set_user_agent("CSIPUA");

	if (eXosip_add_authentication_info("111111", "111111", "password", NULL, NULL))
	{
			syslog_wrapper(LOG_ERR, "eXosip_add_authentication_info failed");
			exit(1);
	}

	{
		osip_message_t *reg = NULL;
		int i;
		
		regparam.regid = eXosip_register_build_initial_register(fromuser, proxy, contact, regparam.expiry * 2, &reg);
		
		if (regparam.regid < 1)
		{
			syslog_wrapper(LOG_ERR, "eXosip_register_build_initial_register failed");
			exit(1);
		}

		i = eXosip_register_send_register(regparam.regid, reg);
		if (i != 0) {
			syslog_wrapper(LOG_ERR, "eXosip_register_send_register failed");
			exit(1);
		}
	}

#ifdef OSIP_MT
	register_thread = osip_thread_create(20000, register_proc, &regparam);
	if (register_thread == NULL) {
		syslog_wrapper(LOG_ERR, "pthread_create failed");
		exit(1);
	}
#endif

	for (;;)
	{
		eXosip_event_t *event;

		if (!(event = eXosip_event_wait(0, 1)))
		{
#ifndef OSIP_MT
			eXosip_execute();
			eXosip_automatic_action();
#endif
			osip_usleep(10000);
			continue;
		}
#ifndef OSIP_MT
		eXosip_execute();
#endif

		eXosip_automatic_action();
		switch (event->type)
		{
		case EXOSIP_REGISTRATION_NEW:
			syslog_wrapper(LOG_INFO, "received new registration");
			break;
		case EXOSIP_REGISTRATION_SUCCESS:
			syslog_wrapper(LOG_INFO, "registrered successfully");
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			regparam.auth = 1;
			break;
		case EXOSIP_REGISTRATION_TERMINATED:
			printf("Registration terminated\n");
			break;
		default:
			syslog_wrapper(LOG_DEBUG,
				"recieved unknown eXosip event (type, did, cid) = (%d, %d, %d)",
				event->type, event->did, event->cid);

		}
		eXosip_event_free(event);
	}

	return 0;

}
