/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <random/rand32.h>
#include <syscall_handler.h>

Z_SYSCALL_HANDLER(sys_rand32_get)
{
	return z_impl_sys_rand32_get();
}
