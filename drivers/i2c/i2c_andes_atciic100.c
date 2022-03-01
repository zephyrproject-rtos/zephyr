/*
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file I2C driver for AndesTech atciic100 IP
 */

#include <drivers/i2c.h>
#include "i2c_andes_atciic100.h"

#define DT_DRV_COMPAT andestech_atciic100

struct i2c_atciic100_device_config {
	uint32_t		i2c_base_addr;
	uint32_t		i2c_irq_num;
	struct _i2c_info	*i2c_info;
};

static int i2c_atciic100_master_send(const struct device *dev,
	uint16_t addr, const uint8_t *data, uint32_t num, bool phase_stop);
static int i2c_atciic100_master_receive(const struct device *dev,
	uint16_t addr, uint8_t *data, uint32_t num, bool phase_stop);
static int i2c_atciic100_slave_send(const struct device *dev,
	const uint8_t *data, uint32_t num);
static int i2c_atciic100_slave_receive(const struct device *dev,
	uint8_t *data, uint32_t num);
static void i2c_master_fifo_write(const struct device *dev, uint8_t is_init);
static void i2c_master_fifo_read(const struct device *dev);
static void i2c_slave_fifo_read(const struct device *dev, uint8_t is_fifo_full);
static void i2c_slave_fifo_write(const struct device *dev);
static void i2c_slave_addr_hit_handler(const struct device *dev);

static int i2c_atciic100_init(const struct device *dev);

const struct i2c_slave_callbacks *slave_callbacks;
struct i2c_slave_config *slave_config;

static void i2c_atciic100_default_control(const struct device *dev)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t reg = 0;

	k_sem_init(&i2c_info->i2c_transfer_sem, 0, 1);
	k_sem_init(&i2c_info->i2c_busy_sem, 0, 1);
	k_sem_give(&i2c_info->i2c_transfer_sem);
	k_sem_give(&i2c_info->i2c_busy_sem);

	/* Reset I2C bus */
	reg = INWORD(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_RESET_I2C);
	OUTWORD(I2C_CMD(dev), reg);

	/* I2C query FIFO depth */
	reg = INWORD(I2C_CFG(dev));
	switch (reg & 0x3) {
	case 0x0:
		i2c_info->fifo_depth = 2;
		break;
	case 0x1:
		i2c_info->fifo_depth = 4;
		break;
	case 0x2:
		i2c_info->fifo_depth = 8;
		break;
	case 0x3:
		i2c_info->fifo_depth = 16;
		break;
	}

	/*
	 * I2C setting: slave mode(default), standard speed
	 * 7-bit, CPU mode
	 */
	OUTWORD(I2C_SET(dev), 0x0);
	reg = INWORD(I2C_SET(dev));
	reg |= ((SETUP_T_SUDAT_STD << 24)	|
		(SETUP_T_SP_STD  << 21)		|
		(SETUP_T_HDDAT_STD << 16)	|
		(SETUP_T_SCL_RATIO_STD << 13)	|
		(SETUP_T_SCLHI_STD << 4)	|
		SETUP_MASTER			|
		SETUP_I2C_EN);

	OUTWORD(I2C_SET(dev), reg);

	/* Enable completion & address hit interrupt */
	reg = INWORD(I2C_INTE(dev));
	reg |= (IEN_CMPL | IEN_ADDR_HIT);
	OUTWORD(I2C_INTE(dev), reg);

	i2c_info->driver_state = I2C_DRV_INIT;
	i2c_info->status.mode = 1;
	i2c_info->status.arbitration_lost = 0;
	i2c_info->status.bus_error = 0;
}

