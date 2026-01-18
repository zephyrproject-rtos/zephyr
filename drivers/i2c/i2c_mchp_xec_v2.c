/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Microchip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_i2c_v2

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
#include "zephyr/sys/slist.h"
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(i2c_xec_v3, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"        /* dependency on logging */
#include "i2c_mchp_xec_v2.h" /* register defines */

/* Microchip MEC I2C controller using byte mode.
 * This driver is for HW version 3.7 and above.
 */

/* #define XEC_I2C_DEBUG_USE_SPIN_LOOP */
/* #define XEC_I2C_DEBUG_ISR */
/* #define XEC_I2C_DEBUG_STATE */
/* #define XEC_I2C_DEBUG_STATE_ENTRIES 256 */

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

struct i2c_xec_config {
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
#define I2c_MEC5_XFR_STS_BER 0x02
#define I2c_MEC5_XFR_STS_LAB 0x04

struct i2c_xec_cm_xfr {
	volatile uint8_t *mbuf;
	volatile size_t mlen;
	volatile uint8_t xfr_sts;
	uint8_t mdir;
	uint8_t target_addr;
	uint8_t mflags;
};

struct i2c_xec_data {
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
	uint8_t xfr_state;
	uint8_t cm_dir;
	uint8_t tm_dir;
	uint8_t read_discard;
	uint8_t msg_idx;
	uint8_t num_msgs;
	struct i2c_msg *msgs;
	struct i2c_xec_cm_xfr cm_xfr;
	volatile uint8_t mdone;
#ifdef CONFIG_I2C_TARGET
	uint16_t targ_addr;
	uint8_t targ_data;
	uint8_t targ_ignore;
	uint8_t targ_active;
	uint8_t ntargets;
	sys_slist_t target_list;
	struct i2c_target_config *curr_target;
	uint8_t *targ_buf_ptr;
	uint32_t targ_buf_len;
#endif
#ifdef XEC_I2C_DEBUG_STATE
	volatile uint32_t dbg_state_idx;
	uint8_t dbg_states[XEC_I2C_DEBUG_STATE_ENTRIES];
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

#ifdef XEC_I2C_DEBUG_ISR
volatile uint32_t i2c_xec_isr_cnt;
volatile uint32_t i2c_xec_isr_sts;
volatile uint32_t i2c_xec_isr_compl;
volatile uint32_t i2c_xec_isr_cfg;

static inline void i2c_xec_dbg_isr_init(void)
{
	i2c_xec_isr_cnt = 0;
}
#define XEC_I2C_DEBUG_ISR_INIT() i2c_xec_dbg_isr_init()
#else
#define XEC_I2C_DEBUG_ISR_INIT()
#endif

#ifdef XEC_I2C_DEBUG_STATE
static void xec_i2c_dbg_state_init(struct i2c_xec_data *data)
{
	data->dbg_state_idx = 0u;
	memset((void *)data->dbg_states, 0, sizeof(data->dbg_states));
}

static void xec_i2c_dbg_state_update(struct i2c_xec_data *data, uint8_t state)
{
	uint32_t idx = data->dbg_state_idx;

	if (data->dbg_state_idx < XEC_I2C_DEBUG_STATE_ENTRIES) {
		data->dbg_states[idx] = state;
		data->dbg_state_idx = ++idx;
	}
}
#define XEC_I2C_DEBUG_STATE_INIT(pd)          xec_i2c_dbg_state_init(pd)
#define XEC_I2C_DEBUG_STATE_UPDATE(pd, state) xec_i2c_dbg_state_update(pd, state)
#else
#define XEC_I2C_DEBUG_STATE_INIT(pd)
#define XEC_I2C_DEBUG_STATE_UPDATE(pd, state)
#endif

static int xec_i2c_prog_standard_timing(const struct device *dev, uint32_t freq_hz)
{
	const struct i2c_xec_config *devcfg = dev->config;
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
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;

	data->i2c_cr_shadow = ctrl_val;
	sys_write8(ctrl_val, devcfg->base + XEC_I2C_CR_OFS);
}

#ifdef CONFIG_I2C_TARGET
static void xec_i2c_cr_write_mask(const struct device *dev, uint8_t clr_msk, uint8_t set_msk)
{
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;

	data->i2c_cr_shadow = (data->i2c_cr_shadow & (uint8_t)~clr_msk) | set_msk;
	sys_write8(data->i2c_cr_shadow, devcfg->base + XEC_I2C_CR_OFS);
}
#endif

static int wait_bus_free(const struct device *dev, uint32_t nwait)
{
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
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
	const struct i2c_xec_config *devcfg = dev->config;
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
	const struct i2c_xec_config *devcfg = dev->config;
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
	const struct i2c_xec_config *devcfg = dev->config;
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
	const struct i2c_xec_config *devcfg = dev->config;
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
static uint32_t prog_target_addresses(const struct device *dev)
{
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	sys_snode_t *sn = NULL;
	uint32_t val = 0, n = 0;

	SYS_SLIST_FOR_EACH_NODE(&data->target_list, sn) {
		struct i2c_target_config *ptc = CONTAINER_OF(sn, struct i2c_target_config, node);

		if (ptc != NULL) {
			n++;
			if (ptc->address == 0) {
				sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_GC_DIS_POS);
			} else if ((ptc->address == 0x08u) || (ptc->address == 0x61u)) {
				sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_DSA_POS);
			} else {
				if (val == 0) {
					val |= XEC_I2C_OA_1_SET(ptc->address);
				} else {
					val |= XEC_I2C_OA_2_SET(ptc->address);
				}
			}
		}

		if (val != 0) {
			sys_write32(val, rb + XEC_I2C_OA_OFS);
		}
	}

	return n;
}
#endif /* CONFIG_I2C_TARGET */

static int i2c_xec_reset_config(const struct device *dev, uint8_t port)
{
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	uint32_t val = 0;
	int rc = 0;
	uint8_t crval = 0;

	data->i2c_cr_shadow = 0;

	data->state = XEC_I2C_STATE_CLOSED;
	data->i2c_cr_shadow = 0;
	data->i2c_sr = 0;
	data->i2c_compl = 0;
	data->read_discard = 0;
	data->mdone = 0;

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
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	int ret = 0;
	uint32_t cnt = I2C_XEC_RECOVER_SCL_LOW_RETRIES;
	uint8_t bbcr = 0;
	uint8_t lines = 0u;

	i2c_xec_reset_config(dev, data->port_sel);

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
		bbcr = 0x81u; /* SCL & SDA tri-state (inputs) */
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);

		/* 9 clocks */
		for (int j = 0; j < 9; j++) {
			/* drive SCL low */
			bbcr = 0x83u; /* SCL output drive low, SDA tri-state input */
			sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
			k_busy_wait(I2C_XEC_RECOVER_BB_DELAY_US);
			/* drive SCL high */
			bbcr = 0x81u; /* SCL & SDA tri-state inputs */
			sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
			k_busy_wait(I2C_XEC_RECOVER_BB_DELAY_US);
		}

		lines = get_lines(dev);
		if ((lines & XEC_I2C_LINES_MSK) == XEC_I2C_LINES_MSK) { /* Both high? */
			ret = 0;
			goto disable_bb_exit;
		}

		/* generate I2C STOP.  While SCL is high SDA transitions low to high */
		bbcr = 0x85u; /* SCL tri-state input (high), drive SDA low */
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
		k_busy_wait(I2C_XEC_RECOVER_BB_DELAY_US);
		bbcr = 0x81u; /* SCL and SDA tri-state inputs. */
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
		k_busy_wait(I2C_XEC_RECOVER_BB_DELAY_US);

		lines = get_lines(dev);
		if ((lines & XEC_I2C_LINES_MSK) == XEC_I2C_LINES_MSK) { /* Both high? */
			ret = 0;
			goto disable_bb_exit;
		}
	}

disable_bb_exit:
	bbcr = 0x80u;
	sys_write8(0x80u, rb + XEC_I2C_BBCR_OFS);

	return ret;
}

