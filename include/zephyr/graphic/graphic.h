/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for graphic
 * @ingroup graphic_api
 */

#ifndef ZEPHYR_INCLUDE_GRAPHIC_GRAPHIC_H
#define ZEPHYR_INCLUDE_GRAPHIC_GRAPHIC_H

/**
 * @brief Graphic Public APIs for use by applications.
 * @defgroup graphic_api Graphic Subsystem API
 * @since 4.5
 * @version 0.1.0
 * @ingroup multimedia
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/graphic/types.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate and prepare a MEMCPY/CONV operation
 *
 * Allocate a slot for a MEMCPY or CONV operation. Depending on the input / output
 * formats, data will be either copied or converted. At that time, the given parameters
 * are also checked.
 * It returns a > 0 value in case of success with the operation slot value.
 *
 * @param dev	Pointer to the graphic device file
 * @param output Pointer to the graphic_output_area structure
 * @param input	Pointer to the graphic_area structure
 *
 * @retval > 0 Operation ran successfully. Operation ID.
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
int graphic_alloc_memcpy_conv_operation(const struct device *dev,
					struct graphic_output_area *output,
					struct graphic_area *input);

/**
 * @brief Allocate and prepare a BLIT operation
 *
 * Perform a blit operation of a foreground area onto a background area.
 * Parameters are check for validity and in case of success a value > 0 is returned.
 *
 * @param dev	Pointer to the graphic device file
 * @param iarea	Pointer to the structure describing the input area.
 * @param iaddr	Address of the input buffer
 * @param oarea	Pointer to the structure describing the output area.
 * @param oaddr	Address of the output buffer
 *
 * @retval > 0 Operation ran successfully. Operation ID.
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
int graphic_alloc_blit_operation(const struct device *dev,
				 struct graphic_area *iarea, uint8_t *iaddr,
				 struct graphic_area *oarea, uint8_t *oaddr);

/**
 * @brief Allocate and prepare a FILL operation
 *
 * Perform a fill operation of a fixed color (including alpha depending on the
 * area format) onto an output area.
 * Parameters are check for validity and in case of success a value > 0 is returned.
 *
 * @param dev	Pointer to the graphic device file
 * @param output Pointer to the graphic_output_area structure
 * @param fill	Pointer to the graphic_fill_area structure
 *
 * @retval > 0 Operation ran successfully. Operation ID.
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
int graphic_alloc_fill_operation(const struct device *dev, struct graphic_output_area *output,
				 struct graphic_filled_area *fill);

/**
 * @brief Free an operation
 *
 * Discard an allocated operation.
 *
 * @param dev	Pointer to the graphic device file
 * @param op_id	Identifier of the operation to free
 *
 * @retval 0 Operation ran successfully.
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
int graphic_free_operation(const struct device *dev, int op_id);

/**
 * @brief Submit an operation
 *
 * Perform an operation
 *
 * @param dev	Pointer to the graphic device file
 * @param op_id Operation to be submitted
 *
 * @retval 0 Operation ran successfully
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
int graphic_submit_operation(const struct device *dev, int op_id);

/**
 * @brief Sync on processing of an operation
 *
 * Wait for the end of processing of an operation
 *
 * @param dev	Pointer to the graphic device file
 * @param op_id Operation identifier
 * @param timeout Timeout waiting for the end of the operation
 *
 * @retval 0 Operation ran successfully
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
int graphic_sync_operation(const struct device *dev, int op_id, k_timeout_t timeout);

/**
 * @brief Cancel an operation
 *
 * Cancel an operation already submitted. Operation might be either not started (yet)
 * or ongoing, in which case driver will try to stop it if possible
 *
 * @param dev	Pointer to the graphic device file
 * @param op_id Operation to be submitted
 *
 * @retval 0 Operation ran successfully
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
int graphic_cancel_operation(const struct device *dev, int op_id);

/**
 * @brief Get operation status
 *
 * Get the status regarding an operation
 *
 * @param dev	Pointer to the graphic device file
 * @param op_id Operation to be submitted
 * @param status	Pointer to the status structure
 *
 * @retval 0 Operation ran successfully
 * @retval -EINVAL Parameter given are invalid.
 * @retval -ENOTSUP Requested operation (with given parameters) is not supported
 * @retval -EIO Error while performing the operation
 * @retval -ENOSYS API is not implemented.
 */
int graphic_get_operation_status(const struct device *dev, int op_id,
				 enum graphic_operation_status *status);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_GRAPHIC_GRAPHIC_H */
