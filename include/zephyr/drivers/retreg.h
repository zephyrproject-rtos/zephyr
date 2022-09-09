/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public API for Retention Registers drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RETREG_H_
#define ZEPHYR_INCLUDE_DRIVERS_RETREG_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Retention Registers Interface
 * @defgroup retreg_interface Retention Registers Interface
 * @ingroup io_interfaces
 * @{
 */

struct retreg_layout_group {
	size_t reg_count; /* count of reg sequence of the same size */
	size_t reg_size;  /* the size in bytes */
};

struct retreg_layout {
	size_t group_num;
	struct retreg_layout_group *groups;
};
/**
 * @}
 */

/**
 * @brief Retention Registers internal Interface
 * @defgroup retreg_internal_interface Retention Registers internal Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Retention Register read implementation handler type
 */
typedef int (*retreg_api_read)(const struct device *dev, uint32_t idx,
			      void *data,
			      size_t len);
/**
 * @brief Retention Register write implementation handler type
 */
typedef int (*retreg_api_write)(const struct device *dev, uint32_t idx,
			       const void *data, size_t len);


/**
 * Retention parameters. Contents of this structure suppose to be
 * filled in during the device initialization and stay constant
 * through a runtime.
 */
struct retreg_parameters {
	struct retreg_layout *retreg_layout;
};

/**
 * @brief Retrieve paramiters of the retention registers.
 *
 * @param dev    Retention registers device whose paramiters to retrieve.
 * @param params The pointer to the paramiters will be returned in
 *		 this argument.
 */
typedef const struct retreg_parameters *(*retreg_api_get_parameters)(const struct device *dev);


__subsystem struct retreg_driver_api {
	retreg_api_read read;
	retreg_api_write write;
	retreg_api_get_parameters get_parameters;
};

/**
 * @}
 */

/**
 * @addtogroup retreg_interface
 * @{
 */

/**
 *  @brief  Read data from retention register
 *
 *  @param  dev             : device
 *  @param  reg_idx         : index of the retention register
 *  @param  data            : Buffer to store read data
 *  @param  len             : Number of bytes to read.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int retreg_read(const struct device *dev, uint32_t reg_idx, void *data,
			 size_t len);

static inline int z_impl_retreg_read(const struct device *dev, uint32_t reg_idx, void *data,
			 size_t len)
{
	const struct retreg_driver_api *api =
		(const struct retreg_driver_api *)dev->api;

	return api->read(dev, reg_idx, data, len);
}

/**
 *  @brief  Write data to retention register
 *
 *  @param  dev             : device
 *  @param  reg_idx         : index of the retention register
 *  @param  data            : data to write
 *  @param  len             : Number of bytes to write.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int retreg_write(const struct device *dev, uint32_t reg_idx, const void *data,
			 size_t len);

static inline int z_impl_retreg_write(const struct device *dev, uint32_t reg_idx, const void *data,
			 size_t len)
{
	const struct retreg_driver_api  *api =
		(const struct retreg_driver_api *)dev->api;

	return api->write(dev, reg_idx, data, len);
}


/**
 * @brief Retrieve a retention registers layout.
 *
 * A retention registers device layout is a run-length encoded description of the
 * susbsequent registers size on the device.
 *
 * For retention devices which have uniform register sizes, this routine
 * returns an array of length 1, which specifies the page size and
 * number of pages in the memory.
 *
 * Layouts for retention devices with nonuniform registers sizes will be
 * returned as an array with multiple elements, each of which
 * describes a group of registers that all have the same size. In this
 * case, the sequence of array elements specifies the order in which
 * these groups occur on the device.
 *
 * @param dev         retention registers device whose layout to retrieve.
 *
 * @retval Pointer to the layout data. It might be NULL if layout is not
 *	   supported by the driver.
 */
__syscall struct retreg_layout *reteg_get_layout(const struct device *dev,
			    const struct retreg_layout **layout,
			    size_t *layout_size);

static inline const struct retreg_layout *z_impl_get_retreg_layout(const struct device *dev)
{
	const struct retreg_driver_api  *api =
		(const struct retreg_driver_api *)dev->api;

	const struct retreg_parameters *params;

	params = api->get_parameters(dev);
	return params->retreg_layout;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/retreg.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RETREG_H_ */
