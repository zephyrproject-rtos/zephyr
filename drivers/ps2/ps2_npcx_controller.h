/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PS2_PS2_NPCX_CONTROLLER_H_
#define ZEPHYR_DRIVERS_PS2_PS2_NPCX_CONTROLLER_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write @p value to a PS/2 device via the PS/2 controller.
 *
 * @param dev Pointer to the device structure for PS/2 controller instance.
 * @param channel_id Channel ID of the PS/2 to write data.
 * @param value the data write to the PS/2 device.
 *
 * @retval 0 If successful.
 * @retval -EINVAL Channel ID is invalid.
 * @retval -ETIMEDOUT Timeout occurred for a PS/2 write transaction.
 */
int ps2_npcx_ctrl_write(const struct device *dev, uint8_t channel_id,
			uint8_t value);

/**
 * @brief Set the PS/2 controller to turn on/off the PS/2 channel.
 *
 * @param dev Pointer to the device structure for PS/2 controller instance.
 * @param channel_id Channel ID of the PS/2 to enable or disable.
 * @param enable True to enable channel, false to disable channel.
 *
 * @retval 0 If successful.
 * @retval -EINVAL Channel ID is invalid.
 */
int ps2_npcx_ctrl_enable_interface(const struct device *dev, uint8_t channel,
				   bool enable);

/**
 * @brief Record the callback_isr function pointer for the given PS/2 channel.
 *
 * @param dev Pointer to the device structure for PS/2 controller instance.
 * @param channel_id Channel ID of the PS/2 to configure the callback_isr.
 * @param callback_isr Pointer to the callback_isr.
 *
 * @retval 0 If successful.
 * @retval -EINVAL callback_isr is NULL.
 */
int ps2_npcx_ctrl_configure(const struct device *dev, uint8_t channel_id,
			    ps2_callback_t callback_isr);
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_PS2_PS2_NPCX_CONTROLLER_H_ */
