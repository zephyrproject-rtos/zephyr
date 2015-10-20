/* i8259.c - Disable Intel 8259A PIC (Programmable Interrupt Controller) */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
 * This module disables the Intel 8259A PIC (Programmable Interrupt Controller)
 * to prevent it from generating spurious interrupts.
 */


#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <device.h>

#include <drivers/pic.h>
#include <board.h>

#define PIC_DISABLE 0xff /* Disable PIC command */

/**
 *
 * @brief Initialize the Intel 8259A PIC device driver
 *
 * This routine disables the 8259A PIC device to prevent it from
 * generating spurious interrupts.
 *
 * @return N/A
 */

int _i8259_init(struct device *unused)
{
	ARG_UNUSED(unused);
	sys_out8(PIC_DISABLE, PIC_PORT2(PIC_SLAVE_BASE_ADRS));
	sys_out8(PIC_DISABLE, PIC_PORT2(PIC_MASTER_BASE_ADRS));
	return 0;
}
