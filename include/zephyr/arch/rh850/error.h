/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RH850 public error handling
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_ERROR_H_

/** @brief TRAP 0 vector number that raises a kernel fatal error */
#define RH850_TRAP_EXCEPT 2

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Raise a kernel fatal error through TRAP 0
 *
 * @param reason_p Reason code for the fatal error handler, passed in r6
 */
#define ARCH_EXCEPT(reason_p)                                                                      \
	do {                                                                                       \
		register unsigned int _r6 __asm__("r6") = (reason_p);                              \
                                                                                                   \
		__asm__ volatile("trap " STRINGIFY(RH850_TRAP_EXCEPT) : : "r"(_r6) : "memory");    \
	} while (false)

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_ERROR_H_ */
