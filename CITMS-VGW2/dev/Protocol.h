#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

namespace ScheduleServer
{
    namespace Protocol
    {
        // 统一数据接口通用键值定义
        const std::string KEY_ID_COMMAND_TYPE = "command_type";
        const std::string KEY_ID_SUCCESS = "success";
        const std::string KEY_ID_ERROR_CODE = "error_code";
        const std::string KEY_ID_ERROR_MESSAGE = "error_message";

        // 请求媒体接入协议键值定义
        const std::string KEY_ID_CLIENT_UUID = "client_uuid";
        const std::string KEY_ID_CLIENT_IP = "client_ip";
        const std::string KEY_ID_CLIENT_MAC = "client_mac";
        const std::string KEY_ID_CLIENT_PORT = "client_port";
        const std::string KEY_ID_CLIENT_PROXY = "client_proxy";
        const std::string KEY_ID_DEVICE_UUID = "devce_uuid";
        const std::string KEY_ID_DEVICE_TYPE = "devce_type";
        const std::string KEY_ID_DEVICE_VERSION = "devce_version";
        const std::string KEY_ID_DEVICE_IP = "devce_ip";
        const std::string KEY_ID_DEVICE_MAC = "devce_mac";
        const std::string KEY_ID_DEVICE_PORT = "devce_port";
        const std::string KEY_ID_DEVICE_CHANNELSTART = "devce_channelstart";
        const std::string KEY_ID_DEVICE_CHANNELCOUNT = "devce_channelcount";
        const std::string KEY_ID_DEVICE_USER = "devce_user";
        const std::string KEY_ID_DEVICE_PASSWORD = "devce_password";
        const std::string KEY_ID_DEVICE_RECONNECTFLAG = "devce_reconnectflag";
        const std::string KEY_ID_CHANNEL = "channel";
        const std::string KEY_ID_TRANSPORT = "transport";

        // 数据包回调协议键值定义
        const std::string KEY_ID_PACKET_DEVICE_UUID = "packet_device_uuid";
        const std::string KEY_ID_PACKET_DEVICE_TYPE = "packet_device_type";
        const std::string KEY_ID_PACKET_CHANNEL = "packet_channel";
        const std::string KEY_ID_PACKET_DATA_TYPE = "packet_data_type";
        const std::string KEY_ID_PACKET_TIMESTAMP = "packet_timestamp";
        const std::string KEY_ID_PACKET_MARK = "packet_mark";
        const std::string ITEM_ID_TRUE = "1";
        const std::string ITEM_ID_FALSE = "0";

        const std::string ITEM_ID_OK = "1";
        const std::string ITEM_ID_ERROR = "0";

        const std::string ITEM_ID_COMMAND_TYPE_HEARTBEAT = "HeartBeat";
        const std::string ITEM_ID_COMMAND_TYPE_CALLBACK = "RequestCallback";
        const std::string ITEM_ID_COMMAND_TYPE_SHUTDOWN = "ShutDown";

        const std::string ITEM_ID_COMMAND_TYPE_GETSYSPARAMETERS = "GetSysParameters";
        const std::string ITEM_ID_COMMAND_TYPE_SETSYSPARAMETERS = "SetSysParameters";
        const std::string ITEM_ID_COMMAND_TYPE_GETSTREAMACCESSLIST = "GetStreamAccessList";
        const std::string ITEM_ID_COMMAND_TYPE_GETMODULELIST = "GetModuleList";
        const std::string ITEM_ID_COMMAND_TYPE_VIDEOCONNECT = "VideoConnect";
        const std::string ITEM_ID_COMMAND_TYPE_VIDEODISCONNECT = "VideoDisconnect";
        const std::string ITEM_ID_COMMAND_TYPE_FASTUPDATE = "FastUpdate";
        const std::string ITEM_ID_COMMAND_TYPE_SNAPSHOT = "SnapShot";
        const std::string ITEM_ID_COMMAND_TYPE_STARTVOICECOM = "StartVoiceCom";
        const std::string ITEM_ID_COMMAND_TYPE_STOPVOICECOM = "StopVoiceCom";
        const std::string ITEM_ID_COMMAND_TYPE_SENDVOICEDATA = "SendVoiceData";
        const std::string ITEM_ID_COMMAND_TYPE_STARTRECORDREVIEW = "StartRecordReview";
        const std::string ITEM_ID_COMMAND_TYPE_STOPRECORDREVIEW = "StopRecordReview";
        const std::string ITEM_ID_COMMAND_TYPE_STARTRECORDDOWNLOAD = "StartRecordDownload";
        const std::string ITEM_ID_COMMAND_TYPE_STOPRECORDDOWNLOAD = "StopRecordDownload";

        const std::string ITEM_ID_TRANSPORT_UDP = "udp";            // UDP方式传输数据包  
        const std::string ITEM_ID_TRANSPORT_TCP = "tcp";            // TCP方式传输数据包
        const std::string ITEM_ID_TRANSPORT_RTP = "rtp";            // RTP方式传输数据包
        const std::string ITEM_ID_TRANSPORT_UDI = "udi";            // 统一数据接口方式传输数据包
        const std::string ITEM_ID_TRANSPORT_SHARE = "share";        // 共享内存方式传输数据包
        const std::string ITEM_ID_TRANSPORT_STORAGE = "storage";    // 存储数据包
    };
}

#endif // _PROTOCOL_H_