#ifndef _DPSTREAMACCESSPLUGIN_H_        
#define _DPSTREAMACCESSPLUGIN_H_ 

#pragma once

//输出DLL
#define DLL_EXPORTS

#include "StdAfx.h"
#include "DPVideoServerConnect.h"

namespace ScheduleServer
{
	namespace Plugin
    {
        namespace DPNVR
        {
            const string module_name = "PLUGIN.DH"; // 插件模块名称
            const string module_version = "1.0";    // 插件版本号
        }
    };

    class CDPStreamAccessPlugin
    {
    public:
        CDPStreamAccessPlugin(void);
        ~CDPStreamAccessPlugin(void);

        // 插件配置
        virtual void config(void);

        // 插件模块初始化
        virtual void init(void);

        // 插件模块卸载
        virtual void uninit(void);

        // 输出设备类型
        virtual DEVICE_TYPE get_device_type(void);

        // 媒体流接入
        virtual RETCODE access_stream(const REQUEST_INFO& request_info, 
            const CHAR * in_data_ptr,
            ULONG in_data_length,
            CHAR * out_data_ptr,
            ULONG& out_data_length);

    private:
        STRING _config_file;                        // 插件配置文件
        DPPLUGIN_CONFIG _dpplugin_config;           // 大华插件配置信息
        CDPVideoServerConnect* _dpconnect_ptr;      // 大华设备接入功能对象指针
    };
}

#endif  // _DHSTREAMACCESSPLUGIN_H_         
