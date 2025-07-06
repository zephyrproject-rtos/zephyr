/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_i2c

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_numaker, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"
#include <soc.h>
#include <NuMicro.h>

/* i2c Master Mode Status */
#define  M_START               0x08  /* Start */
#define  M_REPEAT_START        0x10  /* Master Repeat Start */
#define  M_TRAN_ADDR_ACK       0x18  /* Master Transmit Address ACK */
#define  M_TRAN_ADDR_NACK      0x20  /* Master Transmit Address NACK */
#define  M_TRAN_DATA_ACK       0x28  /* Master Transmit Data ACK */
#define  M_TRAN_DATA_NACK      0x30  /* Master Transmit Data NACK */
#define  M_ARB_LOST            0x38  /* Master Arbitration Los */
#define  M_RECE_ADDR_ACK       0x40  /* Master Receive Address ACK */
#define  M_RECE_ADDR_NACK      0x48  /* Master Receive Address NACK */
#define  M_RECE_DATA_ACK       0x50  /* Master Receive Data ACK */
#define  M_RECE_DATA_NACK      0x58  /* Master Receive Data NACK */
#define  BUS_ERROR             0x00  /* Bus error */

/* i2c Slave Mode Status */
#define  S_REPEAT_START_STOP   0xA0  /* Slave Transmit Repeat Start or Stop */
#define  S_TRAN_ADDR_ACK       0xA8  /* Slave Transmit Address ACK */
#define  S_TRAN_DATA_ACK       0xB8  /* Slave Transmit Data ACK */
#define  S_TRAN_DATA_NACK      0xC0  /* Slave Transmit Data NACK */
#define  S_TRAN_LAST_DATA_ACK  0xC8  /* Slave Transmit Last Data ACK */
#define  S_RECE_ADDR_ACK       0x60  /* Slave Receive Address ACK */
#define  S_RECE_ARB_LOST       0x68  /* Slave Receive Arbitration Lost */
#define  S_RECE_DATA_ACK       0x80  /* Slave Receive Data ACK */
#define  S_RECE_DATA_NACK      0x88  /* Slave Receive Data NACK */

/* i2c GC Mode Status */
#define  GC_ADDR_ACK           0x70  /* GC mode Address ACK */
#define  GC_ARB_LOST           0x78  /* GC mode Arbitration Lost */
#define  GC_DATA_ACK           0x90  /* GC mode Data ACK */
#define  GC_DATA_NACK          0x98  /* GC mode Data NACK */

/* i2c Other Status */
#define  ADDR_TRAN_ARB_LOST    0xB0  /* Address Transmit Arbitration Lost */
#define  BUS_RELEASED          0xF8  /* Bus Released */

struct i2c_numaker_config {
	I2C_T *i2c_base;
	const struct reset_dt_spec reset;
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clkctrl_dev;
	uint32_t irq_n;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	uint32_t bitrate;
};

struct i2c_numaker_data {
	struct k_sem lock;
	uint32_t dev_config;
	/* Master transfer context */
	struct {
		struct k_sem xfer_sync;
		uint16_t addr;
		struct i2c_msg *msgs_beg;
		struct i2c_msg *msgs_pos;
		struct i2c_msg *msgs_end;
		uint8_t *buf_beg;
		uint8_t *buf_pos;
		uint8_t *buf_end;
	} master_xfer;
#ifdef CONFIG_I2C_TARGET
	/* Slave transfer context */
	struct {
		struct i2c_target_config *slave_config;
		bool slave_addressed;
	} slave_xfer;
#endif
};

