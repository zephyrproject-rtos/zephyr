/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/atomic.h>
#include <exception>
#include <cstdlib>

#if __cplusplus >= 201103L
#define __NORETURN [[ noreturn ]]
#else
#define __NORETURN FUNC_NORETURN
#endif

static void cxa_terminate_default_handler() noexcept
{
	abort();
}

namespace std
{

static terminate_handler __cxa_terminate_handler = cxa_terminate_default_handler;

terminate_handler get_terminate() noexcept
{
	return reinterpret_cast<terminate_handler>(
		atomic_ptr_get(reinterpret_cast<const atomic_ptr_t *>(&__cxa_terminate_handler)));
}

terminate_handler set_terminate(terminate_handler func) noexcept
{
	return reinterpret_cast<terminate_handler>(
		atomic_ptr_set(reinterpret_cast<atomic_ptr_t *>(&__cxa_terminate_handler),
			       reinterpret_cast<void*>(func)));
}

__NORETURN void terminate() noexcept
{
	terminate_handler term_handler = get_terminate();
	term_handler();
	while(1) {}
}

}  // std
