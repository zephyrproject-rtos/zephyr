/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__MUTEX_HPP
#define ZEPHYR__INCLUDE__ZPP__MUTEX_HPP

#include <zpp/compiler.hpp>

#include <kernel.h>
#include <sys/__assert.h>

#include <chrono>
#include <mutex>

namespace zpp {

/**
 * @brief A recursive mutex class.
 */
class mutex {
public:
	/**
 	 * @brief Default contructor
 	 */
	mutex() noexcept
	{
		k_mutex_init(&m_mutex);
	}

	/**
 	 * @brief Lock the mutex. Wait for ever until it is locked.
	 *
	 * @return true if successfully locked.
 	 */
	[[nodiscard]] bool lock() noexcept
	{
		if (k_mutex_lock(&m_mutex, K_FOREVER) == 0) {
			return true;
		} else {
			return false;
		}
	}

	/**
 	 * @brief Try locking the mutex without waiting.
	 *
	 * @return true if successfully locked.
 	 */
	[[nodiscard]] bool try_lock() noexcept
	{
		if (k_mutex_lock(&m_mutex, K_NO_WAIT) == 0) {
			return true;
		} else {
			return false;
		}
	}

	/**
 	 * @brief Try locking the mutex with a timeout.
	 *
	 * @param timeout The time to wait before returning
	 *
	 * @return true if successfully locked.
 	 */
	template < class Rep, class Period>
	[[nodiscard]] bool
	try_lock_for(const std::chrono::duration<Rep, Period>& timeout) noexcept
	{
		using namespace std::chrono;

		if (k_mutex_lock(&m_mutex,
			duration_cast<milliseconds>(timeout).count()) == 0)
		{
			return true;
		} else {
			return false;
		}
	}

	/**
 	 * @brief Unlock the mutex.
 	 */
	void unlock() noexcept
	{
		k_mutex_unlock(&m_mutex);
	}

	/**
 	 * @brief get the native zephyr mutex handle.
	 *
	 * @return A pointer to the zephyr k_mutex.
 	 */
	auto native_handle() noexcept
	{
		return &m_mutex;
	}
private:
	struct k_mutex m_mutex;
public:
	mutex(const mutex&) = delete;
	mutex(mutex&&) = delete;
	mutex& operator=(const mutex&) = delete;
	mutex& operator=(mutex&&) = delete;
};

/**
 * @brief std::lock_guard using zpp::mutex as a lock.
 */
using lock_guard = ::std::lock_guard<::zpp::mutex>;

} // namespace zpp

#endif // ZEPHYR__INCLUDE__ZPP__MUTEX_HPP
