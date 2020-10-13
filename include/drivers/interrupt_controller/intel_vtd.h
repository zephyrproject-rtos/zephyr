/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTEL_VTD_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTEL_VTD_H_

#include <drivers/pcie/msi.h>

typedef int (*vtd_alloc_entries_f)(const struct device *dev,
				   uint8_t n_entries);

typedef uint32_t (*vtd_remap_msi_f)(const struct device *dev,
				    msi_vector_t *vector);

typedef int (*vtd_remap_f)(const struct device *dev,
			   msi_vector_t *vector);

struct vtd_driver_api {
	vtd_alloc_entries_f allocate_entries;
	vtd_remap_msi_f remap_msi;
	vtd_remap_f remap;
};

/**
 * @brief Allocate contiguous IRTEs
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param n_entries How many IRTE to allocate
 *
 * Note: It will try to allocate all, or it will fail.
 *
 * @return The first allocated IRTE index, or -EBUSY on failure
 */
static inline int vtd_allocate_entries(const struct device *dev,
				       uint8_t n_entries)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->allocate_entries(dev, n_entries);
}

/**
 * @brief Generate the MSI Message Address data for the given vector
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param vector A valid allocated MSI vector
 *
 * @return The MSI Message Address value
 */
static inline uint32_t vtd_remap_msi(const struct device *dev,
				     msi_vector_t *vector)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->remap_msi(dev, vector);
}

/**
 * @brief Remap the given vector
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param vector A valid allocated MSI vector
 *
 * @return 0 on success, a negative errno otherwise
 */
static inline int vtd_remap(const struct device *dev,
			    msi_vector_t *vector)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->remap(dev, vector);
}


#endif /* ZEPHYR_INCLUDE_DRIVERS_INTEL_VTD_H_ */
