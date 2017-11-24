// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "RTSPRecvTask.h"
#include "MiscTool.h"
#include "TimeConsuming.h"

using namespace ScheduleServer;

CRTSPRecvTask::CRTSPRecvTask(RTSP_RECV_TASK_INFO& task_info) :
_status(RTSPRecvTaskBegin),
_start_push(false),
_start_recv(false),
//_last_frame_timestamp(0),
_last_packet_timestamp(0),
_ua(NULL),
_initialized(false),
_wait_for_pause_handle(::CreateEvent(NULL, TRUE/*非自动复位*/, FALSE/*初始状态*/, NULL))
{
	while(string::npos != task_info.rtsp_url.find("$")) task_info.rtsp_url = task_info.rtsp_url.replace(task_info.rtsp_url.find("$"), 1, "%24");
	//while(string::npos != task_info.rtsp_url.find("&")) task_info.rtsp_url = task_info.rtsp_url.replace(task_info.rtsp_url.find("&"), 1, "%26");

	_task_info = task_info;
	
	if(!init_rtsp())
	{
		_status = RTSPRecvTaskRTSP_Connected;
		_initialized = true;
	}
	else
	{
		_status = RTSPRecvTask_Done;
		_initialized = false;
	}
}

CRTSPRecvTask::~CRTSPRecvTask()
{
	_task_info.clear();

	::CloseHandle(_wait_for_pause_handle);
}

void CRTSPRecvTask::pause()
{
	::ResetEvent(_wait_for_pause_handle);

	_status = RTSPRecvTask_Pause;
}

void CRTSPRecvTask::restart(string& url)
{
	::WaitForSingleObject(_wait_for_pause_handle, 5000);

	_start_push = false;
	_start_recv = false;
	_initialized = false;

	pFormatCtx = NULL;
	pCodecCtx = NULL;
	pCodec = NULL;
	packet = NULL;
	video_index = -1;
	av_free_packet(packet);
	_last_dh_video_type = NAL_INVALID;
	_dh_video_frame_length = 0;

	avformat_close_input(&pFormatCtx);

	while(string::npos != url.find("$")) url = url.replace(url.find("$"), 1, "%24");
	//while(string::npos != url.find("&")) url = url.replace(url.find("&"), 1, "%26");

	_task_info.rtsp_url = url;

	if(!init_rtsp())
	{
		_status = RTSPRecvTaskRTSP_Connected;
		_initialized = true;
	}
	else
	{
		_status = RTSPRecvTask_Done;
		_initialized = false;
	}
}

map<unsigned long, unsigned long> CRTSPRecvTask::_latest_packet_timestamp;
int CRTSPRecvTask::rtsp_interrupt_callback(void* ctx)
{
	DWORD key = (DWORD)ctx;
	
	if(_latest_packet_timestamp.end() == _latest_packet_timestamp.find(key))
		return 0;

	if(_latest_packet_timestamp[key] && 2000 < timeGetTime() - _latest_packet_timestamp[key])
	{
		//TRACE("\nI 1");
		return 1;
	}

	//TRACE("\nI 0");
	return 0;
}

