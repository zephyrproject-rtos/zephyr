/*
 * Copyright (c) 2026 Microchip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_i2c_v3

#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/mchp_xec_i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(i2c_xec_v3, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"
#include "i2c_mchp_xec_regs.h"

/* Microchip MEC I2C controller using byte mode.
 * This driver is for HW version 3.8 and above.
 */

#define RESET_WAIT_US 20

/* I2C timeout is  10 ms (WAIT_INTERVAL * WAIT_COUNT) */
#define WAIT_INTERVAL   50
#define WAIT_COUNT      200
#define STOP_WAIT_COUNT 500
#define PIN_CFG_WAIT    50

/* I2C recover SCL low retries */
#define I2C_XEC_RECOVER_SCL_LOW_RETRIES 10
/* I2C recover SDA low retries */
#define I2C_XEC_RECOVER_SDA_LOW_RETRIES 3
/* I2C recovery bit bang delay */
#define I2C_XEC_RECOVER_BB_DELAY_US     5
/* I2C recovery SCL sample delay */
#define I2C_XEC_RECOVER_SCL_DELAY_US    50

#define I2C_XEC_CTRL_WR_DLY 8

/* get_lines bit positions */
#define XEC_I2C_SCL_LINE_POS 0
#define XEC_I2C_SDA_LINE_POS 1
#define XEC_I2C_LINES_MSK    (BIT(XEC_I2C_SCL_LINE_POS) | BIT(XEC_I2C_SDA_LINE_POS))

#define XEC_I2C_CR_PIN_ESO_ACK                                                                     \
	(BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ACK_POS))

#define XEC_I2C_CR_PIN_ESO_ENI_ACK (XEC_I2C_CR_PIN_ESO_ACK | BIT(XEC_I2C_CR_ENI_POS))

#define XEC_I2C_CR_START                                                                           \
	(BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_STA_POS) |             \
	 BIT(XEC_I2C_CR_ACK_POS))

#define XEC_I2C_CR_START_ENI (XEC_I2C_CR_START | BIT(XEC_I2C_CR_ENI_POS))

#define XEC_I2C_CR_RPT_START                                                                       \
	(BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_STA_POS) | BIT(XEC_I2C_CR_ACK_POS))

#define XEC_I2C_CR_RPT_START_ENI (XEC_I2C_CR_RPT_START | BIT(XEC_I2C_CR_ENI_POS))

#define XEC_I2C_CR_STOP                                                                            \
	(BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_STO_POS) |             \
	 BIT(XEC_I2C_CR_ACK_POS))

#define XEC_I2C_TM_HOST_READ_IGNORE_VAL 0xffu

#define XEC_I2C_TM_REGISTER_WAIT_MS 1000u

#define XEC_I2C_TM_SHAD_ADDR_ANOMALY
#define XEC_I2C_TM_SHAD_ADDR_ANOMALY_WAIT_US 96

enum xec_i2c_state {
	XEC_I2C_STATE_CLOSED = 0,
	XEC_I2C_STATE_OPEN,
};

enum xec_i2c_error {
	XEC_I2C_ERR_NONE = 0,
	XEC_I2C_ERR_BUS,
	XEC_I2C_ERR_LOST_ARB,
	XEC_I2C_ERR_TIMEOUT,
};

enum xec_i2c_direction {
	XEC_I2C_DIR_NONE = 0,
	XEC_I2C_DIR_WR,
	XEC_I2C_DIR_RD,
};

enum xec_i2c_start {
	XEC_I2C_START_NONE = 0,
	XEC_I2C_START_NORM,
	XEC_I2C_START_RPT,
};

enum i2c_xec_isr_state {
	I2C_XEC_ISR_STATE_GEN_START = 0,
	I2C_XEC_ISR_STATE_CHK_ACK,
	I2C_XEC_ISR_STATE_WR_DATA,
	I2C_XEC_ISR_STATE_RD_DATA,
	I2C_XEC_ISR_STATE_GEN_STOP,
	I2C_XEC_ISR_STATE_EV_IDLE,
	I2C_XEC_ISR_STATE_NEXT_MSG,
	I2C_XEC_ISR_STATE_EXIT_1,
#ifdef CONFIG_I2C_TARGET
	I2C_XEC_ISR_STATE_TM_HOST_RD,
	I2C_XEC_ISR_STATE_TM_HOST_WR,
	I2C_XEC_ISR_STATE_TM_EV_STOP,
#endif
	I2C_XEC_ISR_STATE_MAX
};

enum i2c_xec_std_freq {
	XEC_I2C_STD_FREQ_100K = 0,
	XEC_I2C_STD_FREQ_400K,
	XEC_I2C_STD_FREQ_1M,
	XEC_I2C_STD_FREQ_MAX,
};

struct xec_i2c_timing {
	uint32_t freq_hz;
	uint32_t data_tm;    /* data timing  */
	uint32_t idle_sc;    /* idle scaling */
	uint32_t timeout_sc; /* timeout scaling */
	uint32_t bus_clock;  /* bus clock hi/lo pulse widths */
	uint8_t rpt_sta_htm; /* repeated start hold time */
};

struct i2c_xec_v3_config {
	mem_addr_t base;
	uint32_t clock_freq;
	struct gpio_dt_spec sda_gpio;
	struct gpio_dt_spec scl_gpio;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t enc_pcr;
	uint8_t port;
};

#define I2C_XEC_XFR_FLAG_START_REQ 0x01
#define I2C_XEC_XFR_FLAG_STOP_REQ  0x02

#define I2C_XEC_XFR_STS_NACK 0x01
#define I2C_MEC5_XFR_STS_BER 0x02
#define I2C_MEC5_XFR_STS_LAB 0x04

struct i2c_xec_cm_xfr {
	volatile uint8_t *mbuf;
	volatile size_t mlen;
	volatile uint8_t xfr_sts;
	uint8_t mdir;
	uint8_t target_addr;
	uint8_t mflags;
};

#define I2C_XEC_TARG_PROG0_IDX    0
#define I2C_XEC_TARG_PROG1_IDX    1
#define I2C_XEC_TARG_SMB_HA_IDX   2
#define I2C_XEC_TARG_SMB_DA_IDX   3
#define I2C_XEC_TARG_GEN_CALL_IDX 4
#define I2C_XEC_TARG_ADDR_MAX     5

#define I2C_XEC_TARG_BITMAP_MSK 0x1fu

struct i2c_xec_target {
	uint16_t targ_addr;
	uint8_t targ_data;
	uint8_t targ_ignore;
	uint8_t targ_active;
	uint8_t targ_bitmap;
	uint32_t read_shad_addr_cnt;
	uint8_t *targ_buf_ptr;
	uint32_t targ_buf_len;
	struct i2c_target_config *curr_target;
	struct i2c_target_config *tcfgs[I2C_XEC_TARG_ADDR_MAX];
};

struct i2c_xec_v3_data {
	struct k_work kworkq;
	const struct device *dev;
	struct k_mutex lock_mut;
	struct k_sem sync_sem;
	uint32_t i2c_config;
	uint32_t clock_freq;
	uint32_t i2c_compl;
	uint8_t i2c_cr_shadow;
	uint8_t i2c_sr;
	uint8_t port_sel;
	uint8_t wraddr;
	uint8_t state;
	uint8_t cm_dir;
	uint8_t msg_idx;
	uint8_t num_msgs;
	struct i2c_msg *msgs;
	struct i2c_xec_cm_xfr cm_xfr;
	volatile uint8_t mdone;
#ifdef CONFIG_I2C_TARGET
	struct i2c_xec_target tg;
#endif
};

static const struct xec_i2c_timing xec_i2c_timing_tbl[] = {
	{KHZ(100), XEC_I2C_SMB_DATA_TM_100K, XEC_I2C_SMB_IDLE_SC_100K, XEC_I2C_SMB_TMO_SC_100K,
	 XEC_I2C_SMB_BUS_CLK_100K, XEC_I2C_SMB_RSHT_100K},
	{KHZ(400), XEC_I2C_SMB_DATA_TM_400K, XEC_I2C_SMB_IDLE_SC_400K, XEC_I2C_SMB_TMO_SC_400K,
	 XEC_I2C_SMB_BUS_CLK_400K, XEC_I2C_SMB_RSHT_400K},
	{MHZ(1), XEC_I2C_SMB_DATA_TM_1M, XEC_I2C_SMB_IDLE_SC_1M, XEC_I2C_SMB_TMO_SC_1M,
	 XEC_I2C_SMB_BUS_CLK_1M, XEC_I2C_SMB_RSHT_1M},
};

