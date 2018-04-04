/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <entropy.h>
#include <syscall_handler.h>

_SYSCALL_HANDLER(entropy_get_entropy, dev, buffer, len)
{
	_SYSCALL_DRIVER_ENTROPY(dev, get_entropy);
	_SYSCALL_MEMORY_WRITE(buffer, len);
	return _impl_entropy_get_entropy((struct device *)dev, (u8_t *)buffer,
					 len);
}
