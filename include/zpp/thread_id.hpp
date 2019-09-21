/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__THREAD_ID_HPP
#define ZEPHYR__INCLUDE__ZPP__THREAD_ID_HPP

#include <zpp/compiler.hpp>

#include <kernel.h>
#include <sys/__assert.h>

namespace zpp {

/**
 * @brief Thead ID
 */
class thread_id {
public:
	thread_id(const thread_id&) noexcept = default;
	thread_id(thread_id&&) noexcept = default;
	thread_id& operator=(const thread_id&) noexcept = default;
	thread_id& operator=(thread_id&&) noexcept = default;

	/**
	 * @brief Default constructor inializing ID to K_ANY
	 */
	constexpr thread_id() noexcept
	{
	}

	/**
	 * @brief Constructor inializing ID to @a tid
	 *
	 * @param tid The ID to use for inialization
	 */
	constexpr explicit thread_id(k_tid_t tid) noexcept
		: m_tid(tid)
	{
	}

	/**
	 * @brief test if the ID is valid
	 *
	 * @return false if the ID equals K_ANY
	 */
	constexpr explicit operator bool() const noexcept
	{
		return m_tid != K_ANY;
	}

	/**
	 * @brief Create an ID with value K_ANY
	 *
	 * @return An ID with value K_ANY
	 */
	constexpr static thread_id any() noexcept {
		return thread_id(K_ANY);
	}

	/**
	 * @brief Get the Zephyr native ID value.
	 *
	 * @return The native ID value
	 */
	constexpr auto native_handle() const noexcept {
		return m_tid;
	}
private:
	k_tid_t	m_tid { K_ANY };
};

/**
 * @brief Compare if two ID are equal
 *
 * @param lhs Left hand side for comparison
 * @param rhs Right hand side for comparison
 *
 * @return true is @a lhs and @a rhs are equal
 */
constexpr bool
operator==(const thread_id& lhs, const thread_id& rhs) noexcept
{
	return (lhs.native_handle() == rhs.native_handle());
}

/**
 * @brief Compare if two ID are not equal
 *
 * @param lhs Left hand side for comparison
 * @param rhs Right hand side for comparison
 *
 * @return true is @a lhs and @a rhs are not equal
 */
constexpr bool
operator!=(const thread_id& lhs, const thread_id& rhs) noexcept
{
	return (lhs.native_handle() != rhs.native_handle());
}

/**
 * @brief Helper to output the ID with zpp::print("{}", id)
 *
 * @param id The ID to print
 */
inline void
print_arg(thread_id id)
{
	printk("%p", id.native_handle());
}

} // namespace zpp

#endif // ZEPHYR__INCLUDE__ZPP__THREAD_ID_HPP
