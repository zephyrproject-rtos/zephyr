/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/clock_control.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <drivers/i2c.h>

#define SPEED_100KHZ_BUS    0
#define SPEED_400KHZ_BUS    1
#define SPEED_1MHZ_BUS      2

#define WAIT_INTERVAL       5
/* 250 us */
#define TIMEOUT       100
/* 25 us */
#define MAX_CLK_STRETCHING  5

struct xec_speed_cfg {
	u32_t bus_clk;
	u32_t data_timing;
	u32_t start_hold_time;
	u32_t config;
	u32_t timeout_scale;
};

struct i2c_xec_config {
	u32_t port_sel;
	u32_t base_addr;
};

struct i2c_xec_data {
	u32_t pending_stop;
};

/* Recommended programming values based on 16MHz
 * i2c_baud_clk_period/bus_clk_period - 2 = (low_period + hi_period)
 * bus_clk_reg (16MHz/100KHz -2) = 0x4F + 0x4F
 *             (16MHz/400KHz -2) = 0x0F + 0x17
 *             (16MHz/1MHz -2) = 0x05 + 0x09
 */
static const struct xec_speed_cfg xec_cfg_params[] = {
	[SPEED_100KHZ_BUS] = {
		.bus_clk            = 0x00004F4F,
		.data_timing        = 0x0C4D5006,
		.start_hold_time    = 0x0000004D,
		.config             = 0x01FC01ED,
		.timeout_scale      = 0x4B9CC2C7,
	},
	[SPEED_400KHZ_BUS] = {
		.bus_clk            = 0x00000F17,
		.data_timing        = 0x040A0A06,
		.start_hold_time    = 0x0000000A,
		.config             = 0x01000050,
		.timeout_scale      = 0x159CC2C7,
	},
	[SPEED_1MHZ_BUS] = {
		.bus_clk            = 0x00000509,
		.data_timing        = 0x04060601,
		.start_hold_time    = 0x00000006,
		.config             = 0x10000050,
		.timeout_scale      = 0x089CC2C7,
	},
};

static int xec_spin_yield(int *counter)
{
	*counter = *counter + 1;

	if (*counter > TIMEOUT) {
		return -ETIMEDOUT;
	}

	if (*counter > MAX_CLK_STRETCHING * 2) {
		k_yield();
	} else {
		k_busy_wait(5);
	}

	return 0;
}

static void recover_from_error(u32_t ba)
{
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				   MCHP_I2C_SMB_CTRL_ESO |
				   MCHP_I2C_SMB_CTRL_STO |
				   MCHP_I2C_SMB_CTRL_ACK;
}

static int wait_bus_free(u32_t ba)
{
	int ret;
	int counter = 0;

	while (!(MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_NBB)) {
		ret = xec_spin_yield(&counter);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int wait_completion(u32_t ba)
{
	int ret;
	int counter = 0;

	/* Wait for transaction to be completed */
	while (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_PIN) {
		ret = xec_spin_yield(&counter);

		if (ret < 0) {
			recover_from_error(ba);
			return ret;
		}
	}

	/* Check if Slave send ACK/NACK */
	if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_LRB_AD0) {
		recover_from_error(ba);
		return -EIO;
	}

	/* Check for bus error */
	if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_BER) {
		recover_from_error(ba);
		return -EBUSY;
	}

	return 0;
}

static bool check_lines(u32_t ba)
{
	return ((!(MCHP_I2C_SMB_BB_CTRL(ba) & MCHP_I2C_SMB_BB_CLKI_RO)) ||
		(!(MCHP_I2C_SMB_BB_CTRL(ba) & MCHP_I2C_SMB_BB_DATI_RO)));
}

