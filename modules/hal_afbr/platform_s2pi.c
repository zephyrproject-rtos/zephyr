/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/rtio/rtio.h>

#include <platform/argus_s2pi.h>
#include <platform/argus_irq.h>
#include <api/argus_status.h>

/** Used to get instance data from slave index */
#include <../drivers/sensor/broadcom/afbr_s50/platform.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(afbr_s2pi, CONFIG_SENSOR_LOG_LEVEL);

status_t S2PI_GetStatus(s2pi_slave_t slave)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		return ERROR_FAIL;
	}

	return atomic_get(&data->s2pi.rtio.state);
}

/** This basic implementation assumes the AFBR is alone in the bus, hence no
 * need to get a mutex. If more AFBR's are lumped together in a bus, these
 * two APIs need an implementation.
 */
status_t S2PI_TryGetMutex(s2pi_slave_t slave)
{
	return STATUS_OK;
}

void S2PI_ReleaseMutex(s2pi_slave_t slave)
{
	/* See comment above. */
}

static void S2PI_complete_callback(struct rtio *ctx,
				   const struct rtio_sqe *sqe,
				   void *arg)
{
	struct afbr_s50_platform_data *data = (struct afbr_s50_platform_data *)arg;
	struct rtio_cqe *cqe;
	int err = 0;
	status_t status = STATUS_OK;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = err ? err : cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	/** If the transfer was aborted, skip notifying users */
	if (atomic_cas(&data->s2pi.rtio.state, STATUS_BUSY, STATUS_IDLE)) {
		status = STATUS_OK;
	} else if (atomic_cas(&data->s2pi.rtio.state, ERROR_ABORTED, STATUS_IDLE)) {
		status = ERROR_ABORTED;
	} else {
		return;
	}

	if (err) {
		status = ERROR_FAIL;
	}

	if (data->s2pi.rtio.callback.handler) {
		(void)data->s2pi.rtio.callback.handler(status, data->s2pi.rtio.callback.data);
	}
}

status_t S2PI_TransferFrame(s2pi_slave_t slave,
			    uint8_t const *txData,
			    uint8_t *rxData,
			    size_t frameSize,
			    s2pi_callback_t callback,
			    void *callbackData)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		return ERROR_S2PI_INVALID_SLAVE;
	}

	CHECKIF(atomic_get(&data->s2pi.mode) == STATUS_S2PI_GPIO_MODE) {
		LOG_ERR("S2PI is in GPIO MODE");
		return ERROR_S2PI_INVALID_STATE;
	}

	/** Check if the SPI bus is busy */
	if (!atomic_cas(&data->s2pi.rtio.state, STATUS_IDLE, STATUS_BUSY)) {
		LOG_WRN("SPI bus is busy");
		return STATUS_BUSY;
	}

	data->s2pi.rtio.callback.handler = callback;
	data->s2pi.rtio.callback.data = callbackData;

	struct rtio *ctx = data->s2pi.rtio.ctx;
	struct rtio_iodev *iodev = data->s2pi.rtio.iodev;

	struct rtio_sqe *xfer_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(ctx);

	CHECKIF(!xfer_sqe || !cb_sqe) {
		LOG_ERR("Failed to allocate SQE, please check RTIO queue "
			"configuration on afbr_s50.c");
		return ERROR_FAIL;
	}

	if (rxData) {
		rtio_sqe_prep_transceive(xfer_sqe, iodev, RTIO_PRIO_HIGH,
					 txData, rxData, frameSize, NULL);
	} else {
		rtio_sqe_prep_write(xfer_sqe, iodev, RTIO_PRIO_HIGH,
				    txData, frameSize, NULL);
	}
	xfer_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(cb_sqe, S2PI_complete_callback, data, NULL);

	rtio_submit(ctx, 0);

	return STATUS_OK;
}

status_t S2PI_Abort(s2pi_slave_t slave)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		return ERROR_S2PI_INVALID_SLAVE;
	}

	(void)atomic_set(&data->s2pi.rtio.state, ERROR_ABORTED);

	S2PI_complete_callback(data->s2pi.rtio.ctx, NULL, data);

	return STATUS_OK;
}

