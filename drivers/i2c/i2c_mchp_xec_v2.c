/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Microchip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_i2c_v2

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_mchp, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#define SPEED_100KHZ_BUS	0
#define SPEED_400KHZ_BUS	1
#define SPEED_1MHZ_BUS		2

#define EC_OWN_I2C_ADDR		0x7F
#define RESET_WAIT_US		20

/* I2C timeout is  10 ms (WAIT_INTERVAL * WAIT_COUNT) */
#define WAIT_INTERVAL		50
#define WAIT_COUNT		200
#define STOP_WAIT_COUNT		500
#define PIN_CFG_WAIT		50

/* I2C Read/Write bit pos */
#define I2C_READ_WRITE_POS	0

/* I2C recover SCL low retries */
#define I2C_RECOVER_SCL_LOW_RETRIES	10
/* I2C recover SDA low retries */
#define I2C_RECOVER_SDA_LOW_RETRIES	3
/* I2C recovery bit bang delay */
#define I2C_RECOVER_BB_DELAY_US		5
/* I2C recovery SCL sample delay */
#define I2C_RECOVER_SCL_DELAY_US	50

/* I2C SCL and SDA lines(signals) */
#define I2C_LINES_SCL_HI	BIT(SOC_I2C_SCL_POS)
#define I2C_LINES_SDA_HI	BIT(SOC_I2C_SDA_POS)
#define I2C_LINES_BOTH_HI	(I2C_LINES_SCL_HI | I2C_LINES_SDA_HI)

#define I2C_START		0U
#define I2C_RPT_START		1U

#define I2C_ENI_DIS		0U
#define I2C_ENI_EN		1U

#define I2C_WAIT_PIN_DEASSERT	0U
#define I2C_WAIT_PIN_ASSERT	1U

#define I2C_XEC_CTRL_WR_DLY	8

#define I2C_XEC_STATE_STOPPED	1U
#define I2C_XEC_STATE_OPEN	2U

#define I2C_XEC_OK		0
#define I2C_XEC_ERR_LAB		1
#define I2C_XEC_ERR_BUS		2
#define I2C_XEC_ERR_TMOUT	3

#define XEC_GPIO_CTRL_BASE	DT_REG_ADDR(DT_NODELABEL(gpio_000_036))

struct xec_speed_cfg {
	uint32_t bus_clk;
	uint32_t data_timing;
	uint32_t start_hold_time;
	uint32_t idle_scale;
	uint32_t timeout_scale;
};

struct i2c_xec_config {
	uint32_t port_sel;
	uint32_t base_addr;
	uint32_t clock_freq;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t pcr_idx;
	uint8_t pcr_bitpos;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

struct i2c_xec_data {
	uint8_t state;
	uint8_t read_discard;
	uint8_t speed_id;
	struct i2c_target_config *target_cfg;
	bool target_attached;
	bool target_read;
	uint32_t i2c_compl;
	uint8_t i2c_ctrl;
	uint8_t i2c_addr;
	uint8_t i2c_status;
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
		.idle_scale         = 0x01FC01ED,
		.timeout_scale      = 0x4B9CC2C7,
	},
	[SPEED_400KHZ_BUS] = {
		.bus_clk            = 0x00000F17,
		.data_timing        = 0x040A0A06,
		.start_hold_time    = 0x0000000A,
		.idle_scale         = 0x01000050,
		.timeout_scale      = 0x159CC2C7,
	},
	[SPEED_1MHZ_BUS] = {
		.bus_clk            = 0x00000509,
		.data_timing        = 0x04060601,
		.start_hold_time    = 0x00000006,
		.idle_scale         = 0x10000050,
		.timeout_scale      = 0x089CC2C7,
	},
};

static void i2c_ctl_wr(const struct device *dev, uint8_t ctrl)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;

	data->i2c_ctrl = ctrl;
	regs->CTRLSTS = ctrl;
	for (int i = 0; i < I2C_XEC_CTRL_WR_DLY; i++) {
		regs->BLKID = ctrl;
	}
}

static int i2c_xec_reset_config(const struct device *dev);

