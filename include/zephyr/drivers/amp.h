/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the Asynchronous Multiprocessing (AMP) driver API
 * @ingroup amp_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_AMP_H_

#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 * @defgroup amp_interface AMP
 * @version 0.0.1
 * @{
 */

/** @brief Core identification */
struct amp_core_identification {
#if defined(CONFIG_AMP_IDENTIFICATION_INCLUDES_CLUSTER_TYPE) || defined(__DOXYGEN__)
	const uint32_t cluster_type; /**< Cluster type */
#endif
#if defined(CONFIG_AMP_IDENTIFICATION_INCLUDES_CLUSTER_NUMBER) || defined(__DOXYGEN__)
	const uint32_t cluster_id; /**< Cluster number */
#endif
	const uint32_t core_id; /**< Core number */
};


/**
 * @brief Get amp_core_identification from a node_id
 *
 * TODO: Use Kconfig (or something similar) to check whether the cluster_type /
 * cluster_id needs to be set!
 */
#define AMP_DT_GET_CORE_IDENTIFICATION(node_id)			       \
	{\
		.core_id = DT_REG_ADDR(node_id),\
		.cluster_id = DT_REG_ADDR(DT_PARENT(node_id)),\
		.cluster_type = DT_REG_ADDR(DT_PARENT(DT_PARENT(node_id))),\
	}

/**
 * @brief Get memory mapping to load data into other cores memory
 */
struct amp_memory_mapping {
	/** Device address from the perspective of the target core */
	const uintptr_t target_device_address;

	/** Size of the data that should be written */
	const size_t target_area_size;

	/** Start of the memory region from the own virtual memory view */
	uintptr_t own_virtual_address_start;

	/** Start of the memory region on the view from the target code */
	uintptr_t target_device_area_start;

	/** Size of the mapped region */
	size_t mapped_region_size;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Internal typedefs for readability and the subsystem that shouldn't appear in
 * the documentation
 */
typedef ssize_t (*amp_get_core_option_size_t)(const struct device *dev,
			       const struct amp_core_identification *core_identification);
typedef int (*amp_prepare_core_t)(const struct device *dev,
				  const struct amp_core_identification *core_identification,
				  const void *core_options);
typedef int (*amp_start_core_t)(const struct device *dev,
				const struct amp_core_identification *core_identification);
typedef int (*amp_stop_core_t)(const struct device *dev,
			       const struct amp_core_identification *core_identification);

typedef int (*amp_get_virtual_address_t)(const struct device *dev,
					 const struct amp_core_identification *core_identification,
					 struct amp_memory_mapping *mapping);

typedef void *(*amp_get_dt_core_config_t)(const struct device *dev,
					 const struct amp_core_identification *core_identification);

__subsystem struct amp_driver_api {
	amp_get_core_option_size_t amp_get_core_option_size;
	amp_prepare_core_t amp_prepare_core;
	amp_start_core_t amp_start_core;
	amp_stop_core_t amp_stop_core;
	amp_get_dt_core_config_t amp_get_dt_core_config;

	amp_get_virtual_address_t amp_get_virtual_address;
};

/** @endcond */

/**
 * @brief Get the size of core_options parameter used in amp_prepare_core.
 *
 * Get the required size in bytes for the core_options for a given core so a
 * copy from userspace can be done in the syscall helper, if necessary, to avoid
 * TOCTOU vulnerabilities. The size should be limited since the data will be
 * stored on the stack.
 *
 * @param dev Root device node for AMP
 * @param core_identification Core for which the option size should be returned
 *
 * @retval number of bytes in size or 0, if not used
 */
__syscall ssize_t amp_get_core_option_size(const struct device *dev,
			       const struct amp_core_identification *core_identification);

static inline ssize_t z_impl_amp_get_core_option_size(const struct device *dev,
					  const struct amp_core_identification *core_identification)
{
	const struct amp_driver_api *api = (const struct amp_driver_api *)dev->api;
	if (api->amp_get_core_option_size) {
		return api->amp_get_core_option_size(dev, core_identification);
	}

	return 0;
}

/**
 * @brief Configure a core and make memory loadable
 *
 * This applies configuration to a core (or saves it in the driver, if it is
 * needed later) and puts there core into a state where it doesn't execute code
 * but it's possible to load data into core specific memory (usually TCM).
 *
 * @param dev Root device node for AMP
 * @param core_identification Core which should be configured and made loadable
 * @param core_options Driver and Core specific options for the core
 *
 * @retval 0 if successful (or not needed)
 * @retval -ENODEV core_identification refers to a core that doesn't exist (or is disabled)
 * @retval -EINVAL invalid core_options configuration
 * @retval -EIO generic error while trying to configure other core
 */
__syscall int amp_prepare_core(const struct device *dev,
			       const struct amp_core_identification *core_identification,
			       const void *core_options);

static inline int z_impl_amp_prepare_core(const struct device *dev,
					  const struct amp_core_identification *core_identification,
					  const void *core_options)
{
	const struct amp_driver_api *api = (const struct amp_driver_api *)dev->api;

	if (api->amp_prepare_core) {
		return api->amp_prepare_core(dev, core_identification, core_options);
	}

	/*
	 * Some cores might not need preperation (they have no configuration options
	 * and are always loadable). For these no -ENOSYS should be returned due to
	 * not implementing it but instead show that there were no errors
	 */
	return 0;
}

/**
 * @brief Start executing code on another core
 *
 * Start executing code on a core that is configured and has code loaded
 *
 * @param dev Root device node for AMP
 * @param core_identification Core which should start executing code
 *
 * @retval 0 if successful (or not needed)
 * @retval -ENODEV core_identification refers to a core that doesn't exist (or is disabled)
 * @retval -EIO generic error while trying to start the core
 */
__syscall int amp_start_core(const struct device *dev,
			     const struct amp_core_identification *core_identification);

static inline int z_impl_amp_start_core(const struct device *dev,
					const struct amp_core_identification *core_identification)
{
	const struct amp_driver_api *api = (const struct amp_driver_api *)dev->api;

	if (api->amp_start_core) {
		return api->amp_start_core(dev, core_identification);
	}

	return -ENOSYS;
}

/**
 * @brief Stop executing code on another core
 *
 * @param dev Root device node for AMP
 * @param core_identification Core which should start executing code
 *
 * @retval 0 if successful (or not needed)
 * @retval -ENODEV core_identification refers to a core that doesn't exist (or is disabled)
 * @retval -EIO generic error while trying to stop the core
 */
__syscall int amp_stop_core(const struct device *dev,
			    const struct amp_core_identification *core_identification);

static inline int z_impl_amp_stop_core(const struct device *dev,
				       const struct amp_core_identification *core_identification)
{
	const struct amp_driver_api *api = (const struct amp_driver_api *)dev->api;

	if (api->amp_stop_core) {
		return api->amp_stop_core(dev, core_identification);
	}

	return -ENOSYS;
}

/**
 * @brief Get the core_options specified in the devicetree for a given core
 *
 * Get the core_options specified in the devicetree for a given CPU node to be
 * used in core_start.
 *
 * @param dev Root device node for AMP
 * @param core_identification Core for which the options should be returned
 *
 * @retval Pointer ot the core options or NULL
 */
__syscall void *amp_get_dt_core_config(const struct device *dev,
			       const struct amp_core_identification *core_identification);

static inline void *z_impl_amp_get_dt_core_config(const struct device *dev,
					  const struct amp_core_identification *core_identification)
{
	const struct amp_driver_api *api = (const struct amp_driver_api *)dev->api;
	if (api->amp_get_dt_core_config) {
		return api->amp_get_dt_core_config(dev, core_identification);
	}

	return 0;
}

/**
 * @brief Get virtual address mapping
 *
 * @param dev Root device node for AMP
 * @param core_identification Core which should start executing code
 * @param[in,out] mapping memory mapping. Refer to amp_memory_mapping for more info
 *
 * @see amp_memory_mapping
 *
 * @retval 0 if successful (or not needed)
 * @retval -ENODEV core_identification refers to a core that doesn't exist (or is disabled)
 * @retval -EFAULT requested area is not mappable
 *
 * TODO: It would probably be useful to have more/other return codes
 */
__syscall int amp_get_virtual_address(const struct device *dev,
				      const struct amp_core_identification *core_identification,
				      struct amp_memory_mapping *mapping);

static inline int
z_impl_amp_get_virtual_address(const struct device *dev,
			       const struct amp_core_identification *core_identification,
			       struct amp_memory_mapping *mapping)
{
	const struct amp_driver_api *api = (const struct amp_driver_api *)dev->api;

	/*
	 * TODO: Handle fully linear memory so that no implementation is needed for
	 * those SoCs
	 */
	const int ret = api->amp_get_virtual_address(dev, core_identification, mapping);

	return ret;
}

#include <zephyr/syscalls/amp.h>

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_AMP_H_ */
