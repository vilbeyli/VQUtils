//	VQUtils 
//	Copyright(C) 2020 - Volkan Ilbeyli
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

#define NOMINMAX

#include "Multithreading.h"
#include "Utils.h"
#include "Log.h"

#include <algorithm>

#define RUN_THREADPOOL_UNIT_TEST 0


const size_t ThreadPool::sHardwareThreadCount = std::thread::hardware_concurrency();

static void RUN_THREAD_POOL_UNIT_TEST()
{
	ThreadPool p;
	p.Initialize(ThreadPool::sHardwareThreadCount, "TEST POOL");

	constexpr long long sz = 40000000;
	auto sumRnd = [&]()
	{
		std::vector<long long> nums(sz, 0);
		for (int i = 0; i < sz; ++i)
		{
			nums[i] = MathUtil::RandI(0, 5000);
		}
		unsigned long long result = 0;
		for (int i = 0; i < sz; ++i)
		{
			if (nums[i] > 3000)
				result += nums[i];
		}
		return result;
	};
	auto sum = [&]()
	{
		std::vector<long long> nums(sz, 0);
		for (int i = 0; i < sz; ++i)
		{
			nums[i] = MathUtil::RandI(0, 5000);
		}
		unsigned long long result = 0;
		for (int i = 0; i < sz; ++i)
		{
			result += nums[i];
		}
		return result;
	};

	constexpr int threadCount = 16;
	std::future<unsigned long long> futures[threadCount] =
	{
		p.AddTask(sumRnd),
		p.AddTask(sumRnd),
		p.AddTask(sumRnd),
		p.AddTask(sumRnd),
		p.AddTask(sumRnd),
		p.AddTask(sumRnd),
		p.AddTask(sumRnd),
		p.AddTask(sumRnd),

		p.AddTask(sum),
		p.AddTask(sum),
		p.AddTask(sum),
		p.AddTask(sum),
		p.AddTask(sum),
		p.AddTask(sum),
		p.AddTask(sum),
		p.AddTask(sum),
	};

	std::vector<unsigned long long> results;
	unsigned long long total = 0;
	std::for_each(std::begin(futures), std::end(futures), [&](decltype(futures[0]) f)
	{
		results.push_back(f.get());
		total += results.back();
	});

	std::string strResult = "total (" + std::to_string(total) + ") = ";
	for (int i = 0; i < threadCount; ++i)
	{
		strResult += "(" + std::to_string(results[i]) + ") " + (i == threadCount - 1 ? "" : " + ");
	}
	Log::Info(strResult);
}




void Semaphore::Wait()
{
	std::unique_lock<std::mutex> lk(mtx);
	cv.wait(lk, [&]() {return currVal > 0; });
	--currVal;
	return;
}

void Semaphore::Signal()
{
	std::unique_lock<std::mutex> lk(mtx);
	currVal = std::min<unsigned short>(currVal+1u, maxVal);
	cv.notify_one();
}

void ThreadPool::Initialize(size_t numThreads, const std::string& ThreadPoolName)
{
	mThreadPoolName = ThreadPoolName;
	mbStopWorkers.store(false);
	for (auto i = 0u; i < numThreads; ++i)
	{
		mWorkers.emplace_back(std::thread(&ThreadPool::Execute, this));
	}

#if RUN_THREADPOOL_UNIT_TEST
	RUN_THREAD_POOL_UNIT_TEST();
#endif
}
void ThreadPool::Exit()
{
	mbStopWorkers.store(true);

	mSignal.NotifyAll();

	for (auto& worker : mWorkers)
	{
		worker.join();
	}
}

void ThreadPool::Execute()
{
	Task task;

	while (!mbStopWorkers)
	{
		mSignal.Wait([&] { return mbStopWorkers || !mTaskQueue.IsQueueEmpty(); });

		if (mbStopWorkers)
			break;
		
		if (!mTaskQueue.TryPopTask(task))
		{
			// Spurious wake-ups can happen before a notify_one() is called on the mSignal.
			// This means we can run into the following scenario:
			//- We push one item to the mTaskQueue but we're yet to call NotifyOne() on mSignal on the producer thread
			//- Random wake up of the thread(s) happens
			//- mSignal.condition_variable.wait() no longer blocks as the queue is no longer empty
			//- the first thread succeeds in TryPopTask(), but the rest of them will fail.
			//- if we don't check for queue.empty() in TryPopTask(), then std::queue will throw 'front() called on empty queue'
			continue;
		}

		task();
		mTaskQueue.OnTaskComplete();
	}
}

bool TaskQueue::TryPopTask(Task& task)
{
	std::lock_guard<std::mutex> lk(mutex);
	
	if (queue.empty())
		return false;
	
	task = std::move(queue.front());
	queue.pop();

	return true;
}