static int xec_i2c_prog_standard_timing(const struct device *dev, uint32_t freq_hz)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;

	for (size_t n = 0; n < ARRAY_SIZE(xec_i2c_timing_tbl); n++) {
		const struct xec_i2c_timing *p = &xec_i2c_timing_tbl[n];

		if (freq_hz == p->freq_hz) {
			sys_write32(p->data_tm, rb + XEC_I2C_DT_OFS);
			sys_write32(p->idle_sc, rb + XEC_I2C_ISC_OFS);
			sys_write32(p->timeout_sc, rb + XEC_I2C_TMOUT_SC_OFS);
			sys_write16(p->bus_clock, rb + XEC_I2C_BCLK_OFS);
			sys_write8(p->rpt_sta_htm, rb + XEC_I2C_RSHT_OFS);

			return 0;
		}
	}

	return -EINVAL;
}

static void xec_i2c_cr_write(const struct device *dev, uint8_t ctrl_val)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;

	data->i2c_cr_shadow = ctrl_val;
	sys_write8(ctrl_val, devcfg->base + XEC_I2C_CR_OFS);
}

#ifdef CONFIG_I2C_TARGET
static void xec_i2c_cr_write_mask(const struct device *dev, uint8_t clr_msk, uint8_t set_msk)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;

	data->i2c_cr_shadow = (data->i2c_cr_shadow & (uint8_t)~clr_msk) | set_msk;
	sys_write8(data->i2c_cr_shadow, devcfg->base + XEC_I2C_CR_OFS);
}
#endif

static int wait_bus_free(const struct device *dev, uint32_t nwait)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	uint32_t count = nwait;
	uint8_t sts = 0;

	while (count--) {
		sts = sys_read8(rb + XEC_I2C_SR_OFS);
		data->i2c_sr = sts;

		if ((sts & BIT(XEC_I2C_SR_NBB_POS)) != 0) {
			break; /* bus is free */
		}

		k_busy_wait(WAIT_INTERVAL);
	}

	/* check for bus error, lost arbitration or external stop */
	if (sts == (BIT(XEC_I2C_SR_NBB_POS) | BIT(XEC_I2C_SR_PIN_POS))) {
		return 0;
	}

	if ((sts & BIT(XEC_I2C_SR_BER_POS)) != 0) {
		return XEC_I2C_ERR_BUS;
	}

	if ((sts & BIT(XEC_I2C_SR_LAB_POS)) != 0) {
		return XEC_I2C_ERR_LOST_ARB;
	}

	return XEC_I2C_ERR_TIMEOUT;
}

/* return 0 if SCL and SDA are both high else return -EIO */
#if defined(CONFIG_SOC_SERIES_MEC172X)
static int check_lines(const struct device *dev)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	gpio_port_value_t sda = 0, scl = 0;

	gpio_port_get_raw(devcfg->sda_gpio.port, &sda);
	scl = sda;
	if (devcfg->sda_gpio.port != devcfg->scl_gpio.port) {
		gpio_port_get_raw(devcfg->scl_gpio.port, &scl);
	}

	if ((sda & BIT(devcfg->sda_gpio.pin)) && (scl & BIT(devcfg->scl_gpio.pin))) {
		return 0;
	}

	return -EIO;
}

/* returns uint8_t with bit[0] = SCL and bit[1] = SDA */
static uint8_t get_lines(const struct device *dev)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	gpio_port_value_t sda = 0, scl = 0;
	uint8_t lines = 0;

	gpio_port_get_raw(devcfg->scl_gpio.port, &scl);
	gpio_port_get_raw(devcfg->sda_gpio.port, &sda);

	if ((sda & BIT(devcfg->scl_gpio.pin)) != 0) {
		lines |= BIT(XEC_I2C_SCL_LINE_POS);
	}

	if ((sda & BIT(devcfg->sda_gpio.pin)) != 0) {
		lines |= BIT(XEC_I2C_SDA_LINE_POS);
	}

	return lines;
}
#else
static int check_lines(const struct device *dev)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;
	uint8_t himsk = BIT(XEC_I2C_BBCR_SCL_IN_POS) | BIT(XEC_I2C_BBCR_SDA_IN_POS);
	uint8_t bbcr = 0;

	sys_write8(BIT(XEC_I2C_BBCR_CM_POS), rb + XEC_I2C_BBCR_OFS);
	bbcr = sys_read8(rb + XEC_I2C_BBCR_OFS);

	if ((bbcr & himsk) == himsk) {
		return 0;
	}

	return -EIO;
}

/* returns uint8_t with bit[0] = SCL and bit[1] = SDA */
static uint8_t get_lines(const struct device *dev)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;
	uint8_t bbcr = 0;
	uint8_t lines = 0;

	sys_write8(BIT(XEC_I2C_BBCR_CM_POS), rb + XEC_I2C_BBCR_OFS);
	bbcr = sys_read8(rb + XEC_I2C_BBCR_OFS);

	if ((bbcr & BIT(XEC_I2C_BBCR_SCL_IN_POS)) != 0) {
		lines |= BIT(XEC_I2C_SCL_LINE_POS);
	}

	if ((bbcr & BIT(XEC_I2C_BBCR_SDA_IN_POS)) != 0) {
		lines |= BIT(XEC_I2C_SDA_LINE_POS);
	}

	return lines;
}
#endif

#ifdef CONFIG_I2C_TARGET
/* NOTE: index values are assigned.
 * 0 = programmable address 0, 1 = programmable address 1
 * 2 = I2C general call at target address 0
 * 3 = SMBus host address at 0x08
 * 4 = SMBus device address at 0x61
 */
static void prog_target_addresses(const struct device *dev)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;
	struct i2c_xec_target *ptg = &data->tg;
	mem_addr_t rb = devcfg->base;
	uint32_t t = 0;

	for (uint32_t n = 0; n < I2C_XEC_TARG_ADDR_MAX; n++) {
		struct i2c_target_config *tcfg = ptg->tcfgs[n];

		if (tcfg == NULL) {
			continue;
		}

		if (tcfg->address == XEC_I2C_GEN_CALL_ADDR) {
			sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_GC_DIS_POS);
		} else if ((tcfg->address == XEC_I2C_SMB_HOST_ADDR) ||
			   (tcfg->address == XEC_I2C_SMB_DEVICE_ADDR)) {
			sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_DSA_POS);
		} else { /* Handle the two programmable target addresses */
			t = XEC_I2C_OA_SET(n, tcfg->address);
		}
	}

	if (t != 0) {
		sys_write32(t, rb + XEC_I2C_OA_OFS);
	}
}
#endif /* CONFIG_I2C_TARGET */

static int i2c_xec_reset_config(const struct device *dev, uint32_t config, uint8_t port)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	uint32_t val = 0;
	int rc = 0;
	uint8_t crval = 0;

	data->i2c_config = config;
	data->port_sel = port;
	data->i2c_cr_shadow = 0;

	data->state = XEC_I2C_STATE_CLOSED;
	data->i2c_cr_shadow = 0;
	data->i2c_sr = 0;
	data->i2c_compl = 0;
	data->mdone = 0;
	data->port_sel = port;

	soc_xec_pcr_sleep_en_clear(devcfg->enc_pcr);
	/* reset I2C controller using PCR reset feature */
	soc_xec_pcr_reset_en(devcfg->enc_pcr);

	/* make sure general call and SMBus target address decodes disabled */
	sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_DSA_POS);
	sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_GC_DIS_POS);

	crval = BIT(XEC_I2C_CR_PIN_POS);
	xec_i2c_cr_write(dev, crval);

#ifdef CONFIG_I2C_TARGET
	prog_target_addresses(dev);
