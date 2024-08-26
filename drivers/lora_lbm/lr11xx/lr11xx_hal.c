/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

#include <lr11xx_hal.h>
#include "lr11xx_hal_context.h"

LOG_MODULE_DECLARE(lora_lr11xx, CONFIG_LORA_BASICS_MODEM_DRIVERS_LOG_LEVEL);

/**
 * @brief Wait until radio busy pin returns to inactive state or
 * until CONFIG_LORA_BASICS_MODEM_DRIVERS_HAL_WAIT_ON_BUSY_TIMEOUT_MSEC passes.
 *
 * @retval LR11XX_HAL_STATUS_OK
 */
static void lr11xx_hal_wait_on_busy(const void *context)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;
	bool ret;

	ret = WAIT_FOR(gpio_pin_get_dt(&config->busy) == 0,
		       (1000 * CONFIG_LORA_BASICS_MODEM_DRIVERS_HAL_WAIT_ON_BUSY_TIMEOUT_MSEC),
		       k_usleep(100));
	if (!ret) {
		LOG_ERR("Timeout of %dms hit when waiting for lr11xx busy!",
			CONFIG_LORA_BASICS_MODEM_DRIVERS_HAL_WAIT_ON_BUSY_TIMEOUT_MSEC);
		k_oops();
	}
}

/**
 * @brief Check if device is ready to receive spi transaction.
 *
 * If the device is in sleep mode, it will awake it and then wait until it is ready
 *
 */
static void lr11xx_hal_check_device_ready(const void *context)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;
	struct lr11xx_hal_context_data_t *data = dev->data;

	if (data->radio_status != RADIO_SLEEP) {
		lr11xx_hal_wait_on_busy(context);
	} else {
		/* Busy is HIGH in sleep mode, wake-up the device with a small glitch on NSS */
		const struct gpio_dt_spec *cs = &(config->spi.config.cs.gpio);

		gpio_pin_set_dt(cs, 1);
		gpio_pin_set_dt(cs, 0);
		lr11xx_hal_wait_on_busy(context);
		data->radio_status = RADIO_AWAKE;
	}
}

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

lr11xx_hal_status_t lr11xx_hal_write(const void *context, const uint8_t *command,
				     const uint16_t command_length, const uint8_t *data,
				     const uint16_t data_length)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;
	struct lr11xx_hal_context_data_t *dev_data = dev->data;
	int ret;

#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
	/* Compute the CRC over command array first and over data array then */
	uint8_t cmd_crc = lr11xx_hal_compute_crc(0xFF, command, command_length);

	cmd_crc = lr11xx_hal_compute_crc(cmd_crc, data, data_length);
#endif /* defined( CONFIG_LR11XX_USE_CRC_OVER_SPI ) */

	const struct spi_buf tx_buf[] = {{
						 .buf = (uint8_t *)command,
						 .len = command_length,
					 },
					 {.buf = (uint8_t *)data, .len = data_length
#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
					 },
					 {.buf = &cmd_crc, .len = 1
#endif /* defined( CONFIG_LR11XX_USE_CRC_OVER_SPI ) */
					 }};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};

	lr11xx_hal_check_device_ready(context);
	ret = spi_write_dt(&config->spi, &tx);
	if (ret) {
		return LR11XX_HAL_STATUS_ERROR;
	}

	/* LR11XX_SYSTEM_SET_SLEEP_OC=0x011B opcode.
	 * In sleep mode the radio busy line is held at 1 => do not test it
	 */
	if ((command[0] == 0x01) && (command_length > 1) && (command[1] == 0x1B)) {
		dev_data->radio_status = RADIO_SLEEP;

		/* Add a incompressible delay to prevent trying to wake the radio
		 * before it is full asleep
		 */
		k_sleep(K_USEC(500));
	}

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_direct_read(const void *context, uint8_t *data,
					   const uint16_t data_length)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;
	int ret;

#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
	uint8_t rx_crc;
