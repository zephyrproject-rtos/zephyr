/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INIT_H_
#define ZEPHYR_INCLUDE_INIT_H_

#include <toolchain.h>
#include <kernel.h>
#include <zephyr/types.h>

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

#ifdef CONFIG_SMP
#define _SYS_INIT_LEVEL_SMP		4
#endif

struct device;

/**
 * @brief Static init entry structure for each device driver or services
 *
 * @param init init function for the init entry which will take the dev
 * attribute as parameter. See below.
 * @param dev pointer to a device driver instance structure. Can be NULL
 * if the init entry is not used for a device driver but a service.
 */
struct init_entry {
	/** Initialization function for the init entry which will take
	 * the dev attribute as parameter. See below.
	 */
	int (*init)(const struct device *dev);
	/** Pointer to a device driver instance structure. Can be NULL
	 * if the init entry is not used for a device driver but a services.
	 */
	const struct device *dev;
};

void z_sys_init_run_level(int32_t _level);

/* A counter is used to avoid issues when two or more system devices
 * are declared in the same C file with the same init function.
 */
#define Z_SYS_NAME(_init_fn) _CONCAT(_CONCAT(sys_init_, _init_fn), __COUNTER__)

/**
 * @def Z_INIT_ENTRY_DEFINE
 *
 * @brief Create an init entry object and set it up for boot time initialization
 *
 * @details This macro defines an init entry object that will be automatically
 * configured by the kernel during system initialization. Note that
 * init entries will not be accessible from user mode. Also this macro should
 * not be used directly, use relevant macro such as SYS_INIT() or
 * DEVICE_DEFINE() instead.
 *
 * @param _entry_name Init entry name. It is the name this instance exposes to
 * the system.
 *
 * @param _init_fn Address to the init function of the entry.
 *
 * @param _device A device driver instance pointer or NULL
 *
 * @param _level The initialization level at which configuration
 * occurs.  See SYS_INIT().
 *
 * @param prio The initialization priority of the object, relative to
 * other objects of the same initialization level. See SYS_INIT().
 */
#define Z_INIT_ENTRY_DEFINE(_entry_name, _init_fn, _device, _level, _prio)	\
	static const Z_DECL_ALIGN(struct init_entry)			\
		_CONCAT(__init_, _entry_name) __used			\
	__attribute__((__section__(".init_" #_level STRINGIFY(_prio)))) = { \
		.init = (_init_fn),					\
		.dev = (_device),					\
	}

/**
 * @def SYS_INIT
 *
 * @ingroup device_model
 *
 * @brief Run an initialization function at boot at specified priority
 *
 * @details This macro lets you run a function at system boot.
 *
 * @param _init_fn Pointer to the boot function to run
 *
 * @param _level The initialization level at which configuration occurs.
 * Must be one of the following symbols, which are listed in the order
 * they are performed by the kernel:
 * \n
 * \li PRE_KERNEL_1: Used for initialization objects that have no dependencies,
 * such as those that rely solely on hardware present in the processor/SOC.
 * These objects cannot use any kernel services during configuration, since
 * they are not yet available.
 * \n
 * \li PRE_KERNEL_2: Used for initialization objects that rely on objects
 * initialized as part of the PRE_KERNEL_1 level. These objects cannot use any
 * kernel services during configuration, since they are not yet available.
 * \n
 * \li POST_KERNEL: Used for initialization objects that require kernel services
 * during configuration.
 * \n
 * \li POST_KERNEL_SMP: Used for initialization objects that require kernel
 * services during configuration after SMP initialization.
 * \n
 * \li APPLICATION: Used for application components (i.e. non-kernel components)
 * that need automatic configuration. These objects can use all services
 * provided by the kernel during configuration.
 *
 * @param _prio The initialization priority of the object, relative to
 * other objects of the same initialization level. Specified as an integer
 * value in the range 0 to 99; lower values indicate earlier initialization.
 * Must be a decimal integer literal without leading zeroes or sign (e.g. 32),
 * or an equivalent symbolic name (e.g. \#define MY_INIT_PRIO 32); symbolic
 * expressions are *not* permitted
 * (e.g. CONFIG_KERNEL_INIT_PRIORITY_DEFAULT + 5).
 */
#define SYS_INIT(_init_fn, _level, _prio)					\
	Z_INIT_ENTRY_DEFINE(Z_SYS_NAME(_init_fn), _init_fn, NULL, _level, _prio)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_INIT_H_ */