/* ACK/NACK last data byte, dependent on whether or not message merge is allowed */
static void m_numaker_i2c_master_xfer_msg_read_last_byte(const struct device *dev)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;

	/* Shouldn't invoke with message pointer OOB */
	__ASSERT_NO_MSG(data->master_xfer.msgs_pos < data->master_xfer.msgs_end);
	/* Should invoke with exactly one data byte remaining for read */
	__ASSERT_NO_MSG((data->master_xfer.msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ);
	__ASSERT_NO_MSG((data->master_xfer.buf_end - data->master_xfer.buf_pos) == 1);

	/* Flags of previous message */
	bool do_stop_prev = data->master_xfer.msgs_pos->flags & I2C_MSG_STOP;

	/* Advance to next messages temporarily */
	data->master_xfer.msgs_pos++;

	/* Has next message? */
	if (data->master_xfer.msgs_pos < data->master_xfer.msgs_end) {
		/* Flags of next message */
		struct i2c_msg *msgs_pos = data->master_xfer.msgs_pos;
		bool is_read_next = (msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		bool do_restart_next = data->master_xfer.msgs_pos->flags & I2C_MSG_RESTART;

		/*
		 * Different R/W bit so message merge is disallowed.
		 * Force I2C Repeat Start on I2C Stop/Repeat Start missing
		 */
		if (!is_read_next) {
			if (!do_stop_prev && !do_restart_next) {
				do_restart_next = true;
			}
		}

		if (do_stop_prev || do_restart_next) {
			/* NACK last data byte (required for Master Receiver) */
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
		} else {
			/* ACK last data byte, so to merge adjacent messages into one transaction */
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		}
	} else {
		/* NACK last data byte (required for Master Receiver) */
		I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
	}

	/* Roll back message pointer */
	data->master_xfer.msgs_pos--;
}

/* End the transfer, involving I2C Stop and signal to thread */
static void m_numaker_i2c_master_xfer_end(const struct device *dev, bool do_stop)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;

	if (do_stop) {
		/* Do I2C Stop */
		I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk);
	}

	/* Signal master transfer end */
	k_sem_give(&data->master_xfer.xfer_sync);
}

static void m_numaker_i2c_master_xfer_msg_end(const struct device *dev);
/* Read next data byte, involving ACK/NACK last data byte and message merge */
static void m_numaker_i2c_master_xfer_msg_read_next_byte(const struct device *dev)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;

	switch (data->master_xfer.buf_end - data->master_xfer.buf_pos) {
	case 0:
		/* Last data byte ACKed, we'll do message merge */
		m_numaker_i2c_master_xfer_msg_end(dev);
		break;
	case 1:
		/* Read last data byte for this message */
		m_numaker_i2c_master_xfer_msg_read_last_byte(dev);
		break;
	default:
		/* ACK non-last data byte */
		I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
	}
}

/* End one message transfer, involving message merge and transfer end */
static void m_numaker_i2c_master_xfer_msg_end(const struct device *dev)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;

	/* Shouldn't invoke with message pointer OOB */
	__ASSERT_NO_MSG(data->master_xfer.msgs_pos < data->master_xfer.msgs_end);
	/* Should have transferred up */
	__ASSERT_NO_MSG((data->master_xfer.buf_end - data->master_xfer.buf_pos) == 0);

	/* Flags of previous message */
	bool is_read_prev = (data->master_xfer.msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
	bool do_stop_prev = data->master_xfer.msgs_pos->flags & I2C_MSG_STOP;

	/* Advance to next messages */
	data->master_xfer.msgs_pos++;

	/* Has next message? */
	if (data->master_xfer.msgs_pos < data->master_xfer.msgs_end) {
		/* Flags of next message */
		struct i2c_msg *msgs_pos = data->master_xfer.msgs_pos;
		bool is_read_next = (msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		bool do_restart_next = data->master_xfer.msgs_pos->flags & I2C_MSG_RESTART;

		/*
		 * Different R/W bit so message merge is disallowed.
		 * Force I2C Repeat Start on I2C Stop/Repeat Start missing
		 */
		if (!is_read_prev != !is_read_next) {   /* Logical XOR idiom */
			if (!do_stop_prev && !do_restart_next) {
				LOG_WRN("Cannot merge adjacent messages, force I2C Repeat Start");
				do_restart_next = true;
			}
		}

		if (do_stop_prev) {
			/* Do I2C Stop and then Start */
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_STA_Msk |
					    I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk);
		} else if (do_restart_next) {
			/* Do I2C Repeat Start */
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_STA_Msk | I2C_CTL0_SI_Msk);
		} else {
			/* Merge into the same transaction */

			/* Prepare buffer for current message */
			data->master_xfer.buf_beg = data->master_xfer.msgs_pos->buf;
			data->master_xfer.buf_pos = data->master_xfer.msgs_pos->buf;
			data->master_xfer.buf_end = data->master_xfer.msgs_pos->buf +
						    data->master_xfer.msgs_pos->len;

			if (is_read_prev) {
				m_numaker_i2c_master_xfer_msg_read_next_byte(dev);
			} else {
				/*
				 * Interrupt flag not cleared, expect to re-enter ISR with
				 * context unchanged, except buffer changed for message change.
				 */
			}
		}
	} else {
		if (!do_stop_prev) {
			LOG_WRN("Last message not marked I2C Stop");
		}

		m_numaker_i2c_master_xfer_end(dev, do_stop_prev);
	}
}

