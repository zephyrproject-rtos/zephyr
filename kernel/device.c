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

static inline void device_pm_state_init(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
	*dev->pm = (struct pm_device){
		.usage = ATOMIC_INIT(0),
		.lock = Z_MUTEX_INITIALIZER(dev->pm->lock),
		.condvar = Z_CONDVAR_INITIALIZER(dev->pm->condvar),
	};
#endif /* CONFIG_PM_DEVICE */
}

/**
 * @brief Initialize state for all static devices.
 *
 * The state object is always zero-initialized, but this may not be
 * sufficient.
 */
void z_device_state_init(void)
{
	const struct device *dev = __device_start;

	while (dev < __device_end) {
		device_pm_state_init(dev);
		z_object_init(dev);
		++dev;
	}
}

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
		int rc = entry->init(dev);

		if (dev != NULL) {
			/* Mark device initialized.  If initialization
			 * failed, record the error condition.
			 */
			if (rc != 0) {
				if (rc < 0) {
					rc = -rc;
				}
				if (rc > UINT8_MAX) {
					rc = UINT8_MAX;
				}
				dev->state->init_res = rc;
			}
			dev->state->initialized = true;
		}
	}
}

const struct device *z_impl_device_get_binding(const char *name)
{
	const struct device *dev;

	/* A null string identifies no device.  So does an empty
	 * string.
	 */
	if ((name == NULL) || (name[0] == '\0')) {
		return NULL;
	}

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
		return NULL;
	}

	return z_impl_device_get_binding(name_copy);
}
#include <syscalls/device_get_binding_mrsh.c>

static inline int z_vrfy_device_usable_check(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ_INIT(dev, K_OBJ_ANY));

	return z_impl_device_usable_check(dev);
}
#include <syscalls/device_usable_check_mrsh.c>
#endif /* CONFIG_USERSPACE */

size_t z_device_get_all_static(struct device const **devices)
{
	*devices = __device_start;
	return __device_end - __device_start;
}

bool z_device_ready(const struct device *dev)
{
	/*
	 * if an invalid device pointer is passed as argument, this call
	 * reports the `device` as not ready for usage.
	 */
	if (dev == NULL) {
		return false;
	}

	return dev->state->initialized && (dev->state->init_res == 0U);
}

int device_required_foreach(const struct device *dev,
			  device_visitor_callback_t visitor_cb,
			  void *context)
{
	size_t handle_count = 0;
	const device_handle_t *handles =
		device_required_handles_get(dev, &handle_count);

	/* Iterate over fixed devices */
	for (size_t i = 0; i < handle_count; ++i) {
		device_handle_t dh = handles[i];
		const struct device *rdev = device_from_handle(dh);
		int rc = visitor_cb(rdev, context);

		if (rc < 0) {
			return rc;
		}
	}

	return handle_count;
}

#ifdef CONFIG_PM_DEVICE
int device_any_busy_check(void)
{
	const struct device *dev = __device_start;

	while (dev < __device_end) {
		if (atomic_test_bit(&dev->pm->atomic_flags,
				    PM_DEVICE_ATOMIC_FLAGS_BUSY_BIT)) {
			return -EBUSY;
		}
		++dev;
	}

	return 0;
}

int device_busy_check(const struct device *dev)
{
	if (atomic_test_bit(&dev->pm->atomic_flags,
			    PM_DEVICE_ATOMIC_FLAGS_BUSY_BIT)) {
		return -EBUSY;
	}
	return 0;
}

#endif

void device_busy_set(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
	atomic_set_bit(&dev->pm->atomic_flags,
		       PM_DEVICE_ATOMIC_FLAGS_BUSY_BIT);
#else
	ARG_UNUSED(dev);
#endif
}

void device_busy_clear(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
	atomic_clear_bit(&dev->pm->atomic_flags,
			 PM_DEVICE_ATOMIC_FLAGS_BUSY_BIT);
#else
	ARG_UNUSED(dev);
#endif
}
