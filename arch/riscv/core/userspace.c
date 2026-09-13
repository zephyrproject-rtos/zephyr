/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/sys/util.h>

/* z_riscv_user_string_nlen() (userspace.S) recovers from a bad user pointer
 * through a fault fixup that matches the faulting mepc against the load
 * instruction's address range. On SoCs whose load access fault is imprecise
 * the reported mepc lands past the faulting load, so the fixup does not
 * recognize the fault and it escalates to a fatal reset. The load may be at
 * any offset in the string (a string that starts in an accessible region and
 * runs unterminated into an inaccessible one), so validate before reading.
 *
 * The whole [s, s + maxsize) range is checked first, which covers the common
 * case with a single call. When that fails the string is walked in chunks
 * that end on PMP granularity boundaries. A region accepted by
 * arch_buffer_validate() does not necessarily end on such a boundary (the
 * read-only data window and memory domain partitions are not required to),
 * so a chunk that fails is retried one byte at a time before the string is
 * rejected. A short string that ends right before an inaccessible region is
 * then still measured correctly.
 */
size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err)
{
	uintptr_t addr = (uintptr_t)s;
	size_t len = 0;
	size_t chunk;
	size_t i;

	if (arch_buffer_validate(s, maxsize, 0) == 0) {
		while (len < maxsize && s[len] != '\0') {
			len++;
		}
		*err = 0;
		return len;
	}

	while (len < maxsize) {
		chunk = CONFIG_PMP_GRANULARITY - ((addr + len) & (CONFIG_PMP_GRANULARITY - 1));
		chunk = MIN(chunk, maxsize - len);

		if (arch_buffer_validate((const void *)(addr + len), chunk, 0) != 0) {
			chunk = 1;
			if (arch_buffer_validate((const void *)(addr + len), 1, 0) != 0) {
				*err = -1;
				return 0;
			}
		}

		for (i = 0; i < chunk; i++) {
			if (s[len] == '\0') {
				*err = 0;
				return len;
			}
			len++;
		}
	}

	*err = 0;
	return len;
}