static int i2c_numaker_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	uint32_t bitrate;

	/* Check address size */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bits address not supported");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		bitrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		bitrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		bitrate = MHZ(1);
		break;
	default:
		LOG_ERR("Speed code %d not supported", I2C_SPEED_GET(dev_config));
		return -ENOTSUP;
	}

	I2C_T *i2c_base = config->i2c_base;
	int err = 0;

	k_sem_take(&data->lock, K_FOREVER);
	irq_disable(config->irq_n);

#ifdef CONFIG_I2C_TARGET
	if (data->slave_xfer.slave_addressed) {
		LOG_ERR("Reconfigure with slave being busy");
		err = -EBUSY;
		goto done;
	}
#endif

	I2C_Open(i2c_base, bitrate);
	/* INTEN bit and FSM control bits (STA, STO, SI, AA) are packed in one register CTL0. */
	i2c_base->CTL0 |= (I2C_CTL0_INTEN_Msk | I2C_CTL0_I2CEN_Msk);
	data->dev_config = dev_config;

done:

	irq_enable(config->irq_n);
	k_sem_give(&data->lock);

	return err;
}

static int i2c_numaker_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_numaker_data *data = dev->data;

	if (!dev_config) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	*dev_config = data->dev_config;
	k_sem_give(&data->lock);

	return 0;
}

/*
 * Master active transfer:
 * 1. Do I2C Start to start the transfer (thread)
 * 2. I2C FSM (ISR)
 * 3. Force I2C Stop to end the transfer (thread)
 * Slave passive transfer:
 * 1. Prepare callback (thread)
 * 2. Do data transfer via above callback (ISR)
 */
static int i2c_numaker_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;
	int err = 0;

	k_sem_take(&data->lock, K_FOREVER);
	irq_disable(config->irq_n);

	if (data->slave_xfer.slave_addressed) {
		LOG_ERR("Master transfer with slave being busy");
		err = -EBUSY;
		goto cleanup;
	}

	if (num_msgs == 0) {
		goto cleanup;
	}

	/* Prepare to start transfer */
	data->master_xfer.addr = addr;
	data->master_xfer.msgs_beg = msgs;
	data->master_xfer.msgs_pos = msgs;
	data->master_xfer.msgs_end = msgs + num_msgs;

	/* Do I2C Start to start the transfer */
	I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_STA_Msk | I2C_CTL0_SI_Msk);

	irq_enable(config->irq_n);
	k_sem_take(&data->master_xfer.xfer_sync, K_FOREVER);
	irq_disable(config->irq_n);

	/* Check transfer result */
	if (data->master_xfer.msgs_pos != data->master_xfer.msgs_end) {
		bool is_read;
		bool is_10bit;

		is_read = (data->master_xfer.msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		is_10bit = data->master_xfer.msgs_pos->flags & I2C_MSG_ADDR_10_BITS;
		LOG_ERR("Failed message:");
		LOG_ERR("MSG IDX: %d", data->master_xfer.msgs_pos - data->master_xfer.msgs_beg);
		LOG_ERR("ADDR (%d-bit): 0x%04X", is_10bit ? 10 : 7, addr);
		LOG_ERR("DIR: %s", is_read ? "R" : "W");
		LOG_ERR("Expected %d bytes transferred, but actual %d",
			data->master_xfer.msgs_pos->len,
			data->master_xfer.buf_pos - data->master_xfer.buf_beg);
		err = -EIO;
		goto i2c_stop;
	}

i2c_stop:

	/* Do I2C Stop to release bus ownership */
	I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk);