static int i2c_xec_configure(struct device *dev, u32_t dev_config_raw)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config->config_info);
	u32_t ba = config->base_addr;
	u8_t port_sel = config->port_sel;
	u32_t speed_id;
	u32_t cfg, bb_ctrl;
	u8_t ctrl;

	if (!(dev_config_raw & I2C_MODE_MASTER)) {
		return -ENOTSUP;
	}

	if (dev_config_raw & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		speed_id = SPEED_100KHZ_BUS;
		break;
	case I2C_SPEED_FAST:
		speed_id = SPEED_400KHZ_BUS;
		break;
	case I2C_SPEED_FAST_PLUS:
		speed_id = SPEED_1MHZ_BUS;
		break;
	default:
		return -EINVAL;
	}

	cfg = MCHP_I2C_SMB_CFG(ba) & MCHP_I2C_SMB_CTRL_MASK;
	bb_ctrl = MCHP_I2C_SMB_BB_CTRL(ba) & MCHP_I2C_SMB_BB_MASK;
	ctrl = MCHP_I2C_SMB_CTRL(ba) & MCHP_I2C_SMB_CTRL_MASK;

	/* Assert RESET and clr others */
	cfg |= MCHP_I2C_SMB_CFG_RESET;
	MCHP_I2C_SMB_CFG(ba) = cfg;

	/* Enable bit-bang mode, do not enable SMBus HW timeouts */
	bb_ctrl |= MCHP_I2C_SMB_BB_EN;
	bb_ctrl |= MCHP_I2C_SMB_BB_SCL_DIR_OUT;
	bb_ctrl |= MCHP_I2C_SMB_BB_SDA_DIR_OUT;
	bb_ctrl |= (MCHP_I2C_SMB_BB_CL | MCHP_I2C_SMB_BB_DAT);
	MCHP_I2C_SMB_BB_CTRL(ba) = bb_ctrl;

	/* Bus reset */
	cfg &= ~MCHP_I2C_SMB_CFG_RESET;
	MCHP_I2C_SMB_CFG(ba) = cfg;

	/* Assert PIN bit, ESO = 0 and disables Interrupts (ENI) */
	ctrl =  MCHP_I2C_SMB_CTRL_PIN;
	MCHP_I2C_SMB_CTRL_WO(ba) = ctrl;

	/* Port selection */
	cfg |= (port_sel & MCHP_I2C_SMB_CFG_PORT_SEL_MASK);
	MCHP_I2C_SMB_CFG(ba) = cfg;

	/* Enable controller and I2C filters*/
	cfg |= (MCHP_I2C_SMB_CFG_ENAB | MCHP_I2C_SMB_CFG_FEN);
	MCHP_I2C_SMB_CFG(ba) = cfg;

	/* Set own address */
	MCHP_I2C_SMB_OWN_ADDR(ba) = 0x7F;

	/* Configure speed */
	MCHP_I2C_SMB_BUS_CLK(ba) = xec_cfg_params[speed_id].bus_clk;
	MCHP_I2C_SMB_DATA_TM(ba) = xec_cfg_params[speed_id].data_timing;
	MCHP_I2C_SMB_RSHT(ba) = xec_cfg_params[speed_id].start_hold_time;
	MCHP_I2C_SMB_TMTSC(ba) = xec_cfg_params[speed_id].timeout_scale;

	ctrl |= (MCHP_I2C_SMB_CTRL_PIN | MCHP_I2C_SMB_CTRL_ESO |
		 MCHP_I2C_SMB_CTRL_ENI | MCHP_I2C_SMB_CTRL_ACK);
	MCHP_I2C_SMB_CTRL_WO(ba) = ctrl;

	return 0;
}

static int i2c_xec_poll_write(struct device *dev, struct i2c_msg msg,
			      u16_t addr)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config->config_info);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->driver_data);
	u32_t ba = config->base_addr;
	int ret;

	if (data->pending_stop == 0) {
		/* Check clock and data lines */
		if (check_lines(ba)) {
			return -EBUSY;
		}

		/* Wait until bus is free */
		ret = wait_bus_free(ba);
		if (ret) {
			return ret;
		}

		/* Send slave address */
		MCHP_I2C_SMB_DATA(ba) = (addr & ~BIT(0));

		/* Send start and ack bits */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				MCHP_I2C_SMB_CTRL_ESO | MCHP_I2C_SMB_CTRL_STA |
				MCHP_I2C_SMB_CTRL_ACK;

		ret = wait_completion(ba);
		if (ret) {
			return ret;
		}
	}

	/* Send bytes */
	for (int i = 0U; i < msg.len; i++) {
		MCHP_I2C_SMB_DATA(ba) = msg.buf[i];
		ret = wait_completion(ba);
		if (ret) {
			return ret;
		}

		/* Handle stop bit for last byte to write */
		if (i == (msg.len - 1)) {
			if (msg.flags & I2C_MSG_STOP) {
				/* Send stop and ack bits */
				MCHP_I2C_SMB_CTRL_WO(ba) =
						MCHP_I2C_SMB_CTRL_PIN |
						MCHP_I2C_SMB_CTRL_ESO |
						MCHP_I2C_SMB_CTRL_STO |
						MCHP_I2C_SMB_CTRL_ACK;
				data->pending_stop = 0;
			} else {
				data->pending_stop = 1;
			}
		}
	}

	return 0;
}