#endif

	/* timing registers */
	xec_i2c_prog_standard_timing(dev, data->clock_freq);

	/* enable output driver and ACK logic */
	crval = XEC_I2C_CR_PIN_ESO_ENI_ACK;
	xec_i2c_cr_write(dev, crval);

	/* port and filter enable */
	val = XEC_I2C_CFG_PORT_SET((uint32_t)port);
	val |= BIT(XEC_I2C_CFG_FEN_POS);
	sys_set_bits(rb + XEC_I2C_CFG_OFS, val);

	/* Enable live monitoring of SDA and SCL. No effect on MEC15xx and MEC172x */
	sys_write8(BIT(XEC_I2C_BBCR_CM_POS), rb + XEC_I2C_BBCR_OFS);

	/* enable */
	sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_ENAB_POS);

	/* wait for NBB=1, BER, LAB, or timeout */
	rc = wait_bus_free(dev, WAIT_COUNT);

	return rc;
}

static int i2c_xec_bb_recover(const struct device *dev)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	int ret = 0;
	uint32_t cnt = I2C_XEC_RECOVER_SCL_LOW_RETRIES;
	uint8_t bbcr = 0;
	uint8_t lines = 0u;

	i2c_xec_reset_config(dev, data->i2c_config, data->port_sel);

	lines = get_lines(dev);
	if ((lines & XEC_I2C_LINES_MSK) == XEC_I2C_LINES_MSK) {
		return 0;
	}

	/* Disconnect SDL and SDA from I2C logic and connect to bit-bang logic */
	bbcr = BIT(XEC_I2C_BBCR_EN_POS) | BIT(XEC_I2C_BBCR_CM_POS);
	sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);

	lines = get_lines(dev);

	/* If SCL is low continue sampling hoping it will go high on its own */
	while (!(lines & BIT(XEC_I2C_SCL_LINE_POS))) {
		if (cnt) {
			cnt--;
		} else {
			break;
		}
		k_busy_wait(I2C_XEC_RECOVER_SCL_DELAY_US);
		lines = get_lines(dev);
	}

	lines = get_lines(dev);
	if ((lines & BIT(XEC_I2C_SCL_LINE_POS)) == 0) {
		ret = -EBUSY;
		goto disable_bb_exit;
	}

	/* SCL is high, check SDA */
	if ((lines & BIT(XEC_I2C_SDA_LINE_POS)) != 0) {
		ret = 0; /* both high */
		goto disable_bb_exit;
	}

	/* SCL is high and SDA is low. Loop generating 9 clocks until
	 * we observe SDA high or loop terminates
	 */
	ret = -EBUSY;
	for (int i = 0; i < I2C_XEC_RECOVER_SDA_LOW_RETRIES; i++) {
		/* tri-state SCL & inputs */
		bbcr = BIT(XEC_I2C_BBCR_CM_POS) | BIT(XEC_I2C_BBCR_EN_POS);
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);

		/* 9 clocks */
		for (int j = 0; j < 9; j++) {
			/* drive SCL low  by SCL output drive low, SDA tri-state input */
			bbcr = (BIT(XEC_I2C_BBCR_CM_POS) | BIT(XEC_I2C_BBCR_CD_POS) |
				BIT(XEC_I2C_BBCR_EN_POS));
			sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
			k_busy_wait(I2C_XEC_RECOVER_BB_DELAY_US);
			/* SCL & SDA tri-state inputs, external pull-up should pull signals high */
			bbcr = BIT(XEC_I2C_BBCR_CM_POS) | BIT(XEC_I2C_BBCR_EN_POS);
			sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
			k_busy_wait(I2C_XEC_RECOVER_BB_DELAY_US);
		}

		lines = get_lines(dev);
		if ((lines & XEC_I2C_LINES_MSK) == XEC_I2C_LINES_MSK) { /* Both high? */
			ret = 0;
			goto disable_bb_exit;
		}

		/* generate I2C STOP.  While SCL is high SDA transitions low to high */
		/* SCL tri-state input (high), drive SDA low */
		bbcr = (BIT(XEC_I2C_BBCR_CM_POS) | BIT(XEC_I2C_BBCR_DD_POS) |
			BIT(XEC_I2C_BBCR_EN_POS));
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
		k_busy_wait(I2C_XEC_RECOVER_BB_DELAY_US);
		/* SCL and SDA tri-state inputs. */
		bbcr = BIT(XEC_I2C_BBCR_CM_POS) | BIT(XEC_I2C_BBCR_EN_POS);
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
		k_busy_wait(I2C_XEC_RECOVER_BB_DELAY_US);

		lines = get_lines(dev);
		if ((lines & XEC_I2C_LINES_MSK) == XEC_I2C_LINES_MSK) { /* Both high? */
			ret = 0;
			goto disable_bb_exit;
		}
	}

disable_bb_exit:
	bbcr = BIT(XEC_I2C_BBCR_CM_POS);
	sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);

	return ret;
}

static int i2c_xec_recover_bus(const struct device *dev)
{
	struct i2c_xec_v3_data *const data = dev->data;
	int ret = 0;

	LOG_ERR("I2C attempt bus recovery\n");

	/* Try controller reset first */
	ret = i2c_xec_reset_config(dev, data->i2c_config, data->port_sel);
	if (ret == 0) {
		ret = check_lines(dev);
	}

	if (ret != 0) {
		return 0;
	}

	ret = i2c_xec_bb_recover(dev);
	if (ret == 0) {
		ret = wait_bus_free(dev, WAIT_COUNT);
	}

	return ret;
}

/* Configure I2C controller for speed and hardware port if parameters are support */
static int i2c_xec_cfg(const struct device *dev, uint32_t dev_config_raw, uint8_t port)
{
	struct i2c_xec_v3_data *const data = dev->data;

	if (port >= XEC_I2C_MAX_PORTS) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		data->clock_freq = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		data->clock_freq = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		data->clock_freq = MHZ(1);
		break;
	default:
		return -EINVAL;
	}

	return i2c_xec_reset_config(dev, dev_config_raw, port);
}

/* i2c_configure API */
static int i2c_xec_configure(const struct device *dev, uint32_t dev_config_raw)
{
	struct i2c_xec_v3_data *const data = dev->data;
	int rc = 0;

	if (!(dev_config_raw & I2C_MODE_CONTROLLER)) {
		return -ENOTSUP;
	}

	rc = k_mutex_lock(&data->lock_mut, K_NO_WAIT);
	if (rc != 0) {
		return rc;
	}

	rc = i2c_xec_cfg(dev, dev_config_raw, data->port_sel);

	k_mutex_unlock(&data->lock_mut);

	return rc;
}

/* MCHP XEC v3 specific APIs */
int i2c_xec_v3_get_port(const struct device *dev, uint8_t *port)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;

	if ((dev == NULL) || (port == NULL)) {
		return -EINVAL;
	}

	if (k_mutex_lock(&data->lock_mut, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	*port = (uint8_t)XEC_I2C_CFG_PORT_GET(sys_read32(rb + XEC_I2C_CFG_OFS));

	k_mutex_unlock(&data->lock_mut);

	return 0;
}

int i2c_xec_v3_config(const struct device *dev, uint32_t config, uint8_t port)
{
	struct i2c_xec_v3_data *const data = dev->data;

	if (dev == NULL) {
		return -EINVAL;
	}

	if (k_mutex_lock(&data->lock_mut, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	return i2c_xec_cfg(dev, config, port);
}

/* i2c_get_config API */
static int i2c_xec_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_xec_v3_data *const data = dev->data;
	uint32_t dcfg = 0u;

	if (dev_config == NULL) {
		return -EINVAL;
	}

	dcfg = data->i2c_config;

#ifdef CONFIG_I2C_TARGET
	if (data->tg.targ_bitmap == 0) {
		dcfg |= I2C_MODE_CONTROLLER;
	} else {
		dcfg &= ~I2C_MODE_CONTROLLER;
	}
#else
	dcfg |= I2C_MODE_CONTROLLER;
#endif
	*dev_config = dcfg;

	return 0;
}

/* XEC I2C controller support 7-bit addressing only.
 * Format 7-bit address for as it appears on the bus as an 8-bit
 * value with R/W bit at bit[0], 0(write), 1(read).
 */
static inline uint8_t i2c_xec_fmt_addr(uint16_t addr, uint8_t read)
{
	uint8_t fmt_addr = (uint8_t)((addr & XEC_I2C_TARGET_ADDR_MSK) << 1);

	if (read != 0) {
		fmt_addr |= BIT(0);
	}

	return fmt_addr;
}

/* I2C STOP only if controller owns the bus otherwise
 * clear driver state and re-arm controller for next
 * controller-mode or target-mode transaction.
 * Reason for ugly code sequence:
 * Brain-dead I2C controller has write-only control register
 * containing enable interrupt bit. This is the enable for ACK/NACK,
 * bus error and lost arbitration.
 * NOTE: IDLE interrupt has issues. If it is enabled it can fire if the bus
 * goes IDLE before we perform an action such as generate the STOP.
 */
static int i2c_xec_stop(const struct device *dev, uint32_t flags)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	int rc = 0;
	uint8_t ctrl = 0, sts = 0;

	/* Is the bus busy? */
	sts = sys_read8(rb + XEC_I2C_SR_OFS);
	if ((sts & BIT(XEC_I2C_SR_NBB_POS)) == 0) {
		data->mdone = 0;
		ctrl = (BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) |
			BIT(XEC_I2C_CR_STO_POS) | BIT(XEC_I2C_CR_ACK_POS));

		/* disable IDLE interrupt in config register */
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
		/* clear IDLE R/W1C status in completion register */
		sys_set_bit(rb + XEC_I2C_CMPL_OFS, XEC_I2C_CMPL_IDLE_POS);
		/* clear GIRQ status */
		soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

		/* generate STOP */
		xec_i2c_cr_write(dev, ctrl);

		if (flags & BIT(0)) { /* detect STOP completion with interrupt */
			/* enable IDLE interrupt in config register */
			sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
			rc = k_sem_take(&data->sync_sem, K_MSEC(10));
		} else {
			rc = wait_bus_free(dev, WAIT_COUNT);
		}
	}

	data->cm_dir = XEC_I2C_DIR_NONE;
	data->state = XEC_I2C_STATE_CLOSED;

	return rc;
}