#ifdef CONFIG_I2C_TARGET
	/* Enable slave mode if one slave is registered */
	if (data->slave_xfer.slave_config) {
		I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
	}
#endif

cleanup:

	irq_enable(config->irq_n);
	k_sem_give(&data->lock);

	return err;
}

#ifdef CONFIG_I2C_TARGET
static int i2c_numaker_slave_register(const struct device *dev,
				      struct i2c_target_config *slave_config)
{
	if (!slave_config || !slave_config->callbacks) {
		return -EINVAL;
	}

	if (slave_config->flags & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bits address not supported");
		return -ENOTSUP;
	}

	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;
	int err = 0;

	k_sem_take(&data->lock, K_FOREVER);
	irq_disable(config->irq_n);

	if (data->slave_xfer.slave_config) {
		err = -EBUSY;
		goto cleanup;
	}

	data->slave_xfer.slave_config = slave_config;
	/* Slave address */
	I2C_SetSlaveAddr(i2c_base,
			 0,
			 slave_config->address,
			 I2C_GCMODE_DISABLE);

	/* Slave address state */
	data->slave_xfer.slave_addressed = false;

	/* Enable slave mode */
	I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);

cleanup:

	irq_enable(config->irq_n);
	k_sem_give(&data->lock);

	return err;
}

static int i2c_numaker_slave_unregister(const struct device *dev,
					struct i2c_target_config *slave_config)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;
	int err = 0;

	if (!slave_config) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	irq_disable(config->irq_n);

	if (data->slave_xfer.slave_config != slave_config) {
		err = -EINVAL;
		goto cleanup;
	}

	if (data->slave_xfer.slave_addressed) {
		LOG_ERR("Unregister slave driver with slave being busy");
		err = -EBUSY;
		goto cleanup;
	}

	/* Slave address: Zero */
	I2C_SetSlaveAddr(i2c_base,
			 0,
			 0,
			 I2C_GCMODE_DISABLE);

	/* Slave address state */
	data->slave_xfer.slave_addressed = false;

	/* Disable slave mode */
	I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
	data->slave_xfer.slave_config = NULL;

cleanup:

	irq_enable(config->irq_n);
	k_sem_give(&data->lock);

	return err;
}
#endif

static int i2c_numaker_recover_bus(const struct device *dev)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;

	k_sem_take(&data->lock, K_FOREVER);
	/* Do I2C Stop to release bus ownership */
	I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk);
	k_sem_give(&data->lock);

	return 0;
}

static void i2c_numaker_isr(const struct device *dev)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	I2C_T *i2c_base = config->i2c_base;
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *slave_config = data->slave_xfer.slave_config;
	const struct i2c_target_callbacks *slave_callbacks =
		slave_config ? slave_config->callbacks : NULL;
	uint8_t data_byte;
