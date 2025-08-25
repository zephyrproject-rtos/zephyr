/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_i2c

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_mspm0, CONFIG_I2C_LOG_LEVEL);
#include "i2c-priv.h"

/* Driverlib includes */
#include <ti/driverlib/dl_i2c.h>

#define TI_MSPM0_CONTROLLER_INTERRUPTS                                                             \
	(DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST | DL_I2C_INTERRUPT_CONTROLLER_NACK |         \
	 DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER | DL_I2C_INTERRUPT_CONTROLLER_RX_DONE |        \
	 DL_I2C_INTERRUPT_CONTROLLER_TX_DONE | DL_I2C_INTERRUPT_TIMEOUT_A)

#define TI_MSPM0_TARGET_INTERRUPTS                                                                 \
	(DL_I2C_INTERRUPT_TARGET_RX_DONE | DL_I2C_INTERRUPT_TARGET_TXFIFO_EMPTY |                  \
	 DL_I2C_INTERRUPT_TARGET_START | DL_I2C_INTERRUPT_TARGET_STOP |                            \
	 DL_I2C_INTERRUPT_TIMEOUT_A)

enum i2c_mspm0_state {
	I2C_MSPM0_IDLE,
	I2C_MSPM0_TX_STARTED,
	I2C_MSPM0_TX_INPROGRESS,
	I2C_MSPM0_TX_COMPLETE,
	I2C_MSPM0_RX_STARTED,
	I2C_MSPM0_RX_INPROGRESS,
	I2C_MSPM0_RX_COMPLETE,
	I2C_MSPM0_TARGET_STARTED,
	I2C_MSPM0_TARGET_TX_INPROGRESS,
	I2C_MSPM0_TARGET_RX_INPROGRESS,
	I2C_MSPM0_TARGET_PREEMPTED,
	I2C_MSPM0_TIMEOUT,
	I2C_MSPM0_ERROR
};

struct i2c_mspm0_config {
	I2C_Regs *base;
	uint32_t bitrate;
	uint32_t merge_buf_size;
	uint8_t *merge_buf;
	DL_I2C_ClockConfig gI2CClockConfig;
	const struct mspm0_sys_clock *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
	void (*irq_config_func)(const struct device *dev);
};

struct i2c_mspm0_data {
	volatile enum i2c_mspm0_state state;
	struct i2c_msg msg;
	uint16_t addr;
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_config;
	const struct i2c_target_callbacks *target_callbacks;
#if (CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS != 0)
	uint16_t secondaryAddr;
	/* Secondary don't care mask. Muxed with
	 * the secondary address to create a range
	 * of acceptable addresses.
	 */
	uint16_t secondaryDontCareMask;
#endif /* CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS */
#endif /* CONFIG_I2C_TARGET */
	uint32_t count;
	uint32_t dev_config;
	int target_tx_valid;
	int target_rx_valid;
	struct k_sem *i2c_busy_sem;
	bool is_target;
};

#if (CONFIG_I2C_SCL_LOW_TIMEOUT != 0)
static int i2c_mspm0_configure_timeout(const struct device *dev, uint32_t period,
				       uint32_t timeout_ms)
{
	const struct i2c_mspm0_config *config = dev->config;
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	uint32_t clock_rate;
	uint32_t tick_cycles;
	uint64_t timeout_cycles;
	uint32_t ticks_needed;
	uint16_t counter_value;
	int ret;

	ret = clock_control_get_rate(clk_dev, (struct mspm0_sys_clock *)config->clock_subsys,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	/* Each count is equal to (1 + TPR) * 12 functional clocks */
	tick_cycles = (period + 1) * 12;
	timeout_cycles = (uint64_t)timeout_ms * (clock_rate / 1000);
	ticks_needed = (timeout_cycles + tick_cycles - 1) / tick_cycles;
	/* Lower 4-bits of counter are automatically set to 0x0 */
	counter_value = ticks_needed >> 4;

	if (counter_value > 0xFF) {
		return -EINVAL;
	}

	DL_I2C_enableTimeoutA(config->base);
	DL_I2C_setTimeoutACount(config->base, counter_value);
	return 0;
}
#endif /* CONFIG_I2C_SCL_LOW_TIMEOUT != 0 */

static int i2c_mspm0_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	uint32_t clock_rate;
	uint32_t desired_speed;
	int32_t period;
	int ret;

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clock_subsys,
				     &clock_rate);
	if (ret < 0) {
		ret = -ENODEV;
		goto out;
	}

	if (dev_config & I2C_MSG_ADDR_10_BITS) {
		ret = -EINVAL;
		goto out;
	}

	k_sem_take(data->i2c_busy_sem, K_FOREVER);

	/* Config I2C speed */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		desired_speed = 100000;
		break;
	case I2C_SPEED_FAST:
		desired_speed = 400000;
		break;
	default:
		ret = -EINVAL;
		goto sem_give;
	}

	/* Calculate the timer period based on the desired speed and clock rate.
	 * This function works out to be ceil(clockRate/(desiredSpeed*10)) - 1
	 */
	period = clock_rate / (desired_speed * 10);
	period -= (clock_rate % (desired_speed * 10) == 0) ? 1 : 0;

	/* Set the I2C speed */
	if (period <= 0) {
		ret = -EINVAL;
		goto sem_give;
	}

	DL_I2C_setTimerPeriod(config->base, period);
	data->dev_config = dev_config;

