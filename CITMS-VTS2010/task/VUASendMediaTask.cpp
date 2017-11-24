// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "VUASendMediaTask.h"
#include "MiscTool.h"

using namespace ScheduleServer;

CVUASendMediaTask::CVUASendMediaTask(VUA_SEND_MEDIA_TASK_INFO& task_info) :
_status(VUASendMediaTask_Begin),
_ua(NULL),
_to_be_ended(false)
{
	_task_info = task_info;
	
	_ua = SINGLETON(CScheduleServer).fetch_ua(_task_info.ua_id);

	if(NULL == _ua)
		return;
}

CVUASendMediaTask::~CVUASendMediaTask()
{
}

SS_Error CVUASendMediaTask::run()
{
	if(NULL == _ua)
		return SS_NoErr;

	if(false == _ua->_start_hls)
		return SS_NoErr;

	if(true == _to_be_ended)
	{
		on_done();
		return SS_NoErr;
	}

	int ret = (true == _task_info.send_audio) ? _ua->send_audio() : _ua->send_video();

	return SS_NoErr;
}

SS_Error CVUASendMediaTask::on_done()
{
	_is_done = true;

	return SS_NoErr;
}

SS_Error CVUASendMediaTask::on_exception()
{
	return SS_NoErr;
}

SS_Error CVUASendMediaTask::close()
{
	_to_be_ended = true;

	return SS_NoErr;
}