static int check_msgs(struct i2c_msg *msgs, uint8_t num_msgs)
{
	for (uint8_t n = 0u; n < num_msgs; n++) {
		struct i2c_msg *m = &msgs[n];

		if ((m->flags & I2C_MSG_ADDR_10_BITS) != 0) {
			return -EINVAL;
		}
	}

	return 0;
}

static int i2c_xec_xfr_begin(const struct device *dev, uint16_t addr)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	struct i2c_msg *m = data->msgs;
	mem_addr_t rb = devcfg->base;
	uint8_t target_addr = 0;
	uint8_t ctrl = XEC_I2C_CR_START_ENI;
	uint8_t start = XEC_I2C_START_NORM;

	target_addr = i2c_xec_fmt_addr(addr, 0);
	data->wraddr = target_addr;

	if ((data->msgs[0].flags & I2C_MSG_READ) != 0) {
		target_addr |= BIT(0);
		xfr->mdir = XEC_I2C_DIR_RD;
	} else {
		xfr->mdir = XEC_I2C_DIR_WR;
	}

	data->mdone = 0;
	xfr->mbuf = m->buf;
	xfr->mlen = m->len;
	xfr->xfr_sts = 0;
	xfr->target_addr = target_addr;
	xfr->mflags = I2C_XEC_XFR_FLAG_START_REQ;

	if (((sys_read8(rb + XEC_I2C_SR_OFS) & BIT(XEC_I2C_SR_NBB_POS)) == 0) &&
	    ((data->cm_dir != xfr->mdir) || (m->flags & I2C_MSG_RESTART))) {
		start = XEC_I2C_START_RPT;
		ctrl = XEC_I2C_CR_RPT_START_ENI;
	}

	data->cm_dir = xfr->mdir;
	if (m->flags & I2C_MSG_STOP) {
		xfr->mflags |= I2C_XEC_XFR_FLAG_STOP_REQ;
	}

	soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 0);
	soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

	/* Generate (RPT)-START and transmit address for write or read */
	if (ctrl == XEC_I2C_CR_START_ENI) { /* START? */
		sys_write8(target_addr, rb + XEC_I2C_DATA_OFS);
		xec_i2c_cr_write(dev, ctrl);
	} else { /* RPT-START */
		xec_i2c_cr_write(dev, ctrl);
		sys_write8(target_addr, rb + XEC_I2C_DATA_OFS);
	}

	soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 1u);

	if (k_sem_take(&data->sync_sem, K_MSEC(100)) != 0) {
		return -ETIMEDOUT;
	}

	if (xfr->xfr_sts) { /* error */
		return -EIO;
	}

	return 0;
}

/* i2c_transfer API - Synchronous using interrupts
 * The call wrapper in i2c.h returns if num_msgs is 0.
 * It does not check for msgs being a NULL pointer and accesses msgs.
 * NOTE 1: Zephyr I2C documentation states an I2C driver can be switched
 * between Host and Target modes by registering and unregistering targets.
 * NOTE 2:
 * XEC I2C controller supports up to 5 target addresses:
 * Two address match registers.
 * I2C general call (address 0). UNTESTED!
 * Two SMBus fixed addresses defined in the SMBus spec. UNTESTED!
 * We many need to remove general call and the two SMBus fixed address code logic!
 */
static int i2c_xec_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	int rc = 0;

	k_mutex_lock(&data->lock_mut, K_FOREVER);
#ifdef CONFIG_I2C_TARGET
	if (data->tg.targ_bitmap != 0) {
		k_mutex_unlock(&data->lock_mut);
		return -EBUSY;
	}
#endif
	pm_device_busy_set(dev);
	k_sem_reset(&data->sync_sem);

	memset(&data->cm_xfr, 0, sizeof(struct i2c_xec_cm_xfr));

	rc = check_msgs(msgs, num_msgs);
	if (rc != 0) {
		goto xec_unlock;
	}

	if (data->state != XEC_I2C_STATE_OPEN) {
		rc = check_lines(dev);
		data->i2c_sr = sys_read8(rb + XEC_I2C_SR_OFS);
		data->i2c_compl = sys_read32(rb + XEC_I2C_CMPL_OFS);

		if (rc || (data->i2c_sr & BIT(XEC_I2C_SR_BER_POS))) {
			rc = i2c_xec_recover_bus(dev);
		}
	}

	if (rc) {
		data->state = XEC_I2C_STATE_CLOSED;
		goto xec_unlock;
	}

	data->state = XEC_I2C_STATE_OPEN;

	data->msg_idx = 0;
	data->num_msgs = num_msgs;
	data->msgs = msgs;

	rc = i2c_xec_xfr_begin(dev, addr);
	if (rc) { /* if error issue STOP if bus is still owned by controller */
		i2c_xec_stop(dev, 0);
	}

xec_unlock:
	if ((sys_read8(rb + XEC_I2C_SR_OFS) & BIT(XEC_I2C_SR_NBB_POS)) == 0) {
		data->cm_dir = XEC_I2C_DIR_NONE;
		data->state = XEC_I2C_STATE_CLOSED;
	}

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->lock_mut);

	return rc;
}

#ifdef CONFIG_I2C_TARGET
static struct i2c_target_config *find_target(struct i2c_xec_v3_data *data, uint16_t i2c_addr)
{
	struct i2c_xec_target *ptg = &data->tg;

	if (i2c_addr == ptg->tcfgs[I2C_XEC_TARG_PROG0_IDX]->address) {
		return ptg->tcfgs[I2C_XEC_TARG_PROG0_IDX];
	}

	if (i2c_addr == ptg->tcfgs[I2C_XEC_TARG_PROG1_IDX]->address) {
		return ptg->tcfgs[I2C_XEC_TARG_PROG1_IDX];
	}

	if (i2c_addr == XEC_I2C_GEN_CALL_ADDR) {
		return ptg->tcfgs[I2C_XEC_TARG_GEN_CALL_IDX];
	}

	if (i2c_addr == XEC_I2C_SMB_HOST_ADDR) {
		return ptg->tcfgs[I2C_XEC_TARG_SMB_HA_IDX];
	}

	if (i2c_addr == XEC_I2C_SMB_DEVICE_ADDR) {
		return ptg->tcfgs[I2C_XEC_TARG_SMB_DA_IDX];
	}

	return NULL;
}