static int i2c_xec_recover_bus(const struct device *dev)
{
	struct i2c_xec_data *const data = dev->data;
	int ret = 0;

	LOG_ERR("I2C attempt bus recovery\n");

	/* Try controller reset first */
	ret = i2c_xec_reset_config(dev, data->port_sel);
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

static int i2c_xec_cfg(const struct device *dev, uint32_t dev_config_raw)
{
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	uint8_t port = devcfg->port;

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

	data->i2c_config = dev_config_raw;
#ifdef CONFIG_I2C_XEC_PORT_MUX
	port = I2C_XEC_PORT_GET(dev_config_raw);
#endif

	return i2c_xec_reset_config(dev, port);
}

/* i2c_configure API */
static int i2c_xec_configure(const struct device *dev, uint32_t dev_config_raw)
{
	struct i2c_xec_data *const data = dev->data;
	int rc = 0;

	if (!(dev_config_raw & I2C_MODE_CONTROLLER)) {
		return -ENOTSUP;
	}

	rc = k_mutex_lock(&data->lock_mut, K_NO_WAIT);
	if (rc != 0) {
		return rc;
	}

	rc = i2c_xec_cfg(dev, dev_config_raw);

	k_mutex_unlock(&data->lock_mut);

	return rc;
}

/* i2c_get_config API */
static int i2c_xec_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_xec_data *const data = dev->data;
	uint32_t dcfg = 0u;

	if (dev_config == NULL) {
		return -EINVAL;
	}

	dcfg = data->i2c_config;

#ifdef CONFIG_I2C_TARGET
	if (data->ntargets == 0) {
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
	uint8_t fmt_addr = (uint8_t)((addr & 0x7fu) << 1);

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
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	int rc = 0;
	uint8_t ctrl = 0, sts = 0;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x20);

	/* Is the bus busy? */
	sts = sys_read8(rb + XEC_I2C_SR_OFS);
	if ((sts & BIT(XEC_I2C_SR_NBB_POS)) == 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x21);
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
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x22);
			sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
			rc = k_sem_take(&data->sync_sem, K_MSEC(10));
		} else {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x23);
			rc = wait_bus_free(dev, WAIT_COUNT);
		}

		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x24);
	}

	data->cm_dir = XEC_I2C_DIR_NONE;
	data->state = XEC_I2C_STATE_CLOSED;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x25);

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
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	struct i2c_msg *m = data->msgs;
	mem_addr_t rb = devcfg->base;
	int rc = 0;
	uint8_t target_addr = 0;
	uint8_t ctrl = XEC_I2C_CR_START_ENI;
	uint8_t start = XEC_I2C_START_NORM;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x10);

	target_addr = i2c_xec_fmt_addr(addr, 0);
	data->wraddr = target_addr;

	if ((data->msgs[0].flags & I2C_MSG_READ) != 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x11);
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

	if ((sys_read8(rb + XEC_I2C_SR_OFS) & BIT(XEC_I2C_SR_NBB_POS)) == 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x12);
		if ((data->cm_dir != xfr->mdir) || (m->flags & I2C_MSG_RESTART)) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x13);
			start = XEC_I2C_START_RPT;
			ctrl = XEC_I2C_CR_RPT_START_ENI;
		}
	}

	data->cm_dir = xfr->mdir;
	if (m->flags & I2C_MSG_STOP) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x14);
		xfr->mflags |= I2C_XEC_XFR_FLAG_STOP_REQ;
	}

	soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 0);
	soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x15);

	/* Generate (RPT)-START and transmit address for write or read */
	if (ctrl == XEC_I2C_CR_START_ENI) { /* START? */
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x16);
		sys_write8(target_addr, rb + XEC_I2C_DATA_OFS);
		xec_i2c_cr_write(dev, ctrl);
	} else { /* RPT-START */
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x17);
		xec_i2c_cr_write(dev, ctrl);
		sys_write8(target_addr, rb + XEC_I2C_DATA_OFS);
	}

	soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 1u);
	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x18);

