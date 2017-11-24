// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
// ScheduleServer.cpp : Defines the entry point for the console application.
//
#include "ScheduleServer.h"
#include "CommandContextParse.h"
//#include "ICECommunicator.h"
#include "MediaData.h"
//#include "json.h"
#include "TimeConsuming.h"
#include "resource.h"
#include "VUASendMediaTask.h"
#include "RTSPRecvTask.h"
#include "RTMPPushTask.h"

#include "HttpService.h"

using namespace ScheduleServer;

double CTimeConsuming::_clock_frequency = 0;

CScheduleServer::CScheduleServer() :
_cur_path(""),
_enalble(false),
_register_thread(NULL)
{
	_ua_map.clear();
}

CScheduleServer::~CScheduleServer()
{
}

#
SS_Error CScheduleServer::start(std::string path)
{
	//设置当前程序路径
	_cur_path = path;

	//根据系统时间设置随机数
	::srand(timeGetTime());
		
	WSADATA dat;
	WSAStartup(MAKEWORD(2,2),&dat);

	//获取系统信息
	SYSTEM_INFO theSystemInfo;
	::GetSystemInfo(&theSystemInfo);

	//启动任务线程////////////////////////////////////////////////////////////////////////
	CTaskThreadPool::add_threads((!(theSystemInfo.dwNumberOfProcessors) ? 1 : theSystemInfo.dwNumberOfProcessors), this);

	SINGLETON(HttpServer::CHttpService).Initialize();
	SINGLETON(HttpServer::CHttpService).Start(SINGLETON(CConfigBox).get_property("HttpServer", "localhost"), SINGLETON(CConfigBox).get_property_as_int("HttpServerPort", 80));

	_enalble = true;//服务可用

	return SS_NoErr;

}

SS_Error CScheduleServer::shutdown()
{
	_enalble = false;

	//关闭任务线程////////////////////////////////////////////////////////////////////////
	CTaskThreadPool::remove_threads();

	LOG_WRITE("Task threads have been removed!", 1, true);

	//删除所有UA////////////////////////////////////////////////////////////////////////
	remove_all_ua();

	//关闭socket////////////////////////////////////////////////////////////////////////
	WSACleanup();//for JRTP

	return SS_NoErr;
}

void CScheduleServer::write_log(std::string& log, int level, bool show_on_screen)
{
	SINGLETON(CLogger).trace_out(log, level, DEFAULT_TRACE_TYPE, show_on_screen);
}

SS_Error CScheduleServer::add_task(CTask* task, unsigned long index)
{
	if(false == _enalble)
		return SS_AddTaskFail;

	if(NULL  == task)
		return SS_InvalidTask;

	CTaskThread* task_thread = CTaskThreadPool::select_thread(index);

	if(NULL == task_thread)
		return SS_AddTaskFail;

	if(SS_NoErr != task_thread->add_task(task))
	{
		return SS_AddTaskFail;
	}

	return SS_NoErr;
}

int CScheduleServer::start_rtmp_push(const unsigned long uuid)
{
#if 0
	reg_ua(uuid, UA_MobilePhone);

	CUserAgent* ua = SINGLETON(CScheduleServer).fetch_ua(uuid);
	
	int ret = -1;
	if(NULL != ua)
	{
		string url = "rtmp://";
		url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
		url += "/hls/";
		url += MiscTools::parse_type_to_string<unsigned long>(uuid);
		ret = 1;//ua->rtmp_connect(const_cast<char*>(url.c_str()));
		if(!ret)
			ua->_start_hls = true;
		else
			return ret;
	}

	memset(ua->_dh_video_frame, 0, sizeof(ua->_dh_video_frame));
	ua->_dh_video_frame_length = 0;
	ua->_last_dh_video_type = NAL_INVALID;

	VUA_SEND_MEDIA_TASK_INFO task_info;

	task_info;
	task_info.task_id = ::timeGetTime();
	task_info.ua_id = uuid;
	task_info.send_audio = false;

	//创建模拟UA发送音视频任务
	_video_task = new CVUASendMediaTask(task_info);

	if(SS_NoErr != SINGLETON(CScheduleServer).add_task(_video_task, task_info.task_id))
	{
		delete _video_task;
		_video_task = NULL;
	}
#endif

	return 0;//ret;
}

int CScheduleServer::stop_rtmp_push(unsigned long uuid)
{
	return 0;
}