#endif /* defined( CONFIG_LR11XX_USE_CRC_OVER_SPI ) */

	const struct spi_buf rx_buf[] = {
		{.buf = data, .len = data_length
#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
		},
		{/* read crc sent by lr11xx at the end of the transaction */
		 .buf = &rx_crc,
		 .len = 1
#endif /* defined( CONFIG_LR11XX_USE_CRC_OVER_SPI ) */
		}};

	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};

	lr11xx_hal_check_device_ready(context);
	ret = spi_read_dt(&config->spi, &rx);
	if (ret) {
		return LR11XX_HAL_STATUS_ERROR;
	}

#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
	/* check crc value */
	uint8_t computed_crc = lr11xx_hal_compute_crc(0xFF, data, data_length);

	if (rx_crc != computed_crc) {
		return LR11XX_HAL_STATUS_ERROR;
	}
#endif /* defined( CONFIG_LR11XX_USE_CRC_OVER_SPI ) */

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_read(const void *context, const uint8_t *command,
				    const uint16_t command_length, uint8_t *data,
				    const uint16_t data_length)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;
	int ret;

#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
	/* Compute the CRC over command array first and over data array then */
	uint8_t cmd_crc = lr11xx_hal_compute_crc(0xFF, command, command_length);
#endif

	/* When hal_read is called by lr11xx_crypto_restore_from_flash during LoRa initialization,
	 * we sleep for 1 ms so we don't get stuck in an endless wait loop
	 */
	if ((command[0] == 0x05) && (command[1] == 0x0B)) {
		k_sleep(K_MSEC(1));
	}

	const struct spi_buf tx_buf[] = {{
						 .buf = (uint8_t *)command,
						 .len = command_length,
#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
					 },
					 {.buf = &cmd_crc, .len = 1
#endif /* defined( CONFIG_LR11XX_USE_CRC_OVER_SPI ) */
					 }};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};

	lr11xx_hal_check_device_ready(context);
	ret = spi_write_dt(&config->spi, &tx);
	if (ret) {
		return LR11XX_HAL_STATUS_ERROR;
	}

	if (data_length > 0) {
		uint8_t dummy_byte;

		const struct spi_buf rx_buf[] = {
			/* save dummy for crc calculation */
			{
				.buf = &dummy_byte,
				.len = 1,
			},
			{.buf = data, .len = data_length
#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
			},
			{/* read crc sent by lr11xx at the end of the transaction */
			 .buf = &cmd_crc,
			 .len = 1
#endif /* defined( CONFIG_LR11XX_USE_CRC_OVER_SPI ) */
			}};

		const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};

		lr11xx_hal_check_device_ready(context);
		ret = spi_read_dt(&config->spi, &rx);
		if (ret) {
			return LR11XX_HAL_STATUS_ERROR;
		}

#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
		/* Check CRC value */
		uint8_t computed_crc = lr11xx_hal_compute_crc(0xFF, &dummy_byte, 1);

		computed_crc = lr11xx_hal_compute_crc(computed_crc, data, data_length);
		if (cmd_crc != computed_crc) {
			return LR11XX_HAL_STATUS_ERROR;
		}
#endif /* defined( CONFIG_LR11XX_USE_CRC_OVER_SPI ) */
	}

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_reset(const void *context)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;
	struct lr11xx_hal_context_data_t *data = dev->data;

	gpio_pin_set_dt(&config->reset, 1);
	k_sleep(K_MSEC(1));
	gpio_pin_set_dt(&config->reset, 0);
	k_sleep(K_MSEC(1));

	/* Wait 200ms until internal lr11xx fw is ready */
	k_sleep(K_MSEC(200));
	data->radio_status = RADIO_AWAKE;

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_wakeup(const void *context)
{
	lr11xx_hal_check_device_ready(context);

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_abort_blocking_cmd(const void *context)
{
	/* Send a dummy command to abort the ongoing command */
	uint8_t abort_cmd[1] = {0x00};

	return lr11xx_hal_write(context, abort_cmd, sizeof(abort_cmd), NULL, 0);
}