int CRTSPRecvTask::init_rtsp()
{
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	_latest_packet_timestamp[(DWORD)pFormatCtx] = 0;

	pFormatCtx->interrupt_callback.callback = CRTSPRecvTask::rtsp_interrupt_callback;
	pFormatCtx->interrupt_callback.opaque = pFormatCtx;

	AVDictionary* options = NULL;
	av_dict_set(&options, "rtsp_transport", "tcp", 0);

	int ret = avformat_open_input(&pFormatCtx, _task_info.rtsp_url.c_str(), NULL, &options);
	if(0 != ret)//avformat_open_input(&pFormatCtx, _task_info.rtsp_url.c_str(), NULL, &options))
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	video_index = -1;
	for(int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			video_index = i;
			break;
		}
	}

	if(-1 == video_index)
	{
		printf("Didn't find a video stream.\n");
		return -2;
	}

	/*pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec == NULL)
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
	av_dump_format(pFormatCtx, 0, _task_info.rtsp_url.c_str(), 0);
	printf("-------------------------------------------------\n");

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));//must intialize

	SINGLETON(CScheduleServer).reg_ua(_task_info.ua_id, UA_MobilePhone);
	_ua = SINGLETON(CScheduleServer).fetch_ua(_task_info.ua_id);

	if(NULL == _ua)
		return -3;

	memset(_dh_video_frame, 0, sizeof(_dh_video_frame));
	_dh_video_frame_length = 0;
	_last_dh_video_type = NAL_INVALID;

	return 0;
}

void CRTSPRecvTask::recv_frame()
{
	if(NULL == _ua)
	{
		return;
	}

	//TRACE("\n<R1 ");
	if(0 > av_read_frame(pFormatCtx, packet))
		return;

	if(packet->stream_index != video_index)
	{
		//TRACE("R3>");
		av_free_packet(packet);
		return;
	}

	//TRACE("R2>");

	_last_packet_timestamp = timeGetTime();
	_latest_packet_timestamp[(DWORD)pFormatCtx] = _last_packet_timestamp;

	unsigned char* dat = packet->data;
	int length = packet->size;

	//TRACE("\n<F %d %x %x %x %x %x %d", packet->size, dat[0], dat[1], dat[2], dat[3], dat[4]);

#if 0
	if(!_last_frame_timestamp)
	{
		_last_frame_timestamp = timeGetTime();
		return;
	}
	
	unsigned long t = timeGetTime();

	//TRACE("\n<G %d", t - frame_timestamp);
	if(70 <= t - _last_frame_timestamp)
	{
		_start_recv = true;
	}
	_last_frame_timestamp = t;

	if(false == _start_recv) return;
#endif

	if(0 == dat[0] && 0 == dat[1] && 0 == dat[2] && 1 == dat[3] && 0x67 == dat[4]) _start_push = true;//wait for idr frame

	if(false == _start_push) 
	{
		av_free_packet(packet);
		return;
	}

	if(false)
	{
		FILE* f = fopen("d:\\rtsp.dat", "ab+");
		fwrite(dat, 1, length, f);
		fclose(f);

		av_free_packet(packet);
		return;
	}

	if(false == (0 == dat[0] && 0 == dat[1] && 0 == dat[2] && 1 == dat[3]))
	{
		av_free_packet(packet);
		return;
	}

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
		_dh_video_frame_length = pps_pos - sps_pos;
		memcpy(_dh_video_frame, dat + sps_pos, _dh_video_frame_length);
		add_dh_video_frame(_dh_video_frame, _dh_video_frame_length, timestamp);
		//TRACE("\n<B %d %d", type, ua->_dh_video_frame_length);
		_dh_video_frame_length = 0;

		//pps
		_dh_video_frame_length = sei_pos - pps_pos;
		memcpy(_dh_video_frame, dat + pps_pos, _dh_video_frame_length);
		add_dh_video_frame(_dh_video_frame, _dh_video_frame_length, timestamp);
		//TRACE("\n<C %d %d", type, ua->_dh_video_frame_length);
		_dh_video_frame_length = 0;

		//sei
		_dh_video_frame_length = idr_pos - sei_pos;
		memcpy(_dh_video_frame, dat + sei_pos, _dh_video_frame_length);
		add_dh_video_frame(_dh_video_frame, _dh_video_frame_length, timestamp);
		//TRACE("\n<D %d %d", type, ua->_dh_video_frame_length);
		_dh_video_frame_length = 0;

		timestamp = timeGetTime();

		//idr
		_dh_video_frame_length = length - idr_pos;
		memcpy(_dh_video_frame, dat + idr_pos, _dh_video_frame_length);
		add_dh_video_frame(_dh_video_frame, _dh_video_frame_length, timestamp);
		//TRACE("\n<E %d %d", type, ua->_dh_video_frame_length);
		_dh_video_frame_length = 0;

		_last_dh_video_type = NAL_SLICE_IDR;

	}
	else if(NAL_SLICE == type)
	{
		//p
		_dh_video_frame_length = length;
		memcpy(_dh_video_frame, dat, _dh_video_frame_length);
		add_dh_video_frame(_dh_video_frame, _dh_video_frame_length, timestamp);
		//TRACE("\n<G %d %d", type, ua->_dh_video_frame_length);
		_dh_video_frame_length = 0;

		_last_dh_video_type = NAL_SLICE;
	}

	av_free_packet(packet);
}

SS_Error CRTSPRecvTask::run()
{
	CTimeConsuming tc('A', 10.0);

	if(_expire && _expire * CLOCKS_PER_SEC <= timeGetTime() - _start_time)
	{
		SINGLETON(CScheduleServer).remove_livecast_map(SINGLETON(CScheduleServer).query_livecast_request(_task_info.ua_id));
		SINGLETON(CScheduleServer).remove_vod_map(SINGLETON(CScheduleServer).query_vod_request(_task_info.ua_id));

		_status = RTSPRecvTask_Done;
	}

	//10秒未收到数据自动终止任务
	if(_last_packet_timestamp && 10 * CLOCKS_PER_SEC < (timeGetTime() - _last_packet_timestamp))
	{
		SINGLETON(CScheduleServer).shutdown_cast(_task_info.ua_id);
	}

	if(RTSPRecvTask_Done == _status)
	{
		on_done();
		return SS_NoErr;
	}

	if(RTSPRecvTask_Pause == _status)
	{
		::SetEvent(_wait_for_pause_handle);

		return SS_NoErr;
	}

	switch(_status)
	{
	case RTSPRecvTaskBegin:
		_status = (!init_rtsp()) ? RTSPRecvTaskRTSP_Connected : RTSPRecvTaskRTSP_Connected;
		break;

	case RTSPRecvTaskRTSP_Connected:
		_status = RTSPRecvTaskRTSP_RECV;
		break;

	case RTSPRecvTaskRTSP_RECV:
		recv_frame();
		break;

	case RTSPRecvTask_Done:
		on_done();
		break;
	}

	return SS_NoErr;

}

SS_Error CRTSPRecvTask::on_done()
{
	avformat_close_input(&pFormatCtx);

	_task_info.clear();

	_is_done = true;

	return SS_NoErr;
}

SS_Error CRTSPRecvTask::on_exception()
{
	return SS_NoErr;
}
