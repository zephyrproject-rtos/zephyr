/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <posix/pthread.h>

#include <system_error>
#include <thread>

namespace std
{
void thread::detach()
{
	int err;
	thread::id tid;
	::pthread_t pth;

	tid = this->get_id();
	pth = (::pthread_t)tid._M_thread;

	err = ::pthread_detach(pth);
	if (err)
		__throw_system_error(err);
}

void thread::join()
{
	int err;
	thread::id tid;
	::pthread_t pth;
	void *unused;

	tid = this->get_id();
	pth = (::pthread_t)tid._M_thread;

	err = ::pthread_join(pth, &unused);
	if (err)
		__throw_system_error(err);
}

thread::_State::~_State()
{
}

unsigned int thread::hardware_concurrency() noexcept
{
	return CONFIG_MP_NUM_CPUS;
}

extern "C" {
static void *execute_native_thread_routine(void *__p)
{
	thread::_State_ptr __t{ static_cast<thread::_State *>(__p) };
	__t->_M_run();
	return nullptr;
}
}

void thread::_M_start_thread(_State_ptr state, void (*)())
{
	const int err = __gthread_create(
		&_M_id._M_thread, &execute_native_thread_routine, state.get());

	if (err)
		__throw_system_error(err);
	state.release();
}

} /* namespace std */

extern "C" {

int __gthread_create(__gthread_t *__threadid, void *(*__func)(void *),
		     void *__args)
{
	return __gthrw_(pthread_create)(__threadid, NULL, __func, __args);
}
}
