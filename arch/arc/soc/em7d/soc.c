/* soc.c - system/hardware module for em_starterkit BSP */

/*
 * Copyright (c) 2016 Synopsys, Inc. All rights reserved.
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
 * This module provides routines to initialize and support board-level hardware
 * for the ARC EM Starter kit board.
 */

#include <nanokernel.h>
#include "soc.h"
#include <init.h>


/**
 *
 * @brief perform basic hardware initialization
 *
 * Hardware initialized:
 * - interrupt unit
 *
 * RETURNS: N/A
 */
static int em7d_arc_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_arc_v2_irq_unit_init();
	return 0;
}

SYS_INIT(em7d_arc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
