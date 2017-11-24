#include "DHStreamAccessPlugin.h"
#include "ConfigBox.h"

using namespace ScheduleServer;

// ----------------------------------------------------------------------
// 函数名: CDHStreamAccessPlugin::CDHStreamAccessPlugin
// 作者: fuym
// 描述: 构造函数
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
CDHStreamAccessPlugin::CDHStreamAccessPlugin(void) :
    _dhconnect_ptr(NULL)
{
}

// ----------------------------------------------------------------------
// 函数名: CDHStreamAccessPlugin::~CDHStreamAccessPlugin
// 作者: fuym
// 描述: 析构函数
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
CDHStreamAccessPlugin::~CDHStreamAccessPlugin(void)
{
    if (_dhconnect_ptr)
    {
        delete _dhconnect_ptr;
        _dhconnect_ptr = NULL;
    }
}

// ----------------------------------------------------------------------
// 函数名: CDHStreamAccessPlugin::config
// 作者: fuym
// 描述: 插件配置
// 返回值: 
//   [void] 
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
void CDHStreamAccessPlugin::config(void)
{
	// 加载插件配置文件
    //CConfigBox config_box;
    //config_box.load(cfg_file);

    // 获取插件配置信息
    _dhplugin_config.real_type = Plugin::DHDVR::default_real_type;//config_box.get_property_as_int("real.type", Plugin::DHDVR::default_real_type);
    _dhplugin_config.wait_time = Plugin::DHDVR::default_wait_time;//config_box.get_property_as_int("wait.time", Plugin::DHDVR::default_wait_time);
    if (_dhconnect_ptr)
    {
        _dhconnect_ptr->config(_dhplugin_config);
    }
}

// ----------------------------------------------------------------------
// 函数名: CDHStreamAccessPlugin::init
// 作者: fuym
// 描述: 插件模块初始化
// 返回值: 
//   [void] 
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
void CDHStreamAccessPlugin::init(void)
{
	// 插件信息定义
	//get_module_info().name = Plugin::DHDVR::module_name;
	//get_module_info().version = Plugin::DHDVR::module_version;

    config();
}

// ----------------------------------------------------------------------
// 函数名: CDHStreamAccessPlugin::uninit
// 作者: fuym
// 描述: 插件模块卸载
// 返回值: 
//   [void] 
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
void CDHStreamAccessPlugin::uninit(void)
{
    //cout << "Enter CDHStreamAccessPlugin::uninit()" << endl;

    // 清空数据包回调对象指针
    //set_packet_callback_ptr(NULL);

    if (_dhconnect_ptr)
    {
        _dhconnect_ptr->uninit();
    }

    //cout << "Leave CDHStreamAccessPlugin::uninit()" << endl;
}

// ----------------------------------------------------------------------
// 函数名: CDHStreamAccessPlugin::get_device_type
// 作者: fuym
// 描述: 输出设备类型
// 返回值: 
//   [DEVICE_TYPE] 
// 参数: 
//   [void] 
// ----------------------------------------------------------------------
DEVICE_TYPE CDHStreamAccessPlugin::get_device_type(void)
{
    return DHCAMERATYPE;
}

// ----------------------------------------------------------------------
// 函数名: CDHStreamAccessPlugin::access_stream
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
RETCODE CDHStreamAccessPlugin::access_stream(const REQUEST_INFO& request_info, 
                      const CHAR * in_data_ptr,
                      ULONG in_data_length,
                      CHAR * out_data_ptr,
                      ULONG& out_data_length)
{
    //if (!is_active()) return RETCODE_FAIL;

    if (!_dhconnect_ptr)
    {
        _dhconnect_ptr = new CDHVideoServerConnect();//(IPacketCallbackPtr)this);
        _dhconnect_ptr->config(_dhplugin_config);
    }

    if (REAL_START == request_info.operation) // 实时视频请求连接
    {
        return _dhconnect_ptr->connect(request_info);
    }
    else if (REAL_STOP == request_info.operation) // 断开实时视频连接
    {
        return _dhconnect_ptr->disconnect(request_info);
    }
    else if (REAL_FASTUPDATE == request_info.operation) // 强制I帧请求
    {
        return _dhconnect_ptr->fast_update(request_info);
    }
    else
    {
        // Do nothing.
    }

    return RETCODE_REQUEST_NOT_IMPLEMENT;
}