static int i2c_atciic100_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t reg = 0;

	if (k_sem_take(&i2c_info->i2c_transfer_sem, K_MSEC(1000))) {
		return DRIVER_ERROR_TIMEOUT;
	}

	if (k_sem_take(&i2c_info->i2c_busy_sem, K_MSEC(1000))) {
		return DRIVER_ERROR_TIMEOUT;
	}

	reg = INWORD(I2C_SET(dev));

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
		return DRIVER_ERROR_UNSUPPORTED;
	case 0x00:
		break;
	default:
		return DRIVER_ERROR_PARAMETER;
	}

	if ((dev_config & I2C_MODE_MASTER) == I2C_MODE_MASTER) {
		reg |= SETUP_MASTER;
		i2c_info->status.mode = 1;
	} else {
		reg &= ~SETUP_MASTER;
		i2c_info->status.mode = 0;
	}

	if ((dev_config & I2C_ADDR_10_BITS) == I2C_ADDR_10_BITS) {
		reg |= SETUP_ADDRESSING;
	} else {
		reg &= ~SETUP_ADDRESSING;
	}

	OUTWORD(I2C_SET(dev), reg);

	i2c_info->driver_state |= I2C_DRV_CFG_PARAM;

	k_sem_give(&i2c_info->i2c_transfer_sem);
	k_sem_give(&i2c_info->i2c_busy_sem);

	return DRIVER_OK;
}

static int i2c_atciic100_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	uint32_t reg, config = 0;

	if (dev_cfg->i2c_info == NULL) {
		return -EIO;
	}

	reg = INWORD(I2C_SET(dev));

	if (reg & SETUP_MASTER) {
		config |= I2C_MODE_MASTER;
	}

	switch (reg & SETUP_SPEED_MSK) {
	case SETUP_SPEED_STD:
		config |= I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;

	case SETUP_SPEED_FAST:
		config |= I2C_SPEED_SET(I2C_SPEED_FAST);
		break;

	case SETUP_SPEED_FAST_PLUS:
		config |= I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);

	default:
		return -EIO;
	}

	*dev_config = config;

	return 0;
}

static int i2c_atciic100_transfer(const struct device *dev,
	struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint8_t burst_write_buf[512] = {0};
	uint8_t burst_write_len = 0;
	uint8_t count = 0;
	uint8_t status = 0;

	if (k_sem_take(&i2c_info->i2c_transfer_sem, K_MSEC(1000))) {
		return DRIVER_ERROR_TIMEOUT;
	}

	if (i2c_info->status.mode == 1) {
		if (num_msgs == 1) {
			if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				status = i2c_atciic100_master_send(dev,
					addr, msgs->buf, msgs->len, true);
			} else {
				status = i2c_atciic100_master_receive(dev,
					addr, msgs->buf, msgs->len, true);
			}
		} else {
			if ((msgs[1].flags & I2C_MSG_RESTART) == I2C_MSG_RESTART) {
				status = i2c_atciic100_master_send(dev,
					addr, msgs[0].buf, msgs[0].len, true);

				if (status) {
					k_sem_give(&i2c_info->i2c_transfer_sem);
					return status;
				}
				status = i2c_atciic100_master_receive(dev,
					addr, msgs[1].buf, msgs[1].len, true);
			} else {
				burst_write_len = msgs[0].len + msgs[1].len;

				if (burst_write_len > MAX_XFER_SZ) {
					k_sem_give(&i2c_info->i2c_transfer_sem);
					return DRIVER_ERROR_UNSUPPORTED;
				}

				for (count = 0; count < burst_write_len; count++) {
					if (count < msgs[0].len) {
						burst_write_buf[count] = msgs[0].buf[count];
					} else {
						burst_write_buf[count] = msgs[1].buf[count - msgs[0].len];
					}
				}
				status = i2c_atciic100_master_send(dev,
					addr, burst_write_buf, burst_write_len, true);
			}
		}
	} else {
		if (num_msgs == 1) {
			if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				status = i2c_atciic100_slave_send(dev, msgs->buf, msgs->len);
			} else {
				status = i2c_atciic100_slave_receive(dev, msgs->buf, msgs->len);
			}
		} else {
			k_sem_give(&i2c_info->i2c_transfer_sem);
			return DRIVER_ERROR_UNSUPPORTED;
		}
	}

	k_sem_give(&i2c_info->i2c_transfer_sem);

	if (status) {
		return status;
	}

	return DRIVER_OK;
}


