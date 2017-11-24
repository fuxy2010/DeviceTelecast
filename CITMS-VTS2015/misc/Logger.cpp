#include "Logger.h"
#include <errno.h>
#include <sys\timeb.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif

using namespace std;

// ----------------------------------------------------------------------
// 函数名: CLogger::CLogger
// 作者: fuym
// 描述: CLogger类的构造函数，初始化相关成员
// 返回值: 无
// 参数: 
//   [void]  
// ----------------------------------------------------------------------
CLogger::CLogger() :
    //_on_screen(false),
    _trace_level(DEFAULT_TRACE_LEVEL),
    _dir(DEFAULT_LOG_DIR),
    _prefix(DEFAULT_LOG_PREFIX)
{
    _logger_ptr_map.clear();
}

// ----------------------------------------------------------------------
// 函数名: CLogger::~CLogger
// 作者: fuym
// 描述: CLogger类的析构函数，关闭所有日志文件
// 返回值: 无
// 参数: 
//   [void]  
// ----------------------------------------------------------------------
CLogger::~CLogger()
{
    // 关闭所有日志文件
    std::map<int, CSingleLogger*>::iterator iter = _logger_ptr_map.begin();
    while (iter != _logger_ptr_map.end())
    {
        delete iter->second;
        ++iter;
    }
}

// ----------------------------------------------------------------------
// 函数名: CLogger::get_current_datetime_str
// 作者: wangq
// 描述: 获取当前日期时间，输出格式(YYYY-MM-DD HH:mm:ss.xxx)
// 返回值: [std::string] 当前日期字符串
// 参数: 
//   [void]  
// ----------------------------------------------------------------------
std::string CLogger::get_current_datetime_str()
{
    struct _timeb tb;
    _ftime64_s(&tb);

    time_t time = tb.time;

    struct tm t;
#ifdef _WIN32
    localtime_s(&t, &time);
#else
    struct tm tr;
    localtime_r(&time, &tr);
    t = tr;
#endif

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);

    std::ostringstream os;
    os << buf << ".";
    os.fill('0');
    os.width(3);
    os << tb.millitm;
    return os.str();
}

// ----------------------------------------------------------------------
// 函数名: CLogger::open
// 作者: fuym
// 描述: 打开单个日志文件
// 返回值: [void]
// 参数: 
//   [const std::string& prefix] 输入日志信息前缀
//   [const std::string& filename] 输入日志文件名
//   [const std::string& dir] 输入日志文件目录
//   [int trace_type] 输入日志类型
// ----------------------------------------------------------------------
void CLogger::open(const std::string& prefix, const std::string& filename, const std::string& dir, int trace_type)
{
    // 保存日志信息前缀
    _prefix = prefix;

    // 保存并格式化日志目录信息
	if (dir.empty())
    {
        _dir = DEFAULT_LOG_DIR;
    }
	else 
    {
        _dir = simplify_dir(dir);
    }
	create_dir(_dir);

    // 创建单一日志处理对象，打开日志文件
    CSingleLogger* logger_ptr = new CSingleLogger(trace_type, filename, dir);
    logger_ptr->open();

    // 保持日志类型和日志文件信息
    _logger_ptr_map[trace_type] = logger_ptr;
}

// ----------------------------------------------------------------------
// 函数名: CLogger::open
// 作者: fuym
// 描述: 打开多个日志文件
// 返回值: [void]
// 参数: 
//   [const std::string& prefix] 输入日志信息前缀
//   [const std::map<int, std::string>& filename_map] 输入日志文件列表
//   [const std::string& dir] 输入日志文件目录
// ----------------------------------------------------------------------
void CLogger::open(const std::string& prefix, const std::map<int, std::string>& filename_map, const std::string& dir)
{
    // 保存日志信息前缀
    _prefix = prefix;

    // 保存并格式化日志目录信息
	if (dir.empty())
    {
        _dir = DEFAULT_LOG_DIR;
    }
	else 
    {
        _dir = simplify_dir(dir);
    }
	create_dir(_dir);

    // 日志文件列表循环处理
    std::map<int, std::string>::const_iterator iter = filename_map.begin();
    while (iter != filename_map.end())
    {
        // 创建单一日志处理对象，打开日志文件
        CSingleLogger* logger_ptr = new CSingleLogger(iter->first, iter->second, _dir);
        logger_ptr->open();

        // 保持日志类型和日志文件信息
        _logger_ptr_map[iter->first] = logger_ptr;

        ++iter;
    }
}

// ----------------------------------------------------------------------
// 函数名: CLogger::set_trace_level
// 作者: fuym
// 描述: 设置日志输出等级
// 返回值: [void]
// 参数: 
//   [int trace_level] 输入日志输出等级
// ----------------------------------------------------------------------
void CLogger::set_trace_level(int trace_level)
{
    _trace_level = trace_level;
}

