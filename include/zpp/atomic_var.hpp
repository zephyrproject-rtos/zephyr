/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__ATOMIC_VAR_HPP
#define ZEPHYR__INCLUDE__ZPP__ATOMIC_VAR_HPP

#include <zpp/compiler.hpp>

#include <atomic.h>
#include <sys/__assert.h>

#include <cstddef>

namespace zpp {

/**
 * @brief class wrapping an atomic_var_t
 */
class atomic_var {
public:
	/**
	 * @brief the type used to store the value
	 */
	using value_type = atomic_val_t;
public:
	/**
	 * @brief default constructor that sets the value to 0
	 */
	constexpr atomic_var() noexcept
	{
	}

	/**
	 * @brief constructor that sets the value to v
	 *
	 * @param v the value to initialize the atomic_var with
	 */
	constexpr explicit atomic_var(value_type v) noexcept
		: m_var(v)
	{
	}

	/**
	 * @brief the size in bits of value_type
	 *
	 * @return the size of value_type in bits
	 */
	constexpr size_t bit_count() const noexcept {
		return sizeof(value_type) * 8;
	}

	/**
	 * @brief Atomic compare-and-set.
	 *
	 * This routine performs an atomic compare-and-set. If the current
	 * value equals @a old_val, the value is set to @a new_val.
	 * If the current value does not equal @a old_val, the value is
	 * unchanged.
	 *
	 * @param old_val Original value to compare against.
	 * @param new_val New value to store.
	 *
	 * @return true if @a new_val is written, false otherwise.
	 */
	[[nodiscard]] bool cas(value_type old_val, value_type new_val) noexcept
	{
		return atomic_cas(&m_var, old_val, new_val);
	}

	/**
	 * @brief Atomic addition.
	 *
	 * Atomically replace the current value with the result of the
	 * aritmetic addition of the value and @a val.
	 *
	 * @param val The other argument of the arithmetic addition.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type fetch_add(value_type val) noexcept
	{
		return atomic_add(&m_var, val);
	}

	/**
	 * @brief Atomic substraction.
	 *
	 * Atomically replace the current value with the result of the
	 * aritmetic substraction of the value and @a val.
	 *
	 * @param val The other argument of the arithmetic substraction.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type fetch_sub(value_type val) noexcept
	{
		return atomic_sub(&m_var, val);
	}

	/**
	 * @brief Atomic bitwise OR.
	 *
	 * Atomically replace the current value with the result of the
	 * bitwise OR of the value and @a val.
	 *
	 * @param val The other argument of the bitwise OR.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type fetch_or(value_type val) noexcept
	{
		return atomic_or(&m_var, val);
	}

	/**
	 * @brief Atomic bitwise XOR.
	 *
	 * Atomically replace the current value with the result of the
	 * bitwise XOR of the value and @a val.
	 *
	 * @param val The other argument of the bitwise XOR.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type fetch_xor(value_type val) noexcept
	{
		return atomic_xor(&m_var, val);
	}

	/**
	 * @brief Atomic bitwise AND.
	 *
	 * Atomically replace the current value with the result of the
	 * bitwise AND of the value and @a val.
	 *
	 * @param val The other argument of the bitwise AND.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type fetch_and(value_type val) noexcept
	{
		return atomic_and(&m_var, val);
	}

	/**
	 * @brief Atomic bitwise NAND.
	 *
	 * Atomically replace the current value with the result of the
	 * bitwise NAND of the value and @a val.
	 *
	 * @param val The other argument of the bitwise NAND.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type fetch_nand(value_type val) noexcept
	{
		return atomic_nand(&m_var, val);
	}

	/**
	 * @brief Atomic increment.
	 *
	 * Atomically replace the current value with the result of the
	 * aritmetic addition of the value and 1.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type fetch_inc() noexcept
	{
		return atomic_inc(&m_var);
	}

	/**
	 * @brief Atomic decrement.
	 *
	 * Atomically replace the current value with the result of the
	 * aritmetic substraction of the value and 1.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type fetch_dec() noexcept
	{
		return atomic_dec(&m_var);
	}

	/**
	 * @brief Atomically loads and returns the current value of the atomic
	 * variable.
	 *
	 * @return The current value of the atomic variable.
	 */
	[[nodiscard]] value_type load() const noexcept
	{
		return atomic_get(&m_var);
	}

	/**
	 * @brief Atomically replace current value of the atomic variable.
	 *
	 * @param val The value to store in the atomic variable
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type store(value_type val) noexcept
	{
		return atomic_set(&m_var, val);
	}

	/**
	 * @brief Atomically clear the atomic variable.
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type clear() noexcept
	{
		return atomic_clear(&m_var);
	}

	/**
	 * @brief atomically get a bit from the bitset
	 *
	 * @param bit the index of the bit to return
	 *
	 * @return the requested bit value
	 */
	[[nodiscard]] bool load(size_t bit) const noexcept
	{
		__ASSERT_NO_MSG(bit < (sizeof(value_type) * 8));
		return atomic_test_bit(&m_var, bit);
	}

	/**
	 * @brief atomically set a bit a value
	 *
	 * @param bit the index of the bit to set
	 * @param val the value the bit should be set to
	 */
	void store(size_t bit, bool val) noexcept
	{
		__ASSERT_NO_MSG(bit < (sizeof(value_type) * 8));
		atomic_set_bit_to(&m_var, bit, val);
	}

