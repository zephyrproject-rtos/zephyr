/*
 * Copyright (c) 2024 Microchip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_i2c

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_mchp, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_ecia_api.h>
#include <mec_i2c_api.h>

/* #define MEC5_I2C_DEBUG_USE_SPIN_LOOP */
/* #define MEC5_I2C_DEBUG_ISR */
/* #define MEC5_I2C_DEBUG_STATE */
#define MEC5_I2C_DEBUG_STATE_ENTRIES 64

#define RESET_WAIT_US		20

/* I2C timeout is  10 ms (WAIT_INTERVAL * WAIT_COUNT) */
#define WAIT_INTERVAL		50
#define WAIT_COUNT		200
#define STOP_WAIT_COUNT		500
#define PIN_CFG_WAIT		50

/* I2C recover SCL low retries */
#define I2C_MEC5_RECOVER_SCL_LOW_RETRIES	10
/* I2C recover SDA low retries */
#define I2C_MEC5_RECOVER_SDA_LOW_RETRIES	3
/* I2C recovery bit bang delay */
#define I2C_MEC5_RECOVER_BB_DELAY_US		5
/* I2C recovery SCL sample delay */
#define I2C_MEC5_RECOVER_SCL_DELAY_US		50

#define I2C_MEC5_CTRL_WR_DLY	8

enum mec5_i2c_state {
	MEC5_I2C_STATE_CLOSED = 0,
	MEC5_I2C_STATE_OPEN,
};

enum mec5_i2c_error {
	MEC5_I2C_ERR_NONE = 0,
	MEC5_I2C_ERR_BUS,
	MEC5_I2C_ERR_LOST_ARB,
	MEC5_I2C_ERR_TIMEOUT,
};

enum mec5_i2c_direction {
	MEC5_I2C_DIR_NONE = 0,
	MEC5_I2C_DIR_WR,
	MEC5_I2C_DIR_RD,
};

enum mec5_i2c_start {
	MEC5_I2C_START_NONE = 0,
	MEC5_I2C_START_NORM,
	MEC5_I2C_START_RPT,
};

enum i2c_mec5_isr_state {
	I2C_MEC5_ISR_STATE_GEN_START = 0,
	I2C_MEC5_ISR_STATE_CHK_ACK,
	I2C_MEC5_ISR_STATE_WR_DATA,
	I2C_MEC5_ISR_STATE_RD_DATA,
	I2C_MEC5_ISR_STATE_GEN_STOP,
	I2C_MEC5_ISR_STATE_EV_IDLE,
	I2C_MEC5_ISR_STATE_NEXT_MSG,
	I2C_MEC5_ISR_STATE_EXIT_1,
	I2C_MEC5_ISR_STATE_MAX
};