static int i2c_atciic100_master_send(const struct device *dev,
	uint16_t addr, const uint8_t *data, uint32_t num, bool phase_stop)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t reg = 0;

	/*
	 * max to 10-bit address, data = null or num = 0 means no payload for
	 * acknowledge polling, if no I2C payload, set Phase_data=0x0
	 */
	if (addr > 0x3FF) {
		return DRIVER_ERROR_PARAMETER;
	}

	if (k_sem_take(&i2c_info->i2c_busy_sem, K_MSEC(1000))) {
		return DRIVER_ERROR_TIMEOUT;
	}

	/* Direction => tx:0, rx:1 */
	i2c_info->status.direction = 0;
	i2c_info->status.arbitration_lost = 0;
	i2c_info->status.bus_error = 0;

	i2c_info->driver_state = I2C_DRV_MASTER_TX;

	/* Step0, disable all I2C interrupts */
	reg = INWORD(I2C_INTE(dev));
	reg &= (~IEN_ALL);
	OUTWORD(I2C_INTE(dev), reg);

	/* Step1, Clear FIFO */
	reg = INWORD(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_CLEAR_FIFO);
	OUTWORD(I2C_CMD(dev), reg);

	/*
	 * Step2
	 * Enable start, addr, data and stop phase.
	 * If no payload, clear data phase.
	 * Stop condition triggered when transmission finish in Master mode.
	 * The bus is busy until STOP condition triggered.
	 * 10-bit slave address must set STOP bit.
	 * I2C direction : master tx, set xfer data count.
	 */
	reg = INWORD(I2C_CTRL(dev));
	reg &= (~(CTRL_PHASE_STOP | CTRL_DIR | CTRL_DATA_COUNT));
	reg |= (CTRL_PHASE_START | CTRL_PHASE_ADDR | (phase_stop << 9) |
		(num & CTRL_DATA_COUNT));

	if (!num) {
		reg &= (~CTRL_PHASE_DATA);
	} else {
		reg |= CTRL_PHASE_DATA;
	}

	OUTWORD(I2C_CTRL(dev), reg);

	/* Step3 init I2C info */
	i2c_info->slave_addr = addr;
	i2c_info->xfered_data_wt_ptr = 0;
	i2c_info->xfer_wt_num = num;
	i2c_info->xfer_cmpl_count = 0;
	i2c_info->middleware_tx_buf = (uint8_t *)data;

	/* I2C slave address, general call address = 0x0(7-bit or 10-bit) */
	reg = INWORD(I2C_ADDR(dev));
	reg &= (~SLAVE_ADDR_MSK);
	reg |= (i2c_info->slave_addr & (SLAVE_ADDR_MSK));
	OUTWORD(I2C_ADDR(dev), reg);

	/*
	 * Step4 Enable Interrupts: Complete, Arbitration Lose
	 * Enable/Disable the FIFO Empty Interrupt
	 * Fill the FIFO before enable FIFO Empty Interrupt
	 */
	reg = INWORD(I2C_INTE(dev));

	i2c_master_fifo_write(dev, 1);

	reg |= (IEN_CMPL | IEN_ARB_LOSE);

	if (num > 0) {
		reg |= IEN_FIFO_EMPTY;
	} else {
		reg &= (~IEN_FIFO_EMPTY);
	}

	OUTWORD(I2C_INTE(dev), reg);

	/* Step5, I2C Write 0x1 to the Command register to issue the transaction */
	reg = INWORD(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_ISSUE_TRANSACTION);
	OUTWORD(I2C_CMD(dev), reg);

	return DRIVER_OK;
}

