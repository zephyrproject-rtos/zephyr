
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

#define PRE_KERNEL_CORE		0
#define PRE_KERNEL_EARLY	1
#define PRE_KERNEL_LATE		2
#define NANO_EARLY		3
#define NANO_LATE		4
#define MICRO_EARLY		5
#define MICRO_LATE		6
#define APP_EARLY		7
#define APP_LATE		8

/** @def __define_initconfig
 *
 *  @brief Define an init object
 *
 *  @details This macro declares an init object to be placed in a
 *  given init level section in the image. This macro should not be used
 *  directly.
 *
 *  @param cfg_name Name of the config object created with
 *  DECLARE_DEVICE_INIT_CONFIG() macro that will be referenced by
 *  init object.
 *
 *  @param id The init level id where the init object will be placed
 *  in the image.
 *
 *  @param data The pointer to the driver data for the driver instance.
 *  @sa DECLARE_DEVICE_INIT_CONFIG()
 */
#define __define_initconfig(cfg_name, id, data)			    \
	 static struct device (__initconfig_##cfg_name##id) __used  \
	__attribute__((__section__(".initconfig" #id ".init"))) = { \
		 .config = &(config_##cfg_name),\
		 .driver_data = data}
/*
 * There are four distinct init levels, pre_kernel, nano, micro
 * and app. Each init level a unique set of restrictions placed on the
 * component being initialized within the level.
 *   pre_kernel:
 *     At this level no kernel objects or services are available to
 *     the component. pre_kernel has three phases, core, early and
 *     late. The core phase is intended for components that rely
 *     solely on hardware present in the processor/SOC and do *not*
 *     rely on services from any other component in the system. The
 *     early phase can be used by components that do *not* need kernel
 *     services and may rely on components from the core phase. The
 *     late phase can be used by components that do *not* need kernel
 *     services and may rely on components from the core and early
 *     phases.
 *  nano:
 *     At this level nano kernel services are available to the
 *     component. All services provided by the components initialized
 *     in the pre_kernel are also available. The nano level has an
 *     early and late phase.  Components in the early phase may rely
 *     on the nano kernel and pre_kernel services.  Components in the
 *     late phase may rely on, nano kernel, pre_kernel services and
 *     nano_early services
 *  micro:
 *     At this level micro kernel, nano kernel and pre_kernel services
 *     are available to the component. The micor level has an
 *     early and late phase.  Components in the early phase may rely
 *     on micro kernel, nano kernel and pre_kernel services.
 *     Components in the late phase may rely on, micro kernel, nano
 *     kernel, pre_kernel services and  micro_early services
 *  app:
 *    The app level is not intended for core kernel components but for
 *    the application developer to add any components that they wish to
 *    have initialized automatically during kernel initialization.  The
 *    app level is executed as the final init stage in both nanokernel
 *    and microkernel configurations. The application component may
 *    rely any component configured into the system.
*/

/* Run on pre_kernel stack; no {micro,nano} kernel objects available */
#define pre_kernel_core_init(cfg, data)	 __define_initconfig(cfg, 0, data)
#define pre_kernel_early_init(cfg, data) __define_initconfig(cfg, 1, data)
#define pre_kernel_late_init(cfg, data)	 __define_initconfig(cfg, 2, data)

/* Run from nano kernel idle task; no micro kernel objects available */
#define nano_early_init(cfg, data)	__define_initconfig(cfg, 3, data)
#define nano_late_init(cfg, data)	__define_initconfig(cfg, 4, data)

/* Run from micro kernel idle task. */
#define micro_early_init(cfg, data)	__define_initconfig(cfg, 5, data)
#define micro_late_init(cfg, data)	__define_initconfig(cfg, 6, data)

/* Run in the idle task; In a nano kernel only system run after
 * nano_late_init(). In a micro kernel system after micro_late_init()
 */
#define app_early_init(cfg, data)	__define_initconfig(cfg, 7, data)
#define app_late_init(cfg, data)	__define_initconfig(cfg, 8, data)


#endif /* _INIT_H_ */
