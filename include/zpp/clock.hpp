/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__CLOCK_HPP
#define ZEPHYR__INCLUDE__ZPP__CLOCK_HPP

#include <zpp/compiler.hpp>

#include <kernel.h>
#include <sys/__assert.h>

#include <chrono>
#include <limits>

namespace zpp {

/**
 * @brief Clock measuring elapsed time since the system booted.
 */
class uptime_clock {
public:
	using rep = s64_t;
	using period = std::milli;
	using duration = std::chrono::duration<rep, period>;
	using time_point = std::chrono::time_point<uptime_clock>;
	static constexpr bool is_steady = false;

	/**
	 * @brief Get current uptime.
	 *
	 * @return current uptime as time_point.
	 */
	static time_point now() noexcept
	{
		return time_point(duration(k_uptime_get()));
	}
};


/**
 * @brief Clock representing the system’s hardware clock.
 */
class cycle_clock {
public:
	using rep = u64_t;
	using period = std::nano;
	using duration = std::chrono::duration<rep, period>;
	using time_point = std::chrono::time_point<cycle_clock>;
	static constexpr bool is_steady = false;

	/**
	 * @brief Get current cycle count.
	 *
	 * @return current cycle count as time_point
	 */
	static time_point now() noexcept
	{
		u64_t res = SYS_CLOCK_HW_CYCLES_TO_NS64( k_cycle_get_32() );
		return time_point(duration(res));
	}
};

/**
 * @brief convert a duration to tick
 *
 * @param d the std::chrono::duration to convert
 *
 * @return the number of tick @a d represents
 */
template< class Rep, class Period >
constexpr s32_t to_tick( const std::chrono::duration<Rep, Period>& d) noexcept
{
	using namespace std::chrono;
	milliseconds ms = duration_cast<milliseconds>(d);

	if (ms.count() >= std::numeric_limits<s32_t>::max()) {
		return std::numeric_limits<s32_t>::max();
	} else {
		return (s32_t)ms.count();
	}
}

} // namespace zpp

#endif // ZEPHYR__INCLUDE__ZPP__CLOCK_HPP
