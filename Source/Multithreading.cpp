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

#include "Multithreading.h"
#include "Utils.h"
#include "Log.h"

#include <cassert>
#include <algorithm>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

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

static void SetThreadName(std::thread& th, const wchar_t* threadName) {
	HRESULT hr = SetThreadDescription(th.native_handle(), threadName);
	if (FAILED(hr)) {
		// Handle error if needed
	}
}
void ThreadPool::Initialize(size_t numThreads, const std::string& ThreadPoolName, unsigned int MarkerColor)
{
	mMarkerColor = MarkerColor;
	mThreadPoolName = ThreadPoolName;
	mbStopWorkers.store(false);
	for (auto i = 0u; i < numThreads; ++i)
	{
		mWorkers.emplace_back(std::thread(&ThreadPool::Execute, this));
		SetThreadName(mWorkers.back(), StrUtil::ASCIIToUnicode(ThreadPoolName).c_str());
	}

#if RUN_THREADPOOL_UNIT_TEST
	RUN_THREAD_POOL_UNIT_TEST();
#endif
}
void ThreadPool::Destroy()
{
	mbStopWorkers.store(true);

	mSignal.NotifyAll();

	
	for (size_t iThread = 0; iThread < mWorkers.size(); ++iThread)
	{
		std::thread& worker = mWorkers[iThread];
		if (!worker.joinable())
		{
			Log::Info("%s : Thread[%d] is not joinable", mThreadPoolName.c_str(), iThread);
			continue;
		}
		worker.join();
	}
}

void ThreadPool::RunRemainingTasksOnThisThread()
{
	Task task; 
	while (mTaskQueue.TryPopTask(task))
	{ 
		task(); 
		mTaskQueue.OnTaskComplete(); 
	}
}
void ThreadPool::Execute()
{
	Task task;

	while (!mbStopWorkers.load())
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
	
	task = std::move(queue.top().task);
	queue.pop();
	return true;
}

std::vector<std::pair<size_t, size_t>> PartitionWorkItemsIntoRanges(size_t NumWorkItems, size_t NumWorkerThreadCount)
{
	// @NumWorkItems is distributed as equally as possible between all @NumWorkerThreadCount threads.
	// Two numbers are determined
	// - NumWorkItemPerThread: number of work items each thread will get equally
	// - NumWorkItemPerThread_Extra : number of +1's to be added to each worker
	const size_t NumWorkItemPerThread = NumWorkItems / NumWorkerThreadCount; // amount of work each worker is to get, 
	const size_t NumWorkItemPerThread_Extra = NumWorkItems % NumWorkerThreadCount;

	std::vector<std::pair<size_t, size_t>> vRanges(NumWorkItemPerThread == 0
		? NumWorkItems  // if NumWorkItems < NumWorkerThreadCount, then only create ranges according to NumWorkItems
		: NumWorkerThreadCount // each worker thread gets a range
	);

	size_t iRange = 0;
	for (auto& range : vRanges)
	{
		range.first = iRange != 0 ? vRanges[iRange - 1].second + 1 : 0;
		range.second = range.first + NumWorkItemPerThread - 1 + (NumWorkItemPerThread_Extra > iRange ? 1 : 0);
		assert(range.first <= range.second); // ensure work context bounds
		++iRange;
	}

	return vRanges;
}

size_t CalculateNumThreadsToUse(const size_t NumWorkItems, const size_t NumWorkerThreads, const size_t NumMinimumWorkItemCountPerThread)
{
#define DIV_AND_ROUND_UP(x, y) ((x+y-1)/(y))
	const size_t NumWorkItemsPerAvailableWorkerThread = DIV_AND_ROUND_UP(NumWorkItems, NumWorkerThreads);
	size_t NumWorkerThreadsToUse = NumWorkerThreads;
	if (NumWorkItemsPerAvailableWorkerThread < NumMinimumWorkItemCountPerThread)
	{
		const float OffRatio = float(NumMinimumWorkItemCountPerThread) / float(NumWorkItemsPerAvailableWorkerThread);
		NumWorkerThreadsToUse = static_cast<size_t>(NumWorkerThreadsToUse / OffRatio); // clamp down
		NumWorkerThreadsToUse = std::max((size_t)1, NumWorkerThreadsToUse);
	}
	return NumWorkerThreadsToUse;
}

