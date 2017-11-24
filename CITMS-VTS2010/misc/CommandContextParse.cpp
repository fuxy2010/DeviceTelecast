// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "CommandContextParse.h"
#include <iostream>
#include <fstream>	
#include <sstream>

using namespace std;

CCommandContextParse::CCommandContextParse(const std::string& command_context, const std::string& interval_flag, const std::string& equal_flag) :
_interval_flag(""),
_equal_flag("")
{
	_command_line.clear();

	_interval_flag = interval_flag;

	_equal_flag = equal_flag;

	string::size_type pos_first = 0;

	string::size_type pos_second = command_context.find(_interval_flag);

	while(string::npos != pos_second)
	{
		_command_line.push_back(trim(command_context.substr(pos_first, pos_second - pos_first)));

		pos_first = pos_second + 1;

		pos_second = command_context.find(_interval_flag, pos_first);
	}

	if(pos_first < command_context.size())
	{
		_command_line.push_back(trim(command_context.substr(pos_first, command_context.size() - pos_first)));
	}
}

CCommandContextParse::~CCommandContextParse()
{
	_command_line.clear();
}

std::string CCommandContextParse::get_value(const std::string& key)
{
    string result = "";
    string::size_type pos = -1;

    for (vector<string>::iterator iter =_command_line.begin(); iter != _command_line.end(); iter++)
    {
        // 去除字符串前后空格
        string line = trim(*iter);

        // 忽略空行
        if (line.empty()) continue; 

		pos = line.find(_equal_flag);

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

void CCommandContextParse::get_values_with_prefix(const std::string& key_prefix, std::vector<std::string>& values)
{
	values.clear();

    string::size_type pos = -1;

    for(vector<string>::iterator iter =_command_line.begin(); iter != _command_line.end(); iter++)
    {
        // 去除字符串前后空格
        string line = trim(*iter);

        // 忽略空行
        if (line.empty()) continue; 

		pos = line.find(_equal_flag);

        if (pos == string::npos) continue;

        // 判断键值前缀是否相同
        string line_key = trim(line.substr(0, pos));

        if (line_key.find(key_prefix) == string::npos) continue;

        string line_key_prefix = line_key.substr(0, key_prefix.size());

        string line_key_suffix = trim(line_key.substr(key_prefix.size(), line_key.size()));

        if (line_key_prefix == key_prefix)
        {
            values.push_back(trim(line.substr(pos + 1)));
        }
    }
}

void CCommandContextParse::get_values_map_with_prefix(const std::string& key_prefix, std::map<std::string, std::string>& values_map)
{
	values_map.clear();

    string::size_type pos = -1;

    for (vector<string>::iterator iter =_command_line.begin(); iter != _command_line.end(); iter++)
    {
        // 去除字符串前后空格
        string line = trim(*iter);

        // 忽略空行
        if (line.empty()) continue; 

		pos = line.find(_equal_flag);
        if (pos == string::npos) continue;

        // 判断键值前缀是否相同
        string line_key = trim(line.substr(0, pos));

        if (line_key.find(key_prefix) == string::npos) continue;

        string line_key_prefix = line_key.substr(0, key_prefix.size()); 

        string line_key_suffix = trim(line_key.substr(key_prefix.size(), line_key.size()));

        if (line_key_prefix == key_prefix)
        {
			values_map[key_prefix + line_key_suffix] = trim(line.substr(pos + 1));
        }
    }
}

std::string CCommandContextParse::trim(const std::string& str) 
{
    string t = str;

    t.erase(0, t.find_first_not_of(" \t\n\r"));
    t.erase(t.find_last_not_of(" \t\n\r") + 1);

    return t;
}
