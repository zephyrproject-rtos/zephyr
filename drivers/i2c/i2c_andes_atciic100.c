/*
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file I2C driver for AndesTech atciic100 IP
 */

#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/irq.h>
#include "i2c_andes_atciic100.h"

#define DT_DRV_COMPAT andestech_atciic100

typedef void (*atciic100_dt_init_func_t)(void);

struct i2c_atciic100_config {
	uint32_t			base;
	uint32_t			irq_num;
	atciic100_dt_init_func_t	dt_init_fn;
};

static int i2c_atciic100_controller_send(const struct device *dev,
	uint16_t addr, const uint8_t *data, uint32_t num, uint8_t flags);
static int i2c_atciic100_controller_receive(const struct device *dev,
	uint16_t addr, uint8_t *data, uint32_t num, uint8_t flags);
static void i2c_controller_fifo_write(const struct device *dev,
	uint8_t is_init);
static void i2c_controller_fifo_read(const struct device *dev);
static int i2c_atciic100_init(const struct device *dev);

#if defined(CONFIG_I2C_TARGET)
static void i2c_atciic100_target_send(const struct device *dev,
		const uint8_t *data);
static void i2c_atciic100_target_receive(const struct device *dev,
		uint8_t *data);
#endif

static void i2c_atciic100_default_control(const struct device *dev)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint32_t reg = 0;

	k_sem_init(&dev_data->bus_lock, 1, 1);
	k_sem_init(&dev_data->device_sync_sem, 0, 1);

	/* Reset I2C bus */
	reg = sys_read32(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_RESET_I2C);
	sys_write32(reg, I2C_CMD(dev));

	/* I2C query FIFO depth */
	reg = sys_read32(I2C_CFG(dev));
	switch (reg & 0x3) {
	case 0x0:
		dev_data->fifo_depth = 2;
		break;
	case 0x1:
		dev_data->fifo_depth = 4;
		break;
	case 0x2:
		dev_data->fifo_depth = 8;
		break;
	case 0x3:
		dev_data->fifo_depth = 16;
		break;
	}

	/*
	 * I2C setting: target mode(default), standard speed
	 * 7-bit, CPU mode
	 */
	sys_write32(0x0, I2C_SET(dev));
	reg = sys_read32(I2C_SET(dev));
	reg |= ((SETUP_T_SUDAT_STD << 24)	|
		(SETUP_T_SP_STD  << 21)		|
		(SETUP_T_HDDAT_STD << 16)	|
		(SETUP_T_SCL_RATIO_STD << 13)	|
		(SETUP_T_SCLHI_STD << 4)	|
		SETUP_I2C_EN);

	sys_write32(reg, I2C_SET(dev));

	dev_data->driver_state = I2C_DRV_INIT;
	dev_data->status.mode = 0;
	dev_data->status.arbitration_lost = 0;
	dev_data->status.target_ack = 0;
}

static int i2c_atciic100_configure(const struct device *dev,
		uint32_t dev_config)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint32_t reg = 0;
	int ret = 0;

	reg = sys_read32(I2C_SET(dev));

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		reg |= SETUP_SPEED_STD;
		break;

	case I2C_SPEED_FAST:
		reg |= SETUP_SPEED_FAST;
		break;

	case I2C_SPEED_FAST_PLUS:
		reg |= SETUP_SPEED_FAST_PLUS;

	case I2C_SPEED_HIGH:
		ret = -EIO;
		goto unlock;
	case 0x00:
		break;
	default:
		ret = -EIO;
		goto unlock;
	}

	if (dev_config & I2C_MODE_CONTROLLER) {
		reg |= SETUP_CONTROLLER;
		dev_data->status.mode = 1;
	} else {
		reg &= ~SETUP_CONTROLLER;
		dev_data->status.mode = 0;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		reg |= SETUP_ADDRESSING;
	} else {
		reg &= ~SETUP_ADDRESSING;
	}

	sys_write32(reg, I2C_SET(dev));

	dev_data->driver_state |= I2C_DRV_CFG_PARAM;

