/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Apple-specific pplx implementations
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "pplx/pplx.h"
#include "pplx/threadpool.h"
#include <dispatch/dispatch.h>
#include <errno.h>
#include <libkern/OSAtomic.h>
#include <pthread.h>
#include <sys/time.h>
#include <memory>
#include <future> // Remove later
#include <chrono>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>

// DEVNOTE:
// The use of mutexes is suboptimal for synchronization of task execution.
// Given that scheduler implementations should use GCD queues, there are potentially better mechanisms available to
// coordinate tasks (such as dispatch groups).

namespace pplx
{
namespace details
{
namespace platform
{
_PPLXIMP long GetCurrentThreadId()
{
    pthread_t threadId = pthread_self();
    return (long)threadId;
}

void YieldExecution() { 
	auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	printf("ZZZ YieldExecution ts=%llu\n", static_cast<unsigned long long>(epoch)); 
	sleep(0); 
}

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

void apple_scheduler::schedule(TaskProc_t proc, void* param)
{
#if 0
	if (!scheduleWorker) {
		scheduleWorker = std::unique_ptr<Worker>(new Worker());
	}

   	auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("ZZZ adding task: ts=%llu\n", static_cast<unsigned long long>(epoch));
    #if 0
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async_f(queue, param, proc);
    #else
    	scheduleWorker->add(proc, param);
    #endif

	 void *array[128];
  size_t size;
  char **strings;
  size_t i;
  size_t len = 1024;
  int status;
  size = backtrace (array, 128);
  strings = backtrace_symbols (array, size);
  for (i = 0; i < size; i++) {
  			Dl_info info;
		if (dladdr(array[i], &info)) {
		     auto demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
		     if (!status) {
		     	printf("%s\n", demangled);
		     } else {
		     	printf("%s\n", strings[i]);
		     }
		} else {
			printf("%s\n", strings[i]);
		}
  }

  free (strings);
  printf("\n");
#else

#if 1
	printf("ZZZ dispatch async\n");
	    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	    dispatch_async_f(queue, param, proc);
#else
   	   	auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("ZZZ adding task: ts=%llu\n", static_cast<unsigned long long>(epoch));
    crossplat::threadpool::shared_instance().service().post(boost::bind(proc, param));
//    std::async(std::launch::async, std::bind(proc, param));
#endif
#endif
}

} // namespace details

} // namespace pplx
