/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) timepoint runtime interface
 * implementation
 */
#include <xenlib_osal_platform.h>

xenlib_osal_timeout_t xenlib_osal_timepoint_timeout(xenlib_osal_timepoint_t tp)
{
	return sys_timepoint_timeout(tp);
}

bool xenlib_osal_timepoint_expired(xenlib_osal_timepoint_t tp)
{
	return sys_timepoint_expired(tp);
}

xenlib_osal_timepoint_t xenlib_osal_timepoint_calc(xenlib_osal_timeout_t timeout)
{
	return sys_timepoint_calc(timeout);
}
