/*
 * Copyright (c) 2022 Nuvoton Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief kscan_matrix public API header file.
 *
 * This file contains the required interface for controlling the keyboard scan
 * matrix input/output signals.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_KB_SCAN_MATRIX_H_
#define ZEPHYR_INCLUDE_DRIVERS_KB_SCAN_MATRIX_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Keyboard matrix column index to drive.
 *
 * Keyboard matrix column enumeration.
 */
enum keyboard_column_drive_index {
	KEYBOARD_COLUMN_DRIVE_ALL = -2, /* Drive all columns */
	KEYBOARD_COLUMN_DRIVE_NONE = -1 /* Drive no columns (tri-state all) */
	/* 0 ~ MAX_COLUMN-1 for the corresponding column */
};

/**
 * @brief Keyboard scan ISR callback called when user presses to trigger the
 * keybaord scan interrupt.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
typedef void (*kscan_isr_callback_t)(const struct device *dev);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Keyboard scan matrix driver API definition and system call entry points.
 *
 * (Internal use only.)
 */
typedef int (*kscan_matrix_config_t)(const struct device *dev, kscan_isr_callback_t isr_callback);
typedef int (*kscan_matrix_drive_column_t)(const struct device *dev, int col);
typedef int (*kscan_matrix_read_row_t)(const struct device *dev, int *row);
typedef int (*kscan_matrix_resume_detection_t)(const struct device *dev, bool resume);

__subsystem struct kscan_matrix_driver_api {
	kscan_matrix_config_t matrix_config;
	kscan_matrix_drive_column_t matrix_drive_column;
	kscan_matrix_read_row_t matrix_read_row;
	kscan_matrix_resume_detection_t matrix_resume_detection;
};

/**
 * @endcond
 */

/**
 * @brief Configure a Keyboard matrix scan instance.
 * This function is optional and only needed for an SoC to scan the keyboard matrix by itself.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param isr_callback called when a key is pressed which causes the level
 * change on kscan input pin.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS if the interface is not implemented.
 * @retval Negative errno code if failure.
 */
__syscall int kscan_matrix_configure(const struct device *dev, kscan_isr_callback_t isr_callback);

static inline int z_impl_kscan_matrix_configure(const struct device *dev,
					     kscan_isr_callback_t isr_callback)
{
	const struct kscan_matrix_driver_api *api = (struct kscan_matrix_driver_api *)dev->api;

	if (api->matrix_config == NULL) {
		return -ENOSYS;
	}

	return api->matrix_config(dev, isr_callback);
}
/**
 * @brief Drive the specified columns of keyboard matrix to low.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param col the specified columns of keyboard matrix.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS if the interface is not implemented.
 * @retval Negative errno code if failure.
 */
__syscall int kscan_matrix_drive_column(const struct device *dev, int col);

static inline int z_impl_kscan_matrix_drive_column(const struct device *dev, int col)
{
	const struct kscan_matrix_driver_api *api = (struct kscan_matrix_driver_api *)dev->api;

	if (api->matrix_drive_column == NULL) {
		return -ENOSYS;
	}

	return api->matrix_drive_column(dev, col);
}

/**
 * @brief Read current row state of keyboard matrix.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param row current row state of keyboard matrix.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS if the interface is not implemented.
 * @retval Negative errno code if failure.
 */
__syscall int kscan_matrix_read_row(const struct device *dev, int *row);

static inline int z_impl_kscan_matrix_read_row(const struct device *dev, int *row)
{
	const struct kscan_matrix_driver_api *api = (struct kscan_matrix_driver_api *)dev->api;

	if (api->matrix_read_row == NULL) {
		return -ENOSYS;
	}

	return api->matrix_read_row(dev, row);
}

/**
 * @brief Resume detection pressing from keyboard matrix .
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param resume True if resume detection of keyboard matrix; otherwise, suspend it.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS if the interface is not implemented.
 * @retval Negative errno code if failure.
 */
__syscall int kscan_matrix_resume_detection(const struct device *dev, bool resume);

static inline int z_impl_kscan_matrix_resume_detection(const struct device *dev, bool resume)
{
	const struct kscan_matrix_driver_api *api = (struct kscan_matrix_driver_api *)dev->api;

	if (api->matrix_resume_detection == NULL) {
		return -ENOSYS;
	}

	return api->matrix_resume_detection(dev, resume);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/kscan_matrix.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_KB_SCAN_MATRIX_H_ */