unlock:
	k_sem_give(&dev_data->bus_lock);

	return ret;
}

static int i2c_atciic100_transfer(const struct device *dev,
	struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	int ret = 0;
	int count = 0;
	uint8_t burst_write_len = msgs[0].len + msgs[1].len;
	uint8_t burst_write_buf[I2C_MAX_COUNT + BURST_CMD_COUNT];

	k_sem_take(&dev_data->bus_lock, K_FOREVER);

	if ((msgs[0].flags == I2C_MSG_WRITE)
		&& (msgs[1].flags == (I2C_MSG_WRITE | I2C_MSG_STOP))) {

		burst_write_len = msgs[0].len + msgs[1].len;
		if (burst_write_len > MAX_XFER_SZ) {
			return -EIO;
		}
		for (count = 0; count < burst_write_len; count++) {
			if (count < msgs[0].len) {
				burst_write_buf[count] = msgs[0].buf[count];
			} else {
				burst_write_buf[count] =
					msgs[1].buf[count - msgs[0].len];
			}
		}
		ret = i2c_atciic100_controller_send(dev, addr, burst_write_buf,
							burst_write_len, true);
		goto exit;
	}

	for (uint8_t i = 0; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_atciic100_controller_send(dev,
				addr, msgs[i].buf, msgs[i].len, msgs[i].flags);
		} else {
			ret = i2c_atciic100_controller_receive(dev,
				addr, msgs[i].buf, msgs[i].len, msgs[i].flags);
		}

		if (ret < 0) {
			goto exit;
		}
	}

exit:
	/* Wait for transfer complete */
	k_sem_give(&dev_data->bus_lock);
	return ret;
}


static int i2c_atciic100_controller_send(const struct device *dev,
	uint16_t addr, const uint8_t *data, uint32_t num, uint8_t flags)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint32_t reg = 0;

	/*
	 * Max to 10-bit address.
	 * Parameters data = null or num = 0 means no payload for
	 * acknowledge polling. If no I2C payload, set Phase_data=0x0.
	 */
	if (addr > 0x3FF) {
		return -EIO;
	}

	/* Disable all I2C interrupts */
	reg = sys_read32(I2C_INTE(dev));
	reg &= (~IEN_ALL);
	sys_write32(reg, I2C_INTE(dev));

	dev_data->status.mode = 1;
	reg = sys_read32(I2C_SET(dev));
	reg |= SETUP_CONTROLLER;
	sys_write32(reg, I2C_SET(dev));

	/* Direction => tx:0, rx:1 */
	dev_data->status.arbitration_lost = 0;
	dev_data->status.target_ack = 0;
	dev_data->driver_state = I2C_DRV_CONTROLLER_TX;

	/* Step1, Clear FIFO */
	reg = sys_read32(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_CLEAR_FIFO);
	sys_write32(reg, I2C_CMD(dev));

	/*
	 * Step2
	 * Enable START, ADDRESS, DATA and STOP phase.
	 * If no payload, clear DATA phase.
	 * STOP condition triggered when transmission finish in controller mode.
	 * The bus is busy until STOP condition triggered.
	 * For 10-bit target address, we must set STOP bit.
	 * I2C direction : controller tx, set xfer DATA count.
	 */
	reg = sys_read32(I2C_CTRL(dev));
	reg &= (~(CTRL_PHASE_START | CTRL_PHASE_ADDR | CTRL_PHASE_STOP |
		CTRL_DIR | CTRL_DATA_COUNT));

	if (flags & I2C_MSG_STOP) {
		reg |= CTRL_PHASE_STOP;
	}
	if ((flags & I2C_MSG_RESTART) == 0) {
		reg |= (CTRL_PHASE_START | CTRL_PHASE_ADDR);
	}
	if (num) {
		reg |= (CTRL_PHASE_DATA | (num & CTRL_DATA_COUNT));
	}

	sys_write32(reg, I2C_CTRL(dev));

	/* Step3 init I2C info */
	dev_data->target_addr = addr;
	dev_data->xfered_data_wt_ptr = 0;
	dev_data->xfer_wt_num = num;
	dev_data->middleware_tx_buf = (uint8_t *)data;

	/* In I2C target address, general call address = 0x0(7-bit or 10-bit) */
	reg = sys_read32(I2C_ADDR(dev));
	reg &= (~TARGET_ADDR_MSK);
	reg |= (dev_data->target_addr & (TARGET_ADDR_MSK));
	sys_write32(reg, I2C_ADDR(dev));

	/*
	 * Step4 Enable Interrupts: Complete, Arbitration Lose
	 * Enable/Disable the FIFO Empty Interrupt
	 * Fill the FIFO before enabling FIFO Empty Interrupt
	 */
	reg = sys_read32(I2C_INTE(dev));

	i2c_controller_fifo_write(dev, 1);

	reg |= (IEN_CMPL | IEN_ARB_LOSE | IEN_ADDR_HIT);

	if (num > 0) {
		reg |= IEN_FIFO_EMPTY;
	} else {
		reg &= (~IEN_FIFO_EMPTY);
	}

	sys_write32(reg, I2C_INTE(dev));

	/*
	 * Step5,
	 * I2C Write 0x1 to the Command register to issue the transaction
	 */
	reg = sys_read32(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_ISSUE_TRANSACTION);
	sys_write32(reg, I2C_CMD(dev));

	k_sem_take(&dev_data->device_sync_sem, K_FOREVER);

	if (dev_data->status.target_ack != 1) {
		return -EIO;
	}
	dev_data->status.target_ack = 0;
	return 0;
}

