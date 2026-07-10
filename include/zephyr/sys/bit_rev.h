/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_BIT_REV_H_
#define ZEPHYR_INCLUDE_SYS_BIT_REV_H_

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

/**
 * @brief Reverse the bits in a 32-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint32_t sys_bit_rev32(uint32_t x)
{
#if defined(__clang__)
	return __builtin_bitreverse32(x);
#elif defined(CONFIG_ARM)
	__asm__("rbit %0, %1" : "=r"(x) : "r"(x));
	return x;
#elif defined(CONFIG_ARM64)
	__asm__("rbit %w0, %w1" : "=r"(x) : "r"(x));
	return x;
#elif defined(CONFIG_RISCV_ISA_EXT_ZBKB)
	unsigned long x_rev;

	__asm__("rev8 %0, %1\n"
		"brev8 %0, %0\n"
		: "=r"(x_rev)
		: "r"(x));

	return x_rev >> (__riscv_xlen - 32);
#else
	/* Fallback implementation for other architectures */
	uint32_t x_rev = 0;

	for (uint8_t i = 0; i < 32; i++) {
		if (IS_BIT_SET(x, i)) {
			x_rev |= BIT(32 - 1 - i);
		}
	}

	return x_rev;
#endif
}

/**
 * @brief Reverse the bits in a 16-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint16_t sys_bit_rev16(uint16_t x)
{
#if defined(__clang__)
	return __builtin_bitreverse16(x);
#elif defined(CONFIG_ARM) || defined(CONFIG_ARM64) || defined(CONFIG_RISCV_ISA_EXT_ZBKB)
	return sys_bit_rev32(x) >> 16;
#else
	/* Fallback implementation for other architectures */
	uint16_t x_rev = 0;

	for (uint8_t i = 0; i < 16; i++) {
		if (IS_BIT_SET(x, i)) {
			x_rev |= BIT(16 - 1 - i);
		}
	}

	return x_rev;
#endif
}

/**
 * @brief Reverse the bits in an 8-bit value.
 *
 * @param x Value to reverse
 *
 * @return reversed value
 */
static inline uint8_t sys_bit_rev8(uint8_t x)
{
#if defined(__clang__)
	return __builtin_bitreverse8(x);
#elif defined(CONFIG_ARM) || defined(CONFIG_ARM64)
	return sys_bit_rev32(x) >> 24;
#elif defined(CONFIG_RISCV_ISA_EXT_ZBKB)
	__asm__("rev8 %0, %1\n" : "=r"(x) : "r"(x));
	return x;
#else
	/* Fallback implementation for other architectures */
	uint8_t x_rev = 0;

	for (uint8_t i = 0; i < 8; i++) {
		if (IS_BIT_SET(x, i)) {
			x_rev |= BIT(8 - 1 - i);
		}
	}

	return x_rev;
#endif
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_BIT_REV_H_ */
