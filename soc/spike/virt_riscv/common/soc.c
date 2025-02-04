/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief QEMU RISC-V virt machine hardware depended interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/htif.h>

/*
 * Exit QEMU and tell error number.
 *   Higher 16bits: indicates error number.
 *   Lower 16bits : set FINISHER_FAIL
 */

void sys_arch_reboot(int type)
{
	int success = 0;
	ARG_UNUSED(type);

	#if defined(CONFIG_UART_HTIF)
    tohost = (uint64_t)((success << 1) | 1); // HTIF Exit Command
    while (1);  // Halt execution
	#endif
}