static int wait_bus_free(const struct device *dev, uint32_t nwait)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	uint32_t count = nwait;
	uint8_t sts = 0;

	while (count--) {
		sts = regs->CTRLSTS;
		data->i2c_status = sts;
		if (sts & MCHP_I2C_SMB_STS_NBB) {
			break; /* bus is free */
		}
		k_busy_wait(WAIT_INTERVAL);
	}

	/* NBB -> 1 not busy can occur for STOP, BER, or LAB */
	if (sts == (MCHP_I2C_SMB_STS_PIN | MCHP_I2C_SMB_STS_NBB)) {
		/* No service requested(PIN=1), NotBusy(NBB=1), and no errors */
		return 0;
	}

	if (sts & MCHP_I2C_SMB_STS_BER) {
		return I2C_XEC_ERR_BUS;
	}

	if (sts & MCHP_I2C_SMB_STS_LAB) {
		return I2C_XEC_ERR_LAB;
	}

	return I2C_XEC_ERR_TMOUT;
}

/*
 * returns state of I2C SCL and SDA lines.
 * b[0] = SCL, b[1] = SDA
 * Call soc specific routine to read GPIO pad input.
 * Why? We can get the pins from our PINCTRL info but
 * we do not know which pin is I2C clock and which pin
 * is I2C data. There's no ordering in PINCTRL DT unless
 * we impose an order.
 */
static uint32_t get_lines(const struct device *dev)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	uint8_t port = regs->CFG & MCHP_I2C_SMB_CFG_PORT_SEL_MASK;
	uint32_t lines = 0u;

	soc_i2c_port_lines_get(port, &lines);

	return lines;
}

static int i2c_xec_reset_config(const struct device *dev)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;

	data->state = I2C_XEC_STATE_STOPPED;
	data->read_discard = 0;

	/* Assert RESET and clr others */
	regs->CFG = MCHP_I2C_SMB_CFG_RESET;
	k_busy_wait(RESET_WAIT_US);
	/* clear reset, set filter enable, select port */
	regs->CFG = 0;
	regs->CFG = MCHP_I2C_SMB_CFG_FLUSH_SXBUF_WO |
		    MCHP_I2C_SMB_CFG_FLUSH_SRBUF_WO |
		    MCHP_I2C_SMB_CFG_FLUSH_MXBUF_WO |
		    MCHP_I2C_SMB_CFG_FLUSH_MRBUF_WO;

	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);

	/* PIN=1 to clear all status except NBB and synchronize */
	i2c_ctl_wr(dev, MCHP_I2C_SMB_CTRL_PIN);

	/*
	 * Controller implements two peripheral addresses for itself.
	 * It always monitors whether an external controller issues START
	 * plus target address. We should write valid peripheral addresses
	 * that do not match any peripheral on the bus.
	 * An alternative is to use the default 0 value which is the
	 * general call address and disable the general call match
	 * enable in the configuration register.
	 */
	regs->OWN_ADDR = EC_OWN_I2C_ADDR | (EC_OWN_I2C_ADDR << 8);
#ifdef CONFIG_I2C_TARGET
	if (data->target_cfg) {
		regs->OWN_ADDR = data->target_cfg->address;
	}
#endif
	/* Port number and filter enable MUST be written before enabling */
	regs->CFG |= BIT(14); /* disable general call */
	regs->CFG |= MCHP_I2C_SMB_CFG_FEN;
	regs->CFG |= (cfg->port_sel & MCHP_I2C_SMB_CFG_PORT_SEL_MASK);

	/*
	 * Before enabling the controller program the desired bus clock,
	 * repeated start hold time, data timing, and timeout scaling
	 * registers.
	 */
	regs->BUSCLK = xec_cfg_params[data->speed_id].bus_clk;
	regs->RSHTM = xec_cfg_params[data->speed_id].start_hold_time;
	regs->DATATM = xec_cfg_params[data->speed_id].data_timing;
	regs->TMOUTSC = xec_cfg_params[data->speed_id].timeout_scale;
	regs->IDLSC = xec_cfg_params[data->speed_id].idle_scale;

	/*
	 * PIN=1 clears all status except NBB
	 * ESO=1 enables output drivers
	 * ACK=1 enable ACK generation when data/address is clocked in.
	 */
	i2c_ctl_wr(dev, MCHP_I2C_SMB_CTRL_PIN |
			MCHP_I2C_SMB_CTRL_ESO |
			MCHP_I2C_SMB_CTRL_ACK);

	/* Enable controller */
	regs->CFG |= MCHP_I2C_SMB_CFG_ENAB;
	k_busy_wait(RESET_WAIT_US);

	/* wait for NBB=1, BER, LAB, or timeout */
	int rc = wait_bus_free(dev, WAIT_COUNT);

	return rc;
}

