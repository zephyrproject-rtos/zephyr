/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache 2.0
 */


#ifndef _ZEPHYR_SYSCALL_LIST_H_
#define _ZEPHYR_SYSCALL_LIST_H_

#ifndef _ASMLANGUAGE

enum {
	K_SYSCALL_BAD,
	K_SYSCALL_SEM_INIT,
	K_SYSCALL_SEM_GIVE,
	K_SYSCALL_SEM_TAKE,
	K_SYSCALL_SEM_RESET,
	K_SYSCALL_SEM_COUNT_GET,

	K_SYSCALL_LIMIT /* Always last */
};

#endif /* _ASMLANGUAGE */

#endif /* _ZEPHYR_SYSCALL_LIST_H_ */
