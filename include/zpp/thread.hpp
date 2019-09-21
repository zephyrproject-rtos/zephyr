/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__THREAD_HPP
#define ZEPHYR__INCLUDE__ZPP__THREAD_HPP

#include <zpp/compiler.hpp>
#include <zpp/thread_prio.hpp>
#include <zpp/thread_id.hpp>
#include <zpp/thread_attr.hpp>
#include <zpp/clock.hpp>

#include <kernel.h>
#include <sys/__assert.h>

#include <functional>
#include <chrono>
#include <tuple>

namespace zpp {

/**
 * @brief provide functions that access the current thread of execution.
 */
namespace this_thread {

/**
 * @brief Get the thread ID of the current thread
 *
 * @return The thread ID of the current thread.
 */
inline auto get_id() noexcept
{
	return ::zpp::thread_id(k_current_get());
}

/**
 * @brief Yield the current thread.
 */
inline void yield() noexcept
{
	k_yield();
}

/**
 * @brief Busy wait for a specified time duration
 *
 * @param wait_duration The time to busy wait
 */
template< class Rep, class Period >
inline void
busy_wait_for(const std::chrono::duration<Rep, Period>& wait_duration)
{
	using namespace std::chrono;

	microseconds us = duration_cast<microseconds>(wait_duration);
	k_busy_wait(us.count());
}

/**
 * @brief Suspend the current thread for a specified time duration
 *
 * @param wait_duration The time to sleep
 */
template< class Rep, class Period >
inline auto
sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration)
{
	s32_t res = k_sleep(to_tick(sleep_duration));

	return std::chrono::milliseconds(res);
}

/**
 * @brief Suspend the current thread until a specified time point
 *
 * @param wait_duration The time point util the current thread will sleep
 */
template< class Clock, class Duration >
inline void
sleep_until( const std::chrono::time_point<Clock, Duration>& sleep_time)
{
	using namespace std::chrono;

	Duration dt;
	while ( (dt = sleep_time - Clock::now()) > Duration::zero()) {
		k_sleep(to_tick(dt));
	}
}

/**
 * @brief Abort the current thread
 */
inline void abort() noexcept
{
	k_thread_abort(k_current_get());
}

/**
 * @brief Suspend the current thread
 */
inline void suspend() noexcept
{
	k_thread_suspend(k_current_get());
}

/**
 * @brief Get the current thread's priority
 *
 * @return The priority of the current thread
 */
inline thread_prio get_priority() noexcept
{
	return thread_prio( k_thread_priority_get(k_current_get()) );
}

/**
 * @brief Set the current thread's priority
 *
 * @param prio The new priority of the current thread
 */
inline void set_priority(thread_prio prio) noexcept
{
	k_thread_priority_set(k_current_get(), prio.native_value());
}

} // namespace this_thread


template <class T> typename std::decay<T>::type decay_copy(T&& v) noexcept
{
	return std::forward<T>(v);
}

template <struct k_thread* TCB, k_thread_stack_t* Stack, u32_t Size>
class thread_tcb final {
public:
	constexpr auto get_tcb() const noexcept { return TCB; }
	constexpr auto get_stack() const noexcept { return Stack; }
	constexpr auto get_stack_size() const noexcept { return Size; }
};

/**
 * @brief The class thread repesents a single Zephyr thread.
 */
class thread {
private:
	template <typename CallInfo>
	static void callback_helper(void* a1, void* a2, void* a3) noexcept
	{
		CallInfo* cip = static_cast<CallInfo*>(a1);

		std::apply(cip->m_f, cip->m_args);

		cip->~CallInfo();
	}
public:
	/**
	 * @brief Creates a object which doesn't represent a Zephyr thread.
	 */
	constexpr thread() noexcept
	{
	}

	/**
	 * @brief Creates a object which represents Zephyr thread with tid.
	 *
	 * @param tid The ID to manage
	 */
	constexpr explicit thread(thread_id tid) noexcept
		: m_tid(tid)
	{
	}

	/**
	 * @brief Creates a object which represents a new Zephyr thread.
	 *
	 * @param tcb The TCB to use
	 * @param attr The creation attributes to use
	 * @param f The thread entry point
	 * @param args The arguments to pass to @a f
	 */
	template< class TCB, typename Callback, typename... CallbackArgs>
	constexpr thread(
			const TCB& tcb,
			const thread_attr& attr,
			Callback&& f,
			CallbackArgs&&... args) noexcept
	{
		typedef typename std::decay<Callback>::type CallInfoF;
		typedef std::tuple<typename std::decay<CallbackArgs>::type...>
				CallInfoArgs;

		struct call_info {
			constexpr call_info(CallInfoF&& f,
					CallInfoArgs&& args) noexcept
				: m_f(f)
				, m_args(args) {}

			CallInfoF	m_f;
			CallInfoArgs	m_args;
		};

		auto stack = tcb.get_stack();
		auto stack_size = tcb.get_stack_size();

		call_info* cip =
			new(stack) call_info (
				decay_copy(std::forward<Callback>(f)),
				decay_copy(std::forward_as_tuple(args...)));

		stack = stack + sizeof(call_info);
		stack_size = stack_size - sizeof(call_info);

		auto tid = k_thread_create(tcb.get_tcb(),
					stack, stack_size,
					&callback_helper<call_info>,
					(void*)cip, nullptr, nullptr,
					attr.native_prio(),
					attr.native_options(),
					attr.native_delay());

		m_tid = thread_id(tid);
	}

