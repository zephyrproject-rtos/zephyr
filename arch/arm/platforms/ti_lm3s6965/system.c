/* system.c - system/hardware module for ti_lm3s6965 platform */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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
This module provides routines to initialize and support board-level hardware
for the ti_lm3s6965 platform.
 */

#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <board.h>

#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers and the
 * integrated 16550-compatible UART device driver.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int ti_lm3s6965_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(ti_lm3_0, "", ti_lm3s6965_init, NULL);
pre_kernel_core_init(ti_lm3_0, NULL);