#endif
	uint32_t status;

	if (I2C_GET_TIMEOUT_FLAG(i2c_base)) {
		I2C_ClearTimeoutFlag(i2c_base);
		return;
	}

	status = I2C_GET_STATUS(i2c_base);

	switch (status) {
	case M_START: /* Start */
	case M_REPEAT_START: /* Master Repeat Start */
		/* Prepare buffer for current message */
		data->master_xfer.buf_beg = data->master_xfer.msgs_pos->buf;
		data->master_xfer.buf_pos = data->master_xfer.msgs_pos->buf;
		data->master_xfer.buf_end = data->master_xfer.msgs_pos->buf +
					      data->master_xfer.msgs_pos->len;

		/* Write I2C address */
		struct i2c_msg *msgs_pos = data->master_xfer.msgs_pos;
		bool is_read = (msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		uint16_t addr = data->master_xfer.addr;
		int addr_rw = is_read ? ((addr << 1) | 1) : (addr << 1);

		I2C_SET_DATA(i2c_base, (uint8_t) (addr_rw & 0xFF));
		I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
		break;
	case M_TRAN_ADDR_ACK: /* Master Transmit Address ACK */
	case M_TRAN_DATA_ACK: /* Master Transmit Data ACK */
		__ASSERT_NO_MSG(data->master_xfer.buf_pos);
		if (data->master_xfer.buf_pos < data->master_xfer.buf_end) {
			I2C_SET_DATA(i2c_base, *data->master_xfer.buf_pos++);
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* End this message */
			m_numaker_i2c_master_xfer_msg_end(dev);
		}
		break;
	case M_TRAN_ADDR_NACK:  /* Master Transmit Address NACK */
	case M_TRAN_DATA_NACK:  /* Master Transmit Data NACK */
	case M_RECE_ADDR_NACK:  /* Master Receive Address NACK */
	case M_ARB_LOST:  /* Master Arbitration Lost */
		m_numaker_i2c_master_xfer_end(dev, true);
		break;
	case M_RECE_ADDR_ACK:  /* Master Receive Address ACK */
	case M_RECE_DATA_ACK:  /* Master Receive Data ACK */
		__ASSERT_NO_MSG(data->master_xfer.buf_pos);

		if (status == M_RECE_ADDR_ACK) {
			__ASSERT_NO_MSG(data->master_xfer.buf_pos < data->master_xfer.buf_end);
		} else if (status == M_RECE_DATA_ACK) {
			__ASSERT_NO_MSG((data->master_xfer.buf_end -
					 data->master_xfer.buf_pos) >= 1);
			*data->master_xfer.buf_pos++ = I2C_GET_DATA(i2c_base);
		}

		m_numaker_i2c_master_xfer_msg_read_next_byte(dev);
		break;
	case M_RECE_DATA_NACK:  /* Master Receive Data NACK */
		__ASSERT_NO_MSG((data->master_xfer.buf_end - data->master_xfer.buf_pos) == 1);
		*data->master_xfer.buf_pos++ = I2C_GET_DATA(i2c_base);
		/* End this message */
		m_numaker_i2c_master_xfer_msg_end(dev);
		break;
	case BUS_ERROR:	 /* Bus error */
		m_numaker_i2c_master_xfer_end(dev, true);
		break;
#ifdef CONFIG_I2C_TARGET
	/* NOTE: Don't disable interrupt here because slave mode relies on */
	/* for passive transfer in ISR. */

	/* Slave Transmit */
	case S_TRAN_ADDR_ACK:  /* Slave Transmit Address ACK */
	case ADDR_TRAN_ARB_LOST:  /* Slave Transmit Arbitration Lost */
		data->slave_xfer.slave_addressed = true;
		if (slave_callbacks->read_requested(slave_config, &data_byte) == 0) {
			/* Non-last data byte */
			I2C_SET_DATA(i2c_base, data_byte);
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* Go S_TRAN_LAST_DATA_ACK on error */
			I2C_SET_DATA(i2c_base, 0xFF);
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
		}
		break;
	case S_TRAN_DATA_ACK:  /* Slave Transmit Data ACK */
		if (slave_callbacks->read_processed(slave_config, &data_byte) == 0) {
			/* Non-last data byte */
			I2C_SET_DATA(i2c_base, data_byte);
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* Go S_TRAN_LAST_DATA_ACK on error */
			I2C_SET_DATA(i2c_base, 0xFF);
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
		}
		break;
	case S_TRAN_DATA_NACK:  /* Slave Transmit Data NACK */
	case S_TRAN_LAST_DATA_ACK:  /* Slave Transmit Last Data ACK */
		/* Go slave end */
		data->slave_xfer.slave_addressed = false;
		slave_callbacks->stop(slave_config);
		I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		break;
		/* Slave Receive */
	case S_RECE_DATA_ACK:  /* Slave Receive Data ACK */
		data_byte = I2C_GET_DATA(i2c_base);
		if (slave_callbacks->write_received(slave_config, data_byte) == 0) {
			/* Write OK, ACK next data byte */
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* Write FAILED, NACK next data byte */
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
		}
		break;
	case S_RECE_DATA_NACK:  /* Slave Receive Data NACK */
		/* Go slave end */
		data->slave_xfer.slave_addressed = false;
		slave_callbacks->stop(slave_config);
		I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		break;
	case S_RECE_ADDR_ACK:  /* Slave Receive Address ACK */
	case S_RECE_ARB_LOST:  /* Slave Receive Arbitration Lost */
		data->slave_xfer.slave_addressed = true;
		if (slave_callbacks->write_requested(slave_config) == 0) {
			/* Write ready, ACK next byte */
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* Write not ready, NACK next byte */
			I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
		}
		break;
	case S_REPEAT_START_STOP:  /* Slave Transmit/Receive Repeat Start or Stop */
		/* Go slave end */
		data->slave_xfer.slave_addressed = false;
		slave_callbacks->stop(slave_config);
		I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		break;
#endif  /* CONFIG_I2C_TARGET */

	case BUS_RELEASED:  /* Bus Released */
		/* Ignore the interrupt raised by BUS_RELEASED. */
		break;
	default:
		__ASSERT(false, "Uncaught I2C FSM state");
		m_numaker_i2c_master_xfer_end(dev, true);
	}
}

