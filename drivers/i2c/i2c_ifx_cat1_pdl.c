/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief I2C driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_i2c_pdl

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_infineon_cat1, CONFIG_I2C_LOG_LEVEL);

#include "cy_scb_i2c.h"

#define I2C_CAT1_EVENTS_MASK                                                                       \
	(CY_SCB_I2C_MASTER_WR_CMPLT_EVENT | CY_SCB_I2C_MASTER_RD_CMPLT_EVENT |                     \
	 CY_SCB_I2C_MASTER_ERR_EVENT)

#define I2C_CAT1_SLAVE_EVENTS_MASK                                                                 \
	(CY_SCB_I2C_SLAVE_READ_EVENT | CY_SCB_I2C_SLAVE_WRITE_EVENT |                              \
	 CY_SCB_I2C_SLAVE_RD_BUF_EMPTY_EVENT | CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT |                   \
	 CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT | CY_SCB_I2C_SLAVE_RD_BUF_EMPTY_EVENT |                   \
	 CY_SCB_I2C_SLAVE_ERR_EVENT)

/* States for ASYNC operations */
#define CAT1_I2C_PENDING_NONE  (0U)
#define CAT1_I2C_PENDING_RX    (1U)
#define CAT1_I2C_PENDING_TX    (2U)
#define CAT1_I2C_PENDING_TX_RX (3U)

/* I2C speed */
#define CAT1_I2C_SPEED_STANDARD_HZ  (100000UL)
#define CAT1_I2C_SPEED_FAST_HZ      (400000UL)
#define CAT1_I2C_SPEED_FAST_PLUS_HZ (1000000UL)

/* Data structure */
struct ifx_cat1_event_callback_data {
	cy_israddress callback;
	void *callback_arg;
};

struct ifx_cat1_i2c_data {
	CySCB_Type *base;
	cy_stc_scb_i2c_context_t context;
	uint16_t pending;
	uint32_t irq_cause;
	cy_stc_scb_i2c_master_xfer_config_t rx_config;
	cy_stc_scb_i2c_master_xfer_config_t tx_config;
	struct ifx_cat1_event_callback_data callback_data;
	struct k_sem operation_sem;
	struct k_sem transfer_sem;
	uint32_t error_status;
	uint32_t async_pending;
	struct ifx_cat1_resource_inst hw_resource;
	struct ifx_cat1_clock clock;
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C)
	uint8_t clock_peri_group;
#endif
	struct i2c_target_config *p_target_config;
	uint8_t i2c_target_wr_byte;
	uint8_t target_wr_buffer[CONFIG_I2C_INFINEON_CAT1_TARGET_BUF];
	uint8_t slave_address;
	uint32_t frequencyhal_hz;
	cy_stc_syspm_callback_t i2c_deep_sleep;
	cy_stc_syspm_callback_params_t i2c_deep_sleep_param;
};

/* Device config structure */
struct ifx_cat1_i2c_config {
	uint32_t master_frequency;
	CySCB_Type *reg_addr;
	const struct pinctrl_dev_config *pcfg;
	uint8_t irq_priority;
	uint32_t irq_num;
	void (*irq_config_func)(const struct device *dev);
	cy_cb_scb_i2c_handle_events_t i2c_handle_events_func;
};

/* Default SCB/I2C configuration structure */
static cy_stc_scb_i2c_config_t _i2c_default_config = {
	.i2cMode = CY_SCB_I2C_MASTER,
	.useRxFifo = false,
	.useTxFifo = true,
	.slaveAddress = 0U,
	.slaveAddressMask = 0U,
	.acceptAddrInFifo = false,
	.ackGeneralAddr = false,
	.enableWakeFromSleep = false,
	.enableDigitalFilter = false,
	.lowPhaseDutyCycle = 8U,
	.highPhaseDutyCycle = 8U,
};

typedef void (*ifx_cat1_i2c_event_callback_t)(void *callback_arg, uint32_t event);

en_clk_dst_t _ifx_cat1_scb_get_clock_index(uint32_t block_num);
int32_t _get_hw_block_num(CySCB_Type *reg_addr);