#if (CONFIG_I2C_SCL_LOW_TIMEOUT != 0)
	ret = i2c_mspm0_configure_timeout(dev, period, CONFIG_I2C_SCL_LOW_TIMEOUT);
#endif /* CONFIG_I2C_SCL_LOW_TIMEOUT */

	/* Config other settings */
	DL_I2C_setControllerTXFIFOThreshold(config->base, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
	DL_I2C_setControllerRXFIFOThreshold(config->base, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
	DL_I2C_enableControllerClockStretching(config->base);

	/* Configure Interrupts */
	DL_I2C_enableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);

	/* Enable module */
	DL_I2C_enableController(config->base);

sem_give:
	k_sem_give(data->i2c_busy_sem);
out:
	return ret;
}

static int i2c_mspm0_init(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	int ret;
	uint32_t speed_config;

	/* Init power */
	DL_I2C_reset(config->base);
	DL_I2C_enablePower(config->base);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);
	DL_I2C_resetControllerTransfer(config->base);
#ifdef CONFIG_I2C_TARGET
	/* Workaround for errata I2C_ERR_04 */
	DL_I2C_disableTargetWakeup(config->base);
#endif
	/* Init GPIO */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Config clocks and analog filter */
	DL_I2C_setClockConfig(config->base, &config->gI2CClockConfig);
	DL_I2C_disableAnalogGlitchFilter(config->base);

	/* Set frequency */
	speed_config = i2c_map_dt_bitrate(config->bitrate);
	i2c_mspm0_configure(dev, speed_config);

	/* Enable interrupts */
	config->irq_config_func(dev);

	return 0;
}

static int i2c_mspm0_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_mspm0_data *data = dev->data;
	*dev_config = data->dev_config;
	return 0;
}