	/**
	 * @brief Move constructor
	 *
	 * @param other the thread to move to this thread object, after the
	 *        move @a other will not manage the thread anymore
	 */
	constexpr thread(thread&& other) noexcept
		: m_tid(other.m_tid)
	{
		other.m_tid = thread_id::any();
	}

	/**
	 * @brief Move assignment operator
	 *
	 * @param other the thread to move to this thread object, after the
	 *        move @a other will not manage the thread anymore
	 */
	constexpr thread& operator=(thread&& other) noexcept
	{
		m_tid = other.m_tid;
		other.m_tid = thread_id::any();

		return *this;
	}

	/**
	 * @brief Destructor, will abort the thread.
	 */
	~thread() noexcept
	{
		if (m_tid) {
			k_thread_abort(m_tid.native_handle());
		}
	}

	/**
	 * @brief check if this object manages a thread
	 *
	 * @return true if this object manages a thread
	 */
	constexpr explicit operator bool() const noexcept {
		return !!m_tid;
	}

	/**
	 * @brief Detach this object from the thread.
	 */
	constexpr void detach() noexcept
	{
		m_tid = thread_id::any();
	}

	/**
	 * @brief wakeup the thread this object mamages.
	 */
	void wakeup() noexcept
	{
		if (m_tid) {
			k_wakeup(m_tid.native_handle());
		}
	}

	/**
	 * @brief start the thread this object mamages.
	 */
	void start() noexcept
	{
		if (m_tid) {
			k_thread_start(m_tid.native_handle());
		}
	}

	/**
	 * @brief abort the thread this object mamages.
	 */
	void abort() noexcept
	{
		if (m_tid) {
			k_thread_abort(m_tid.native_handle());
			m_tid = thread_id::any();
		}
	}

	/**
	 * @brief resume the thread this object mamages.
	 */
	void resume() noexcept
	{
		if (m_tid) {
			k_thread_resume(m_tid.native_handle());
		}
	}

	/**
	 * @brief suspend the thread this object mamages.
	 */
	void suspend() noexcept
	{
		if (m_tid) {
			k_thread_suspend(m_tid.native_handle());
		}
	}

	/**
	 * @brief Get priority of the thread this object mamages.
	 *
	 * @return Thread priority
	 */
	thread_prio priority() noexcept
	{
		if (m_tid) {
			return thread_prio(
				k_thread_priority_get(m_tid.native_handle()) );
		} else {
			return {};
		}
	}

	/**
	 * @brief Set priority of the thread this object mamages.
	 *
	 * @param prio The new thread priority
	 */
	void set_priority(thread_prio prio) const noexcept
	{
		if (m_tid) {
			k_thread_priority_set(m_tid.native_handle(),
						prio.native_value());
		}
	}

	/**
	 * @brief Set name of the thread this object mamages.
	 *
	 * @param name The new thread name
	 */
	bool set_name(const char* name) noexcept
	{
		if (m_tid) {
			int rc = k_thread_name_set(m_tid.native_handle(), name);
			if (rc == 0) {
				return true;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}

	/**
	 * @brief Get name of the thread this object mamages.
	 *
	 * @return The thread name or nullptr
	 */
	const char* name() const noexcept
	{
		if (m_tid) {
			return k_thread_name_get(m_tid.native_handle());
		} else {
			return nullptr;
		}
	}
private:
	thread_id	m_tid;
public:
	thread(const thread&) = delete;
	thread& operator=(const thread&) = delete;
};

} // namespace zpp

/**
 * @brief Macro to define a Thread Control Block
 *
 * @param sym The symbol name of the TCB
 * @param stack_size The stack size of the thread
 */
#define ZPP_THREAD_TCB_DEFINE(sym, stack_size) \
	namespace { \
		K_THREAD_STACK_DEFINE(sym##_native_stack, (stack_size)); \
		struct k_thread sym##_native_tcb; \
	} \
	const ::zpp::thread_tcb<&sym##_native_tcb, sym##_native_stack, \
				K_THREAD_STACK_SIZEOF(sym##_native_stack)> sym

#endif // ZEPHYR__INCLUDE__ZPP__THREAD_HPP
