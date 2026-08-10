/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/toolchain.h>

/* not declared by every C library's stdlib.h */
extern FUNC_NORETURN void _exit(int status);

/*
 * Lock-free and callable from user mode: a slot is reserved by atomically
 * incrementing the count, then the handler pointer is stored; exit() skips
 * slots whose store has not yet landed.
 */
Z_LIBC_DATA static void (*atexit_handlers[CONFIG_COMMON_LIBC_ATEXIT_MAX])(void);
Z_LIBC_DATA static atomic_t atexit_count;

int atexit(void (*func)(void))
{
	atomic_val_t slot;

	if (func == NULL) {
		return -1;
	}

	do {
		slot = atomic_get(&atexit_count);
		if (slot >= CONFIG_COMMON_LIBC_ATEXIT_MAX) {
			return -1;
		}
	} while (!atomic_cas(&atexit_count, slot, slot + 1));

	atexit_handlers[slot] = func;

	return 0;
}

FUNC_NORETURN void exit(int status)
{
	for (int i = (int)atomic_get(&atexit_count) - 1; i >= 0; i--) {
		if (atexit_handlers[i] != NULL) {
			atexit_handlers[i]();
		}
	}

	_exit(status);
}
