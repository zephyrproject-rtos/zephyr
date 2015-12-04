/*
 * Copyright (c) 2011-2015, Wind River Systems, Inc.
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

/**
 * @file
 * @brief System/hardware module for the QEMU platform
 *
 * This module provides routines to initialize and support board-level hardware
 * for the QEMU platform.
 */

#include <nanokernel.h>
#include "board.h"
#include <uart.h>
#include <drivers/pic.h>
#include <device.h>
#include <init.h>
#include <loapic.h>
#include <drivers/ioapic.h>



/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller and UARTs present in the
 * platform.
 *
 * @return 0
 */
static int qemu_x86_init(struct device *arg)
{
	ARG_UNUSED(arg);

	return 0;
}

#ifdef CONFIG_IOAPIC
DECLARE_DEVICE_INIT_CONFIG(ioapic_0, "", _ioapic_init, NULL);
SYS_DEFINE_DEVICE(ioapic_0, NULL, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_IOAPIC */

#ifdef CONFIG_LOAPIC
DECLARE_DEVICE_INIT_CONFIG(loapic_0, "", _loapic_init, NULL);
SYS_DEFINE_DEVICE(loapic_0, NULL, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_LOAPIC */

#if defined(CONFIG_PIC_DISABLE)

DECLARE_DEVICE_INIT_CONFIG(pic_0, "", _i8259_init, NULL);
SYS_DEFINE_DEVICE(pic_0, NULL, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_PIC_DISABLE */

DECLARE_DEVICE_INIT_CONFIG(qemu_x86_0, "", qemu_x86_init, NULL);
SYS_DEFINE_DEVICE(qemu_x86_0, NULL, SECONDARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