static int i2c_mspm0_reset_peripheral_controller(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	int ret = 0;

	DL_I2C_reset(config->base);
	DL_I2C_disablePower(config->base);

	DL_I2C_enablePower(config->base);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	DL_I2C_disableTargetWakeup(config->base);

	/* Config clocks and analog filter */
	DL_I2C_setClockConfig(config->base, &config->gI2CClockConfig);
	DL_I2C_disableAnalogGlitchFilter(config->base);

	/* Configure Controller Mode */
	DL_I2C_resetControllerTransfer(config->base);

	/* Config other settings */
	DL_I2C_setControllerTXFIFOThreshold(config->base, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
	DL_I2C_setControllerRXFIFOThreshold(config->base, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
	DL_I2C_enableControllerClockStretching(config->base);

	/* Configure Interrupts */
	DL_I2C_clearInterruptStatus(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);
	DL_I2C_enableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);

	/* Enable module */
	DL_I2C_enableController(config->base);

	return ret;
}

static int i2c_mspm0_receive(const struct device *dev, struct i2c_msg msg, uint16_t addr)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	/* Update cached msg and addr */
	data->msg = msg;
	data->addr = addr;

	/* Send a read request to Target */
	data->count = 0;
	data->state = I2C_MSPM0_RX_STARTED;
	DL_I2C_startControllerTransfer(config->base, data->addr, DL_I2C_CONTROLLER_DIRECTION_RX,
				       data->msg.len);

	while (!(DL_I2C_getControllerStatus(config->base) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
	}

	if (data->state == I2C_MSPM0_TIMEOUT) {
		return -ETIMEDOUT;
	}

	/* If error, return error */
	if (((DL_I2C_getControllerStatus(config->base) & DL_I2C_CONTROLLER_STATUS_ERROR) != 0x00) ||
	    (data->state == I2C_MSPM0_ERROR)) {
		return -EIO;
	}

	return 0;
}

static int i2c_mspm0_transmit(const struct device *dev, struct i2c_msg msg, uint16_t addr)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	/* Update cached msg, addr, and state */
	data->msg = msg;
	data->addr = addr;
	data->state = I2C_MSPM0_IDLE;

	/* Flush anything that is left in the stale FIFO */
	DL_I2C_flushControllerTXFIFO(config->base);

	/*
	 *  Fill the FIFO
	 *  The FIFO is 8-bytes deep, and this function will return number
	 *  of bytes written to FIFO
	 */
	data->count = DL_I2C_fillControllerTXFIFO(config->base, data->msg.buf, data->msg.len);

	/* Enable TXFIFO trigger interrupt if there are more bytes to send */
	if (data->count < data->msg.len) {
		DL_I2C_enableInterrupt(config->base, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
	} else {
		DL_I2C_disableInterrupt(config->base, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
	}

	/*
	 * Send the packet to the controller.
	 * This function will send Start + Stop automatically
	 */
	data->state = I2C_MSPM0_TX_STARTED;
	while (!(DL_I2C_getControllerStatus(config->base) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
	}

	DL_I2C_startControllerTransfer(config->base, data->addr, DL_I2C_CONTROLLER_DIRECTION_TX,
				       data->msg.len);

	while (!(DL_I2C_getControllerStatus(config->base) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
	}

	if (data->state == I2C_MSPM0_TIMEOUT) {
		return -ETIMEDOUT;
	}

	/* If error, return error */
	if (((DL_I2C_getControllerStatus(config->base) & DL_I2C_CONTROLLER_STATUS_ERROR) != 0x00) ||
	    (data->state == I2C_MSPM0_ERROR)) {
		return -EIO;
	}

	return 0;
}

static int i2c_mspm0_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	uint32_t dev_config;
	uint8_t *internal_buf = config->merge_buf;
	uint16_t merge_buf_size = config->merge_buf_size;
	uint16_t current_internal_buf_size = 0;
	uint8_t *transaction_buf;
	uint16_t transaction_len;
	struct i2c_msg transaction_msg;
	int j;
	int ret;

	k_sem_take(data->i2c_busy_sem, K_FOREVER);

	if (data->is_target) {
		/* Currently target is registered, initiating transfer is not allowed */
		k_sem_give(data->i2c_busy_sem);
		return -EBUSY;
	}

	/* Transmit each message */
	for (int i = 0; i < num_msgs; i++) {
		bool merge_next_msg =
			(i + 1 < num_msgs) && !(msgs[i].flags & I2C_MSG_STOP) &&
			!(msgs[i + 1].flags & I2C_MSG_RESTART) &&
			((msgs[i].flags & I2C_MSG_WRITE) == (msgs[i + 1].flags & I2C_MSG_WRITE));

		if (merge_next_msg || current_internal_buf_size != 0) {
			if ((current_internal_buf_size + msgs[i].len) > merge_buf_size) {
				LOG_ERR("Need to use the internal driver "
					"buffer but its size is insufficient "
					"(%u + %u > %u). ",
					current_internal_buf_size, msgs[i].len, merge_buf_size);
				ret = -ENOSPC;
				break;
			}
			if (!(msgs[i].flags & I2C_MSG_READ)) {
				memcpy(internal_buf + current_internal_buf_size, msgs[i].buf,
				       msgs[i].len);
			}
			current_internal_buf_size += msgs[i].len;
		}

		/* Continue merging messages */
		if (merge_next_msg) {
			continue;
		}

		/* No merge was performed, send standard frame */
		if (current_internal_buf_size == 0) {
			transaction_buf = msgs[i].buf;
			transaction_len = msgs[i].len;
		} else {
			transaction_buf = internal_buf;
			transaction_len = current_internal_buf_size;
		}
		/* Fill transaction_msg */
		transaction_msg.flags = msgs[i].flags;
		transaction_msg.buf = transaction_buf;
		transaction_msg.len = transaction_len;

		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			ret = i2c_mspm0_receive(dev, transaction_msg, addr);
		} else {
			ret = i2c_mspm0_transmit(dev, transaction_msg, addr);
		}

		if (ret != 0) {
			break;
		}

		/* For merged reads, populate data in original user buffers */
		if ((transaction_msg.flags & I2C_MSG_READ) && (transaction_buf == internal_buf)) {
			j = i;
			while (current_internal_buf_size >= msgs[j].len) {
				current_internal_buf_size -= msgs[j].len;
				memcpy(msgs[j].buf, transaction_buf + current_internal_buf_size,
				       msgs[j].len);
				j--;
			}
		}
	}

	if (ret == -ETIMEDOUT) {
		i2c_mspm0_get_config(dev, &dev_config);
		i2c_mspm0_reset_peripheral_controller(dev);
	}

	k_sem_give(data->i2c_busy_sem);

	if (ret == -ETIMEDOUT) {
		i2c_mspm0_configure(dev, dev_config);
	}

	return ret;
}

#ifdef CONFIG_I2C_TARGET
static int i2c_mspm0_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	/* Device is already registered as target */
	if (data->is_target == true || data->target_config == cfg) {
		return -EINVAL;
	}

	data->target_config = cfg;
	data->target_callbacks = cfg->callbacks;
	data->addr = cfg->address;
	k_sem_take(data->i2c_busy_sem, K_FOREVER);

	if (data->state == I2C_MSPM0_TARGET_PREEMPTED) {
		DL_I2C_clearInterruptStatus(config->base, TI_MSPM0_TARGET_INTERRUPTS);
	}

	DL_I2C_disableController(config->base);
	DL_I2C_disableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);

	DL_I2C_setTargetTXFIFOThreshold(config->base, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
	DL_I2C_setTargetRXFIFOThreshold(config->base, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
	DL_I2C_enableTargetTXTriggerInTXMode(config->base);
	DL_I2C_enableTargetTXEmptyOnTXRequest(config->base);
	DL_I2C_enableTargetClockStretching(config->base);

	DL_I2C_setTargetOwnAddress(config->base, data->addr);
#if (CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS != 0)
	DL_I2C_setTargetOwnAddressAlternate(config->base,
					    CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS);
#if (CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS_MASK != 0)
	DL_I2C_setTargetOwnAddressAlternateMask(config->base,
						CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS_MASK);
#endif /* CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS_MASK != 0 */
	DL_I2C_enableTargetOwnAddressAlternate(config->base);
#endif /* CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS != 0 */

	DL_I2C_clearInterruptStatus(config->base, DL_I2C_INTERRUPT_TARGET_TXFIFO_EMPTY);
	DL_I2C_enableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);
	DL_I2C_enableTarget(config->base);

	data->dev_config &= ~I2C_MODE_CONTROLLER;
	data->is_target = true;
	data->state = I2C_MSPM0_IDLE;

	k_sem_give(data->i2c_busy_sem);
	return 0;
}

static int i2c_mspm0_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	if (data->is_target == false) {
		return 0;
	}

	data->target_config = NULL;
	data->is_target = false;
	k_sem_take(data->i2c_busy_sem, K_FOREVER);

	DL_I2C_disableTarget(config->base);
	DL_I2C_disableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);

	k_sem_give(data->i2c_busy_sem);
	return 0;
}

