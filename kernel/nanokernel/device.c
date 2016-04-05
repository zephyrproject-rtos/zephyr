/*
 * Copyright (c) 2015-2016 Intel Corporation.
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

#include <string.h>
#include <device.h>
#include <misc/util.h>

extern struct device __device_init_start[];
extern struct device __device_PRIMARY_start[];
extern struct device __device_SECONDARY_start[];
extern struct device __device_NANOKERNEL_start[];
extern struct device __device_MICROKERNEL_start[];
extern struct device __device_APPLICATION_start[];
extern struct device __device_init_end[];

static struct device *config_levels[] = {
	__device_PRIMARY_start,
	__device_SECONDARY_start,
	__device_NANOKERNEL_start,
	__device_MICROKERNEL_start,
	__device_APPLICATION_start,
	__device_init_end,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
struct device_pm_ops device_pm_ops_nop = {device_pm_nop, device_pm_nop};
#endif

/**
 * @brief Execute all the device initialization functions at a given level
 *
 * @details Invokes the initialization routine for each device object
 * created by the DEVICE_INIT() macro using the specified level.
 * The linker script places the device objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */
void _sys_device_do_config_level(int level)
{
	struct device *info;

	for (info = config_levels[level]; info < config_levels[level+1]; info++) {
		struct device_config *device = info->config;

		device->init(info);
	}
}

/**
 * @brief Retrieve the device structure for a driver by name
 *
 * @details Device objects are created via the DEVICE_INIT() macro and
 * placed in memory by the linker. If a driver needs to bind to another driver
 * it can use this function to retrieve the device structure of the lower level
 * driver by the name the driver exposes to the system.
 *
 * @param name device name to search for.
 *
 * @return pointer to device structure; NULL if not found or cannot be used.
 */
struct device *device_get_binding(char *name)
{
	struct device *info;

	for (info = __device_init_start; info != __device_init_end; info++) {
		if (info->driver_api && !strcmp(name, info->config->name)) {
			return info;
		}
	}

	return NULL;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
int device_pm_nop(struct device *unused_device, int unused_policy)
{
	return 0;
}

void device_list_get(struct device **device_list, int *device_count)
{

	*device_list = __device_init_start;
	*device_count = __device_init_end - __device_init_start;
}
#endif