static int i2c_atciic100_controller_receive(const struct device *dev,
	uint16_t addr, uint8_t *data, uint32_t num, uint8_t flags)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint32_t reg = 0;

	/*
	 * Max to 10-bit address.
	 * Parameters data = null or num = 0 means no payload for
	 * acknowledge polling. If no I2C payload, set Phase_data=0x0.
	 */
	if (addr > 0x3FF) {
		return -EIO;
	}

	/* Disable all I2C interrupts */
	reg = sys_read32(I2C_INTE(dev));
	reg &= (~IEN_ALL);
	sys_write32(reg, I2C_INTE(dev));

	dev_data->status.mode = 1;
	reg = sys_read32(I2C_SET(dev));
	reg |= SETUP_CONTROLLER;
	sys_write32(reg, I2C_SET(dev));

	/* Direction => tx:0, rx:1 */
	dev_data->status.arbitration_lost = 0;
	dev_data->status.target_ack = 0;
	dev_data->driver_state = I2C_DRV_CONTROLLER_RX;

	/* Step1, Clear FIFO */
	reg = sys_read32(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_CLEAR_FIFO);
	sys_write32(reg, I2C_CMD(dev));

	/*
	 * Step2
	 * Enable START, ADDRESS, DATA and STOP phase.
	 * If no payload, clear DATA phase.
	 * STOP condition triggered when transmission finish in Controller mode.
	 * The bus is busy until STOP condition triggered.
	 * For 10-bit target address, we must set STOP bit.
	 * I2C direction : controller rx, set xfer data count.
	 */
	reg = sys_read32(I2C_CTRL(dev));
	reg &= (~(CTRL_PHASE_START | CTRL_PHASE_ADDR | CTRL_PHASE_STOP |
		CTRL_DIR | CTRL_DATA_COUNT));
	reg |= (CTRL_PHASE_START | CTRL_PHASE_ADDR | CTRL_DIR);

	if (flags & I2C_MSG_STOP) {
		reg |= CTRL_PHASE_STOP;
	}
	if (num) {
		reg |= (CTRL_PHASE_DATA | (num & CTRL_DATA_COUNT));
	}

	sys_write32(reg, I2C_CTRL(dev));

	/* Step3 init I2C info */
	dev_data->target_addr = addr;
	dev_data->xfered_data_rd_ptr = 0;
	dev_data->xfer_rd_num = num;
	dev_data->middleware_rx_buf = (uint8_t *)data;

	/* In I2C target address, general call address = 0x0(7-bit or 10-bit) */
	reg = sys_read32(I2C_ADDR(dev));
	reg &= (~TARGET_ADDR_MSK);
	reg |= (dev_data->target_addr & (TARGET_ADDR_MSK));
	sys_write32(reg, I2C_ADDR(dev));

	/*
	 * Step4 Enable Interrupts: Complete, Arbitration Lose
	 * Enable/Disable the FIFO Full Interrupt
	 */
	reg = sys_read32(I2C_INTE(dev));
	reg |= (IEN_CMPL | IEN_FIFO_FULL | IEN_ARB_LOSE | IEN_ADDR_HIT);
	sys_write32(reg, I2C_INTE(dev));

	/*
	 * Step5,
	 * I2C write 0x1 to the Command register to issue the transaction
	 */
	reg = sys_read32(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_ISSUE_TRANSACTION);
	sys_write32(reg, I2C_CMD(dev));

	k_sem_take(&dev_data->device_sync_sem, K_FOREVER);
	if (dev_data->status.target_ack != 1) {
		return -EIO;
	}
	dev_data->status.target_ack = 0;
	return 0;
}

