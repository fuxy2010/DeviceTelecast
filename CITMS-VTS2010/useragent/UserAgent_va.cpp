// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "UserAgent.h"
#include "MiscTool.h"
#include "JRTPSession.h"
#include "ScheduleServer.h"
#include "resource.h"
#include "winbase.h"

using namespace ScheduleServer;

const unsigned long CUserAgent::_max_audio_packet_num = 100;//音频数据包队列最大长度，音频包到达时间不均匀故加大此值
const unsigned long CUserAgent::_max_video_packet_num = 100;//视频数据包队列最大长度

CSSMutex CUserAgent::_rtmp_send_mutex;

CUserAgent::CUserAgent(USER_AGENT_INFO& info) :
_start_hls(false),
_audio_codec(NULL),
_first_video_packet(true),
_first_video_timestamp(0),
_first_audio_packet(true),
_first_audio_timestamp(0)
{
	_info = info;

	remove_all_audio_packet();
	remove_all_video_packet();

	_audio_codec = new CWUACodec();

	_audio_statistics = new CUAStatistics(0);
	_audio_statistics->resume();

	_video_statistics = new CUAStatistics(1);
	_video_statistics->resume();

	//编码器必须重新创建，因为摄像设备的视频采集的分辨率可能会改变
	//_x264_handle = H264EncodeInit(_camera_ctrl->imgWidth, _camera_ctrl->imgHeight, 5, 1000, 10, 0);
	_x264_handle = H264EncodeInit(VIDEO_WIDTH, VIDEO_HEIGHT, 10, 1000, 30, 384, 1000 / VIDEO_SAMPLE_INTERVAL);

	if(1 > _x264_handle)
	{
		//MessageBox("编码器创建失败","提示",MB_ICONQUESTION|MB_OK);
		return;
	}

}

CUserAgent::~CUserAgent()
{
	H264EncodeClose(_x264_handle);

	delete _audio_codec;
	_audio_codec = NULL;

	delete _audio_statistics;
	_audio_statistics = NULL;

	delete _video_statistics;
	_video_statistics = NULL;


	remove_all_audio_packet();
	remove_all_video_packet();
}

SS_Error CUserAgent::remove_all_audio_packet()
{
	{
		CSSLocker lock(&_sample_audio_packet_list_mutex);

		for(std::list<AUDIO_PACKET_PTR>::iterator iter = _sample_audio_packet_list.begin();
			iter != _sample_audio_packet_list.end();
			++iter)
		{
			CMemPool::free_audio_packet(*iter);
		}

		_sample_audio_packet_list.clear();
	}

	return SS_NoErr;
}

SS_Error CUserAgent::remove_all_video_packet()
{
	{
		CSSLocker lock(&_sample_video_packet_list_mutex);

		for(std::list<VIDEO_PACKET_PTR>::iterator iter = _sample_video_packet_list.begin();
			iter != _sample_video_packet_list.end();
			++iter)
		{
			CMemPool::free_video_packet(*iter);
		}

		_sample_video_packet_list.clear();
	}

	return SS_NoErr;
}

SS_Error CUserAgent::add_sample_audio_packet(const char* frame, const unsigned long& length)
{
	int packet_len = _audio_codec->encode(reinterpret_cast<short*>(const_cast<char*>(frame)), _aac_packet);

	//TRACE("\n%d[%x, %x, %x, %x, %x, %x, %x, %x] > ", packet_len, _aac_packet[0], _aac_packet[1], _aac_packet[2], _aac_packet[3], _aac_packet[4], _aac_packet[5], _aac_packet[6], _aac_packet[7]);

	if(0 >= packet_len)
		return SS_NoErr;

	AUDIO_PACKET_PTR packet_ptr;
	CMemPool::malloc_audio_packet(packet_ptr);

	packet_ptr.packet->energy = 1;

	if(true == _first_audio_packet)
	{
		_first_audio_timestamp = timeGetTime();
		_first_audio_packet = false;

		packet_ptr.packet->sequence = 0xffff;
	}
	else
	{
		packet_ptr.packet->sequence = 0;
	}

	packet_ptr.packet->timestamp = ::timeGetTime() - _first_audio_timestamp;
	packet_ptr.packet->ua_id = 1;
	::memcpy(packet_ptr.packet->payload, _aac_packet, packet_len);
	packet_ptr.packet->payload_size = packet_len;

	{
		CSSLocker lock(&_sample_audio_packet_list_mutex);

		if(_max_audio_packet_num < _sample_audio_packet_list.size())
		{
			CMemPool::free_audio_packet(_sample_audio_packet_list.front());
			_sample_audio_packet_list.pop_front();
		}

		_sample_audio_packet_list.push_back(packet_ptr);
	}

	return SS_NoErr;
}

AUDIO_PACKET_PTR CUserAgent::fetch_sample_audio_packet()
{
	{
		CSSLocker lock(&_sample_audio_packet_list_mutex);

		//为平滑计队列中有3个以上的包才取 if(false == _raw_audio_frame_list.empty())
		if(10 <= _sample_audio_packet_list.size())
		{
			AUDIO_PACKET_PTR packet_ptr = _sample_audio_packet_list.front();//*(_audio_packet_list.begin());
			_sample_audio_packet_list.pop_front();

			//TRACE("<FA 3> ");

			return packet_ptr;
		}
	}

	AUDIO_PACKET_PTR packet_ptr;
	
	return packet_ptr;
}

