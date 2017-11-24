#include "ModuleManager.h"
#include <iostream>

using namespace ScheduleServer;

CModuleManager::CModuleManager()
{
    _module_ptr_map.clear();
}

CModuleManager::~CModuleManager()
{
    unload_all();
}

IModule* CModuleManager::load(const std::string& module_file_path)
{
    HMODULE module = ::LoadLibrary(module_file_path.c_str());
    
	if(NULL == module)//加载插件失败
    {
		LPVOID lpMsgBuf; 

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
            NULL, 
            GetLastError(), 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language 
            (LPTSTR)&lpMsgBuf, 
            0, 
            NULL 
            );

		std::cout << "\nFail in LoadLibrary! file: " << module_file_path << std::endl
			<< (LPCTSTR)lpMsgBuf << ", error code: " << GetLastError() << std::endl;

        LocalFree(lpMsgBuf);

        return NULL;
    }

	//在映射中查找插件类指针
	std::map<HMODULE, IModule*>::iterator iter = _module_ptr_map.find(module);

	//如果找到直接返回
	if(_module_ptr_map.end() != iter)
		return iter->second;

	//如果没找到
	IModule* module_ptr = NULL;

	//获取设备接入模块插件的接口函数指针
	FARPROC far_proc = ::GetProcAddress(module, DLL_EXPORT_FUNCTION_NAME);

	if(far_proc)
	{
		PLUG_IN_MODULE_API module_api = (PLUG_IN_MODULE_API)far_proc;

		module_ptr = module_api();//创建设备接入模块插件对应的类对象

		if(NULL == module_ptr)
		{
			::FreeLibrary(module);
			return NULL;
		}
	}
	else
	{
		::FreeLibrary(module);
		return NULL;
	}

	//插件类对象创建成功

	// 根据插件名卸载已加载的插件
	unload(module_ptr->get_module_description().file_path);

	// 保存插件动态链接库文件
	module_ptr->get_module_description().file_path = module_file_path;

	// 插件初始化
	module_ptr->start();

	//加入插件列表
	_module_ptr_map[module] = module_ptr;//此时module_ptr一定不为NULL

	return module_ptr;
}

void CModuleManager::unload_all()
{
	for(std::map<HMODULE, IModule*>::iterator iter = _module_ptr_map.begin();
		iter != _module_ptr_map.end();
		++iter)
	{
		IModule* module_ptr = iter->second;

		if(NULL  != module_ptr)
		{
			module_ptr->shutdown();
			delete module_ptr;
			module_ptr = NULL;
		}

		::FreeLibrary(iter->first);
	}

	_module_ptr_map.clear();

}

bool CModuleManager::unload(const std::string& module_file_path)
{
	for(std::map<HMODULE, IModule*>::iterator iter = _module_ptr_map.begin();
		iter != _module_ptr_map.end();
		++iter)
	{
		IModule* module_ptr = iter->second;

		if(NULL == module_ptr)
		{
			::FreeLibrary(iter->first);

			_module_ptr_map.erase(iter);

			iter = _module_ptr_map.begin();//再重新开始找

			continue;
		}

		if(module_file_path == module_ptr->get_module_description().file_path)
		{
			module_ptr->shutdown();
			delete module_ptr;
			module_ptr = NULL;

			::FreeLibrary(iter->first);

			_module_ptr_map.erase(iter);

			return true;
		}
	}

    return false;
}

IModule* CModuleManager::get_module(const std::string& module_file_path)
{
	for(std::map<HMODULE, IModule*>::iterator iter = _module_ptr_map.begin();
		iter != _module_ptr_map.end();
		++iter)
	{
		IModule* module_ptr = iter->second;

		if(NULL == module_ptr)
		{
			::FreeLibrary(iter->first);

			_module_ptr_map.erase(iter);

			iter = _module_ptr_map.begin();//再重新开始找

			continue;
		}

		if(module_file_path == module_ptr->get_module_description().file_path)
			return module_ptr;
	}

	return NULL;
}

HMODULE CModuleManager::get_module_handle(const std::string& module_file_path)
{
	for(std::map<HMODULE, IModule*>::iterator iter = _module_ptr_map.begin();
		iter != _module_ptr_map.end();
		++iter)
	{
		IModule* module_ptr = iter->second;

		if(NULL == module_ptr)
		{
			::FreeLibrary(iter->first);

			_module_ptr_map.erase(iter);

			iter = _module_ptr_map.begin();//再重新开始找

			continue;
		}

		if(module_file_path == module_ptr->get_module_description().file_path)
			return iter->first;
	}

    return NULL;
}

void CModuleManager::get_module_list(IModulePtrList& miptr_list)
{
	std::map<HMODULE, IModulePtr>::iterator iter;
    for (iter = _module_ptr_map.begin(); iter != _module_ptr_map.end(); iter++)
    {
        miptr_list.push_back(iter->second);
    }
}

