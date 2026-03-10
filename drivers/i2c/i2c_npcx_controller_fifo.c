/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* I2C controller functions for 'FIFO' mode */

#include <zephyr/drivers/i2c.h>

#include <soc.h>

#include <zephyr/logging/log.h>

#include "i2c_npcx_controller.h"
LOG_MODULE_REGISTER(i2c_npcx_fifo, CONFIG_I2C_LOG_LEVEL);

static inline void i2c_ctrl_fifo_stall_scl(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/* Enable writing to SCL_LVL/SDA_LVL bit in SMBnCTL3 */
	inst->SMBCTL4 |= BIT(NPCX_SMBCTL4_LVL_WE);
	/* Force SCL bus to low and keep SDA floating */
	inst->SMBCTL3 = (inst->SMBCTL3 & ~BIT(NPCX_SMBCTL3_SCL_LVL)) | BIT(NPCX_SMBCTL3_SDA_LVL);
	/* Disable writing to them */
	inst->SMBCTL4 &= ~BIT(NPCX_SMBCTL4_LVL_WE);
}

static inline void i2c_ctrl_fifo_free_scl(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/* Enable writing to SCL_LVL/SDA_LVL bit in SMBnCTL3 */
	inst->SMBCTL4 |= BIT(NPCX_SMBCTL4_LVL_WE);
	/*
	 * Release SCL bus. Then it might be still driven by module itself or
	 * slave device.
	 */
	inst->SMBCTL3 |= BIT(NPCX_SMBCTL3_SCL_LVL) | BIT(NPCX_SMBCTL3_SDA_LVL);
	/* Disable writing to them */
	inst->SMBCTL4 &= ~BIT(NPCX_SMBCTL4_LVL_WE);
}

static inline void i2c_ctrl_fifo_stall_sda(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/* Enable writing to SCL_LVL/SDA_LVL bit in SMBnCTL3 */
	inst->SMBCTL4 |= BIT(NPCX_SMBCTL4_LVL_WE);
	/* Force SDA bus to low and keep SCL floating */
	inst->SMBCTL3 = (inst->SMBCTL3 & ~BIT(NPCX_SMBCTL3_SDA_LVL)) | BIT(NPCX_SMBCTL3_SCL_LVL);
	/* Disable writing to them */
	inst->SMBCTL4 &= ~BIT(NPCX_SMBCTL4_LVL_WE);
}

static inline void i2c_ctrl_fifo_free_sda(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/* Enable writing to SCL_LVL/SDA_LVL bit in SMBnCTL3 */
	inst->SMBCTL4 |= BIT(NPCX_SMBCTL4_LVL_WE);
	/*
	 * Release SDA bus. Then it might be still driven by module itself or
	 * slave device.
	 */
	inst->SMBCTL3 |= BIT(NPCX_SMBCTL3_SDA_LVL) | BIT(NPCX_SMBCTL3_SCL_LVL);
	/* Disable writing to them */
	inst->SMBCTL4 &= ~BIT(NPCX_SMBCTL4_LVL_WE);
}

bool i2c_ctrl_toggle_scls(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/* Drive the clock high. */
	i2c_ctrl_fifo_free_scl(dev);
	k_busy_wait(I2C_RECOVER_BUS_DELAY_US);
	/*
	 * Toggle SCL to generate 9 clocks. If the I2C target releases the SDA, we can stop
	 * toggle the SCL and issue a STOP.
	 */
	for (int j = 0; j < 9; j++) {
		if (IS_BIT_SET(inst->SMBCTL3, NPCX_SMBCTL3_SDA_LVL)) {
			break;
		}
		i2c_ctrl_fifo_stall_scl(dev);
		k_busy_wait(I2C_RECOVER_BUS_DELAY_US);
		i2c_ctrl_fifo_free_scl(dev);
		k_busy_wait(I2C_RECOVER_BUS_DELAY_US);
	}
	/* Drive the SDA line to issue STOP. */
	i2c_ctrl_fifo_stall_sda(dev);
	k_busy_wait(I2C_RECOVER_BUS_DELAY_US);
	i2c_ctrl_fifo_free_sda(dev);
	k_busy_wait(I2C_RECOVER_BUS_DELAY_US);
	if (i2c_ctrl_is_scl_sda_both_high(dev)) {
		return true;
	}

	return false;
}