struct i2c_mec5_config {
	struct i2c_smb_regs *base;
	uint32_t clock_freq;
	uint8_t port_sel;
	struct gpio_dt_spec sda_gpio;
	struct gpio_dt_spec scl_gpio;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

#define I2C_MEC5_XFR_FLAG_START_REQ 0x01
#define I2C_MEC5_XFR_FLAG_STOP_REQ 0x02

#define I2C_MEC5_XFR_STS_NACK	    0x01
#define I2c_MEC5_XFR_STS_BER	    0x02
#define I2c_MEC5_XFR_STS_LAB	    0x04

struct i2c_mec5_cm_xfr {
	volatile uint8_t *mbuf;
	volatile size_t mlen;
	volatile uint8_t xfr_sts;
	uint8_t mdir;
	uint8_t target_addr;
	uint8_t mflags;
};

struct i2c_mec5_data {
	struct mec_i2c_smb_ctx ctx;
	struct k_sem lock;
	struct k_sem sync;
	uint32_t i2c_status;
	uint8_t wraddr;
	uint8_t state;
	uint8_t cm_dir;
	uint8_t tm_dir;
	uint8_t read_discard;
	uint8_t speed_id;
	uint8_t msg_idx;
	uint8_t num_msgs;
	struct i2c_msg *msgs;
	struct i2c_mec5_cm_xfr cm_xfr;
	volatile uint8_t mdone;
#ifdef MEC5_I2C_DEBUG_STATE
	volatile uint32_t dbg_state_idx;
	uint8_t dbg_states[MEC5_I2C_DEBUG_STATE_ENTRIES];
#endif
};

#ifdef MEC5_I2C_DEBUG_ISR
volatile uint32_t i2c_mec5_isr_cnt;
volatile uint32_t i2c_mec5_isr_sts;
volatile uint32_t i2c_mec5_isr_compl;
volatile uint32_t i2c_mec5_isr_cfg;

static inline void i2c_mec5_dbg_isr_init(void)
{
	i2c_mec5_isr_cnt = 0;
}
#define MEC5_I2C_DEBUG_ISR_INIT() i2c_mec5_dbg_isr_init()
#else
#define MEC5_I2C_DEBUG_ISR_INIT()
#endif

#ifdef MEC5_I2C_DEBUG_STATE
static void mec5_i2c_dbg_state_init(struct i2c_mec5_data *data)
{
	data->dbg_state_idx = 0u;
	memset((void *)data->dbg_states, 0, sizeof(data->dbg_states));
}

static void mec5_i2c_dbg_state_update(struct i2c_mec5_data *data, uint8_t state)
{
	uint32_t idx = data->dbg_state_idx;

	if (data->dbg_state_idx < MEC5_I2C_DEBUG_STATE_ENTRIES) {
		data->dbg_states[idx] = state;
		data->dbg_state_idx = ++idx;
	}
}
#define MEC5_I2C_DEBUG_STATE_INIT(pd) mec5_i2c_dbg_state_init(pd)
#define MEC5_I2C_DEBUG_STATE_UPDATE(pd, state) mec5_i2c_dbg_state_update(pd, state)
#else
#define MEC5_I2C_DEBUG_STATE_INIT(pd)
#define MEC5_I2C_DEBUG_STATE_UPDATE(pd, state)
#endif

/* NOTE: I2C controller detects Lost Arbitration during START, Rpt-START,
 * data, and ACK phases not during STOP phase.
 */

static int wait_bus_free(const struct device *dev, uint32_t nwait)
{
	struct i2c_mec5_data *data = dev->data;
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	uint32_t count = nwait;
	uint32_t sts = 0;

	while (count--) {
		sts = mec_i2c_smb_status(hwctx, 0);
		data->i2c_status = sts;
		if (sts & BIT(MEC_I2C_STS_LL_NBB_POS)) {
			break; /* bus is free */
		}
		k_busy_wait(WAIT_INTERVAL);
	}

	/* check for bus error, lost arbitration or external stop */
	if ((sts & 0xffu) == (BIT(MEC_I2C_STS_LL_NBB_POS) | BIT(MEC_I2C_STS_LL_NIPEND_POS))) {
		return 0;
	}

	if (sts & BIT(MEC_I2C_STS_LL_BER_POS)) {
		return MEC5_I2C_ERR_BUS;
	}

	if (sts & BIT(MEC_I2C_STS_LL_LRB_AD0_POS)) {
		return MEC5_I2C_ERR_LOST_ARB;
	}

	return MEC5_I2C_ERR_TIMEOUT;
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
static int check_lines(const struct device *dev)
{
	const struct i2c_mec5_config *cfg = dev->config;
	gpio_port_value_t sda = 0, scl = 0;

	gpio_port_get_raw(cfg->sda_gpio.port, &sda);
	scl = sda;
	if (cfg->sda_gpio.port != cfg->scl_gpio.port) {
		gpio_port_get_raw(cfg->scl_gpio.port, &scl);
	}

	if ((sda & BIT(cfg->sda_gpio.pin)) && (scl & BIT(cfg->scl_gpio.pin))) {
		return 0;
	}

	return -EIO;
}

static int i2c_mec5_bb_recover(const struct device *dev)
{
	struct i2c_mec5_data *data = dev->data;
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	int ret = 0;
	uint32_t cnt = I2C_MEC5_RECOVER_SCL_LOW_RETRIES;
	uint8_t pinmsk = BIT(MEC_I2C_BB_SCL_POS) | BIT(MEC_I2C_BB_SDA_POS);
	uint8_t pins = 0u;

	/* Switch I2C pint to controller's bit-bang drivers and tri-state */
	mec_i2c_smb_bbctrl(hwctx, 1u, pinmsk);
	pins = mec_i2c_smb_bbctrl_pin_states(hwctx);

	/* If SCL is low continue sampling hoping it will go high on its own */
	while (!(pins & BIT(MEC_I2C_BB_SCL_POS))) {
		if (cnt) {
			cnt--;
		} else {
			break;
		}
		k_busy_wait(I2C_MEC5_RECOVER_SCL_DELAY_US);
		pins = mec_i2c_smb_bbctrl_pin_states(hwctx);
	}

	pins = mec_i2c_smb_bbctrl_pin_states(hwctx);
	if (!(pins & BIT(MEC_I2C_BB_SCL_POS))) {
		ret = -EBUSY;
		goto disable_bb_exit;
	}

	/* SCL is high, check SDA */
	if (pins & BIT(MEC_I2C_BB_SDA_POS)) {
		ret = 0; /* both high */
		goto disable_bb_exit;
	}

	/* SCL is high and SDA is low. Loop generating 9 clocks until
	 * we observe SDA high or loop terminates
	 */
	ret = -EBUSY;
	for (int i = 0; i < I2C_MEC5_RECOVER_SDA_LOW_RETRIES; i++) {
		mec_i2c_smb_bbctrl(hwctx, 1u, pinmsk);

		/* 9 clocks */
		for (int j = 0; j < 9; j++) {
			pinmsk = BIT(MEC_I2C_BB_SDA_POS);
			mec_i2c_smb_bbctrl(hwctx, 1u, pinmsk);
			k_busy_wait(I2C_MEC5_RECOVER_BB_DELAY_US);
			pinmsk |= BIT(MEC_I2C_BB_SCL_POS);
			mec_i2c_smb_bbctrl(hwctx, 1u, pinmsk);
			k_busy_wait(I2C_MEC5_RECOVER_BB_DELAY_US);
		}

		pins = mec_i2c_smb_bbctrl_pin_states(hwctx);
		if (pins == 0x3u) { /* Both high? */
			ret = 0;
			goto disable_bb_exit;
		}

		/* generate I2C STOP */
		pinmsk = BIT(MEC_I2C_BB_SDA_POS);
		mec_i2c_smb_bbctrl(hwctx, 1u, pinmsk);
		k_busy_wait(I2C_MEC5_RECOVER_BB_DELAY_US);
		pinmsk |= BIT(MEC_I2C_BB_SCL_POS);
		mec_i2c_smb_bbctrl(hwctx, 1u, pinmsk);
		k_busy_wait(I2C_MEC5_RECOVER_BB_DELAY_US);

		pins = mec_i2c_smb_bbctrl_pin_states(hwctx);
		if (pins == 0x3u) { /* Both high? */
			ret = 0;
			goto disable_bb_exit;
		}
	}

disable_bb_exit:
	mec_i2c_smb_bbctrl(hwctx, 0u, 0u);

	return ret;
}

static int i2c_mec5_reset_config(const struct device *dev)
{
	const struct i2c_mec5_config *devcfg = dev->config;
	struct i2c_mec5_data *data = dev->data;
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	struct mec_i2c_smb_cfg mcfg;
	int ret;

	hwctx->base = devcfg->base;
	hwctx->i2c_ctrl_cached = 0;

	data->state = MEC5_I2C_STATE_CLOSED;
	data->i2c_status = 0;
	data->read_discard = 0;
	data->mdone = 0;

	mcfg.std_freq = data->speed_id;
	mcfg.cfg_flags = 0u;
	mcfg.port = devcfg->port_sel;
	mcfg.target_addr1 = 0;
	mcfg.target_addr2 = 0;

	ret = mec_i2c_smb_init(hwctx, &mcfg, NULL);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	/* wait for NBB=1, BER, LAB, or timeout */
	ret = wait_bus_free(dev, WAIT_COUNT);

	return ret;
}

static int i2c_mec5_recover_bus(const struct device *dev)
{
	int ret;

	LOG_ERR("I2C attempt bus recovery\n");

	/* Try controller reset first */
	ret = i2c_mec5_reset_config(dev);
	if (ret == 0) {
		ret = check_lines(dev);
	}

	if (!ret) {
		return 0;
	}

	ret = i2c_mec5_bb_recover(dev);
	if (ret == 0) {
		ret = wait_bus_free(dev, WAIT_COUNT);
	}

	return ret;
}

/* i2c_configure API */
static int i2c_mec5_configure(const struct device *dev, uint32_t dev_config_raw)
{
	struct i2c_mec5_data *data = dev->data;

	if (!(dev_config_raw & I2C_MODE_CONTROLLER)) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		data->speed_id = MEC_I2C_STD_FREQ_100K;
		break;
	case I2C_SPEED_FAST:
		data->speed_id = MEC_I2C_STD_FREQ_400K;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->speed_id = MEC_I2C_STD_FREQ_1M;
		break;
	default:
		return -EINVAL;
	}

	int ret = i2c_mec5_reset_config(dev);

	return ret;
}

/* i2c_get_config API */
static int i2c_mec5_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_mec5_data *data = dev->data;
	uint32_t dcfg = 0u;

