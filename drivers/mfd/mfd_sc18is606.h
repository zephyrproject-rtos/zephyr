/*
 * Copyright (c), 2025 tinyvision.ai Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MFD SC18IS606 interface
 *
 */

#ifndef ZEPHYR_DRIVERS_MFD_SC18IS606_H_
#define ZEPHYR_DRIVERS_MFD_SC18IS606_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif



/**
 * SC18IS606 Data Struct
 *
 * Contains the mutex, interrupt callback and semaphore
 */
struct sc18is606_data {
	struct k_mutex bridge_lock;	/**< Mutex for the mfd device base */
	struct gpio_callback int_cb;	/**< Gpio call back for gpio functions */
	struct k_sem int_sem;		/**< Semaphore to  gate access	*/
};

/**
 * SC18IS606 Configuration Struct
 *
 * Contains the i2c controller, reset and interrupt gpios
 */
struct sc18is606_config {
	const struct i2c_dt_spec i2c_controller;	/**< I2C controller for the device */
	const struct gpio_dt_spec reset_gpios;		/**< Device reset gpio */
	const struct gpio_dt_spec int_gpios;		/**< Device interrupt gpio */
};

/**
 * Claim the SC18IS606 bridge
 *
 * @warning After calling this routine, the device cannot be used by any other thread
 * until the calling bridge releases it with the counterpart function of this.
 *
 * @param[in]  dev SC18IS606  device
 *
 * @return Result of claiming the bridge control
 * @retval 0 Device is claimed
 * @retval -EBUSY The device cannot be claimed
 */
static inline int nxp_sc18is606_claim(const struct device *dev)
{
	struct sc18is606_data *data = dev->data;

	return k_mutex_lock(&data->bridge_lock, K_FOREVER);
}

/**
 * Release the SC18IS606 bridge
 *
 * @warning this routine can only be called once a device has been locked
 *
 * @param dev SC18IS606 bridge
 *
 * @retval 0  Device is released
 * @retval -EINVAL The device has no locks on it.
 */
static inline int nxp_sc18is606_release(const struct device *dev)
{
	struct sc18is606_data *data = dev->data;

	return k_mutex_unlock(&data->bridge_lock);
}

/**
 * Transfer data using I2C to or from the bridge
 *
 * This routine implements the synchronization between the SPI controller and GPIO cntroller
 *
 * @param dev SC18IS606 bridge
 * @param tx_data Data to be sent out
 * @param tx_len Tx Data length
 * @param rx_data Container to receive data
 * @param rx_len  size of expected receipt
 * @param id_buf Function id data if used
 *
 * @retval 0 Transfer success
 * @retval -EAGAIN device lock timed out
 * @retval -EBUSY device already locked
 */
int nxp_sc18is606_transfer(const struct device *dev, const uint8_t *tx_data, uint8_t tx_len,
			   uint8_t *rx_data, uint8_t rx_len, uint8_t *id_buf);

#ifdef __cplusplus
}
#endif

#endif