void CScheduleServer::singleton_test()
{
	cout << "<ScheduleServer: " << reinterpret_cast<unsigned long>(this) << "> ";
}

void CScheduleServer::print_contex(const SS_CTX& ctx, const bool to_send)
{
	std::string content = "";

	for(SS_CTX::const_iterator iter = ctx.begin();
		iter != ctx.end();
		++iter)
	{
		content += iter->first;
		content += ",";
		content += iter->second;
		content += ";\r\n";
	}

	if(false == to_send)
	{
		LOG_WRITE("\n<<<<<< Receive ICE request >>>>>>\n" + content, 1, true);
	}
	else
	{
		LOG_WRITE("\n<<<<<< Send ICE request >>>>>>\n" + content, 1, true);
	}
}

void CScheduleServer::shutdown_cast(unsigned long id)
{
	CSSLocker lock(&_shutdown_cast_mutex);
#if 0
	/*CRTMPPushTask* rtmp_task = dynamic_cast<CRTMPPushTask*>(_rtmp_task_map[id]);
	if(NULL != rtmp_task)
	{
		rtmp_task->shutdown();
	}

	CRTSPRecvTask* rtsp_task = dynamic_cast<CRTSPRecvTask*>(_pull_task_map[id]);
	if(NULL != rtsp_task)
	{
		rtsp_task->shutdown();
	}*/
#else
	CTask* task = _push_task_map[id];
	if(NULL != task)
	{
		task->shutdown();
	}
	_push_task_map.erase(id);

	task = _pull_task_map[id];
	if(NULL != task)
	{
		task->shutdown();
	}
	_pull_task_map.erase(id);
#endif

	remove_livecast_map(SINGLETON(CScheduleServer).query_livecast_request(MiscTools::parse_type_to_string<unsigned long>(id)));
	remove_vod_map(SINGLETON(CScheduleServer).query_vod_request(MiscTools::parse_type_to_string<unsigned long>(id)));
}

void CScheduleServer::update_expire(unsigned long id, unsigned long expire)
{
	CTask* task = _push_task_map[id];
	if(NULL != task)
	{
		task->update_expire(expire);
	}

	task = _pull_task_map[id];
	if(NULL != task)
	{
		task->update_expire(expire);
	}
}

#include "DPStreamAccessPlugin.h"
#include "DPSDKLivePullTask.h"
void CScheduleServer::dh_test()
{
	return;

	{
		SDK_RECV_TASK_INFO task_info;

		task_info.task_id = 123;
		task_info.ua_id = 123;

		task_info.sdk_url = "";

		//创建模拟UA发送音视频任务
		CDPSDKLivePullTask* task = new CDPSDKLivePullTask(task_info);
		
		if(SS_NoErr != SINGLETON(CScheduleServer).add_task(task, task_info.task_id))
		{
			delete task;
			task = NULL;
		}
		return;
	}	

	unsigned long id = 888;

	CDPStreamAccessPlugin plugin;

	plugin.init();

	REQUEST_INFO request_info;
	request_info.operation = REAL_START;
	request_info.source_info.uuid = timeGetTime();
	request_info.source_info.device_info.ip = "192.168.0.190";
	request_info.source_info.device_info.port = 9000;
	request_info.source_info.device_info.user = "fym";
	request_info.source_info.device_info.password = "88888888";
	request_info.source_info.channel = 0;
	request_info.source_info.uuid = id;//timeGetTime();

	start_rtmp_push(request_info.source_info.uuid);

	unsigned long t;
	plugin.access_stream(request_info, NULL, 0, NULL, t);
	
	//return;

	reg_ua(request_info.source_info.uuid, UA_MobilePhone);

	CUserAgent* ua = SINGLETON(CScheduleServer).fetch_ua(request_info.source_info.uuid);

	//int ret = -1;
	if(NULL == ua) return;

#if 1
	//RTMP
	//unsigned long id = 123;
	string hls_url = "";
	CRTMPPushTask* rtmp_push_task = NULL;
	CRTSPRecvTask* rtsp_recv_task = NULL;
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
			return;
		}

		if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtmp_push_task, task_info.task_id))
		{
			delete rtmp_push_task;
			rtmp_push_task = NULL;

			return;
		}
	}
#endif
}

#if 0
#define __STDC_CONSTANT_MACROS
#define	__STDC_LIMIT_MACROS
#define UINT_LEAST64_MAX UINT64_MAX
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus 
}
#endif
#endif

