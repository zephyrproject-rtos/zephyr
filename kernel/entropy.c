/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <syscall_handler.h>
#include <entropy.h>

int _impl_entropy_get_entropy(struct device *dev,
			      u8_t *buffer,
			      u16_t length)
{
	const struct entropy_driver_api *api = dev->driver_api;

	__ASSERT(api->get_entropy != NULL,
		"Callback pointer should not be NULL");
	return api->get_entropy(dev, buffer, length);
}
