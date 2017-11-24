#include "ConfigBox.h"
#include <iostream>
#include <fstream>	
#include <sstream>

using namespace std;

// ----------------------------------------------------------------------
// 函数名: CConfigBox::CConfigBox
// 作者: fuym
// 描述: CConfigBox类的构造函数，成员初始化
// 返回值: 无
// 参数: 
//   [void]
// ----------------------------------------------------------------------
CConfigBox::CConfigBox() :
    _is_open(false),
    _is_modified(false),
    _config_file("")
{
    _config_lines.clear();
}

// ----------------------------------------------------------------------
// 函数名: CConfigBox::~CConfigBox
// 作者: fuym
// 描述: CConfigBox类的析构函数
// 返回值: 无
// 参数: 
//   [void]
// ----------------------------------------------------------------------
CConfigBox::~CConfigBox()
{
	_config_lines.clear();
}

// ----------------------------------------------------------------------
// 函数名: CConfigBox::load
// 作者: fuym
// 描述: 打开配置文件，读取配置信息列表
// 返回值: [void]
// 参数: 
//   [const std::string& config_file] 输入配置文件名  
// ----------------------------------------------------------------------
void CConfigBox::load(const std::string& config_file)
{
    // 解决ifstream打开文件中文路径名问题
    // 设置代码页为简体中文，936是简体中文的代码页。
    std::locale loc1 = std::locale::global(std::locale(".936"));

    // 打开配置文件
    ifstream ifs;
    ifs.open(config_file.c_str());
    _is_open = ifs.is_open();

    //恢复原来的代码页
    std::locale::global(std::locale(loc1));

    if (_is_open)
    {
        // 保存配置文件
        _config_file = config_file;
        
        // 读取配置信息
        _config_lines.clear();
        char buffer[1024];
        while(!ifs.eof())
        {
            memset(buffer, '\0', 1024);
            ifs.getline(buffer, 1024);
            string line = buffer;
            _config_lines.push_back(line);
        }

        _is_modified = false;
    }

	ifs.close();
}

// ----------------------------------------------------------------------
// 函数名: CConfigBox::save
// 作者: fuym
// 描述: 保存配置信息
// 返回值: [void]
// 参数: 
//   [void]
// ----------------------------------------------------------------------
void CConfigBox::save()
{
    // 如果配置文件打开失败或配置信息未修改，不做保存
    if (!_is_open || !_is_modified) return;

    ofstream ofs;
    ofs.open(_config_file.c_str());
    
    vector<string>::iterator iter = _config_lines.begin();
    while (iter != _config_lines.end())
    {
        if (iter != _config_lines.begin()) ofs << endl;
        ofs << *iter;
        iter++;
    }

    ofs.close();
    _is_modified = false;
}

// ----------------------------------------------------------------------
// 函数名: CConfigBox::get_property
// 作者: fuym
// 描述: 根据键值获取字符串属性值
// 返回值: [std::string] 字符串属性值
// 参数: 
//   [const std::string& key] 输入键值  
//   [const std::string& default_property] 输入缺省属性值  
// ----------------------------------------------------------------------
std::string CConfigBox::get_property(const std::string& key, const std::string& default_property)
{
    string result = default_property;

    string::size_type pos = -1;
    vector<string>::iterator iter;
    for (iter =_config_lines.begin(); iter != _config_lines.end(); iter++)
    {
        // 去除字符串前后空格
        string line = trim(*iter);

        // 忽略空行
        if (line.empty()) continue; 

        // 忽略注释行
        if (line.at(0) == '#') continue; 

        pos = line.find('=');
        if (pos == string::npos) continue;

        // 判断键值
        string line_key = trim(line.substr(0, pos)); 
        if (line_key == key)
        {
            result = trim(line.substr(pos + 1));
            break;
        }
    }

    return result;
}

// ----------------------------------------------------------------------
// 函数名: CConfigBox::get_property_as_int
// 作者: fuym
// 描述: 根据键值获取数字类型属性值
// 返回值: [int] 数字类型属性值
// 参数: 
//   [const std::string& key] 输入键值  
//   [int default_property] 输入缺省属性值  
// ----------------------------------------------------------------------
int CConfigBox::get_property_as_int(const std::string& key, int default_property)
{
    int result = default_property;

    string str = get_property(key, "");
    if (!str.empty())
    {
        result = atoi(str.c_str());
    }

    return result;
}