status_t S2PI_SetIrqCallback(s2pi_slave_t slave,
			     s2pi_irq_callback_t callback,
			     void *callbackData)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		return ERROR_S2PI_INVALID_SLAVE;
	}

	data->s2pi.irq.handler = callback;
	data->s2pi.irq.data = callbackData;

	if (data->s2pi.irq.handler) {
		err = gpio_pin_interrupt_configure_dt(data->s2pi.gpio.irq, GPIO_INT_EDGE_TO_ACTIVE);
		CHECKIF(err) {
			LOG_ERR("Failed to enable IRQ GPIO: %d", err);
			return ERROR_FAIL;
		}
	} else {
		err = gpio_pin_interrupt_configure_dt(data->s2pi.gpio.irq, GPIO_INT_DISABLE);
		CHECKIF(err) {
			LOG_ERR("Failed to disable IRQ GPIO: %d", err);
			return ERROR_FAIL;
		}
	}

	return STATUS_OK;
}

uint32_t S2PI_ReadIrqPin(s2pi_slave_t slave)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		LOG_ERR("Error getting platform data: %d", err);
		return 0;
	}

	const struct gpio_dt_spec *irq = data->s2pi.gpio.irq;
	int value;

	value =  gpio_pin_get_dt(irq);
	CHECKIF(value < 0) {
		LOG_ERR("Error reading IRQ pin: %d", value);
		return 0;
	}

	return !value;
}

status_t S2PI_CycleCsPin(s2pi_slave_t slave)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		LOG_ERR("Error getting platform data: %d", err);
		return ERROR_S2PI_INVALID_SLAVE;
	}

	const struct gpio_dt_spec *cs = data->s2pi.gpio.spi.cs;

	err = gpio_pin_set_dt(cs, 1);
	CHECKIF(err) {
		LOG_ERR("Error setting CS pin logical high: %d", err);
		return ERROR_FAIL;
	}

	err = gpio_pin_set_dt(cs, 0);
	CHECKIF(err) {
		LOG_ERR("Error setting CS pin logical low: %d", err);
		return ERROR_FAIL;
	}

	return STATUS_OK;
}

status_t S2PI_CaptureGpioControl(s2pi_slave_t slave)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		LOG_ERR("Error getting platform data: %d", err);
		return ERROR_S2PI_INVALID_SLAVE;
	}

	err = pinctrl_apply_state(data->s2pi.pincfg, PINCTRL_STATE_PRIV_START);
	CHECKIF(err) {
		LOG_ERR("Error applying gpio pinctrl state: %d", err);
		return ERROR_FAIL;
	}

	err = gpio_pin_configure_dt(data->s2pi.gpio.spi.miso, GPIO_INPUT | GPIO_PULL_UP);
	err |= gpio_pin_configure_dt(data->s2pi.gpio.spi.mosi, GPIO_OUTPUT);
	err |= gpio_pin_configure_dt(data->s2pi.gpio.spi.clk, GPIO_OUTPUT);
	CHECKIF(err) {
		LOG_ERR("Error configuring GPIO pins: %d", err);
		return ERROR_FAIL;
	}

	(void)atomic_set(&data->s2pi.mode, STATUS_S2PI_GPIO_MODE);

	return STATUS_OK;
}

status_t S2PI_ReleaseGpioControl(s2pi_slave_t slave)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		LOG_ERR("Error getting platform data: %d", err);
		return ERROR_S2PI_INVALID_SLAVE;
	}

	(void)gpio_pin_configure_dt(data->s2pi.gpio.spi.miso, GPIO_DISCONNECTED);
	(void)gpio_pin_configure_dt(data->s2pi.gpio.spi.mosi, GPIO_DISCONNECTED);
	(void)gpio_pin_configure_dt(data->s2pi.gpio.spi.clk, GPIO_DISCONNECTED);

	err = pinctrl_apply_state(data->s2pi.pincfg, PINCTRL_STATE_DEFAULT);
	CHECKIF(err) {
		LOG_ERR("Error applying default pinctrl state: %d", err);
		return ERROR_FAIL;
	}

	(void)atomic_set(&data->s2pi.mode, STATUS_IDLE);

	return STATUS_OK;
}