#ifdef CONFIG_I2C_INFINEON_CAT1_ASYNC
cy_rslt_t _i2c_abort_async(const struct device *dev)
{
	struct ifx_cat1_i2c_data *data = dev->data;

	uint16_t timeout_us = 10000;

	if (data->pending == CAT1_I2C_PENDING_NONE) {
		return CY_RSLT_SUCCESS;
	}

	if (data->pending == CAT1_I2C_PENDING_RX) {
		Cy_SCB_I2C_MasterAbortRead(data->base, &data->context);
	} else {
		Cy_SCB_I2C_MasterAbortWrite(data->base, &data->context);
	}

	/* After abort, next I2C operation can be initiated only after
	 * CY_SCB_I2C_MASTER_BUSY is cleared, so waiting for that event to occur.
	 */
	while ((CY_SCB_I2C_MASTER_BUSY & data->context.masterStatus) && (timeout_us != 0)) {
		Cy_SysLib_DelayUs(1);
		timeout_us--;
	}

	if (0 == timeout_us) {
		return CY_SCB_I2C_MASTER_MANUAL_TIMEOUT;
	}

	data->pending = CAT1_I2C_PENDING_NONE;

	return CY_RSLT_SUCCESS;
}

static void ifx_master_event_handler(void *callback_arg, uint32_t event)
{
	const struct device *dev = (const struct device *)callback_arg;
	struct ifx_cat1_i2c_data *data = dev->data;

	if (((CY_SCB_I2C_MASTER_ERR_EVENT | CY_SCB_I2C_SLAVE_ERR_EVENT) & event) != 0U) {
		/* In case of error abort transfer */
		(void)_i2c_abort_async(dev);
		data->error_status = 1;
	}

	/* Release semaphore if operation complete
	 * When we have pending TX, RX operations, the semaphore will be released
	 * after TX, RX complete.
	 */
	if (((data->async_pending == CAT1_I2C_PENDING_TX_RX) &&
	     ((CY_SCB_I2C_MASTER_RD_CMPLT_EVENT & event) != 0U)) ||
	    (data->async_pending != CAT1_I2C_PENDING_TX_RX)) {

		/* Release semaphore (After I2C async transfer is complete) */
		k_sem_give(&data->transfer_sem);
	}

	if (0 != (CY_SCB_I2C_SLAVE_READ_EVENT & event)) {
		if (data->p_target_config->callbacks->read_requested) {
			data->p_target_config->callbacks->read_requested(data->p_target_config,
									 &data->i2c_target_wr_byte);
			data->context.slaveTxBufferIdx = 0;
			data->context.slaveTxBufferCnt = 0;
			data->context.slaveTxBufferSize = 1;
			data->context.slaveTxBuffer = &data->i2c_target_wr_byte;
		}
	}

	if (0 != (CY_SCB_I2C_SLAVE_RD_BUF_EMPTY_EVENT & event)) {
		if (data->p_target_config->callbacks->read_processed) {
			data->p_target_config->callbacks->read_processed(data->p_target_config,
									 &data->i2c_target_wr_byte);
			data->context.slaveTxBufferIdx = 0;
			data->context.slaveTxBufferCnt = 0;
			data->context.slaveTxBufferSize = 1;
			data->context.slaveTxBuffer = &data->i2c_target_wr_byte;
		}
	}

	if (0 != (CY_SCB_I2C_SLAVE_WRITE_EVENT & event)) {
		Cy_SCB_I2C_SlaveConfigWriteBuf(data->base, (uint8_t *)data->target_wr_buffer,
					       CONFIG_I2C_INFINEON_CAT1_TARGET_BUF, &data->context);
		if (data->p_target_config->callbacks->write_requested) {
			data->p_target_config->callbacks->write_requested(data->p_target_config);
		}
	}

	if (0 != (CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT & event)) {
		if (data->p_target_config->callbacks->write_received) {
			for (int i = 0; i < data->context.slaveRxBufferIdx; i++) {
				data->p_target_config->callbacks->write_received(
					data->p_target_config, data->target_wr_buffer[i]);
			}
		}
		if (data->p_target_config->callbacks->stop) {
			data->p_target_config->callbacks->stop(data->p_target_config);
		}
	}

	if (0 != (CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT & event)) {
		if (data->p_target_config->callbacks->stop) {
			data->p_target_config->callbacks->stop(data->p_target_config);
		}
	}
}
#endif

void ifx_cat1_i2c_register_callback(const struct device *dev,
				    ifx_cat1_i2c_event_callback_t callback, void *callback_arg)
{

	struct ifx_cat1_i2c_data *const data = dev->data;
	const struct ifx_cat1_i2c_config *const config = dev->config;

	data->callback_data.callback = (cy_israddress)callback;
	data->callback_data.callback_arg = callback_arg;

	Cy_SCB_I2C_RegisterEventCallback(data->base, config->i2c_handle_events_func,
					 &(data->context));

	data->irq_cause = 0;
}