int CScheduleServer::rtsp_test()
{
	return 0;

	unsigned long id = 123;
	CRTMPPushTask* rtmp_push_task = NULL;
	CRTSPRecvTask* rtsp_recv_task = NULL;
	string hls_url = "";

	//RTMP
	{
		RTMP_PUSH_TASK_INFO task_info;

		task_info.task_id = id;
		task_info.ua_id = id;
		task_info.rtmp_url = "rtmp://";
		task_info.rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
		task_info.rtmp_url += "/hls/123";
		//task_info.rtmp_url = "rtmp://121.41.15.6/hls/123";
		//task_info.rtmp_url = "rtmp://192.168.0.123/hls/123";

		//创建模拟UA发送音视频任务
		rtmp_push_task = new CRTMPPushTask(task_info);

		if(false == rtmp_push_task->is_initialized())
			return -1;

		if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtmp_push_task, task_info.task_id))
		{
			delete rtmp_push_task;
			rtmp_push_task = NULL;

			return -2;
		}
	}

	//RTSP
	{
		RTSP_RECV_TASK_INFO task_info;

		task_info.task_id = id + 1;
		task_info.ua_id = id;

		task_info.rtsp_url = "rtsp://192.168.0.190:9090/dss/monitor/params?cameraid=1000004%240&substream=1";
		//task_info.rtsp_url = "rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004%247%26begintime=1482474300%26endtime=1482474397%26substream=1";
		//task_info.rtsp_url = "rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004$7&begintime=1482474300&endtime=1482474397&substream=1";

		if(string::npos != task_info.rtsp_url.find("$"))
			task_info.rtsp_url = task_info.rtsp_url.replace(task_info.rtsp_url.find("$"), 1, "%24");

		
		//创建模拟UA发送音视频任务
		rtsp_recv_task = new CRTSPRecvTask(task_info);

		if(false == rtsp_recv_task->is_initialized())
		{
			rtmp_push_task->shutdown();
			return -3;
		}

		if(SS_NoErr != SINGLETON(CScheduleServer).add_task(rtsp_recv_task, task_info.task_id))
		{
			delete rtsp_recv_task;
			rtsp_recv_task = NULL;

			rtmp_push_task->shutdown();

			return -5;
		}

		hls_url = "http://";
		hls_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
		hls_url += ":";
		hls_url += SINGLETON(CConfigBox).get_property("HLSServerPort", "8080");
		hls_url += "/hls/";
		hls_url += MiscTools::parse_type_to_string<unsigned long>(id);
		hls_url += ".m3u8";
	}

	//success////////////////////////////////////////////////////////////////////////

	return 0;

	unsigned long time = timeGetTime();

	{
		RTMP_PUSH_TASK_INFO task_info;

		task_info.task_id = time + 1;
		task_info.ua_id = SINGLETON(CConfigBox).get_property_as_int("HLSID", 123);

		task_info.rtmp_url = "rtmp://";
		task_info.rtmp_url += SINGLETON(CConfigBox).get_property("HLSServer", "localhost");
		task_info.rtmp_url += ":" + SINGLETON(CConfigBox).get_property("RTMPServerPort", "1935");
		task_info.rtmp_url += "/hls/";
		task_info.rtmp_url += MiscTools::parse_type_to_string<unsigned long>(task_info.ua_id);
		//task_info.rtmp_url = "rtmp://121.41.15.6/hls/123";
		//task_info.rtmp_url = "rtmp://192.168.0.123/hls/123";

		//创建模拟UA发送音视频任务
		CRTMPPushTask* _rtmp_push_task = new CRTMPPushTask(task_info);

		if(SS_NoErr != SINGLETON(CScheduleServer).add_task(_rtmp_push_task, task_info.task_id))
		{
			delete _rtmp_push_task;
			_rtmp_push_task = NULL;
		}
	}
	
	{
		RTSP_RECV_TASK_INFO task_info;

		task_info.task_id = time;
		task_info.ua_id = 123;
		task_info.rtsp_url = SINGLETON(CConfigBox).get_property("RTSPURL", "rtsp://192.168.0.190:9090/dss/monitor/params?cameraid=1000004%240&substream=1");

		//创建模拟UA发送音视频任务
		CRTSPRecvTask* _rtsp_recv_task = new CRTSPRecvTask(task_info);

		if(SS_NoErr != SINGLETON(CScheduleServer).add_task(_rtsp_recv_task, task_info.task_id))
		{
			delete _rtsp_recv_task;
			_rtsp_recv_task = NULL;
		}
	}
	
