// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _CHRONOGRAPH_H_       
#define _CHRONOGRAPH_H_

//微秒级计时器
class CChronograph
{
public:
	CChronograph(unsigned long initial_timestamp, unsigned long interval)	:
	_timestamp(initial_timestamp),
	_interval(interval)
	{
	}

	virtual ~CTimeConsuming() {}

public:
	bool time_is_up()
	{
		unsigned long cur_timestamp = timeGetTime();

		//防止timeGetTime()返回值到达值域上限后归零
		unsigned long durance = (_timestamp > cur_timestamp) ? (0xFFFFFFFF - _timestamp + cur_timestamp) : (cur_timestamp - _timestamp);

		return (_interval <= durance);
	}

	void set_timestamp(unsigned long timestamp)
	{
		_timestamp = timestamp;
	}

	void set_timestamp()
	{
		_timestamp = timeGetTime();
	}

private:
	unsigned _timestamp;
	unsigned _interval;
};

#endif