static int i2c_atciic100_master_receive(const struct device *dev,
	uint16_t addr, uint8_t *data, uint32_t num, bool phase_stop)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t reg = 0;

	/*
	 * max to 10-bit address, data = null or num = 0 means no payload for
	 * acknowledge polling, if no I2C payload, set Phase_data=0x0
	 */
	if (addr > 0x3FF) {
		return DRIVER_ERROR_PARAMETER;
	}

	if (k_sem_take(&i2c_info->i2c_busy_sem, K_MSEC(1000))) {
		return DRIVER_ERROR_TIMEOUT;
	}

	/* Direction => tx:0, rx:1 */
	i2c_info->status.direction = 1;
	i2c_info->status.arbitration_lost = 0;
	i2c_info->status.bus_error = 0;

	i2c_info->driver_state = I2C_DRV_MASTER_RX;

	/* Step0, disable all I2C interrupts */
	reg = INWORD(I2C_INTE(dev));
	reg &= (~IEN_ALL);
	OUTWORD(I2C_INTE(dev), reg);

	/* Step1, Clear FIFO */
	reg = INWORD(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_CLEAR_FIFO);
	OUTWORD(I2C_CMD(dev), reg);

	/*
	 * Step2
	 * Enable start, addr, data and stop phase.
	 * If no payload, clear data phase.
	 * Stop condition triggered when transmission finish in Master mode.
	 * The bus is busy until STOP condition triggered.
	 * 10-bit slave address must set STOP bit.
	 * I2C direction : master rx, set xfer data count.
	 */
	reg = INWORD(I2C_CTRL(dev));
	reg &= (~(CTRL_PHASE_STOP | CTRL_DATA_COUNT));
	reg |= (CTRL_PHASE_START | CTRL_PHASE_ADDR | (phase_stop << 9) |
		(num & CTRL_DATA_COUNT) | CTRL_DIR);

	if (!num) {
		reg &= (~CTRL_PHASE_DATA);
	} else {
		reg |= CTRL_PHASE_DATA;
	}

	OUTWORD(I2C_CTRL(dev), reg);

	/* Step3 init I2C info */
	i2c_info->slave_addr = addr;
	i2c_info->xfered_data_rd_ptr = 0;
	i2c_info->xfer_rd_num = num;
	i2c_info->xfer_cmpl_count = 0;
	i2c_info->middleware_rx_buf = (uint8_t *)data;

	/* I2C slave address, general call address = 0x0(7-bit or 10-bit) */
	reg = INWORD(I2C_ADDR(dev));
	reg &= (~SLAVE_ADDR_MSK);
	reg |= (i2c_info->slave_addr & (SLAVE_ADDR_MSK));
	OUTWORD(I2C_ADDR(dev), reg);

	/*
	 * Step4 Enable Interrupts: Complete, Arbitration Lose
	 * Enable/Disable the FIFO Full Interrupt
	 */
	reg = INWORD(I2C_INTE(dev));
	reg |= (IEN_CMPL | IEN_FIFO_FULL | IEN_ARB_LOSE);
	OUTWORD(I2C_INTE(dev), reg);

	/* Step5, I2C Write 0x1 to the Command register to issue the transaction */
	reg = INWORD(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_ISSUE_TRANSACTION);
	OUTWORD(I2C_CMD(dev), reg);

	return DRIVER_OK;
}

static int i2c_atciic100_slave_send(const struct device *dev,
	const uint8_t *data, uint32_t num)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t reg = 0;

	if (k_sem_take(&i2c_info->i2c_busy_sem, K_MSEC(1000))) {
		return DRIVER_ERROR_TIMEOUT;
	}

	/* Clear FIFO */
	reg = INWORD(I2C_CMD(dev));
	reg &= (~CMD_MSK);
	reg |= (CMD_CLEAR_FIFO);
	OUTWORD(I2C_CMD(dev), reg);

	/* Direction => tx:0, rx:1 */
	i2c_info->status.direction = 0;
	i2c_info->status.bus_error = 0;
	i2c_info->xfered_data_wt_ptr = 0;
	i2c_info->xfer_wt_num = num;
	i2c_info->xfer_cmpl_count = 0;
	i2c_info->middleware_tx_buf = (uint8_t *)data;

	i2c_slave_fifo_write(dev);

	reg = INWORD(I2C_INTE(dev));
	reg |= (IEN_FIFO_EMPTY | IEN_CMPL);
	OUTWORD(I2C_INTE(dev), reg);

	return DRIVER_OK;
}

static int i2c_atciic100_slave_receive(const struct device *dev,
	uint8_t *data, uint32_t num)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;

	/*
	 * since middleware just read data from Xfer_Data_Rd_Buf,
	 * no set busy flag, driver slave rx is independent to
	 * middleware slave rx, if set busy flag will affect
	 * slave tx behavior
	 * Direction => tx:0, rx:1
	 */
	i2c_info->status.direction = 1;
	i2c_info->status.bus_error = 0;

	/*
	 * no I2C reset controller, no I2C clear fifo since middleware just
	 * read data from Xfer_Data_Rd_Buf
	 * with minimal change I2C HW setting
	 * Xfer_Data_Rd_Buf already read the data from hw fifo and keep,
	 * currently middleware able to take from the buffer
	 * error hit
	 */
	if (num > MAX_XFER_SZ) {
		return DRIVER_ERROR;
	}

	MEMCPY(data, &i2c_info->xfer_data_rd_buf[i2c_info->middleware_rx_ptr], num);

	i2c_info->xfer_rd_num = num;
	i2c_info->xfer_cmpl_count = 0;
	i2c_info->middleware_rx_ptr += num;

	/* slave rx is no blocking, blocking operation is owned by middleware */

	return DRIVER_OK;
}