#if defined(CONFIG_I2C_TARGET)
static void i2c_atciic100_target_send(const struct device *dev,
	const uint8_t *data)
{
	uint32_t reg = 0;

	/* Clear FIFO */
	reg = sys_read32(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_CLEAR_FIFO);
	sys_write32(reg, I2C_CMD(dev));

	sys_write32(*data, I2C_DATA(dev));
}

static void i2c_atciic100_target_receive(const struct device *dev,
	uint8_t *data)
{
	*data = sys_read32(I2C_DATA(dev));
}
#endif

static void i2c_controller_fifo_write(const struct device *dev,
		uint8_t is_init)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint32_t i = 0, write_fifo_count = 0, reg = 0;
	uint8_t write_data;

	write_fifo_count = dev_data->xfer_wt_num - dev_data->xfered_data_wt_ptr;

	if (write_fifo_count >= dev_data->fifo_depth) {
		write_fifo_count = dev_data->fifo_depth;
	}

	if (is_init) {
		write_fifo_count = 2;
	}

	/* I2C write a patch of data(FIFO_Depth) to FIFO */
	for (i = 0; i < write_fifo_count; i++) {

		write_data =
			dev_data->middleware_tx_buf[dev_data->xfered_data_wt_ptr];
		sys_write32((write_data & DATA_MSK), I2C_DATA(dev));
		dev_data->xfered_data_wt_ptr++;

		/* Disable the FIFO Empty Interrupt if no more data to send */
		if (dev_data->xfered_data_wt_ptr == dev_data->xfer_wt_num) {
			reg = sys_read32(I2C_INTE(dev));
			reg &= (~IEN_FIFO_EMPTY);
			sys_write32(reg, I2C_INTE(dev));
		}
	}
}

/* Basic fifo read function */
static void i2c_controller_fifo_read(const struct device *dev)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint32_t i = 0, read_fifo_count = 0, reg = 0;
	uint8_t read_data;

	read_fifo_count = dev_data->xfer_rd_num - dev_data->xfered_data_rd_ptr;

	if (read_fifo_count >= dev_data->fifo_depth) {
		read_fifo_count = dev_data->fifo_depth;
	}

	/* I2C read a patch of data(FIFO_Depth) from FIFO */
	for (i = 0; i < read_fifo_count; i++) {

		read_data = sys_read32(I2C_DATA(dev)) & DATA_MSK;

		dev_data->middleware_rx_buf[dev_data->xfered_data_rd_ptr] =
			read_data;
		dev_data->xfered_data_rd_ptr++;

		/* Disable the FIFO Full Interrupt if no more data to receive */
		if (dev_data->xfered_data_rd_ptr == dev_data->xfer_rd_num) {
			reg = sys_read32(I2C_INTE(dev));
			reg &= (~IEN_FIFO_FULL);
			sys_write32(reg, I2C_INTE(dev));
		}
	}
}