static int i2c_numaker_init(const struct device *dev)
{
	const struct i2c_numaker_config *config = dev->config;
	struct i2c_numaker_data *data = dev->data;
	int err = 0;
	struct numaker_scc_subsys scc_subsys;

	/* Validate this module's reset object */
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	/* Clean mutable context */
	memset(data, 0x00, sizeof(*data));

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->master_xfer.xfer_sync, 0, 1);

	SYS_UnlockReg();

	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = config->clk_modidx;
	scc_subsys.pcc.clk_src = config->clk_src;
	scc_subsys.pcc.clk_div = config->clk_div;

	/* Equivalent to CLK_EnableModuleClock() */
	err = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t) &scc_subsys);
	if (err != 0) {
		goto cleanup;
	}
	/* Equivalent to CLK_SetModuleClock() */
	err = clock_control_configure(config->clkctrl_dev,
				      (clock_control_subsys_t) &scc_subsys,
				      NULL);
	if (err != 0) {
		goto cleanup;
	}

	/* Configure pinmux (NuMaker's SYS MFP) */
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		goto cleanup;
	}

	/* Reset I2C to default state, same as BSP's SYS_ResetModule(id_rst) */
	reset_line_toggle_dt(&config->reset);

	err = i2c_numaker_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (err != 0) {
		goto cleanup;
	}

	config->irq_config_func(dev);

cleanup:

	SYS_LockReg();
	return err;
}

static DEVICE_API(i2c, i2c_numaker_driver_api) = {
	.configure = i2c_numaker_configure,
	.get_config = i2c_numaker_get_config,
	.transfer = i2c_numaker_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_numaker_slave_register,
	.target_unregister = i2c_numaker_slave_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
	.recover_bus = i2c_numaker_recover_bus,
};

#define I2C_NUMAKER_INIT(inst)                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                              \
                                                                                   \
	static void i2c_numaker_irq_config_func_##inst(const struct device *dev)   \
	{                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst),                                    \
			    DT_INST_IRQ(inst, priority),                           \
			    i2c_numaker_isr,                                       \
			    DEVICE_DT_INST_GET(inst),                              \
			    0);                                                    \
                                                                                   \
		irq_enable(DT_INST_IRQN(inst));                                    \
	}                                                                          \
                                                                                   \
	static const struct i2c_numaker_config i2c_numaker_config_##inst = {       \
		.i2c_base = (I2C_T *) DT_INST_REG_ADDR(inst),                      \
		.reset = RESET_DT_SPEC_INST_GET(inst),                             \
		.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),       \
		.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),                \
		.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),               \
		.clkctrl_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),\
		.irq_n = DT_INST_IRQN(inst),                                       \
		.irq_config_func = i2c_numaker_irq_config_func_##inst,             \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                    \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                    \
	};                                                                         \
                                                                                   \
	static struct i2c_numaker_data i2c_numaker_data_##inst;                    \
                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst,                                            \
				  i2c_numaker_init,                                \
				  NULL,                                            \
				  &i2c_numaker_data_##inst,                        \
				  &i2c_numaker_config_##inst,                      \
				  POST_KERNEL,                                     \
				  CONFIG_I2C_INIT_PRIORITY,                        \
				  &i2c_numaker_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_NUMAKER_INIT);
