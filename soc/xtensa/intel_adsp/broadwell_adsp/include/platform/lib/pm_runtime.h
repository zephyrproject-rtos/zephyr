/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * Author: Tomasz Lauda <tomasz.lauda@linux.intel.com>
 */

/**
 * \file platform/haswell/include/platform/lib/pm_runtime.h
 * \brief Runtime power management header file for Haswell
 * \author Tomasz Lauda <tomasz.lauda@linux.intel.com>
 */

#ifdef __SOF_LIB_PM_RUNTIME_H__

#ifndef __PLATFORM_LIB_PM_RUNTIME_H__
#define __PLATFORM_LIB_PM_RUNTIME_H__

#include <stdint.h>

struct pm_runtime_data;

/**
 * \brief Initializes platform specific runtime power management.
 * \param[in,out] prd Runtime power management data.
 */
static inline void platform_pm_runtime_init(struct pm_runtime_data *prd) { }

/**
 * \brief Retrieves platform specific power management resource.
 *
 * \param[in] context Type of power management context.
 * \param[in] index Index of the device.
 * \param[in] flags Flags, set of RPM_...
 */
static inline void platform_pm_runtime_get(uint32_t context, uint32_t index,
					   uint32_t flags) { }

/**
 * \brief Releases platform specific power management resource.
 *
 * \param[in] context Type of power management context.
 * \param[in] index Index of the device.
 * \param[in] flags Flags, set of RPM_...
 */
static inline void platform_pm_runtime_put(uint32_t context, uint32_t index,
					   uint32_t flags) { }

static inline void platform_pm_runtime_enable(uint32_t context,
					      uint32_t index) {}

static inline void platform_pm_runtime_disable(uint32_t context,
					       uint32_t index) {}

static inline bool platform_pm_runtime_is_active(uint32_t context,
						 uint32_t index)
{
	return false;
}

#endif /* __PLATFORM_LIB_PM_RUNTIME_H__ */

#else

#error "This file shouldn't be included from outside of sof/lib/pm_runtime.h"

#endif /* __SOF_LIB_PM_RUNTIME_H__ */
