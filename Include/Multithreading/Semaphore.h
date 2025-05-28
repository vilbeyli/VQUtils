#pragma once
#include <mutex>
#include <condition_variable>

//
// Synchronization Object similar to std::mutex except it allows multiple threads instead of just one
//
class Semaphore
{
public:
	Semaphore(int val, int max) : maxVal(max), currVal(val) {}

	inline void P() { Wait(); }
	inline void V() { Signal(); }

	inline void Wait() 
	{
		std::unique_lock<std::mutex> lk(mtx);
		cv.wait(lk, [&]() {return currVal > 0; });
		--currVal;
		return;
	}

	inline void Signal() 
	{
		std::unique_lock<std::mutex> lk(mtx);
		currVal = std::min<unsigned short>(currVal+1u, maxVal);
		cv.notify_one();
	}

private:
	unsigned short currVal, maxVal; // 65,535 max threads assumed.
	std::mutex mtx;
	std::condition_variable cv;
};