/*
 * If SCL is low sample I2C_RECOVER_SCL_LOW_RETRIES times with a 5 us delay
 * between samples. If SCL remains low then return -EBUSY
 * If SCL is High and SDA is low then loop up to I2C_RECOVER_SDA_LOW_RETRIES
 * times driving the pins:
 * Drive SCL high
 * delay I2C_RECOVER_BB_DELAY_US
 * Generate 9 clock pulses on SCL checking SDA before each falling edge of SCL
 *   If SDA goes high exit clock loop else to all 9 clocks
 * Drive SDA low, delay 5 us, release SDA, delay 5 us
 * Both lines are high then exit SDA recovery loop
 * Both lines should not be driven
 * Check both lines: if any bad return error else return success
 * NOTE 1: Bit-bang mode uses a HW MUX to switch the lines away from the I2C
 * controller logic to BB logic.
 * NOTE 2: Bit-bang mode requires HW timeouts to be disabled.
 * NOTE 3: Bit-bang mode requires the controller's configuration enable bit
 * to be set.
 * NOTE 4: The controller must be reset after using bit-bang mode.
 */
static int i2c_xec_recover_bus(const struct device *dev)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	int i, j, ret;

	LOG_ERR("I2C attempt bus recovery\n");

	/* reset controller to a known state */
	regs->CFG = MCHP_I2C_SMB_CFG_RESET;
	k_busy_wait(RESET_WAIT_US);
	regs->CFG = BIT(14) | MCHP_I2C_SMB_CFG_FEN |
		    (cfg->port_sel & MCHP_I2C_SMB_CFG_PORT_SEL_MASK);
	regs->CFG |= MCHP_I2C_SMB_CFG_FLUSH_SXBUF_WO |
		     MCHP_I2C_SMB_CFG_FLUSH_SRBUF_WO |
		     MCHP_I2C_SMB_CFG_FLUSH_MXBUF_WO |
		     MCHP_I2C_SMB_CFG_FLUSH_MRBUF_WO;
	regs->CTRLSTS = MCHP_I2C_SMB_CTRL_PIN;
	regs->BBCTRL = MCHP_I2C_SMB_BB_EN | MCHP_I2C_SMB_BB_CL |
		       MCHP_I2C_SMB_BB_DAT;
	regs->CFG |= MCHP_I2C_SMB_CFG_ENAB;

	if (!(regs->BBCTRL & MCHP_I2C_SMB_BB_CLKI_RO)) {
		for (i = 0;; i++) {
			if (i >= I2C_RECOVER_SCL_LOW_RETRIES) {
				ret = -EBUSY;
				goto recov_exit;
			}
			k_busy_wait(I2C_RECOVER_SCL_DELAY_US);
			if (regs->BBCTRL & MCHP_I2C_SMB_BB_CLKI_RO) {
				break; /* SCL went High */
			}
		}
	}

	if (regs->BBCTRL & MCHP_I2C_SMB_BB_DATI_RO) {
		ret = 0;
		goto recov_exit;
	}

	ret = -EBUSY;
	/* SDA recovery */
	for (i = 0; i < I2C_RECOVER_SDA_LOW_RETRIES; i++) {
		/* SCL output mode and tri-stated */
		regs->BBCTRL = MCHP_I2C_SMB_BB_EN |
			       MCHP_I2C_SMB_BB_SCL_DIR_OUT |
			       MCHP_I2C_SMB_BB_CL |
			       MCHP_I2C_SMB_BB_DAT;
		k_busy_wait(I2C_RECOVER_BB_DELAY_US);

		for (j = 0; j < 9; j++) {
			if (regs->BBCTRL & MCHP_I2C_SMB_BB_DATI_RO) {
				break;
			}
			/* drive SCL low */
			regs->BBCTRL = MCHP_I2C_SMB_BB_EN |
				       MCHP_I2C_SMB_BB_SCL_DIR_OUT |
				       MCHP_I2C_SMB_BB_DAT;
			k_busy_wait(I2C_RECOVER_BB_DELAY_US);
			/* release SCL: pulled high by external pull-up */
			regs->BBCTRL = MCHP_I2C_SMB_BB_EN |
				       MCHP_I2C_SMB_BB_SCL_DIR_OUT |
				       MCHP_I2C_SMB_BB_CL |
				       MCHP_I2C_SMB_BB_DAT;
			k_busy_wait(I2C_RECOVER_BB_DELAY_US);
		}

		/* SCL is High. Produce rising edge on SCL for STOP */
		regs->BBCTRL = MCHP_I2C_SMB_BB_EN | MCHP_I2C_SMB_BB_CL |
				MCHP_I2C_SMB_BB_SDA_DIR_OUT; /* drive low */
		k_busy_wait(I2C_RECOVER_BB_DELAY_US);
		regs->BBCTRL = MCHP_I2C_SMB_BB_EN | MCHP_I2C_SMB_BB_CL |
				MCHP_I2C_SMB_BB_DAT; /* release SCL */
		k_busy_wait(I2C_RECOVER_BB_DELAY_US);

		/* check if SCL and SDA are both high */
		uint8_t bb = regs->BBCTRL &
			(MCHP_I2C_SMB_BB_CLKI_RO | MCHP_I2C_SMB_BB_DATI_RO);

		if (bb == (MCHP_I2C_SMB_BB_CLKI_RO | MCHP_I2C_SMB_BB_DATI_RO)) {
			ret = 0; /* successful recovery */
			goto recov_exit;
		}
	}