#if 0
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	//AVCodecContext	*pCodecCtx;
	//AVCodec			*pCodec;
	//AVFrame	*pFrame,*pFrameYUV;
	AVPacket *packet;
	int ret, got_picture;

	//------------SDL----------------
	//int screen_w,screen_h;
	//SDL_Window *screen; 
	//SDL_Renderer* sdlRenderer;
	//SDL_Texture* sdlTexture;
	//SDL_Rect sdlRect;
	//SDL_Thread *video_tid;
	//SDL_Event event;

	//struct SwsContext *img_convert_ctx;

	char filepath[]="rtsp://192.168.0.190:9090/dss/monitor/params?cameraid=1000004%240&substream=1";

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if(0 != avformat_open_input(&pFormatCtx,filepath,NULL,NULL)){
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if(avformat_find_stream_info(pFormatCtx, NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}

	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
			break;
		}
	}

	if(videoindex==-1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}

	start_rtmp_push(123);
	
	/*pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}

	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}*/
	
	//pFrame=av_frame_alloc();
	//pFrameYUV=av_frame_alloc();
		
	//Output Info-----------------------------
	printf("---------------- File Information ---------------\n");
	av_dump_format(pFormatCtx,0,filepath,0);
	printf("-------------------------------------------------\n");
		
	packet=(AVPacket *)av_malloc(sizeof(AVPacket));

	bool start_push = false;
	
	unsigned long frame_timestamp = timeGetTime();
	bool start_recv = false;
	while(0 <= av_read_frame(pFormatCtx, packet))
	{
		if(packet->stream_index==videoindex)
		{
			unsigned char* dat = packet->data;
			int length = packet->size;

			//TRACE("\n<F %d %x %x %x %x %x %d", packet->size, dat[0], dat[1], dat[2], dat[3], dat[4]);
			//continue;
			unsigned long t = timeGetTime();
			
			//TRACE("\n<G %d", t - frame_timestamp);
			if(70 <= t - frame_timestamp)
			{
				start_recv = true;
			}
			frame_timestamp = t;

			if(false == start_recv) continue;

			if(0 == dat[0] && 0 == dat[1] && 0 == dat[2] && 1 == dat[3] && NAL_SPS == dat[4] & 0x1f)
			{
				start_push  = true;
			}

			if(false == start_push) continue;		
			
			if(false)
			{
				FILE* f = fopen("e:\\rtsp.dat", "ab+");
				fwrite(dat, 1, length, f);
				fclose(f);
				continue;
			}

			CUserAgent* ua = SINGLETON(CScheduleServer).fetch_ua(123);
			if(NULL == ua) continue;

			int offset = 0;
			NAL_TYPE type =  CRTMPSession::get_video_packet_type(dat, length, offset);
			unsigned long timestamp = timeGetTime();

			if(NAL_SPS == type)
			{
				timestamp = timeGetTime();			

				//sps
				int sps_pos = 0;
				int pps_pos = 0;
				int sei_pos = 0;
				int idr_pos = 0;

				CRTMPSession::find_frame_header4(dat + sps_pos + 4, length - sps_pos - 4, offset);
				pps_pos = sps_pos + offset + 4;

				CRTMPSession::find_frame_header4(dat + pps_pos + 4, length - pps_pos- 4, offset);
				sei_pos = pps_pos + offset + 4;

				CRTMPSession::find_frame_header4(dat + sei_pos + 4, length - sei_pos- 4, offset);
				idr_pos = sei_pos + offset + 4;

				unsigned long t = timeGetTime();

				//sps
				ua->_dh_video_frame_length = pps_pos - sps_pos;
				memcpy(ua->_dh_video_frame, dat + sps_pos, ua->_dh_video_frame_length);
				ua->add_dh_video_frame(ua->_dh_video_frame, ua->_dh_video_frame_length, timestamp);
				//TRACE("\n<B %d %d", type, ua->_dh_video_frame_length);
				ua->_dh_video_frame_length = 0;
		
				//pps
				ua->_dh_video_frame_length = sei_pos - pps_pos;
				memcpy(ua->_dh_video_frame, dat + pps_pos, ua->_dh_video_frame_length);
				ua->add_dh_video_frame(ua->_dh_video_frame, ua->_dh_video_frame_length, timestamp);
				//TRACE("\n<C %d %d", type, ua->_dh_video_frame_length);
				ua->_dh_video_frame_length = 0;		

				//sei
				ua->_dh_video_frame_length = idr_pos - sei_pos;
				memcpy(ua->_dh_video_frame, dat + sei_pos, ua->_dh_video_frame_length);
				ua->add_dh_video_frame(ua->_dh_video_frame, ua->_dh_video_frame_length, timestamp);
				//TRACE("\n<D %d %d", type, ua->_dh_video_frame_length);
				ua->_dh_video_frame_length = 0;

				timestamp = timeGetTime();
		
				//idr
				ua->_dh_video_frame_length = length - idr_pos;
				memcpy(ua->_dh_video_frame, dat + idr_pos, ua->_dh_video_frame_length);
				ua->add_dh_video_frame(ua->_dh_video_frame, ua->_dh_video_frame_length, timestamp);
				//TRACE("\n<E %d %d", type, ua->_dh_video_frame_length);
				ua->_dh_video_frame_length = 0;

				ua->_last_dh_video_type = NAL_SLICE_IDR;

			}
			else if(NAL_SLICE == type)
			{
				//p
				ua->_dh_video_frame_length = length;
				memcpy(ua->_dh_video_frame, dat, ua->_dh_video_frame_length);
				ua->add_dh_video_frame(ua->_dh_video_frame, ua->_dh_video_frame_length, timestamp);
				//TRACE("\n<G %d %d", type, ua->_dh_video_frame_length);
				ua->_dh_video_frame_length = 0;

				ua->_last_dh_video_type = NAL_SLICE;
			}

			continue;

		}
		av_free_packet(packet);
	}
	
	//av_frame_free(&pFrameYUV);
	//av_frame_free(&pFrame);
	//avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