	if (!dev_config) {
		return -EINVAL;
	}

	switch (data->speed_id) {
	case MEC_I2C_STD_FREQ_1M:
		dcfg = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
		break;
	case MEC_I2C_STD_FREQ_400K:
		dcfg = I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	default:
		dcfg = I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	}

	dcfg |= I2C_MODE_CONTROLLER;
	*dev_config = dcfg;

	return 0;
}

/* MEC5 I2C controller support 7-bit addressing only.
 * Format 7-bit address for as it appears on the bus as an 8-bit
 * value with R/W bit at bit[0], 0(write), 1(read).
 */
static inline uint8_t i2c_mec5_fmt_addr(uint16_t addr, uint8_t read)
{
	uint8_t fmt_addr = (uint8_t)((addr & 0x7fu) << 1);

	if (read) {
		fmt_addr |= BIT(0);
	}

	return fmt_addr;
}

/* Issue I2C STOP only if controller owns the bus otherwise
 * clear driver state and re-arm controller for next
 * controller-mode or target-mode transaction.
 * Reason for ugly code sequence:
 * Brain-dead I2C controller has write-only control register
 * containing enable interrupt bit. This is the enable for ACK/NACK,
 * bus error and lost arbitration.
 */
static int i2c_mec5_stop(const struct device *dev)
{
	struct i2c_mec5_data *data = dev->data;
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x20);

	/* Can we trust GIRQ status has been cleared before we re-enable? */
	if (mec_i2c_smb_is_bus_owned(hwctx)) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x21);
		data->mdone = 0;
		mec_i2c_smb_stop_gen(hwctx);
		mec_i2c_smb_girq_status_clr(hwctx);
		mec_i2c_smb_idle_intr_enable(hwctx, 1);
		mec_i2c_smb_girq_ctrl(hwctx, MEC_I2C_SMB_GIRQ_EN);
