/*
 * Copyright (c) 2020 Oticon A/S.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public PDM Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PDM_H_
#define ZEPHYR_INCLUDE_DRIVERS_PDM_H_

/**
 * @brief PDM Interface
 * @defgroup pdm_interface PDM Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @typedef pdm_data_handler_t
 * @brief User data handler for PDM samples
 *
 * @param data      Pointer to the PDM sample
 * @param data_size Size of PDM sample
 */
typedef void (*pdm_data_handler_t)(int16_t *data,
					uint16_t data_size);

struct pdm_config {
	/* Application data handler */
	pdm_data_handler_t	data_handler;
	/* Memory slab used for the released buffers */
	struct k_mem_slab	*mem_slab;
};

/**
 * @typedef pdm_configure_t
 * @brief API for configuring PDM device
 * See @a pdm_configure()
 */
typedef int (*pdm_configure_t)(struct device *dev,
				struct pdm_config *cfg);

/**
 * @typedef pdm_start_t
 * @brief API for starting PDM sampling
 * See @a pdm_start() for argument description
 */
typedef int (*pdm_start_t)(struct device *dev);

/**
 * @typedef pdm_stop_t
 * @brief API for stopping PDM sampling
 * See @a pdm_stop() for argument description
 */
typedef int (*pdm_stop_t)(struct device *dev);

/** @brief PDM driver API definition. */
__subsystem struct pdm_driver_api {
	pdm_configure_t		configure;
	pdm_start_t		start;
	pdm_stop_t		stop;
};

/**
 * @brief Function for configuring the PDM device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Pointer to the PDM configure struct.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pdm_configure(struct device *dev,
			    struct pdm_config *cfg);

static inline int z_impl_pdm_configure(struct device *dev,
					struct pdm_config *cfg)
{
	struct pdm_driver_api *api;

	api = (struct pdm_driver_api *)dev->driver_api;
	return api->configure(dev, cfg);
}

/**
 * @brief Function for starting PDM sampling.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pdm_start(struct device *dev);

static inline int z_impl_pdm_start(struct device *dev)
{
	struct pdm_driver_api *api;

	api = (struct pdm_driver_api *)dev->driver_api;
	return api->start(dev);
}

/**
 * @brief Function for stopping PDM sampling.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pdm_stop(struct device *dev);

static inline int z_impl_pdm_stop(struct device *dev)
{
	struct pdm_driver_api *api;

	api = (struct pdm_driver_api *)dev->driver_api;
	return api->stop(dev);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/pdm.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PDM_H_ */
