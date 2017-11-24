#ifndef _PACKET_H_      
#define _PACKET_H_ 

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "Config.h"

// 数据包识别码字节数定义
#define INDENTITY_SIZE 4

// 媒体数据包最大载荷大小定义
#define MAX_PAYLOAD_SIZE 20000                            

namespace ScheduleServer
{
    const CHAR IDENTITY[INDENTITY_SIZE] = {'s', 'Y', 'n', 'C'};     // 数据包识别码定义
    
    // 媒体数据包描述信息结构定义
    typedef struct  
    {
        CHAR identity[INDENTITY_SIZE];                              // 数据包识别码(4字节)
        USHORT payload_block_size;                                  // 净荷数据包大小(2字节，最大65535)
        USHORT source_uuid;                                         // 媒体源标识(2字节，最大65535)
        USHORT payload_origin_size;                                 // 净荷数据包原始数据块大小(2字节，最大65535)
        BYTE payload_block_id: 4;                                   // 拆分数据包编号(最大16)
        BYTE payload_block_count: 4;                                // 拆分数据包数目(最大16)   
        BYTE data_type: 6;                                          // 数据类型(最大64) 
        BYTE iframe: 1;                                             // I帧标志(0/1) 
        BYTE mark: 1;                                               // 数据标记(0/1, 预留某些设备SDK使用)
        UINT sequence;                                              // 数据包序列号(4字节)
        UINT64 time_stamp;                                          // 时戳(毫秒级)(8字节)
    } PACKET_INFO, *PACKET_INFO_PTR;

    // 媒体数据包结构定义
    typedef struct 
    {
        PACKET_INFO info;                                           // 描述信息
        CHAR payload[MAX_PAYLOAD_SIZE];                             // 净负荷            
    } PACKET_DATA, *PACKET_DATA_PTR;

    // 数据包队列类型定义
    typedef std::list<PACKET_DATA> PACKET_DATA_LIST;
    typedef std::deque<PACKET_DATA_PTR> PACKET_DATA_PTR_QUEUE;

    // 初始化数据包描述信息
    inline void init_packet_info(PACKET_INFO& packet_info)
    {
        ::memset(&packet_info, 0, sizeof(PACKET_INFO));

        packet_info.identity[0] = IDENTITY[0];
        packet_info.identity[1] = IDENTITY[1];
        packet_info.identity[2] = IDENTITY[2];
        packet_info.identity[3] = IDENTITY[3];

        packet_info.source_uuid = (USHORT)INVALID_SOURCE_UUID;
        packet_info.payload_block_count = 1;
    };

    // 判断数据包描述信息格式是否正确
    inline BOOL is_packet_legal(const PACKET_INFO& packet_info)
    {
        // 比较数据包标识
        for (INT i=0; i<INDENTITY_SIZE; i++)
        {
            if (packet_info.identity[i] != IDENTITY[i]) return FALSE;
        }

        // 检测数据包净荷值
        if (packet_info.payload_block_size > MAX_PAYLOAD_SIZE) return FALSE;

        return TRUE;
    }
}

#endif  // _PACKET_H_       
