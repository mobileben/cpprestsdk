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


_PPLXIMP void linux_scheduler::schedule(TaskProc_t proc, void* param)
{
   	auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("ZZZ adding task: ts=%llu\n", static_cast<unsigned long long>(epoch));
	#if 1
    crossplat::threadpool::shared_instance().service().post(boost::bind(proc, param));
    #else
    std::async(std::launch::async, std::bind(proc, param));
    #endif /* ORIGINAL */
}

} // namespace details

} // namespace pplx