recov_exit:
	/* BB mode disable reconnects SCL and SDA to I2C logic. */
	regs->BBCTRL = 0;
	regs->CTRLSTS = MCHP_I2C_SMB_CTRL_PIN; /* clear status */
	i2c_xec_reset_config(dev); /* reset controller */

	return ret;
}

#ifdef CONFIG_I2C_TARGET
/*
 * Restart I2C controller as target for ACK of address match.
 * Setting PIN clears all status in I2C.Status register except NBB.
 */
static void restart_target(const struct device *dev)
{
	i2c_ctl_wr(dev, MCHP_I2C_SMB_CTRL_PIN | MCHP_I2C_SMB_CTRL_ESO |
			MCHP_I2C_SMB_CTRL_ACK | MCHP_I2C_SMB_CTRL_ENI);
}

/*
 * Configure I2C controller acting as target to NACK the next received byte.
 * NOTE: Firmware must re-enable ACK generation before the start of the next
 * transaction otherwise the controller will NACK its target addresses.
 */
static void target_config_for_nack(const struct device *dev)
{
	i2c_ctl_wr(dev, MCHP_I2C_SMB_CTRL_PIN | MCHP_I2C_SMB_CTRL_ESO |
			MCHP_I2C_SMB_CTRL_ENI);
}
#endif

static int wait_pin(const struct device *dev, bool pin_assert, uint32_t nwait)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;

	for (;;) {
		k_busy_wait(WAIT_INTERVAL);

		data->i2c_compl = regs->COMPL;
		data->i2c_status = regs->CTRLSTS;

		if (data->i2c_status & MCHP_I2C_SMB_STS_BER) {
			return I2C_XEC_ERR_BUS;
		}

		if (data->i2c_status & MCHP_I2C_SMB_STS_LAB) {
			return I2C_XEC_ERR_LAB;
		}

		if (!(data->i2c_status & MCHP_I2C_SMB_STS_PIN)) {
			if (pin_assert) {
				return 0;
			}
		} else if (!pin_assert) {
			return 0;
		}

		if (nwait) {
			--nwait;
		} else {
			break;
		}
	}

	return I2C_XEC_ERR_TMOUT;
}

static int gen_start(const struct device *dev, uint8_t addr8,
		     bool is_repeated)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	uint8_t ctrl = MCHP_I2C_SMB_CTRL_ESO | MCHP_I2C_SMB_CTRL_STA |
		       MCHP_I2C_SMB_CTRL_ACK;

	data->i2c_addr = addr8;

	if (is_repeated) {
		i2c_ctl_wr(dev, ctrl);
		regs->I2CDATA = addr8;
	} else {
		ctrl |= MCHP_I2C_SMB_CTRL_PIN;
		regs->I2CDATA = addr8;
		i2c_ctl_wr(dev, ctrl);
	}

	return 0;
}