/* I2C can respond to 3 fixed address and 2 configurable
 * address 0x00 if GC_DIS == 0 in configuration register
 * addresses 0x08 and 0x61 if DSA == 1 in configuration register
 * Own addresses 1 and 2 which are programmable.
 * NOTE: Zephyr uses target_register to enable target mode and
 * target_unregister to disable target mode. The app will use
 * these for switching between host and target modes.
 * Since our HW supports multiple target, the app must unregister all
 * targets before Host mode is allowed.
 */
static int check_targ_config(struct i2c_target_config *cfg)
{
	if (cfg == NULL) {
		return -EINVAL;
	}

	if (((cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) != 0) ||
	    (cfg->address > XEC_I2C_TARGET_ADDR_MSK)) {
		return -EINVAL;
	}

	return 0;
}

static int target_i2c_gen_call(const struct device *dev, struct i2c_target_config *cfg,
			       uint8_t enable)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *data = dev->data;
	struct i2c_xec_target *ptg = &data->tg;
	mem_addr_t rb = devcfg->base;

	if (enable != 0) {
		if (ptg->tcfgs[I2C_XEC_TARG_GEN_CALL_IDX] != NULL) {
			return -EEXIST;
		}

		ptg->tcfgs[I2C_XEC_TARG_GEN_CALL_IDX] = cfg;
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_GC_DIS_POS);
		ptg->targ_bitmap |= BIT(I2C_XEC_TARG_GEN_CALL_IDX);
	} else {
		sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_GC_DIS_POS);
		ptg->tcfgs[I2C_XEC_TARG_GEN_CALL_IDX] = NULL;
		ptg->targ_bitmap &= ~BIT(I2C_XEC_TARG_GEN_CALL_IDX);
	}

	return 0;
}

static int target_smb_hd(const struct device *dev, struct i2c_target_config *cfg, uint8_t enable,
			 uint8_t targ_idx)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *data = dev->data;
	struct i2c_xec_target *ptg = &data->tg;
	mem_addr_t rb = devcfg->base;

	if (enable != 0) {
		if (ptg->tcfgs[targ_idx] != NULL) {
			return -EEXIST;
		}

		ptg->targ_bitmap |= BIT(targ_idx);
		ptg->tcfgs[targ_idx] = cfg;

		sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_DSA_POS);
	} else {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_DSA_POS);
		ptg->tcfgs[targ_idx] = NULL;
		ptg->targ_bitmap &= ~BIT(targ_idx);
	}

	return 0;
}

/* MEC I2C controller own address register implements two 7-bit addresses located at
 * bits[6:0] and bits[14:8].
 */
static int target_prog_addr(const struct device *dev, struct i2c_target_config *cfg, uint8_t en)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *data = dev->data;
	struct i2c_xec_target *ptg = &data->tg;
	mem_addr_t rb = devcfg->base;
	uint32_t oar = sys_read32(rb + XEC_I2C_OA_OFS);
	uint32_t msk = XEC_I2C_TARGET_ADDR_MSK;
	int rc = -EEXIST;
	uint16_t oaddr = 0;
	uint8_t pos = 0;

	for (uint32_t n = 0; n < XEC_I2C_OA_NUM_TARGETS; n++) {
		oaddr = (oar & msk) >> pos;
		if (oaddr == 0) { /* address disabled? */
			if (en != 0) {
				ptg->tcfgs[n + I2C_XEC_TARG_PROG0_IDX] = cfg;
				ptg->targ_bitmap |= BIT(I2C_XEC_TARG_PROG0_IDX + n);
				soc_mmcr_mask_set(rb + XEC_I2C_OA_OFS, cfg->address, msk);
				rc = 0;
				break;
			}
		} else {
			if ((oaddr == cfg->address) && (en == 0)) {
				soc_mmcr_mask_set(rb + XEC_I2C_OA_OFS, 0, msk);
				ptg->tcfgs[n + I2C_XEC_TARG_PROG0_IDX] = NULL;
				ptg->targ_bitmap &= ~BIT(I2C_XEC_TARG_PROG0_IDX + n);
				rc = 0;
				break;
			}
		}

		msk <<= 8;
		pos += 8u;
	}

	return rc;
}

static int config_target_address(const struct device *dev, struct i2c_target_config *cfg,
				 uint8_t enable)
{
	int rc = 0;

	switch (cfg->address) {
	case XEC_I2C_GEN_CALL_ADDR:
		rc = target_i2c_gen_call(dev, cfg, enable);
		break;
	case XEC_I2C_SMB_HOST_ADDR:
		rc = target_smb_hd(dev, cfg, enable, I2C_XEC_TARG_SMB_HA_IDX);
		break;
	case XEC_I2C_SMB_DEVICE_ADDR:
		rc = target_smb_hd(dev, cfg, enable, I2C_XEC_TARG_SMB_DA_IDX);
		break;
	default:
		rc = target_prog_addr(dev, cfg, enable);
	}

	return rc;
}

/* Register target specified by struct i2c_target_config
 * Hardware supports two 7-bit target addresses and three fixed addresses.
 * I2C general call address 0 controlled by enable bit in configuration registers
 * SMBus Host address 0x08 and device address 0x61 controlled by one enable bit in the
 * configuration register.
 * 1. Check if i2c_target_config is valid. If valid take lock.
 * 2. Check if struct i2c_target_config is currently registers, if so return 0.
 * 3. Check if address is one of the fixed address if yes turn on the fixed address
 * 4. Check if one of the two programmable addresses is available (0), if yes configure it
 *    else return no address available (-ENOSPC).
 */
static int i2c_xec_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_xec_v3_config *drvcfg = dev->config;
	struct i2c_xec_v3_data *data = dev->data;
	struct i2c_xec_target *ptg = &data->tg;
	mm_reg_t rb = drvcfg->base;
	int rc = 0;

	rc = check_targ_config(cfg);
	if (rc != 0) {
		return rc;
	}

	rc = k_mutex_lock(&data->lock_mut, K_MSEC(XEC_I2C_TM_REGISTER_WAIT_MS));
	if (rc != 0) {
		return -EBUSY;
	}

	rc = config_target_address(dev, cfg, 1u);

	if (ptg->targ_bitmap != 0) {
		sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_AAT_IEN_POS);
		soc_ecia_girq_ctrl(drvcfg->girq, drvcfg->girq_pos, 1u);
	} else {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_AAT_IEN_POS);
		soc_ecia_girq_ctrl(drvcfg->girq, drvcfg->girq_pos, 0);
	}

	k_mutex_unlock(&data->lock_mut);

	return rc;
}

static int i2c_xec_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_xec_v3_config *drvcfg = dev->config;
	struct i2c_xec_v3_data *data = dev->data;
	struct i2c_xec_target *ptg = &data->tg;
	mm_reg_t rb = drvcfg->base;
	int rc = 0;

	rc = check_targ_config(cfg);
	if (rc != 0) {
		return rc;
	}

	rc = k_mutex_lock(&data->lock_mut, K_MSEC(XEC_I2C_TM_REGISTER_WAIT_MS));
	if (rc != 0) {
		return -EBUSY;
	}

	rc = config_target_address(dev, cfg, 0);

	if (ptg->targ_bitmap == 0) {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_AAT_IEN_POS);
		soc_ecia_girq_ctrl(drvcfg->girq, drvcfg->girq_pos, 0);
	}

	k_mutex_unlock(&data->lock_mut);

	return rc;
}
#endif /* CONFIG_I2C_TARGET */

/* ISR helpers and state handlers */
static int i2c_xec_is_ber_lab(struct i2c_xec_v3_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;

	if ((data->i2c_sr & (BIT(XEC_I2C_SR_BER_POS) | BIT(XEC_I2C_SR_LAB_POS))) != 0) {
		if (data->i2c_sr & BIT(XEC_I2C_SR_BER_POS)) {
			xfr->xfr_sts |= XEC_I2C_ERR_BUS;
		} else {
			xfr->xfr_sts |= XEC_I2C_ERR_LOST_ARB;
		}

		soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 0);
		data->i2c_sr = sys_read8(rb + XEC_I2C_SR_OFS);
		data->i2c_compl = sys_read32(rb + XEC_I2C_CMPL_OFS);
		data->mdone = 0x51;

		return 1;
	}

	return 0;
}

