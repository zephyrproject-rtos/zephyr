/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTEL_VTD_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTEL_VTD_H_

#include <zephyr/drivers/pcie/msi.h>

typedef int (*vtd_alloc_entries_f)(const struct device *dev,
				   uint8_t n_entries);

typedef uint32_t (*vtd_remap_msi_f)(const struct device *dev,
				    msi_vector_t *vector,
				    uint8_t n_vector);

typedef int (*vtd_remap_f)(const struct device *dev,
			   uint8_t irte_idx,
			   uint16_t vector,
			   uint32_t flags,
			   uint16_t src_id);

typedef int (*vtd_set_irte_vector_f)(const struct device *dev,
				     uint8_t irte_idx,
				     uint16_t vector);

typedef int (*vtd_get_irte_by_vector_f)(const struct device *dev,
					uint16_t vector);

typedef uint16_t (*vtd_get_irte_vector_f)(const struct device *dev,
					  uint8_t irte_idx);

typedef int (*vtd_set_irte_irq_f)(const struct device *dev,
				  uint8_t irte_idx,
				  unsigned int irq);

typedef int (*vtd_get_irte_by_irq_f)(const struct device *dev,
				     unsigned int irq);

typedef void (*vtd_set_irte_msi_f)(const struct device *dev,
				   uint8_t irte_idx,
				   bool msi);

typedef bool (*vtd_irte_is_msi_f)(const struct device *dev,
				  uint8_t irte_idx);

__subsystem struct vtd_driver_api {
	vtd_alloc_entries_f allocate_entries;
	vtd_remap_msi_f remap_msi;
	vtd_remap_f remap;
	vtd_set_irte_vector_f set_irte_vector;
	vtd_get_irte_by_vector_f get_irte_by_vector;
	vtd_get_irte_vector_f get_irte_vector;
	vtd_set_irte_irq_f set_irte_irq;
	vtd_get_irte_by_irq_f get_irte_by_irq;
	vtd_set_irte_msi_f set_irte_msi;
	vtd_irte_is_msi_f irte_is_msi;
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
 * @param vector A valid allocated MSI vector array
 * @param n_vector the size of the vector array
 *
 * @return The MSI Message Address value
 */
static inline uint32_t vtd_remap_msi(const struct device *dev,
				     msi_vector_t *vector,
				     uint8_t n_vector)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->remap_msi(dev, vector, n_vector);
}

/**
 * @brief Remap the given vector
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param irte_idx A previously allocated irte entry index number
 * @param vector An allocated interrupt vector
 * @param flags interrupt flags
 * @param src_id a valid source ID or USHRT_MAX if none
 *
 * @return 0 on success, a negative errno otherwise
 */
static inline int vtd_remap(const struct device *dev,
			    uint8_t irte_idx,
			    uint16_t vector,
			    uint32_t flags,
			    uint16_t src_id)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->remap(dev, irte_idx, vector, flags, src_id);
}

/**
 * @brief Set the vector on the allocated irte
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param irte_idx A previously allocated irte entry index number
 * @param vector An allocated interrupt vector
 *
 * @return 0, a negative errno otherwise
 */
static inline int vtd_set_irte_vector(const struct device *dev,
				      uint8_t irte_idx,
				      uint16_t vector)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->set_irte_vector(dev, irte_idx, vector);
}

/**
 * @brief Get the irte allocated for the given vector
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param vector An allocated interrupt vector
 *
 * @return 0 or positive value on success, a negative errno otherwise
 */
static inline int vtd_get_irte_by_vector(const struct device *dev,
					 uint16_t vector)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->get_irte_by_vector(dev, vector);
}

/**
 * @brief Get the vector given to the IRTE
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param irte_idx A previously allocated irte entry index number
 *
 * @return the vector set to this IRTE
 */
static inline uint16_t vtd_get_irte_vector(const struct device *dev,
					   uint8_t irte_idx)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->get_irte_vector(dev, irte_idx);
}

/**
 * @brief Set the irq on the allocated irte
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param irte_idx A previously allocated irte entry index number
 * @param irq A valid IRQ number
 *
 * @return 0, a negative errno otherwise
 */
static inline int vtd_set_irte_irq(const struct device *dev,
				   uint8_t irte_idx,
				   unsigned int irq)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->set_irte_irq(dev, irte_idx, irq);
}

/**
 * @brief Get the irte allocated for the given irq
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param irq A valid IRQ number
 *
 * @return 0 or positive value on success, a negative errno otherwise
 */
static inline int vtd_get_irte_by_irq(const struct device *dev,
				      unsigned int irq)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->get_irte_by_irq(dev, irq);
}

static inline void vtd_set_irte_msi(const struct device *dev,
				    uint8_t irte_idx,
				    bool msi)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->set_irte_msi(dev, irte_idx, msi);
}

static inline bool vtd_irte_is_msi(const struct device *dev,
				   uint8_t irte_idx)
{
	const struct vtd_driver_api *api =
		(const struct vtd_driver_api *)dev->api;

	return api->irte_is_msi(dev, irte_idx);
}


#endif /* ZEPHYR_INCLUDE_DRIVERS_INTEL_VTD_H_ */
