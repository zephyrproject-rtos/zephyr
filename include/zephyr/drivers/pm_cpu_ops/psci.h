/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM PSCI specific APIs for CPU power management.
 * @ingroup power_management_cpu_api
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PM_CPU_OPS_PSCI_H_
#define ZEPHYR_INCLUDE_DRIVERS_PM_CPU_OPS_PSCI_H_

#include <zephyr/types.h>
#include <zephyr/arch/arm64/arm-smccc.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PSCI version decoding (independent of PSCI version) */

/** @cond INTERNAL_HIDDEN */
#define PSCI_VERSION_MAJOR_SHIFT		16
#define PSCI_VERSION_MINOR_MASK			\
		((1U << PSCI_VERSION_MAJOR_SHIFT) - 1)
#define PSCI_VERSION_MAJOR_MASK			~PSCI_VERSION_MINOR_MASK
/** @endcond */

/**
 * @brief Extract the major version field from a PSCI version value
 *
 * @param ver PSCI version value
 */
#define PSCI_VERSION_MAJOR(ver)			\
		(((ver) & PSCI_VERSION_MAJOR_MASK) >> PSCI_VERSION_MAJOR_SHIFT)
/**
 * @brief Extract the minor version field from a PSCI version value
 *
 * @param ver PSCI version value
 */
#define PSCI_VERSION_MINOR(ver)			\
		((ver) & PSCI_VERSION_MINOR_MASK)

/**
 * @brief Get the PSCI firmware version
 *
 * Returns the version of the detected PSCI firmware, with the major version
 * in the upper 16 bits and the minor version in the lower 16 bits. Use
 * PSCI_VERSION_MAJOR() and PSCI_VERSION_MINOR() to decode the fields.
 *
 * @return PSCI firmware version
 */
uint32_t psci_version(void);

/**
 * @brief Function to call PSCI CPU_SUSPEND
 *
 * This function is API for CPU_SUSPEND PSCI SMC call.
 *
 * @param state CPU Power state
 * @param entry_point The address entry when returning from suspend
 *
 * @retval 0 if enter suspend successfully, others for failure
 */

int psci_cpu_suspend(uint32_t state, uintptr_t entry_point);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PM_CPU_OPS_PSCI_H_ */
