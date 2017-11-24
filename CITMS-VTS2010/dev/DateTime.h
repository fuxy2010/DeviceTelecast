#ifndef _DATETIME_H_        
#define _DATETIME_H_ 

#include "Config.h"
#include <time.h>
#include <sys\timeb.h>

namespace ScheduleServer
{
    // 日期时间信息结构定义
    typedef struct _DATETIME_INFO
    {
        UINT year;                                                   // 年(1970-2100)
        UINT month;                                                  // 月(1-12)
        UINT day;                                                    // 日(1-31)
        UINT hour;                                                   // 时(0-23)
        UINT minute;                                                 // 分(0-59)
        UINT second;                                                 // 秒(0-59)
    } DATETIME_INFO;

    namespace DateTime
    {
        // 延时
        void sleep(ULONG millisec);

        // 构造时间信息
        INT64 make_time(UINT year, UINT month, UINT day, UINT hour = 0, UINT minute = 0, UINT second = 0);

        // 转换时间值(从1970年1月1日0时0分0秒到此时的秒数)为日期时间信息
        DATETIME_INFO convert_date(INT64 time);

        // 转换日期时间信息为时间值(从1970年1月1日0时0分0秒到此时的秒数)
        INT64 convert_time(const DATETIME_INFO& datetime_info);

        // 获取当前时间值(从1970年1月1日0时0分0秒到此时的秒数)
        INT64 get_current_time(void);

        // 获取当前时间值(从1970年1月1日0时0分0秒到此时的毫秒数)
        INT64 get_current_timestamp(void);
    }
}

using namespace ScheduleServer;

// ----------------------------------------------------------------------
// 函数名: DateTime::sleep
// 作者: fuym
// 描述: 按毫秒延时
// 返回值: [void]
// 参数: 
//   [INT millisec] 毫秒值
// ----------------------------------------------------------------------
inline void DateTime::sleep(ULONG millisec)
{
#ifdef _WIN32
    ::Sleep(millisec);
#else
    IceUtil::ThreadControl::sleep(IceUtil::Time::milliSeconds(millisec));
#endif
}

// ----------------------------------------------------------------------
// 函数名: DateTime::make_time
// 作者: fuym
// 描述: 构造时间信息
// 返回值: [LONG] 输出时间值(从1970年1月1日0时0分0秒到此时的秒数)
// 参数: 
//   [INT year] 年
//   [INT month] 月
//   [INT day] 日
//   [INT hour] 时
//   [INT minute] 分
//   [INT second] 秒
// ----------------------------------------------------------------------
inline INT64 DateTime::make_time(UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second)
{
    DATETIME_INFO dt_info;
    dt_info.year = year;
    dt_info.month = month;
    dt_info.day = day;
    dt_info.hour = hour;
    dt_info.minute = minute;
    dt_info.second = second;

    return convert_time(dt_info);
}

// ----------------------------------------------------------------------
// 函数名: DateTime::convert_date
// 作者: fuym
// 描述: 转换时间值(从1970年1月1日0时0分0秒到此时的秒数)为日期时间信息
// 返回值: [DATETIME_INFO] 输出日期时间信息
// 参数: 
//   [LONG time] 输入时间值(秒)
// ----------------------------------------------------------------------
inline DATETIME_INFO DateTime::convert_date(INT64 time)
{
    DATETIME_INFO dt_info;

    time_t timep = (time_t)time;
    struct tm t;

#ifdef _WIN32
    localtime_s(&t, &timep);
#else
    struct tm tr;
    localtime_r(&time, &tr);
    t = tr;
#endif

    dt_info.year = t.tm_year + 1900;
    dt_info.month = t.tm_mon + 1;
    dt_info.day = t.tm_mday;
    dt_info.hour = t.tm_hour;
    dt_info.minute = t.tm_min;
    dt_info.second = t.tm_sec;

    return dt_info;
}

// ----------------------------------------------------------------------
// 函数名: DateTime::convert_time
// 作者: fuym
// 描述: 转换日期时间信息为时间值(从1970年1月1日0时0分0秒到此时的秒数)
// 返回值: [LONG] 输出时间值(秒)
// 参数: 
//   [const DATETIME_INFO& datetime_info] 输入日期时间信息
// ----------------------------------------------------------------------
inline INT64 DateTime::convert_time(const DATETIME_INFO& datetime_info)
{
    struct tm t; 
    t.tm_year = datetime_info.year - 1900;
    t.tm_mon = datetime_info.month - 1; 
    t.tm_mday = datetime_info.day; 
    t.tm_hour = datetime_info.hour; 
    t.tm_min = datetime_info.minute; 
    t.tm_sec = datetime_info.second; 
    t.tm_isdst = 0; 

    return ::mktime(&t);
}

// ----------------------------------------------------------------------
// 函数名: DateTime::get_current_time
// 作者: fuym
// 描述: 获取当前时间值(从1970年1月1日0时0分0秒到此时的秒数)
// 返回值: [LONG] 输出当前时间值(秒)
// 参数: 
//   [void]
// ----------------------------------------------------------------------
inline INT64 DateTime::get_current_time(void)
{
    return time(NULL);
}

// ----------------------------------------------------------------------
// 函数名: DateTime::get_current_timestamp
// 作者: fuym
// 描述: 获取当前时间
// 返回值: [INT64] 输出当前时间值(毫秒)
// 参数: 
//   [void]
// ----------------------------------------------------------------------
inline INT64 DateTime::get_current_timestamp(void)
{
    struct _timeb t;
    _ftime64_s(&t);
    return (t.time * 1000 + t.millitm);
}

#endif  // _DATETIME_H_         
