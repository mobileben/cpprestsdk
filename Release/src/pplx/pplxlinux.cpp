/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Parallel Patterns Library - Linux version
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "pplx/pplx.h"
#include "pplx/threadpool.h"
#include "sys/syscall.h"
#include <thread>
#include <future> // Remove later
#include <chrono>

#ifdef _WIN32
#error "ERROR: This file should only be included in non-windows Build"
#endif

namespace pplx
{
namespace details
{
namespace platform
{
_PPLXIMP long GetCurrentThreadId() { return reinterpret_cast<long>(reinterpret_cast<void*>(pthread_self())); }

_PPLXIMP void YieldExecution() { std::this_thread::yield(); }
} // namespace platform

struct WorkerTask {
	std::function<void(void*)> func;
	void *data;
	WorkerTask() : data(0) {}
	WorkerTask(std::function<void(void*)> f, void *d) : func(f), data(d) {}
};

struct Worker {
    std::queue<WorkerTask>	queue_;
	std::thread 				thread_;
    std::mutex                  mutex_;
    std::condition_variable     cond_;

    Worker() {
    	thread_ = std::thread(std::ref(*this));
    }
    ~Worker() {
    	thread_.join();
    }
    void add(TaskProc_t proc, void* param) {
   		std::unique_lock<std::mutex> mlock(mutex_);
	    queue_.push(WorkerTask(std::bind(proc, param), param));
	    mlock.unlock();
	    cond_.notify_one();
    }
    void operator()() {
   		while (true) {
	        auto& queue = queue_;
	        auto& mutex = mutex_;
	        WorkerTask task;
	        {   // Create new scope for lock
	            std::unique_lock<std::mutex> mlock(mutex);
	            
	            while (queue.empty()) {
	                cond_.wait(mlock);
	            }
	            
	                task = std::move(queue.front());
	                queue.pop();
	        }
	        task.func(task.data);
	    }
    }
};

static std::unique_ptr<Worker> scheduleWorker;

_PPLXIMP void linux_scheduler::schedule(TaskProc_t proc, void* param)
{
   	auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("ZZZ adding task: ts=%llu\n", static_cast<unsigned long long>(epoch));
	if (!scheduleWorker) {
		scheduleWorker = std::unique_ptr<Worker>(new Worker());
	}
    scheduleWorker->add(proc, param);

#if 0
	#if 1
	printf("binding\n");
    crossplat::threadpool::shared_instance().service().post(boost::bind(proc, param));
    #else
    std::async(std::launch::async, std::bind(proc, param));
    #endif /* ORIGINAL */
#endif
}

} // namespace details

} // namespace pplx
