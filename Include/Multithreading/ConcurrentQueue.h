#pragma once
#include <mutex>
#include <queue>

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
	return item;
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
