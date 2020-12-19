/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PSCI_H_
#define ZEPHYR_INCLUDE_DRIVERS_PSCI_H_

/**
 * @file
 * @brief Public API for ARM PSCI
 */

#include <zephyr/types.h>
#include <arch/arm/arm-smccc.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PSCI version decoding (independent of PSCI version) */
#define PSCI_VERSION_MAJOR_SHIFT		16
#define PSCI_VERSION_MINOR_MASK			\
		((1U << PSCI_VERSION_MAJOR_SHIFT) - 1)
#define PSCI_VERSION_MAJOR_MASK			~PSCI_VERSION_MINOR_MASK

#define PSCI_VERSION_MAJOR(ver)			\
		(((ver) & PSCI_VERSION_MAJOR_MASK) >> PSCI_VERSION_MAJOR_SHIFT)
#define PSCI_VERSION_MINOR(ver)			\
		((ver) & PSCI_VERSION_MINOR_MASK)

/**
 * @brief ARM PSCI Driver API
 * @defgroup arm_psci ARM PSCI Driver API
 * @{
 */

typedef uint32_t (*psci_get_version_f)(const struct device *dev);

typedef int (*psci_cpu_off_f)(const struct device *dev, uint32_t state);

typedef int (*psci_cpu_on_f)(const struct device *dev, unsigned long cpuid,
			     unsigned long entry_point);

typedef int (*psci_affinity_info_f)(const struct device *dev,
				    unsigned long target_affinity,
				    unsigned long lowest_affinity_level);

__subsystem struct psci_driver_api {
	psci_get_version_f get_version;
	psci_cpu_off_f cpu_off;
	psci_cpu_on_f cpu_on;
	psci_affinity_info_f affinity_info;
};

/**
 * @brief Return the version of PSCI implemented
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return The PSCI version
 */
__syscall uint32_t psci_get_version(const struct device *dev);

static inline uint32_t z_impl_psci_get_version(const struct device *dev)
{
	const struct psci_driver_api *api =
		(const struct psci_driver_api *)dev->api;

	return api->get_version(dev);
}

/**
 * @brief Power down the calling core
 *
 * This call is intended foruse in hotplug. A core that is powered down by
 * CPU_OFF can only be powered up again in response to a CPU_ON
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param state Not used
 *
 * @return The call does not return when successful
 */
__syscall int psci_cpu_off(const struct device *dev, uint32_t state);

static inline int z_impl_psci_cpu_off(const struct device *dev, uint32_t state)
{
	const struct psci_driver_api *api =
		(const struct psci_driver_api *)dev->api;

	return api->cpu_off(dev, state);
}

/**
 * @brief Power up a core
 *
 * This call is used to power up cores that either have not yet been booted
 * into the calling supervisory software or have been previously powered down
 * with a CPU_OFF call
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param cpuid This parameter contains a copy of the affinity fields of the
 *              MPIDR register
 * @param entry_point Address at which the core must commence execution, when
 *                    it enters the return Non-secure Exception level.
 *
 * @return 0 on success, a negative errno otherwise
 */
__syscall int psci_cpu_on(const struct device *dev, unsigned long cpuid,
			  unsigned long entry_point);

static inline int z_impl_psci_cpu_on(const struct device *dev,
				     unsigned long cpuid,
				     unsigned long entry_point) {
	const struct psci_driver_api *api =
		(const struct psci_driver_api *)dev->api;

	return api->cpu_on(dev, cpuid, entry_point);
}

/**
 * @brief Enable the caller to request status of an affinity instance
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param target_affinity This parameter contains a copy of the affinity fields
 *                        of the MPIDR register
 * @param lowest_affinity_level Denotes the lowest affinity level field that is
 *                              valid in the target_affinity parameter
 *
 * @return 2 if the affinity instance is transitioning to an ON sate, 1 off, 0
 *         on, a negative errno otherwise
 */
__syscall int psci_affinity_info(const struct device *dev,
				 unsigned long target_affinity,
				 unsigned long lowest_affinity_level);

static inline int z_impl_psci_affinity_info(const struct device *dev,
					    unsigned long target_affinity,
					    unsigned long lowest_affinity_level)
{
	const struct psci_driver_api *api =
		(const struct psci_driver_api *)dev->api;

	return api->affinity_info(dev, target_affinity, lowest_affinity_level);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/psci.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PSCI_H_ */
