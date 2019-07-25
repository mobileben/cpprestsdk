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

#ifdef _WIN32
#error "ERROR: This file should only be included in non-windows Build"
#endif

namespace pplx
{
namespace details
{
namespace platform
{
	std::vector<std::unique_ptr<boost::asio::detail::thread>> m_threads;
    boost::asio::io_service m_service;
    boost::asio::io_service::work m_work(m_service);
    boost::asio::io_service::strand strand(m_service);

    bool init(false);

_PPLXIMP long GetCurrentThreadId() { return reinterpret_cast<long>(reinterpret_cast<void*>(pthread_self())); }

_PPLXIMP void YieldExecution() { std::this_thread::yield(); }
} // namespace platform


    static void* thread_start(void* arg) CPPREST_NOEXCEPT
    {
#if defined(__ANDROID__)
        // Calling get_jvm_env() here forces the thread to be attached.
        crossplat::get_jvm_env();
        pthread_cleanup_push(detach_from_java, nullptr);
#endif // __ANDROID__
        platform::m_service.run();
#if defined(__ANDROID__)
        pthread_cleanup_pop(true);
#endif // __ANDROID__
        return arg;
    }

_PPLXIMP void linux_scheduler::schedule(TaskProc_t proc, void* param)
{
	if (!platform::init) {
		platform::m_threads.push_back(
         std::unique_ptr<boost::asio::detail::thread>(new boost::asio::detail::thread([&] { thread_start(this); })));
        platform::m_threads.push_back(
            std::unique_ptr<boost::asio::detail::thread>(new boost::asio::detail::thread([&] { thread_start(this); })));
        platform::init = true;
	}

    //crossplat::threadpool::shared_instance().service().post(boost::bind(proc, param));
	    //platform::m_service.post(boost::bind(proc, param));
	    platform::strand.post(boost::bind(proc, param));
}

} // namespace details

} // namespace pplx