/* I2C controller inline functions for 'FIFO' mode */
/*
 * I2C local functions which touch the registers in 'Normal' bank. These
 * utilities will change bank back to FIFO mode when leaving themselves in case
 * the other utilities access the registers in 'FIFO' bank.
 */
void i2c_ctrl_fifo_hold_bus(const struct device *dev, int stall)
{
	i2c_ctrl_bank_sel(dev, NPCX_I2C_BANK_NORMAL);

	if (stall) {
		i2c_ctrl_fifo_stall_scl(dev);
	} else {
		i2c_ctrl_fifo_free_scl(dev);
	}

	i2c_ctrl_bank_sel(dev, NPCX_I2C_BANK_FIFO);
}

static inline int i2c_ctrl_fifo_tx_avail(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	return NPCX_I2C_FIFO_MAX_SIZE - (inst->SMBTXF_STS & 0x3f);
}

static inline int i2c_ctrl_fifo_rx_occupied(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	return inst->SMBRXF_STS & 0x3f;
}

void i2c_ctrl_stop(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);
	struct i2c_ctrl_data *const data = dev->data;
	uint32_t delay_cycle = data->stop_dealy_cycle_time;
	uint32_t delay_start = k_cycle_get_32();

	while (k_cycle_get_32() - delay_start < delay_cycle) {
		arch_nop();
	}

	inst->SMBCTL1 |= BIT(NPCX_SMBCTL1_STOP);
}

/* I2C controller `FIFO` interrupt functions */
void i2c_ctrl_handle_write_int_event(const struct device *dev)
{
	struct i2c_ctrl_data *const data = dev->data;

	/* START condition is issued */
	if (data->oper_state == NPCX_I2C_WAIT_START) {
		/* Write slave address with W bit */
		i2c_ctrl_data_write(dev, ((data->addr << 1) & ~BIT(0)));
		/* Start to proceed write process */
		data->oper_state = NPCX_I2C_WRITE_DATA;
		return;
	}

	/* Write message data bytes to FIFO */
	if (data->oper_state == NPCX_I2C_WRITE_DATA) {
		/* Calculate how many remaining bytes need to transmit */
		size_t tx_remain = i2c_ctrl_calculate_msg_remains(dev);
		size_t tx_avail = MIN(tx_remain, i2c_ctrl_fifo_tx_avail(dev));

		for (int i = 0U; i < tx_avail; i++) {
			i2c_ctrl_data_write(dev, *(data->ptr_msg++));
		}

		/* Is there any remaining bytes? */
		if (data->ptr_msg == data->msg->buf + data->msg->len) {
			data->oper_state = NPCX_I2C_WRITE_SUSPEND;
		}
		return;
	}

	/* Issue STOP after sending message? */
	if (data->oper_state == NPCX_I2C_WRITE_SUSPEND) {
		if (data->msg->flags & I2C_MSG_STOP) {
			/* Generate a STOP condition immediately */
			i2c_ctrl_stop(dev);
			/* Clear rx FIFO threshold and status bits */
			i2c_ctrl_fifo_clear_status(dev);
			/* Wait for STOP completed */
			data->oper_state = NPCX_I2C_WAIT_STOP;
		} else {
			uint8_t next_msg_idx = data->msg_curr_idx + 1;

			if (next_msg_idx < data->msg_max_num) {
				struct i2c_msg *msg;

				data->msg_curr_idx = next_msg_idx;
				msg = data->msg_head + next_msg_idx;
				data->msg = msg;
				data->ptr_msg = msg->buf;
				if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
					data->oper_state = NPCX_I2C_WRITE_DATA;
				} else {
					data->is_write = 0;
					data->oper_state = NPCX_I2C_WAIT_RESTART;
					i2c_ctrl_start(dev);
				}
				return;
			}
			/* Disable interrupt and handle next message */
			i2c_ctrl_irq_enable(dev, 0);
		}
	}

	i2c_ctrl_notify(dev, 0);
}

