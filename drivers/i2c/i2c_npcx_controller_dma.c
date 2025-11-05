/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* I2C controller functions for 'DMA' mode */

#include <zephyr/drivers/i2c.h>

#include <soc.h>

#include <zephyr/logging/log.h>

#include "i2c_npcx_controller.h"
LOG_MODULE_REGISTER(i2c_npcx_dma, CONFIG_I2C_LOG_LEVEL);

static inline uint16_t i2c_ctrl_dma_transferred_bytes(const struct device *dev)
{
	uint16_t lens;
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/* return number of bytes of DMA transmitted or received transactions */
	lens = (inst->DATA_CNT1 << 8) + inst->DATA_CNT2;

	return lens;
}

static inline void i2c_ctrl_dma_nack(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	inst->DMA_CTRL |= BIT(NPCX_DMA_CTL_LAST_PEC);
}

static size_t i2c_ctrl_calc_dma_lens(const struct device *dev)
{
	size_t remains = i2c_ctrl_calculate_msg_remains(dev);

	return MIN(remains, NPCX_I2C_DMA_MAX_SIZE);
}

static bool i2c_ctrl_dma_is_last_pkg(const struct device *dev, size_t remains)
{
	struct i2c_ctrl_data *const data = dev->data;

	return data->ptr_msg + remains == data->msg->buf + data->msg->len;
}

static inline void i2c_ctrl_dma_start(const struct device *dev, uint8_t *addr, uint16_t lens)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);
	uint32_t dma_addr = (uint32_t)addr;

	if (lens == 0) {
		return;
	}

	/* Configure the address of DMA transmitted or received transactions */
	inst->DMA_ADDR1 = (uint8_t)(dma_addr & 0xff);
	inst->DMA_ADDR2 = (uint8_t)((dma_addr >> 8) & 0xff);
	inst->DMA_ADDR3 = (uint8_t)((dma_addr >> 16) & 0xff);
	inst->DMA_ADDR4 = (uint8_t)((dma_addr >> 24) & 0xff);

	/* Configure the length of DMA transmitted or received transactions */
	inst->DATA_LEN1 = (uint8_t)(lens & 0xff);
	inst->DATA_LEN2 = (uint8_t)((lens >> 8) & 0xff);

	/* Clear DMA status bit and release bus */
	if (IS_BIT_SET(inst->DMA_CTRL, NPCX_DMA_CTL_IRQSTS)) {
		i2c_ctrl_dma_clear_status(dev);
	}
	/* Start the DMA transaction */
	inst->DMA_CTRL |= BIT(NPCX_DMA_CTL_ENABLE);
}

size_t i2c_ctrl_dma_proceed_write(const struct device *dev)
{
	/* Calculate how many remaining bytes need to transmit */
	size_t dma_lens = i2c_ctrl_calc_dma_lens(dev);
	struct i2c_ctrl_data *const data = dev->data;

	LOG_DBG("W: dma lens %d, last %d", dma_lens, i2c_ctrl_dma_transferred_bytes(dev));

	/* No DMA transactions */
	if (dma_lens == 0) {
		return 0;
	}

	/* Start DMA transmitted transaction again */
	i2c_ctrl_dma_start(dev, data->ptr_msg, dma_lens);

	return dma_lens;
}

size_t i2c_ctrl_dma_proceed_read(const struct device *dev)
{
	/* Calculate how many remaining bytes need to receive */
	size_t dma_lens = i2c_ctrl_calc_dma_lens(dev);
	struct i2c_ctrl_data *const data = dev->data;

	LOG_DBG("R: dma lens %d, last %d", dma_lens, i2c_ctrl_dma_transferred_bytes(dev));

	if (dma_lens == 0) {
		return 0;
	}

	/* Last byte for NACK in received transaction */
	if (i2c_ctrl_dma_is_last_pkg(dev, dma_lens) && (data->msg->flags & I2C_MSG_STOP) != 0) {
		/* Issue NACK in the end of DMA transation */
		i2c_ctrl_dma_nack(dev);
	}

	/* Start DMA if bus is idle */
	i2c_ctrl_dma_start(dev, data->ptr_msg, dma_lens);

	return dma_lens;
}

void i2c_ctrl_stop(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	inst->SMBCTL1 |= BIT(NPCX_SMBCTL1_STOP);
}

/* I2C controller recover function in `DMA` mode */
bool i2c_ctrl_toggle_scls(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/*
	 * Toggle SCL to generate 9 clocks. If the I2C target releases the SDA, we can stop
	 * toggle the SCL and issue a STOP.
	 */
	for (int j = 0; j < 9; j++) {
		if (IS_BIT_SET(inst->SMBCTL3, NPCX_SMBCTL3_SDA_LVL)) {
			break;
		}

		/* Toggle SCL line for one cycle. */
		inst->SMBCST |= BIT(NPCX_SMBCST_TGSCL);
		k_busy_wait(I2C_RECOVER_BUS_DELAY_US);
	}
	/* Generate a STOP condition */
	i2c_ctrl_stop(dev);
	k_busy_wait(I2C_RECOVER_BUS_DELAY_US);
	if (i2c_ctrl_is_scl_sda_both_high(dev)) {
		return true;
	}

	return false;
}

