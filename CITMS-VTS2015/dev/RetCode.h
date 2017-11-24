#ifndef _RETCODE_H_         
#define _RETCODE_H_ 

#include "Config.h"

namespace ScheduleServer
{
    // 流媒体服务返回值类型定义
    typedef enum
    {
        RETCODE_OK = 0,                                             // 服务请求成功
        RETCODE_FAIL,                                               // 服务请求失败
        RETCODE_EXCEPTION,                                          // 异常错误
        RETCODE_MEMORY_FAIL,                                        // 内存分配失败

        RETCODE_REQUEST_PARAMETERS_ERROR,                           // 服务请求参数错误
        RETCODE_REQUEST_SDK_FAIL,                                   // 服务调用SDK失败
        RETCODE_REQUEST_NOT_IMPLEMENT,                              // 服务功能未实现    
        RETCODE_REQUEST_NOT_SUPPORT,                                // 服务功能不支持

        RETCODE_MODULE_LOAD_FAIL,                                   // 插件加载失败
        RETCODE_MODULE_UNLOAD_FAIL,                                 // 插件卸载失败
        RETCODE_MODULE_NOT_FOUND,                                   // 未找到插件
        RETCODE_MODULE_NOT_IDLE,                                    // 插件不处于空闲状态
        RETCODE_MODULE_ALREADY_LOAD,                                // 插件已经加载
        RETCODE_DEVICE_NOT_SUPPORT,                                 // 设备类型不支持

        RETCODE_ACCESS_CHECKIN,                                     // 通道接入
        RETCODE_ACCESS_CHECKOUT,                                    // 通道断开接入
        RETCODE_ACCESS_FAIL,                                        // 通道接入失败
        RETCODE_ACCESS_MAX,                                         // 通道接入数已达到最大值

        RETCODE_ACCESS_LOGIN_PASSWORD_ERROR,                        // 设备注册密码错误
        RETCODE_ACCESS_LOGIN_REGIST_FAIL,                           // 设备注册失败
        RETCODE_ACCESS_NETWORK_FAIL_CONNECT,                        // 连接服务器失败

        RETCODE_ACCESS_RECORDFILE_ERROR,                            // 录像文件错误

        RETCODE_RTSPSERVER_MODULE_NOT_FOUND,                        // 未找到RTSP协议栈插件       
        RETCODE_RTSPSERVER_ERROR,                                   // RTSP协议栈功能调用失败       

        RETCODE_COMMUNICATOR_ERROR,                                 // ICE通信器错误
        RETCODE_DBPROXY_ERROR,                                      // 数据库代理错误
        RETCODE_REGPROXY_ERROR,                                     // 平台注册代理错误    
        RETCODE_MONITORPROXY_ERROR,                                 // 前端服务器代理错误    
        RETCODE_ALARMPROXY_ERROR,                                   // 报警服务器代理错误    

        RETCODE_DBPROXY_QUERY_FAIL,                                 // 数据库查询失败

        RETCODE_SERVER_NOT_REGIST,                                  // 流媒体服务器未注册    

        RETCODE_UNKNOWN_ERROR                                       // 未知错误    
    } RETCODE;

    typedef struct 
    {
        RETCODE ret_code;
        STRING message;
    } RETINFO;

    const STRING DEFAULT_ERROR_MESSAGE = "未知错误";

    const RETINFO global_retinfo_list[] = {
        {RETCODE_OK,                                                "服务请求成功"},
        {RETCODE_FAIL,                                              "服务请求失败"},
        {RETCODE_EXCEPTION,                                         "异常错误"},
        {RETCODE_MEMORY_FAIL,                                       "内存分配失败"},

        {RETCODE_REQUEST_PARAMETERS_ERROR,                          "服务请求参数错误"},
        {RETCODE_REQUEST_SDK_FAIL,                                  "服务调用SDK失败"},
        {RETCODE_REQUEST_NOT_IMPLEMENT,                             "服务功能未实现"},    
        {RETCODE_REQUEST_NOT_SUPPORT,                               "服务功能不支持"},

        {RETCODE_MODULE_LOAD_FAIL,                                  "插件加载失败"},
        {RETCODE_MODULE_UNLOAD_FAIL,                                "插件卸载失败"},
        {RETCODE_MODULE_NOT_FOUND,                                  "未找到插件"},
        {RETCODE_MODULE_NOT_IDLE,                                   "插件不处于空闲状态"},
        {RETCODE_MODULE_ALREADY_LOAD,                               "插件已经加载"},
        {RETCODE_DEVICE_NOT_SUPPORT,                                "设备类型不支持"},

        {RETCODE_ACCESS_CHECKIN,                                    "通道接入"},
        {RETCODE_ACCESS_CHECKOUT,                                   "通道断开接入"},
        {RETCODE_ACCESS_FAIL,                                       "通道接入失败"},
        {RETCODE_ACCESS_MAX,                                        "通道接入数已达到最大值"},

        {RETCODE_ACCESS_LOGIN_PASSWORD_ERROR,                       "设备注册密码错误"},
        {RETCODE_ACCESS_LOGIN_REGIST_FAIL,                          "设备注册失败"},
        {RETCODE_ACCESS_NETWORK_FAIL_CONNECT,                       "连接服务器失败"},

        {RETCODE_ACCESS_LOGIN_PASSWORD_ERROR,                       "设备注册密码错误"},
        {RETCODE_ACCESS_LOGIN_REGIST_FAIL,                          "设备注册失败"},

        {RETCODE_ACCESS_RECORDFILE_ERROR,                           "录像文件错误"},

        {RETCODE_RTSPSERVER_MODULE_NOT_FOUND,                       "未找到RTSP协议栈插件"},       
        {RETCODE_RTSPSERVER_ERROR,                                  "RTSP协议栈功能调用失败"},       

        {RETCODE_COMMUNICATOR_ERROR,                                "ICE通信器错误"},
        {RETCODE_DBPROXY_ERROR,                                     "数据库代理错误"},
        {RETCODE_REGPROXY_ERROR,                                    "平台注册代理错误"},    
        {RETCODE_MONITORPROXY_ERROR,                                "前端服务器代理错误"},
        {RETCODE_ALARMPROXY_ERROR,                                  "报警服务器代理错误"},

        {RETCODE_DBPROXY_QUERY_FAIL,                                "数据库查询失败"},

        {RETCODE_SERVER_NOT_REGIST,                                 "流媒体服务器未注册"},

        {RETCODE_UNKNOWN_ERROR,                                     "未知错误"}
    };
}

#endif  // _RETCODE_H_      
