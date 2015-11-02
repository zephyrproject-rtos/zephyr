/* ia32.c - system/hardware module for the ia32 platform */

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

/*
 * DESCRIPTION
 * This module provides routines to initialize and support board-level hardware
 * for the ia32 platform.
 */

#include <nanokernel.h>
#include "board.h"
#include <uart.h>
#include <drivers/pic.h>
#include <device.h>
#include <init.h>
#include <loapic.h>
#include <drivers/ioapic.h>
#include <drivers/hpet.h>



/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller and UARTs present in the
 * platform.
 *
 * @return 0
 */
static int ia32_init(struct device *arg)
{
	ARG_UNUSED(arg);

	return 0;
}

#ifdef CONFIG_IOAPIC
DECLARE_DEVICE_INIT_CONFIG(ioapic_0, "", _ioapic_init, NULL);
pre_kernel_core_init(ioapic_0, NULL);

#endif /* CONFIG_IOAPIC */

#ifdef CONFIG_LOAPIC
DECLARE_DEVICE_INIT_CONFIG(loapic_0, "", _loapic_init, NULL);
pre_kernel_core_init(loapic_0, NULL);

#endif /* CONFIG_LOAPIC */

#if defined(CONFIG_PIC_DISABLE)

DECLARE_DEVICE_INIT_CONFIG(pic_0, "", _i8259_init, NULL);
pre_kernel_core_init(pic_0, NULL);

#endif /* CONFIG_PIC_DISABLE */

DECLARE_DEVICE_INIT_CONFIG(ia32_0, "", ia32_init, NULL);
pre_kernel_early_init(ia32_0, NULL);
