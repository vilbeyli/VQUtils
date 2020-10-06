//	VQUtils 
//	Copyright(C) 2020  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <atomic>

// utility function for checking if a std::future<> is ready without blocking
template<typename R> bool is_ready(std::future<R> const& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }


// --------------------------------------------------------------------------------------------------------------------------------------
//
// Sync Objects
//
//---------------------------------------------------------------------------------------------------------------------------------------
// 
// Wraps a std::conditional_variable with a std::mutex
//
class Signal
{
public:
	inline void NotifyOne() { cv.notify_one(); };
	inline void NotifyAll() { cv.notify_all(); };

	inline void Wait()               { std::unique_lock<std::mutex> lk(this->mtx); this->cv.wait(lk); }
	inline void Wait(bool(*pPred)()) { std::unique_lock<std::mutex> lk(this->mtx); this->cv.wait(lk, pPred); }
	template<class Functor>  // https://stackoverflow.com/questions/6458612/c0x-proper-way-to-receive-a-lambda-as-parameter-by-reference
	inline void Wait(Functor fn)    { std::unique_lock<std::mutex> lk(this->mtx); this->cv.wait(lk, fn); }

private:
	std::mutex mtx;
	std::condition_variable cv;
};

//
// Synchronization Object similar to std::mutex except it allows multiple threads instead of just one
//
class Semaphore
{
public:
	Semaphore(int val, int max) : maxVal(max), currVal(val) {}

	inline void P() { Wait(); }
	inline void V() { Signal(); }
	void Wait();
	void Signal();

private:
	unsigned short currVal, maxVal; // 65,535 max threads assumed.
	std::mutex mtx;
	std::condition_variable cv;
};


// --------------------------------------------------------------------------------------------------------------------------------------
//
// Thread Pool
//
//---------------------------------------------------------------------------------------------------------------------------------------
//
// Task Queue for thread pools
//
using Task = std::function<void()>;
class TaskQueue
{

// http://www.cplusplus.com/reference/thread/thread/
// https://stackoverflow.com/a/32593825/2034041
public:
	template<class T>
	void AddTask(std::shared_ptr<T>& pTask);
	bool TryPopTask(Task& task);

	inline bool IsQueueEmpty()      const { std::unique_lock<std::mutex> lock(mutex); return queue.empty(); }
	inline int  GetNumActiveTasks() const { return activeTasks; }

	// must be called after Task() completes.
	inline void OnTaskComplete() { --activeTasks; }

private:
	std::atomic<int> activeTasks = 0;
	mutable std::mutex mutex; // https://stackoverflow.com/a/25521702/2034041
	std::queue<Task> queue;
};
template<class T>
inline void TaskQueue::AddTask(std::shared_ptr<T>& pTask)
{
	std::unique_lock<std::mutex> lock(mutex);
	queue.emplace([=]() { (*pTask)(); });
	++activeTasks;
}



//
// A Collection of threads picking up tasks from its queue and executes on threads
// src: https://www.youtube.com/watch?v=eWTGtp3HXiw
//
class ThreadPool
{
public:
	const static size_t ThreadPool::sHardwareThreadCount;

	void Initialize(size_t numWorkers, const std::string& ThreadPoolName);
	void Exit();

	inline int GetNumActiveTasks() const { return IsExiting() ? 0 : mTaskQueue.GetNumActiveTasks(); };

	inline bool IsExiting() const { return mbStopWorkers.load(); }

	// Adds a task to the thread pool and returns the std::future<> 
	// containing the return type of the added task.
	//
	template<class T>
	auto AddTask(T task) -> std::future<decltype(task())>;


private:
	void Execute(); // workers run Execute();

	Signal                   mSignal;
	std::atomic<bool>        mbStopWorkers;
	TaskQueue                mTaskQueue;
	std::vector<std::thread> mWorkers;
	std::string              mThreadPoolName;
};