/* I2C controller `DMA` interrupt functions */
void i2c_ctrl_handle_write_int_event(const struct device *dev)
{
	struct i2c_ctrl_data *const data = dev->data;

	/* START condition is issued */
	if (data->oper_state == NPCX_I2C_WAIT_START) {
		/* Write slave address with W bit */
		i2c_ctrl_data_write(dev, ((data->addr << 1) & ~BIT(0)));

		/* Start first DMA transmitted transaction */
		i2c_ctrl_dma_proceed_write(dev);

		/* Start to proceed write process */
		data->oper_state = NPCX_I2C_WRITE_DATA;
	}
	/* Skip the other SDAST events */
}

void i2c_ctrl_handle_read_int_event(const struct device *dev)
{
	struct i2c_ctrl_data *const data = dev->data;

	/* START or RESTART condition is issued */
	if (data->oper_state == NPCX_I2C_WAIT_START || data->oper_state == NPCX_I2C_WAIT_RESTART) {
		/* Configure first DMA received transaction before sending address */
		i2c_ctrl_dma_proceed_read(dev);

		/* Write slave address with R bit */
		i2c_ctrl_data_write(dev, ((data->addr << 1) | BIT(0)));

		/* Start to proceed read process */
		data->oper_state = NPCX_I2C_READ_DATA;
	}
	/* Skip the other SDAST events */
}

void i2c_ctrl_handle_write_dma_int_event(const struct device *dev)
{
	struct i2c_ctrl_data *const data = dev->data;

	/* Write message data bytes to FIFO */
	if (data->oper_state == NPCX_I2C_WRITE_DATA) {
		/* Record how many bytes transmitted via DMA */
		data->ptr_msg += i2c_ctrl_dma_transferred_bytes(dev);

		/* If next DMA transmitted transaction proceeds, return immediately */
		if (i2c_ctrl_dma_proceed_write(dev) != 0) {
			return;
		}

		/* No more remaining bytes */
		if (data->msg->flags & I2C_MSG_STOP) {
			/* Generate a STOP condition immediately */
			i2c_ctrl_stop(dev);
			/* Clear DMA status bit and release bus */
			i2c_ctrl_dma_clear_status(dev);
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
					i2c_ctrl_dma_proceed_write(dev);
				} else {
					data->is_write = 0;
					data->oper_state = NPCX_I2C_WAIT_RESTART;
					i2c_ctrl_start(dev);
					/* Clear DMA status bit and release bus */
					i2c_ctrl_dma_clear_status(dev);
				}

				return;
			}

			/* Disable interrupt and hold bus until handling next message */
			i2c_ctrl_irq_enable(dev, 0);
			/* Wait for the other messages */
			data->oper_state = NPCX_I2C_WRITE_SUSPEND;
		}

		return i2c_ctrl_notify(dev, 0);
	}
}

void i2c_ctrl_handle_read_dma_int_event(const struct device *dev)
{
	struct i2c_ctrl_data *const data = dev->data;

	/* Read message data bytes from FIFO */
	if (data->oper_state == NPCX_I2C_READ_DATA) {
		/* Record how many bytes received via DMA */
		data->ptr_msg += i2c_ctrl_dma_transferred_bytes(dev);

		/* If next DMA received transaction proceeds, return immediately */
		if (i2c_ctrl_dma_proceed_read(dev) != 0) {
			return;
		}

		/* Is the STOP condition issued? */
		if ((data->msg->flags & I2C_MSG_STOP) != 0) {
			/* Generate a STOP condition immediately */
			i2c_ctrl_stop(dev);

			/* Clear DMA status bit and release bus */
			i2c_ctrl_dma_clear_status(dev);

			/* Wait for STOP completed */
			data->oper_state = NPCX_I2C_WAIT_STOP;
		} else {
			uint8_t next_msg_idx = data->msg_curr_idx + 1;

			if (next_msg_idx < data->msg_max_num) {
				struct i2c_msg *msg;

				msg = data->msg_head + next_msg_idx;
				if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
					data->msg_curr_idx = next_msg_idx;
					data->msg = msg;
					data->ptr_msg = msg->buf;
					i2c_ctrl_dma_proceed_read(dev);

					return;
				}
			}

			/* Disable i2c interrupt first */
			i2c_ctrl_irq_enable(dev, 0);
			data->oper_state = NPCX_I2C_READ_SUSPEND;
		}

		return i2c_ctrl_notify(dev, 0);
	}
}