static void i2c_master_fifo_write(const struct device *dev, uint8_t is_init)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t i = 0, write_fifo_count = 0, reg = 0;
	uint8_t write_data;

	write_fifo_count = i2c_info->xfer_wt_num - i2c_info->xfered_data_wt_ptr;

	if (write_fifo_count >= i2c_info->fifo_depth) {
		write_fifo_count = i2c_info->fifo_depth;
	}

	if (is_init) {
		write_fifo_count = 2;
	}

	/* I2C write a patch of data(FIFO_Depth) to FIFO */
	for (i = 0; i < write_fifo_count; i++) {

		write_data = i2c_info->middleware_tx_buf[i2c_info->xfered_data_wt_ptr];
		OUTWORD(I2C_DATA(dev), (write_data & DATA_MSK));
		i2c_info->xfered_data_wt_ptr++;

		/* Disable the FIFO Empty Interrupt if no more data to send */
		if (i2c_info->xfered_data_wt_ptr == i2c_info->xfer_wt_num) {
			reg = INWORD(I2C_INTE(dev));
			reg &= (~IEN_FIFO_EMPTY);
			OUTWORD(I2C_INTE(dev), reg);
		}
	}
}

/* basic fifo read function */
static void i2c_master_fifo_read(const struct device *dev)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t i = 0, read_fifo_count = 0, reg = 0;
	uint8_t read_data;

	read_fifo_count = i2c_info->xfer_rd_num - i2c_info->xfered_data_rd_ptr;

	if (read_fifo_count >= i2c_info->fifo_depth) {
		read_fifo_count = i2c_info->fifo_depth;
	}

	/* I2C read a patch of data(FIFO_Depth) from FIFO */
	for (i = 0; i < read_fifo_count; i++) {

		read_data = INWORD(I2C_DATA(dev)) & DATA_MSK;

		i2c_info->middleware_rx_buf[i2c_info->xfered_data_rd_ptr] = read_data;
		i2c_info->xfered_data_rd_ptr++;

		/* Disable the FIFO Full Interrupt if no more data to receive */
		if (i2c_info->xfered_data_rd_ptr == i2c_info->xfer_rd_num) {
			reg = INWORD(I2C_INTE(dev));
			reg &= (~IEN_FIFO_FULL);
			OUTWORD(I2C_INTE(dev), reg);
		}
	}
}

static void i2c_slave_fifo_write(const struct device *dev)
{
	/*
	 * slave TX 1 byte each time, since no information about number of data
	 * to transfer, check nack_assert to complete slave tx
	 */
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint8_t write_data;

	write_data = i2c_info->middleware_tx_buf[i2c_info->xfered_data_wt_ptr];
	OUTWORD(I2C_DATA(dev), (write_data & DATA_MSK));

	i2c_info->xfered_data_wt_ptr++;
	i2c_info->xfer_cmpl_count++;
}

static void i2c_slave_fifo_read(const struct device *dev, uint8_t is_fifo_full)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t i = 0, read_fifo_count = 0, curr_rx_data_count = 0;
	uint8_t read_data;

	/*
	 * slave rx data count depend on master tx data count
	 * it may be more than fifo length(4 bytes)
	 * slave: If DMA is not enabled(FIFO mode), DataCnt is the number of
	 * bytes transmitted/received from the bus master.
	 * It is reset to 0 when the controller is addressed
	 * and then increased by one for each byte of data
	 * transmitted/received
	 */
	curr_rx_data_count = i2c_info->slave_rx_cmpl_ctrl_reg_val & CTRL_DATA_COUNT;

	/* error hit */
	if (curr_rx_data_count > MAX_XFER_SZ) {
		while (1) {
			;
		}
	}

	if (is_fifo_full) {
		read_fifo_count = i2c_info->fifo_depth;
	} else {
		read_fifo_count = curr_rx_data_count - i2c_info->last_rx_data_count;
	}

	/* error hit */
	if (read_fifo_count > MAX_XFER_SZ) {
		while (1) {
			;
		}
	}

	/* I2C read a patch of data(FIFO_Depth) from FIFO */
	for (i = 0; i < read_fifo_count; i++) {
		read_data = INWORD(I2C_DATA(dev)) & DATA_MSK;

		i2c_info->xfer_data_rd_buf[i2c_info->xfered_data_rd_ptr] = read_data;
		i2c_info->xfered_data_rd_ptr++;

		if (i2c_info->xfered_data_rd_ptr == MAX_XFER_SZ) {
			/* slave rx buffer overwrite */
			i2c_info->xfered_data_rd_ptr_ow++;
			i2c_info->xfered_data_rd_ptr = 0;
		}
	}
	i2c_info->last_rx_data_count = curr_rx_data_count;
}

