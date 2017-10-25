/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <ipm.h>

_SYSCALL_HANDLER(ipm_send, dev, wait, id, data, size)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_IPM);
	_SYSCALL_MEMORY_READ(data, size);
	return _impl_ipm_send((struct device *)dev, wait, id,
			      (const void *)data, size);
}

_SYSCALL_HANDLER1_SIMPLE(ipm_max_data_size_get, K_OBJ_DRIVER_IPM,
			 struct device *);

_SYSCALL_HANDLER1_SIMPLE(ipm_max_id_val_get, K_OBJ_DRIVER_IPM,
			 struct device *);

_SYSCALL_HANDLER(ipm_set_enabled, dev, enable)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_IPM);
	return _impl_ipm_set_enabled((struct device *)dev, enable);
}
