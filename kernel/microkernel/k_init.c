/* k_init.c */

/*
 * Copyright (c) 1997-2010, 2012-2014 Wind River Systems, Inc.
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

#include <microkernel.h>
#include <micro_private.h>
#include <nano_private.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>
#include <device.h>
#include <init.h>

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
#include <arch/cpu.h>
#endif

extern void _k_init_dynamic(void);     /* defined by sysgen */

char __noinit __stack _k_server_stack[CONFIG_MICROKERNEL_SERVER_STACK_SIZE];

#ifdef CONFIG_TASK_DEBUG
int _k_debug_halt;
#endif

#ifdef CONFIG_INIT_STACKS
static uint32_t _k_server_command_stack_storage
						[CONFIG_COMMAND_STACK_SIZE] = {
	[0 ... CONFIG_COMMAND_STACK_SIZE - 1] = 0xAAAAAAAA };
#else
static uint32_t __noinit _k_server_command_stack_storage
						[CONFIG_COMMAND_STACK_SIZE];
#endif

struct nano_stack _k_command_stack = {NULL,
									  _k_server_command_stack_storage,
									  _k_server_command_stack_storage};


extern void _k_server(int unused1, int unused2);
extern int _k_kernel_idle(void);

/**
 *
 * @brief Mainline for microkernel's idle task
 *
 * This routine completes kernel initialization and starts any application
 * tasks in the EXE task group. From then on it takes care of doing idle
 * processing whenever there is no other work for the kernel to do.
 *
 * @return N/A
 */
void _main(void)
{
	_sys_device_do_config_level(_SYS_INIT_LEVEL_NANOKERNEL);

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/*
	 * record timestamp for microkernel's _main() function
	 */
	extern uint64_t __main_tsc;

	__main_tsc = _NanoTscRead();
#endif

	/*
	 * Most variables and data structure are statically initialized in
	 * kernel_main.c: this only initializes what must be dynamically
	 * initialized at runtime.
	 */
	_k_init_dynamic();

	task_fiber_start(_k_server_stack,
			   CONFIG_MICROKERNEL_SERVER_STACK_SIZE,
			   _k_server,
			   0,
			   0,
			   CONFIG_MICROKERNEL_SERVER_PRIORITY,
			   0);

	_sys_device_do_config_level(_SYS_INIT_LEVEL_MICROKERNEL);
	_sys_device_do_config_level(_SYS_INIT_LEVEL_APPLICATION);


#ifdef CONFIG_WORKLOAD_MONITOR
	_k_workload_monitor_calibrate();
#endif

	task_group_start(EXE_GROUP);

	_k_kernel_idle();
}