static void i2c_fifo_empty_handler(const struct device *dev)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;

	if (i2c_info->driver_state & I2C_DRV_MASTER_TX) {
		i2c_master_fifo_write(dev, 0);
	} else if (i2c_info->driver_state & I2C_DRV_SLAVE_TX) {
		i2c_slave_fifo_write(dev);
	}
}

static void i2c_fifo_full_handler(const struct device *dev)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;

	if (i2c_info->driver_state & I2C_DRV_MASTER_RX) {
		i2c_master_fifo_read(dev);
	} else if (i2c_info->driver_state & I2C_DRV_SLAVE_RX) {
		i2c_slave_fifo_read(dev, 1);
	}
}

static void i2c_cmpl_handler(const struct device *dev)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t reg_set = 0, reg_ctrl = 0, reg_int = 0, reg_cmd = 0;

	reg_set = INWORD(I2C_SET(dev));

	/* master mode */
	if (reg_set & SETUP_MASTER) {
		/* Disable all I2C Interrupts */
		reg_int = INWORD(I2C_INTE(dev));
		reg_int &= (~IEN_ALL);
		OUTWORD(I2C_INTE(dev), reg_int);
	} else {
		/* Disable I2C FIFO Full, EMPTY Interrupt */
		reg_int = INWORD(I2C_INTE(dev));
		reg_int &= (~(IEN_FIFO_FULL | IEN_FIFO_EMPTY));
		OUTWORD(I2C_INTE(dev), reg_int);
	}

	/*
	 * check the DataCnt field of the Control Register
	 * to know if all data are successfully transmitted.
	 * -> Master: The number of bytes to transmit/receive.
	 * 0 means 256 bytes. DataCnt will be decreased by one
	 * for each byte transmitted/received.
	 */
	if (i2c_info->driver_state & (I2C_DRV_MASTER_TX | I2C_DRV_MASTER_RX)) {

		/* Get the remain number of data */
		reg_ctrl = INWORD(I2C_CTRL(dev)) & CTRL_DATA_COUNT;

		if (i2c_info->driver_state & I2C_DRV_MASTER_TX) {
			i2c_info->xfer_cmpl_count = i2c_info->xfer_wt_num - reg_ctrl;

			/* clear & set driver state to master tx complete */
			i2c_info->driver_state = I2C_DRV_MASTER_TX_CMPL;

			/* clear busy bit on i2c complete event as master dma/cpu tx */
			k_sem_give(&i2c_info->i2c_busy_sem);
		}

		if (i2c_info->driver_state & I2C_DRV_MASTER_RX) {
			i2c_master_fifo_read(dev);

			i2c_info->xfer_cmpl_count = i2c_info->xfer_rd_num - reg_ctrl;

			/* clear & set driver state to master rx complete */
			i2c_info->driver_state = I2C_DRV_MASTER_RX_CMPL;

			/* clear busy bit on i2c complete event as master cpu rx */
			k_sem_give(&i2c_info->i2c_busy_sem);
		}
	}

	/*
	 * check the DataCnt field of the Control Register
	 * to know if all data are successfully transmitted.
	 * -> Slave: DataCnt is the number of
	 * bytes transmitted/received from the bus master.
	 * It is reset to 0 when the controller is addressed
	 * and then increased by one for each byte of data
	 * transmitted/received.
	 */
	if (i2c_info->driver_state & (I2C_DRV_SLAVE_TX | I2C_DRV_SLAVE_RX)) {
		reg_set = INWORD(I2C_SET(dev));
		reg_ctrl = INWORD(I2C_CTRL(dev));

		if (i2c_info->driver_state & I2C_DRV_SLAVE_TX) {
			/* clear & set driver state to slave tx complete */
			i2c_info->xfer_cmpl_count = i2c_info->xfered_data_rd_ptr - i2c_info->middleware_rx_ptr;
			i2c_info->driver_state = I2C_DRV_SLAVE_TX_CMPL;
			slave_config->flags = I2C_FLAG_SLAVE_TX_DONE;
			slave_callbacks->write_received(slave_config, i2c_info->xfer_cmpl_count);
		}

		if (i2c_info->driver_state & I2C_DRV_SLAVE_RX) {
			i2c_slave_fifo_read(dev, 0);

			i2c_info->xfer_cmpl_count = i2c_info->xfered_data_rd_ptr - i2c_info->middleware_rx_ptr;

			/* clear & set driver state to slave rx complete */
			i2c_info->driver_state = I2C_DRV_SLAVE_RX_CMPL;
			slave_config->flags = I2C_FLAG_SLAVE_RX_DONE;
			slave_callbacks->read_processed(slave_config, &(i2c_info->xfer_cmpl_count));
		}

		/* if the Completion Interrupt asserts, clear the FIFO and go next transaction. */
		reg_cmd = INWORD(I2C_CMD(dev));
		reg_cmd &= (~CMD_MSK);
		reg_cmd |= (CMD_CLEAR_FIFO);
		OUTWORD(I2C_CMD(dev), reg_cmd);

		/* Release busy flag as slave cpu tx/rx complete */
		if (k_sem_count_get(&i2c_info->i2c_busy_sem) == 0) {
			k_sem_give(&i2c_info->i2c_busy_sem);
		}
	}
}

