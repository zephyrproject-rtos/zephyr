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

#include <errno.h>
#include <string.h>
#include <device.h>
#include <misc/util.h>
#include <atomic.h>

extern struct device __device_init_start[];
extern struct device __device_PRE_KERNEL_1_start[];
extern struct device __device_PRE_KERNEL_2_start[];
extern struct device __device_POST_KERNEL_start[];
extern struct device __device_APPLICATION_start[];

/* Deprecated */
extern struct device __device_PRIMARY_start[];
extern struct device __device_SECONDARY_start[];
extern struct device __device_NANOKERNEL_start[];
extern struct device __device_MICROKERNEL_start[];

extern struct device __device_init_end[];

static struct device *config_levels[] = {
	__device_PRE_KERNEL_1_start,
	__device_PRE_KERNEL_2_start,
	__device_POST_KERNEL_start,
	__device_APPLICATION_start,

	/* Deprecated levels */
	__device_PRIMARY_start,
	__device_SECONDARY_start,
	__device_NANOKERNEL_start,
	__device_MICROKERNEL_start,

	/* End marker */
	__device_init_end,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
struct device_pm_ops device_pm_ops_nop = {device_pm_nop, device_pm_nop};
extern uint32_t __device_busy_start[];
extern uint32_t __device_busy_end[];
#define DEVICE_BUSY_SIZE (__device_busy_end - __device_busy_start)
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

struct device *device_get_binding(const char *name)
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

int device_pm_control_nop(struct device *unused_device,
		       uint32_t unused_ctrl_command, void *unused_context)
{
	return 0;
}

void device_list_get(struct device **device_list, int *device_count)
{

	*device_list = __device_init_start;
	*device_count = __device_init_end - __device_init_start;
}


int device_any_busy_check(void)
{
	int i = 0;

	for (i = 0; i < DEVICE_BUSY_SIZE; i++) {
		if (__device_busy_start[i] != 0) {
			return -EBUSY;
		}
	}
	return 0;
}

int device_busy_check(struct device *chk_dev)
{
	if (atomic_test_bit((const atomic_t *)__device_busy_start,
				 (chk_dev - __device_init_start))) {
		return -EBUSY;
	}
	return 0;
}

#endif

void device_busy_set(struct device *busy_dev)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	atomic_set_bit((atomic_t *) __device_busy_start,
				 (busy_dev - __device_init_start));
#endif
}

void device_busy_clear(struct device *busy_dev)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	atomic_clear_bit((atomic_t *) __device_busy_start,
				 (busy_dev - __device_init_start));
#endif
}
