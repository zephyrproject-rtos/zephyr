/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <hwinfo.h>

Z_SYSCALL_HANDLER(hwinfo_get_device_id, buffer, length) {

	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buffer, length));

	return _impl_hwinfo_get_device_id((u8_t *)buffer, (size_t)length);
}