#ifdef XEC_I2C_DEBUG_USE_SPIN_LOOP
	while (!data->mdone) {
		;
	}
#else
	rc = k_sem_take(&data->sync_sem, K_MSEC(100));
	if (rc != 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x19);
		return -ETIMEDOUT;
	}
#endif
	if (xfr->xfr_sts) { /* error */
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x1A);
		return -EIO;
	}

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x1B);

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
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	mem_addr_t rb = devcfg->base;
	int rc = 0;

	k_mutex_lock(&data->lock_mut, K_FOREVER);
#ifdef CONFIG_I2C_TARGET
	if (data->ntargets != 0) {
		k_mutex_unlock(&data->lock_mut);
		return -EBUSY;
	}
#endif
	pm_device_busy_set(dev);
	k_sem_reset(&data->sync_sem);

	XEC_I2C_DEBUG_ISR_INIT();

	memset(&data->cm_xfr, 0, sizeof(struct i2c_xec_cm_xfr));

	rc = check_msgs(msgs, num_msgs);
	if (rc != 0) {
		goto xec_unlock;
	}

	if (data->state != XEC_I2C_STATE_OPEN) {
		XEC_I2C_DEBUG_STATE_INIT(data);

		rc = check_lines(dev);
		data->i2c_sr = sys_read8(rb + XEC_I2C_SR_OFS);
		data->i2c_compl = sys_read32(rb + XEC_I2C_CMPL_OFS);

		if (rc || (data->i2c_sr & BIT(XEC_I2C_SR_BER_POS))) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x50);
			rc = i2c_xec_recover_bus(dev);
		}
	}

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x1);

	if (rc) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x2);
		data->state = XEC_I2C_STATE_CLOSED;
		goto xec_unlock;
	}

	data->state = XEC_I2C_STATE_OPEN;

	data->msg_idx = 0;
	data->num_msgs = num_msgs;
	data->msgs = msgs;

	rc = i2c_xec_xfr_begin(dev, addr);
	if (rc) { /* if error issue STOP if bus is still owned by controller */
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x7);
		i2c_xec_stop(dev, 0);
	}