#endif
	
	return 0;
}

unsigned long _latest_timestamp = 0;
int interrupt_cb(void *ctx)
{
	if(_latest_timestamp && 5000 < timeGetTime() - _latest_timestamp)
	{
		TRACE("\nI 1");
		return 1;
	}

	//TRACE("\nI 0");

	return 0;
}

int CScheduleServer::rtsp_test2()
{
	return 0;
#if 1
	while(1)
	{
		AVFormatContext	*pFormatCtx;
		int				i, videoindex;
		AVPacket *packet;
		int ret, got_picture;

		char filepath[] = "rtsp://192.168.0.190:9090/dss/monitor/params?cameraid=1000004%240&substream=1";
		//char filepath[] = "rtsp://192.168.0.190:9320/dss/playback/pu?cameraid=1000004%241&begintime=1483411300&endtime=1483411330&substream=1";

		av_register_all();
		avformat_network_init();
		pFormatCtx = avformat_alloc_context();

		pFormatCtx->flags |= AVFMT_FLAG_NONBLOCK;
		pFormatCtx->interrupt_callback.callback = interrupt_cb;
		pFormatCtx->interrupt_callback.opaque = pFormatCtx;

		AVDictionary* options = NULL;
		av_dict_set(&options, "rtsp_transport", "tcp", 0);

		if(0 != avformat_open_input(&pFormatCtx, filepath, NULL, &options))
		{
			printf("Couldn't open input stream.\n");
			return -1;
		}

		if(avformat_find_stream_info(pFormatCtx, NULL)<0){
			printf("Couldn't find stream information.\n");
			return -1;
		}

		videoindex=-1;
		for(i=0; i<pFormatCtx->nb_streams; i++)
		{
			if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
			{
				videoindex=i;
				break;
			}
		}

		if(videoindex==-1)
		{
			printf("Didn't find a video stream.\n");
			return -1;
		}

		//Output Info-----------------------------
		printf("---------------- File Information ---------------\n");
		av_dump_format(pFormatCtx,0,filepath,0);
		printf("-------------------------------------------------\n");

		packet=(AVPacket *)av_malloc(sizeof(AVPacket));

		int count = 5000;

		while(0 <= av_read_frame(pFormatCtx, packet))
		{
			TRACE("\n<F RECV");

			_latest_timestamp = timeGetTime();

			if(packet->stream_index==videoindex)
			{
				unsigned char* dat = packet->data;
				int length = packet->size;

				TRACE("\n<F VIDEO %d %x %x %x %x %x %d", packet->size, dat[0], dat[1], dat[2], dat[3], dat[4]);

			}
			else
			{
				TRACE("\n<F AUDIO");
			}
			av_free_packet(packet);

			if(!count--) break;
		}

		TRACE("\n<F STOP");

		//av_frame_free(&pFrameYUV);
		//av_frame_free(&pFrame);
		//avcodec_close(pCodecCtx);
		avformat_close_input(&pFormatCtx);

		Sleep(3000);
	}	
#endif
	
	return 0;
}

