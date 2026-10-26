/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) memory/data barrier
 * runtime interface implementation
 *
 */
#include <xenlib_osal_platform.h>

void xenlib_osal_dmem_fence_full(void)
{
	z_barrier_dmem_fence_full();
}

void xenlib_osal_dsync_fence_full(void)
{
	z_barrier_dsync_fence_full();
}
