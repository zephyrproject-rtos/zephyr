/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa public exception handling
 *
 * Xtensa-specific kernel exception handling interface. Included by
 * arch/xtensa/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_EXCEPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

/* Xtensa uses a variable length stack frame depending on how many
 * register windows are in use.  This isn't a struct type, it just
 * matches the register/stack-unit width.
 */
struct arch_esf {
	int dummy;
};

#endif

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_EXCEPTION_H_ */
