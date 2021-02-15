/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <device.h>
#include <sys/atomic.h>
#include <syscall_handler.h>

extern const struct init_entry __init_start[];
extern const struct init_entry __init_PRE_KERNEL_1_start[];
extern const struct init_entry __init_PRE_KERNEL_2_start[];
extern const struct init_entry __init_POST_KERNEL_start[];
extern const struct init_entry __init_APPLICATION_start[];
extern const struct init_entry __init_end[];

#ifdef CONFIG_SMP
extern const struct init_entry __init_SMP_start[];
#endif

extern const struct device __device_start[];
extern const struct device __device_end[];

extern uint32_t __device_init_status_start[];

#ifdef CONFIG_PM_DEVICE
extern uint32_t __device_busy_start[];
extern uint32_t __device_busy_end[];
#define DEVICE_BUSY_SIZE (__device_busy_end - __device_busy_start)
#endif

/**
 * @brief Execute all the init entry initialization functions at a given level
 *
 * @details Invokes the initialization routine for each init entry object
 * created by the INIT_ENTRY_DEFINE() macro using the specified level.
 * The linker script places the init entry objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */
void z_sys_init_run_level(int32_t level)
{
	static const struct init_entry *levels[] = {
		__init_PRE_KERNEL_1_start,
		__init_PRE_KERNEL_2_start,
		__init_POST_KERNEL_start,
		__init_APPLICATION_start,
#ifdef CONFIG_SMP
		__init_SMP_start,
#endif
		/* End marker */
		__init_end,
	};
	const struct init_entry *entry;

	for (entry = levels[level]; entry < levels[level+1]; entry++) {
		const struct device *dev = entry->dev;

		if (dev != NULL) {
			z_object_init(dev);
		}

		if ((entry->init(dev) != 0) && (dev != NULL)) {
			/* Initialization failed.
			 * Set the init status bit so device is not declared ready.
			 */
			sys_bitfield_set_bit(
				(mem_addr_t) __device_init_status_start,
				(dev - __device_start));
		}
	}
}

const struct device *z_impl_device_get_binding(const char *name)
{
	const struct device *dev;

	/* Split the search into two loops: in the common scenario, where
	 * device names are stored in ROM (and are referenced by the user
	 * with CONFIG_* macros), only cheap pointer comparisons will be
	 * performed. Reserve string comparisons for a fallback.
	 */
	for (dev = __device_start; dev != __device_end; dev++) {
		if (z_device_ready(dev) && (dev->name == name)) {
			return dev;
		}
	}

	for (dev = __device_start; dev != __device_end; dev++) {
		if (z_device_ready(dev) && (strcmp(name, dev->name) == 0)) {
			return dev;
		}
	}

	return NULL;
}

#ifdef CONFIG_USERSPACE
static inline const struct device *z_vrfy_device_get_binding(const char *name)
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

size_t z_device_get_all_static(struct device const **devices)
{
	*devices = __device_start;
	return __device_end - __device_start;
}

bool z_device_ready(const struct device *dev)
{
	/* Set bit indicates device failed initialization */
	return !(sys_bitfield_test_bit((mem_addr_t)__device_init_status_start,
					(dev - __device_start)));
}

#ifdef CONFIG_PM_DEVICE
int device_pm_control_nop(const struct device *unused_device,
			  uint32_t unused_ctrl_command,
			  void *unused_context,
			  device_pm_cb cb,
			  void *unused_arg)
{
	return -ENOTSUP;
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

int device_busy_check(const struct device *chk_dev)
{
	if (atomic_test_bit((const atomic_t *)__device_busy_start,
			    (chk_dev - __device_start))) {
		return -EBUSY;
	}
	return 0;
}

#endif

void device_busy_set(const struct device *busy_dev)
{
#ifdef CONFIG_PM_DEVICE
	atomic_set_bit((atomic_t *) __device_busy_start,
		       (busy_dev - __device_start));
#else
	ARG_UNUSED(busy_dev);
#endif
}

void device_busy_clear(const struct device *busy_dev)
{
#ifdef CONFIG_PM_DEVICE
	atomic_clear_bit((atomic_t *) __device_busy_start,
			 (busy_dev - __device_start));
#else
	ARG_UNUSED(busy_dev);
#endif
}