static void i2c_fifo_empty_handler(const struct device *dev)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;

	if (dev_data->driver_state & I2C_DRV_CONTROLLER_TX) {
		i2c_controller_fifo_write(dev, 0);
	}
}

static void i2c_fifo_full_handler(const struct device *dev)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;

	if (dev_data->driver_state & I2C_DRV_CONTROLLER_RX) {
		i2c_controller_fifo_read(dev);
	}
}

static void i2c_cmpl_handler(const struct device *dev, uint32_t reg_stat)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint32_t reg_set = 0, reg_ctrl = 0, reg = 0;

	reg_set = sys_read32(I2C_SET(dev));

	/* Controller mode */
	if (dev_data->status.mode == 1) {
		/* Disable all I2C interrupts */
		reg = sys_read32(I2C_INTE(dev));
		reg &= (~IEN_ALL);
		sys_write32(reg, I2C_INTE(dev));
	}

	if (dev_data->driver_state &
		(I2C_DRV_CONTROLLER_TX | I2C_DRV_CONTROLLER_RX)) {

		/* Get the remain number of data */
		reg_ctrl = sys_read32(I2C_CTRL(dev)) & CTRL_DATA_COUNT;

		if (dev_data->driver_state & I2C_DRV_CONTROLLER_TX) {
			/* Clear & set driver state to controller tx complete */
			dev_data->driver_state = I2C_DRV_CONTROLLER_TX_CMPL;
		}

		if (dev_data->driver_state & I2C_DRV_CONTROLLER_RX) {
			i2c_controller_fifo_read(dev);
			/* Clear & set driver state to controller rx complete */
			dev_data->driver_state = I2C_DRV_CONTROLLER_RX_CMPL;
		}

		k_sem_give(&dev_data->device_sync_sem);
	}

#if defined(CONFIG_I2C_TARGET)
	if (dev_data->driver_state & (I2C_DRV_TARGET_TX | I2C_DRV_TARGET_RX)) {
		reg_set = sys_read32(I2C_SET(dev));
		reg_ctrl = sys_read32(I2C_CTRL(dev));

		if (dev_data->driver_state & I2C_DRV_TARGET_TX) {
			dev_data->driver_state = I2C_DRV_TARGET_TX_CMPL;
		}

		if (dev_data->driver_state & I2C_DRV_TARGET_RX) {
			dev_data->driver_state = I2C_DRV_TARGET_RX_CMPL;
		}

		/* If the Completion Interrupt asserts,
		 * clear the FIFO and go next transaction.
		 */
		uint32_t reg_cmd = 0;

		reg_cmd = sys_read32(I2C_CMD(dev));
		reg_cmd &= (~CMD_MSK);
		reg_cmd |= (CMD_CLEAR_FIFO);
		sys_write32(reg_cmd, I2C_CMD(dev));
	}

	/* Enable Completion & Address Hit Interrupt */
	/* Enable Byte Receive & Transfer for default target mode */
	reg = 0x0;
	reg |= (IEN_CMPL | IEN_ADDR_HIT | STATUS_BYTE_RECV | STATUS_BYTE_TRANS);
	sys_write32(reg, I2C_INTE(dev));

	reg = sys_read32(I2C_SET(dev));
	reg &= ~(SETUP_CONTROLLER);
	sys_write32(reg, I2C_SET(dev));

	reg &= (~TARGET_ADDR_MSK);
	reg |= (dev_data->target_config->address & (TARGET_ADDR_MSK));
	sys_write32(reg, I2C_ADDR(dev));

	dev_data->driver_state = I2C_DRV_INIT;
	dev_data->status.mode = 0;
	dev_data->status.arbitration_lost = 0;
