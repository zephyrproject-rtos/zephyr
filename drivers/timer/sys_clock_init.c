/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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
 * @brief Initialize system clock driver
 *
 * Initializing the timer driver is done in this module to reduce code
 * duplication.  Although both nanokernel and microkernel systems initialize
 * the timer driver at the same point, the two systems differ in when the system
 * can begin to process system clock ticks.  A nanokernel system can process
 * system clock ticks once the driver has initialized.  However, in a
 * microkernel system all system clock ticks are deferred (and stored on the
 * kernel server command stack) until the kernel server fiber starts and begins
 * processing any queued ticks.
 */

#include <nanokernel.h>
#include <init.h>
#include <drivers/system_timer.h>

SYS_DEVICE_DEFINE("sys_clock", _sys_clock_driver_init,
		  sys_clock_device_ctrl, POST_KERNEL,
		  CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
