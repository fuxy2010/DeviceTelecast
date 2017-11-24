// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "GeneralDef.h"
#include "TaskThread.h"

using namespace ScheduleServer;

void CTaskThread::sleep_ms(unsigned long interval)
{
#if 1
	unsigned begin, end;

	begin = timeGetTime();

	do 
	{
		end = timeGetTime();
	}
	while (interval > end - begin);
#else
	LARGE_INTEGER litmp;
	LONGLONG quad_part1, quad_part2;
	double dif_minus, freq, dif_sec;

	QueryPerformanceFrequency(&litmp);
	freq = (double)litmp.QuadPart;// 获得计数器的时钟频率

	QueryPerformanceCounter(&litmp);
	quad_part1 = litmp.QuadPart;// 获得初始值

	do
	{
		QueryPerformanceCounter(&litmp);
		quad_part2 = litmp.QuadPart;//获得中止值

		dif_minus = (double)(quad_part2 - quad_part1);
		dif_sec = dif_minus / freq;// 获得对应的时间值，单位为秒
	}
	while((interval / 1000) > dif_sec);
#endif
}

SS_Error CTaskThread::add_task(CTask* task)
{
	if(NULL  == task || true == _joined)
		return SS_InvalidTask;

	CSSLocker lock(&_mutex);

	task->set_owner(this);

	_task_queue.push_back(task);

	return SS_NoErr;
}

SS_Error CTaskThread::remove_all_tasks()
{
	while(_task_queue.size())
	{
		CTask* task = NULL;
		{
			CSSLocker lock(&_mutex);

			task  = *(_task_queue.begin());
			_task_queue.pop_front();
		}

		delete task;
		task = NULL;		
	}

	_task_queue.clear();

	return SS_NoErr;
}

unsigned long CTaskThread::get_task_num()
{
	CSSLocker lock(&_mutex);
	
	return static_cast<unsigned long>(_task_queue.size());
}

void CTaskThread::entry()
{
	statistic_enter('T');

	unsigned long start = timeGetTime();//本次线程循环执行起始时间
	bool has_run = false;//线程本次循环是否执行过任务

#ifdef _DEBUG
	//SINGLETON(CScheduleServer).singleton_test();
	//std::cout << "\nTask thread " << reinterpret_cast<unsigned long>(this) << " has " << _task_queue.size() << " tasks.";
	//if(_task_queue.size())
	//	std::cout << "\n<" << reinterpret_cast<unsigned long>(this) << ", " << _task_queue.size() << ">";
#endif

	//本次并发执行任务数
	unsigned long parallel_task_num = 100;//_parallel_task_num + (CTaskThreadPool::get_resident_task_count() % ((!CTaskThreadPool::get_task_thread_num()) ? 1 : CTaskThreadPool::get_task_thread_num()));

	unsigned long i = 0;
	while(i < parallel_task_num)//任务线程每次执行最多运行parallel_task_num个任务
	{
		if(true == _joined)
			break;

		CTask* task = NULL;

		//找到队列中第i个有效任务
		{
			//避免在加锁的范围内运行run
			CSSLocker lock(&_mutex);

			//任务队列为空时线程本次执行结束
			if(true == _task_queue.empty())
				break;

			//已遍历一次任务时线程本次执行结束
			if(i >= _task_queue.size())
				break;

			//查找_task_queue中第i+1个任务
			std::list<CTask*>::iterator iter = _task_queue.begin();

#if 0
			unsigned long j = 0;
			while(i > j++)
			{
				++iter;
			}
#else
			advance(iter, i);
#endif

			++i;//假定iter指向的task可用

			if(NULL != *iter)//iter指向非空任务指针
			{
				task = *iter;

				if(true == task->is_done())
				{
					_task_queue.erase(iter);
					--i;//推翻之前的假定
				}
			}
			else//iter指向空任务指针
			{
				_task_queue.erase(iter);
				--i;//推翻之前的假定
			}
		}

		//执行队列中第i个任务
		if(NULL != task)//注意task定义的位置及其初值
		{
			if(true == task->is_done())//一旦task的is_done()为true则不能再次run,避免重复调用on_done()
			{
				delete task;
				task = NULL;
			}
			else
			{
				task->run();
				has_run = true;
			}
		}
		
	}

	statistic_leave();

	//如果线程本次执行时间在30ms以内则休眠30ms，避免CPU占用率过高
	//注意不能在加锁区域执行Sleep否则影响效率
	if(timeGetTime() == start)
		Sleep(1);
}

void CTaskThread::on_start()
{
	_task_queue.clear();
}

void CTaskThread::on_close()
{
	cout << "\nremove all tasks." << endl;
	remove_all_tasks();
}

CTaskThread** CTaskThreadPool::_task_thread_array = NULL;
unsigned long CTaskThreadPool::_task_thread_num = 0;
HANDLE CTaskThreadPool::_create_task_thread_handle = ::CreateEvent(NULL, TRUE, FALSE, NULL);
bool CTaskThreadPool::_is_firstly_select = true;
unsigned long CTaskThreadPool::_resident_task_cout = 0;

SS_Error CTaskThreadPool::add_threads(unsigned long task_thread_num, void* parent)
{
	if(NULL != _task_thread_array)
		return SS_StartTaskThreadFail;

	_task_thread_array = new CTaskThread*[task_thread_num];

	if(NULL == _task_thread_array)
		return SS_StartTaskThreadFail;

	for(unsigned short x = 0; x < task_thread_num; ++x)
	{
		_task_thread_array[x] = new CTaskThread();
		if(SS_StartTaskThreadFail == _task_thread_array[x]->start(parent))
			return SS_StartTaskThreadFail;
	}

	_task_thread_num = task_thread_num;

	::SetEvent(_create_task_thread_handle);//通知select_thread可以运行

	return SS_NoErr;
}

void CTaskThreadPool::remove_threads()
{
	for(unsigned short x = 0; x < _task_thread_num; ++x)
	{
		_task_thread_array[x]->shutdown();
	}

	for(unsigned short y = 0; y < _task_thread_num; ++y)
	{
		delete _task_thread_array[y];
		_task_thread_array[y] = NULL;
	}

	delete[] _task_thread_array;
	_task_thread_array = NULL;

	_task_thread_num = 0;
}

CTaskThread* CTaskThreadPool::select_thread(unsigned long index)
{
	if(true == _is_firstly_select)//首次调用才检查
	{
		::WaitForSingleObject(_create_task_thread_handle, INFINITE);//等待任务线程创建完毕
		::CloseHandle(_create_task_thread_handle);

		_is_firstly_select = false;
	}

	if(!_task_thread_num)
		return NULL;

	//if(index)//index != 0时，专为添加DBUpdateTask用,根据index对任务线程数取模的结果选择线程
	//{
	//	return _task_thread_array[index % _task_thread_num];
	//}

	//选择任务数最少的线程
	//调用get_task_num易造成死锁
	/*CTaskThread* selected_thread = _task_thread_array[0];
	unsigned long task_num = _task_thread_array[0]->get_task_num();

	for(unsigned short x = 1; x < _task_thread_num; ++x)
	{
		selected_thread = (task_num < _task_thread_array[x]->get_task_num()) ? selected_thread : _task_thread_array[x];
	}

	return selected_thread;*/

	return _task_thread_array[index % _task_thread_num];

}