#include "librtmp_send264.h"

FILE *fp_send1;
int read_buffer1(unsigned char *buf, int buf_size)
{
	if(!feof(fp_send1)){
		int true_size=fread(buf,1,buf_size,fp_send1);
		return true_size;
	}else{
		return -1;
	}
}

void hk_264()
{
	FILE* file = fopen("temp.dat", "rb");

	if(NULL == file)
		return;

	long index = 0;
	long last_frame_index = 0;
	bool start_write = false;

	while(!feof(file))
	{
		unsigned char t[8];
		fread(t, sizeof(unsigned char), sizeof(t), file);

		if(0 == t[0] && 0 == t[1] && 0 == t[2] && 1 == t[3])
		{
			TRACE("\n<Frame %d length: %d @ %d", t[4] & 0x1f, index - last_frame_index, index);

			if(8 == (t[4] & 0x1f)) start_write = true;

			if(last_frame_index && true == start_write)
			{
				unsigned long frame_len = index - last_frame_index;

				fseek(file, last_frame_index, SEEK_SET);

				unsigned char *frame = new unsigned char[frame_len];

				fread(frame, sizeof(unsigned char), frame_len, file);

				FILE* f = fopen("hk.264", "ab+");
				fwrite(&frame_len, sizeof(unsigned long), 1, f);
				fwrite(frame, 1, frame_len, f);
				fclose(f);

				TRACE("\n------------------ <Write %d %d", frame[4] & 0x1f, frame_len);

				delete frame;
				frame = NULL;
			}

			last_frame_index = index;
		}
		
		fseek(file, ++index, SEEK_SET);
	}
}

#include <Poco/URI.h>
#include <Poco/NumberFormatter.h>
#include <Poco/String.h>

void resolve_uri1()
{
	//string url = "http://127.0.0.1:80/api/startrealplay?connectstring=rtsp://192.168.0.190:9090/dss/monitor/params?cameraid=1000004%240&substream=1";
	//string url = "http://192.168.0.245:8089/api/startrealplay?connectstring=rtsp%3A%2F%2Fadmin%3Aadmin%3A192.168.7.11%2Fcam%2Frealmonitor%3Fchannel%3D1%26subtype%3D0&_=149846188816";
	string url = "ivms8600://fym:12345@192.168.0.1:8000/params?cameraid=1000004&condition=x&begintime=1482474300&endtime=1482474397&fps=25";

	Poco::URI uri(url);
	std::string userInfo = uri.getUserInfo();

	string _username = userInfo.substr(0, userInfo.find(':'));
	if (userInfo.length() > _username.length())
	{
		string _password = userInfo.substr(_username.length() + 1);
	}
	string _ip = uri.getHost();
	string _port = Poco::NumberFormatter::format(uri.getPort());
	auto paramsVector = uri.getQueryParameters();
	for (auto i = 0; i < paramsVector.size(); ++i) {
		std::string key = Poco::toLower(paramsVector[i].first);
		std::string val = paramsVector[i].second;

		string pair = key + " : " + val;
		cout << pair << endl;
		/*if ("cameraid" == key)
		_camera_id = val;
		else if ("condition" == key)
		_condition = val;
		else if ("begintime" == key)
		_begin_time = MiscTools::parse_string_to_type<unsigned long long>(val);
		else if ("endtime" == key)
		_end_time = MiscTools::parse_string_to_type<unsigned long long>(val);
		else if ("dev" == key) {
		std::string tmpType = Poco::toLower(val);
		if ("dh" == tmpType)
		_dev_type = IVMS8600_DH;
		else if ("hk" == tmpType)
		_dev_type = IVMS8600_HK;*/
	}

}

