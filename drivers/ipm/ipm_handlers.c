/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <ipm.h>

_SYSCALL_HANDLER(ipm_send, dev, wait, id, data, size)
{
	_SYSCALL_DRIVER_IPM(dev, send);
	_SYSCALL_MEMORY_READ(data, size);
	return _impl_ipm_send((struct device *)dev, wait, id,
			      (const void *)data, size);
}

_SYSCALL_HANDLER(ipm_max_data_size_get, dev)
{
	_SYSCALL_DRIVER_IPM(dev, max_data_size_get);
	return _impl_max_data_size_get((struct device *)dev);
}

_SYSCALL_HANDLER(ipm_max_id_val_get, dev)
{
	_SYSCALL_DRIVER_IPM(dev, max_id_val_get);
	return _impl_max_id_val_get((struct device *)dev);
}

_SYSCALL_HANDLER(ipm_set_enabled, dev, enable)
{
	_SYSCALL_DRIVER_IPM(dev, set_enabled);
	return _impl_ipm_set_enabled((struct device *)dev, enable);
}
