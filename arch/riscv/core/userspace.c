/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/internal/syscall_handler.h>

/* Inner routine implemented in userspace.S. */
size_t z_riscv_user_string_nlen(const char *s, size_t maxsize, int *err_arg);

size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err)
{
#ifdef CONFIG_RISCV_USER_STRING_NLEN_VALIDATE
	/*
	 * z_riscv_user_string_nlen() recovers from a bad user pointer through a
	 * fault fixup that matches the faulting mepc against the load
	 * instruction's address range. On SoCs whose load access fault is
	 * imprecise, the reported mepc lands past the faulting load, so the
	 * fixup does not recognise the fault and it escalates to a fatal reset.
	 * Pre-validate the pointer so an invalid address is rejected before the
	 * dereferencing load is issued.
	 */
	if (arch_buffer_validate((void *)s, 1, 0) != 0) {
		*err = -1;
		return 0;
	}
#endif /* CONFIG_RISCV_USER_STRING_NLEN_VALIDATE */

	return z_riscv_user_string_nlen(s, maxsize, err);
}