#ifdef MEC5_I2C_DEBUG_USE_SPIN_LOOP
		while (!data->mdone) {
			;
		}
#else
		k_sem_take(&data->sync, K_FOREVER);
#endif
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x22);
	}

	data->cm_dir = MEC5_I2C_DIR_NONE;
	data->state = MEC5_I2C_STATE_CLOSED;

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x23);

	return 0;
}

static int check_msgs(struct i2c_msg *msgs, uint8_t num_msgs)
{
	for (uint8_t n = 0u; n < num_msgs; n++) {
		struct i2c_msg *m = &msgs[n];

		if (m->flags & I2C_MSG_ADDR_10_BITS) {
			return -EINVAL;
		}
	}

	return 0;
}

static int i2c_mec5_xfr_begin(const struct device *dev, uint16_t addr)
{
	struct i2c_mec5_data *data = dev->data;
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	struct i2c_mec5_cm_xfr *xfr = &data->cm_xfr;
	struct i2c_msg *m = data->msgs;
	int ret = 0;
	uint8_t target_addr = 0;
	uint8_t start = MEC_I2C_NORM_START;

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x10);

	target_addr = i2c_mec5_fmt_addr(addr, 0);
	data->wraddr = target_addr;

	if (data->msgs[0].flags & I2C_MSG_READ) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x11);
		target_addr |= BIT(0);
		xfr->mdir = MEC5_I2C_DIR_RD;
	} else {
		xfr->mdir = MEC5_I2C_DIR_WR;
	}

	data->mdone = 0;
	xfr->mbuf = m->buf;
	xfr->mlen = m->len;
	xfr->xfr_sts = 0;
	xfr->target_addr = target_addr;
	xfr->mflags = I2C_MEC5_XFR_FLAG_START_REQ;

	if (mec_i2c_smb_is_bus_owned(hwctx)) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x12);
		if ((data->cm_dir != xfr->mdir) || (m->flags & I2C_MSG_RESTART)) {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x13);
			start = MEC_I2C_RPT_START;
		}
	}

	data->cm_dir = xfr->mdir;
	if (m->flags & I2C_MSG_STOP) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x14);
		xfr->mflags |= I2C_MEC5_XFR_FLAG_STOP_REQ;
	}

	mec_i2c_smb_girq_ctrl(hwctx, (MEC_I2C_SMB_GIRQ_DIS | MEC_I2C_SMB_GIRQ_CLR_STS));

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x15);

	/* Generate (RPT)-START and transmit address for write or read */
	ret = mec_i2c_smb_start_gen(hwctx, target_addr, MEC_I2C_SMB_BYTE_ENI);
	if (ret != MEC_RET_OK) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x16);
		return -EIO;
	}

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x17);
	mec_i2c_smb_girq_ctrl(hwctx, MEC_I2C_SMB_GIRQ_EN);