#endif

}

#if defined(CONFIG_I2C_TARGET)
static void andes_i2c_target_event(const struct device *dev,
		uint32_t reg_stat, uint32_t reg_ctrl)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint32_t reg_set = 0;
	uint8_t  val;
	/*
	 * Here is the entry for target mode driver to detect
	 * target RX/TX action depend on controller TX/RX action.
	 * A new I2C data transaction(START-ADDRESS-DATA-STOP)
	 */
	if (reg_stat & STATUS_ADDR_HIT) {
		if (k_sem_take(&dev_data->bus_lock, K_NO_WAIT) != 0) {
			return;
		}

		if (((reg_ctrl & CTRL_DIR) >> 8) == I2C_TARGET_TX) {
			/* Notify middleware to do target rx action */
			dev_data->driver_state = I2C_DRV_TARGET_TX;
			dev_data->target_callbacks->read_requested
				(dev_data->target_config, &val);
			i2c_atciic100_target_send(dev, &val);

		} else if (((reg_ctrl & CTRL_DIR) >> 8) == I2C_TARGET_RX) {
			/* Notify middleware to do target tx action */
			dev_data->driver_state = I2C_DRV_TARGET_RX;
			dev_data->target_callbacks->write_requested
				(dev_data->target_config);
		}
		reg_set |= (CMD_ACK);
		sys_write32(reg_set, I2C_CMD(dev));
	}

	if (reg_stat & STATUS_BYTE_RECV) {
		i2c_atciic100_target_receive(dev, &val);
		dev_data->target_callbacks->write_received
			(dev_data->target_config, val);

		reg_set = 0;
		if ((reg_stat & STATUS_CMPL) == 0) {
			reg_set |= (CMD_ACK);
			sys_write32(reg_set, I2C_CMD(dev));
		} else {
			reg_set |= (CMD_NACK);
			sys_write32(reg_set, I2C_CMD(dev));
		}

	} else if (reg_stat & STATUS_BYTE_TRANS) {
		dev_data->target_callbacks->read_processed
			(dev_data->target_config, &val);
		i2c_atciic100_target_send(dev, &val);
	}

	if (reg_stat & STATUS_CMPL) {
		i2c_cmpl_handler(dev, reg_stat);
		k_sem_give(&dev_data->bus_lock);
	}
}

static int i2c_atciic100_target_register(const struct device *dev,
	struct i2c_target_config *cfg)
{
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;
	uint16_t reg_addr = 0;
	uint32_t reg;

	reg_addr &= (~TARGET_ADDR_MSK);
	reg_addr |= (cfg->address & (TARGET_ADDR_MSK));

	sys_write32(reg_addr, I2C_ADDR(dev));

	dev_data->target_callbacks = cfg->callbacks;
	dev_data->target_config = cfg;

	/* Enable Completion & Address Hit Interrupt */
	/* Enable Byte Receive & Transfer for default target mode */
	reg = 0x0;
	reg |= (IEN_CMPL | IEN_ADDR_HIT | STATUS_BYTE_RECV | STATUS_BYTE_TRANS);
	sys_write32(reg, I2C_INTE(dev));

	return 0;
}

static int i2c_atciic100_target_unregister(const struct device *dev,
	struct i2c_target_config *cfg)
{
	uint32_t reg;

	/* Disable all I2C interrupts */
	reg = sys_read32(I2C_INTE(dev));
	reg &= (~IEN_ALL);
	sys_write32(reg, I2C_INTE(dev));

	sys_write32(0x0, I2C_ADDR(dev));

	return 0;
}
#endif