xec_unlock:
	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x8);

	if ((sys_read8(rb + XEC_I2C_SR_OFS) & BIT(XEC_I2C_SR_NBB_POS)) == 0) {
		data->cm_dir = XEC_I2C_DIR_NONE;
		data->state = XEC_I2C_STATE_CLOSED;
	}

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->lock_mut);

	return rc;
}

#ifdef CONFIG_I2C_TARGET
static struct i2c_target_config *find_target(struct i2c_xec_data *data, uint16_t i2c_addr)
{
	sys_snode_t *sn = NULL;

	SYS_SLIST_FOR_EACH_NODE(&data->target_list, sn) {
		struct i2c_target_config *ptc = CONTAINER_OF(sn, struct i2c_target_config, node);

		if ((ptc != NULL) && (ptc->address == i2c_addr)) {
			return ptc;
		}
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
static int i2c_xec_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *data = dev->data;
	mem_addr_t rb = devcfg->base;
	uint32_t oaval = 0;
	int rc = 0;

	if (cfg == NULL) {
		return -EINVAL;
	}

	if (((cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) != 0) || (cfg->address > 0x7FU)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock_mut, K_FOREVER);

	rc = -ENFILE;
	if (data->ntargets < 5) {
		struct i2c_target_config *ptc = find_target(data, cfg->address);

		if (ptc == NULL) {
			data->ntargets++;
			sys_slist_append(&data->target_list, &cfg->node);
			if (cfg->address == XEC_I2C_GEN_CALL_ADDR) {
				/* enable general call */
				sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_GC_DIS_POS);
			} else if ((cfg->address == XEC_I2C_SMB_HOST_ADDR) ||
				   (cfg->address == XEC_I2C_SMB_DEVICE_ADDR)) {
				/* enable DSA */
				sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_DSA_POS);
			} else { /* use one of the two own addresses */
				oaval = sys_read32(rb + XEC_I2C_OA_OFS);
				if (XEC_I2C_OA_1_GET(oaval) == 0) {
					oaval |= XEC_I2C_OA_1_SET(cfg->address);
					sys_write32(oaval, rb + XEC_I2C_OA_OFS);
				} else if (XEC_I2C_OA_2_GET(oaval) == 0) {
					oaval |= XEC_I2C_OA_2_SET(cfg->address);
					sys_write32(oaval, rb + XEC_I2C_OA_OFS);
				}
			}
			rc = 0;
		}
	}

	if (rc == 0) {
		soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 1u);
	}

	k_mutex_unlock(&data->lock_mut);

	return 0;
}

static int i2c_xec_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *data = dev->data;
	mem_addr_t rb = devcfg->base;
	uint32_t oaval = 0;
	uint16_t taddr1 = 0, taddr2 = 0;
	int rc = 0;
	bool removed = false;

	if (cfg == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock_mut, K_FOREVER);

	if (data->ntargets == 0) {
		goto targ_unreg_release_lock;
	}

	removed = sys_slist_find_and_remove(&data->target_list, &cfg->node);

	if (removed == false) {
		rc = -ENOSYS;
		goto targ_unreg_release_lock;
	}

	data->ntargets--;

	if (cfg->address == XEC_I2C_GEN_CALL_ADDR) { /* disable general call */
		sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_GC_DIS_POS);
	} else if ((cfg->address == XEC_I2C_SMB_HOST_ADDR) ||
		   (cfg->address == XEC_I2C_SMB_DEVICE_ADDR)) {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_DSA_POS);
	} else { /* one of the own addresses */
		oaval = sys_read32(rb + XEC_I2C_OA_OFS);
		taddr1 = XEC_I2C_OA_1_GET(oaval);
		taddr2 = XEC_I2C_OA_2_GET(oaval);
		if (taddr1 == (uint32_t)cfg->address) {
			oaval &= ~XEC_I2C_OA_1_MSK;
			sys_write32(oaval, rb + XEC_I2C_OA_OFS);
		} else if (taddr2 == (uint32_t)cfg->address) {
			oaval &= ~XEC_I2C_OA_2_MSK;
			sys_write32(oaval, rb + XEC_I2C_OA_OFS);
		}
	}