status_t S2PI_WriteGpioPin(s2pi_slave_t slave, s2pi_pin_t pin, uint32_t value)
{
	struct afbr_s50_platform_data *data;
	const struct gpio_dt_spec *gpio_pin;
	int err;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		LOG_ERR("Error getting platform data: %d", err);
		return ERROR_S2PI_INVALID_SLAVE;
	}

	CHECKIF(!(atomic_get(&data->s2pi.mode) == STATUS_S2PI_GPIO_MODE)) {
		LOG_ERR("S2PI is in SPI MODE");
		return ERROR_S2PI_INVALID_STATE;
	}

	switch (pin) {
	case S2PI_CS:
		gpio_pin = data->s2pi.gpio.spi.cs;
		break;
	case S2PI_CLK:
		gpio_pin = data->s2pi.gpio.spi.clk;
		break;
	case S2PI_MOSI:
		gpio_pin = data->s2pi.gpio.spi.mosi;
		break;
	case S2PI_MISO:
		gpio_pin = data->s2pi.gpio.spi.miso;
		break;
	default:
		CHECKIF(true) {
			__ASSERT(false, "Not implemented for pin: %d", pin);
			return ERROR_FAIL;
		}
	}

	err = gpio_pin_set_raw(gpio_pin->port, gpio_pin->pin, value);
	CHECKIF(err) {
		LOG_ERR("Error setting GPIO pin: %d", err);
		return ERROR_FAIL;
	}

	/* Delay suggested by API documentation of S2PI_WriteGpioPin */
	k_sleep(K_USEC(10));

	return STATUS_OK;
}

status_t S2PI_ReadGpioPin(s2pi_slave_t slave, s2pi_pin_t pin, uint32_t *value)
{
	struct afbr_s50_platform_data *data;
	const struct gpio_dt_spec *gpio_pin;
	int err;
	int pin_value;

	err = afbr_s50_platform_get_by_id(slave, &data);
	CHECKIF(err) {
		LOG_ERR("Error getting platform data: %d", err);
		return ERROR_S2PI_INVALID_SLAVE;
	}

	CHECKIF(!(atomic_get(&data->s2pi.mode) == STATUS_S2PI_GPIO_MODE)) {
		LOG_ERR("S2PI is in SPI MODE");
		return ERROR_S2PI_INVALID_STATE;
	}

	switch (pin) {
	case S2PI_CS:
		gpio_pin = data->s2pi.gpio.spi.cs;
		break;
	case S2PI_CLK:
		gpio_pin = data->s2pi.gpio.spi.clk;
		break;
	case S2PI_MOSI:
		gpio_pin = data->s2pi.gpio.spi.mosi;
		break;
	case S2PI_MISO:
		gpio_pin = data->s2pi.gpio.spi.miso;
		break;
	default:
		CHECKIF(true) {
			__ASSERT(false, "Not implemented for pin: %d", pin);
			return ERROR_FAIL;
		}
	}

	pin_value = gpio_pin_get_raw(gpio_pin->port, gpio_pin->pin);
	*value = (uint32_t)pin_value;

	return STATUS_OK;
}

static void s2pi_irq_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb,
				   uint32_t pins)
{
	struct afbr_s50_platform_data *data = CONTAINER_OF(cb,
							   struct afbr_s50_platform_data,
							   s2pi.irq.cb);

	if (data->s2pi.irq.handler) {
		data->s2pi.irq.handler(data->s2pi.irq.data);
	}
}

static int s2pi_init(struct afbr_s50_platform_data *data)
{
	int err;

	(void)atomic_set(&data->s2pi.rtio.state, STATUS_IDLE);
	(void)atomic_set(&data->s2pi.mode, STATUS_IDLE);

	CHECKIF(!data->s2pi.gpio.irq || !data->s2pi.gpio.spi.cs) {
		LOG_ERR("GPIOs not supplied");
		return -EIO;
	}

	if (!gpio_is_ready_dt(data->s2pi.gpio.irq)) {
		LOG_ERR("IRQ GPIO not ready");
		return -EIO;
	}

	err = gpio_pin_configure_dt(data->s2pi.gpio.irq, GPIO_INPUT);
	CHECKIF(err) {
		LOG_ERR("Failed to configure IRQ GPIO");
		return -EIO;
	}

	gpio_init_callback(&data->s2pi.irq.cb, s2pi_irq_gpio_callback,
			   BIT(data->s2pi.gpio.irq->pin));

	err = gpio_add_callback(data->s2pi.gpio.irq->port, &data->s2pi.irq.cb);
	CHECKIF(err) {
		LOG_ERR("Failed to add IRQ callback");
		return -EIO;
	}

	return 0;
}

static struct afbr_s50_platform_init_node s2pi_init_node = {
	.init_fn = s2pi_init,
};

static int s2pi_platform_init_hooks(void)
{
	afbr_s50_platform_init_hooks_add(&s2pi_init_node);

	return 0;
}

SYS_INIT(s2pi_platform_init_hooks, POST_KERNEL, 0);
