/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Validation macros for z_secure_vrfy_* functions (S-side only)
 *
 * Use these macros in hand-written z_secure_vrfy_* functions to validate
 * that pointers passed from NS code actually point into NS-accessible memory
 * before touching them. Accessing secure memory through an NS-supplied
 * pointer is a confused-deputy vulnerability.
 *
 * Example:
 *   int z_secure_vrfy_foo_encrypt(const uint8_t *in, size_t len, uint8_t *out)
 *   {
 *       Z_SECURE_MEMORY_READ(in, len);
 *       Z_SECURE_MEMORY_WRITE(out, len);
 *       return z_secure_impl_foo_encrypt(in, len, out);
 *   }
 *
 * These macros are only meaningful when CONFIG_ARM_SECURE_FIRMWARE is set.
 * Under any other configuration they compile to empty statements so that
 * single-image builds still link.
 */

#ifndef ZEPHYR_INCLUDE_INTERNAL_SECURE_CALL_HANDLER_H_
#define ZEPHYR_INCLUDE_INTERNAL_SECURE_CALL_HANDLER_H_

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_ARM_SECURE_FIRMWARE)

/*
 * arm_cmse.h is the standard CMSIS header for CMSE intrinsics.  It lives in
 * CMSIS/Core/Include/ which is always on the include path for ARM targets.
 * We use cmse_check_address_range() directly instead of the Zephyr-private
 * cortex_m/cmse.h wrappers, which are not on the public include path.
 */
#include <arm_cmse.h>
#include <zephyr/arch/secure_domain.h>

/**
 * @brief Abort if @p expr evaluates to zero/NULL/false.
 * @ingroup secure_call
 */
#define Z_SECURE_VERIFY(expr)                                                                      \
	do {                                                                                       \
		if (!(expr)) {                                                                     \
			arch_secure_call_oops();                                                   \
		}                                                                                  \
	} while (0)

/**
 * @brief Validate that [ptr, ptr+size) is fully in NS-readable memory.
 *
 * Uses the CMSE TT instruction (cmse_check_address_range) to check that the
 * entire range is Non-Secure and readable from the current privilege level.
 *
 * @ingroup secure_call
 */
#define Z_SECURE_MEMORY_READ(ptr, size)                                                            \
	Z_SECURE_VERIFY(cmse_check_address_range((void *)(uintptr_t)(ptr), (size),                 \
						 CMSE_NONSECURE | CMSE_MPU_READ) != NULL)

/**
 * @brief Validate that [ptr, ptr+size) is fully in NS-writable memory.
 * @ingroup secure_call
 */
#define Z_SECURE_MEMORY_WRITE(ptr, size)                                                           \
	Z_SECURE_VERIFY(cmse_check_address_range((void *)(uintptr_t)(ptr), (size),                 \
						 CMSE_NONSECURE | CMSE_MPU_READWRITE) != NULL)

#else /* !CONFIG_ARM_SECURE_FIRMWARE */

/* No-ops for single-image and NS builds so headers including this file compile */
#define Z_SECURE_VERIFY(expr)                                                                      \
	do {                                                                                       \
	} while (0)
#define Z_SECURE_MEMORY_READ(ptr, sz)                                                              \
	do {                                                                                       \
	} while (0)
#define Z_SECURE_MEMORY_WRITE(ptr, sz)                                                             \
	do {                                                                                       \
	} while (0)

#endif /* CONFIG_ARM_SECURE_FIRMWARE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_INTERNAL_SECURE_CALL_HANDLER_H_ */
