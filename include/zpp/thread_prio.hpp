/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__THREAD_PRIO_HPP
#define ZEPHYR__INCLUDE__ZPP__THREAD_PRIO_HPP

#include <zpp/compiler.hpp>

#include <kernel.h>
#include <sys/__assert.h>

namespace zpp {

/**
 * @brief Thread priority
 */
class thread_prio {
public:
	/**
	 * @brief Default constructor initializing priority to zero
	 */
	constexpr thread_prio() noexcept {}

	/**
	 * @brief Constructor initializing priority to @a prio
	 *
	 * @param prio The priority value to use for initialization
	 */
	constexpr explicit thread_prio(int prio) noexcept
		: m_prio(prio)
	{
		if (m_prio < -CONFIG_NUM_COOP_PRIORITIES) {
			m_prio = -CONFIG_NUM_COOP_PRIORITIES;
		} else if (m_prio > (CONFIG_NUM_PREEMPT_PRIORITIES - 1)) {
			m_prio = CONFIG_NUM_PREEMPT_PRIORITIES - 1;
		}
	}

	/**
	 * @brief Get the Zephyr native priority value
	 *
	 * @return the Zephyr native priority value
	 */
	constexpr int native_value() const noexcept
	{
		return m_prio;
	}

	/**
	 * @brief Get the most prior cooperative priority
	 *
	 * @return The most prior cooperative priority
	 */
	static constexpr thread_prio highest_coop() noexcept
	{
		return thread_prio(-CONFIG_NUM_COOP_PRIORITIES);
	}

	/**
	 * @brief Get the least prior cooperative priority
	 *
	 * @return The least prior cooperative priority
	 */
	static constexpr thread_prio lowest_coop() noexcept
	{
		return thread_prio(-1);
	}

	/**
	 * @brief Get the most prior preemptive priority
	 *
	 * @return The most prior preemptive priority
	 */
	static constexpr thread_prio highest_preempt() noexcept
	{
		return thread_prio(0);
	}

	/**
	 * @brief Get the least prior preemptive priority
	 *
	 * @return The least prior preemptive priority
	 */
	static constexpr thread_prio lowest_preempt() noexcept
	{
		return thread_prio(CONFIG_NUM_PREEMPT_PRIORITIES - 1);
	}

	/**
	 * @brief Create a cooperative priority value
	 *
	 * @param prio the value, from 0 to highest_coop
	 *
	 * @return The priority value
	 */
	static constexpr thread_prio coop(int prio) noexcept
	{
		return thread_prio( lowest_coop().native_value() - prio );
	}

	/**
	 * @brief Create a preemptive priority value
	 *
	 * @param prio the value, from 0 to highest_preempt
	 *
	 * @return The priority value
	 */
	static constexpr thread_prio preempt(int prio) noexcept
	{
		return (prio >= CONFIG_NUM_PREEMPT_PRIORITIES) ?
			highest_preempt() :
			thread_prio( lowest_preempt().native_value() - prio);
	}

	/**
	 * @brief Get the minium numeric value
	 *
	 * @return The smallest numeric value
	 */
	static constexpr thread_prio min_numeric() noexcept
	{
		return highest_coop();
	}

	/**
	 * @brief Get the maxium numeric value
	 *
	 * @return The highest numeric value
	 */
	static constexpr thread_prio max_numeric() noexcept
	{
		return lowest_preempt();
	}
private:
	int m_prio { 0 };
};

/**
 * @brief Compare if two priorities are equal
 *
 * @param lhs Left hand side for comparison
 * @param rhs Right hand side for comparison
 *
 * @return true is @a lhs and @a rhs are equal
 */
constexpr bool
operator==(const thread_prio& lhs, const thread_prio& rhs) noexcept
{
	return (lhs.native_value() == rhs.native_value());
}

/**
 * @brief Compare if two priorities are not equal
 *
 * @param lhs Left hand side for comparison
 * @param rhs Right hand side for comparison
 *
 * @return true is @a lhs and @a rhs are not equal
 */
constexpr bool
operator!=(const thread_prio& lhs, const thread_prio& rhs) noexcept
{
	return (lhs.native_value() != rhs.native_value());
}

/**
 * @brief Reduce priority by @a rhs
 *
 * @param lhs Left hand side for the operation
 * @param rhs Right hand side for the operation
 *
 * @return A priority value that is @a rhs less prior than @a lhs
 */
constexpr thread_prio
operator-(thread_prio lhs, int rhs) noexcept
{
	return thread_prio(lhs.native_value() + rhs);
}

/**
 * @brief Increase priority by @a rhs
 *
 * @param lhs Left hand side for the operation
 * @param rhs Right hand side for the operation
 *
 * @return A priority value that is @a rhs more prior than @a lhs
 */
constexpr thread_prio
operator+(thread_prio lhs, int rhs) noexcept
{
	return thread_prio(lhs.native_value() - rhs);
}

/**
 * @brief Helper to output the priority with zpp::print("{}", prio)
 *
 * @param prio The priority to print
 */
inline void print_arg(thread_prio prio)
{
	printk("%d", prio.native_value());
}

} // namespace zpp

#endif // ZEPHYR__INCLUDE__ZPP__THREAD_PRIO_HPP