int CUserAgent::input_video_frame(unsigned char* frame)
{
	//编码
	//long start = timeGetTime();
	int i_nal = H264EncodeFrame(_x264_handle, -1, frame, _stream_buf, _nal_len);
	//TRACE("\nVE<%d> ", timeGetTime() - start);

	if(i_nal<=0)
		return -1;

#if 0
	TRACE("\n=========\n");
	unsigned char* p = _stream_buf;
	int length = 0;
	for(int i = 0; i < i_nal; i++)
	{
		length += _nal_len[i];
		//TRACE("F<%d [%x, %x, %x, %x, %x] > ", _nal_len[i], *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4));
		int ret = _rtmp_session.input_video_packet(p, _nal_len[i], timeGetTime() - _start_time);
		//TRACE("F<%d %d> ", _nal_len[i], ret);//CRTMPSession::get_packet_type(p, _nal_len[i], seprator_len));

		p += _nal_len[i];
	}
#if 0
	{
		FILE* f = fopen("live.h264", "ab+");
		fwrite(_stream_buf, 1, length, f);
		fclose(f);
	}
#endif
#endif

	if(true == _first_video_packet)
	{
		_first_video_timestamp = timeGetTime();
		_first_video_packet = false;
	}

	//写码流文件
	unsigned long timestamp = timeGetTime() - _first_video_timestamp;//同一帧内数据包均采用同样的时戳

	unsigned char *p = _stream_buf;
	for(int i = 0; i < i_nal; i++)
	{
		VIDEO_PACKET_PTR packet_ptr;

		CMemPool::malloc_video_packet(packet_ptr);

		packet_ptr.packet->ua_id = 1;
		packet_ptr.packet->sequence = 0;
		packet_ptr.packet->timestamp = timestamp;
		packet_ptr.packet->payload_size = _nal_len[i];//净荷长度
		packet_ptr.packet->mark = ((i == i_nal - 1) ? true : false);
		memcpy(packet_ptr.packet->payload, p, _nal_len[i]);
		//检测帧类型
		//packet_ptr.packet->frame = (1 == isH264IntraFrame(const_cast<unsigned char*>(p), _nal_len[i])) ? 1 : 2;
		p += _nal_len[i];

		if(SS_NoErr != add_sample_video_packet(packet_ptr))
		{
			CMemPool::free_video_packet(packet_ptr);
		}
	}

	return 0;
}

SS_Error CUserAgent::add_sample_video_packet(VIDEO_PACKET_PTR packet)
{
	//插入队列前必须先判断否则排序报错!!!
	if(NULL == packet.packet)
		return SS_InsertMediaDataFail;

	{
		CSSLocker lock(&_sample_video_packet_list_mutex);

		if(_max_video_packet_num < _sample_video_packet_list.size())
		{
			CMemPool::free_video_packet(_sample_video_packet_list.front());
			_sample_video_packet_list.pop_front();

			TRACE("~");
		}

		_sample_video_packet_list.push_back(packet);
	}

	return SS_NoErr;
}

//取最早的视频数据包
VIDEO_PACKET_PTR CUserAgent::fetch_sample_video_packet()
{
	{
		CSSLocker lock(&_sample_video_packet_list_mutex);

		//if(false == _sample_video_packet_list.empty())
		if(10 <= _sample_video_packet_list.size())
		{
			VIDEO_PACKET_PTR packet = _sample_video_packet_list.front();//*(_video_packet_list.begin());
			_sample_video_packet_list.pop_front();

			return packet;
		}
	}

	//如果队列空，则返回一个空数据包
	VIDEO_PACKET_PTR packet;
	return packet;
}

int CUserAgent::send_audio()
{
	AUDIO_PACKET_PTR packet_ptr = fetch_sample_audio_packet();

	if(NULL == packet_ptr.packet || !(packet_ptr.packet->payload_size))
	{
		CMemPool::free_audio_packet(packet_ptr);
		return -1;
	}

	if(0xffff == packet_ptr.packet->sequence)
	{
		_rtmp_session.send_aac_spec(1, 44100, 1);
	}

	int ret = -1;
	{
		CSSLocker lock(&_rtmp_send_mutex);

		ret = _rtmp_session.input_audio_packet(packet_ptr.packet->payload, packet_ptr.packet->payload_size, packet_ptr.packet->timestamp);
		TRACE("\nA<%d %d> ", packet_ptr.packet->payload_size, ret);//CRTMPSession::get_packet_type(p, _nal_len[i], seprator_len));
	}

	CMemPool::free_audio_packet(packet_ptr);

	return ret;
}

int CUserAgent::send_video()
{
	VIDEO_PACKET_PTR packet_ptr = fetch_sample_video_packet();

	if(NULL == packet_ptr.packet || !packet_ptr.packet->payload_size)
	{
		CMemPool::free_video_packet(packet_ptr);
		return -1;
	}

	int ret = -1;
	{
		CSSLocker lock(&_rtmp_send_mutex);

		ret = _rtmp_session.input_video_packet(packet_ptr.packet->payload, packet_ptr.packet->payload_size, packet_ptr.packet->timestamp);
		//TRACE("V<%d %d> ", packet_ptr.packet->payload_size, ret);//CRTMPSession::get_packet_type(p, _nal_len[i], seprator_len));
	}

	CMemPool::free_video_packet(packet_ptr);

	return ret;
}

