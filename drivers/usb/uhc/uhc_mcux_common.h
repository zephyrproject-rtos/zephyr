/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_UHC_MCUX_COMMON_H
#define ZEPHYR_INCLUDE_UHC_MCUX_COMMON_H

struct uhc_mcux_ep_handle {
	uint8_t ep;
	void *udev;
	usb_host_pipe_handle mcux_ep_handle;
};

struct uhc_mcux_data {
	const struct device *dev;
	const usb_host_controller_interface_t *mcux_if;
	struct uhc_mcux_ep_handle ep_handles[USB_HOST_CONFIG_MAX_PIPES];
	usb_host_instance_t mcux_host;
	struct k_thread drv_stack_data;
	uint8_t controller_id; /* MCUX hal controller id, 0xFF is invalid value */
};

/* call MCUX HAL controller driver to control controller */
int uhc_mcux_control(const struct device *dev, uint32_t control, void *param);

/* call MCUX HAL controller driver to control controller */
int uhc_mcux_bus_control(const struct device *dev, usb_host_bus_control_t type);

/* MCUX controller driver common lock function */
int uhc_mcux_lock(const struct device *dev);

/* MCUX controller driver common unlock function */
int uhc_mcux_unlock(const struct device *dev);

/* Initialize endpoint for MCUX HAL driver */
usb_host_pipe_handle uhc_mcux_init_hal_ep(const struct device *dev,
					  struct uhc_transfer *const xfer);

/* Initialize transfer for MCUX HAL driver */
int uhc_mcux_hal_init_transfer_common(const struct device *dev, usb_host_transfer_t *mcux_xfer,
				      usb_host_pipe_handle mcux_ep_handle,
				      struct uhc_transfer *const xfer,
				      host_inner_transfer_callback_t cb);

#endif /* ZEPHYR_INCLUDE_UHC_MCUX_COMMON_H */