void resolve_uri2()
{
	//string url = "http://127.0.0.1:80/api/startrealplay?connectstring=rtsp://192.168.0.190:9090/dss/monitor/params?cameraid=1000004%240&substream=1";
	string url = "http://192.168.0.245:8089/api/startrealplay?connectstring=rtsp%3A%2F%2Fadmin%3Aadmin%3A192.168.7.11%2Fcam%2Frealmonitor%3Fchannel%3D1%26subtype%3D0&_=149846188816";
	//string url = "ivms8600://fym:12345@192.168.0.1:8000/params?cameraid=1000004&condition=x&begintime=1482474300&endtime=1482474397&fps=25";

	Poco::URI uri(url);
	
	auto paramsVector = uri.getQueryParameters();
	for (auto i = 0; i < paramsVector.size(); ++i) {
		std::string key = Poco::toLower(paramsVector[i].first);
		std::string val = paramsVector[i].second;

		string pair = key + " : " + val;
		cout << pair << endl;
		/*if ("cameraid" == key)
		_camera_id = val;
		else if ("condition" == key)
		_condition = val;
		else if ("begintime" == key)
		_begin_time = MiscTools::parse_string_to_type<unsigned long long>(val);
		else if ("endtime" == key)
		_end_time = MiscTools::parse_string_to_type<unsigned long long>(val);
		else if ("dev" == key) {
		std::string tmpType = Poco::toLower(val);
		if ("dh" == tmpType)
		_dev_type = IVMS8600_DH;
		else if ("hk" == tmpType)
		_dev_type = IVMS8600_HK;*/
	}

}

int CScheduleServer::vod_test()
{
	//FILE* file = fopen("hbdddj.dat", "rb");
	//FILE* file = fopen("yhlkk.dat", "rb");
	//FILE* file = fopen("glldj.dat", "rb");
	FILE* file = fopen("ivms_vod2.dat", "rb");

	if (NULL == file)
		return 0;

	while (!feof(file))
	{
		unsigned long flag = 0;
		fread(&flag, sizeof(unsigned long), 1, file);

		int length = 0;
		fread(&length, sizeof(int), 1, file);

		TRACE("\n<HK0 %x %d", flag, length);

		fseek(file, length, SEEK_CUR);

		continue;

		unsigned char* temp = new unsigned char[length];
		fread(temp, sizeof(unsigned char), length, file);		

		//TRACE("\n<HK0 %x %d %x %x %x %x", flag, length, temp[0], temp[1], temp[2], temp[3]);
		//TRACE("\n<HK1 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK2 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK0 %x %d >> %x %x %x %x <-> %x %x %x %x", flag, length, temp[0], temp[1], temp[2], temp[3], temp[length - 4], temp[length - 3], temp[length - 2], temp[length - 1]);

		if (false)
		{
			FILE* f = fopen("temp.dat", "ab+");
			fwrite(temp, 1, length, f);
			fclose(f);
		}

		delete temp;
		temp = NULL;
	}

	TRACE("\n<HK0 ==================");
}

int CScheduleServer::h264_test()
{
#if 0
	resolve_uri2();
	return 0;

	//hk_264();
	//return 0;

	FILE* file = fopen("hk.dat", "rb");

	if(NULL == file)
		return 0;

	while(!feof(file))
	{
		unsigned long flag = 0;
		fread(&flag, sizeof(unsigned long), 1, file);
		
		int length = 0;
		fread(&length, sizeof(int), 1, file);

		unsigned char *temp = new unsigned char[length];
		fread(temp, sizeof(unsigned char), length, file);

		//TRACE("\n<HK0 %x %d %x %x %x %x", flag, length, temp[0], temp[1], temp[2], temp[3]);
		//TRACE("\n<HK1 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		//TRACE("\n<HK2 %x %d %x %x %x %x", flag, length, temp[40], temp[41], temp[42], temp[43]);
		TRACE("\n<HK0 %x %d >> %x %x %x %x <-> %x %x %x %x", flag, length, temp[0], temp[1], temp[2], temp[3], temp[length - 4], temp[length - 3], temp[length - 2], temp[length - 1]);

		if(false)
		{
			FILE* f = fopen("temp.dat", "ab+");
			fwrite(temp, 1, length, f);
			fclose(f);
		}

		delete temp;
		temp = NULL;
	}

	return 0;

	while(1)
	{
		//fp_send1 = fopen("cuc_ieschool.h264", "rb");
		//fp_send1 = fopen("e:\\800600.264", "rb");
		fp_send1 = fopen("d:\\Current\\rtsp.dat", "rb");

		//初始化并连接到服务器
		//RTMP264_Connect("rtmp://121.41.15.6/hls/123");
		RTMP264_Connect("rtmp://192.168.0.123/hls/111");

		//发送
		RTMP264_Send(read_buffer1);

		//断开连接并释放相关资源
		RTMP264_Close();

		Sleep(2000);
	}
#endif
	return 0;
}