targ_unreg_release_lock:
	k_mutex_unlock(&data->lock_mut);

	return rc;
}
#endif /* CONFIG_I2C_TARGET */

/* ISR helpers and state handlers */
static int i2c_xec_is_ber_lab(struct i2c_xec_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;

	if ((data->i2c_sr & (BIT(XEC_I2C_SR_BER_POS) | BIT(XEC_I2C_SR_LAB_POS))) != 0) {
		if (data->i2c_sr & BIT(XEC_I2C_SR_BER_POS)) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x82);
			xfr->xfr_sts |= XEC_I2C_ERR_BUS;
		} else {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x83);
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

static int i2c_xec_next_msg(struct i2c_xec_data *data)
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
static int state_check_ack_tm(struct i2c_xec_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;
	int next_state = I2C_XEC_ISR_STATE_MAX;
	int rc = 0;
	uint16_t i2c_addr = 0;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC0);

	if ((data->i2c_sr & BIT(XEC_I2C_SR_AAT_POS)) != 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC1);
		/* enable STOP detect and IDLE interrupts */
		sys_set_bit(rb + XEC_I2C_CMPL_OFS, XEC_I2C_CMPL_IDLE_POS);
		sys_set_bits(rb + XEC_I2C_CFG_OFS,
			     (BIT(XEC_I2C_CFG_IDLE_IEN_POS) | BIT(XEC_I2C_CFG_STD_IEN_POS)));

		data->targ_active = 1u;
		data->targ_ignore = 1u;
		data->targ_data = XEC_I2C_TM_HOST_READ_IGNORE_VAL;
		data->targ_addr = sys_read8(rb + XEC_I2C_IAS_OFS);
		/* extract I2C address from bus value */
		i2c_addr = (data->targ_addr >> 1); /* bits[7:1]=address, bit[0]=R/nW */
		data->curr_target = find_target(data, i2c_addr);

		const struct i2c_target_callbacks *tcbs = data->curr_target->callbacks;

		if ((data->targ_addr & BIT(0)) != 0) { /* Host requesting read from target */
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC2);
			if ((tcbs != NULL) && (tcbs->read_requested != NULL)) {
				rc = tcbs->read_requested(data->curr_target, &data->targ_data);
				if (rc == 0) {
					XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC3);
					data->targ_ignore = 0;
				}
			}

			/* read & discard target address clears I2C.SR.AAT */
			sys_read8(rb + XEC_I2C_DATA_OFS);
			/* as target transmitter writing I2C.DATA releases clock stretching */
			sys_write8(data->targ_data, rb + XEC_I2C_DATA_OFS);
		} else { /* Host requesting write to target */
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC4);
			if ((tcbs != NULL) && (tcbs->write_requested != NULL)) {
				rc = tcbs->write_requested(data->curr_target);
				if (rc == 0) {
					XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC5);
					data->targ_ignore = 0u;
				}
			}

			if (data->targ_ignore != 0) {
				xec_i2c_cr_write_mask(dev, BIT(XEC_I2C_CR_ACK_POS), 0);
			}

			/* as target receiver reading I2C.DATA releases clock stretching
			 * and clears I2C.SR.AAT.
			 */
			sys_read8(rb + XEC_I2C_DATA_OFS);
		}

		return I2C_XEC_ISR_STATE_EXIT_1;
	}

	if (data->targ_active != 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC6);
		next_state = I2C_XEC_ISR_STATE_TM_HOST_WR;
		if ((data->targ_addr & BIT(0)) != 0) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC7);
			next_state = I2C_XEC_ISR_STATE_TM_HOST_RD;
		}
	}

	return next_state;
}
#endif

