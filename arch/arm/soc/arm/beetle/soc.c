/*
 * Copyright (c) 2016 Linaro Limited.
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
 * @brief System/hardware module for the ARM LTD Beetle SoC
 *
 * This module provides routines to initialize and support board-level hardware
 * for the ARM LTD Beetle SoC.
 */

 #include <nanokernel.h>
 #include <device.h>
 #include <init.h>
 #include <soc.h>

 #include <arch/cpu.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * Assumption:
 * MAINCLK = 24Mhz
 *
 * @return 0
 */
static int arm_beetle_init(struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Setup various clocks and wakeup sources */
	soc_power_init();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(arm_beetle_init, PRE_KERNEL_1, 0);