static int gen_stop(const struct device *dev)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	uint8_t ctrl = MCHP_I2C_SMB_CTRL_PIN | MCHP_I2C_SMB_CTRL_ESO |
		       MCHP_I2C_SMB_CTRL_STO | MCHP_I2C_SMB_CTRL_ACK;

	data->i2c_ctrl = ctrl;
	regs->CTRLSTS = ctrl;

	return 0;
}

static int do_stop(const struct device *dev, uint32_t nwait)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	int ret;

	data->state = I2C_XEC_STATE_STOPPED;
	data->read_discard = 0;

	gen_stop(dev);
	ret = wait_bus_free(dev, nwait);
	if (ret) {
		uint32_t lines = get_lines(dev);

		if (lines != I2C_LINES_BOTH_HI) {
			i2c_xec_recover_bus(dev);
		} else {
			ret = i2c_xec_reset_config(dev);
		}
	}

	if (ret == 0) {
		/* stop success: prepare for next transaction */
		regs->CTRLSTS = MCHP_I2C_SMB_CTRL_PIN | MCHP_I2C_SMB_CTRL_ESO |
				MCHP_I2C_SMB_CTRL_ACK;
	}

	return ret;
}

static int do_start(const struct device *dev, uint8_t addr8, bool is_repeated)
{
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	int ret;

	gen_start(dev, addr8, is_repeated);
	ret = wait_pin(dev, I2C_WAIT_PIN_ASSERT, WAIT_COUNT);
	if (ret) {
		i2c_xec_reset_config(dev);
		return ret;
	}

	/* PIN 1->0: check for NACK */
	if (data->i2c_status & MCHP_I2C_SMB_STS_LRB_AD0) {
		gen_stop(dev);
		ret = wait_bus_free(dev, WAIT_COUNT);
		if (ret) {
			i2c_xec_reset_config(dev);
		}
		return -EIO;
	}

	return 0;
}

static int i2c_xec_configure(const struct device *dev,
			     uint32_t dev_config_raw)
{
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	if (!(dev_config_raw & I2C_MODE_CONTROLLER)) {
		return -ENOTSUP;
	}

	if (dev_config_raw & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		data->speed_id = SPEED_100KHZ_BUS;
		break;
	case I2C_SPEED_FAST:
		data->speed_id = SPEED_400KHZ_BUS;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->speed_id = SPEED_1MHZ_BUS;
		break;
	default:
		return -EINVAL;
	}

	int ret = i2c_xec_reset_config(dev);

	return ret;
}

/* I2C Controller transmit: polling implementation */
static int ctrl_tx(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	int ret = 0;
	uint8_t mflags = msg->flags;
	uint8_t addr8 = (uint8_t)((addr & 0x7FU) << 1);

	if (data->state == I2C_XEC_STATE_STOPPED) {
		data->i2c_addr = addr8;
		/* Is bus free and controller ready? */
		ret = wait_bus_free(dev, WAIT_COUNT);
		if (ret) {
			ret = i2c_xec_recover_bus(dev);
			if (ret) {
				return ret;
			}
		}

		ret = do_start(dev, addr8, I2C_START);
		if (ret) {
			return ret;
		}

		data->state = I2C_XEC_STATE_OPEN;

	} else if (mflags & I2C_MSG_RESTART) {
		data->i2c_addr = addr8;
		ret = do_start(dev, addr8, I2C_RPT_START);
		if (ret) {
			return ret;
		}
	}

	for (size_t n = 0; n < msg->len; n++) {
		regs->I2CDATA = msg->buf[n];
		ret = wait_pin(dev, I2C_WAIT_PIN_ASSERT, WAIT_COUNT);
		if (ret) {
			i2c_xec_reset_config(dev);
			return ret;
		}
		if (data->i2c_status & MCHP_I2C_SMB_STS_LRB_AD0) { /* NACK? */
			do_stop(dev, STOP_WAIT_COUNT);
			return -EIO;
		}
	}

	if (mflags & I2C_MSG_STOP) {
		ret = do_stop(dev, STOP_WAIT_COUNT);
	}

	return ret;
}