// ----------------------------------------------------------------------
// 函数名: CSingleLogger::GetPropertiesWithPrefix
// 作者: fuym
// 描述: 根据键值前缀获取属性列表
// 返回值: [void]
// 参数: 
//   [const std::string& key_prefix] 输入键值前缀  
//   [std::vector<std::string>& properties] 输出属性列表  
// ----------------------------------------------------------------------
void CConfigBox::get_properties_with_prefix(const std::string& key_prefix, std::vector<std::string>& properties)
{
    string::size_type pos = -1;
    vector<string>::iterator iter;
    for (iter =_config_lines.begin(); iter != _config_lines.end(); iter++)
    {
        // 去除字符串前后空格
        string line = trim(*iter);

        // 忽略空行
        if (line.empty()) continue; 

        // 忽略注释行
        if (line.at(0) == '#') continue; 

        pos = line.find('=');
        if (pos == string::npos) continue;

        // 判断键值前缀是否相同
        string line_key = trim(line.substr(0, pos));
        if (line_key.find(key_prefix) == string::npos) continue;

        string line_key_prefix = line_key.substr(0, key_prefix.size()); 
        string line_key_suffix = trim(line_key.substr(key_prefix.size(), line_key.size()));
        if (line_key_prefix == key_prefix)
        {
            properties.push_back(trim(line.substr(pos + 1)));
        }
    }
}

// ----------------------------------------------------------------------
// 函数名: CConfigBox::get_properties_map_with_prefix
// 作者: fuym
// 描述: 根据键值前缀获取属性列表
// 返回值: 
//   [void] 
// 参数: 
//   [const std::string& key_prefix] 输入键值前缀   
//   [std::map<std::string, std::string>& properties_map] 输出属性列表
// ----------------------------------------------------------------------
void CConfigBox::get_properties_map_with_prefix(const std::string& key_prefix, std::map<std::string, std::string>& properties_map)
{
    string::size_type pos = -1;
    vector<string>::iterator iter;
    for (iter =_config_lines.begin(); iter != _config_lines.end(); iter++)
    {
        // 去除字符串前后空格
        string line = trim(*iter);

        // 忽略空行
        if (line.empty()) continue; 

        // 忽略注释行
        if (line.at(0) == '#') continue; 

        pos = line.find('=');
        if (pos == string::npos) continue;

        // 判断键值前缀是否相同
        string line_key = trim(line.substr(0, pos));
        if (line_key.find(key_prefix) == string::npos) continue;

        string line_key_prefix = line_key.substr(0, key_prefix.size()); 
        string line_key_suffix = trim(line_key.substr(key_prefix.size(), line_key.size()));
        if (line_key_prefix == key_prefix)
        {
			properties_map[key_prefix + line_key_suffix] = trim(line.substr(pos + 1));
        }
    }
}

// ----------------------------------------------------------------------
// 函数名: CSingleLogger::set_property
// 作者: fuym
// 描述: 根据键值设置属性
// 返回值: [void]
// 参数: 
//   [const std::string& key] 输入键值  
//   [const std::string& property] 输入属性  
// ----------------------------------------------------------------------
void CConfigBox::set_property(const std::string& key, const std::string& property)	
{
    bool is_key_found = false;
    ostringstream oss;
    oss << key << "=" << property;

    string::size_type pos = -1;
    vector<string>::iterator iter;
    for (iter =_config_lines.begin(); iter != _config_lines.end(); iter++)
    {
        // 去除字符串前后空格
        string line = trim(*iter);

        // 忽略空行
        if (line.empty()) continue; 

        // 忽略注释行
        if (line.at(0) == '#') continue; 

        pos = line.find('=');
        if (pos == string::npos) continue;

        // 判断键值
        string line_key = trim(line.substr(0, pos)); 
        if (line_key == key)
        {
            is_key_found = true;
            *iter = oss.str();
            break;
        }
    }

    if (!is_key_found)
    {
        _config_lines.push_back(oss.str());
    }

    _is_modified = true;
}

// ----------------------------------------------------------------------
// 函数名: CSingleLogger::set_property_as_int
// 作者: fuym
// 描述: 根据键值设置属性
// 返回值: [void]
// 参数: 
//   [const std::string& key] 输入键值  
//   [int property] 输入属性  
// ----------------------------------------------------------------------
void CConfigBox::set_property_as_int(const std::string& key, int property)
{
    ostringstream oss;
    oss << property;
    set_property(key, oss.str());
}

// ----------------------------------------------------------------------
// 函数名: CSingleLogger::trim
// 作者: fuym
// 描述: 删除字符串前后空格、TAB键和回车换行符
// 返回值: [std::string] 字符串
// 参数: 
//   [const std::string& str] 输入字符串  
// ----------------------------------------------------------------------
std::string CConfigBox::trim(const std::string& str) 
{
    string t = str;
    t.erase(0, t.find_first_not_of(" \t\n\r"));
    t.erase(t.find_last_not_of(" \t\n\r") + 1);
    return t;
}
