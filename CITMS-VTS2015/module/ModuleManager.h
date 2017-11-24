#ifndef _MODULE_MANAGER_H_
#define _MODULE_MANAGER_H_

#include "IModule.h"
#include <windows.h>

namespace ScheduleServer
{
	class CModuleManager
	{
	public:
		// 构造函数
		CModuleManager();

		// 析构函数
		~CModuleManager();

		// 加载插件, 返回插件指针
		IModule* load(const std::string& module_file_path);

		// 根据插件名卸载插件
		bool unload(const std::string& module_file_path);

		// 卸载所有插件
		void unload_all();

		// 根据插件名查找插件指针
		IModule* get_module(const std::string& module_file_path);

		// 根据插件名查找插件句柄
		HMODULE get_module_handle(const std::string& module_file_path);

		// 获取加载插件指针列表
		void get_module_list(IModulePtrList& miptr_list);

	private:
		// 插件对象列表
		std::map<HMODULE, IModule*> _module_ptr_map;
	};
}

#endif // _MODULE_MANAGER_H_