	/**
	 * @brief atomically set a bit to true/1
	 *
	 * @param bit the index of the bit to set
	 */
	void set(size_t bit) noexcept
	{
		__ASSERT_NO_MSG(bit < (sizeof(value_type) * 8));
		atomic_set_bit(&m_var, bit);
	}

	/**
	 * @brief atomically set a bit to false/0
	 *
	 * @param bit the index of the bit to set
	 */
	void clear(size_t bit) noexcept
	{
		__ASSERT_NO_MSG(bit < (sizeof(value_type) * 8));
		atomic_clear_bit(&m_var, bit);
	}

	/**
	 * @brief atomically clear a bit while returning the previous value.
	 *
	 * @param bit the index of the bit to set
	 *
	 * @return the bit value before it was cleared
	 */
	[[nodiscard]] bool fetch_and_clear(size_t bit) noexcept
	{
		__ASSERT_NO_MSG(bit < (sizeof(value_type) * 8));
		return atomic_test_and_clear_bit(&m_var, bit);
	}

	/**
	 * @brief atomically set a bit while returning the previous value.
	 *
	 * @param bit the index of the bit to set
	 *
	 * @return the bit value before it was set
	 */
	[[nodiscard]] bool fetch_and_set(size_t bit) noexcept
	{
		__ASSERT_NO_MSG(bit < (sizeof(value_type) * 8));
		return atomic_test_and_set_bit(&m_var, bit);
	}

	/**
	 * @brief Atomically loads and returns the current value of the atomic
	 * variable.
	 *
	 * @return The current value of the atomic variable.
	 */
	operator value_type () const noexcept {
		return load();
	}

	/**
	 * @brief Atomically replace current value of the atomic variable.
	 *
 	 * @param val The value to store in the atomic variable
	 *
	 * @return The value immediately preceding the effect or this function.
	 */
	value_type operator=(value_type val) noexcept {
		return store(val);
	}

	/**
	 * @brief Perform atomic pre-increment.
	 *
	 * Perform atomic pre-increment, equivalent to fetch_add(1) + 1
	 *
	 * @return The value immediately after the effect or this function.
	 */
	value_type operator++() noexcept {
		return fetch_add(1) + 1;
	}

	/**
	 * @brief Perform atomic post-increment.
	 *
	 * Perform atomic pre-increment, equivalent to fetch_add(1)
	 *
	 * @return The value immediately before the effect or this function.
	 */
	value_type operator++(int) noexcept {
		return fetch_add(1);
	}

	/**
	 * @brief Perform atomic pre-decrement.
	 *
	 * Perform atomic pre-decrement, equivalent to fetch_sub(1) - 1
	 *
	 * @return The value immediately after the effect or this function.
	 */
	value_type operator--() noexcept {
		return fetch_sub(1) - 1;
	}

	/**
	 * @brief Perform atomic post-decrement.
	 *
	 * Perform atomic pre-decrement, equivalent to fetch_sub(1)
	 *
	 * @return The value immediately before the effect or this function.
	 */
	value_type operator--(int) noexcept {
		return fetch_sub(1);
	}

	/**
	 * @brief Perform atomic addition.
	 *
	 * Perform atomic addition, equivalent to fetch_add(@a val) + @a val
	 *
	 * @param val The argument for the arithmetic operation
	 *
	 * @return The value immediately after the effect or this function.
	 */
	value_type operator+=(value_type val) noexcept {
		return fetch_add(val) + val;
	}

	/**
	 * @brief Perform atomic substraction.
	 *
	 * Perform atomic addition, equivalent to fetch_sub(@a val) - @a val
	 *
	 * @param val The argument for the arithmetic operation
	 *
	 * @return The value immediately after the effect or this function.
	 */
	value_type operator-=(value_type val) noexcept {
		return fetch_sub(val) - val;
	}

	/**
	 * @brief Perform atomic bitwise AND.
	 *
	 * Perform atomic bitwise AND, equivalent to fetch_and(@a val) & @a val
	 *
	 * @param val The argument for the arithmetic operation
	 *
	 * @return The value immediately after the effect or this function.
	 */
	value_type operator&=(value_type val) noexcept {
		return fetch_and(val) & val;
	}

	/**
	 * @brief Perform atomic bitwise OR.
	 *
	 * Perform atomic bitwise OR, equivalent to fetch_or(@a val) | @a val
	 *
	 * @param val The argument for the arithmetic operation
	 *
	 * @return The value immediately after the effect or this function.
	 */
	value_type operator|=(value_type val) noexcept {
		return fetch_or(val) | val;
	}

	/**
	 * @brief Perform atomic bitwise XOR.
	 *
	 * Perform atomic bitwise XOR, equivalent to fetch_xor(@a val) ^ @a val
	 *
	 * @param val The argument for the arithmetic operation
	 *
	 * @return The value immediately after the effect or this function.
	 */
	value_type operator^=(value_type val) noexcept {
		return fetch_xor(val) ^ val;
	}
private:
	atomic_t m_var{};
public:
	atomic_var(const atomic_var&) = delete;
	atomic_var(atomic_var&&) = delete;
	atomic_var& operator=(const atomic_var&) = delete;
	atomic_var& operator=(atomic_var&&) = delete;
};

} // namespace zpp

#endif // ZEPHYR__INCLUDE__ZPP__ATOMIC_VAR_HPP
