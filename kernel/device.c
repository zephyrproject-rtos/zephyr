/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <device.h>
#include <sys/atomic.h>
#include <syscall_handler.h>

extern struct device __device_init_start[];
extern struct device __device_PRE_KERNEL_1_start[];
extern struct device __device_PRE_KERNEL_2_start[];
extern struct device __device_POST_KERNEL_start[];
extern struct device __device_APPLICATION_start[];
extern struct device __device_init_end[];


#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
extern u32_t __device_busy_start[];
extern u32_t __device_busy_end[];
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
void z_sys_device_do_config_level(s32_t level)
{
	struct device *info;
	static struct device *config_levels[] = {
		__device_PRE_KERNEL_1_start,
		__device_PRE_KERNEL_2_start,
		__device_POST_KERNEL_start,
		__device_APPLICATION_start,
		/* End marker */
		__device_init_end,
	};

	for (info = config_levels[level]; info < config_levels[level+1];
								info++) {
		int retval;
		struct device_config *device_conf = info->config;

		retval = device_conf->init(info);
		if (retval != 0) {
			/* Initialization failed. Clear the API struct so that
			 * device_get_binding() will not succeed for it.
			 */
			info->driver_api = NULL;
		} else {
			z_object_init(info);
		}
	}
}

struct device *z_impl_device_get_binding(const char *name)
{
	struct device *info;

	/* Split the search into two loops: in the common scenario, where
	 * device names are stored in ROM (and are referenced by the user
	 * with CONFIG_* macros), only cheap pointer comparisons will be
	 * performed.  Reserve string comparisons for a fallback.
	 */
	for (info = __device_init_start; info != __device_init_end; info++) {
		if ((info->driver_api != NULL) &&
		    (info->config->name == name)) {
			return info;
		}
	}

	for (info = __device_init_start; info != __device_init_end; info++) {
		if (info->driver_api == NULL) {
			continue;
		}

		if (strcmp(name, info->config->name) == 0) {
			return info;
		}
	}

	return NULL;
}

#ifdef CONFIG_USERSPACE
static inline struct device *z_vrfy_device_get_binding(const char *name)
{
	char name_copy[Z_DEVICE_MAX_NAME_LEN];

	if (z_user_string_copy(name_copy, (char *)name, sizeof(name_copy))
	    != 0) {
		return 0;
	}

	return z_impl_device_get_binding(name_copy);
}
#include <syscalls/device_get_binding_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
int device_pm_control_nop(struct device *unused_device,
		       u32_t unused_ctrl_command,
		       void *unused_context,
		       device_pm_cb cb,
		       void *unused_arg)
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
		if (__device_busy_start[i] != 0U) {
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
#else
	ARG_UNUSED(busy_dev);
#endif
}

void device_busy_clear(struct device *busy_dev)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	atomic_clear_bit((atomic_t *) __device_busy_start,
				 (busy_dev - __device_init_start));
#else
	ARG_UNUSED(busy_dev);
#endif
}
