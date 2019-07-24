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
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>

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

_PPLXIMP void YieldExecution() { 
	auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	printf("ZZZ YieldExecution ts=%llu\n", static_cast<unsigned long long>(epoch)); 
	std::this_thread::yield(); 
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
		    auto start = std::chrono::high_resolution_clock::now();
		   	auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		   	printf("ZZZ about to execute scheduled task ts=%llu\n", static_cast<unsigned long long>(epoch));
	        task.func(task.data);
	        auto now = std::chrono::high_resolution_clock::now();
	        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
    	    epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		   	printf("ZZZ execute scheduled task completed %llu, ts=%llu\n", static_cast<unsigned long long>(diff), static_cast<unsigned long long>(epoch));
	    }
    }
};

static std::unique_ptr<Worker> scheduleWorker;

_PPLXIMP void linux_scheduler::schedule(TaskProc_t proc, void* param)
{
#if 1
	#if 1
   	auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("ZZZ adding task: ts=%llu\n", static_cast<unsigned long long>(epoch));
    crossplat::threadpool::shared_instance().service().post(boost::bind(proc, param));
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
    std::async(std::launch::async, std::bind(proc, param));
    #endif /* ORIGINAL */
#endif
}

} // namespace details

} // namespace pplx