#ifdef MEC5_I2C_DEBUG_USE_SPIN_LOOP
	while (!data->mdone) {
		;
	}
#else
	k_sem_take(&data->sync, K_FOREVER);
#endif
	if (xfr->xfr_sts) { /* error */
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x18);
		return -EIO;
	}

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x19);

	return 0;
}

/* i2c_transfer API - Synchronous using interrupts */
static int i2c_mec5_transfer(const struct device *dev, struct i2c_msg *msgs,
			     uint8_t num_msgs, uint16_t addr)
{
	struct i2c_mec5_data *data = dev->data;
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	int ret = 0;

	if (!msgs || !num_msgs) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER); /* decrements count */
	k_sem_reset(&data->sync);

	MEC5_I2C_DEBUG_ISR_INIT();

	memset(&data->cm_xfr, 0, sizeof(struct i2c_mec5_cm_xfr));

	ret = check_msgs(msgs, num_msgs);
	if (ret) {
		goto mec5_unlock;
	}

	if (data->state != MEC5_I2C_STATE_OPEN) {
		MEC5_I2C_DEBUG_STATE_INIT(data);

		ret = check_lines(dev);
		data->i2c_status = mec_i2c_smb_status(hwctx, 1);
		if (ret || (data->i2c_status & BIT(MEC_I2C_STS_LL_BER_POS))) {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x50);
			ret = i2c_mec5_recover_bus(dev);
		}
	}

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x1);

	if (ret) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x2);
		data->state = MEC5_I2C_STATE_CLOSED;
		goto mec5_unlock;
	}

	data->state = MEC5_I2C_STATE_OPEN;

	data->msg_idx = 0;
	data->num_msgs = num_msgs;
	data->msgs = msgs;

	ret = i2c_mec5_xfr_begin(dev, addr);
	if (ret) { /* if error issue STOP if bus is still owned by controller */
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x7);
		i2c_mec5_stop(dev);
	}

mec5_unlock:
	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x8);
	if (!mec_i2c_smb_is_bus_owned(hwctx)) {
		data->cm_dir = MEC5_I2C_DIR_NONE;
		data->state = MEC5_I2C_STATE_CLOSED;
	}
	k_sem_give(&data->lock); /* increment count up to limit */

	return ret;
}

/* TODO - Controller can handle two targets */
static int i2c_mec5_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	return -ENOTSUP;
}

static int i2c_mec5_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	return -ENOTSUP;
}