static int i2c_xec_next_msg(struct i2c_xec_v3_data *data)
{
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	struct i2c_msg *m = NULL;
	uint32_t idx = (uint32_t)data->msg_idx;

	if (++idx >= (uint32_t)data->num_msgs) {
		xfr->mbuf = NULL;
		xfr->mlen = 0;
		xfr->mflags = 0;
		xfr->mdir = XEC_I2C_DIR_NONE;
		return 0;
	}

	data->msg_idx = (uint8_t)(idx & 0xffu);
	m = &data->msgs[idx];

	xfr->mbuf = m->buf;
	xfr->mlen = m->len;
	xfr->mdir = XEC_I2C_DIR_WR;
	xfr->mflags = 0;
	xfr->target_addr = data->wraddr;

	if ((m->flags & I2C_MSG_READ) != 0) {
		xfr->mdir = XEC_I2C_DIR_RD;
		xfr->target_addr |= BIT(0);
	}

	if ((m->flags & I2C_MSG_STOP) != 0) {
		xfr->mflags = I2C_XEC_XFR_FLAG_STOP_REQ;
	}

	if (((m->flags & I2C_MSG_RESTART) != 0) || (data->cm_dir != xfr->mdir)) {
		xfr->mflags |= I2C_XEC_XFR_FLAG_START_REQ;
	}

	data->cm_dir = xfr->mdir;

	return 1;
}

#ifdef CONFIG_I2C_TARGET
/* When address as a target, I2C hardware sets bits based on the matched target address.
 * I2C.STATUS.AAT = 1
 *   The received address  matches I2C general call or one of the two own addresses.
 *   I2C.STATUS.LRB/AD0 = 0(own address match), 1(I2C general call)
 *   If DSA is enable then we must check the actual received address value to determine
 *   if it matches SMBus Host or Device address.
 */

static void xec_i2c_tm_host_rd_req(struct i2c_xec_v3_data *data,
				   const struct i2c_target_callbacks *tcbs)
{
	struct i2c_xec_target *ptg = &data->tg;
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;

	if (((tcbs != NULL) && (tcbs->read_requested != NULL)) &&
	    (tcbs->read_requested(ptg->curr_target, &ptg->targ_data) == 0)) {
		ptg->targ_ignore = 0;
	}

	/* read & discard target address clears I2C.SR.AAT */
	sys_read8(rb + XEC_I2C_DATA_OFS);
	/* as target transmitter writing I2C.DATA releases clock stretching */
	sys_write8(ptg->targ_data, rb + XEC_I2C_DATA_OFS);
}

static void xec_i2c_tm_host_wr_req(struct i2c_xec_v3_data *data,
				   const struct i2c_target_callbacks *tcbs)
{
	struct i2c_xec_target *ptg = &data->tg;
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;

	if (((tcbs != NULL) && (tcbs->write_requested != NULL)) &&
	    (tcbs->write_requested(ptg->curr_target) == 0)) {
		ptg->targ_ignore = 0u;
	}

	if (ptg->targ_ignore != 0) {
		xec_i2c_cr_write_mask(dev, BIT(XEC_I2C_CR_ACK_POS), 0);
	}

	/* as target receiver reading I2C.DATA releases clock stretching
	 * and clears I2C.SR.AAT.
	 */
	sys_read8(rb + XEC_I2C_DATA_OFS);
}

static int state_check_ack_tm(struct i2c_xec_v3_data *data)
{
	struct i2c_xec_target *ptg = &data->tg;
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	const struct i2c_target_callbacks *tcbs = NULL;
	mem_addr_t rb = devcfg->base;
	int next_state = I2C_XEC_ISR_STATE_MAX;
	uint16_t i2c_addr = 0;

#ifdef XEC_I2C_TM_SHAD_ADDR_ANOMALY
	k_busy_wait(XEC_I2C_TM_SHAD_ADDR_ANOMALY_WAIT_US);
#endif
	if ((data->i2c_sr & BIT(XEC_I2C_SR_AAT_POS)) != 0) {
		/* enable STOP detect and IDLE interrupts */
		sys_set_bit(rb + XEC_I2C_CMPL_OFS, XEC_I2C_CMPL_IDLE_POS);
		sys_set_bits(rb + XEC_I2C_CFG_OFS,
			     (BIT(XEC_I2C_CFG_IDLE_IEN_POS) | BIT(XEC_I2C_CFG_STD_IEN_POS)));

		ptg->targ_active = 1u;
		ptg->targ_ignore = 1u;
		ptg->targ_data = XEC_I2C_TM_HOST_READ_IGNORE_VAL;

		ptg->targ_addr = sys_read8(rb + XEC_I2C_IAS_OFS);

		/* extract I2C address from bus value */
		i2c_addr = (ptg->targ_addr >> 1); /* bits[7:1]=address, bit[0]=R/nW */
		ptg->curr_target = find_target(data, i2c_addr);
		if (ptg->curr_target != NULL) {
			tcbs = ptg->curr_target->callbacks;
		}

		if ((ptg->targ_addr & BIT(0)) != 0) { /* Host requesting read from target */
			xec_i2c_tm_host_rd_req(data, tcbs);
		} else { /* Host requesting write to target */
			xec_i2c_tm_host_wr_req(data, tcbs);
		}

		next_state = I2C_XEC_ISR_STATE_EXIT_1;
	} else {
		if (ptg->targ_active != 0) {
			next_state = I2C_XEC_ISR_STATE_TM_HOST_WR;
			if ((ptg->targ_addr & BIT(0)) != 0) {
				next_state = I2C_XEC_ISR_STATE_TM_HOST_RD;
			}
		}
	}

	return next_state;
}
#endif /* CONFIG_I2C_TARGET */

static int state_check_ack(struct i2c_xec_v3_data *data)
{
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	int next_state = I2C_XEC_ISR_STATE_MAX;

#ifdef CONFIG_I2C_TARGET
	next_state = state_check_ack_tm(data);
	if (next_state != I2C_XEC_ISR_STATE_MAX) {
		return next_state;
	}
#endif

	if ((data->i2c_sr & BIT(XEC_I2C_SR_LRB_AD0_POS)) == 0) { /* ACK? */
		next_state = I2C_XEC_ISR_STATE_WR_DATA;

		if (xfr->mdir == XEC_I2C_DIR_RD) {
			next_state = I2C_XEC_ISR_STATE_RD_DATA;
		}
	} else {
		next_state = I2C_XEC_ISR_STATE_GEN_STOP;
		xfr->xfr_sts |= I2C_XEC_XFR_STS_NACK;
	}

	return next_state;
}

static int state_data_wr(struct i2c_xec_v3_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	mem_addr_t rb = devcfg->base;
	int next_state = I2C_XEC_ISR_STATE_EXIT_1;

	if (xfr->mlen > 0) {
		sys_write8(*xfr->mbuf, rb + XEC_I2C_DATA_OFS);
		xfr->mbuf++;
		xfr->mlen--;
	} else {
		if (xfr->mflags & I2C_XEC_XFR_FLAG_STOP_REQ) {
			next_state = I2C_XEC_ISR_STATE_GEN_STOP;
		} else {
			next_state = I2C_XEC_ISR_STATE_NEXT_MSG;
		}
	}

	return next_state;
}

/* NOTE: Reading I2C controller Data register causes HW to
 * generate clocks for the next data byte plus (n)ACK bit.
 * In addition the Controller will always ACK received data
 * unless the I2C.CTRL auto-ACK bit is cleared.
 * If the message has I2C_MSG_STOP flag set:
 * Reading the next to last byte generates clocks for the last byte.
 * Therefore we must clear the auto-ACK bit in I2C.CTRL before
 * reading the next to last byte from I2C.Data register.
 * Before reading the last byte we must write I2C.CTRL to begin
 * generating the I2C STOP sequence. We can then read the
 * last byte from the I2C.Data register without causing clocks
 * to be generated. We hope the Controller HW does not have a
 * race condition between STOP generation and the read of I2C.Data.
 */