#ifdef USE_I2C_SET_PERI_DIVIDER
uint32_t _i2c_set_peri_divider(const struct device *dev, uint32_t freq, bool is_slave)
{
/* Peripheral clock values for different I2C speeds according PDL API Reference Guide */
/* Must be between 1.55 MHz and 12.8 MHz for running i2c master at 100KHz   */
#define _SCB_PERI_CLOCK_SLAVE_STD 8000000
/* Must be between 7.82 MHz and 15.38 MHz for running i2c master at 400KHz  */
#define _SCB_PERI_CLOCK_SLAVE_FST 12500000

/* Must be between 1.55 MHz and 3.2 MHz for running i2c slave at 100KHz     */
#define _SCB_PERI_CLOCK_MASTER_STD  2000000
/* Must be between 7.82 MHz and 10 MHz for running i2c slave at 400KHz      */
#define _SCB_PERI_CLOCK_MASTER_FST  8500000
/* Must be between 14.32 MHz and 25.8 MHz for running i2c slave at 1MHz  */
#define _SCB_PERI_CLOCK_MASTER_FSTP 20000000

/* Must be between 15.84 MHz and 89.0 MHz for running i2c master at 1MHz */
#if defined(COMPONENT_CAT1A) || defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C)
#define _SCB_PERI_CLOCK_SLAVE_FSTP 50000000
#elif defined(COMPONENT_CAT2)
#define _SCB_PERI_CLOCK_SLAVE_FSTP 24000000
#elif defined(COMPONENT_CAT5)
#define _SCB_PERI_CLOCK_SLAVE_FSTP 48000000
#endif

	struct ifx_cat1_i2c_data *data = dev->data;
	CySCB_Type *base = data->base;
	uint32_t block_num = data->hw_resource.block_num;
	uint32_t data_rate = 0;
	uint32_t peri_freq = 0;
	cy_rslt_t status;

	/* Return the actual data rate on success, 0 otherwise */
	if (freq == 0) {
		return 0;
	}

	if (freq <= CY_SCB_I2C_STD_DATA_RATE) {
		peri_freq = is_slave ? _SCB_PERI_CLOCK_SLAVE_STD : _SCB_PERI_CLOCK_MASTER_STD;
	} else if (freq <= CY_SCB_I2C_FST_DATA_RATE) {
		peri_freq = is_slave ? _SCB_PERI_CLOCK_SLAVE_FST : _SCB_PERI_CLOCK_MASTER_FST;
	} else if (freq <= CY_SCB_I2C_FSTP_DATA_RATE) {
		peri_freq = is_slave ? _SCB_PERI_CLOCK_SLAVE_FSTP : _SCB_PERI_CLOCK_MASTER_FSTP;
	}

	if (peri_freq <= 0) {
		return 0;
	}

	if (_ifx_cat1_utils_peri_pclk_assign_divider(_ifx_cat1_scb_get_clock_index(block_num),
						     &data->clock) == CY_SYSCLK_SUCCESS) {
		status = ifx_cat1_clock_set_enabled(&data->clock, false, false);
		if (status == CY_RSLT_SUCCESS) {
			status = ifx_cat1_clock_set_frequency(&data->clock, peri_freq, NULL);
		}

		if (status == CY_RSLT_SUCCESS) {
			status = ifx_cat1_clock_set_enabled(&data->clock, true, false);
		}

		if (status == CY_RSLT_SUCCESS) {
			data_rate =
				(is_slave)
					? Cy_SCB_I2C_GetDataRate(
						  base, ifx_cat1_clock_get_frequency(&data->clock))
					: Cy_SCB_I2C_SetDataRate(
						  base, freq,
						  ifx_cat1_clock_get_frequency(&data->clock));
		}
	}

	return data_rate;
}
#endif

