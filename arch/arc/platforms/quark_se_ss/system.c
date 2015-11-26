/* system.c - system/hardware module for quark_se_ss BSP */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
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
 * for the Quark SE platform.
 */

#include <nanokernel.h>
#include <board.h>
#include <init.h>
#include <quark_se/shared_mem.h>
#include <ipi/ipi_quark_se.h>

/* Cannot use microkernel, since only nanokernel is supported */
#if defined(CONFIG_MICROKERNEL)
#error "Microkernel support is not available"
#endif

/**
 *
 * @brief perform basic hardware initialization
 *
 * Hardware initialized:
 * - interrupt unit
 *
 * RETURNS: N/A
 */
static int quark_se_arc_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_arc_v2_irq_unit_init();
	shared_data->flags |= ARC_READY;
	return 0;
}
DECLARE_DEVICE_INIT_CONFIG(quark_se_arc_0, "", quark_se_arc_init, NULL);
SYS_DEFINE_DEVICE(quark_se_arc_0, NULL, PRIMARY,
					CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
