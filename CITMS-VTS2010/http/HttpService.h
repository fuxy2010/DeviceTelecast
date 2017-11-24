#ifndef _HTTP_SERVICE_H_       
#define _HTTP_SERVICE_H_

//#include "stdafx.h"
//#include "GeneralDef.h"

#ifdef _DEBUG
#pragma comment(lib, "../utility/poco-1.7.6/lib/PocoFoundationd.lib")
#pragma comment(lib, "../utility/poco-1.7.6/lib/PocoNetd.lib")
#pragma comment(lib, "../utility/poco-1.7.6/lib/PocoUtild.lib")
#else
#pragma comment(lib, "../utility/poco-1.7.6/lib/PocoFoundation.lib")
#pragma comment(lib, "../utility/poco-1.7.6/lib/PocoNet.lib")
#pragma comment(lib, "../utility/poco-1.7.6/lib/PocoUtil.lib")
#endif

#include <iostream>       // std::cout
#include "Locker.h"
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/AbstractHTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/PartHandler.h>
#include <Poco/Net/MessageHeader.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Exception.h>
#include <Poco/ErrorHandler.h>
#include <Poco/Util/TimerTaskAdapter.h>
#include <Poco/Util/Timer.h>
#include <Poco/URI.h>

//http://blog.csdn.net/fengyishang/article/details/45220823
//http://blog.csdn.net/pud_zha/article/details/8820604
//http://blog.csdn.net/yue7603835/article/details/41739207

namespace Poco {
	namespace Net {
		class HTTPServer;
	};
	namespace Util {
		class Timer;
		class TimerTask;
	}
}

namespace HttpServer
{
	class TiXmlElement;

	class CHttpService
	{
	public:
		static CHttpService& GetInstance();

		CHttpService();
		~CHttpService();

		void Initialize();
		void Uninitialize();

		// 开启HTTP服务线程
		void Start(const std::string& ipAddr, int port);
		// 停止HTTP服务线程
		void Stop();

	private:
		void DoStart(Poco::Util::TimerTask& task);


	private:
		std::string m_ipAddr;
		int			m_port;//{};
		Poco::Net::HTTPServer *	m_server;	// HTTP Server instance
		CSSMutex	m_srvMutex;
		Poco::Util::Timer *		m_timer;

		CSSMutex m_dirMutex;

		int m_state;
		bool m_running;
	};
}

#endif
