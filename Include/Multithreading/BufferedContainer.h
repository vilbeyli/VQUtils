#pragma once
#include <mutex>
#include <array>

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
	bool IsEmpty() const
	{
		std::unique_lock<std::mutex> lk(mMtx);
		return mBufferPool[iBuffer].empty();
	}
private:
	mutable std::mutex mMtx;
	std::array<TContainer, 2> mBufferPool;
	int iBuffer = 0; // ping-pong index
};
