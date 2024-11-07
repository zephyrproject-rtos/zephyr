/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT cirrus_cs47l63

#include "cs47l63_comm.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "bsp_driver_if.h"
#include "cs47l63.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(CS47L63, CONFIG_CS47L63_LOG_LEVEL);

#define CS47L63_DEVID_VAL 0x47A63
#define PAD_LEN 4 /* Four bytes padding after address */
/* Delay the processing thread to allow interrupts to settle after boot */
#define CS47L63_PROCESS_THREAD_DELAY_MS 10

static const struct gpio_dt_spec hw_codec_gpio = GPIO_DT_SPEC_INST_GET(0, gpio9_gpios);
static const struct gpio_dt_spec hw_codec_irq = GPIO_DT_SPEC_INST_GET(0, irq_gpios);
static const struct gpio_dt_spec hw_codec_reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios);

const static struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

static const struct spi_dt_spec spi = SPI_DT_SPEC_INST_GET(
	0, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_LINES_SINGLE, 0);
static bsp_callback_t bsp_callback;
static void *bsp_callback_arg;

static struct gpio_callback gpio_cb;

static struct k_thread cs47l63_data;
static K_THREAD_STACK_DEFINE(cs47l63_stack, CONFIG_CS47L63_STACK_SIZE);

static K_SEM_DEFINE(sem_cs47l63, 0, 1);

static struct k_mutex cirrus_reg_oper_mutex;

static void notification_callback(uint32_t event_flags, void *arg)
{
	LOG_DBG("Notification from CS47L63, flags: %d", event_flags);
}

/* Locks the mutex and holds the CS pin
 * for consecutive transactions
 */
static int spi_mutex_lock(void)
{
	int ret;

	ret = k_mutex_lock(&cirrus_reg_oper_mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("Failed to lock mutex: %d", ret);
		return ret;
	}

	/* If operation mode set to HOLD or the SPI_LOCK_ON is set when
	 * taking the mutex something is wrong
	 */
	if ((spi.config.operation & SPI_HOLD_ON_CS) || (spi.config.operation & SPI_LOCK_ON)) {
		LOG_ERR("SPI_HOLD_ON_CS and SPI_LOCK_ON must be freed before releasing mutex");
		return -EPERM;
	}

	return 0;
}

/* Unlocks mutex and CS pin */
static int spi_mutex_unlock(void)
{
	int ret;
	/* If operation mode still set to HOLD or
	 * the SPI_LOCK_ON is still set when releasing the mutex
	 * something is wrong
	 */
	if ((spi.config.operation & SPI_HOLD_ON_CS) || (spi.config.operation & SPI_LOCK_ON)) {
		LOG_ERR("SPI_HOLD_ON_CS and SPI_LOCK_ON must be freed before releasing mutex");
		return -EPERM;
	}

	ret = k_mutex_unlock(&cirrus_reg_oper_mutex);
	if (ret) {
		LOG_ERR("Failed to unlock mutex: %d", ret);
		return ret;
	}

	return 0;
}

/* Pin interrupt handler for CS47L63 */
static void cs47l63_comm_pin_int_handler(const struct device *gpio_port, struct gpio_callback *cb,
					 uint32_t pins)
{
	__ASSERT(bsp_callback != NULL, "No callback registered");

	if (pins == BIT(hw_codec_irq.pin)) {
		bsp_callback(BSP_STATUS_OK, bsp_callback_arg);
		k_sem_give(&sem_cs47l63);
	}
}

static uint32_t cs47l63_comm_reg_read(uint32_t bsp_dev_id, uint8_t *addr_buffer,
				      uint32_t addr_length, uint8_t *data_buffer,
				      uint32_t data_length, uint32_t pad_len)
{
	if (pad_len != PAD_LEN) {
		LOG_ERR("Trying to pad more than 4 bytes: %d", pad_len);
		return BSP_STATUS_FAIL;
	}

	int ret;

	uint8_t pad_buffer[PAD_LEN] = { 0 };

	struct spi_buf_set rx;
	struct spi_buf rx_buf[] = { { .buf = addr_buffer, .len = addr_length },
				    { .buf = pad_buffer, .len = pad_len },
				    { .buf = data_buffer, .len = data_length } };

	rx.buffers = rx_buf;
	rx.count = ARRAY_SIZE(rx_buf);

	ret = spi_mutex_lock();
	if (ret) {
		return BSP_STATUS_FAIL;
	}

	ret = spi_transceive_dt(&spi, &rx, &rx);
	if (ret) {
		LOG_ERR("Failed transceive operation: %d", ret);
		return BSP_STATUS_FAIL;
	}

	ret = spi_mutex_unlock();
	if (ret) {
		return BSP_STATUS_FAIL;
	}

	return BSP_STATUS_OK;
}

