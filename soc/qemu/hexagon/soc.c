/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/arch.h>
#include <hexagon_vm.h>

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	hexagon_vm_stop(VM_STOP_RESTART);
}