#include "RTSPRecvTask.h"
#include "RTMPPushTask.h"
#include "DPSDKLivePullTask.h"
#include "DPSDKVodPullTask.h"
#include "HKPSDKLivePullTask.h"
#include "HKPSDKVodPullTask.h"
#include "HIKSDKLivePullTask.h"
#include "HIKSDKVodPullTask.h"
int CScheduleServer::http_test()
{
	//http://127.0.0.1:80/api/startrealplay?connectstring=ivms8600%3A%2F%2Fadmin%3AHsjj1590%40192.168.5.211%3A6600%2Fparams%3Fdev%3Dhk%26cameraid%3D1925%26fps%3D25&_=1499738463489
	string uri = "http://127.0.0.1:80/api/startrealplay?connectstring=ivms8600%3A%2F%2Fadmin%3AHsjj1590%40192.168.5.211%3A6600%2Fparams%3Fdev%3Dhk%26cameraid%3D1925%26fps%3D25&_=1499738463489";

	size_t pos = uri.find("connectstring=") + 14;
	string url = uri.substr(pos, uri.length() - pos);

	TRACE("\n<LIVE SDK START %s", url.c_str());

	if (NULL == strstr(url.c_str(), "dsssdk")
		&& NULL == strstr(url.c_str(), "DSSSDK")
		&& NULL == strstr(url.c_str(), "ivms8600")
		&& NULL == strstr(url.c_str(), "IVMS8600")
		&& NULL == strstr(url.c_str(), "hiksdk")
		&& NULL == strstr(url.c_str(), "HIKSDK"))
		return 0;

	LiveCastRequest req;
	req.url = url;

	if(true)
	{
		unsigned long id = 111;//timeGetTime() & 0xffffff;
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
			task_info.video_frame_interval = 40;//1000 / MiscTools::parse_string_to_type<float>(url.substr(url.find_last_of("=") + 1, url.length() - url.find_last_of("=") - 1));

												//创建模拟UA发送音视频任务
			rtmp_push_task = new CRTMPPushTask(task_info);

			if (false == rtmp_push_task->is_initialized())
			{
				return 0;
			}

			if (SS_NoErr != SINGLETON(CScheduleServer).add_task(rtmp_push_task, task_info.task_id))
			{
				delete rtmp_push_task;
				rtmp_push_task = NULL;

				return 0;
			}
		}

		//SDK
		{
			SDK_RECV_TASK_INFO task_info;

			task_info.task_id = id + 1;
			task_info.ua_id = id;

			task_info.sdk_url = req.url;
			{
				Poco::URI para_uri(task_info.sdk_url);
				auto parameters = para_uri.getQueryParameters();
				for (auto i = 0; i < parameters.size(); ++i)
				{
					task_info.context[Poco::toLower(parameters[i].first)] = parameters[i].second;
				}
			}

			//创建模拟UA发送音视频任务
			//sdk_recv_task = new CDPSDKLivePullTask(task_info);
			if (NULL != strstr(url.c_str(), "dsssdk") || NULL != strstr(url.c_str(), "DSSSDK"))
				sdk_recv_task = new CDPSDKLivePullTask(task_info);
			else if (NULL != strstr(url.c_str(), "ivms8600") || NULL != strstr(url.c_str(), "IVMS8600"))
				sdk_recv_task = new CHKPSDKLivePullTask(task_info);
			else if (NULL != strstr(url.c_str(), "hiksdk") || NULL != strstr(url.c_str(), "hiksdk"))
				sdk_recv_task = new CHIKSDKLivePullTask(task_info);

			if (false == sdk_recv_task->is_initialized())
			{
				rtmp_push_task->shutdown();
				return 0;
			}

			if (SS_NoErr != SINGLETON(CScheduleServer).add_task(sdk_recv_task, task_info.task_id))
			{
				delete sdk_recv_task;
				sdk_recv_task = NULL;

				rtmp_push_task->shutdown();

				return 0;
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
	}

	return 0;
}