// ----------------------------------------------------------------------
// 函数名: CLogger::can_trace
// 作者: fuym
// 描述: 根据日志输出等级判断是否将信息写入日志文件
// 返回值: [bool] true 写入日志文件 false 不写入日志文件
// 参数: 
//   [int trace_level] 输入日志输出等级
// ----------------------------------------------------------------------
bool CLogger::can_trace(int trace_level)
{
    return (trace_level <= _trace_level);
}

// ----------------------------------------------------------------------
// 函数名: CLogger::trace_out
// 作者: fuym
// 描述: 根据日志输出等级和日志类型输出日志信息，并按日期切分日志文件
// 返回值: [void]
// 参数: 
//   [const std::string& message] 输入日志信息
//   [int trace_level] 输入日志输出等级
//   [int trace_type] 输入日志类型
//   [bool on_screen] 输入是否在屏幕显示日志信息
// ----------------------------------------------------------------------
void CLogger::trace_out(const std::string& message, int trace_level, int trace_type, bool on_screen)
{
    // 判断是否将信息写入日志文件
    if (!can_trace(trace_level)) return;

    // 在日志信息前面附加记录时间和前缀
    std::ostringstream ss;
    ss << message << " " << _prefix << ": " << get_current_datetime_str() << endl;

    // 根据日志类型查找对应的日志信息输出对象
    CSingleLogger* pSingleLogger = _logger_ptr_map[trace_type];
    if (pSingleLogger != NULL)
    {
        // 记录日志信息
        pSingleLogger->trace_out(ss.str());
    }
    
    // 在屏幕输出日志信息
    if (true == on_screen)  
    {
        trace_onscreen(ss.str());
    }
}

// ----------------------------------------------------------------------
// 函数名: CLogger::trace_onscreen
// 作者: fuym
// 描述: 显示日志信息
// 返回值: [void]
// 参数: 
//   [const std::string& message] 输入日志信息
// ----------------------------------------------------------------------
void CLogger::trace_onscreen(const std::string& message)
{
    CSSLocker lock(&_screen_mutex);
    cout << message << endl;
}

// ----------------------------------------------------------------------
// 函数名: CLogger::get_log_dir
// 作者: fuym
// 描述: 获取日志目录
// 返回值: [std::string] 输出日志目录
// 参数: 
//   [void]
// ----------------------------------------------------------------------
std::string CLogger::get_log_dir()
{
    return _dir;
}

// ----------------------------------------------------------------------
// 函数名: CLogger::create_dir
// 作者: wangq
// 描述: 生成目录
// 返回值: [void]
// 参数: 
//   [const std::string& path] 目录路径
// ----------------------------------------------------------------------
void CLogger::create_dir(const std::string& path)
{
    int res = -1;
#ifdef _WIN32
    res = ::_mkdir(path.c_str());
#else
    res = ::mkdir(path.c_str(), 0x777);
#endif

    if(res == -1)
    {
        if(errno != EEXIST)
        {
            throw "cannot create directory " + path;
        }
    }
}

// ----------------------------------------------------------------------
// 函数名: CLogger::simplify_dir
// 作者: wangq
// 描述: 标准化目录字符串
// 返回值: [std::string] 转化后的目录路径
// 参数: 
//   [const std::string& path] 目录路径字符串
// ----------------------------------------------------------------------
std::string CLogger::simplify_dir(const std::string& path)
{
    string result = path;

    string::size_type pos;

#ifdef _WIN32
    for(pos = 0; pos < result.size(); ++pos)
    {
        if(result[pos] == '\\')
        {
            result[pos] = '/';
        }
    }
#endif

    pos = 0;
    while((pos = result.find("//", pos)) != string::npos)
    {
        result.erase(pos, 1);
    }

    pos = 0;
    while((pos = result.find("/./", pos)) != string::npos)
    {
        result.erase(pos, 2);
    }

    if(result.substr(0, 2) == "./")
    {
        result.erase(0, 2);
    }

    if(result == "/." ||
        result.size() == 4 && isalpha(result[0]) && result[1] == ':' && result[2] == '/' && result[3] == '.')
    {
        return result.substr(0, result.size() - 1);
    }

    if(result.size() >= 2 && result.substr(result.size() - 2, 2) == "/.")
    {
        result.erase(result.size() - 2, 2);
    }

    if(result == "/" || result.size() == 3 && isalpha(result[0]) && result[1] == ':' && result[2] == '/')
    {
        return result;
    }

    if(result.size() >= 1 && result[result.size() - 1] == '/')
    {
        result.erase(result.size() - 1);
    }

    return result;
}

