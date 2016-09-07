
/*
 * Copyright (c) 2015 Intel Corporation.
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

#ifndef _INIT_H_
#define _INIT_H_

#include <device.h>
#include <toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * System initialization levels. The PRIMARY and SECONDARY levels are
 * executed in the kernel's initialization context, which uses the interrupt
 * stack. The remaining levels are executed in the kernel's main task
 * (i.e. the nanokernel's background task or the microkernel's idle task).
 */

#define _SYS_INIT_LEVEL_PRIMARY      0
#define _SYS_INIT_LEVEL_SECONDARY    1
#define _SYS_INIT_LEVEL_NANOKERNEL   2
#define _SYS_INIT_LEVEL_MICROKERNEL  3
#define _SYS_INIT_LEVEL_APPLICATION  4

/**
 * @def SYS_INIT
 *
 * @brief Run an initailization function at boot at specified priority
 *
 * @details This macro lets you run a function at system boot.
 *
 * @param init_fn Pointer to the boot function to run
 *
 * @param level The initialization level, See DEVICE_INIT for details.
 *
 * @param prio Priority within the selected initialization level. See
 * DEVICE_INIT for details.
 */
#define SYS_INIT(init_fn, level, prio) \
	DEVICE_INIT(sys_init_##init_fn, "", init_fn, NULL, NULL, level, prio)

/**
 * @def SYS_INIT_PM
 *
 * @brief Run an initailization function at boot at specified priority,
 * and define functions to run at suspend/resume.
 *
 * @copydetails SYS_INIT
 * @param device_pm_ops Pointer to power management functions.
 * @param drv_name Name of this system device
 */

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#define SYS_INIT_PM(drv_name, init_fn, device_pm_ops, level, prio) \
	DEVICE_INIT_PM(sys_init_##init_fn, drv_name, init_fn, device_pm_ops, \
		NULL, NULL, level, prio)
#else
#define SYS_INIT_PM(drv_name, init_fn, device_pm_ops, level, prio) \
	DEVICE_INIT(sys_init_##init_fn, "", init_fn, NULL, NULL, level, prio)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _INIT_H_ */
