#pragma once
#include "EventSignal.h"

#include <atomic>
#include <queue>
#include <future>

// --------------------------------------------------------------------------------------------------------------------------------------
//
// Thread Pool
//
//---------------------------------------------------------------------------------------------------------------------------------------
//
// Task Queue for thread pools
//
enum class ETaskPriority
{
	IDLE = 0,
	LOW = 1,
	NORMAL = 2,
	HIGH = 3,
	VERY_HIGH = 4,
	CRITICAL = 5,
	REAL_TIME = 6
};
using Task = std::function<void()>;
class TaskQueue
{

// http://www.cplusplus.com/reference/thread/thread/
// https://stackoverflow.com/a/32593825/2034041
public:
	template<class T>
	void AddTask(std::shared_ptr<T>& pTask, ETaskPriority priority = ETaskPriority::NORMAL);
	bool TryPopTask(Task& task);

	inline bool IsQueueEmpty()      const { std::unique_lock<std::mutex> lock(mutex); return queue.empty(); }
	inline int  GetNumActiveTasks() const { return activeTasks; }

	// must be called after Task() completes.
	inline void OnTaskComplete() { --activeTasks; }

private:
	struct TaskEntry
	{
		Task task;
		ETaskPriority priority;
		uint64_t sequence; // for maintaining order within same priority
	};
	struct CompareTasks
	{
		bool operator()(const TaskEntry& a, const TaskEntry& b) const
		{
			if (a.priority != b.priority)
				return a.priority < b.priority;
			return a.sequence > b.sequence; // lower sequence = earlier task
		}
	};

	std::atomic<int> activeTasks = 0;
	mutable std::mutex mutex; // https://stackoverflow.com/a/25521702/2034041
	std::priority_queue<TaskEntry, std::vector<TaskEntry>, CompareTasks> queue;
	uint64_t sequenceCounter = 0;
};
template<class T>
inline void TaskQueue::AddTask(std::shared_ptr<T>& pTask, ETaskPriority priority)
{
	std::unique_lock<std::mutex> lock(mutex);
	queue.push(TaskEntry{ [=]() { (*pTask)(); }, priority, sequenceCounter++ });
	++activeTasks;
}



//
// A Collection of threads picking up tasks from its queue and executes on threads
// src: https://www.youtube.com/watch?v=eWTGtp3HXiw
//
class ThreadPool
{
public:
	const static size_t sHardwareThreadCount;

	void Initialize(size_t numWorkers, const std::string& ThreadPoolName, unsigned int MarkerColor = 0xFFAAAAAA);
	void Destroy();

	inline int GetNumActiveTasks() const { return IsExiting() ? 0 : mTaskQueue.GetNumActiveTasks(); };
	inline size_t GetThreadPoolSize() const { return mWorkers.size(); }
	
	inline std::string GetThreadPoolName() const { return mThreadPoolName; }
	inline std::string GetThreadPoolWorkerName() const { return mThreadPoolName + "_Worker"; }
	void RunRemainingTasksOnThisThread();

	inline bool IsExiting() const { return mbStopWorkers.load(); }

	// Adds a task to the thread pool and returns the std::future<> 
	// containing the return type of the added task.
	//
	template<class T>
	auto AddTask(T task, ETaskPriority priority = ETaskPriority::NORMAL) -> std::future<decltype(task())>;

private:
	void Execute(); // workers run Execute();

	EventSignal              mSignal;
	std::atomic<bool>        mbStopWorkers;
	TaskQueue                mTaskQueue;
	std::vector<std::thread> mWorkers;
	std::string              mThreadPoolName;

public:
	unsigned int             mMarkerColor;
};

template<class T>
auto ThreadPool::AddTask(T task, ETaskPriority priority) -> std::future<decltype(task())>
{
	// Notes on C++11 Threading:
	// ------------------------------------------------------------------------------------
	// 
	// To get a return value from an executing thread, we use std::packaged_task<> together
	// with std::future to access the results later.
	//
	// e.g. http://en.cppreference.com/w/cpp/thread/packaged_task
	// 
	// ------------------------------------------------------------------------------------
	using task_return_t = decltype(task()); // return type of task

	// use a shared_ptr<> of packaged tasks here as we execute them in the thread pool workers as well
	// as accesing its get_future() on the thread that calls this AddTask() function.
	auto pTask = std::make_shared< std::packaged_task<task_return_t()>>(std::move(task));
	mTaskQueue.AddTask(pTask, priority);
	//Log::Info("[%s] TaskQueue::AddTask()", this->mThreadPoolName.c_str());

	mSignal.NotifyOne();
	//Log::Info("[%s] EventSignal::NotifyOne()", this->mThreadPoolName.c_str());
	return pTask->get_future();
}

std::vector<std::pair<size_t, size_t>> PartitionWorkItemsIntoRanges(size_t NumWorkItems, size_t NumWorkerThreadCount);
size_t CalculateNumThreadsToUse(const size_t NumWorkItems, const size_t NumWorkerThreads, const size_t NumMinimumWorkItemCountPerThread);
