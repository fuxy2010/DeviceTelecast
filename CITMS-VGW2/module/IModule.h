// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _I_MODULE_H_
#define _I_MODULE_H_

#include <string>
#include <vector>
#include <map>

#include "GeneralDef.h"

// DLL输出定义
#ifdef DLL_EXPORTS
#define MODULE_API __declspec (dllexport)
#else
#define MODULE_API __declspec(dllimport)
#endif

namespace ScheduleServer
{
	class IModule;

	//设备接入模块插件接口函数名
	#define DLL_EXPORT_FUNCTION_NAME "ModuleInstance"

	//设备接入模块插件接口函数的声明，该函数的定义在每个插件的头文件中
	#define DECLARE_EXPORT_FUNCTION() \
		MODULE_API IModule* ModuleInstance();

	//设备接入模块插件接口函数的定义，该函数的定义在每个插件的cpp文件中
	#define IMPLEMENT_EXPORT_FUNCTION(class_name) \
		IModule* ModuleInstance() \
		{\
			return new class_name##();\
		}

	//设备接入模块插件接口函数类型
	typedef IModule* (*PLUG_IN_MODULE_API)();

	typedef IModule* IModulePtr;                    // 模块指针类型定义
	typedef std::vector<IModulePtr> IModulePtrList; // 模块指针列表类型定义

	typedef struct tagMODULE_DESCRIPTION
	{
		//std::string name;//模块名称
		//std::string version;//模块版本
		std::string file_path;//插件模块对应的动态链接库文件绝对路径，仅对设备接入插件模块有效
		bool available;//模块对象使用状态

		// 缺省构造函数
		tagMODULE_DESCRIPTION() : file_path(""), available(false)
		{
		}
	}
	MODULE_DESCRIPTION, *MODULE_DESCRIPTION_PTR;

	//模块基类
	class IModule
	{
	public:
		// 缺省构造函数
		IModule() {};

		// 缺省析构函数
		virtual ~IModule() {};

		// 获取模块信息
		MODULE_DESCRIPTION& get_module_description() {return _module_description;};

		// 模块初始化
		virtual SS_Error start() = 0;

		// 模块卸载
		virtual SS_Error shutdown() = 0;

		bool is_available() { return _module_description.available; };

	private:
		MODULE_DESCRIPTION _module_description;   // 模块信息
	};

}

#endif // _I_MODULE_H_