static int state_check_ack(struct i2c_xec_data *data)
{
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	int next_state = I2C_XEC_ISR_STATE_MAX;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x83);

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
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x84);
		next_state = I2C_XEC_ISR_STATE_GEN_STOP;
		xfr->xfr_sts |= I2C_XEC_XFR_STS_NACK;
	}

	return next_state;
}

static int state_data_wr(struct i2c_xec_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	mem_addr_t rb = devcfg->base;
	int next_state = I2C_XEC_ISR_STATE_EXIT_1;
	uint8_t msgbyte;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x90);

	if (xfr->mlen > 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x91);
		msgbyte = *xfr->mbuf;

		sys_write8(msgbyte, rb + XEC_I2C_DATA_OFS);

		xfr->mbuf++;
		xfr->mlen--;
	} else {
		if (xfr->mflags & I2C_XEC_XFR_FLAG_STOP_REQ) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x92);
			next_state = I2C_XEC_ISR_STATE_GEN_STOP;
		} else {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x93);
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
static int state_data_rd(struct i2c_xec_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	mem_addr_t rb = devcfg->base;
	int next_state = I2C_XEC_ISR_STATE_NEXT_MSG;
	uint8_t ctrl = 0;
	uint8_t msgbyte = 0;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0xa0);

	if (xfr->mlen > 0) {
		next_state = I2C_XEC_ISR_STATE_EXIT_1;
		if ((xfr->mflags & I2C_XEC_XFR_FLAG_START_REQ) != 0) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xa1);
			/* HW clocks in address it transmits. Read and discard.
			 * HW generates clocks for first data byte.
			 */
			xfr->mflags &= ~(I2C_XEC_XFR_FLAG_START_REQ);
			if ((xfr->mlen == 1) && ((xfr->mflags & I2C_XEC_XFR_FLAG_STOP_REQ) != 0)) {
				XEC_I2C_DEBUG_STATE_UPDATE(data, 0xa2);
				/* disable auto-ACK and make sure ENI=1 */
				ctrl = BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ENI_POS);
				xec_i2c_cr_write(dev, ctrl);
			}
			/* read byte currently in HW buffer and generate clocks for next byte */
			msgbyte = sys_read8(rb + XEC_I2C_DATA_OFS);
		} else if ((xfr->mflags & I2C_XEC_XFR_FLAG_STOP_REQ) != 0) {
			if (xfr->mlen != 1) {
				XEC_I2C_DEBUG_STATE_UPDATE(data, 0xa3);
				if (xfr->mlen == 2) {
					XEC_I2C_DEBUG_STATE_UPDATE(data, 0xa4);
					ctrl = BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ENI_POS);
					xec_i2c_cr_write(dev, ctrl);
				}

				msgbyte = sys_read8(rb + XEC_I2C_DATA_OFS);

				*xfr->mbuf = msgbyte;
				xfr->mbuf++;
				xfr->mlen--;
			} else { /* Begin STOP generation and read last byte */
				XEC_I2C_DEBUG_STATE_UPDATE(data, 0xa5);
				xfr->mflags &= ~(I2C_XEC_XFR_FLAG_STOP_REQ);

				sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
				xec_i2c_cr_write(dev, XEC_I2C_CR_STOP);
				/* read triggers STOP generation */
				msgbyte = sys_read8(rb + XEC_I2C_DATA_OFS);

				*xfr->mbuf = msgbyte;
				xfr->mlen = 0;
			}
		} else { /* No START or STOP flags */
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xa6);
			msgbyte = sys_read8(rb + XEC_I2C_DATA_OFS);

			*xfr->mbuf = msgbyte;
			xfr->mbuf++;
			xfr->mlen--;
		}
	}

	return next_state;
}

