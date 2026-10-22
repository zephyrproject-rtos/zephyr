/* ST Microelectronics STMEMS hal i/f
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zephyrproject-rtos/modules/hal/st/sensor/stmemsc/
 */

#include "stmemsc.h"

void stmemsc_mdelay(uint32_t millisec)
{
	k_msleep(millisec);
}