/*
 * I2C Controller receive: polling implementation
 * Transmitting a target address with BIT[0] == 1 causes the controller
 * to enter controller-read mode where every read of I2CDATA generates
 * clocks for the next byte. When we generate START or Repeated-START
 * and transmit an address the address is also clocked in during
 * address transmission. The address must read and discarded.
 * Read of I2CDATA returns data currently in I2C read buffer, sets
 * I2CSTATUS.PIN = 1, and !!generates clocks for the next
 * byte!!
 * For this controller to NACK the last byte we must clear the
 * I2C CTRL register ACK bit BEFORE reading the next to last
 * byte. Before reading the last byte we configure I2C CTRL to generate a STOP
 * and then read the last byte from I2 DATA.
 * When controller is in STOP mode it will not generate clocks when I2CDATA is
 * read. UGLY HW DESIGN.
 * We will NOT attempt to follow this HW design for Controller read except
 * when all information is available: STOP message flag set AND number of
 * bytes to read including dummy is >= 2. General usage can result in the
 * controller not NACK'ing the last byte.
 */
static int ctrl_rx(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	int ret = 0;
	size_t data_len = msg->len;
	uint8_t mflags = msg->flags;
	uint8_t addr8 = (uint8_t)(((addr & 0x7FU) << 1) | BIT(0));
	uint8_t temp = 0;

	if (data->state == I2C_XEC_STATE_STOPPED) {
		data->i2c_addr = addr8;
		/* Is bus free and controller ready? */
		ret = wait_bus_free(dev, WAIT_COUNT);
		if (ret) {
			i2c_xec_reset_config(dev);
			return ret;
		}

		ret = do_start(dev, addr8, I2C_START);
		if (ret) {
			return ret;
		}

		data->state = I2C_XEC_STATE_OPEN;

		/* controller clocked address into I2CDATA */
		data->read_discard = 1U;

	} else if (mflags & I2C_MSG_RESTART) {
		data->i2c_addr = addr8;
		ret = do_start(dev, addr8, I2C_RPT_START);
		if (ret) {
			return ret;
		}

		/* controller clocked address into I2CDATA */
		data->read_discard = 1U;
	}

	if (!data_len) { /* requested message length is 0 */
		ret = 0;
		if (mflags & I2C_MSG_STOP) {
			data->state = I2C_XEC_STATE_STOPPED;
			data->read_discard = 0;
			ret = do_stop(dev, STOP_WAIT_COUNT);
		}
		return ret;
	}

	if (data->read_discard) {
		data_len++;
	}

	uint8_t *p8 = &msg->buf[0];

	while (data_len) {
		if (mflags & I2C_MSG_STOP) {
			if (data_len == 2) {
				i2c_ctl_wr(dev, MCHP_I2C_SMB_CTRL_ESO);
			} else if (data_len == 1) {
				break;
			}
		}
		temp = regs->I2CDATA; /* generates clocks */
		if (data->read_discard) {
			data->read_discard = 0;
		} else {
			*p8++ = temp;
		}
		ret = wait_pin(dev, I2C_WAIT_PIN_ASSERT, WAIT_COUNT);
		if (ret) {
			i2c_xec_reset_config(dev);
			return ret;
		}
		data_len--;
	}

	if (mflags & I2C_MSG_STOP) {
		data->state = I2C_XEC_STATE_STOPPED;
		data->read_discard = 0;
		ret = do_stop(dev, STOP_WAIT_COUNT);
		if (ret == 0) {
			*p8 = regs->I2CDATA;
		}
	}

	return ret;
}