static int state_next_msg(struct i2c_xec_data *data)
{
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	int next_state = I2C_XEC_ISR_STATE_MAX;
	int ret = i2c_xec_next_msg(data);

	if (ret) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0xb0);
		if (xfr->mflags & I2C_XEC_XFR_FLAG_START_REQ) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xb1);
			next_state = I2C_XEC_ISR_STATE_GEN_START;
		} else {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xb2);
			next_state = I2C_XEC_ISR_STATE_WR_DATA;
			if (xfr->mdir == XEC_I2C_DIR_RD) {
				XEC_I2C_DEBUG_STATE_UPDATE(data, 0xb3);
				next_state = I2C_XEC_ISR_STATE_RD_DATA;
			}
		}
	} else { /* no more messages */
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0xb3);
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
static int state_tm_host_read(struct i2c_xec_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_target_config *tcfg = data->curr_target;
	const struct i2c_target_callbacks *tcbs = tcfg->callbacks;
	mem_addr_t rb = devcfg->base;
	int rc = 0;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC8);

	if ((tcbs == NULL) || (tcbs->read_processed == NULL)) {
		data->targ_ignore = 1u;
	}

	if (data->targ_ignore == 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0xC9);
		rc = tcbs->read_processed(tcfg, &data->targ_data);
		if (rc != 0) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xCA);
			data->targ_ignore = 1;
			data->targ_data = XEC_I2C_TM_HOST_READ_IGNORE_VAL;
		}
	}

	sys_write8(data->targ_data, rb + XEC_I2C_DATA_OFS);

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
static int state_tm_host_write(struct i2c_xec_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_target_config *tcfg = data->curr_target;
	const struct i2c_target_callbacks *tcbs = tcfg->callbacks;
	mem_addr_t rb = devcfg->base;
	int rc = 0;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0xCB);

	/* read shadow data register. No side-effects */
	data->targ_data = sys_read8(rb + XEC_I2C_IDS_OFS);

	if (data->targ_ignore == 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0xCC);
		rc = tcbs->write_received(tcfg, data->targ_data);
		if (rc != 0) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xCD);
			data->targ_ignore = 1u;
			/* clear HW auto-ACK. We NAK future received bytes */
			xec_i2c_cr_write_mask(dev, BIT(XEC_I2C_CR_ACK_POS), 0);
		}
	}

	/* must read I2C.DATA to release SCL */
	sys_read8(rb + XEC_I2C_DATA_OFS);

	return I2C_XEC_ISR_STATE_EXIT_1;
}

static int state_tm_stop_event(struct i2c_xec_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_target_config *tcfg = data->curr_target;
	const struct i2c_target_callbacks *tcbs = tcfg->callbacks;
	mem_addr_t rb = devcfg->base;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0xE4);

	if (tcbs != NULL) {
		if (tcbs->stop != NULL) {
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0xE5);
			tcbs->stop(tcfg);
		}
	}

	/* Race condition:
	 * Docs state: "After the function(stop callback) returns the controller
	 * shall enter a state where it is ready to react to new start conditions"
	 *
	 */
	data->targ_active = 0;
	data->targ_ignore = 0;
	data->curr_target = NULL;
	/* HW requires a read and discard of I2C.DATA register to clear the read-only
	 * STOP detect status in I2C.SR.
	 */
	sys_read8(rb + XEC_I2C_DATA_OFS);
	xec_i2c_cr_write(dev, XEC_I2C_CR_PIN_ESO_ENI_ACK);

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0xE6);

	return I2C_XEC_ISR_STATE_EXIT_1;
}

static void tm_cleanup(struct i2c_xec_data *data)
{
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0xE8);

	data->targ_active = 0;
	data->targ_ignore = 0;
	data->curr_target = NULL;

	sys_read8(rb + XEC_I2C_DATA_OFS);
	/* re-arm I2C to detect external Host activity */
	xec_i2c_cr_write(dev, XEC_I2C_CR_PIN_ESO_ENI_ACK);
}
#endif /* CONFIG_I2C_TARGET */

static void xec_i2c_kwork_thread(struct k_work *work)
{
	struct i2c_xec_data *data = CONTAINER_OF(work, struct i2c_xec_data, kworkq);
	const struct device *dev = data->dev;
	const struct i2c_xec_config *devcfg = dev->config;
	mem_addr_t rb = devcfg->base;
	struct i2c_xec_cm_xfr *xfr = &data->cm_xfr;
	uint32_t i2c_cfg = 0;
	bool run_sm = true;
	int state = I2C_XEC_ISR_STATE_CHK_ACK;
	int next_state = I2C_XEC_ISR_STATE_MAX;
	int idle_active = 0;

	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x80);

#ifdef XEC_I2C_DEBUG_ISR
	i2c_xec_isr_cnt++;
	i2c_xec_isr_sts = sys_read8(rb + XEC_I2C_SR_OFS);
	i2c_xec_isr_compl = sys_read32(rb + XEC_I2C_CMPL_OFS);
	i2c_xec_isr_cfg = sys_read32(rb + XEC_I2C_CFG_OFS);
	while (data->mdone) { /* should not hang here */
		;
	}
