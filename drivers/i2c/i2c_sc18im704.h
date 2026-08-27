/*
 * Copyright (c), 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_SC18IM704_H_
#define ZEPHYR_DRIVERS_I2C_I2C_SC18IM704_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SC18IM704_CMD_STOP			0x50
#define SC18IM704_CMD_I2C_START			0x53
#define SC18IM704_CMD_READ_REG			0x52
#define SC18IM704_CMD_WRITE_REG			0x57
#define SC18IM704_CMD_READ_GPIO			0x49
#define SC18IM704_CMD_WRITE_GPIO		0x4f
#define SC18IM704_CMD_POWER_DOWN		0x5a

#define SC18IM704_REG_BRG0			0x00
#define SC18IM704_REG_BRG1			0x01
#define SC18IM704_REG_GPIO_CONF1		0x02
#define SC18IM704_REG_GPIO_CONF2		0x03
#define SC18IM704_REG_GPIO_STATE		0x04
#define SC18IM704_REG_I2C_ADDR			0x06
#define SC18IM704_REG_I2C_CLK_L			0x07
#define SC18IM704_REG_I2C_CLK_H			0x08
#define SC18IM704_REG_I2C_TIMEOUT		0x09
#define SC18IM704_REG_I2C_STAT			0x0a

#define SC18IM704_I2C_STAT_OK			0xf0
#define SC18IM704_I2C_STAT_NACK_ADDR		0xf1
#define SC18IM704_I2C_STAT_NACK_DATA		0xf2
#define SC18IM704_I2C_STAT_TIMEOUT		0xf8

/**
 * @brief Claim the SC18IM704 device.
 *
 * @warning After calling this routine, the device cannot be used by any other
 * thread until the sc18im704_release routine is called.
 *
 * @param dev SC18IM704 device.
 *
 * @retval 0 Device claimed.
 * @retval -EBUSY The device could not be claimed.
 */
int sc18im704_claim(const struct device *dev);

/**
 * @brief Release the SC18IM704 device claim.
 *
 * @warning This routine must only be used after a sc18im704_claim.
 *
 * @param dev SC18IM704 device.
 *
 * @retval 0 Device claim to release.
 * @retval -EPERM The current thread hasn't claimed the device.
 * @retval -EINVAL The device has no active claims.
 */
int sc18im704_release(const struct device *dev);

/**
 * @brief Exchange data with the SC18IM704 device.
 *
 * @param dev SC18IM704 device.
 * @param tx_data The data buffer to write from.
 * @param tx_len The length of the tx_data buffer.
 * @param rx_data The data buffer to read to.
 * @param rx_len The length of the rx_data buffer.
 *
 * @retval 0 If successful.
 * @retval -EAGAIN The device did not respond in time (1 second timeout).
 */
int sc18im704_transfer(const struct device *dev,
		       const uint8_t *tx_data, uint8_t tx_len,
		       uint8_t *rx_data, uint8_t rx_len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_SC18IM704_H_ */
