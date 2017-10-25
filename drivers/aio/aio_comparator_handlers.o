/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <aio_comparator.h>

_SYSCALL_HANDLER(aio_cmp_disable, dev, index)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AIO);
	return _impl_aio_cmp_disable((struct device *)dev, index);
}

_SYSCALL_HANDLER1_SIMPLE(aio_cmp_get_pending_int, K_OBJ_DRIVER_AIO,
			 struct device *);