static int ifx_cat1_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	struct ifx_cat1_i2c_data *data = dev->data;
	const struct ifx_cat1_i2c_config *config = dev->config;
	cy_en_scb_i2c_status_t rslt;
	int ret;

	if (dev_config != 0) {
		switch (I2C_SPEED_GET(dev_config)) {
		case I2C_SPEED_STANDARD:
			data->frequencyhal_hz = CAT1_I2C_SPEED_STANDARD_HZ;
			break;
		case I2C_SPEED_FAST:
			data->frequencyhal_hz = CAT1_I2C_SPEED_FAST_HZ;
			break;
		case I2C_SPEED_FAST_PLUS:
			data->frequencyhal_hz = CAT1_I2C_SPEED_FAST_PLUS_HZ;
			break;
		default:
			LOG_ERR("Unsupported speed");
			return -ERANGE;
		}

		/* This is deprecated and could be ignored in the future */
		if (dev_config & I2C_ADDR_10_BITS) {
			LOG_ERR("10-bit addressing mode is not supported");
			return -EIO;
		}

		if (dev_config & I2C_MODE_CONTROLLER) {
			_i2c_default_config.i2cMode = CY_SCB_I2C_MASTER;
		} else {
			_i2c_default_config.i2cMode = CY_SCB_I2C_SLAVE;
		}
	}

	/* Acquire semaphore (block I2C operation for another thread) */
	ret = k_sem_take(&data->operation_sem, K_FOREVER);
	if (ret) {
		return -EIO;
	}

	_i2c_default_config.slaveAddress = data->slave_address;

	/* Configure the I2C resource to be master */
	rslt = Cy_SCB_I2C_Init(config->reg_addr, &_i2c_default_config, &data->context);
	if (rslt != CY_SCB_I2C_SUCCESS) {
		LOG_ERR("I2C configure failed with err 0x%x", rslt);
		k_sem_give(&data->operation_sem);
		return -EIO;
	}

#ifdef USE_I2C_SET_PERI_DIVIDER
	_i2c_set_peri_divider(dev, CAT1_I2C_SPEED_STANDARD_HZ,
			      (_i2c_default_config.i2cMode == CY_SCB_I2C_SLAVE));

#endif

	Cy_SCB_I2C_Enable(config->reg_addr);
	irq_enable(config->irq_num);

#ifdef CONFIG_I2C_INFINEON_CAT1_ASYNC
	/* Register an I2C event callback handler */
	ifx_cat1_i2c_register_callback(dev, ifx_master_event_handler, (void *)dev);
#endif

#ifdef CONFIG_PM
	data->i2c_deep_sleep_param.context = &data->context;
	Cy_SysPm_RegisterCallback(&data->i2c_deep_sleep);
#endif
	/* Release semaphore */
	k_sem_give(&data->operation_sem);
	return 0;
}

static int ifx_cat1_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct ifx_cat1_i2c_data *data = dev->data;
	uint32_t config;

	switch (data->frequencyhal_hz) {
	case CAT1_I2C_SPEED_STANDARD_HZ:
		config = I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case CAT1_I2C_SPEED_FAST_HZ:
		config = I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	case CAT1_I2C_SPEED_FAST_PLUS_HZ:
		config = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
		break;
	default:
		LOG_ERR("Unsupported speed");
		return -ERANGE;
	}

	/* Return current configuration */
	*dev_config = config | I2C_MODE_CONTROLLER;

	return 0;
}

static int ifx_cat1_i2c_msg_validate(struct i2c_msg *msg, uint8_t num_msgs)
{
	for (uint32_t i = 0u; i < num_msgs; i++) {
		if ((I2C_MSG_ADDR_10_BITS & msg[i].flags) || (msg[i].buf == NULL)) {
			return -EINVAL;
		}
	}
	return 0;
}

#ifdef CONFIG_I2C_INFINEON_CAT1_ASYNC

static cy_rslt_t _i2c_master_transfer_async(const struct device *dev, uint16_t address,
					    const void *tx, size_t tx_size, void *rx,
					    size_t rx_size)
{
	struct ifx_cat1_i2c_data *data = dev->data;

	data->rx_config.slaveAddress = (uint8_t)address;
	data->tx_config.slaveAddress = (uint8_t)address;

	data->rx_config.buffer = rx;
	data->rx_config.bufferSize = rx_size;

	data->tx_config.buffer = (void *)tx;
	data->tx_config.bufferSize = tx_size;

	if (data->pending) {
		return CY_SCB_I2C_MASTER_NOT_READY;
	}

	if (tx_size) {
		data->pending = (rx_size) ? CAT1_I2C_PENDING_TX_RX : CAT1_I2C_PENDING_TX;
		Cy_SCB_I2C_MasterWrite(data->base, &data->tx_config, &data->context);
		/* Receive covered by interrupt handler - i2c_isr_handler() */
	} else if (rx_size) {
		data->pending = CAT1_I2C_PENDING_RX;
		Cy_SCB_I2C_MasterRead(data->base, &data->rx_config, &data->context);
	} else {
		return CY_SCB_I2C_BAD_PARAM;
	}

	return CY_RSLT_SUCCESS;
}