static int state_data_rd(struct i2c_xec_v3_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	mem_addr_t rb = devcfg->base;
	int next_state = I2C_XEC_ISR_STATE_NEXT_MSG;
	uint8_t ctrl = 0;

	if (xfr->mlen > 0) {
		next_state = I2C_XEC_ISR_STATE_EXIT_1;
		if ((xfr->mflags & I2C_XEC_XFR_FLAG_START_REQ) != 0) {
			/* HW clocks in address it transmits. Read and discard.
			 * HW generates clocks for first data byte.
			 */
			xfr->mflags &= ~(I2C_XEC_XFR_FLAG_START_REQ);
			if ((xfr->mlen == 1) && ((xfr->mflags & I2C_XEC_XFR_FLAG_STOP_REQ) != 0)) {
				/* disable auto-ACK and make sure ENI=1 */
				ctrl = BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ENI_POS);
				xec_i2c_cr_write(dev, ctrl);
			}
			/* read byte currently in HW buffer and generate clocks for next byte */
			sys_write8(sys_read8(rb + XEC_I2C_DATA_OFS), rb + XEC_I2C_BLKID_OFS);
		} else if ((xfr->mflags & I2C_XEC_XFR_FLAG_STOP_REQ) != 0) {
			if (xfr->mlen != 1) {
				if (xfr->mlen == 2) {
					ctrl = BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ENI_POS);
					xec_i2c_cr_write(dev, ctrl);
				}

				*xfr->mbuf = sys_read8(rb + XEC_I2C_DATA_OFS);
				xfr->mbuf++;
				xfr->mlen--;
			} else { /* Begin STOP generation and read last byte */
				xfr->mflags &= ~(I2C_XEC_XFR_FLAG_STOP_REQ);
				sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
				xec_i2c_cr_write(dev, XEC_I2C_CR_STOP);
				/* read triggers STOP generation */
				*xfr->mbuf = sys_read8(rb + XEC_I2C_DATA_OFS);
				xfr->mlen = 0;
			}
		} else { /* No START or STOP flags */
			*xfr->mbuf = sys_read8(rb + XEC_I2C_DATA_OFS);
			xfr->mbuf++;
			xfr->mlen--;
		}
	}

	return next_state;
}

static int state_next_msg(struct i2c_xec_v3_data *data)
{
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	int next_state = I2C_XEC_ISR_STATE_MAX;
	int ret = i2c_xec_next_msg(data);

	if (ret) {
		if (xfr->mflags & I2C_XEC_XFR_FLAG_START_REQ) {
			next_state = I2C_XEC_ISR_STATE_GEN_START;
		} else {
			next_state = I2C_XEC_ISR_STATE_WR_DATA;
			if (xfr->mdir == XEC_I2C_DIR_RD) {
				next_state = I2C_XEC_ISR_STATE_RD_DATA;
			}
		}
	} else { /* no more messages */
		data->mdone = 1;
	}

	return next_state;
}

#ifdef CONFIG_I2C_TARGET
/* state I2C_XEC_ISR_STATE_TM_HOST_RD (external Host I2 Read data phase)
 * external Host I2C Read. Application callback returned error code.
 * We "ignore" remaining protocol until STOP.
 * This I2C controller clock stretches on target address match and
 * on each ACK of data bytes we write from the external Host.
 * We must write a value to the I2C.DATA register to cause this controller
 * to release SCL allowing the external Host to generate clocks on SCL.
 */
static int state_tm_host_read(struct i2c_xec_v3_data *data)
{
	struct i2c_xec_target *ptg = &data->tg;
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_target_config *tcfg = ptg->curr_target;
	const struct i2c_target_callbacks *tcbs = tcfg->callbacks;
	mem_addr_t rb = devcfg->base;
	int rc = 0;

	if ((tcbs == NULL) || (tcbs->read_processed == NULL)) {
		ptg->targ_ignore = 1u;
	}

	if (ptg->targ_ignore == 0) {
		rc = tcbs->read_processed(tcfg, &ptg->targ_data);
		if (rc != 0) {
			ptg->targ_ignore = 1;
			ptg->targ_data = XEC_I2C_TM_HOST_READ_IGNORE_VAL;
		}
	}

	sys_write8(ptg->targ_data, rb + XEC_I2C_DATA_OFS);

	return I2C_XEC_ISR_STATE_EXIT_1;
}

/* state I2C_XEC_ISR_STATE_TM_HOST_WR (external Host I2C Write data phase)
 * external Host generated START and target write address matching this I2C target.
 * We invoked application write requested callback which returned an error code.
 * This means we must "ignore" I2C bus activity until the external Host generates STOP.
 * When the external Host generates clocks and data this controller will clock stretch
 * after the 9th clock if auto-ACK is enabled. We must read and discard the data byte
 * from I2C.DATA.
 */
static int state_tm_host_write(struct i2c_xec_v3_data *data)
{
	struct i2c_xec_target *ptg = &data->tg;
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_target_config *tcfg = ptg->curr_target;
	const struct i2c_target_callbacks *tcbs = tcfg->callbacks;
	mem_addr_t rb = devcfg->base;
	int rc = 0;

	/* read shadow data register. No side-effects */
	ptg->targ_data = sys_read8(rb + XEC_I2C_IDS_OFS);

	if (ptg->targ_ignore == 0) {
		rc = tcbs->write_received(tcfg, ptg->targ_data);
		if (rc != 0) {
			ptg->targ_ignore = 1u;
			/* clear HW auto-ACK. We NAK future received bytes */
			xec_i2c_cr_write_mask(dev, BIT(XEC_I2C_CR_ACK_POS), 0);
		}
	}

	/* must read I2C.DATA to release SCL */
	sys_read8(rb + XEC_I2C_DATA_OFS);

	return I2C_XEC_ISR_STATE_EXIT_1;
}

static int state_tm_stop_event(struct i2c_xec_v3_data *data)
{
	struct i2c_xec_target *ptg = &data->tg;
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_target_config *tcfg = ptg->curr_target;
	const struct i2c_target_callbacks *tcbs = tcfg->callbacks;
	mem_addr_t rb = devcfg->base;

	if ((tcbs != NULL) && (tcbs->stop != NULL)) {
		tcbs->stop(tcfg);
	}

	/* Race condition:
	 * Docs state: "After the function(stop callback) returns the controller
	 * shall enter a state where it is ready to react to new start conditions"
	 */
	ptg->targ_active = 0;
	ptg->targ_ignore = 0;
	ptg->curr_target = NULL;
	/* HW requires a read and discard of I2C.DATA register to clear the read-only
	 * STOP detect status in I2C.SR.
	 */
	sys_read8(rb + XEC_I2C_DATA_OFS);
	xec_i2c_cr_write(dev, XEC_I2C_CR_PIN_ESO_ENI_ACK);

	return I2C_XEC_ISR_STATE_EXIT_1;
}

static void tm_cleanup(struct i2c_xec_v3_data *data)
{
	struct i2c_xec_target *ptg = &data->tg;
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;

	ptg->targ_active = 0;
	ptg->targ_ignore = 0;
	ptg->curr_target = NULL;

	sys_read8(rb + XEC_I2C_DATA_OFS);
	/* re-arm I2C to detect external Host activity */
	xec_i2c_cr_write(dev, XEC_I2C_CR_PIN_ESO_ENI_ACK);
}
#endif /* CONFIG_I2C_TARGET */

/* SonarQube does not like GNU extensions required for CONTAINER_OF macro.
 * We work around this by placing struct k_work as the first member of struct i2c_xec_v3_data.
 * Therefore the pointer to struct k_work passed to xec_i2c_kwork_thread is also the address
 * of this instance's struct i2c_xec_v3_data.
 */
BUILD_ASSERT(offsetof(struct i2c_xec_v3_data, kworkq) == 0,
	     "Member kworkq must be located as the first memory of struct i2c_xec_v3_data");

static void xec_i2c_kwork_thread(struct k_work *work)
{
	struct i2c_xec_v3_data *data = (struct i2c_xec_v3_data *)work;
	const struct device *dev = data->dev;
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	uint32_t i2c_cfg = 0;
	bool run_sm = true;
	int state = I2C_XEC_ISR_STATE_CHK_ACK;
	int next_state = I2C_XEC_ISR_STATE_MAX;

	i2c_cfg = sys_read32(rb + XEC_I2C_CFG_OFS);
	data->i2c_compl = sys_read32(rb + XEC_I2C_CMPL_OFS);
	data->i2c_sr = sys_read8(rb + XEC_I2C_SR_OFS);
	if (((i2c_cfg & BIT(XEC_I2C_CFG_IDLE_IEN_POS)) != 0) &&
	    ((data->i2c_sr & BIT(XEC_I2C_SR_NBB_POS)) != 0)) {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
		state = I2C_XEC_ISR_STATE_EV_IDLE;
	}

#ifdef CONFIG_I2C_TARGET
	if ((data->i2c_sr & BIT(XEC_I2C_SR_STO_POS)) != 0) {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_STD_IEN_POS);
		state = I2C_XEC_ISR_STATE_TM_EV_STOP;
	}
