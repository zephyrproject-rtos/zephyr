/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__TIMER_HPP
#define ZEPHYR__INCLUDE__ZPP__TIMER_HPP

#include <zpp/compiler.hpp>

#include <kernel.h>
#include <sys/__assert.h>

#include <chrono>
#include <functional>
#include <type_traits>

namespace zpp {

/**
 * @brief base class for the timer class
 */
class timer_base {
protected:
	timer_base() noexcept
	{
	}
public:
	/**
	 * @brief Destructor that stops the timer
	 */
	~timer_base()
	{
		stop();
	}

	/**
	 * @brief Start a timer with @a duration and @a period.
	 *
	 * @param duration The first time out
	 * @param period the time of the repeat period
	 */
	template <class Rep1, class Period1, class Rep2, class Period2>
	void start(const std::chrono::duration<Rep1, Period1>& duration,
		   const std::chrono::duration<Rep2, Period2>& period) noexcept
	{
		using namespace std::chrono;

		k_timer_start(&m_timer,
				duration_cast<milliseconds>(duration).count(),
				duration_cast<milliseconds>(period).count()
			);
	}

	/**
	 * @brief Start a single shot timer with @a duration
	 *
	 * @param duration The timeout
	 */
	template <class Rep, class Period>
	void start(const std::chrono::duration<Rep, Period>& duration) noexcept
	{
		using namespace std::chrono;

		k_timer_start(&m_timer,
				duration_cast<milliseconds>(duration).count(),
				0
			);
	}

	/**
	 * @brief Stop the timer
	 */
	void stop () noexcept
	{
		k_timer_stop(&m_timer);
	}

	/**
	 * @brief get the timer status
	 *
	 * @return the timer status
	 */
	auto status() noexcept
	{
		return k_timer_status_get(&m_timer);
	}

	/**
	 * @brief sync with the timer
	 */
	auto sync() noexcept
	{
		return k_timer_status_sync(&m_timer);
	}

	/**
	 * @brief Get remaining time
	 *
	 * @return the remaining time
	 */
	std::chrono::milliseconds remaining_time() noexcept
	{
		u32_t r = k_timer_remaining_get(&m_timer);
		return std::chrono::milliseconds(r);
	}

	/**
	 * @brief Zephyr native handle.
	 *
	 * @return pointer to the k_timer
	 */
	auto native_handle() noexcept
	{
		return &m_timer;
	}
private:
	struct k_timer	m_timer { };
public:
	timer_base(const timer_base&) = delete;
	timer_base(timer_base&&) = delete;
	timer_base& operator=(const timer_base&) = delete;
	timer_base& operator=(timer_base&&) = delete;
};

/**
 * @brief timer class
 *
 * @param ExpireCallback Type of the expire callback
 * @param StopCallback Type of the stop callback
 */
template <class ExpireCallback = std::nullptr_t,
	  class StopCallback = std::nullptr_t>
class timer : public timer_base
{
private:
	timer(ExpireCallback ecb, StopCallback scb, bool) noexcept
		: timer_base()
		, m_expire_callback(ecb)
		, m_stop_callback(scb)
	{
		k_timer_expiry_t ecb_func = nullptr;

		if constexpr (std::is_invocable<ExpireCallback,
							decltype(this)>::value)
		{
			ecb_func = [](struct k_timer* t) noexcept {
				auto self = get_user_data(t);
				if (self != nullptr) {
					std::invoke(self->m_expire_callback,
								self);
				}
			};
		}

		k_timer_stop_t scb_func = nullptr;

		if constexpr (std::is_invocable<StopCallback,
							decltype(this)>::value)
		{
			scb_func = [](struct k_timer* t) noexcept {
				auto self = get_user_data(t);
				if (self != nullptr) {
					std::invoke(self->m_stop_callback,
								self);
				}
			};
		}

		k_timer_init( native_handle(), ecb_func, scb_func);
		k_timer_user_data_set( native_handle(), this);
	}
public:
	/**
	 * @brief Create timer with expire and stop callbacks
	 *
	 * @param ecb The expire callback
	 * @param scb The stop callback
	 */
	timer(ExpireCallback ecb, StopCallback scb) noexcept
		: timer(ecb, scb, true)
	{
		static_assert(std::is_invocable<ExpireCallback,
						decltype(this)>::value,
				"Invalid expire callback");
		static_assert(std::is_invocable<StopCallback,
						decltype(this)>::value,
				"Invalid stop callback");
	}

	/**
	 * @brief Create timer with only a expire callback
	 *
	 * @param ecb The expire callback
	 */
	explicit timer(ExpireCallback ecb) noexcept
		: timer(ecb, nullptr, true)
	{
		static_assert(std::is_invocable<ExpireCallback,
						decltype(this)>::value,
				"Invalid expire callback");
	}

	/**
	 * @brief Create timer without callbacks
	 */
	timer() noexcept
		: timer(nullptr, nullptr, true)
	{
	}
private:
	static timer* get_user_data(struct k_timer* t) noexcept
	{
		return static_cast<timer*>(k_timer_user_data_get(t));
	}
private:
	ExpireCallback	m_expire_callback;
	StopCallback	m_stop_callback;
public:
	timer(const timer&) = delete;
	timer(timer&&) = delete;
	timer& operator=(const timer&) = delete;
	timer& operator=(timer&&) = delete;
};

} // namespace zpp

#endif // ZEPHYR__INCLUDE__ZPP__TIMER_HPP
