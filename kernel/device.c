/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/kobject.h>
#include <zephyr/syscall_handler.h>

extern const struct device __device_start[];
extern const struct device __device_end[];


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
		z_object_init(dev);
		++dev;
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
		if (z_device_is_ready(dev) && (dev->name == name)) {
			return dev;
		}
	}

	for (dev = __device_start; dev != __device_end; dev++) {
		if (z_device_is_ready(dev) && (strcmp(name, dev->name) == 0)) {
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

static inline bool z_vrfy_device_is_ready(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ_INIT(dev, K_OBJ_ANY));

	return z_impl_device_is_ready(dev);
}
#include <syscalls/device_is_ready_mrsh.c>
#endif /* CONFIG_USERSPACE */

size_t z_device_get_all_static(struct device const **devices)
{
	*devices = __device_start;
	return __device_end - __device_start;
}

bool z_device_is_ready(const struct device *dev)
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

static int device_visitor(const device_handle_t *handles,
			   size_t handle_count,
			   device_visitor_callback_t visitor_cb,
			   void *context)
{
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

int device_required_foreach(const struct device *dev,
			    device_visitor_callback_t visitor_cb,
			    void *context)
{
	size_t handle_count = 0;
	const device_handle_t *handles = device_required_handles_get(dev, &handle_count);

	return device_visitor(handles, handle_count, visitor_cb, context);
}

int device_supported_foreach(const struct device *dev,
			     device_visitor_callback_t visitor_cb,
			     void *context)
{
	size_t handle_count = 0;
	const device_handle_t *handles = device_supported_handles_get(dev, &handle_count);

	return device_visitor(handles, handle_count, visitor_cb, context);
}
