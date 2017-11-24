#ifndef _SHARED_DATA_H_
#define _SHARED_DATA_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "Packet.h"

#define SHARE_BUFFER_SIZE 10000

namespace ScheduleServer
{
    // 数据包缓冲区共享内存结构定义
    typedef struct  
    {
        INT read_item;                                      // 读取数据包位置
        INT write_item;                                     // 写入数据包位置    
        PACKET_DATA packet_data_list[SHARE_BUFFER_SIZE];    // 数据包缓冲区
    } SHARED_DATA;
}

#endif // _SHARED_DATA_H_