static void i2c_atciic100_irq_handler(void *arg)
{
	const struct device *dev = (struct device *)arg;
	struct i2c_atciic100_dev_data_t *dev_data = dev->data;

	uint32_t reg_set, reg_stat = 0, reg_ctrl = 0;

	reg_stat = sys_read32(I2C_STAT(dev));
	reg_set = sys_read32(I2C_SET(dev));
	reg_ctrl = sys_read32(I2C_CTRL(dev));

	/* Clear interrupts status */
	sys_write32((reg_stat & STATUS_W1C_ALL), I2C_STAT(dev));

#if defined(CONFIG_I2C_TARGET)
	if (dev_data->status.mode == 0) {
		andes_i2c_target_event(dev, reg_stat, reg_ctrl);
		return;
	}
#endif
	if (reg_stat & STATUS_ADDR_HIT) {
		dev_data->status.target_ack = 1;
	}

	if (reg_stat & STATUS_FIFO_EMPTY) {
		i2c_fifo_empty_handler(dev);
	}

	if (reg_stat & STATUS_FIFO_FULL) {
		/* Store hw receive data count quickly */
		i2c_fifo_full_handler(dev);
	}

	if (reg_stat & STATUS_CMPL) {
		/* Store hw receive data count quickly */
		i2c_cmpl_handler(dev, reg_stat);
	}

	if ((reg_stat & STATUS_ARB_LOSE) && (reg_set & SETUP_CONTROLLER)) {
		dev_data->status.arbitration_lost = 1;
	}
}

static const struct i2c_driver_api i2c_atciic100_driver = {
	.configure		= (i2c_api_configure_t)i2c_atciic100_configure,
	.transfer		= (i2c_api_full_io_t)i2c_atciic100_transfer,
#if defined(CONFIG_I2C_TARGET)
	.target_register	=
		(i2c_api_target_register_t)i2c_atciic100_target_register,
	.target_unregister	=
		(i2c_api_target_unregister_t)i2c_atciic100_target_unregister
#endif
};

static int i2c_atciic100_init(const struct device *dev)
{
	const struct i2c_atciic100_config *dev_cfg = dev->config;

	/* Disable all interrupts. */
	sys_write32(0x00000000, I2C_INTE(dev));
	/* Clear interrupts status. */
	sys_write32(0xFFFFFFFF, I2C_STAT(dev));

	dev_cfg->dt_init_fn();

	i2c_atciic100_default_control(dev);

#if defined(CONFIG_I2C_TARGET)
	i2c_atciic100_configure(dev, I2C_SPEED_SET(I2C_SPEED_STANDARD));
#else
	i2c_atciic100_configure(dev, I2C_SPEED_SET(I2C_SPEED_STANDARD)
							| I2C_MODE_CONTROLLER);
#endif
	irq_enable(dev_cfg->irq_num);

	return 0;
}

#define I2C_INIT(n)							\
	static struct i2c_atciic100_dev_data_t				\
					i2c_atciic100_dev_data_##n;	\
	static void i2c_dt_init_##n(void);				\
	static const struct i2c_atciic100_config			\
		i2c_atciic100_config_##n = {				\
		.base	= DT_INST_REG_ADDR(n),				\
		.irq_num	= DT_INST_IRQN(n),			\
		.dt_init_fn	= i2c_dt_init_##n			\
	};								\
	I2C_DEVICE_DT_INST_DEFINE(n,					\
		i2c_atciic100_init,					\
		NULL,							\
		&i2c_atciic100_dev_data_##n,				\
		&i2c_atciic100_config_##n,				\
		POST_KERNEL,						\
		CONFIG_I2C_INIT_PRIORITY,				\
		&i2c_atciic100_driver);					\
									\
	static void i2c_dt_init_##n(void)				\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
		DT_INST_IRQ(n, priority),				\
		i2c_atciic100_irq_handler,				\
		DEVICE_DT_INST_GET(n),					\
		0);							\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_INIT)
