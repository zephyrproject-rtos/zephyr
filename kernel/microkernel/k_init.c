/* k_init.c */

/*
 * Copyright (c) 1997-2010, 2012-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <micro_private.h>
#include "nanokernel.h"
#include <nano_private.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>
#include <device.h>
#include <init.h>

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
#include <arch/cpu.h>
#endif

extern void _k_init_node(void);     /* defined by sysgen */

char __noinit __stack _k_server_stack[CONFIG_MICROKERNEL_SERVER_STACK_SIZE];

#ifdef CONFIG_TASK_DEBUG
int _k_debug_halt = 0;
#endif

#ifdef CONFIG_INIT_STACKS
static uint32_t _k_server_command_stack_storage
						[CONFIG_COMMAND_STACK_SIZE] =
	{ [0 ... CONFIG_COMMAND_STACK_SIZE - 1] = 0xAAAAAAAA };
#else
static uint32_t __noinit _k_server_command_stack_storage
						[CONFIG_COMMAND_STACK_SIZE];
#endif

struct nano_stack _k_command_stack = {NULL,
									  _k_server_command_stack_storage,
									  _k_server_command_stack_storage};


extern void K_swapper(int i1, int i2);

void _k_kernel_init(void)
{
#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/*
	 * record timestamp for microkernel's main() function
	 * (an approximation, as kernel_init() is the first thing done by
	 * main())
	 */
	extern uint64_t __main_tsc;

	__main_tsc = _NanoTscRead();
#endif

	/*
	 * Note: most variables & data structure are globally initialized in
	 * kernel_main.c
	 */
	_k_init_node();

	task_fiber_start(_k_server_stack,
			   CONFIG_MICROKERNEL_SERVER_STACK_SIZE,
			   K_swapper,
			   0,
			   0,
			   CONFIG_MICROKERNEL_SERVER_PRIORITY,
			   0);

	_sys_device_do_config_level(MICRO_EARLY);
	_sys_device_do_config_level(MICRO_LATE);
	_sys_device_do_config_level(APP_EARLY);
	_sys_device_do_config_level(APP_LATE);


#ifdef CONFIG_WORKLOAD_MONITOR
	_k_workload_monitor_calibrate();
#endif
}
