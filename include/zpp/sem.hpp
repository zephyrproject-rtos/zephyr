/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__SEM_HPP
#define ZEPHYR__INCLUDE__ZPP__SEM_HPP

#include <zpp/compiler.hpp>
#include <zpp/thread.hpp>

#include <kernel.h>
#include <sys/__assert.h>

#include <chrono>
#include <limits>

namespace zpp {

/**
 * @brief Counting semephore
 */
class sem final {
public:
	/**
	 * @brief Type used as counter
	 */
	using counter_type = u32_t;

	/**
	 * @brief Maxium value of the counter
	 */
	constexpr static counter_type max_count =
				std::numeric_limits<counter_type>::max();
public:
	/**
	 * @brief Constructor initializing initial count and count limit.
	 *
	 * @param initial_count The initial count value for the semaphore
	 * @param count_limit The maxium count the semaphore can have
	 */
	sem(counter_type initial_count, counter_type count_limit) noexcept
	{
		k_sem_init(&m_sem, initial_count, count_limit);
	}

	/**
	 * @brief Constructor initializing initial count.
	 *
	 * Contructor initializing initial count to @a initial_count and
	 * the maxium count limit to max_count.
	 *
	 * @param initial_count The initial count value for the semaphore
	 */
	explicit sem(counter_type initial_count) noexcept
		: sem(initial_count, max_count)
	{
	}

	/**
	 * @brief Default onstructor.
	 *
	 * Contructor initializing initial count to 0 and the maxium count
	 * limit to max_count.
	 */
	sem() noexcept
		: sem(0, max_count)
	{
	}

	/**
	 * @brief Take the semaphore waiting forever
	 *
	 * @return true when semaphore was taken
	 */
	[[nodiscard]] bool take() noexcept
	{
		if (k_sem_take(&m_sem, K_FOREVER) == 0) {
			return true;
		} else {
			return false;
		}
	}

	/**
	 * @brief Try to take the semaphore without waiting
	 *
	 * @return true when semaphore was taken
	 */
	[[nodiscard]] bool try_take() noexcept
	{
		if (k_sem_take(&m_sem, K_NO_WAIT) == 0) {
			return true;
		} else {
			return false;
		}
	}

	/**
	 * @brief Try to take the semaphore waiting a certain timeout
	 *
	 * @param timeout_duration The timeout to wait before giving up
	 *
	 * @return true when semaphore was taken
	 */
	template < class Rep, class Period>
	[[nodiscard]] bool
	try_take_for(const std::chrono::duration<Rep, Period>&
						timeout_duration) noexcept
	{
		using namespace std::chrono;

		milliseconds ms = duration_cast<milliseconds>(timeout_duration);

		if (k_sem_take(&m_sem, ms.count()) == 0) {
			return true;
		} else {
			return false;
		}
	}

	/**
	 * @brief Give the semaphore.
	 */
	void give() noexcept
	{
		k_sem_give(&m_sem);
	}

	/**
	 * @brief Reset the semaphore counter to zero.
	 */
	void reset() noexcept
	{
		k_sem_reset(&m_sem);
	}

	/**
	 * @brief Get current semaphore count
	 *
	 * @return The current semaphore count.
	 */
	counter_type count() const noexcept
	{
		return k_sem_count_get(&m_sem);
	}

	/**
	 * @brief Give the semaphore.
	 */
	void operator++(int) noexcept
	{
		give();
	}

	/**
	 * @brief Take the semaphore waiting forever
	 */
	void operator--(int) noexcept
	{
		while (!take()) {
			this_thread::yield();
		}
	}

	/**
	 * @brief Give the semaphore n times.
	 *
	 * @param n The number of times to give the semaphore
	 */
	void operator+=(int n) noexcept
	{
		while (n-- > 0) {
			give();
		}
	}

	/**
	 * @brief Take the semaphore n times, waiting forever.
	 *
	 * @param n The number of times to take the semaphore
	 */
	void operator-=(int n) noexcept
	{
		while (n-- > 0) {
			while (!take()) {
				this_thread::yield();
			}
		}
	}

	/**
 	 * @brief get the native zephyr sem handle.
	 *
	 * @return A pointer to the zephyr k_sem.
 	 */
	auto native_handle() noexcept
	{
		return &m_sem;
	}
private:
	mutable struct k_sem m_sem;
public:
	sem(const sem&) = delete;
	sem(sem&&) = delete;
	sem& operator=(const sem&) = delete;
	sem& operator=(sem&&) = delete;
};

} // namespace zpp

#endif // ZEPHYR__INCLUDE__ZPP__SEM_HPP
