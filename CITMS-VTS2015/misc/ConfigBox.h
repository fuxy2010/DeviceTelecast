#ifndef _CCONFIGBOX_H_
#define _CCONFIGBOX_H_

#include <string>
#include <vector>
#include <map>

//
// 通用配置信息读写类
//
class CConfigBox
{
public:
    CConfigBox();
    virtual ~CConfigBox();

    // 加载配置文件
    virtual void load(const std::string& config_file);

    // 保存配置文件
    virtual void save();

    // 读取配置信息
    virtual std::string get_property(const std::string& key, const std::string& default_property);
    virtual int get_property_as_int(const std::string& key, int default_property);
    virtual void get_properties_with_prefix(const std::string& key_prefix, std::vector<std::string>& properties);	
    virtual void get_properties_map_with_prefix(const std::string& key_prefix, std::map<std::string, std::string>& properties_map);

    // 写配置信息
    virtual void set_property(const std::string& key, const std::string& property);	
    virtual void set_property_as_int(const std::string& key, int property);	

private:
    // 删除字符串前后空格、TAB键和回车换行符
    std::string trim(const std::string& str);

private:
    bool _is_open;                          // 配置文件是否打开成功标志
    bool _is_modified;                      // 配置信息是否被修改标志
    std::string _config_file;               // 配置文件名
    std::vector<std::string> _config_lines; // 配置信息列表
};

#endif // _CCONFIGBOX_H_
