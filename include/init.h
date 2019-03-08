
/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INIT_H_
#define ZEPHYR_INCLUDE_INIT_H_

#include <device.h>
#include <toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * System initialization levels. The PRE_KERNEL_1 and PRE_KERNEL_2 levels are
 * executed in the kernel's initialization context, which uses the interrupt
 * stack. The remaining levels are executed in the kernel's main task.
 */

#define _SYS_INIT_LEVEL_PRE_KERNEL_1	0
#define _SYS_INIT_LEVEL_PRE_KERNEL_2	1
#define _SYS_INIT_LEVEL_POST_KERNEL	2
#define _SYS_INIT_LEVEL_APPLICATION	3


/* A counter is used to avoid issues when two or more system devices
 * are declared in the same C file with the same init function.
 */
#define Z_SYS_NAME(init_fn) _CONCAT(_CONCAT(sys_init_, init_fn), __COUNTER__)

/**
 * @def SYS_INIT
 *
 * @brief Run an initialization function at boot at specified priority
 *
 * @details This macro lets you run a function at system boot.
 *
 * @param init_fn Pointer to the boot function to run
 *
 * @param level The initialization level, See DEVICE_AND_API_INIT for details.
 *
 * @param prio Priority within the selected initialization level. See
 * DEVICE_AND_API_INIT for details.
 */
#define SYS_INIT(init_fn, level, prio) \
	DEVICE_AND_API_INIT(Z_SYS_NAME(init_fn), "", init_fn, NULL, NULL, level,\
	prio, NULL)

/**
 * @def SYS_DEVICE_DEFINE
 *
 * @brief Run an initialization function at boot at specified priority,
 * and define device PM control function.
 *
 * @details This macro lets you run a function at system boot.
 *
 * @param drv_name Name of this system device
 * @param init_fn Pointer to the boot function to run
 * @param pm_control_fn Pointer to device_pm_control function.
 *                      Can be empty function (device_pm_control_nop) if not
 *                      implemented.
 * @param level The initialization level, See DEVICE_INIT for details.
 * @param prio Priority within the selected initialization level. See
 * 	       DEVICE_INIT for details.
 */
#define SYS_DEVICE_DEFINE(drv_name, init_fn, pm_control_fn, level, prio) \
	DEVICE_DEFINE(Z_SYS_NAME(init_fn), drv_name, init_fn, pm_control_fn, \
		      NULL, NULL, level, prio, NULL)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_INIT_H_ */