/* ISR helpers and state handlers */
static int i2c_mec5_is_ber_lab(struct i2c_mec5_data *data)
{
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	struct i2c_mec5_cm_xfr *xfr = &data->cm_xfr;

	if (data->i2c_status & (BIT(MEC_I2C_STS_LL_BER_POS) | BIT(MEC_I2C_STS_LL_LAB_POS))) {
		if (data->i2c_status & BIT(MEC_I2C_STS_LL_BER_POS)) {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x82);
			xfr->xfr_sts |= MEC5_I2C_ERR_BUS;
		} else {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x83);
			xfr->xfr_sts |= MEC5_I2C_ERR_LOST_ARB;
		}
		mec_i2c_smb_girq_ctrl(hwctx, MEC_I2C_SMB_GIRQ_DIS);
		mec_i2c_smb_status(hwctx, 1);
		data->mdone = 0x51;
		return 1;
	}

	return 0;
}

static int i2c_mec5_next_msg(struct i2c_mec5_data *data)
{
	struct i2c_mec5_cm_xfr *xfr = &data->cm_xfr;
	struct i2c_msg *m = NULL;
	uint32_t idx = (uint32_t)data->msg_idx;

	if (++idx >= (uint32_t)data->num_msgs) {
		xfr->mbuf = NULL;
		xfr->mlen = 0;
		xfr->mflags = 0;
		xfr->mdir = MEC5_I2C_DIR_NONE;
		return 0;
	}

	data->msg_idx = (uint8_t)(idx & 0xffu);
	m = &data->msgs[idx];

	xfr->mbuf = m->buf;
	xfr->mlen = m->len;
	xfr->mdir = MEC5_I2C_DIR_WR;
	xfr->mflags = 0;
	xfr->target_addr = data->wraddr;
	if (m->flags & I2C_MSG_READ) {
		xfr->mdir = MEC5_I2C_DIR_RD;
		xfr->target_addr |= BIT(0);
	}
	if (m->flags & I2C_MSG_STOP) {
		xfr->mflags = I2C_MEC5_XFR_FLAG_STOP_REQ;
	}
	if (data->cm_dir != xfr->mdir) {
		xfr->mflags |= I2C_MEC5_XFR_FLAG_START_REQ;
	}
	data->cm_dir = xfr->mdir;

	return 1;
}

static int state_check_ack(struct i2c_mec5_data *data)
{
	struct i2c_mec5_cm_xfr *xfr = &data->cm_xfr;
	int next_state = I2C_MEC5_ISR_STATE_GEN_STOP;

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x83);

	if (!(data->i2c_status & BIT(MEC_I2C_STS_LL_LRB_AD0_POS))) { /* ACK? */
		next_state = I2C_MEC5_ISR_STATE_WR_DATA;
		if (xfr->mdir == MEC5_I2C_DIR_RD) {
			next_state = I2C_MEC5_ISR_STATE_RD_DATA;
		}
	} else {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x84);
		xfr->xfr_sts |= I2C_MEC5_XFR_STS_NACK;
	}

	return next_state;
}