static int i2c_mspm0_reset_peripheral_target(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	DL_I2C_reset(config->base);
	DL_I2C_disablePower(config->base);

	DL_I2C_enablePower(config->base);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	DL_I2C_disableTargetWakeup(config->base);

	/* Config clocks and analog filter */
	DL_I2C_setClockConfig(config->base, (DL_I2C_ClockConfig *)&config->gI2CClockConfig);
	DL_I2C_disableAnalogGlitchFilter(config->base);

	DL_I2C_setTargetOwnAddress(config->base, data->addr);
	DL_I2C_setTargetTXFIFOThreshold(config->base, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
	DL_I2C_setTargetRXFIFOThreshold(config->base, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
	DL_I2C_enableTargetTXTriggerInTXMode(config->base);
	DL_I2C_enableTargetTXEmptyOnTXRequest(config->base);

#if (CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS != 0)
	if (data->secondaryAddr != 0x00) {
		DL_I2C_setTargetOwnAddressAlternate(config->base, data->secondaryAddr);
		DL_I2C_setTargetOwnAddressAlternateMask(config->base, data->secondaryDontCareMask);
		DL_I2C_enableTargetOwnAddressAlternate(config->base);
	}
#endif

	DL_I2C_clearInterruptStatus(config->base, DL_I2C_INTERRUPT_TARGET_TXFIFO_EMPTY);

	DL_I2C_enableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);

	data->state = I2C_MSPM0_IDLE;

	/* Enable module */
	DL_I2C_enableTarget(config->base);

	return 0;
}
#endif /* CONFIG_I2C_TARGET */

static void i2c_mspm0_isr(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	switch (DL_I2C_getPendingInterrupt(config->base)) {
	case DL_I2C_IIDX_CONTROLLER_RX_DONE:
		data->state = I2C_MSPM0_RX_COMPLETE;
		break;
	case DL_I2C_IIDX_CONTROLLER_TX_DONE:
		DL_I2C_disableInterrupt(config->base, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
		data->state = I2C_MSPM0_TX_COMPLETE;
		break;
	case DL_I2C_IIDX_CONTROLLER_RXFIFO_TRIGGER:
		if (data->state != I2C_MSPM0_RX_COMPLETE) {
			/* Fix for RX_DONE happening before the last RXFIFO_TRIGGER */
			data->state = I2C_MSPM0_RX_INPROGRESS;
		}
		/* Receive all bytes from target */
		while (DL_I2C_isControllerRXFIFOEmpty(config->base) != true) {
			if (data->count < data->msg.len) {
				data->msg.buf[data->count++] =
					DL_I2C_receiveControllerData(config->base);
			} else {
				/* Ignore and remove from FIFO if the buffer is full */
				DL_I2C_receiveControllerData(config->base);
			}
		}
		break;
	case DL_I2C_IIDX_CONTROLLER_TXFIFO_TRIGGER:
		data->state = I2C_MSPM0_TX_INPROGRESS;
		/* Fill TX FIFO with next bytes to send */
		if (data->count < data->msg.len) {
			data->count += DL_I2C_fillControllerTXFIFO(config->base,
								   &data->msg.buf[data->count],
								   data->msg.len - data->count);
		}
		break;
	case DL_I2C_IIDX_CONTROLLER_ARBITRATION_LOST:
	case DL_I2C_IIDX_CONTROLLER_NACK:
		if ((data->state == I2C_MSPM0_RX_STARTED) ||
		    (data->state == I2C_MSPM0_TX_STARTED)) {
			/* NACK interrupt if I2C Target is disconnected */
			data->state = I2C_MSPM0_ERROR;
		}

	/* Not implemented */
	case DL_I2C_IIDX_CONTROLLER_RXFIFO_FULL:
	case DL_I2C_IIDX_CONTROLLER_TXFIFO_EMPTY:
	case DL_I2C_IIDX_CONTROLLER_START:
	case DL_I2C_IIDX_CONTROLLER_STOP:
	case DL_I2C_IIDX_CONTROLLER_EVENT1_DMA_DONE:
	case DL_I2C_IIDX_CONTROLLER_EVENT2_DMA_DONE:
		break;
		/* target interrupts */
#ifdef CONFIG_I2C_TARGET
	case DL_I2C_IIDX_TARGET_START:
		if (k_sem_take(data->i2c_busy_sem, K_NO_WAIT) != 0 &&
		    data->state == I2C_MSPM0_IDLE) {
			/* we do not have control of the peripheral. And it is not a restart. Some
			 * configuration or other function is making modifications
			 * to the peripheral so we must cancel the transaction. The
			 * only supported way to cancel the transaction is disabling
			 * the target peripheral entirely.
			 */
			DL_I2C_disableTarget(config->base);
			data->state = I2C_MSPM0_TARGET_PREEMPTED;
		} else {
			/* semaphore has successfully been obtained */
			data->state = I2C_MSPM0_TARGET_STARTED;

			/* Flush TX FIFO to clear out any stale data */
			DL_I2C_flushTargetTXFIFO(config->base);
		}
		break;
	case DL_I2C_IIDX_TARGET_RX_DONE:
		if (data->state == I2C_MSPM0_TARGET_STARTED) {
			data->state = I2C_MSPM0_TARGET_RX_INPROGRESS;
			if (data->target_callbacks->write_requested != NULL) {
#if (CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS != 0)
				if (data->secondaryAddr != 0x00) {
					data->target_config->address =
						DL_I2C_getTargetAddressMatch(config->base);
				}
#endif
				data->target_rx_valid = data->target_callbacks->write_requested(
					data->target_config);
			}
		}
		/* Store received data in buffer */
		if (data->target_callbacks->write_received != NULL) {
			uint8_t nextByte;

			while (DL_I2C_isTargetRXFIFOEmpty(config->base) != true) {
				if (data->target_rx_valid == 0) {
					nextByte = DL_I2C_receiveTargetData(config->base);
					data->target_rx_valid =
						data->target_callbacks->write_received(
							data->target_config, nextByte);

					if (data->target_rx_valid == 0) {
						DL_I2C_setTargetACKOverrideValue(
							config->base,
							DL_I2C_TARGET_RESPONSE_OVERRIDE_VALUE_ACK);
					} else {
						DL_I2C_setTargetACKOverrideValue(
							config->base,
							DL_I2C_TARGET_RESPONSE_OVERRIDE_VALUE_NACK);
					}

				} else {
					/* Prevent overflow and just ignore data */
					DL_I2C_receiveTargetData(config->base);
					DL_I2C_setTargetACKOverrideValue(
						config->base,
						DL_I2C_TARGET_RESPONSE_OVERRIDE_VALUE_NACK);
				}
			}
		}

		break;
	case DL_I2C_IIDX_TARGET_TXFIFO_EMPTY:
		if (data->state == I2C_MSPM0_TARGET_STARTED) {
			/* First byte detected from a read operation. */
			data->state = I2C_MSPM0_TARGET_TX_INPROGRESS;
			if (data->target_callbacks->read_requested != NULL) {
				uint8_t nextByte;
#if (CONFIG_I2C_MSPM0_ALTERNATE_TARGET_ADDRESS != 0)
				if (data->secondaryAddr != 0x00) {
					data->target_config->address =
						DL_I2C_getTargetAddressMatch(config->base);
				}
#endif
				data->target_tx_valid = data->target_callbacks->read_requested(
					data->target_config, &nextByte);
				if (data->target_tx_valid == 0) {
					DL_I2C_transmitTargetData(config->base, nextByte);
				} else {
					/* In this case, no new data is desired to be filled, thus
					 * 0's are transmitted
					 */
					DL_I2C_transmitTargetData(config->base, 0x00);
				}
			} else {
				/* read_requested function is not found. The target data will
				 * continue to transmit to fulfill the error and not hang
				 * the controller by stretching indefinitely
				 */
				DL_I2C_transmitTargetDataCheck(config->base, 0xFF);
				data->target_tx_valid = -1;
			}
		} else {
			/* still using the FIFO, we call read_processed in order to add
			 * additional data rather than from a buffer. If the write-received
			 * function chooses to return 0 (no more data present), then 0's will
			 * be filled in
			 */
			if (data->target_callbacks->read_processed != NULL) {
				uint8_t nextByte;

				if (data->target_tx_valid == 0) {
					data->target_tx_valid =
						data->target_callbacks->read_processed(
							data->target_config, &nextByte);
				}

				if (data->target_tx_valid == 0) {
					DL_I2C_transmitTargetData(config->base, nextByte);
				} else {
					/* In this case, no new data is desired to be filled, thus
					 * 0's are transmitted
					 */
					DL_I2C_transmitTargetData(config->base, 0x00);
				}
			} else {
				/* Read_processed function is not found. The target data will
				 * continue to transmit to fulfill the error and not hang
				 * the controller by stretching indefinitely
				 */
				DL_I2C_transmitTargetDataCheck(config->base, 0xFF);
				data->target_tx_valid = -1;
			}
		}
		break;
	case DL_I2C_IIDX_TARGET_STOP:
		data->state = I2C_MSPM0_IDLE;
		k_sem_give(data->i2c_busy_sem);
		if (data->target_callbacks->stop) {
			data->target_callbacks->stop(data->target_config);
		}
		break;
	case DL_I2C_IIDX_TARGET_RXFIFO_TRIGGER:
	case DL_I2C_IIDX_TARGET_RXFIFO_FULL:
	case DL_I2C_IIDX_TARGET_GENERAL_CALL:
	case DL_I2C_IIDX_TARGET_EVENT1_DMA_DONE:
	case DL_I2C_IIDX_TARGET_EVENT2_DMA_DONE:
		break;
#endif /* CONFIG_I2C_TARGET */
	case DL_I2C_IIDX_TIMEOUT_A:
		data->state = I2C_MSPM0_TIMEOUT;

		if (data->is_target == false) {
			DL_I2C_disableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);
			DL_I2C_clearInterruptStatus(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);
			DL_I2C_flushControllerTXFIFO(config->base);

		}
#ifdef CONFIG_I2C_TARGET
		else {
			DL_I2C_disableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);
			DL_I2C_clearInterruptStatus(config->base, TI_MSPM0_TARGET_INTERRUPTS);
			if (data->target_callbacks->stop) {
				data->target_callbacks->stop(data->target_config);
			}
			i2c_mspm0_reset_peripheral_target(dev);
			k_sem_give(data->i2c_busy_sem);
		}
		break;
#endif
	default:
		break;
	}
}

