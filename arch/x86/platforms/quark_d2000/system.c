/* system.c - system/hardware module for the Quark D2000 BSP */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
DESCRIPTION
This module provides routines to initialize and support board-level
hardware for the Quark D2000 BSP.
*/

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/printk.h>
#include <misc/__assert.h>
#include "board.h"
#include <drivers/mvic.h>
#include <init.h>

/**
 *
 * _InitHardware - perform basic hardware initialization
 *
 * Initialize the Quark D2000 Interrupt Controller (MVIC) device driver and the
 * Intel 8250 UART device driver.
 * Also initialize the timer device driver, if required.
 *
 * RETURNS: N/A
 */
static int quark_d2000_init(struct device *arg)
{
	ARG_UNUSED(arg);
	*((unsigned char *)(COM1_BASE_ADRS + SYNOPSIS_UART_DLF_OFFSET)) =
		COM1_DLF;
	*((unsigned char *)(COM2_BASE_ADRS + SYNOPSIS_UART_DLF_OFFSET)) =
		COM2_DLF;

	return 0;
}
DECLARE_DEVICE_INIT_CONFIG(quark_d2000_0, "", quark_d2000_init, NULL);
SYS_DEFINE_DEVICE(quark_d2000_0, NULL, PRIMARY,
					CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#ifdef CONFIG_MVIC
DECLARE_DEVICE_INIT_CONFIG(mvic_0, "", _mvic_init, NULL);
SYS_DEFINE_DEVICE(mvic_0, NULL, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_IOAPIC */
