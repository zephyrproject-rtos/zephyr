/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <sys/printk.h>
#include <inttypes.h>
#include <logging/log_ctrl.h>
#include "posix_soc_if.h"

FUNC_NORETURN void z_arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	posix_print_error_and_exit("Exiting due to fatal error\n");
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

