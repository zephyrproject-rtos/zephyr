/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * Author: Tomasz Lauda <tomasz.lauda@linux.intel.com>
 */

/**
 * \file platform/apollolake/include/platform/pm_runtime.h
 * \brief Runtime power management header file for Apollolake
 * \author Tomasz Lauda <tomasz.lauda@linux.intel.com>
 */

#ifndef __INCLUDE_PLATFORM_PM_RUNTIME__
#define __INCLUDE_PLATFORM_PM_RUNTIME__

#include <sof/pm_runtime.h>

/** \brief Platform specific runtime power management data. */
struct platform_pm_runtime_data {
	/* TBD */
};

/**
 * \brief Initializes platform specific runtime power management.
 *
 * \param[in,out] prd Runtime power management data.
 */
void platform_pm_runtime_init(struct pm_runtime_data *prd);

/**
 * \brief Retrieves platform specific power management resource.
 *
 * \param[in] context Type of power management context.
 * \param[in] index Index of the device.
 * \param[in] flags Flags, set of RPM_...
 */
void platform_pm_runtime_get(enum pm_runtime_context context, uint32_t index,
			     uint32_t flags);

/**
 * \brief Releases platform specific power management resource.
 *
 * \param[in] context Type of power management context.
 * \param[in] index Index of the device.
 * \param[in] flags Flags, set of RPM_...
 */
void platform_pm_runtime_put(enum pm_runtime_context context, uint32_t index,
			     uint32_t flags);


/**
 * \brief Power gates platform specific hardware resources.
 * \param[in] context Type of power management context.
 */
void platform_pm_runtime_power_off(void);

#endif /* __INCLUDE_PLATFORM_PM_RUNTIME__ */
