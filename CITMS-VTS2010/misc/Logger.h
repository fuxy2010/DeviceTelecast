#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <list>
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <time.h>

#include "Locker.h"

// 缺省日志类型
#define DEFAULT_TRACE_TYPE  0

// 缺省日志输出等级
#define DEFAULT_TRACE_LEVEL 0

// 缺省日志目录
#define DEFAULT_LOG_DIR     ".//"

// 缺省日志信息前缀
#define DEFAULT_LOG_PREFIX  ""

typedef enum
{
    RELEASE_LEVEL = 0,      // 记录程序运行信息 
    DEBUG_LEVEL_1 = 1,      // 记录调试信息(服务接口、参数和返回值)
    DEBUG_LEVEL_2 = 2,      // 记录调试信息(功能函数、参数和返回值)
    DEBUG_LEVEL_3 = 3,      // 记录调试信息(数据库操作和SQL语句)
    DEBUG_LEVEL_4 = 4       // 记录调试信息(所有信息)
} TRACE_LEVEL;

//
// 日志记录类
//
class CLogger;
typedef std::map<int, std::string> LOGFILE_BY_TYPE;

class CLogger
{
public:
    CLogger();
    virtual ~CLogger();
    
    // 打开单个日志文件
    void open(const std::string& prefix, 
              const std::string& filename, 
              const std::string& dir = DEFAULT_LOG_DIR,
              int trace_type = DEFAULT_TRACE_TYPE);

    // 打开多个日志文件
    void open(const std::string& prefix, 
              const LOGFILE_BY_TYPE& filename_map, 
              const std::string& dir = DEFAULT_LOG_DIR);

    // 设置是否输出日志到控制台
    void set_on_screen(bool on_screen);

    // 设置日志输出等级
    void set_trace_level(int trace_level);

    // 根据日志等级判断是否输出日志
    bool can_trace(int trace_level);

    // 输出日志信息
    void trace_out(const std::string& message, 
                   int trace_level = DEFAULT_TRACE_LEVEL, 
                   int trace_type = DEFAULT_TRACE_TYPE,
                   bool on_screen = false);

    // 显示日志信息
    void trace_onscreen(const std::string& message);

    // 获取日志目录
    std::string get_log_dir();

    //生成目录
    void create_dir(const std::string& path);

    //规范化目录格式
    std::string simplify_dir(const std::string& path);

private:
    //
    // 单一日志类
    //
    class CSingleLogger
    {
    public:
        CSingleLogger(int trace_type, const std::string& filename, const std::string& dir) : 
            _trace_type(trace_type),
            _filename(filename),
            _dir(dir)
        {
            _date = get_current_date_str();
        }

        ~CSingleLogger() 
        {
            close();
        }

        // 设置日志类型
        inline int get_trace_type() 
        {
            return _trace_type;
        }

        // 打开日志文件
        void open()
        {
            close();

            // 解决ifstream打开文件中文路径名问题
            // 设置代码页为简体中文，936是简体中文的代码页。
            std::locale loc1 = std::locale::global(std::locale(".936"));

            std::string filepath = _dir + "/" + _date + " " + _filename;
            _ofs.open(filepath.c_str(), std::ios_base::out | std::ios_base::app);

            //恢复原来的代码页
            std::locale::global(std::locale(loc1));
        };

        // 输出日志信息
        void trace_out(const std::string& message)
        {
            CSSLocker lock(&_log_mutex);

            // 获取当前日期信息
            std::string date = get_current_date_str();

            // 如果日期变化，关闭原日志文件, 并打开新的日志文件
            if (date != _date)
            {
                close();

                _date = date;
                open();
            }

            _ofs << message << std::endl;
        };

        // 关闭日志文件
        inline void close()
        {
            if (_ofs.is_open())
            {
                _ofs.close();
            }
        };

    private:
        // 获取当前日期字符串，输出格式(YYYY-MM-DD)
        std::string get_current_date_str()
        {
            time_t timep;
            time(&timep);
            struct tm t;

#ifdef _WIN32
            localtime_s(&t, &timep);
#else
            struct tm tr;
            localtime_r(&time, &tr);
            t = tr;
#endif
            char buf[32] = {0};
            strftime(buf, sizeof(buf), "%Y-%m-%d", &t);

            return std::string(buf);
        }

    private:
        int _trace_type;        // 日志输出类型
        std::string _filename;  // 日志文件名
        std::string _dir;       // 日志文件存储目录
        std::string _date;      // 日志开始记录日期
        std::ofstream _ofs;     // 日志文件流
        CSSMutex _log_mutex;      // 互斥量
    };

	// 获取当前日期时间字符串
	std::string get_current_datetime_str();

private:
    int _trace_level;                               // 日志输出等级
    std::string _dir;                               // 日志文件目录
    std::string _prefix;                            // 日志信息前缀
    std::map<int, CSingleLogger*> _logger_ptr_map;  // 日志操作类列表
    CSSMutex _screen_mutex;                           // 互斥量
};

#endif // _LOGGER_H_