static uint32_t cs47l63_comm_reg_write(uint32_t bsp_dev_id, uint8_t *addr_buffer,
				       uint32_t addr_length, uint8_t *data_buffer,
				       uint32_t data_length, uint32_t pad_len)
{
	if (pad_len != PAD_LEN) {
		LOG_ERR("Trying to pad more than 4 bytes: %d", pad_len);
		return BSP_STATUS_FAIL;
	}

	int ret;

	uint8_t pad_buffer[PAD_LEN] = { 0 };

	struct spi_buf_set tx;
	struct spi_buf tx_buf[] = { { .buf = addr_buffer, .len = addr_length },
				    { .buf = pad_buffer, .len = pad_len },
				    { .buf = data_buffer, .len = data_length } };

	tx.buffers = tx_buf;
	tx.count = ARRAY_SIZE(tx_buf);

	ret = spi_mutex_lock();
	if (ret) {
		return BSP_STATUS_FAIL;
	}

	ret = spi_write_dt(&spi, &tx);
	if (ret) {
		LOG_ERR("SPI failed to write: %d", ret);
		return BSP_STATUS_FAIL;
	}

	ret = spi_mutex_unlock();
	if (ret) {
		return BSP_STATUS_FAIL;
	}

	return BSP_STATUS_OK;
}

static uint32_t cs47l63_comm_gpio_set(uint32_t gpio_id, uint8_t gpio_state)
{
	int ret;

	ret = gpio_pin_set_raw(gpio_dev, gpio_id, gpio_state);

	if (ret) {
		LOG_ERR("Failed to set gpio state, ret: %d", ret);
		return BSP_STATUS_FAIL;
	}

	return BSP_STATUS_OK;
}

/* Register callback for pin interrupt from CS47L63 */
static uint32_t cs47l63_comm_gpio_cb_register(uint32_t gpio_id, bsp_callback_t cb, void *cb_arg)
{
	int ret;

	bsp_callback = cb;
	bsp_callback_arg = cb_arg;

	gpio_init_callback(&gpio_cb, cs47l63_comm_pin_int_handler, BIT(gpio_id));

	ret = gpio_add_callback(gpio_dev, &gpio_cb);
	if (ret) {
		return BSP_STATUS_FAIL;
	}

	ret = gpio_pin_interrupt_configure(gpio_dev, gpio_id, GPIO_INT_EDGE_TO_INACTIVE);
	if (ret) {
		return BSP_STATUS_FAIL;
	}

	return BSP_STATUS_OK;
}

static uint32_t cs47l63_comm_timer_set(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg)
{
	if (cb != NULL || cb_arg != NULL) {
		LOG_ERR("Timer with callback not supported");
		return BSP_STATUS_FAIL;
	}

	k_msleep(duration_ms);

	return BSP_STATUS_OK;
}

static uint32_t cs47l63_comm_set_supply(uint32_t supply_id, uint8_t supply_state)
{
	LOG_DBG("Tried to set supply, not supported");
	/* OK is returned in order to make reset function work */
	return BSP_STATUS_OK;
}

static uint32_t cs47l63_comm_i2c_reset(uint32_t bsp_dev_id, bool *was_i2c_busy)
{
	LOG_ERR("Tried to reset I2C, not supported");
	return BSP_STATUS_FAIL;
}

static uint32_t cs47l63_comm_i2c_read_repeated_start(uint32_t bsp_dev_id, uint8_t *write_buffer,
						     uint32_t write_length, uint8_t *read_buffer,
						     uint32_t read_length, bsp_callback_t cb,
						     void *cb_arg)
{
	LOG_ERR("Tried to read repeated start I2C, not supported");
	return BSP_STATUS_FAIL;
}

static uint32_t cs47l63_comm_i2c_write(uint32_t bsp_dev_id, uint8_t *write_buffer,
				       uint32_t write_length, bsp_callback_t cb, void *cb_arg)
{
	LOG_ERR("Tried writing to I2C, not supported");
	return BSP_STATUS_FAIL;
}

static uint32_t cs47l63_comm_i2c_db_write(uint32_t bsp_dev_id, uint8_t *write_buffer_0,
					  uint32_t write_length_0, uint8_t *write_buffer_1,
					  uint32_t write_length_1, bsp_callback_t cb, void *cb_arg)
{
	LOG_ERR("Tried to write double buffered I2C, not supported");
	return BSP_STATUS_FAIL;
}

static uint32_t cs47l63_comm_enable_irq(void)
{
	LOG_ERR("Tried to enable irq, not supported");
	return BSP_STATUS_FAIL;
}

static uint32_t cs47l63_comm_disable_irq(void)
{
	LOG_ERR("Tried to disable irq, not supported");
	return BSP_STATUS_FAIL;
}

static uint32_t cs47l63_comm_spi_throttle_speed(uint32_t speed_hz)
{
	LOG_ERR("Tried to throttle SPI speed, not supported");
	return BSP_STATUS_FAIL;
}

static uint32_t cs47l63_comm_spi_restore_speed(void)
{
	LOG_ERR("Tried to restore SPI speed, not supported");
	return BSP_STATUS_FAIL;
}