#else

static cy_rslt_t _i2c_master_read_write(const struct device *dev, CySCB_Type *base,
					uint16_t dev_addr, const uint8_t *data, uint16_t size,
					uint32_t timeout, bool send_stop,
					cy_en_scb_i2c_direction_t direction)
{
	struct ifx_cat1_i2c_data *ifx_data = dev->data;

	cy_en_scb_i2c_command_t ack = CY_SCB_I2C_ACK;

	cy_en_scb_i2c_status_t status =
		(ifx_data->context.state == CY_SCB_I2C_IDLE)
			? Cy_SCB_I2C_MasterSendStart(base, dev_addr, direction, timeout,
						     &ifx_data->context)
			: Cy_SCB_I2C_MasterSendReStart(base, dev_addr, direction, timeout,
						       &ifx_data->context);

	if (status == CY_SCB_I2C_SUCCESS) {
		while (size > 0) {
			if (direction == CY_SCB_I2C_WRITE_XFER) {
				status = Cy_SCB_I2C_MasterWriteByte(base, *data, timeout,
								    &ifx_data->context);
			} else {
				if (size == 1) {
					ack = CY_SCB_I2C_NAK;
				}
				status = Cy_SCB_I2C_MasterReadByte(base, ack, (uint8_t *)data,
								   timeout, &ifx_data->context);
			}

			if (status != CY_SCB_I2C_SUCCESS) {
				break;
			}
			--size;
			++data;
		}
	}

	if (send_stop) {
		/* SCB in I2C mode is very time sensitive. In practice we have to request STOP after
		 */
		/* each block, otherwise it may break the transmission */
		Cy_SCB_I2C_MasterSendStop(base, timeout, &ifx_data->context);
	}

	return status;
}

#endif

