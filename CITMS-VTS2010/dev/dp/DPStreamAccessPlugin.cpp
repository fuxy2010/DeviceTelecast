#include "DPStreamAccessPlugin.h"
#include "ConfigBox.h"

using namespace ScheduleServer;

// ----------------------------------------------------------------------
// 函数名: CDPStreamAccessPlugin::CDPStreamAccessPlugin
// 作者: fuym
// 描述: 构造函数
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
CDPStreamAccessPlugin::CDPStreamAccessPlugin(void) :
    _dpconnect_ptr(NULL)
{
}

// ----------------------------------------------------------------------
// 函数名: CDPStreamAccessPlugin::~CDPStreamAccessPlugin
// 作者: fuym
// 描述: 析构函数
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
CDPStreamAccessPlugin::~CDPStreamAccessPlugin(void)
{
    if (_dpconnect_ptr)
    {
        delete _dpconnect_ptr;
        _dpconnect_ptr = NULL;
    }
}

// ----------------------------------------------------------------------
// 函数名: CDPStreamAccessPlugin::config
// 作者: fuym
// 描述: 插件配置
// 返回值: 
//   [void] 
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
void CDPStreamAccessPlugin::config(void)
{
	// 加载插件配置文件
    //CConfigBox config_box;
    //config_box.load(cfg_file);

    // 获取插件配置信息
    _dpplugin_config.real_type = Plugin::DPNVR::default_real_type;//config_box.get_property_as_int("real.type", Plugin::DPNVR::default_real_type);
    _dpplugin_config.wait_time = Plugin::DPNVR::default_wait_time;//config_box.get_property_as_int("wait.time", Plugin::DPNVR::default_wait_time);
    if (_dpconnect_ptr)
    {
        _dpconnect_ptr->config(_dpplugin_config);
    }
}

// ----------------------------------------------------------------------
// 函数名: CDPStreamAccessPlugin::init
// 作者: fuym
// 描述: 插件模块初始化
// 返回值: 
//   [void] 
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
void CDPStreamAccessPlugin::init(void)
{
	// 插件信息定义
	//get_module_info().name = Plugin::DPNVR::module_name;
	//get_module_info().version = Plugin::DPNVR::module_version;

    config();
}

// ----------------------------------------------------------------------
// 函数名: CDPStreamAccessPlugin::uninit
// 作者: fuym
// 描述: 插件模块卸载
// 返回值: 
//   [void] 
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
void CDPStreamAccessPlugin::uninit(void)
{
    //cout << "Enter CDPStreamAccessPlugin::uninit()" << endl;

    // 清空数据包回调对象指针
    //set_packet_callback_ptr(NULL);

    if (_dpconnect_ptr)
    {
        _dpconnect_ptr->uninit();
    }

    //cout << "Leave CDPStreamAccessPlugin::uninit()" << endl;
}

// ----------------------------------------------------------------------
// 函数名: CDPStreamAccessPlugin::get_device_type
// 作者: fuym
// 描述: 输出设备类型
// 返回值: 
//   [DEVICE_TYPE] 
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
DEVICE_TYPE CDPStreamAccessPlugin::get_device_type(void)
{
    return DHCAMERATYPE;
}

// ----------------------------------------------------------------------
// 函数名: CDPStreamAccessPlugin::access_stream
// 作者: fuym
// 描述: 媒体接入处理
// 返回值: 
//   [RETCODE] 
// 参数: 
//   [const REQUEST_INFO& request_info] 输入媒体接入请求信息
//   [const CHAR * in_data_ptr] 输入数据
//   [ULONG in_data_length] 输入数据长度
//   [CHAR * out_data_ptr] 输出数据
//   [ULONG& out_data_length] 输出数据长度
// ----------------------------------------------------------------------
RETCODE CDPStreamAccessPlugin::access_stream(const REQUEST_INFO& request_info, 
                      const CHAR * in_data_ptr,
                      ULONG in_data_length,
                      CHAR * out_data_ptr,
                      ULONG& out_data_length)
{
    //if (!is_active()) return RETCODE_FAIL;

    if (!_dpconnect_ptr)
    {
        _dpconnect_ptr = new CDPVideoServerConnect();//(IPacketCallbackPtr)this);
        _dpconnect_ptr->config(_dpplugin_config);
    }

    if (REAL_START == request_info.operation) // 实时视频请求连接
    {
        return _dpconnect_ptr->connect(request_info);
    }
    else if (REAL_STOP == request_info.operation) // 断开实时视频连接
    {
        return _dpconnect_ptr->disconnect(request_info);
    }
    else if (REAL_FASTUPDATE == request_info.operation) // 强制I帧请求
    {
        return _dpconnect_ptr->fast_update(request_info);
    }
    else
    {
        // Do nothing.
    }

    return RETCODE_REQUEST_NOT_IMPLEMENT;
}