static int i2c_xec_transfer(const struct device *dev, struct i2c_msg *msgs,
			    uint8_t num_msgs, uint16_t addr)
{
	struct i2c_xec_data *data = dev->data;
	int ret = 0;

#ifdef CONFIG_I2C_TARGET
	if (data->target_attached) {
		LOG_ERR("Device is registered as target");
		return -EBUSY;
	}
#endif

	for (uint8_t i = 0; i < num_msgs; i++) {
		struct i2c_msg *m = &msgs[i];

		if ((m->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = ctrl_tx(dev, m, addr);
		} else {
			ret = ctrl_rx(dev, m, addr);
		}
		if (ret) {
			data->state = I2C_XEC_STATE_STOPPED;
			data->read_discard = 0;
			LOG_ERR("i2x_xfr: flags: %x error: %d", m->flags, ret);
			break;
		}
	}

	return ret;
}

static void i2c_xec_bus_isr(const struct device *dev)
{
#ifdef CONFIG_I2C_TARGET
	const struct i2c_xec_config *cfg =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data = dev->data;
	const struct i2c_target_callbacks *target_cb =
		data->target_cfg->callbacks;
	struct i2c_smb_regs *regs = (struct i2c_smb_regs *)cfg->base_addr;
	int ret;
	uint32_t status;
	uint32_t compl_status;
	uint8_t val;
	uint8_t dummy = 0U;

	/* Get current status */
	status = regs->CTRLSTS;
	compl_status = regs->COMPL & MCHP_I2C_SMB_CMPL_RW1C_MASK;

	/* Idle interrupt enabled and active? */
	if ((regs->CFG & MCHP_I2C_SMB_CFG_ENIDI) &&
	    (compl_status & MCHP_I2C_SMB_CMPL_IDLE_RWC)) {
		regs->CFG &= ~MCHP_I2C_SMB_CFG_ENIDI;
		if (status & MCHP_I2C_SMB_STS_NBB) {
			restart_target(dev);
			goto clear_iag;
		}
	}

	if (!data->target_attached) {
		goto clear_iag;
	}

	/* Bus Error */
	if (status & MCHP_I2C_SMB_STS_BER) {
		if (target_cb->stop) {
			target_cb->stop(data->target_cfg);
		}
		restart_target(dev);
		goto clear_iag;
	}

	/* External stop */
	if (status & MCHP_I2C_SMB_STS_EXT_STOP) {
		if (target_cb->stop) {
			target_cb->stop(data->target_cfg);
		}
		restart_target(dev);
		goto clear_iag;
	}

	/* Address byte handling */
	if (status & MCHP_I2C_SMB_STS_AAS) {
		if (status & MCHP_I2C_SMB_STS_PIN) {
			goto clear_iag;
		}

		uint8_t rx_data = regs->I2CDATA;

		if (rx_data & BIT(I2C_READ_WRITE_POS)) {
			/* target transmitter mode */
			data->target_read = true;
			val = dummy;
			if (target_cb->read_requested) {
				target_cb->read_requested(
					data->target_cfg, &val);

				/* Application target transmit handler
				 * does not have data to send. In
				 * target transmit mode the external
				 * Controller is ACK's data we send.
				 * All we can do is keep sending dummy
				 * data. We assume read_requested does
				 * not modify the value pointed to by val
				 * if it has not data(returns error).
				 */
			}
			/*
			 * Writing I2CData causes this HW to release SCL
			 * ending clock stretching. The external Controller
			 * senses SCL released and begins generating clocks
			 * and capturing data driven by this controller
			 * on SDA. External Controller ACK's data until it
			 * wants no more then it will NACK.
			 */
			regs->I2CDATA = val;
			goto clear_iag; /* Exit ISR */
		} else {
			/* target receiver mode */
			data->target_read = false;
			if (target_cb->write_requested) {
				ret = target_cb->write_requested(
							data->target_cfg);
				if (ret) {
					/*
					 * Application handler can't accept
					 * data. Configure HW to NACK next
					 * data transmitted by external
					 * Controller.
					 * !!! TODO We must re-program our HW
					 * for address ACK before next
					 * transaction is begun !!!
					 */
					target_config_for_nack(dev);
				}
			}
			goto clear_iag; /* Exit ISR */
		}
	}

	if (data->target_read) { /* Target transmitter mode */

		/* Master has Nacked, then just write a dummy byte */
		status = regs->CTRLSTS;
		if (status & MCHP_I2C_SMB_STS_LRB_AD0) {

			/*
			 * ISSUE: HW will not detect external STOP in
			 * target transmit mode. Enable IDLE interrupt
			 * to catch PIN 0 -> 1 and NBB 0 -> 1.
			 */
			regs->CFG |= MCHP_I2C_SMB_CFG_ENIDI;

			/*
			 * dummy write causes this controller's PIN status
			 * to de-assert 0 -> 1. Data is not transmitted.
			 * SCL is not driven low by this controller.
			 */
			regs->I2CDATA = dummy;

			status = regs->CTRLSTS;

		} else {
			val = dummy;
			if (target_cb->read_processed) {
				target_cb->read_processed(
					data->target_cfg, &val);
			}
			regs->I2CDATA = val;
		}
	} else { /* target receiver mode */
		/*
		 * Reading the I2CData register causes this target to release
		 * SCL. The external Controller senses SCL released generates
		 * clocks for transmitting the next data byte.
		 * Reading I2C Data register causes PIN status 0 -> 1.
		 */
		val = regs->I2CDATA;
		if (target_cb->write_received) {
			/*
			 * Call back returns error if we should NACK
			 * next byte.
			 */
			ret = target_cb->write_received(data->target_cfg, val);
			if (ret) {
				/*
				 * Configure HW to NACK next byte. It will not
				 * generate clocks for another byte of data
				 */
				target_config_for_nack(dev);
			}
		}
	}

clear_iag:
	regs->COMPL = compl_status;
	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);
#endif
}

