#ifndef _DHVIDEOSERVERCONNECT_H
#define _DHVIDEOSERVERCONNECT_H

#include "Packet.h"
#include "TypeDef.h"
#include "RetCode.h"

#include "./dhsdk/dhnetsdk.h"

// 大华设备SDK
#pragma comment(lib, "../dev/dh/dhsdk/dhnetsdk.lib")
#pragma comment(lib, "WS2_32.lib")

#define VIDEO_PROTOCOL_MAINSTREAM	0
#define VIDEO_PROTOCOL_CHILDSTREAM  1

namespace ScheduleServer
{
    namespace Plugin
    {
        namespace DHDVR
        {
            const INT default_real_type = 0;        // 缺省实时视频码流类型    
            const INT default_wait_time = 3000;     // 缺省设备连接等待时间(3000毫秒)
        }

		const INT invalid_userid = -1;
		const INT invalid_handle = -1;
    };

    // 大华插件配置信息
    typedef struct _DHPLUGIN_CONFIG
    {
        INT real_type;  // 实时视频码流类型(默认主码流DH_RType_RealPlay)
        INT wait_time;  // 设备连接等待时间(默认3000毫秒)
        
        _DHPLUGIN_CONFIG(void) : real_type(Plugin::DHDVR::default_real_type), wait_time(Plugin::DHDVR::default_wait_time)
        {
        }
    } DHPLUGIN_CONFIG;

    // 大华通道连接信息
    typedef struct _DHCHANNEL_INFO
    {
        STREAMSOURCE_INFO source_info;          // 接入请求通道信息

        LONG login_id;                          // 设备注册ID
        LONG real_handle;                       // 视频连接句柄
        LONG voice_handle;                      // 语音对讲句柄

        PACKET_DATA packet_data;                // 媒体数据包
        UINT sequence;                          // 接受数据包序号
        
        _DHCHANNEL_INFO(void) : login_id(), 
            real_handle(Plugin::invalid_handle), voice_handle(Plugin::invalid_handle),
            sequence(0)
        {
            // 初始化数据包描述信息
            init_packet_info(packet_data.info);
        }
    } DHCHANNEL_INFO;

    //
    // 大华设备接入功能类
    //
    class CDHVideoServerConnect
    {
    public:
        CDHVideoServerConnect();//IPacketCallbackPtr packet_callback_ptr = NULL);
        virtual ~CDHVideoServerConnect(void);

        // 关闭大华设备接入
        void uninit(void);

        // 视频连接
        RETCODE connect(const REQUEST_INFO& request_info);

        // 断开视频连接
        RETCODE disconnect(const REQUEST_INFO& request_info);

        // 强制I帧
        RETCODE fast_update(const REQUEST_INFO& request_info);

        // 数据包回调
        void data_callback(DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, int bMark, DWORD source_uuid);

        // 设置配置信息
        void config(const DHPLUGIN_CONFIG& dhplugin_config)
        {
            _dhplugin_config = dhplugin_config;
        };

        // 设备断开时回调
        void device_disconnect_callback(LONG lLoginID, char *pchDVRIP, LONG nDVRPort);

        // 设备断线重连时回调
        void device_reconnect_callback(LONG lLoginID, char *pchDVRIP, LONG nDVRPort);

    private:
        // 关闭通道
        RETCODE close_channel(DHCHANNEL_INFO& dhchannel_info);

    public:
        static CDHVideoServerConnect* global_connector;

    private:
        // 大华SDK初始化成功标志
        BOOL _is_init_ok;

        // 大华插件配置信息
        DHPLUGIN_CONFIG _dhplugin_config;

        // 数据包回调对象指针
        //IPacketCallbackPtr _packet_callback_ptr;

        // 通道信息列表
        std::map<INT, DHCHANNEL_INFO> _dhchannel_info_map;

	private:
		int test();
    };
}

#endif