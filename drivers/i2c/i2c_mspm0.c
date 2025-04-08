/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_i2c

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <soc.h>

/* Logging includes */
#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_mspm0);
#include "i2c-priv.h"

/* Driverlib includes */
#include <ti/driverlib/dl_i2c.h>
#include <ti/driverlib/dl_gpio.h>

#define TI_MSPM0_TARGET_TIMEOUT_50_MS 	(193)

#define TI_MSPM0_CONTROLLER_INTERRUPTS	\
	(DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST |	\
	DL_I2C_INTERRUPT_CONTROLLER_NACK |	\
	DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER |	\
	DL_I2C_INTERRUPT_CONTROLLER_RX_DONE | DL_I2C_INTERRUPT_CONTROLLER_TX_DONE \
	IF_ENABLED(CONFIG_I2C_CONTROLLER_TIMEOUT, (| DL_I2C_INTERRUPT_TIMEOUT_A)))

#define TI_MSPM0_TARGET_INTERRUPTS	\
	(DL_I2C_INTERRUPT_TARGET_RX_DONE | DL_I2C_INTERRUPT_TARGET_TXFIFO_EMPTY |	\
	DL_I2C_INTERRUPT_TARGET_START |	DL_I2C_INTERRUPT_TARGET_STOP	\
	IF_ENABLED(CONFIG_I2C_TARGET_TIMEOUT, (| DL_I2C_INTERRUPT_TIMEOUT_A)))

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
	I2C_Regs * base;
	uint32_t bitrate;
	DL_I2C_ClockConfig gI2CClockConfig;
	const struct mspm0_clockSys *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
	void (*interrupt_init_function)(const struct device *dev);
};

struct i2c_mspm0_data {
	volatile enum i2c_mspm0_state state; /* Current state of I2C transmission */
	struct i2c_msg msg;		/* Cache msg */
	uint16_t addr;			/* Cache slave address */
#ifdef CONFIG_I2C_MSPM0_MULTI_TARGET_ADDRESS
	uint16_t secondaryAddr;	/* Secondary slave address */
	uint16_t secondaryDontCareMask; /* Secondary don't care mask. Muxed with
								     * the secondary address to create a range
									 * of acceptable addresses. */
#endif
	uint32_t count;			/* Count for progress in I2C transmission */
	uint32_t dev_config;	/* Configuration last passed */
	uint32_t is_target;
	const struct i2c_target_callbacks *target_callbacks;
	struct i2c_target_config *target_config;
	int target_tx_valid;
	int target_rx_valid;
	struct k_sem i2c_busy_sem;
};

static int i2c_mspm0_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(clkmux));
	uint32_t clockRate;
	uint32_t desiredSpeed;
	int32_t period;
	int ret;

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clock_subsys, &clockRate);

	if(ret < 0){
		return -ENODEV;
	}

	k_sem_take(&data->i2c_busy_sem, K_FOREVER);

	/* 10-bit addressing not supported */
	if (dev_config & I2C_MSG_ADDR_10_BITS) {
		k_sem_give(&data->i2c_busy_sem);
		return -EINVAL;
	}

	/* Config I2C speed */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		desiredSpeed = 100000;
		break;
	case I2C_SPEED_FAST:
		desiredSpeed = 400000;
		break;
	case I2C_SPEED_FAST_PLUS:
		desiredSpeed = 1000000;
		break;
	default:
		k_sem_give(&data->i2c_busy_sem);
		return -EINVAL;
	}

	/* Calculate the timer period based on the desired speed and clock rate.
	 * This function works out to be ceil(clockRate/(desiredSpeed*10)) - 1
	 */

	period = clockRate / (desiredSpeed * 10);
	period -= (clockRate % (desiredSpeed * 10) == 0)?1:0;

	/* Set the I2C speed */
	if(period <= 0){
		ret = -EINVAL;
	} else {
		ret = 0;
		DL_I2C_setTimerPeriod(config->base, period);
	}

	data->dev_config = dev_config;

	k_sem_give(&data->i2c_busy_sem);

	return ret;
}

static int i2c_mspm0_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_mspm0_data *data = dev->data;

	*dev_config = data->dev_config;

	return 0;
}

