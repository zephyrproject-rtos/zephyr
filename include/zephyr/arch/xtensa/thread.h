/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_H_

#ifndef _ASMLANGUAGE

/* Xtensa doesn't use these structs, but Zephyr core requires they be
 * defined so they can be included in struct _thread_base.  Dummy
 * field exists for sizeof compatibility with C++.
 */

struct _callee_saved {
	char dummy;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	uint32_t last_cpu;
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_H_ */