static int state_data_wr(struct i2c_mec5_data *data)
{
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	struct i2c_mec5_cm_xfr *xfr = &data->cm_xfr;
	int next_state = I2C_MEC5_ISR_STATE_EXIT_1;
	uint8_t msgbyte;

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x90);

	if (xfr->mlen > 0) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x91);
		msgbyte = *xfr->mbuf;
		mec_i2c_smb_xmit_byte(hwctx, msgbyte);
		xfr->mbuf++;
		xfr->mlen--;
	} else {
		if (xfr->mflags & I2C_MEC5_XFR_FLAG_STOP_REQ) {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x92);
			next_state = I2C_MEC5_ISR_STATE_GEN_STOP;
		} else {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x93);
			next_state = I2C_MEC5_ISR_STATE_NEXT_MSG;
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
static int state_data_rd(struct i2c_mec5_data *data)
{
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	struct i2c_mec5_cm_xfr *xfr = &data->cm_xfr;
	int next_state = I2C_MEC5_ISR_STATE_NEXT_MSG;
	uint8_t msgbyte = 0;

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xa0);

	if (xfr->mlen > 0) {
		next_state = I2C_MEC5_ISR_STATE_EXIT_1;
		if (xfr->mflags & I2C_MEC5_XFR_FLAG_START_REQ) {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xa1);
			/* HW clocks in address it transmits. Read and discard.
			 * HW generates clocks for first data byte.
			 */
			xfr->mflags &= ~(I2C_MEC5_XFR_FLAG_START_REQ);
			if ((xfr->mlen == 1) && (xfr->mflags & I2C_MEC5_XFR_FLAG_STOP_REQ)) {
				MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xa2);
				/* disable auto-ACK and make sure ENI=1 */
				mec_i2c_smb_auto_ack_disable(hwctx, 1);
			}
			/* read byte currently in HW buffer and generate clocks for next byte */
			mec_i2c_smb_read_byte(hwctx, &msgbyte);
		} else if (xfr->mflags & I2C_MEC5_XFR_FLAG_STOP_REQ) {
			if (xfr->mlen != 1) {
				MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xa3);
				if (xfr->mlen == 2) {
					MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xa4);
					mec_i2c_smb_auto_ack_disable(hwctx, 1);
				}
				mec_i2c_smb_read_byte(hwctx, &msgbyte);
				*xfr->mbuf = msgbyte;
				xfr->mbuf++;
				xfr->mlen--;
			} else { /* Begin STOP generation and read last byte */
				MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xa5);
				xfr->mflags &= ~(I2C_MEC5_XFR_FLAG_STOP_REQ);
				mec_i2c_smb_idle_intr_enable(hwctx, 1);
				mec_i2c_smb_stop_gen(hwctx);
				mec_i2c_smb_read_byte(hwctx, &msgbyte);
				*xfr->mbuf = msgbyte;
				xfr->mlen = 0;
			}
		} else { /* No START or STOP flags */
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xa6);
			mec_i2c_smb_read_byte(hwctx, &msgbyte);
			*xfr->mbuf = msgbyte;
			xfr->mbuf++;
			xfr->mlen--;
		}
	}

	return next_state;
}

static int state_next_msg(struct i2c_mec5_data *data)
{
	struct i2c_mec5_cm_xfr *xfr = &data->cm_xfr;
	int next_state = I2C_MEC5_ISR_STATE_MAX;
	int ret = i2c_mec5_next_msg(data);

	if (ret) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xb0);
		if (xfr->mflags & I2C_MEC5_XFR_FLAG_START_REQ) {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xb1);
			next_state = I2C_MEC5_ISR_STATE_GEN_START;
		} else {
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xb2);
			next_state = I2C_MEC5_ISR_STATE_WR_DATA;
			if (xfr->mdir == MEC5_I2C_DIR_RD) {
				MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xb3);
				next_state = I2C_MEC5_ISR_STATE_RD_DATA;
			}
		}
	} else { /* no more messages */
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0xb3);
		data->mdone = 1;
	}

	return next_state;
}

/* Controller Mode ISR  */
static void i2c_mec5_isr(const struct device *dev)
{
	struct i2c_mec5_data *data = dev->data;
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	struct i2c_mec5_cm_xfr *xfr = &data->cm_xfr;
	bool run_sm = true;
	int state = I2C_MEC5_ISR_STATE_CHK_ACK;
	int next_state = I2C_MEC5_ISR_STATE_MAX;
	int idle_active = 0;

	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x80);

#ifdef MEC5_I2C_DEBUG_ISR
	i2c_mec5_isr_cnt++;
	i2c_mec5_isr_sts = sys_read8((mem_addr_t)hwctx->base);
	i2c_mec5_isr_compl = sys_read32((mem_addr_t)hwctx->base + 0x20u);
	i2c_mec5_isr_cfg = sys_read32((mem_addr_t)hwctx->base + 0x28u);
	while (data->mdone) { /* should not hang here */
		;
	}
