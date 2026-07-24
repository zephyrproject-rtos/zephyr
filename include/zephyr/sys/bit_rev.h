/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_BIT_REV_H_
#define ZEPHYR_INCLUDE_SYS_BIT_REV_H_

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Header file for bit reverse operations.
 *
 * @{
 */

#if HAS_BUILTIN(__builtin_bitreverse32)
#define sys_bit_rev32(x) __builtin_bitreverse32(x)
#elif defined(CONFIG_ARCH_HAS_BIT_REV)
#define sys_bit_rev32(x) arch_sys_bit_rev32(x)
#else
/**
 * @brief Reverse the bits in a 32-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint32_t sys_bit_rev32(uint32_t x)
{
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	return (x >> 16) | (x << 16);
}
#endif /* HAS_BUILTIN(__builtin_bitreverse32) */

#if HAS_BUILTIN(__builtin_bitreverse16)
#define sys_bit_rev16(x) __builtin_bitreverse16(x)
#elif defined(CONFIG_ARCH_HAS_BIT_REV)
#define sys_bit_rev16(x) arch_sys_bit_rev16(x)
#else
/**
 * @brief Reverse the bits in a 16-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint16_t sys_bit_rev16(uint16_t x)
{
	x = ((x >> 1) & 0x5555) | ((x & 0x5555) << 1);
	x = ((x >> 2) & 0x3333) | ((x & 0x3333) << 2);
	x = ((x >> 4) & 0x0F0F) | ((x & 0x0F0F) << 4);
	return (x >> 8) | (x << 8);
}
#endif /* HAS_BUILTIN(__builtin_bitreverse16) */

#if HAS_BUILTIN(__builtin_bitreverse8)
#define sys_bit_rev8(x)  __builtin_bitreverse8(x)
#elif defined(CONFIG_ARCH_HAS_BIT_REV)
#define sys_bit_rev8(x)  arch_sys_bit_rev8(x)
#else
/**
 * @brief Reverse the bits in an 8-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint8_t sys_bit_rev8(uint8_t x)
{
	x = ((x >> 1) & 0x55) | ((x & 0x55) << 1);
	x = ((x >> 2) & 0x33) | ((x & 0x33) << 2);
	return (x >> 4) | (x << 4);
}
#endif /* HAS_BUILTIN(__builtin_bitreverse8) */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_BIT_REV_H_ */
