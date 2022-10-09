/*
 * Copyright (c) 2022, tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/input.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_input_setup(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_INPUT(dev, setup));

	return z_impl_input_setup((const struct device *)dev);
}
#include <syscalls/input_setup_mrsh.c>

static inline int z_vrfy_input_release(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_INPUT(dev, release));

	return z_impl_input_release((const struct device *)dev);
}
#include <syscalls/input_release_mrsh.c>

static inline int z_vrfy_input_attr_get(const struct device *dev,
			enum input_attr_type type, union input_attr_data *data)
{
	Z_OOPS(Z_SYSCALL_DRIVER_INPUT(dev, attr_get));

	if (data) {
		union input_attr_data _data;
		int retval = 0;

		retval = z_impl_input_attr_get((const struct device *)dev, type, &_data);
		Z_OOPS(z_user_to_copy(data, &_data, sizeof(union input_attr_data)));

		return retval;
	}

	return z_impl_input_attr_get((const struct device *)dev, type, NULL);
}
#include <syscalls/input_attr_get_mrsh.c>

static inline int z_vrfy_input_attr_set(const struct device *dev,
			enum input_attr_type type, union input_attr_data *data)
{
	Z_OOPS(Z_SYSCALL_DRIVER_INPUT(dev, attr_set));

	if (data) {
		union input_attr_data _data;

		Z_OOPS(z_user_from_copy(&_data, data, sizeof(union input_attr_data)));
		return z_impl_input_attr_set((const struct device *)dev, type, &_data);
	}

	return z_impl_input_attr_set((const struct device *)dev, type, NULL);
}

#include <syscalls/input_attr_set_mrsh.c>

static inline int z_vrfy_input_event_read(const struct device *dev, struct input_event *event)
{
	struct input_event _event;
	int retval = 0;

	Z_OOPS(Z_SYSCALL_DRIVER_INPUT(dev, event_read));

	retval = z_impl_input_event_read((const struct device *)dev, &_event);
	Z_OOPS(z_user_to_copy(event, &_event, sizeof(struct input_event)));

	return retval;
}
#include <syscalls/input_event_read_mrsh.c>

static inline int z_vrfy_input_event_write(const struct device *dev, struct input_event *event)
{
	struct input_event _event;

	Z_OOPS(Z_SYSCALL_DRIVER_INPUT(dev, event_write));
	Z_OOPS(z_user_from_copy(&_event, event, sizeof(struct input_event)));

	return z_impl_input_event_write((const struct device *)dev, &_event);
}
#include <syscalls/input_event_write_mrsh.c>
