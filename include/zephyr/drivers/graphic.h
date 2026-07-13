/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header file for graphi driver API.
 * @ingroup graphic_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GRAPHIC_H
#define ZEPHYR_INCLUDE_DRIVERS_GRAPHIC_H

/**
 * @brief Graphic Driver APIs, for use by the @ref graphic_api.
 * @defgroup Graphic_interface Graphic Driver Interface
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/graphic/graphic.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief graphic_operation_type enum
 *
 * Supported graphic operation of a graphic device.
 */
enum graphic_operation_type {
	/** image copy (with optional conversion) */
	GRAPHIC_OP_MEMCPY_CONV = 1,
	/** image blit (with/without transparency) */
	GRAPHIC_OP_BLIT = 2,
	/** fill (with/without transparency) */
	GRAPHIC_OP_FILL = 3,
};

/**
 * @brief graphic operation structure
 *
 * Structure describing an operation
 */
struct graphic_operation {
	/** Status of the operation */
	enum graphic_operation_status status;
	/** Type of operation (only valid when status is different from UNALLOCATED) */
	enum graphic_operation_type type;
	/** Output information */
	struct graphic_output_area output;
	/** Semaphore used for handling sync */
	struct k_sem sem;

	union {
		struct {
			struct graphic_area foreground;
			struct graphic_area background;
		} blend;
		struct graphic_area input;
		struct graphic_filled_area fill;
	} u;
};

/**
 * @brief Context structure for all graphic drivers
 *
 * A context struct, first member of all graphic driver data structure,
 * used by the subsystem to perform some common check, as well as elements
 * which are common to all graphic drivers.
 */
struct graphic_context {
	/** Running status of the device */
	bool running;
	/** Mutex used to serialize operations */
	struct k_mutex lock;
	/** FIFO to hold submitted operations */
	struct k_fifo submitted;
	/** Pointer to the pool of operations */
	struct graphic_operation *pool;
	/** Size of the operation pool */
	size_t pool_size;
	/** Currently ongoing operation */
	struct graphic_operation *ongoing_op;
};

/**
 * @def_driverbackendgroup{Graphic,graphic_interface}
 * @{
 */

/**
 * @brief Callback API to process a graphic operation
 * See @a graphic_driver_process_operation() for argument description.
 */
typedef int (*graphic_api_process_operation_t)(const struct device *dev,
					       struct graphic_operation *op);

/**
 * @driver_ops{Graphic}
 */
__subsystem struct graphic_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief graphic_driver_process_operation
	 */
	graphic_api_process_operation_t process_operation;
};

/**
 * @}
 */

/**
 * @brief Initialize a graphic driver
 *
 * Perform the initialization of the graphic driver context
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pool	Pointer to the operation pool
 * @param pool_size	Size of the operation pool
 *
 * @retval 0 Operation ran successfully
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
static inline int graphic_driver_init(const struct device *dev, struct graphic_operation *pool,
				      size_t pool_size)
{
	struct graphic_context *ctx = dev->data;

	k_mutex_init(&ctx->lock);
	ctx->pool = pool;
	ctx->pool_size = pool_size;

	return 0;
}

/**
 * @brief Process graphic operation
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param op	Pointer to the graphic_operation structure
 *
 * @retval 0 Operation ran successfully
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
static inline int graphic_driver_process_operation(const struct device *dev,
						   struct graphic_operation *op)
{
	const struct graphic_driver_api *api = DEVICE_API_GET(graphic, dev);

	if (api->process_operation == NULL) {
		return -ENOSYS;
	}

	return api->process_operation(dev, op);
}

static inline void graphic_driver_operation_done(const struct device *dev,
						 enum graphic_operation_status status)
{
	struct graphic_context *ctx = dev->data;
	int ret;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ctx->ongoing_op->status = status;
	k_sem_give(&ctx->ongoing_op->sem);

	/* Try to get the next operation if applicable */
	ctx->ongoing_op = k_fifo_get(&ctx->submitted, K_NO_WAIT);
	if (ctx->ongoing_op == NULL) {
		ctx->running = false;
		ctx->ongoing_op = NULL;
		goto out;
	}

	ret = graphic_driver_process_operation(dev, ctx->ongoing_op);
	if (ret < 0) {
		ctx->running = false;
		ctx->ongoing_op = NULL;
		goto out;
	}

out:
	k_mutex_unlock(&ctx->lock);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GRAPHIC_H */
