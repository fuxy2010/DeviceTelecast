// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _MISC_TOOL_H_
#define _MISC_TOOL_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <windows.h>
#include <direct.h>
#include <time.h>
#include <math.h>

//常用工具
namespace MiscTools
{
	//文件操作
	//获取文件大小
	unsigned long get_file_size(std::string file_path);

	//选择文件存放路径,默认文件大小为20MB
	std::string select_file_path(unsigned long file_size = (20480*1024));

	//从文件绝对路径中抽取文件名
	std::string extract_file_name_from_path(const std::string& file_path);

	//根据字符串哈希出一个10位无符号长整型数字
	unsigned long hast_string_to_ul(const std::string& str);

	//字符串操作
	//删除字符串左边空白
	std::string trim_string_left(const std::string& str);

	//删除字符串右边空白
	std::string trim_string_right(const std::string& str);

	//删除字符串左右两边空白
	std::string trim_string(const std::string& str);

	//将字符串转换为小写
	std::string string_to_lower(const std::string& str);

	//将字符串转换为大写
	std::string string_to_upper(const std::string& str);

	//字符串str是否以substr开头
	bool is_string_start_with(const std::string& str, const std::string& substr);

	//字符串str是否以substr开头
	bool is_string_end_with(const std::string& str, const std::string& substr);

	//忽略大小写比较字符串
	bool compare_string_ignore_case(const std::string& str1, const std::string& str2);

	//将时间转化为形如2010-01-01 12:30:30的字符串
	std::string parse_time_to_string(const struct tm& time);

	//将秒数转化为形如2010-03-17 16:45:00的字符串
	std::string parse_time_to_string(const time_t seconds);

	//将当前时间转化为形如2010-01-01 12:30:30的字符串
	std::string parse_now_to_string();

	//将形如20100101123030的字符串转换为时间
	struct tm parse_string_to_time(const std::string& str);

	//字符串转换为bool类型值
	bool parse_string_to_bool(const std::string& str);

	//字符串标识处理类
	class Tokenizer
	{
	public:
		static const std::string DEFAULT_DELIMITERS;
		Tokenizer(const std::string& str);
		Tokenizer(const std::string& str, const std::string& delimiters);

		//查找下一个标识
		bool NextToken(void);
		bool NextToken(const std::string& delimiters);

		//获取标识字符串
		const std::string GetToken(void) const;

		//重置标识位置
		void Reset(void);

	protected:
		size_t _offset;
		const std::string _string;
		std::string _token;
		std::string _delimiters;
	};

	//根据分隔符delimiters分割字符串
	void split_string(const std::string& str, const std::string& delimiters, std::vector<std::string>& str_vector);

	//将字符串字转化为其他类型
	template<class T> T parse_string_to_type(const std::string& str)
	{
		T value;
		istringstream iss(str);
		iss >> value;
		return value;
	};

	//将十六进制字符串字转化为其他类型
	template<class T> T parse_hex_string_to_type(const std::string& str)
	{
		T value;
		istringstream iss(str);
		iss >> hex >> value;
		return value;
	};

	//将其他类型转化为字符串
	template<class T> std::string parse_type_to_string(const T& value)
	{
		ostringstream oss;
		oss << value;
		return oss.str();
	};

	//将其他类型转化为十六进制字符串
	template<class T> std::string parse_type_to_hex_string(const T& value, int width)
	{
		ostringstream oss;
		oss << hex;
		if (width > 0) {
			oss << setw(width) << setfill('0');
		}
		oss << value;
		return oss.str();
	};

	//时间操作
	//得到当前的年份
	int get_cur_year();

	//计算形如2010-6-1 12:30:01的字符串代表的时戳间的时差秒数
	unsigned long get_time_diff(const std::string& time1, const std::string& time2);

	//将IP字符串转换为网络字节序IP数值
	unsigned long parse_string_to_ip(const char* str);

	//将网络字节序IP数值转换为字符串
	char* parse_ip_to_string(unsigned long ip);

	//当前的时戳距last_timestamp是否超过了INTERVAL
	inline bool time_is_up(unsigned long INTERVAL, unsigned long last_timestamp)
	{
		unsigned long cur_timestamp = timeGetTime();

		//防止timeGetTime()返回值到达值域上限后归零
		unsigned long durance = (last_timestamp > cur_timestamp) ? (0xFFFFFFFF - last_timestamp + cur_timestamp) : (cur_timestamp - last_timestamp);

		return (INTERVAL <= durance);
	}
};

#endif // _MISC_TOOL_H_