void i2c_ctrl_handle_read_int_event(const struct device *dev)
{
	struct i2c_ctrl_data *const data = dev->data;

	/* START or RESTART condition is issued */
	if (data->oper_state == NPCX_I2C_WAIT_START || data->oper_state == NPCX_I2C_WAIT_RESTART) {
		/* Setup threshold of rx FIFO before sending address byte */
		i2c_ctrl_fifo_rx_setup_threshold_nack(dev, data->msg->len,
						      (data->msg->flags & I2C_MSG_STOP) != 0);
		/* Write slave address with R bit */
		i2c_ctrl_data_write(dev, ((data->addr << 1) | BIT(0)));
		/* Start to proceed read process */
		data->oper_state = NPCX_I2C_READ_DATA;
		return;
	}

	/* Read message data bytes from FIFO */
	if (data->oper_state == NPCX_I2C_READ_DATA) {
		/* Calculate how many remaining bytes need to receive */
		size_t rx_remain = i2c_ctrl_calculate_msg_remains(dev);
		size_t rx_occupied = i2c_ctrl_fifo_rx_occupied(dev);

		/* Is it the last read transaction with STOP condition? */
		if (rx_occupied >= rx_remain && (data->msg->flags & I2C_MSG_STOP) != 0) {
			/*
			 * Generate a STOP condition before reading data bytes
			 * from FIFO. It prevents a glitch on SCL.
			 */
			i2c_ctrl_stop(dev);
		} else {
			/*
			 * Hold SCL line here in case the hardware releases bus
			 * immediately after the driver start to read data from
			 * FIFO. Then we might lose incoming data from device.
			 */
			i2c_ctrl_fifo_hold_bus(dev, 1);
		}

		/* Read data bytes from FIFO */
		for (int i = 0; i < rx_occupied; i++) {
			*(data->ptr_msg++) = i2c_ctrl_data_read(dev);
		}
		rx_remain = i2c_ctrl_calculate_msg_remains(dev);

		/* Setup threshold of RX FIFO if needed */
		if (rx_remain > 0) {
			i2c_ctrl_fifo_rx_setup_threshold_nack(
				dev, rx_remain, (data->msg->flags & I2C_MSG_STOP) != 0);
			/* Release bus */
			i2c_ctrl_fifo_hold_bus(dev, 0);
			return;
		}

		if ((data->msg->flags & I2C_MSG_STOP) == 0) {
			uint8_t next_msg_idx = data->msg_curr_idx + 1;

			if (next_msg_idx < data->msg_max_num) {
				struct i2c_msg *msg;

				msg = data->msg_head + next_msg_idx;
				if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {

					data->msg_curr_idx = next_msg_idx;
					data->msg = msg;
					data->ptr_msg = msg->buf;

					/* Setup threshold of RX FIFO first */
					i2c_ctrl_fifo_rx_setup_threshold_nack(
						dev, msg->len, (msg->flags & I2C_MSG_STOP) != 0);
					/* Release bus */
					i2c_ctrl_fifo_hold_bus(dev, 0);
					return;
				}
			}
		}
	}

	/* Is the STOP condition issued? */
	if (data->msg != NULL && (data->msg->flags & I2C_MSG_STOP) != 0) {
		/* Clear rx FIFO threshold and status bits */
		i2c_ctrl_fifo_clear_status(dev);

		/* Wait for STOP completed */
		data->oper_state = NPCX_I2C_WAIT_STOP;
	} else {
		/* Disable i2c interrupt first */
		i2c_ctrl_irq_enable(dev, 0);
		data->oper_state = NPCX_I2C_READ_SUSPEND;
	}

	i2c_ctrl_notify(dev, 0);
}