#endif

	sys_write32(XEC_I2C_CMPL_RW1C_MSK, rb + XEC_I2C_CMPL_OFS);
	sys_write32(BIT(XEC_I2C_WKSR_SB_POS), rb + XEC_I2C_WKSR_OFS);
	soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

	/* Lost Arbitration or Bus Error? */
	if (i2c_xec_is_ber_lab(data)) {
		run_sm = false;
#ifdef CONFIG_I2C_TARGET
		tm_cleanup(data);
#endif
	}

	while (run_sm) {
		switch (state) {
		case I2C_XEC_ISR_STATE_GEN_START:
			if ((data->i2c_sr & BIT(XEC_I2C_SR_NBB_POS)) != 0) { /* START? */
				sys_write8(xfr->target_addr, rb + XEC_I2C_DATA_OFS);
				xec_i2c_cr_write(dev, XEC_I2C_CR_START_ENI);
			} else { /* RPT-START */
				xec_i2c_cr_write(dev, XEC_I2C_CR_RPT_START_ENI);
				sys_write8(xfr->target_addr, rb + XEC_I2C_DATA_OFS);
			}
			run_sm = false;
			break;
		case I2C_XEC_ISR_STATE_CHK_ACK:
			next_state = state_check_ack(data);
			break;
		case I2C_XEC_ISR_STATE_WR_DATA:
			next_state = state_data_wr(data);
			break;
		case I2C_XEC_ISR_STATE_RD_DATA:
			next_state = state_data_rd(data);
			break;
		case I2C_XEC_ISR_STATE_GEN_STOP:
			sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
			xec_i2c_cr_write(dev, XEC_I2C_CR_STOP);
			data->cm_dir = XEC_I2C_DIR_NONE;
			run_sm = false;
			break;
		case I2C_XEC_ISR_STATE_EV_IDLE:
			sys_set_bit(rb + XEC_I2C_CMPL_OFS, XEC_I2C_CMPL_IDLE_POS);
			data->cm_dir = XEC_I2C_DIR_NONE;
			next_state = I2C_XEC_ISR_STATE_NEXT_MSG;
			if (xfr->xfr_sts != 0) {
				data->mdone = 0x13;
				run_sm = false;
			}
#ifdef CONFIG_I2C_TARGET
			tm_cleanup(data);
#endif
			break;
		case I2C_XEC_ISR_STATE_NEXT_MSG:
			next_state = state_next_msg(data);
			break;
		case I2C_XEC_ISR_STATE_EXIT_1:
			data->mdone = 0;
			run_sm = false;
			break;
#ifdef CONFIG_I2C_TARGET
		case I2C_XEC_ISR_STATE_TM_HOST_RD:
			next_state = state_tm_host_read(data);
			break;
		case I2C_XEC_ISR_STATE_TM_HOST_WR:
			next_state = state_tm_host_write(data);
			break;
		case I2C_XEC_ISR_STATE_TM_EV_STOP:
			next_state = state_tm_stop_event(data);
			data->mdone = 0;
			run_sm = false;
			break;
#endif /* CONFIG_I2C_TARGET */
		default:
			sys_write32(XEC_I2C_CMPL_RW1C_MSK, rb + XEC_I2C_CMPL_OFS);
			soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 0);
			if (!data->mdone) {
				data->mdone = 0x66;
			}
			run_sm = false;
			break;
		}

		state = next_state;
	}

	/* ISR common exit path */
	soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

	if (data->mdone == 0) {
		soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 1u);
	} else {
		k_sem_give(&data->sync_sem);
	}
} /* xec_i2c_kwork_thread */

/* Controller Mode ISR
 * We need to disable interrupt before exiting ISR.
 */
static void i2c_xec_isr(const struct device *dev)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	struct i2c_xec_v3_data *data = dev->data;

	/* clears I2C controller's GIRQ enable causing GIRQ result
	 * signal to clear. GIRQ result is the input to the NVIC.
	 */
	soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 0);

	k_work_submit(&data->kworkq);
}

#ifdef CONFIG_PM_DEVICE
/* TODO Add logic to enable I2C wake if target mode is active.
 * For deep sleep this requires enabling GIRQ22 wake clocks feature.
 */
static int i2c_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2c_xec_v3_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;

	LOG_DBG("PM action: %d", (int)action);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_ENAB_POS);
		break;
	case PM_DEVICE_ACTION_RESUME:
		sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_ENAB_POS);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static int i2c_xec_init(const struct device *dev)
{
	const struct i2c_xec_v3_config *cfg = dev->config;
	struct i2c_xec_v3_data *data = dev->data;
	int rc = 0;
	uint32_t i2c_config = 0;

	data->dev = dev;
	data->state = XEC_I2C_STATE_CLOSED;
	data->i2c_compl = 0;
	data->i2c_cr_shadow = 0;
	data->i2c_sr = 0;
	data->mdone = 0;
	data->port_sel = cfg->port;

	rc = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc != 0) {
		LOG_ERR("pinctrl setup failed (%d)", rc);
		return rc;
	}

	i2c_config = i2c_map_dt_bitrate(cfg->clock_freq);
	if (i2c_config == 0) {
		return -EINVAL;
	}

	i2c_config |= I2C_MODE_CONTROLLER;

	/* Default configuration */
	rc = i2c_xec_configure(dev, i2c_config);
	if (rc != 0) {
		return rc;
	}

	k_work_init(&data->kworkq, &xec_i2c_kwork_thread);
	k_mutex_init(&data->lock_mut);
	k_sem_init(&data->sync_sem, 0, 1);

	if (cfg->irq_config_func) {
		cfg->irq_config_func();
	}

	return 0;
}

static DEVICE_API(i2c, i2c_xec_driver_api) = {
	.configure = i2c_xec_configure,
	.get_config = i2c_xec_get_config,
	.transfer = i2c_xec_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_xec_target_register,
	.target_unregister = i2c_xec_target_unregister,
#else
	.target_register = NULL,
	.target_unregister = NULL,
#endif
};

#define XEC_I2C_GIRQ_DT(inst)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, 0))
#define XEC_I2C_GIRQ_POS_DT(inst) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, 0))

#define I2C_XEC_DEVICE(i)                                                                          \
	PINCTRL_DT_INST_DEFINE(i);                                                                 \
	static void i2c_xec_irq_config_func_##i(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(i), DT_INST_IRQ(i, priority), i2c_xec_isr,                \
			    DEVICE_DT_INST_GET(i), 0);                                             \
		irq_enable(DT_INST_IRQN(i));                                                       \
	}                                                                                          \
	static struct i2c_xec_v3_data i2c_xec_v3_data_##i = {.port_sel =                           \
								     DT_INST_PROP(i, port_sel)};   \
	static const struct i2c_xec_v3_config i2c_xec_v3_cfg_##i = {                               \
		.base = (mem_addr_t)DT_INST_REG_ADDR(i),                                           \
		.clock_freq = DT_INST_PROP(i, clock_frequency),                                    \
		.sda_gpio = GPIO_DT_SPEC_INST_GET(i, sda_gpios),                                   \
		.scl_gpio = GPIO_DT_SPEC_INST_GET(i, scl_gpios),                                   \
		.irq_config_func = i2c_xec_irq_config_func_##i,                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),                                         \
		.girq = (uint8_t)XEC_I2C_GIRQ_DT(i),                                               \
		.girq_pos = (uint8_t)XEC_I2C_GIRQ_POS_DT(i),                                       \
		.enc_pcr = (uint8_t)DT_INST_PROP(i, pcr_scr),                                      \
		.port = (uint8_t)DT_INST_PROP(i, port_sel),                                        \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(i, i2c_xec_pm_action);                                            \
	I2C_DEVICE_DT_INST_DEFINE(i, i2c_xec_init, PM_DEVICE_DT_INST_GET(i), &i2c_xec_v3_data_##i, \
				  &i2c_xec_v3_cfg_##i, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,      \
				  &i2c_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_XEC_DEVICE)
