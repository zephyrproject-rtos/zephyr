
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


/** @def SYS_DEFINE_DEVICE
 *
 *  @brief Define device object
 *
 *  @details This macro defines a device object that is automatically
 *  configured by the kernel during system initialization.
 *
 *  @param name Device name.
 *
 *  @param data Pointer to the device's configuration data.
 *  @sa DECLARE_DEVICE_INIT_CONFIG()
 *
 *  @param level The initialization level at which configuration occurs.
 *  Must be one of the following symbols, which are listed in the order
 *  they are performed by the kernel:
 *
 *  PRIMARY: Used for devices that have no dependencies, such as those
 *  that rely solely on hardware present in the processor/SOC. These devices
 *  cannot use any kernel services during configuration, since they are not
 *  yet available.
 *
 *  SECONDARY: Used for devices that rely on the initialization of devices
 *  initialized as part of the PRIMARY level. These devices cannot use any
 *  kernel services during configuration, since they are not yet available.
 *
 *  NANOKERNEL: Used for devices that require nanokernel services during
 *  configuration.
 *
 *  MICROKERNEL: Used for devices that require microkernel services during
 *  configuration.
 *
 *  APPLICATION: Used for application components (i.e. non-kernel components)
 *  that need automatic configuration. These devices can use all services
 *  provided by the kernel during configuration.
 *
 *  @param priority The initialization priority of the device, relative to
 *  other devices of the same initialization level. Specified as an integer
 *  value in the range 0 to 99; lower values indicate earlier initialization.
 *  Must be expressed as a hard-coded decimal integer literal without leading
 *  zeroes (e.g. 32); symbolic expressions are @not permitted.
 */

#define SYS_DEFINE_DEVICE(name, data, level, priority)			    \
	 static struct device (__initconfig_##name) __used  \
	__attribute__((__section__(".init_" #level #priority))) = { \
		 .config = &(config_##name),\
		 .driver_data = data}

/* The following legacy APIs are provided for backwards compatibility */

#define pre_kernel_core_init(cfg, data)	\
		SYS_DEFINE_DEVICE(cfg, data, PRIMARY, 0)
#define pre_kernel_early_init(cfg, data) \
		SYS_DEFINE_DEVICE(cfg, data, SECONDARY, 0)
#define pre_kernel_late_init(cfg, data)	\
		SYS_DEFINE_DEVICE(cfg, data, SECONDARY, 50)
#define nano_early_init(cfg, data) \
		SYS_DEFINE_DEVICE(cfg, data, NANOKERNEL, 0)
#define nano_late_init(cfg, data) \
		SYS_DEFINE_DEVICE(cfg, data, NANOKERNEL, 50)
#define micro_early_init(cfg, data)	\
		SYS_DEFINE_DEVICE(cfg, data, MICROKERNEL, 0)
#define micro_late_init(cfg, data) \
		SYS_DEFINE_DEVICE(cfg, data, MICROKERNEL, 50)
#define app_early_init(cfg, data) \
		SYS_DEFINE_DEVICE(cfg, data, APPLICATION, 0)
#define app_late_init(cfg, data) \
		SYS_DEFINE_DEVICE(cfg, data, APPLICATION, 50)

#endif /* _INIT_H_ */