static void i2c_slave_addr_hit_handler(const struct device *dev)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint32_t reg_int = 0;

	/* slave mode Rx: if address hit, fifo may not be full state */
	if (i2c_info->driver_state & I2C_DRV_SLAVE_RX) {
		/* A new I2C data transaction(start-addr-data-stop) */
		i2c_info->xfered_data_rd_ptr_ow = 0;
		i2c_info->xfered_data_rd_ptr = 0;

		/* keypoint to reset */
		i2c_info->middleware_rx_ptr = 0;
		i2c_info->last_rx_data_count = 0;
		MEMSET(i2c_info->xfer_data_rd_buf, 0, sizeof(i2c_info->xfer_data_rd_buf));

		/* Enable I2C FIFO Full Interrupt */
		reg_int = INWORD(I2C_INTE(dev));
		reg_int |= IEN_FIFO_FULL;
		OUTWORD(I2C_INTE(dev), reg_int);
	}
}

static void i2c_atciic100_irq_handler(void *arg)
{
	const struct device *dev = (struct device *)arg;
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;

	uint32_t reg_set, reg_stat = 0, reg_ctrl = 0;
	uint8_t  evt_id = 0;

	reg_stat = INWORD(I2C_STAT(dev));
	reg_set = INWORD(I2C_SET(dev));
	reg_ctrl = INWORD(I2C_CTRL(dev));

	/* Clear interrupts status */
	OUTWORD(I2C_STAT(dev), (reg_stat & STATUS_W1C_ALL));

	if (reg_stat & STATUS_FIFO_EMPTY) {
		i2c_fifo_empty_handler(dev);
	}

	if (reg_stat & STATUS_FIFO_FULL) {
		/* Store hw receive data count quickly */
		i2c_info->slave_rx_cmpl_ctrl_reg_val = reg_ctrl;
		i2c_fifo_full_handler(dev);
	}

	if (reg_stat & STATUS_CMPL) {
		/* Store hw receive data count quickly */
		i2c_info->slave_rx_cmpl_ctrl_reg_val = reg_ctrl;
		i2c_cmpl_handler(dev);
	}

	if ((reg_stat & STATUS_ARB_LOSE) && (reg_set & SETUP_MASTER)) {
		i2c_info->status.arbitration_lost = 1;
	}


	/*
	 * Here is the entry for slave mode driver to detect
	 * slave RX/TX action depend on master TX/RX action.
	 * A new I2C data transaction(start-addr-data-stop)
	 */
	if (reg_stat & STATUS_ADDR_HIT) {
		/* slave mode */
		if (!(reg_set & SETUP_MASTER)) {
			/* cleared on start of next Slave operation */
			i2c_info->status.general_call = 0;

			if (reg_stat & STATUS_GEN_CALL) {
				i2c_info->status.general_call = 1;

				/* I2C General Call slave address */
				evt_id |= I2C_FLAG_GENERAL_CALL;
				slave_config->flags |= evt_id;
			}

			if (((reg_ctrl & CTRL_DIR) >> 8) == I2C_SLAVE_RX) {
				if (k_sem_take(&i2c_info->i2c_busy_sem, K_NO_WAIT) == 0) {

					/* notify middleware to do slave rx action */
					evt_id |= I2C_FLAG_SLAVE_RX_START;
					slave_config->flags |= evt_id;
					i2c_info->driver_state = I2C_DRV_SLAVE_RX;
					i2c_slave_addr_hit_handler(dev);
					slave_callbacks->read_requested(slave_config, NULL);
				}

			} else if (((reg_ctrl & CTRL_DIR) >> 8) == I2C_SLAVE_TX) {
				/* notify middleware to do slave tx action */
				evt_id |= I2C_FLAG_SLAVE_TX_START;
				slave_config->flags |= evt_id;
				i2c_info->driver_state = I2C_DRV_SLAVE_TX;
				i2c_slave_addr_hit_handler(dev);
				slave_callbacks->write_requested(slave_config);
			}
		}
	}
}