#endif
	i2c_cfg = sys_read32(rb + XEC_I2C_CFG_OFS);
	data->i2c_compl = sys_read32(rb + XEC_I2C_CMPL_OFS);
	data->i2c_sr = sys_read8(rb + XEC_I2C_SR_OFS);
	if (((i2c_cfg & BIT(XEC_I2C_CFG_IDLE_IEN_POS)) != 0) &&
	    ((data->i2c_sr & BIT(XEC_I2C_SR_NBB_POS)) != 0)) {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
		idle_active = 1;
		state = I2C_XEC_ISR_STATE_EV_IDLE;
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0xE1);
	}

#ifdef CONFIG_I2C_TARGET
	if ((data->i2c_sr & BIT(XEC_I2C_SR_STO_POS)) != 0) {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_STD_IEN_POS);
		state = I2C_XEC_ISR_STATE_TM_EV_STOP;
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0xE0);
	}
#endif

	sys_write32(XEC_I2C_CMPL_RW1C_MSK, rb + XEC_I2C_CMPL_OFS);
	sys_write32(BIT(XEC_I2C_WKSR_SB_POS), rb + XEC_I2C_WKSR_OFS);
	soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

	/* Lost Arbitration or Bus Error? */
	if (i2c_xec_is_ber_lab(data)) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x81);
		run_sm = false;
#ifdef CONFIG_I2C_TARGET
		tm_cleanup(data);
#endif
	}

	while (run_sm) {
		switch (state) {
		case I2C_XEC_ISR_STATE_GEN_START:
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x82);
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
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x85);
			sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
			xec_i2c_cr_write(dev, XEC_I2C_CR_STOP);
			data->cm_dir = XEC_I2C_DIR_NONE;
			run_sm = false;
			break;
		case I2C_XEC_ISR_STATE_EV_IDLE:
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x87);
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
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x88);
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
			XEC_I2C_DEBUG_STATE_UPDATE(data, 0x89);
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
	XEC_I2C_DEBUG_STATE_UPDATE(data, 0x8d);
	soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

	if (data->mdone == 0) {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x8e);
		soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 1u);
	} else {
		XEC_I2C_DEBUG_STATE_UPDATE(data, 0x8f);
		k_sem_give(&data->sync_sem);
	}
} /* xec_i2c_kwork_thread */

/* Controller Mode ISR
 * We need to disable interrupt before exiting ISR.
 */
static void i2c_xec_isr(const struct device *dev)
{
	const struct i2c_xec_config *devcfg = dev->config;
	struct i2c_xec_data *data = dev->data;

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
	const struct i2c_xec_config *devcfg = dev->config;
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

static int i2c_mchp_xec_v2_debug_init(const struct device *dev)
{
#ifdef XEC_I2C_DEBUG_STATE
	struct i2c_xec_data *data = dev->data;

	XEC_I2C_DEBUG_STATE_INIT(data);
#endif
#ifdef XEC_I2C_DEBUG_ISR
	XEC_I2C_DEBUG_ISR_INIT();
#endif

	return 0;
}

static int i2c_xec_init(const struct device *dev)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data = dev->data;
	int rc = 0;
	uint32_t i2c_config = 0;

	i2c_mchp_xec_v2_debug_init(dev);

	data->dev = dev;
	data->state = XEC_I2C_STATE_CLOSED;
	data->i2c_compl = 0;
	data->i2c_cr_shadow = 0;
	data->i2c_sr = 0;
	data->mdone = 0;

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
#ifdef CONFIG_I2C_XEC_PORT_MUX
	i2c_config |= I2C_XEC_PORT_SET((uint32_t)cfg->port);
#endif
	/* Default configuration */
	rc = i2c_xec_configure(dev, i2c_config);
	if (rc != 0) {
		return rc;
	}

#ifdef CONFIG_I2C_TARGET
	sys_slist_init(&data->target_list);
#endif
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
	static struct i2c_xec_data i2c_xec_data_##i = {.port_sel = DT_INST_PROP(i, port_sel)};     \
	static const struct i2c_xec_config i2c_xec_config_##i = {                                  \
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
	I2C_DEVICE_DT_INST_DEFINE(i, i2c_xec_init, PM_DEVICE_DT_INST_GET(i), &i2c_xec_data_##i,    \
				  &i2c_xec_config_##i, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,      \
				  &i2c_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_XEC_DEVICE)
