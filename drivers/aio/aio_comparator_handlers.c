/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <aio_comparator.h>

_SYSCALL_HANDLER(aio_cmp_disable, dev, index)
{
	_SYSCALL_DRIVER_AIO_CMP(dev, disable);
	return _impl_aio_cmp_disable((struct device *)dev, index);
}

_SYSCALL_HANDLER(aio_cmp_get_pending_int, dev)
{
	_SYSCALL_DRIVER_AIO_CMP(dev, get_pending_int);
	return _impl_aio_get_pending_int((struct device *)dev, index);
}