static int i2c_mspm0_reset_peripheral_target(const struct device *dev){
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	DL_I2C_reset(config->base);
	DL_I2C_disablePower(config->base);

	DL_I2C_enablePower(config->base);
	delay_cycles(POWER_STARTUP_DELAY);

	DL_I2C_disableTargetWakeup(config->base);

	/* Config clocks and analog filter */
	DL_I2C_setClockConfig(config->base,
			(DL_I2C_ClockConfig *)&config->gI2CClockConfig);
	DL_I2C_disableAnalogGlitchFilter(config->base);

#ifdef CONFIG_I2C_TARGET_TIMEOUT
	DL_I2C_enableTimeoutA(config->base);
    DL_I2C_setTimeoutACount(config->base, TI_MSPM0_TARGET_TIMEOUT_50_MS);
#endif /* CONFIG_I2C_TARGET_TIMEOUT */


	DL_I2C_setTargetOwnAddress(config->base, data->addr);
	DL_I2C_setTargetTXFIFOThreshold(config->base, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
	DL_I2C_setTargetRXFIFOThreshold(config->base, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
	DL_I2C_enableTargetTXTriggerInTXMode(config->base);
	DL_I2C_enableTargetTXEmptyOnTXRequest(config->base);

#ifdef CONFIG_I2C_MSPM0_MULTI_TARGET_ADDRESS
	if(data->secondaryAddr != 0x00){
		DL_I2C_setTargetOwnAddressAlternate(config->base, data->secondaryAddr);
		DL_I2C_setTargetOwnAddressAlternateMask(config->base, data->secondaryDontCareMask);
		DL_I2C_enableTargetOwnAddressAlternate(config->base);
	}
#endif

	DL_I2C_clearInterruptStatus(
		config->base, DL_I2C_INTERRUPT_TARGET_TXFIFO_EMPTY);

	DL_I2C_enableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);

	data->state = I2C_MSPM0_IDLE;
	/* erase the timeout flag */
	data->target_config->flags &= ~I2C_TARGET_FLAGS_ERROR_TIMEOUT;

	/* Enable module */
	DL_I2C_enableTarget(config->base);

	return 0;
}

static int i2c_mspm0_reset_peripheral_controller(const struct device *dev){
	const struct i2c_mspm0_config *config = dev->config;

	DL_I2C_reset(config->base);
	DL_I2C_disablePower(config->base);

	DL_I2C_enablePower(config->base);
	delay_cycles(POWER_STARTUP_DELAY);

	DL_I2C_disableTargetWakeup(config->base);

	/* Config clocks and analog filter */
	DL_I2C_setClockConfig(config->base,
			(DL_I2C_ClockConfig *)&config->gI2CClockConfig);
	DL_I2C_disableAnalogGlitchFilter(config->base);

	/* Configure Controller Mode */
	DL_I2C_resetControllerTransfer(config->base);

#ifdef CONFIG_I2C_CONTROLLER_TIMEOUT
	DL_I2C_enableTimeoutA(config->base);
    DL_I2C_setTimeoutACount(config->base, TI_MSPM0_TARGET_TIMEOUT_50_MS);
#endif /* CONFIG_I2C_TARGET_TIMEOUT */

	/* Config other settings */
	DL_I2C_setControllerTXFIFOThreshold(config->base, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
	DL_I2C_setControllerRXFIFOThreshold(config->base, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
	DL_I2C_enableControllerClockStretching(config->base);

	/* Configure Interrupts */
	DL_I2C_clearInterruptStatus(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);
	DL_I2C_enableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);

	/* Enable module */
	DL_I2C_enableController(config->base);

	return 0;
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
	DL_I2C_startControllerTransfer(config->base, data->addr,
				       DL_I2C_CONTROLLER_DIRECTION_RX, data->msg.len);


	while (!(DL_I2C_getControllerStatus(config->base) & DL_I2C_CONTROLLER_STATUS_IDLE));

	if(data->state == I2C_MSPM0_TIMEOUT){
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

	/* Update cached msg and addr */
	data->msg = msg;
	data->addr = addr;

	/* Update the state */
	data->state = I2C_MSPM0_IDLE;

	/* Flush anything that is left in the stale FIFO */
	DL_I2C_flushControllerTXFIFO(config->base);

	/*
	 * Fill the FIFO
	 *  The FIFO is 8-bytes deep, and this function will return number
	 *  of bytes written to FIFO
	 */
	data->count =
		DL_I2C_fillControllerTXFIFO(config->base, data->msg.buf, data->msg.len);

	/* Enable TXFIFO trigger interrupt if there are more bytes to send */
	if (data->count < data->msg.len) {
		DL_I2C_enableInterrupt(config->base,
					DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
	} else {
		DL_I2C_disableInterrupt(config->base,
					DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
	}

	/*
	 * Send the packet to the controller.
	 * This function will send Start + Stop automatically
	 */
	data->state = I2C_MSPM0_TX_STARTED;
	while (!(DL_I2C_getControllerStatus(config->base) &
		 DL_I2C_CONTROLLER_STATUS_IDLE))
		;

	DL_I2C_startControllerTransfer(config->base, data->addr,
					DL_I2C_CONTROLLER_DIRECTION_TX, data->msg.len);

	while (!(DL_I2C_getControllerStatus(config->base) & DL_I2C_CONTROLLER_STATUS_IDLE));

	if(data->state == I2C_MSPM0_TIMEOUT){
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
	struct i2c_mspm0_data *data = dev->data;
	uint32_t config;
	int ret = 0;

	/* Sending address with no data not supported */
	if (num_msgs == 0) {
		return -EINVAL;
	}

	k_sem_take(&data->i2c_busy_sem, K_FOREVER);

	if (data->is_target) {
		/* currently target is registered. Controller is disabled */
		k_sem_give(&data->i2c_busy_sem);
		return -EBUSY;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_get(dev);
#endif

	/* Transmit each message */
	for (int i = 0; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_mspm0_transmit(dev, msgs[i], addr);
		} else {
			ret = i2c_mspm0_receive(dev, msgs[i], addr);
		}
	}

	if(ret == -ETIMEDOUT){
		i2c_mspm0_get_config(dev, &config);
		i2c_mspm0_reset_peripheral_controller(dev);
	}

	k_sem_give(&data->i2c_busy_sem);

	if(ret == -ETIMEDOUT){
		i2c_mspm0_configure(dev, config);
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_put(dev);
#endif

	return ret;
}

static int i2c_mspm0_target_register(const struct device *dev,
					  struct i2c_target_config *target_config)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	if (target_config == NULL) {
		return -EINVAL;
	}

	if (target_config->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		return -ENOTSUP;
	}

#ifndef CONFIG_I2C_MSPM0_MULTI_TARGET_ADDRESS
	if(target_config->flags & I2C_TARGET_FLAGS_SECONDARY_ADDR){
		return -ENTOSUP;
	}
#endif

	k_sem_take(&data->i2c_busy_sem, K_FOREVER);

	if (data->is_target == true) {
		/* device is already configured as a target */
		if (target_config != data->target_config) {
#ifdef CONFIG_I2C_MSPM0_MULTI_TARGET_ADDRESS
			if (target_config->flags & I2C_TARGET_FLAGS_SECONDARY_ADDR){
				/* parse the 8 - bit mask into don't care portion and
				 * the address itself secondary address in the form:
				 * mask | address. For example, if the value in address is
				 * 0x031A, the range of addresses accepted is
				 * 0'b00110XX, such that 0x18, 0x19, 0x1A, and 0x1B are matched.
				 */

				data->secondaryAddr = target_config->address & 0xFF;
				data->secondaryDontCareMask = (0x0000FF00 & (uint32_t) target_config->address) >> 8;
				DL_I2C_setTargetOwnAddressAlternate(config->base, data->secondaryAddr);
				DL_I2C_setTargetOwnAddressAlternateMask(config->base, data->secondaryDontCareMask);
				DL_I2C_enableTargetOwnAddressAlternate(config->base);
			} else {
#endif
				/* a new target configuration has been given. Reconfigure
				 * address and callbacks
				 */
				DL_I2C_disableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);
				DL_I2C_disableTarget(config->base);
				DL_I2C_setTargetOwnAddress(config->base, target_config->address);
				data->target_config = target_config;
				data->target_callbacks = target_config->callbacks;

#ifdef CONFIG_I2C_MSPM0_MULTI_TARGET_ADDRESS
				/* clear any previous secondary configuration */
				data->secondaryAddr = 0x00;
				DL_I2C_disableTargetOwnAddressAlternate(config->base);
			}
#endif
		if (data->state == I2C_MSPM0_TARGET_PREEMPTED) {
				DL_I2C_clearInterruptStatus(config->base,
							TI_MSPM0_TARGET_INTERRUPTS);
				data->state = I2C_MSPM0_IDLE;
			}
		}
		k_sem_give(&data->i2c_busy_sem);

		DL_I2C_enableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);
		DL_I2C_enableTarget(config->base);
		return 0;
	}

	/* Disable the controller and configure the device to run as a target */
	DL_I2C_disableController(config->base);

	data->target_callbacks = target_config->callbacks;
	data->target_config = target_config;
	data->dev_config &= ~I2C_MODE_CONTROLLER;
	data->is_target = true;
	data->state = I2C_MSPM0_IDLE;

	DL_I2C_setTargetOwnAddress(config->base, target_config->address);
	DL_I2C_setTargetTXFIFOThreshold(config->base, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
	DL_I2C_setTargetRXFIFOThreshold(config->base, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
	DL_I2C_enableTargetTXTriggerInTXMode(config->base);
	DL_I2C_enableTargetTXEmptyOnTXRequest(config->base);


	/* reconfigure the interrupt to use a slave isr? */
	DL_I2C_disableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);


	DL_I2C_clearInterruptStatus(
		config->base, DL_I2C_INTERRUPT_TARGET_TXFIFO_EMPTY);


#ifdef CONFIG_I2C_TARGET_TIMEOUT
	DL_I2C_enableTimeoutA(config->base);
    DL_I2C_setTimeoutACount(config->base, TI_MSPM0_TARGET_TIMEOUT_50_MS);
#endif /* CONFIG_I2C_TARGET_TIMEOUT */

	DL_I2C_enableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);

	DL_I2C_enableTarget(config->base);

	k_sem_give(&data->i2c_busy_sem);
	return 0;
}

static int i2c_mspm0_target_unregister(const struct device *dev,
					    struct i2c_target_config *target_config)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	k_sem_take(&data->i2c_busy_sem, K_FOREVER);

	if (data->is_target == false) {
		/* not currently configured as target. Nothing to do. */
		k_sem_give(&data->i2c_busy_sem);
		return 0;
	}

	DL_I2C_disableTarget(config->base);

	/* Configure Controller Mode */
	DL_I2C_resetControllerTransfer(config->base);
	DL_I2C_enableControllerClockStretching(config->base);

	/* reconfigure the interrupt to use a slave isr? */
	DL_I2C_disableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);

	DL_I2C_enableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);

	DL_I2C_enableController(config->base);

	data->dev_config |= I2C_MODE_CONTROLLER;
	data->is_target = false;

	k_sem_give(&data->i2c_busy_sem);
	return 0;
}

static void i2c_mspm0_isr(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	switch (DL_I2C_getPendingInterrupt(config->base)) {
	/* controller interrupts */
	case DL_I2C_IIDX_CONTROLLER_RX_DONE:
		data->state = I2C_MSPM0_RX_COMPLETE;
		break;
	case DL_I2C_IIDX_CONTROLLER_TX_DONE:
		DL_I2C_disableInterrupt(config->base,
					DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
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
	case DL_I2C_IIDX_TARGET_START:
		if (k_sem_take(&data->i2c_busy_sem, K_NO_WAIT) != 0 && data->state == I2C_MSPM0_IDLE) {
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
#ifdef CONFIG_I2C_MSPM0_MULTI_TARGET_ADDRESS
				if(data->secondaryAddr != 0x00){
					data->target_config->address = DL_I2C_getTargetAddressMatch(config->base);
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
					nextByte =
						DL_I2C_receiveTargetData(config->base);
					data->target_rx_valid =
						data->target_callbacks->write_received(
							data->target_config, nextByte);

					if(data->target_rx_valid == 0){
						DL_I2C_setTargetACKOverrideValue(config->base, DL_I2C_TARGET_RESPONSE_OVERRIDE_VALUE_ACK);
					} else {
						DL_I2C_setTargetACKOverrideValue(config->base, DL_I2C_TARGET_RESPONSE_OVERRIDE_VALUE_NACK);
					}

				} else {
					/* Prevent overflow and just ignore data */
					DL_I2C_receiveTargetData(config->base);
					DL_I2C_setTargetACKOverrideValue(config->base, DL_I2C_TARGET_RESPONSE_OVERRIDE_VALUE_NACK);
				}
			}
		}

		break;
	case DL_I2C_IIDX_TARGET_TXFIFO_EMPTY:
		if(data->state == I2C_MSPM0_TARGET_STARTED) {
			/* First byte detected from a read operation. */
			data->state = I2C_MSPM0_TARGET_TX_INPROGRESS;
			if(data->target_callbacks->read_requested != NULL){
				uint8_t nextByte;
#ifdef CONFIG_I2C_MSPM0_MULTI_TARGET_ADDRESS
				if(data->secondaryAddr != 0x00){
					data->target_config->address = DL_I2C_getTargetAddressMatch(config->base);
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
				 * the controller by stretching indefinitely */
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
					data->target_tx_valid = data->target_callbacks->read_processed(
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
				/* read_processed function is not found. The target data will
				 * continue to transmit to fulfill the error and not hang
				 * the controller by stretching indefinitely */
				DL_I2C_transmitTargetDataCheck(config->base, 0xFF);
				data->target_tx_valid = -1;
			}
		}
		break;
	case DL_I2C_IIDX_TARGET_STOP:
		data->state = I2C_MSPM0_IDLE;
		k_sem_give(&data->i2c_busy_sem);
		if (data->target_callbacks->stop) {
			data->target_callbacks->stop(data->target_config);
		}
		break;
	/* Not implemented */
	case DL_I2C_IIDX_TARGET_RXFIFO_TRIGGER:
	case DL_I2C_IIDX_TARGET_RXFIFO_FULL:
	case DL_I2C_IIDX_TARGET_GENERAL_CALL:
	case DL_I2C_IIDX_TARGET_EVENT1_DMA_DONE:
	case DL_I2C_IIDX_TARGET_EVENT2_DMA_DONE:
		break;
	/* Timeout Interrupts */
	case DL_I2C_IIDX_TIMEOUT_A:
		data->state = I2C_MSPM0_TIMEOUT;

		if(!data->is_target){
			// Controller behavior
			DL_I2C_disableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);
			DL_I2C_clearInterruptStatus(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);
			DL_I2C_flushControllerTXFIFO(config->base);

		} else {
			// Target Behavior
			data->target_config->flags |= I2C_TARGET_FLAGS_ERROR_TIMEOUT;

			/* the event based notify will be done using a stop condition,
				* as there is not another known way to notify the application that a
				* timeout has occurred, so we set a custom flag that the application
				* must check.
				*/
			DL_I2C_disableInterrupt(config->base, TI_MSPM0_TARGET_INTERRUPTS);
			DL_I2C_clearInterruptStatus(config->base, TI_MSPM0_TARGET_INTERRUPTS);

			if (data->target_callbacks->stop) {
				int stopReturn = data->target_callbacks->stop(data->target_config);
				if (stopReturn < 0){
					/* Allow stop to determine whether the target is reset */
					i2c_mspm0_reset_peripheral_target(dev);
				}
			} else {
				/* if no stop, just reset anyways */
				i2c_mspm0_reset_peripheral_target(dev);
			}
			k_sem_give(&data->i2c_busy_sem);
		}
		break;
	default:
		break;
	}
}

static int i2c_mspm0_init(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	int ret;

	k_sem_init(&data->i2c_busy_sem, 0, 1);

	/* Init power */
	DL_I2C_reset(config->base);
	DL_I2C_enablePower(config->base);
	delay_cycles(POWER_STARTUP_DELAY);

	DL_I2C_disableTargetWakeup(config->base);

	/* Init GPIO */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Config clocks and analog filter */
	DL_I2C_setClockConfig(config->base,
		(DL_I2C_ClockConfig *)&config->gI2CClockConfig);
	DL_I2C_disableAnalogGlitchFilter(config->base);

	/* Configure Controller Mode */
	DL_I2C_resetControllerTransfer(config->base);

#ifdef CONFIG_I2C_CONTROLLER_TIMEOUT
	DL_I2C_enableTimeoutA(config->base);
    DL_I2C_setTimeoutACount(config->base, TI_MSPM0_TARGET_TIMEOUT_50_MS);
#endif /* CONFIG_I2C_TARGET_TIMEOUT */


	/* Set frequency */
	uint32_t speed_config = i2c_map_dt_bitrate(config->bitrate);

	k_sem_give(&data->i2c_busy_sem);
	i2c_mspm0_configure(dev, speed_config);

	/* Config other settings */
	DL_I2C_setControllerTXFIFOThreshold(config->base, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
	DL_I2C_setControllerRXFIFOThreshold(config->base, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
	DL_I2C_enableControllerClockStretching(config->base);

	/* Configure Interrupts */
	DL_I2C_enableInterrupt(config->base, TI_MSPM0_CONTROLLER_INTERRUPTS);

	/* Enable interrupts */
	config->interrupt_init_function(dev);
	if (DL_I2C_isControllerEnabled(config->base)) {
		DL_I2C_disableController(config->base);
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_enable(dev);
#endif

	return 0;
}

static const struct i2c_driver_api i2c_mspm0_driver_api = {
	.configure = i2c_mspm0_configure,
	.get_config = i2c_mspm0_get_config,
	.transfer = i2c_mspm0_transfer,
	.target_register = i2c_mspm0_target_register,
	.target_unregister = i2c_mspm0_target_unregister,
};

#ifdef CONFIG_PM_DEVICE

static int i2c_mspm0_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (!data->is_target) {
			DL_I2C_enableController(config->base);
		} else {
			DL_I2C_enableTarget(config->base);
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (!data->is_target) {
			DL_I2C_disableController(config->base);
		} else {
			DL_I2C_disableTarget(config->base);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#endif

/* Macros to assist with the device-specific initialization */
#define INTERRUPT_INIT_FUNCTION_DECLARATION(index)	\
	static void i2c_mspm0_interrupt_init_##index(const struct device *dev)

#define INTERRUPT_INIT_FUNCTION(index)	\
	static void i2c_mspm0_interrupt_init_##index(const struct device *dev)	\
	{	\
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),	\
			i2c_mspm0_isr, DEVICE_DT_INST_GET(index), 0);	\
		irq_enable(DT_INST_IRQN(index));	\
	}

#define MSP_I2C_INIT_FN(index)		\
	\
	PINCTRL_DT_INST_DEFINE(index);	\
	\
	static const struct mspm0_clockSys mspm0_i2c_clockSys##index = MSPM0_CLOCK_SUBSYS_FN(index);\
	\
	INTERRUPT_INIT_FUNCTION_DECLARATION(index);	\
	\
	static const struct i2c_mspm0_config i2c_mspm0_cfg_##index = {		\
		.base = (I2C_Regs *) DT_INST_REG_ADDR(index),					\
		.clock_subsys = &mspm0_i2c_clockSys##index,			\
		.bitrate = DT_INST_PROP(index, clock_frequency),	\
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),				\
		.interrupt_init_function = i2c_mspm0_interrupt_init_##index,	\
		.gI2CClockConfig = {	\
			.clockSel = (DT_INST_CLOCKS_CELL(index, bus) & MSPM0_CLOCK_SEL_MASK),	\
			.divideRatio = DL_I2C_CLOCK_DIVIDE_1,	\
		}	\
	};		\
	\
	static struct i2c_mspm0_data i2c_mspm0_data_##index = { 	\
		.secondaryAddr = 0x00, \
	};	\
	\
	PM_DEVICE_DT_INST_DEFINE(index, i2c_mspm0_pm_action);			\
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_mspm0_init, PM_DEVICE_DT_INST_GET(index),				\
		&i2c_mspm0_data_##index, &i2c_mspm0_cfg_##index, POST_KERNEL,	\
		CONFIG_I2C_INIT_PRIORITY, &i2c_mspm0_driver_api);				\
	\
	INTERRUPT_INIT_FUNCTION(index)

DT_INST_FOREACH_STATUS_OKAY(MSP_I2C_INIT_FN)
