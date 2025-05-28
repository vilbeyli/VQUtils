#pragma once
#include <future>
#include <cassert>

// utility function for checking if a std::future<> is ready without blocking
template<typename R> bool is_ready(std::future<R> const& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

// TaskSignal is a one time sync object, designed to be used with async tasks that return any type T.
template<typename T>
class TaskSignal
{
public:
	TaskSignal() : promise(), future(promise.get_future()) {}

	// Deleted copy/move to prevent misuse (promise/future can't be copied)
	TaskSignal(const TaskSignal&) = delete;
	TaskSignal& operator=(const TaskSignal&) = delete;
	TaskSignal(TaskSignal&& other) noexcept : promise(std::move(other.promise)), future(std::move(other.future)) {}
	TaskSignal& operator=(TaskSignal&&) = delete;

	inline void Notify(const T& value) { promise.set_value(value); }
	inline void Notify(T&& value) { promise.set_value(std::forward<T>(value)); }
	inline T Wait() { assert(future.valid()); return future.get(); }
	inline bool IsReady() const { return is_ready(future); }
	inline void Reset() 
	{  
		promise = std::promise<T>();
		future = promise.get_future();
	}

private:
	std::promise<T> promise; // signal ready
	std::future<T> future;   // wait signal
};

// void TaskSignal is a signal without any data attached -- used just for syncing instead of forwarding 
// any results from an async task.
template<>
class TaskSignal<void>
{
public:
	TaskSignal() : promise(), future(promise.get_future()) {}
	TaskSignal(const TaskSignal&) = delete;
	TaskSignal& operator=(const TaskSignal&) = delete;
	TaskSignal(TaskSignal&& other) noexcept : promise(std::move(other.promise)), future(std::move(other.future)) {}
	TaskSignal& operator=(TaskSignal&&) = delete;

	inline void Notify() { promise.set_value(); }
	inline void Wait() { assert(future.valid()); future.get(); }
	inline bool IsReady() const { return is_ready(future); }
	inline void Reset()
	{
		promise = std::promise<void>();
		future = promise.get_future();
	}

private:
	std::promise<void> promise; // signal ready
	std::future<void> future;   // wait signal
};