static int i2c_xec_poll_read(struct device *dev, struct i2c_msg msg,
			     u16_t addr)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config->config_info);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->driver_data);
	u32_t ba = config->base_addr;
	u8_t byte, ctrl;
	int ret;

	if (!(msg.flags & I2C_MSG_RESTART)) {
		/* Check clock and data lines */
		if (check_lines(ba)) {
			return -EBUSY;
		}

		/* Wait until bus is free */
		ret = wait_bus_free(ba);
		if (ret) {
			return ret;
		}
	}

	/* Send slave address */
	MCHP_I2C_SMB_DATA(ba) = (addr | BIT(0));
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO |
		MCHP_I2C_SMB_CTRL_STA | MCHP_I2C_SMB_CTRL_ACK;

	ret = wait_completion(ba);
	if (ret) {
		return ret;
	}

	if (msg.len == 1) {
		/* Send NACK for last transaction */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_STA;
	}

	/* Read dummy byte */
	byte = MCHP_I2C_SMB_DATA(ba);
	ret = wait_completion(ba);
	if (ret) {
		return ret;
	}

	for (int i = 0U; i < msg.len; i++) {
		while (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_PIN) {
			if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_BER) {
				return -EBUSY;
			}
		}

		if (i == (msg.len - 1)) {
			if (msg.flags & I2C_MSG_STOP) {
				/* Send stop and ack bits */
				ctrl = (MCHP_I2C_SMB_CTRL_PIN |
					MCHP_I2C_SMB_CTRL_ESO |
					MCHP_I2C_SMB_CTRL_STO |
					MCHP_I2C_SMB_CTRL_ACK);
				MCHP_I2C_SMB_CTRL_WO(ba) = ctrl;
				data->pending_stop = 0;
			}
		} else if (i == (msg.len - 2)) {
			/* Send NACK for last transaction */
			MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_STA;
		}
		msg.buf[i] = MCHP_I2C_SMB_DATA(ba);
	}

	return 0;
}

static int i2c_xec_transfer(struct device *dev, struct i2c_msg *msgs,
				u8_t num_msgs, u16_t addr)
{
	int ret = 0;

	addr <<= 1;
	for (int i = 0U; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_xec_poll_write(dev, msgs[i], addr);
			if (ret) {
				return ret;
			}
		} else {
			ret = i2c_xec_poll_read(dev, msgs[i], addr);
		}
	}

	return ret;
}

#if defined(CONFIG_I2C_SLAVE)
static int i2c_xec_slave_register(struct device *dev)
{
	return -ENOTSUP;
}

static int i2c_xec_slave_unregister(struct device *dev)
{
	return -ENOTSUP;
}
#endif

static int i2c_xec_init(struct device *dev);

static const struct i2c_driver_api i2c_xec_driver_api = {
	.configure = i2c_xec_configure,
	.transfer = i2c_xec_transfer,
#if defined(CONFIG_I2C_SLAVE)
	.slave_register = i2c_xec_slave_register,
	.slave_unregister = i2c_xec_slave_unregister,
#endif
};

#ifdef CONFIG_I2C_XEC_0
static struct i2c_xec_data i2c_xec_data_0;
static const struct i2c_xec_config i2c_xec_config_0 = {
	.base_addr = DT_INST_0_MICROCHIP_XEC_I2C_BASE_ADDRESS,
	.port_sel = DT_INST_0_MICROCHIP_XEC_I2C_PORT_SEL,
};
DEVICE_AND_API_INIT(i2c_xec_0, DT_INST_0_MICROCHIP_XEC_I2C_LABEL,
			&i2c_xec_init, &i2c_xec_data_0, &i2c_xec_config_0,
			POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
			&i2c_xec_driver_api);
#endif /* CONFIG_I2C_XEC_0 */

#ifdef CONFIG_I2C_XEC_1
static struct i2c_xec_data i2c_xec_data_1;
static const struct i2c_xec_config i2c_xec_config_1 = {
	.base_addr	= DT_INST_1_MICROCHIP_XEC_I2C_BASE_ADDRESS,
	.port_sel = DT_INST_1_MICROCHIP_XEC_I2C_PORT_SEL,
};
DEVICE_AND_API_INIT(i2c_xec_1, DT_INST_1_MICROCHIP_XEC_I2C_LABEL,
			&i2c_xec_init, &i2c_xec_data_1, &i2c_xec_config_1,
			POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
			&i2c_xec_driver_api);
#endif /* CONFIG_I2C_XEC_1 */

#ifdef CONFIG_I2C_XEC_2
static struct i2c_xec_data i2c_xec_data_2;
static const struct i2c_xec_config i2c_xec_config_2 = {
	.base_addr	= DT_INST_2_MICROCHIP_XEC_I2C_BASE_ADDRESS,
	.port_sel = DT_INST_2_MICROCHIP_XEC_I2C_PORT_SEL,
};
DEVICE_AND_API_INIT(i2c_xec_2, DT_INST_2_MICROCHIP_XEC_I2C_LABEL,
			&i2c_xec_init, &i2c_xec_data_2, &i2c_xec_config_2,
			POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
			&i2c_xec_driver_api);
#endif /* CONFIG_I2C_XEC_2 */

static int i2c_xec_init(struct device *dev)
{
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->driver_data);
	data->pending_stop = 0;
	return 0;
}