#ifdef CONFIG_I2C_TARGET
static int i2c_xec_target_register(const struct device *dev,
				   struct i2c_target_config *config)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data = dev->data;
	int ret;

	if (!config) {
		return -EINVAL;
	}

	if (data->target_attached) {
		return -EBUSY;
	}

	/* Wait for any outstanding transactions to complete so that
	 * the bus is free
	 */
	ret = wait_bus_free(dev, WAIT_COUNT);
	if (ret) {
		return ret;
	}

	data->target_cfg = config;

	ret = i2c_xec_reset_config(dev);
	if (ret) {
		return ret;
	}

	restart_target(dev);

	data->target_attached = true;

	/* Clear before enabling girq bit */
	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);
	mchp_xec_ecia_girq_src_en(cfg->girq, cfg->girq_pos);

	return 0;
}

static int i2c_xec_target_unregister(const struct device *dev,
				     struct i2c_target_config *config)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data = dev->data;

	if (!data->target_attached) {
		return -EINVAL;
	}

	data->target_cfg = NULL;
	data->target_attached = false;

	mchp_xec_ecia_girq_src_dis(cfg->girq, cfg->girq_pos);

	return 0;
}
#endif

static const struct i2c_driver_api i2c_xec_driver_api = {
	.configure = i2c_xec_configure,
	.transfer = i2c_xec_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_xec_target_register,
	.target_unregister = i2c_xec_target_unregister,
#endif
};

static int i2c_xec_init(const struct device *dev)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	int ret;
	uint32_t bitrate_cfg;

	data->state = I2C_XEC_STATE_STOPPED;
	data->target_cfg = NULL;
	data->target_attached = false;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC I2C pinctrl setup failed (%d)", ret);
		return ret;
	}

	bitrate_cfg = i2c_map_dt_bitrate(cfg->clock_freq);
	if (!bitrate_cfg) {
		return -EINVAL;
	}

	/* Default configuration */
	ret = i2c_xec_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_I2C_TARGET
	const struct i2c_xec_config *config =
	(const struct i2c_xec_config *const) (dev->config);

	config->irq_config_func();
#endif
	return 0;
}

#define I2C_XEC_DEVICE(n)						\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void i2c_xec_irq_config_func_##n(void);			\
									\
	static struct i2c_xec_data i2c_xec_data_##n;			\
	static const struct i2c_xec_config i2c_xec_config_##n = {	\
		.base_addr =						\
			DT_INST_REG_ADDR(n),				\
		.port_sel = DT_INST_PROP(n, port_sel),			\
		.clock_freq = DT_INST_PROP(n, clock_frequency),		\
		.girq = DT_INST_PROP_BY_IDX(n, girqs, 0),		\
		.girq_pos = DT_INST_PROP_BY_IDX(n, girqs, 1),		\
		.pcr_idx = DT_INST_PROP_BY_IDX(n, pcrs, 0),		\
		.pcr_bitpos = DT_INST_PROP_BY_IDX(n, pcrs, 1),		\
		.irq_config_func = i2c_xec_irq_config_func_##n,		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_xec_init, NULL,		\
		&i2c_xec_data_##n, &i2c_xec_config_##n,			\
		POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,			\
		&i2c_xec_driver_api);					\
									\
	static void i2c_xec_irq_config_func_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    i2c_xec_bus_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_XEC_DEVICE)
