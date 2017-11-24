// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _COMMAND_CONTEX_PARSE_H_
#define _COMMAND_CONTEX_PARSE_H_

#include <string>
#include <vector>
#include <map>

//命令内容解析类
class CCommandContextParse
{
public:
	CCommandContextParse(const std::string& command_context, const std::string& interval_flag = ";", const std::string& equal_flag = ",");
    virtual ~CCommandContextParse();

    virtual std::string get_value(const std::string& key);
    virtual void get_values_with_prefix(const std::string& key_prefix, std::vector<std::string>& values);	
    virtual void get_values_map_with_prefix(const std::string& key_prefix, std::map<std::string, std::string>& values_map);

private:
    // 删除字符串前后空格、TAB键和回车换行符
    std::string trim(const std::string& str);

private:
	std::string _interval_flag;//间隔符号

	std::string _equal_flag;//等于符号

    std::vector<std::string> _command_line; // 配置信息列表
};

#endif // _COMMAND_CONTEX_PARSE_H_