static int i2c_atciic100_slave_register(const struct device *dev,
	struct i2c_slave_config *cfg)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;
	uint16_t reg_addr = 0;
	int32_t status = DRIVER_OK;

	i2c_info->status.mode = 0;

	reg_addr &= (~SLAVE_ADDR_MSK);
	reg_addr |= (cfg->address & (SLAVE_ADDR_MSK));

	OUTWORD(I2C_ADDR(dev), reg_addr);

	slave_callbacks = cfg->callbacks;
	slave_config = cfg;

	return status;
}

static int i2c_atciic100_slave_unregister(const struct device *dev,
	struct i2c_slave_config *cfg)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);
	struct _i2c_info *i2c_info = dev_cfg->i2c_info;

	i2c_info->status.mode = 1;
	return DRIVER_OK;
}

struct i2c_atciic100_dev_data_t {
	/* list of callbacks */
	sys_slist_t cb;
};

struct _i2c_info info = {0};

static const struct i2c_atciic100_device_config i2c_atciic100_config_0 = {
	.i2c_base_addr	= DT_INST_REG_ADDR(0),
	.i2c_irq_num	= DT_INST_IRQN(0),
	.i2c_info	= &info
};

static const struct i2c_driver_api i2c_atciic100_driver = {
	.configure		= (i2c_api_configure_t)i2c_atciic100_configure,
	.get_config		= (i2c_api_get_config_t)i2c_atciic100_get_config,
	.transfer		= (i2c_api_full_io_t)i2c_atciic100_transfer,
	.slave_register		= (i2c_api_slave_register_t)i2c_atciic100_slave_register,
	.slave_unregister       = (i2c_api_slave_unregister_t)i2c_atciic100_slave_unregister
};

static struct i2c_atciic100_dev_data_t i2c_atciic100_dev_data_0;

DEVICE_DT_INST_DEFINE(0,
	i2c_atciic100_init,
	NULL,
	&i2c_atciic100_dev_data_0,
	&i2c_atciic100_config_0,
	POST_KERNEL,
	CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	&i2c_atciic100_driver);

static int i2c_atciic100_init(const struct device *dev)
{
	const struct i2c_atciic100_device_config *dev_cfg = DEV_I2C_CFG(dev);

	/* Disable all interrupts */
	OUTWORD(I2C_INTE(dev), 0x00000000);

	/* Clear interrupt status */
	OUTWORD(I2C_STAT(dev), 0xFFFFFFFF);

	IRQ_CONNECT(DT_INST_IRQN(0),
		DT_INST_IRQ(0, priority),
		i2c_atciic100_irq_handler,
		DEVICE_DT_INST_GET(0),
		0);

	i2c_atciic100_default_control(dev);

	irq_enable(dev_cfg->i2c_irq_num);

	return DRIVER_OK;
}