template<class T>
auto ThreadPool::AddTask(T task)->std::future<decltype(task())>
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
	using typename task_return_t = decltype(task()); // return type of task

	// use a shared_ptr<> of packaged tasks here as we execute them in the thread pool workers as well
	// as accesing its get_future() on the thread that calls this AddTask() function.
	auto pTask = std::make_shared< std::packaged_task<task_return_t()>>(std::move(task));
	mTaskQueue.AddTask(pTask);
	//Log::Info("[%s] TaskQueue::AddTask()", this->mThreadPoolName.c_str());

	mSignal.NotifyOne();
	//Log::Info("[%s] Signal::NotifyOne()", this->mThreadPoolName.c_str());
	return pTask->get_future();
}

// --------------------------------------------------------------------------------------------------------------------------------------
//
// Buffered Container
//
//---------------------------------------------------------------------------------------------------------------------------------------
//
// Double buffered generic container for multithreaded data communication.
//
// A use case is might be an event handling queue:
// - Producer writes event data into the Front container, might run in high frequency
// - Consumer thread wants to process the events, in batch
// - Consumer thread switches the container into a 'read-only' state with SwapBuffers() 
//   and gets the Back container which the main thread was writing previously.
// - Main thread now writes into the new Front container after the container switching with SwapBuffers()
// 
//
template<class TContainer, class TItem>
class BufferedContainer
{
public:
	TContainer& GetBackContainer() { return mBufferPool[(iBuffer + 1) % 2]; }
	const TContainer& GetBackContainer() const { return mBufferPool[(iBuffer+1)%2]; }
	
	void AddItem(TItem&& item)
	{
		std::unique_lock<std::mutex> lk(mMtx);
		mBufferPool[iBuffer].emplace(std::forward<TItem&&>(item));
	}
	void AddItem(const TItem& item)
	{
		std::unique_lock<std::mutex> lk(mMtx);
		mBufferPool[iBuffer].emplace(item);
	}
	void SwapBuffers() 
	{
		std::unique_lock<std::mutex> lk(mMtx);
		iBuffer ^= 1; 
	}
private:
	mutable std::mutex mMtx;
	std::array<TContainer, 2> mBufferPool;
	int iBuffer = 0; // ping-pong index
};

// --------------------------------------------------------------------------------------------------------------------------------------
//
// ConcurrentQueue
//
//---------------------------------------------------------------------------------------------------------------------------------------
//
// Wrapper for std::queue<> with a std::mutex to enable concurrency
//
template<class T>
class ConcurrentQueue
{
public:
	ConcurrentQueue(void (*pfnProcess)(T&)) : mpfnProcess(pfnProcess) {}

	void Enqueue(const T& item);
	void Enqueue(const T&& item);
	T Dequeue();

	void ProcessItems();

private:
	mutable std::mutex mMtx; // if compare_exchange any better?
	std::queue<T>      mQueue;
	void (*mpfnProcess)(T&);
};

template<class T>
inline void ConcurrentQueue<T>::Enqueue(const T& item)
{
	std::lock_guard<std::mutex> lk(mMtx);
	mQueue.push(item);
}

template<class T>
inline void ConcurrentQueue<T>::Enqueue(const T&& item)
{
	std::lock_guard<std::mutex> lk(mMtx);
	mQueue.push(std::move(item));
}

template<class T>
inline T ConcurrentQueue<T>::Dequeue()
{
	std::lock_guard<std::mutex> lk(mMtx);
	T item = mQueue.front();
	mQueue.pop();
	return T;
}

template<class T>
inline void ConcurrentQueue<T>::ProcessItems()
{
	if (!mpfnProcess)
		return;
	
	std::unique_lock<std::mutex> lk(mMtx);
	do
	{
		T&& item = std::move(mQueue.front());
		mQueue.pop();
		mpfnProcess(item);
	} while (!mQueue.empty());
}
