#pragma once
#include <mutex>
#include <condition_variable>

// --------------------------------------------------------------------------------------------------------------------------------------
//
// Sync Objects
//
//---------------------------------------------------------------------------------------------------------------------------------------
// 
// Wraps a std::conditional_variable with a std::mutex
//
class EventSignal
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