#endif
	idle_active = mec_i2c_smb_is_idle_intr(hwctx);
	data->i2c_status = mec_i2c_smb_status(hwctx, 1);
	mec_i2c_smb_wake_status_clr(hwctx);

	if (idle_active) { /* turn off as soon as possible */
		state = I2C_MEC5_ISR_STATE_EV_IDLE;
		mec_i2c_smb_idle_intr_enable(hwctx, 0);
	}

	/* Lost Arbitration or Bus Error? */
	if (i2c_mec5_is_ber_lab(data)) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x81);
		run_sm = false;
	}

	while (run_sm) {
		switch (state) {
		case I2C_MEC5_ISR_STATE_GEN_START:
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x82);
			mec_i2c_smb_start_gen(hwctx, xfr->target_addr, 1);
			run_sm = false;
			break;
		case I2C_MEC5_ISR_STATE_CHK_ACK:
			next_state = state_check_ack(data);
			break;
		case I2C_MEC5_ISR_STATE_WR_DATA:
			next_state = state_data_wr(data);
			break;
		case I2C_MEC5_ISR_STATE_RD_DATA:
			next_state = state_data_rd(data);
			break;
		case I2C_MEC5_ISR_STATE_GEN_STOP:
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x85);
			mec_i2c_smb_idle_intr_enable(hwctx, 1);
			mec_i2c_smb_stop_gen(hwctx);
			run_sm = false;
			break;
		case I2C_MEC5_ISR_STATE_EV_IDLE:
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x86);
			mec_i2c_smb_idle_status_clr(hwctx);
			next_state = I2C_MEC5_ISR_STATE_NEXT_MSG;
			if (xfr->xfr_sts) {
				data->mdone = 0x13;
				run_sm = false;
			}
			break;
		case I2C_MEC5_ISR_STATE_NEXT_MSG:
			next_state = state_next_msg(data);
			break;
		case I2C_MEC5_ISR_STATE_EXIT_1:
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x87);
			data->mdone = 0;
			run_sm = false;
			break;
		default:
			MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x88);
			mec_i2c_smb_status(hwctx, 1);
			mec_i2c_smb_girq_ctrl(hwctx, MEC_I2C_SMB_GIRQ_DIS);
			if (!data->mdone) {
				data->mdone = 0x66;
			}
			run_sm = false;
			break;
		}

		state = next_state;
	}

	/* ISR common exit path */
	MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x8e);
	mec_i2c_smb_girq_status_clr(hwctx);
	if (data->mdone) {
		MEC5_I2C_DEBUG_STATE_UPDATE(data, 0x8f);
		k_sem_give(&data->sync);
	}
} /* i2c_mec5_isr */

static const struct i2c_driver_api i2c_mec5_driver_api = {
	.configure = i2c_mec5_configure,
	.get_config = i2c_mec5_get_config,
	.transfer = i2c_mec5_transfer,
	.target_register = i2c_mec5_target_register,
	.target_unregister = i2c_mec5_target_unregister,
};

static int i2c_mec5_init(const struct device *dev)
{
	const struct i2c_mec5_config *cfg = dev->config;
	struct i2c_mec5_data *data = dev->data;
	struct mec_i2c_smb_ctx *hwctx = &data->ctx;
	int ret;
	uint32_t bitrate_cfg;

	hwctx->base = cfg->base;
	hwctx->i2c_ctrl_cached = 0;
	data->state = MEC5_I2C_STATE_CLOSED;
	data->i2c_status = 0;
	data->mdone = 0;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("pinctrl setup failed (%d)", ret);
		return ret;
	}

	bitrate_cfg = i2c_map_dt_bitrate(cfg->clock_freq);
	if (!bitrate_cfg) {
		return -EINVAL;
	}

	/* Default configuration */
	ret = i2c_mec5_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret) {
		return ret;
	}

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->sync, 0, 1);

	if (cfg->irq_config_func) {
		cfg->irq_config_func();
	}

	return 0;
}

#define I2C_MEC5_DEVICE(n)						\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void i2c_mec5_irq_config_func_##n(void);			\
									\
	static struct i2c_mec5_data i2c_mec5_data_##n;			\
									\
	static const struct i2c_mec5_config i2c_mec5_config_##n = {	\
		.base = (struct i2c_smb_regs *)DT_INST_REG_ADDR(n),	\
		.port_sel = DT_INST_PROP(n, port_sel),			\
		.clock_freq = DT_INST_PROP(n, clock_frequency),		\
		.sda_gpio = GPIO_DT_SPEC_INST_GET(n, sda_gpios),	\
		.scl_gpio = GPIO_DT_SPEC_INST_GET(n, scl_gpios),	\
		.irq_config_func = i2c_mec5_irq_config_func_##n,	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_mec5_init, NULL,		\
		&i2c_mec5_data_##n, &i2c_mec5_config_##n,		\
		POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,			\
		&i2c_mec5_driver_api);					\
									\
	static void i2c_mec5_irq_config_func_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    i2c_mec5_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_MEC5_DEVICE)

