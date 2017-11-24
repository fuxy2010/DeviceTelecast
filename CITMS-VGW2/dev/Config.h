#ifndef _CONFIG_H_
#define _CONFIG_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>

// bool值定义
#define TRUE 1                                                             
#define FALSE 0

// 无效ID定义
#define INVALID_ID -1

// 无效用户ID定义
#define INVALID_USER_UUID -1

// 无效媒体源ID定义
#define INVALID_SOURCE_UUID -1

// 通用类型定义
typedef int INT;
typedef int BOOL;
typedef unsigned int UINT;
typedef char CHAR;
typedef unsigned char BYTE;
typedef unsigned char * PBYTE;
typedef short SHORT;
typedef unsigned short USHORT;
typedef long LONG;
typedef unsigned long ULONG;
typedef long long INT64;
typedef unsigned long long UINT64;
typedef std::string STRING;
typedef std::vector<BYTE> BYTES;
typedef std::vector<STRING> STRINGLIST;

namespace ScheduleServer
{
    // 服务请求参数类型定义
    typedef std::map<std::string, std::string> CONTEXT;
}

#endif // _CONFIG_H_