static DEVICE_API(i2c, i2c_mspm0_driver_api) = {
	.configure = i2c_mspm0_configure,
	.get_config = i2c_mspm0_get_config,
	.transfer = i2c_mspm0_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_mspm0_target_register,
	.target_unregister = i2c_mspm0_target_unregister,
#endif
};

/* Macros to assist with the device-specific initialization */
#define MERGE_BUF_SIZE(index)                                                                      \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(i2c##index), merge_buf_size),			\
			(DT_PROP(DT_NODELABEL(i2c##index), merge_buf_size)), (0))
#define USES_MERGE_BUF(index) COND_CODE_0(MERGE_BUF_SIZE(index), (0), (1))

#define I2C_MSPM0_CONFIG_IRQ_FUNC_DECLARE(index)                                                   \
	static void i2c_mspm0_irq_config_func_##index(const struct device *dev)

#define I2C_MSPM0_CONFIG_IRQ_FUNC(index)                                                           \
	static void i2c_mspm0_irq_config_func_##index(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2c_mspm0_isr,      \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define MSP_I2C_INIT_FN(index)                                                                     \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct mspm0_sys_clock mspm0_i2c_clockSys##index =                            \
		MSPM0_CLOCK_SUBSYS_FN(index);                                                      \
                                                                                                   \
	I2C_MSPM0_CONFIG_IRQ_FUNC_DECLARE(index);                                                  \
                                                                                                   \
	IF_ENABLED(USES_MERGE_BUF(index),                                                          \
		(static uint8_t mspm0_i2c_##index_msg_buf[MERGE_BUF_SIZE(index)];));              \
	static const struct i2c_mspm0_config i2c_mspm0_cfg_##index = {                             \
		.base = (I2C_Regs *)DT_INST_REG_ADDR(index),                                       \
		.clock_subsys = &mspm0_i2c_clockSys##index,                                        \
		.bitrate = DT_INST_PROP(index, clock_frequency),                                   \
		.merge_buf_size = MERGE_BUF_SIZE(index),                                           \
		IF_ENABLED(USES_MERGE_BUF(index),                                                  \
			(.merge_buf = mspm0_i2c_##index_msg_buf,)) .pinctrl =      \
						   PINCTRL_DT_INST_DEV_CONFIG_GET(index),          \
					  .irq_config_func = i2c_mspm0_irq_config_func_##index,    \
					  .gI2CClockConfig = {                                     \
						  .clockSel = MSPM0_CLOCK_PERIPH_REG_MASK(         \
							  DT_INST_CLOCKS_CELL(index, clk)),        \
						  .divideRatio = DL_I2C_CLOCK_DIVIDE_1,            \
					  }};                                                      \
	K_SEM_DEFINE(i2c_busy_sem_##index, 1, 1);                                                  \
	static struct i2c_mspm0_data i2c_mspm0_data_##index = {                                    \
		.i2c_busy_sem = &i2c_busy_sem_##index,                                             \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_mspm0_init, NULL, &i2c_mspm0_data_##index,            \
				  &i2c_mspm0_cfg_##index, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,   \
				  &i2c_mspm0_driver_api);                                          \
                                                                                                   \
	I2C_MSPM0_CONFIG_IRQ_FUNC(index)

DT_INST_FOREACH_STATUS_OKAY(MSP_I2C_INIT_FN)
