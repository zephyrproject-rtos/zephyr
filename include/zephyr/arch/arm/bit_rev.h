/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_BIT_REV_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_BIT_REV_H_

#ifndef _ASMLANGUAGE

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

#if defined(CONFIG_ARCH_HAS_BIT_REV) || defined(__DOXYGEN__)

/**
 * @brief Reverse the bits in a 32-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint32_t arch_sys_bit_rev32(uint32_t x)
{
	__asm__("rbit %0, %1" : "=r"(x) : "r"(x));
	return x;
}

/**
 * @brief Reverse the bits in a 16-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint16_t arch_sys_bit_rev16(uint16_t x)
{
	return arch_sys_bit_rev32(x) >> 16;
}

/**
 * @brief Reverse the bits in an 8-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint8_t arch_sys_bit_rev8(uint8_t x)
{
	return arch_sys_bit_rev32(x) >> 24;
}
#endif /* CONFIG_ARCH_HAS_BIT_REV || __DOXYGEN__ */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARM_BIT_REV_H_ */