static int ifx_cat1_i2c_transfer(const struct device *dev, struct i2c_msg *msg, uint8_t num_msgs,
				 uint16_t addr)
{
	struct ifx_cat1_i2c_data *data = dev->data;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	int ret;

	if (!num_msgs) {
		return 0;
	}

	/* Acquire semaphore (block I2C transfer for another thread) */
	ret = k_sem_take(&data->operation_sem, K_FOREVER);
	if (ret) {
		return -EIO;
	}

	/* This function checks if msg.buf is not NULL and if
	 * target address is not 10 bit.
	 */
	if (ifx_cat1_i2c_msg_validate(msg, num_msgs) != 0) {
		k_sem_give(&data->operation_sem);
		return -EINVAL;
	}

#ifdef CONFIG_I2C_INFINEON_CAT1_ASYNC

	struct i2c_msg *tx_msg;
	struct i2c_msg *rx_msg;

	data->error_status = 0;
	data->async_pending = CAT1_I2C_PENDING_NONE;

	/* Enable I2C Interrupt */
	data->irq_cause |= I2C_CAT1_EVENTS_MASK;

	for (uint32_t i = 0u; i < num_msgs; i++) {
		tx_msg = NULL;
		rx_msg = NULL;

		if ((msg[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			tx_msg = &msg[i];
			data->async_pending = CAT1_I2C_PENDING_TX;
		}

		if ((msg[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			rx_msg = &msg[i];
			data->async_pending = CAT1_I2C_PENDING_TX;
		}

		if ((tx_msg != NULL) && ((i + 1U) < num_msgs) &&
		    ((msg[i + 1U].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ)) {
			rx_msg = &msg[i + 1U];
			i++;
			data->async_pending = CAT1_I2C_PENDING_TX_RX;
		}

		/* Initiate master write and read transfer
		 * using tx_buff and rx_buff respectively
		 */
		rslt = _i2c_master_transfer_async(dev, addr, (tx_msg == NULL) ? NULL : tx_msg->buf,
						  (tx_msg == NULL) ? 0u : tx_msg->len,
						  (rx_msg == NULL) ? NULL : rx_msg->buf,
						  (rx_msg == NULL) ? 0u : rx_msg->len);

		if (rslt != CY_RSLT_SUCCESS) {
			k_sem_give(&data->operation_sem);
			return -EIO;
		}

		/* Acquire semaphore (block I2C async transfer for another thread) */
		ret = k_sem_take(&data->transfer_sem, K_FOREVER);
		if (ret) {
			k_sem_give(&data->operation_sem);
			return -EIO;
		}

		/* If error_status != 1 we have error during transfer async.
		 * error_status is handling in master_event_handler function.
		 */
		if (data->error_status != 0) {
			/* Release semaphore */
			k_sem_give(&data->operation_sem);
			return -EIO;
		}
	}

	/* Disable I2C Interrupt */
	data->irq_cause &= ~I2C_CAT1_EVENTS_MASK;
#else
	const struct ifx_cat1_i2c_config *config = dev->config;

	for (uint32_t i = 0u; i < num_msgs; i++) {
		bool stop_flag = ((msg[i].flags & I2C_MSG_STOP) != 0u) ? true : false;

		if ((msg[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			rslt = _i2c_master_read_write(dev, config->reg_addr, addr, msg[i].buf,
						      msg[i].len, 0, stop_flag,
						      CY_SCB_I2C_WRITE_XFER);
		}
		if ((msg[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			rslt = _i2c_master_read_write(dev, config->reg_addr, addr, msg[i].buf,
						      msg[i].len, 0, stop_flag,
						      CY_SCB_I2C_READ_XFER);
		}

		if (rslt != CY_RSLT_SUCCESS) {
			/* Release semaphore */
			k_sem_give(&data->operation_sem);
			return -EIO;
		}
	}
#endif

	/* Release semaphore (After I2C transfer is complete) */
	k_sem_give(&data->operation_sem);
	return 0;
}

static int ifx_cat1_i2c_init(const struct device *dev)
{
	struct ifx_cat1_i2c_data *data = dev->data;
	const struct ifx_cat1_i2c_config *config = dev->config;
	/* cy_rslt_t result; */
	int ret;

	/* Dedicate SCB HW resource */
	data->hw_resource.type = IFX_RSC_SCB;
	data->hw_resource.block_num = _get_hw_block_num(config->reg_addr);

	/* Configure semaphores */
	ret = k_sem_init(&data->transfer_sem, 0, 1);
	if (ret) {
		return ret;
	}

	ret = k_sem_init(&data->operation_sem, 1, 1);
	if (ret) {
		return ret;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Assigns a programmable divider to a selected IP block */
	/* en_clk_dst_t clk_idx = _ifx_cat1_scb_get_clock_index(data->hw_resource.block_num);
	 * result = _ifx_cat1_utils_peri_pclk_assign_divider(clk_idx, &data->clock);
	 * if (result != CY_RSLT_SUCCESS) {
	 *	return -ENOTSUP;
	 * }
	 */

	/* Initial value for async operations */
	data->pending = CAT1_I2C_PENDING_NONE;

	config->irq_config_func(dev);

	return 0;
}

void _i2c_free(const struct device *dev)
{
	struct ifx_cat1_i2c_data *data = dev->data;

	if (data->base != NULL) {
		Cy_SCB_I2C_Disable(data->base, &data->context);
		Cy_SCB_I2C_DeInit(data->base);
		data->base = NULL;
	}

	if (IFX_RSC_INVALID != data->hw_resource.type) {
		data->hw_resource.type = IFX_RSC_INVALID;
	}
}

static int ifx_cat1_i2c_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct ifx_cat1_i2c_data *data = (struct ifx_cat1_i2c_data *)dev->data;

	if (!cfg) {
		return -EINVAL;
	}

	if (cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	data->p_target_config = cfg;
	data->slave_address = (uint8_t)cfg->address;

	if (ifx_cat1_i2c_configure(dev, I2C_SPEED_SET(I2C_SPEED_FAST)) != 0) {
		/* Free I2C resource */
		_i2c_free(dev);

		/* Release semaphore */
		k_sem_give(&data->operation_sem);
		return -EIO;
	}

	data->irq_cause |= I2C_CAT1_SLAVE_EVENTS_MASK;

	return 0;
}

static int ifx_cat1_i2c_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct ifx_cat1_i2c_data *data = (struct ifx_cat1_i2c_data *)dev->data;

	/* Acquire semaphore (block I2C operation for another thread) */
	k_sem_take(&data->operation_sem, K_FOREVER);

	_i2c_free(dev);
	data->p_target_config = NULL;

	data->irq_cause &= ~I2C_CAT1_SLAVE_EVENTS_MASK;

	/* Release semaphore */
	k_sem_give(&data->operation_sem);
	return 0;
}

static void i2c_isr_handler(const struct device *dev)
{
	struct ifx_cat1_i2c_data *data = (struct ifx_cat1_i2c_data *)dev->data;

	Cy_SCB_I2C_Interrupt(data->base, &data->context);

	if (data->pending) {
		/* This code is part of _i2c_master_transfer_async() API functionality */
		/* _i2c_master_transfer_async() API uses this interrupt handler for RX transfer
		 */
		if (0 == (Cy_SCB_I2C_MasterGetStatus(data->base, &data->context) &
			  CY_SCB_I2C_MASTER_BUSY)) {
			/* Check if TX is completed and run RX in case when TX and RX are enabled */
			if (data->pending == CAT1_I2C_PENDING_TX_RX) {
				/* Start RX transfer */
				data->pending = CAT1_I2C_PENDING_RX;
				Cy_SCB_I2C_MasterRead(data->base, &data->rx_config, &data->context);
			} else {
				/* Finish async TX or RX separate transfer */
				data->pending = CAT1_I2C_PENDING_NONE;
			}
		}
	}
}

void ifx_cat1_i2c_cb_wrapper(const struct device *dev, uint32_t event)
{
	struct ifx_cat1_i2c_data *const data = dev->data;
	uint32_t anded_events = (data->irq_cause & (uint32_t)event);

	if (anded_events) {
		ifx_cat1_i2c_event_callback_t callback =
			(ifx_cat1_i2c_event_callback_t)data->callback_data.callback;
		callback(data->callback_data.callback_arg, anded_events);
	}
}

/* I2C API structure */
static const struct i2c_driver_api i2c_cat1_driver_api = {
	.configure = ifx_cat1_i2c_configure,
	.transfer = ifx_cat1_i2c_transfer,
	.get_config = ifx_cat1_i2c_get_config,
	.target_register = ifx_cat1_i2c_target_register,
	.target_unregister = ifx_cat1_i2c_target_unregister};

#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C)
#define PERI_INFO(n) .clock_peri_group = DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),
#else
#define PERI_INFO(n)
#endif

#define I2C_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
	},                                                                                         \
	PERI_INFO(n)

#define I2C_CAT1_INIT_FUNC(n)                                                                      \
	static void ifx_cat1_i2c_irq_config_func_##n(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_isr_handler,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}

/* Macros for I2C instance declaration */
#define INFINEON_CAT1_I2C_INIT(n)                                                                  \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	void i2c_handle_events_func_##n(uint32_t event)                                            \
	{                                                                                          \
		ifx_cat1_i2c_cb_wrapper(DEVICE_DT_INST_GET(n), event);                             \
	}                                                                                          \
                                                                                                   \
	I2C_CAT1_INIT_FUNC(n)                                                                      \
                                                                                                   \
	static const struct ifx_cat1_i2c_config i2c_cat1_cfg_##n = {                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.master_frequency = DT_INST_PROP_OR(n, clock_frequency, 100000),                   \
		.reg_addr = (CySCB_Type *)DT_INST_REG_ADDR(n),                                     \
		.irq_priority = DT_INST_IRQ(n, priority),                                          \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.irq_config_func = ifx_cat1_i2c_irq_config_func_##n,                               \
		.i2c_handle_events_func = i2c_handle_events_func_##n,                              \
	};                                                                                         \
                                                                                                   \
	static struct ifx_cat1_i2c_data ifx_cat1_i2c_data##n = {                                   \
		.base = (CySCB_Type *)DT_INST_REG_ADDR(n),                                         \
		I2C_PERI_CLOCK_INIT(n)                                                             \
		.i2c_deep_sleep_param = {(CySCB_Type *)DT_INST_REG_ADDR(n), NULL},                 \
		.i2c_deep_sleep = {&Cy_SCB_I2C_DeepSleepCallback, CY_SYSPM_DEEPSLEEP,              \
				   CY_SYSPM_SKIP_BEFORE_TRANSITION,                                \
				   &ifx_cat1_i2c_data##n.i2c_deep_sleep_param, NULL, NULL, 1}};    \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, ifx_cat1_i2c_init, NULL, &ifx_cat1_i2c_data##n,               \
				  &i2c_cat1_cfg_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,        \
				  &i2c_cat1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_I2C_INIT)
