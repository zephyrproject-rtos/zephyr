/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/arch/posix/posix_soc_if.h>

extern void nsi_raise_sigtrap(void);

FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	if (IS_ENABLED(CONFIG_ARCH_POSIX_TRAP_ON_FATAL)) {
		nsi_raise_sigtrap();
	}

	posix_print_error_and_exit("Exiting due to fatal error\n");
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