/* Thread to process events from CS47L63 */
static void cs47l63_comm_thread(void *cs47l63_driver, void *dummy2, void *dummy3)
{
	int ret;

	while (1) {
		k_sem_take(&sem_cs47l63, K_FOREVER);
		ret = cs47l63_process((cs47l63_t *)cs47l63_driver);
		if (ret) {
			LOG_ERR("CS47L63 failed to process event");
		}
	}
}

static cs47l63_bsp_config_t bsp_config = { .bsp_reset_gpio_id = hw_codec_reset.pin,
					   .bsp_int_gpio_id = hw_codec_irq.pin,
					   .cp_config.bus_type = CS47L63_BUS_TYPE_SPI,
					   .cp_config.spi_pad_len = 4,
					   .notification_cb = &notification_callback,
					   .notification_cb_arg = NULL };

int cs47l63_comm_init(cs47l63_t *cs47l63_driver)
{
	int ret;

	cs47l63_config_t cs47l63_config;

	memset(&cs47l63_config, 0, sizeof(cs47l63_config_t));

	k_mutex_init(&cirrus_reg_oper_mutex);

	if (!spi_is_ready_dt(&spi)) {
		LOG_ERR("CS47L63 is not ready!");
		return -ENXIO;
	}

	if (!gpio_is_ready_dt(&hw_codec_gpio)) {
		LOG_ERR("GPIO is not ready!");
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&hw_codec_gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	if (!gpio_is_ready_dt(&hw_codec_irq)) {
		LOG_ERR("GPIO is not ready!");
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&hw_codec_irq, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	if (!gpio_is_ready_dt(&hw_codec_reset)) {
		LOG_ERR("GPIO is not ready!");
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&hw_codec_reset, GPIO_OUTPUT);
	if (ret) {
		return ret;
	}

	/* Start thread to handle events from CS47L63 */
	(void)k_thread_create(&cs47l63_data, cs47l63_stack, CONFIG_CS47L63_STACK_SIZE,
			      (k_thread_entry_t)cs47l63_comm_thread, (void *)cs47l63_driver, NULL,
			      NULL, K_PRIO_PREEMPT(CONFIG_CS47L63_THREAD_PRIO), 0,
			      K_MSEC(CS47L63_PROCESS_THREAD_DELAY_MS));

	ret = k_thread_name_set(&cs47l63_data, "CS47L63");
	if (ret) {
		return ret;
	}

	/* Initialize CS47L63 drivers */
	ret = cs47l63_initialize(cs47l63_driver);
	if (ret != CS47L63_STATUS_OK) {
		LOG_ERR("Failed to initialize CS47L63");
		return -ENXIO;
	}

	cs47l63_config.bsp_config = bsp_config;

	cs47l63_config.syscfg_regs = cs47l63_syscfg_regs;
	cs47l63_config.syscfg_regs_total = CS47L63_SYSCFG_REGS_TOTAL;

	ret = cs47l63_configure(cs47l63_driver, &cs47l63_config);
	if (ret != CS47L63_STATUS_OK) {
		LOG_ERR("Failed to configure CS47L63");
		return -ENXIO;
	}

	/* Will pin reset the device and wait until boot done */
	ret = cs47l63_reset(cs47l63_driver);
	if (ret != CS47L63_STATUS_OK) {
		LOG_ERR("Failed to reset CS47L63");
		return -ENXIO;
	}

	if (cs47l63_driver->devid != CS47L63_DEVID_VAL) {
		LOG_ERR("Wrong device id: 0x%02x, should be 0x%02x", cs47l63_driver->devid,
			CS47L63_DEVID_VAL);
		return -EIO;
	}

	return 0;
}

static bsp_driver_if_t bsp_driver_if_s = { .set_gpio = &cs47l63_comm_gpio_set,
					   .register_gpio_cb = &cs47l63_comm_gpio_cb_register,
					   .set_timer = &cs47l63_comm_timer_set,
					   .spi_read = &cs47l63_comm_reg_read,
					   .spi_write = &cs47l63_comm_reg_write,

					   /* Functions not supported */
					   .set_supply = &cs47l63_comm_set_supply,
					   .i2c_read_repeated_start =
						   &cs47l63_comm_i2c_read_repeated_start,
					   .i2c_write = &cs47l63_comm_i2c_write,
					   .i2c_db_write = &cs47l63_comm_i2c_db_write,
					   .i2c_reset = &cs47l63_comm_i2c_reset,
					   .enable_irq = &cs47l63_comm_enable_irq,
					   .disable_irq = &cs47l63_comm_disable_irq,
					   .spi_throttle_speed = &cs47l63_comm_spi_throttle_speed,
					   .spi_restore_speed = &cs47l63_comm_spi_restore_speed };

bsp_driver_if_t *bsp_driver_if_g = &bsp_driver_if_s;
