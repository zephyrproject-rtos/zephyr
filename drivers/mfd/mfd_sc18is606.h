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
	struct k_mutex bridge_lock;  /**< Mutex for the mfd device base */
	struct gpio_callback int_cb; /**< Gpio call back for gpio functions */
	struct k_sem int_sem;        /**< Semaphore to  gate access	*/
	uint8_t gpio_enable;
	uint8_t pin_conf;
};

/**
 * SC18IS606 Configuration Struct
 *
 * Contains the i2c controller, reset and interrupt gpios
 */
struct sc18is606_config {
	const struct i2c_dt_spec i2c_controller; /**< I2C controller for the device */
	const struct gpio_dt_spec reset_gpios;   /**< Device reset gpio */
	const struct gpio_dt_spec int_gpios;     /**< Device interrupt gpio */
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

/**
 * Configure a pin on the bridge
 *
 * This routine implements the synchronization between the SPI controller and GPIO cntroller
 *
 * @param dev SC18IS606 bridge
 * @param pin pin to configure
 * @param is_gpio whether it is to be used as a GPIO or CS pin
 * @param mode pin mode, SC18IS606_GPIO_CS_CONF for a CS pin
 *
 * @retval 0 pin mode set successfully
 */
int nxp_sc18is606_set_pin_mode(const struct device *dev, const uint8_t pin, const bool is_gpio,
			       const uint8_t mode);

/** How many pins the bridge has */
#define SC18IS606_GPIO_MAX_PINS 3

/** SPI configuration register */
#define SC18IS606_CONFIG_SPI      0xF0
/** Clear interrupt command */
#define SC18IS606_CLEAR_INTERRUPT 0xF1
/** Idle sleep command */
#define SC18IS606_IDLE_MODE       0xF2
/** GPIO output value register */
#define SC18IS606_GPIO_WRITE      0xF4
/** GPIO input value register  */
#define SC18IS606_GPIO_READ       0xF5
/** GPIO mode register */
#define SC18IS606_GPIO_ENABLE     0xF6
/** GPIO I/O mode configuration register */
#define SC18IS606_GPIO_CONF       0xF7

/** Input I/O mode */
#define SC18IS606_GPIO_CONF_INPUT      0x00
/** Output I/O mode */
#define SC18IS606_GPIO_CONF_PUSH_PULL  0x01
/** Hi-Z I/O mode */
#define SC18IS606_GPIO_CONF_OPEN_DRAIN 0x03

/** SPI LSB/MSB switch bit */
#define SC18IS606_LSB_MASK         GENMASK(5, 5)
/** SPI mode selection bits */
#define SC18IS606_MODE_MASK        GENMASK(3, 2)
/** SPI frequency selection bits */
#define SC18IS606_FREQ_MASK        GENMASK(1, 0)
/** GPIO I/O mode configuration bits */
#define SC18IS606_GPIO_CONF_MASK   GENMASK(1, 0)
/** GPIO mode register bits */
#define SC18IS606_GPIO_ENABLE_MASK GENMASK(2, 0)

/** 'GPIO' Push-Pull mode that must be set for a CS pin */
#define SC18IS606_GPIO_CS_CONF SC18IS606_GPIO_CONF_PUSH_PULL

#ifdef __cplusplus
}
#endif

#endif
