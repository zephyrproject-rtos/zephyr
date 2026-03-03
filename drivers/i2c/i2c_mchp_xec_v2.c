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
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_xec_v2, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"
#include "i2c_mchp_xec_regs.h"

#define SPEED_100KHZ_BUS 0
#define SPEED_400KHZ_BUS 1
#define SPEED_1MHZ_BUS   2

#define EC_OWN_I2C_ADDR 0x7F
#define RESET_WAIT_US   20

/* I2C timeout is  10 ms (WAIT_INTERVAL * WAIT_COUNT) */
#define WAIT_INTERVAL_US     10000U
#define PIN_WAIT_INTERVAL_US 2000U

#define STOP_WAIT_COUNT 500
#define PIN_CFG_WAIT    50

/* I2C Read/Write bit pos */
#define I2C_READ_WRITE_POS 0

/* I2C SCL and SDA lines(signals) */
#define I2C_LINES_SCL_HI_POS 0
#define I2C_LINES_SDA_HI_POS 1
#define I2C_LINES_BOTH_HI    (BIT(I2C_LINES_SCL_HI_POS) | BIT(I2C_LINES_SDA_HI_POS))

#define I2C_START     0U
#define I2C_RPT_START 1U

#define I2C_ENI_DIS 0U
#define I2C_ENI_EN  1U

#define I2C_XEC_CTRL_WR_DLY 8

#define I2C_XEC_STATE_STOPPED 1U
#define I2C_XEC_STATE_OPEN    2U

#define I2C_XEC_OK        0
#define I2C_XEC_ERR_LAB   1
#define I2C_XEC_ERR_BUS   2
#define I2C_XEC_ERR_TMOUT 3

/* I2C recover SCL low retries */
#define I2C_RECOVER_SCL_LOW_RETRIES 10
/* I2C recover SDA low retries */
#define I2C_RECOVER_SDA_LOW_RETRIES 3
/* I2C recovery bit bang delay */
#define I2C_RECOVER_BB_DELAY_US     5
/* I2C recovery SCL sample delay */
#define I2C_RECOVER_SCL_DELAY_US    50

/* Hardware needs multiple 16 MHz clock times to sample pins after using its soft reset */
#define XEC_I2C_CTRL_RESET_DELAY_US 10

/* default value of target data */
#define XEC_I2C_TARGET_DFLT_DATA_VAL 0

struct xec_i2c_timing {
	uint32_t freq_hz;
	uint32_t data_tm;    /* data timing  */
	uint32_t idle_sc;    /* idle scaling */
	uint32_t timeout_sc; /* timeout scaling */
	uint32_t bus_clock;  /* bus clock hi/lo pulse widths */
	uint8_t rpt_sta_htm; /* repeated start hold time */
};

struct i2c_xec_config {
	uint32_t base_addr;
	uint32_t clock_freq;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t enc_pcr;
	uint8_t port_sel;
	struct gpio_dt_spec sda_gpio;
	struct gpio_dt_spec scl_gpio;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

struct i2c_xec_data {
	uint32_t dev_config;
	uint32_t i2c_compl;
	uint8_t i2c_ctrl;
	uint8_t i2c_addr;
	uint8_t i2c_status;
	uint8_t state;
	uint8_t read_discard;
	uint8_t speed_id;
	uint8_t i2c_error;
	struct k_mutex mux;
	struct i2c_target_config *target_cfg;
	bool target_attached;
	bool target_read;
};

static const struct xec_i2c_timing xec_i2c_nl_timing_tbl[] = {
	{KHZ(100), XEC_I2C_SMB_DATA_TM_100K, XEC_I2C_SMB_IDLE_SC_100K, XEC_I2C_SMB_TMO_SC_100K,
	 XEC_I2C_SMB_BUS_CLK_100K, XEC_I2C_SMB_RSHT_100K},
	{KHZ(400), XEC_I2C_SMB_DATA_TM_400K, XEC_I2C_SMB_IDLE_SC_400K, XEC_I2C_SMB_TMO_SC_400K,
	 XEC_I2C_SMB_BUS_CLK_400K, XEC_I2C_SMB_RSHT_400K},
	{MHZ(1), XEC_I2C_SMB_DATA_TM_1M, XEC_I2C_SMB_IDLE_SC_1M, XEC_I2C_SMB_TMO_SC_1M,
	 XEC_I2C_SMB_BUS_CLK_1M, XEC_I2C_SMB_RSHT_1M},
};

static int i2c_xec_reset_config(const struct device *dev);

/* Reset the controller and program the port.
 * After reset the controller timings are for 100 KHz I2C clock.
 * PCR peripheral block reset is used instead of I2C block soft reset.
 */
static void i2c_xec_ctrl_reset(const struct device *dev)
{
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->base_addr;
	uint32_t port_fe_val = XEC_I2C_CFG_PORT_SET(drvcfg->port_sel) | BIT(XEC_I2C_CFG_FEN_POS);

	soc_xec_pcr_reset_en(drvcfg->enc_pcr);
	sys_write32(port_fe_val, regbase + XEC_I2C_CFG_OFS);
	k_busy_wait(XEC_I2C_CTRL_RESET_DELAY_US);
}

static int i2c_xec_prog_timing(mm_reg_t regbase, uint32_t freq_hz)
{
	const struct xec_i2c_timing *ptm = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(xec_i2c_nl_timing_tbl); i++) {
		if (freq_hz == xec_i2c_nl_timing_tbl[i].freq_hz) {
			ptm = &xec_i2c_nl_timing_tbl[i];
			break;
		}
	}

	if (ptm == NULL) {
		return -EINVAL;
	}

	sys_write32(ptm->data_tm, regbase + XEC_I2C_DT_OFS);
	sys_write32(ptm->idle_sc, regbase + XEC_I2C_IDS_OFS);
	sys_write32(ptm->timeout_sc, regbase + XEC_I2C_TMOUT_SC_OFS);
	sys_write32(ptm->bus_clock, regbase + XEC_I2C_BCLK_OFS);
	sys_write8(ptm->rpt_sta_htm, regbase + XEC_I2C_RSHT_OFS);

	return 0;
}

/* Returns I2C frequency in Hz. Unsupported config return 0 */
static uint32_t i2c_xec_freq_from_dev_config(uint32_t dev_config)
{
	uint32_t speed = I2C_SPEED_GET(dev_config);
	uint32_t fhz = 0;

	switch (speed) {
	case I2C_SPEED_STANDARD: /* 100 KHz */
		return (uint32_t)KHZ(100);
	case I2C_SPEED_FAST: /* 400 KHz */
		return (uint32_t)KHZ(400);
	case I2C_SPEED_FAST_PLUS: /* 1 MHz */
		return (uint32_t)MHZ(1);
	default:
		fhz = 0;
	}

	return fhz;
}

#if defined(CONFIG_SOC_SERIES_MEC15XX) || defined(CONFIG_SOC_SERIES_MEC172X)
uint8_t i2c_xec_get_lines(mm_reg_t regbase, const struct gpio_dt_spec *scl_gpio,
			  const struct gpio_dt_spec *sda_gpio)
{
	gpio_port_value_t sda = 0, scl = 0;
	uint8_t lines = 0;

	gpio_port_get_raw(scl_gpio->port, &scl);

	if (scl_gpio->port == sda_gpio->port) { /* both pins in same GPIO port? */
		sda = scl;
	} else {
		gpio_port_get_raw(sda_gpio->port, &sda);
	}

	if ((scl & BIT(scl_gpio->pin)) != 0) {
		lines |= BIT(I2C_LINES_SCL_HI_POS);
	}

	if ((scl & BIT(sda_gpio->pin)) != 0) {
		lines |= BIT(I2C_LINES_SDA_HI_POS);
	}

	return lines;
}
#else
/* I2C HW v3.8 and above support a live control bit in the BB control register.
 * Enabling this bit allows the live state of the SCL and SDA pins to be read
 * with making a call to SoC layer or GPIO driver.
 */
uint8_t i2c_xec_get_lines(mm_reg_t regbase, const struct gpio_dt_spec *scl_gpio,
			  const struct gpio_dt_spec *sda_gpio)
{
	uint8_t bbcr = 0, lines = 0;

	sys_write8(BIT(XEC_I2C_BBCR_CM_POS), regbase + XEC_I2C_BBCR_OFS);
	bbcr = sys_read8(regbase + XEC_I2C_BBCR_OFS);

	if ((bbcr & BIT(XEC_I2C_BBCR_SCL_IN_POS)) != 0) {
		lines |= BIT(I2C_LINES_SCL_HI_POS);
	}

	if ((bbcr & BIT(XEC_I2C_BBCR_SDA_IN_POS)) != 0) {
		lines |= BIT(I2C_LINES_SDA_HI_POS);
	}

	return lines;
}
#endif

static void i2c_ctl_wr(const struct device *dev, uint8_t ctrl)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *cfg = dev->config;
	mm_reg_t rb = cfg->base_addr;

	data->i2c_ctrl = ctrl;
	sys_write8(ctrl, rb + XEC_I2C_CR_OFS);

	for (int i = 0; i < I2C_XEC_CTRL_WR_DLY; i++) {
		sys_write8(ctrl, rb + XEC_I2C_BLKID_OFS);
	}
}

static uint8_t i2c_sr_read(const struct device *dev)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *cfg = dev->config;
	mm_reg_t rb = cfg->base_addr;
	uint8_t sr = sys_read8(rb + XEC_I2C_SR_OFS);

	data->i2c_status = sr;

	return sr;
}

/* Wait for I2C controller to detect the bus is free (idle) meaning both SCL and SDA are
 * released (high) for a certain number of the controller's 16 MHz BAUD clock.
 * Too many ways for I2C to malfunction:
 * Bus Error
 * Lost Arbitration in multi-master scenario.
 * NOTE: WAIT_FOR macro uses kernel timer granularity. Most projects using XEC are configured
 * to use 32KHz for the kernel timer. Granularity is 30.5 microseconds.
 */
static int wait_bus_free(const struct device *dev, uint32_t timeout_us)
{
	struct i2c_xec_data *const data = dev->data;

	if (!WAIT_FOR(((i2c_sr_read(dev) & BIT(XEC_I2C_SR_NBB_POS)) != 0), timeout_us, NULL)) {
		data->i2c_error = I2C_XEC_ERR_TMOUT;
		return -ETIMEDOUT;
	}

	/* NBB -> 1 not busy can occur for STOP, BER, or LAB */
	if (data->i2c_status == (BIT(XEC_I2C_SR_PIN_POS) | BIT(XEC_I2C_SR_NBB_POS))) {
		/* No service requested(PIN=1), NotBusy(NBB=1), and no errors */
		return 0;
	}

	if ((data->i2c_status & BIT(XEC_I2C_SR_BER_POS)) != 0) {
		data->i2c_error = I2C_XEC_ERR_BUS;
		return -EIO;
	}

	if ((data->i2c_status & BIT(XEC_I2C_SR_LAB_POS)) != 0) {
		data->i2c_error = I2C_XEC_ERR_LAB;
		return -ECONNABORTED;
	}

	return -ETIMEDOUT;
}

static void i2c_xec_initial_cfg(const struct device *dev)
{
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	uint32_t val = (BIT(XEC_I2C_CFG_GC_DIS_POS) | BIT(XEC_I2C_CFG_FEN_POS) |
			XEC_I2C_CFG_PORT_SET((uint32_t)drvcfg->port_sel));

	sys_write32(val, rb + XEC_I2C_CFG_OFS);
}

static int i2c_xec_reset_config(const struct device *dev)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	uint32_t freq_hz = 0;
	int rc = 0;

	data->state = I2C_XEC_STATE_STOPPED;
	data->read_discard = 0;

	soc_xec_pcr_sleep_en_clear(drvcfg->enc_pcr);
	soc_ecia_girq_ctrl(drvcfg->girq, drvcfg->girq_pos, 0);
	i2c_xec_ctrl_reset(dev);
	soc_ecia_girq_status_clear(drvcfg->girq, drvcfg->girq_pos);

	/* PIN=1 to clear all status except NBB and synchronize */
	i2c_ctl_wr(dev, BIT(XEC_I2C_CR_PIN_POS));

#ifdef CONFIG_I2C_TARGET
	if (data->target_cfg != NULL) {
		sys_write32(XEC_I2C_OA_1_SET(data->target_cfg->address), rb + XEC_I2C_OA_OFS);
	}
#endif
	/* Port number and filter enable MUST be written before enabling controller */
	i2c_xec_initial_cfg(dev);

	/*
	 * Before enabling the controller program the desired bus clock,
	 * repeated start hold time, data timing, and timeout scaling
	 * registers.
	 */
	freq_hz = i2c_xec_freq_from_dev_config(data->dev_config);

	rc = i2c_xec_prog_timing(rb, freq_hz);

	/*
	 * PIN=1 clears all status except NBB
	 * ESO=1 enables output drivers
	 * ACK=1 enable ACK generation when data/address is clocked in.
	 */
	i2c_ctl_wr(dev,
		   (BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ACK_POS)));

	/* Enable controller */
	sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_ENAB_POS);
	k_busy_wait(RESET_WAIT_US);

	/* wait for NBB=1, BER, LAB, or timeout */
	return wait_bus_free(dev, WAIT_INTERVAL_US);
}

static uint32_t get_lines(const struct device *dev)
{
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;

	return (uint32_t)i2c_xec_get_lines(rb, &drvcfg->scl_gpio, &drvcfg->sda_gpio);
}

#ifdef CONFIG_I2C_TARGET
/*
 * Restart I2C controller as target for ACK of address match.
 * Setting PIN clears all status in I2C.Status register except NBB.
 */
static void restart_target(const struct device *dev)
{
	i2c_ctl_wr(dev, BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) |
				BIT(XEC_I2C_CR_ACK_POS) | BIT(XEC_I2C_CR_ENI_POS));
}

/*
 * Configure I2C controller acting as target to NACK the next received byte.
 * NOTE: Firmware must re-enable ACK generation before the start of the next
 * transaction otherwise the controller will NACK its target addresses.
 */
static void target_config_for_nack(const struct device *dev)
{
	i2c_ctl_wr(dev,
		   BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ENI_POS));
}
#endif

/* We wait for PIN 1->0(assertion) */
static int wait_pin_assert(const struct device *dev, uint32_t pin_wait_us)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	int rc = 0;

	data->i2c_error = 0;

	if (!WAIT_FOR((soc_test_bit8(rb + XEC_I2C_SR_OFS, XEC_I2C_SR_PIN_POS) == 0), pin_wait_us,
		      NULL)) {
		data->i2c_error = I2C_XEC_ERR_TMOUT;
		rc = -ETIMEDOUT;
	}

	data->i2c_status = sys_read8(rb + XEC_I2C_SR_OFS);
	data->i2c_compl = sys_read32(rb + XEC_I2C_CMPL_OFS);

	if ((data->i2c_status & BIT(XEC_I2C_SR_BER_POS)) != 0) {
		data->i2c_error = I2C_XEC_ERR_BUS;
		return -EIO;
	}

	if ((data->i2c_status & BIT(XEC_I2C_SR_LAB_POS)) != 0) {
		data->i2c_error = I2C_XEC_ERR_LAB;
		return -ECONNABORTED;
	}

	return rc;
}

static int gen_start(const struct device *dev, uint8_t addr8, bool is_repeated)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	uint8_t ctrl = BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_STA_POS) | BIT(XEC_I2C_CR_ACK_POS);

	data->i2c_addr = addr8;

	if (is_repeated == true) {
		i2c_ctl_wr(dev, ctrl);
		sys_write8(addr8, rb + XEC_I2C_DATA_OFS);
	} else {
		ctrl |= BIT(XEC_I2C_CR_PIN_POS);
		sys_write8(addr8, rb + XEC_I2C_DATA_OFS);
		i2c_ctl_wr(dev, ctrl);
	}

	return 0;
}

static int gen_stop(const struct device *dev)
{
	uint8_t ctrl = BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_STO_POS) |
		       BIT(XEC_I2C_CR_ACK_POS);

	i2c_ctl_wr(dev, ctrl);

	return 0;
}

static int do_start(const struct device *dev, uint8_t addr8, bool is_repeated)
{
	struct i2c_xec_data *const data = dev->data;
	int ret = 0;

	gen_start(dev, addr8, is_repeated);

	ret = wait_pin_assert(dev, PIN_WAIT_INTERVAL_US);
	if (ret != 0) {
		i2c_xec_reset_config(dev);
		return ret;
	}

	/* PIN 1->0: check for NACK */
	if ((data->i2c_status & BIT(XEC_I2C_SR_LRB_AD0_POS)) != 0) {
		gen_stop(dev);

		ret = wait_bus_free(dev, WAIT_INTERVAL_US);
		if (ret != 0) {
			i2c_xec_reset_config(dev);
		}

		return -EIO;
	}

	return 0;
}

static int i2c_xec_v2_configure(const struct device *dev, uint32_t dev_config_raw)
{
	struct i2c_xec_data *const data = dev->data;

	if ((dev_config_raw & I2C_MODE_CONTROLLER) == 0) {
		return -ENOTSUP;
	}

	if ((dev_config_raw & I2C_ADDR_10_BITS) != 0) {
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

	data->dev_config = dev_config_raw;

	return i2c_xec_reset_config(dev);
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
static int i2c_xec_bus_recovery(mm_reg_t rb)
{
	int i = 0, ret = 0;
	uint8_t bbcr = 0;

	bbcr = BIT(XEC_I2C_BBCR_EN_POS) | BIT(XEC_I2C_BBCR_SCL_POS) | BIT(XEC_I2C_BBCR_SDA_POS);
	sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);

	if (soc_test_bit8(rb + XEC_I2C_BBCR_OFS, XEC_I2C_BBCR_SCL_IN_POS) == 0) {
		for (i = 0;; i++) {
			if (i >= I2C_RECOVER_SCL_LOW_RETRIES) {
				ret = -EBUSY;
				goto recov_exit;
			}

			k_busy_wait(I2C_RECOVER_SCL_DELAY_US);

			if (soc_test_bit8(rb + XEC_I2C_BBCR_OFS, XEC_I2C_BBCR_SCL_IN_POS) != 0) {
				break; /* SCL went High */
			}
		}
	}

	if (soc_test_bit8(rb + XEC_I2C_BBCR_OFS, XEC_I2C_BBCR_SDA_IN_POS) != 0) {
		ret = 0;
		goto recov_exit;
	}

	ret = -EBUSY;
	/* SDA recovery */
	for (i = 0; i < I2C_RECOVER_SDA_LOW_RETRIES; i++) {
		/* SCL output mode and tri-stated */
		bbcr = (BIT(XEC_I2C_BBCR_EN_POS) | BIT(XEC_I2C_BBCR_CD_POS) |
			BIT(XEC_I2C_BBCR_SCL_POS) | BIT(XEC_I2C_BBCR_SDA_POS));
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
		k_busy_wait(I2C_RECOVER_BB_DELAY_US);

		for (int j = 0; j < 9; j++) {
			if (soc_test_bit8(rb + XEC_I2C_BBCR_OFS, XEC_I2C_BBCR_SDA_IN_POS) != 0) {
				break;
			}

			/* drive SCL low */
			bbcr = (BIT(XEC_I2C_BBCR_EN_POS) | BIT(XEC_I2C_BBCR_CD_POS) |
				BIT(XEC_I2C_BBCR_SDA_POS));
			sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
			k_busy_wait(I2C_RECOVER_BB_DELAY_US);

			/* release SCL: pulled high by external pull-up */
			bbcr = (BIT(XEC_I2C_BBCR_EN_POS) | BIT(XEC_I2C_BBCR_CD_POS) |
				BIT(XEC_I2C_BBCR_SCL_POS) | BIT(XEC_I2C_BBCR_SDA_POS));
			sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
			k_busy_wait(I2C_RECOVER_BB_DELAY_US);
		}

		/* SCL is High. Produce rising edge on SCL for STOP */
		bbcr = (BIT(XEC_I2C_BBCR_EN_POS) | BIT(XEC_I2C_BBCR_SCL_POS) |
			BIT(XEC_I2C_BBCR_DD_POS));
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
		k_busy_wait(I2C_RECOVER_BB_DELAY_US);

		bbcr = (BIT(XEC_I2C_BBCR_EN_POS) | BIT(XEC_I2C_BBCR_SCL_POS) |
			BIT(XEC_I2C_BBCR_SDA_POS));
		sys_write8(bbcr, rb + XEC_I2C_BBCR_OFS);
		k_busy_wait(I2C_RECOVER_BB_DELAY_US);

		/* check if SCL and SDA are both high */
		bbcr = sys_read8(rb + XEC_I2C_BBCR_OFS) &
		       (BIT(XEC_I2C_BBCR_SCL_IN_POS) | BIT(XEC_I2C_BBCR_SDA_IN_POS));

		if (bbcr == (BIT(XEC_I2C_BBCR_SCL_IN_POS) | BIT(XEC_I2C_BBCR_SDA_IN_POS))) {
			ret = 0; /* successful recovery */
			goto recov_exit;
		}
	}

recov_exit:
	/* BB mode disable reconnects SCL and SDA to I2C logic. */
	sys_write8(BIT(XEC_I2C_BBCR_CM_POS), rb + XEC_I2C_BBCR_OFS);

	return ret;
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
static int i2c_xec_v2_recover_bus(const struct device *dev)
{
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	uint32_t dev_config = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;
	int rc = 0;

	/* reset controller and configure for 100 KHz before recovery attempt */
	rc = i2c_xec_v2_configure(dev, dev_config);
	if (rc != 0) {
		rc = i2c_xec_bus_recovery(rb);
		if (rc != 0) {
			return rc;
		}
	}

	/* reconfigure controller at previous speed setting */
	rc = i2c_xec_reset_config(dev);

	return rc;
}

static int do_stop(const struct device *dev, uint32_t nwait)
{
	struct i2c_xec_data *const data = dev->data;
	uint32_t lines = 0;
	int ret = 0;

	data->state = I2C_XEC_STATE_STOPPED;
	data->read_discard = 0;

	gen_stop(dev);

	ret = wait_bus_free(dev, nwait);
	if (ret != 0) {
		lines = get_lines(dev);

		if (lines != I2C_LINES_BOTH_HI) {
			i2c_xec_v2_recover_bus(dev);
		} else {
			ret = i2c_xec_reset_config(dev);
		}
	}

	if (ret == 0) {
		/* stop success: prepare for next transaction */
		i2c_ctl_wr(dev, (BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) |
				 BIT(XEC_I2C_CR_ACK_POS)));
	}

	return ret;
}

/* I2C Controller transmit: polling implementation */
static int ctrl_tx(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	int ret = 0;
	uint8_t mflags = msg->flags;
	uint8_t addr8 = (uint8_t)((addr & 0x7FU) << 1);
	bool repeated_start = false;

	if ((data->state == I2C_XEC_STATE_STOPPED) || ((mflags & I2C_MSG_RESTART) != 0)) {
		data->i2c_addr = addr8;

		if (data->state == I2C_XEC_STATE_STOPPED) {
			/* Is bus free and controller ready? */
			ret = wait_bus_free(dev, WAIT_INTERVAL_US);
			if (ret != 0) {
				ret = i2c_xec_v2_recover_bus(dev);
				if (ret != 0) {
					return ret;
				}
			}
		} else {
			repeated_start = true;
		}

		ret = do_start(dev, addr8, repeated_start);
		if (ret != 0) {
			return ret;
		}

		data->state = I2C_XEC_STATE_OPEN;
	}

	for (size_t n = 0; n < msg->len; n++) {
		/* writing I2C.DATA register causes PIN status to de-assert */
		sys_write8(msg->buf[n], rb + XEC_I2C_DATA_OFS);

		ret = wait_pin_assert(dev, PIN_WAIT_INTERVAL_US);
		if (ret != 0) {
			i2c_xec_reset_config(dev);
			return ret;
		}

		if ((data->i2c_status & BIT(XEC_I2C_SR_LRB_AD0_POS)) != 0) { /* NACK? */
			do_stop(dev, STOP_WAIT_COUNT);
			return -EIO;
		}
	}

	if ((mflags & I2C_MSG_STOP) != 0) {
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
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	uint8_t *p8 = NULL;
	size_t data_len = msg->len;
	uint8_t mflags = msg->flags;
	uint8_t addr8 = (uint8_t)(((addr & 0x7FU) << 1) | BIT(0));
	uint8_t temp = 0;
	int ret = 0;
	bool repeated_start = false;

	if ((data->state == I2C_XEC_STATE_STOPPED) || ((mflags & I2C_MSG_RESTART) != 0)) {
		data->i2c_addr = addr8;
		/* controller clocked address into I2CDATA */
		data->read_discard = 1U;
		if (data->state == I2C_XEC_STATE_STOPPED) {
			/* Is bus free and controller ready? */
			ret = wait_bus_free(dev, WAIT_INTERVAL_US);
			if (ret != 0) {
				i2c_xec_reset_config(dev);
				return ret;
			}
		} else {
			repeated_start = true;
		}

		ret = do_start(dev, addr8, repeated_start);
		if (ret != 0) {
			return ret;
		}

		data->state = I2C_XEC_STATE_OPEN;
	}

	if (data_len == 0) { /* requested message length is 0 */
		ret = 0;

		if ((mflags & I2C_MSG_STOP) != 0) {
			data->state = I2C_XEC_STATE_STOPPED;
			data->read_discard = 0;
			ret = do_stop(dev, STOP_WAIT_COUNT);
		}

		return ret;
	}

	if (data->read_discard != 0) {
		data_len++;
	}

	p8 = &msg->buf[0];

	while (data_len != 0) {
		if ((data_len <= 2U) && ((mflags & I2C_MSG_STOP) != 0)) {
			if (data_len == 2U) {
				/* turn-off auto-ACK for next byte and do not clear PIN */
				i2c_ctl_wr(dev, BIT(XEC_I2C_CR_ESO_POS));
			} else {
				/* exit loop, generate STOP, then read last data byte */
				break;
			}
		}

		/* read I2C.DATA register returns current captured data and generates clocks
		 * for the next data byte (read-ahead)
		 */
		temp = sys_read8(rb + XEC_I2C_DATA_OFS);

		if (data->read_discard != 0) {
			data->read_discard = 0;
		} else {
			*p8++ = temp;
		}

		ret = wait_pin_assert(dev, PIN_WAIT_INTERVAL_US);
		if (ret != 0) {
			i2c_xec_reset_config(dev);
			return ret;
		}

		data_len--;
	}

	if ((mflags & I2C_MSG_STOP) != 0) {
		data->state = I2C_XEC_STATE_STOPPED;
		data->read_discard = 0;

		ret = do_stop(dev, STOP_WAIT_COUNT);
		if (ret == 0) {
			/* After instructing I2C controller to genrerate a STOP we must read
			 * the last byte from the target from I2C.DATA. HW does not generate
			 * clocks since we generated a STOP.
			 */
			*p8 = sys_read8(rb + XEC_I2C_DATA_OFS);
		}
	}

	return ret;
}

static int i2c_xec_v2_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	struct i2c_xec_data *data = dev->data;
	int ret = 0;

#ifdef CONFIG_I2C_TARGET
	if (data->target_attached == true) {
		LOG_ERR("Device is registered as target");
		return -EBUSY;
	}
#endif

	k_mutex_lock(&data->mux, K_FOREVER);

	data->i2c_error = I2C_XEC_OK;

	for (uint8_t i = 0; i < num_msgs; i++) {
		struct i2c_msg *m = &msgs[i];

		if ((m->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = ctrl_tx(dev, m, addr);
		} else {
			ret = ctrl_rx(dev, m, addr);
		}

		if (ret != 0) {
			data->state = I2C_XEC_STATE_STOPPED;
			data->read_discard = 0;
			LOG_ERR("i2x_xfr: flags: %x error: %d", m->flags, ret);
			break;
		}
	}

	k_mutex_unlock(&data->mux);

	return ret;
}

#ifdef CONFIG_I2C_TARGET

static int target_wr_req_cb(const struct device *dev, const struct i2c_target_callbacks *tcbs)
{
	struct i2c_xec_data *const data = dev->data;

	if ((tcbs != NULL) && (tcbs->write_requested != NULL)) {
		/* ask the application if it can accept data */
		return tcbs->write_requested(data->target_cfg);
	}

	return -ENODATA;
}

/* Handle external I2C controller reading from target (this controller writes data) */
static void target_tx_handler(const struct device *dev, const struct i2c_target_callbacks *tcbs)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	uint8_t val = XEC_I2C_TARGET_DFLT_DATA_VAL;

	/* Did the external controller NAK'd the byte we transmitted */
	if (soc_test_bit8(rb + XEC_I2C_SR_OFS, XEC_I2C_SR_LRB_AD0_POS) != 0) {
		/*
		 * ISSUE: v3.7 and earlier HW will not detect external STOP in
		 * target transmit mode. Enable IDLE interrupt
		 * to catch PIN 0 -> 1 and NBB 0 -> 1.
		 */
		sys_set_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);

		/*
		 * dummy write causes this controller's PIN status
		 * to de-assert 0 -> 1. Data is not transmitted.
		 * SCL is not driven low by this controller.
		 */
		sys_write8(val, rb + XEC_I2C_DATA_OFS);

		data->i2c_status = sys_read8(rb + XEC_I2C_SR_OFS);
	} else {
		/* get data to send from application via callback */
		if ((tcbs != NULL) && (tcbs->read_processed != NULL)) {
			tcbs->read_processed(data->target_cfg, &val);
		}

		sys_write8(val, rb + XEC_I2C_DATA_OFS);
	}
}

/* Handle external I2C controller writing to this target (this controller receives data)
 * Reading the I2C.Data register causes this controller to release SCL. The external Controller
 * senses SCL release and generates clocks for transmitting the next data byte.
 * Reading I2C Data register in receive mode also causes HW to clear PIN status 0 -> 1.
 * We check if the application registered a non-NULL write received callback and pass the
 * the data. If the app callback returns non-zero we should stop accepting data by configuring
 * our controller to NAK future bytes transmitted by the external I2C controller.
 */
static void target_rx_handler(const struct device *dev, const struct i2c_target_callbacks *tcbs)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	uint8_t val = sys_read8(rb + XEC_I2C_DATA_OFS);

	/* if no callback or callback returns non-zero NAK incoming data */
	if ((tcbs == NULL) || (tcbs->write_received == NULL) ||
	    (tcbs->write_received(data->target_cfg, val) != 0)) {
		/* Clear auto-ACK enable bit. This controller will NAK future bytes */
		target_config_for_nack(dev);
	}
}

static void target_addr_handler(const struct device *dev, const struct i2c_target_callbacks *tcbs)
{
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	uint8_t val = XEC_I2C_TARGET_DFLT_DATA_VAL;
	uint8_t rx_data = sys_read8(rb + XEC_I2C_DATA_OFS);

	if ((rx_data & BIT(I2C_READ_WRITE_POS)) != 0) {
		/* target transmitter mode */
		data->target_read = true;

		/* request data from app otherwise send default data value */
		if ((tcbs != NULL) && (tcbs->read_requested != NULL)) {
			tcbs->read_requested(data->target_cfg, &val);
		}

		/*
		 * Writing I2CData causes this HW to release SCL
		 * ending clock stretching. The external Controller
		 * senses SCL released and begins generating clocks
		 * and capturing data driven by this controller
		 * on SDA. External Controller ACK's data until it
		 * wants no more then it will NACK.
		 */
		sys_write8(val, rb + XEC_I2C_DATA_OFS);
	} else {
		/* target receiver mode */
		data->target_read = false;

		/* ask application if it can accept the data byte */
		if (target_wr_req_cb(dev, tcbs) != 0) {
			/* No, configure this controller to NAK */
			target_config_for_nack(dev);
		}
	}
}
#endif

static void i2c_xec_v2_isr(const struct device *dev)
{
#ifdef CONFIG_I2C_TARGET
	struct i2c_xec_data *const data = dev->data;
	const struct i2c_xec_config *drvcfg = dev->config;
	mm_reg_t rb = drvcfg->base_addr;
	struct i2c_target_config *tcfg = data->target_cfg;
	const struct i2c_target_callbacks *tcbs = NULL;
	uint32_t status = 0, compl_status = 0, config = 0;

	/* Get current status */
	status = sys_read8(rb + XEC_I2C_SR_OFS);
	compl_status = sys_read32(rb + XEC_I2C_CMPL_OFS) & XEC_I2C_CMPL_RW1C_MSK;
	config = sys_read32(rb + XEC_I2C_CFG_OFS);

	/* Idle interrupt enabled and active? */
	if (((config & BIT(XEC_I2C_CFG_IDLE_IEN_POS)) != 0) &&
	    ((compl_status & BIT(XEC_I2C_CMPL_IDLE_POS)) != 0)) {
		sys_clear_bit(rb + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);

		if ((status & BIT(XEC_I2C_SR_NBB_POS)) != 0) {
			restart_target(dev);
			goto clear_iag;
		}
	}

	if (data->target_attached == false) {
		goto clear_iag;
	}

	if (tcfg != NULL) {
		tcbs = tcfg->callbacks;
	}

	/* External STOP or Bus Error: restart target handling */
	if ((status & (BIT(XEC_I2C_SR_BER_POS) | BIT(XEC_I2C_SR_STO_POS))) != 0) {
		if ((status & BIT(XEC_I2C_SR_BER_POS)) != 0) {
			data->i2c_error = I2C_XEC_ERR_BUS;
		}
		if ((tcbs != NULL) && (tcbs->stop != NULL)) {
			tcbs->stop(data->target_cfg);
		}

		restart_target(dev);
		goto clear_iag;
	}

	/* Address byte handling. AAT status is only valid if PIN status bit == 0 */
	if ((status & (BIT(XEC_I2C_SR_AAT_POS) | BIT(XEC_I2C_SR_PIN_POS))) ==
	    BIT(XEC_I2C_SR_AAT_POS)) {
		target_addr_handler(dev, tcbs);
		goto clear_iag;
	}

	if (data->target_read == true) { /* Target transmitter mode */
		target_tx_handler(dev, tcbs);
	} else { /* target receiver mode */
		target_rx_handler(dev, tcbs);
	}

clear_iag:
	sys_write32(compl_status, rb + XEC_I2C_CMPL_OFS);
	soc_ecia_girq_status_clear(drvcfg->girq, drvcfg->girq_pos);
#endif
}

#ifdef CONFIG_I2C_TARGET
static int i2c_xec_v2_target_register(const struct device *dev, struct i2c_target_config *config)
{
	const struct i2c_xec_config *drvcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	int ret = 0;

	if (config == NULL) {
		return -EINVAL;
	}

	if (data->target_attached == true) {
		return -EBUSY;
	}

	/* Wait for any outstanding transactions to complete so that
	 * the bus is free
	 */
	ret = wait_bus_free(dev, WAIT_INTERVAL_US);
	if (ret != 0) {
		return ret;
	}

	data->target_cfg = config;

	ret = i2c_xec_reset_config(dev);
	if (ret != 0) {
		return ret;
	}

	restart_target(dev);

	data->target_attached = true;

	/* Clear before enabling girq bit */
	soc_ecia_girq_status_clear(drvcfg->girq, drvcfg->girq_pos);
	soc_ecia_girq_ctrl(drvcfg->girq, drvcfg->girq_pos, 1U);

	return 0;
}

static int i2c_xec_v2_target_unregister(const struct device *dev, struct i2c_target_config *config)
{
	const struct i2c_xec_config *drvcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;

	if (data->target_attached == false) {
		return -EINVAL;
	}

	data->target_cfg = NULL;
	data->target_attached = false;

	soc_ecia_girq_ctrl(drvcfg->girq, drvcfg->girq_pos, 0);
	soc_ecia_girq_status_clear(drvcfg->girq, drvcfg->girq_pos);

	return 0;
}
#endif

static DEVICE_API(i2c, i2c_xec_v2_driver_api) = {
	.configure = i2c_xec_v2_configure,
	.transfer = i2c_xec_v2_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_xec_v2_target_register,
	.target_unregister = i2c_xec_v2_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int i2c_xec_v2_init(const struct device *dev)
{
	const struct i2c_xec_config *drvcfg = dev->config;
	struct i2c_xec_data *const data = dev->data;
	int ret = 0;
	uint32_t bitrate_cfg = 0;

	data->state = I2C_XEC_STATE_STOPPED;
	data->target_cfg = NULL;
	data->target_attached = false;

	ret = pinctrl_apply_state(drvcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC I2C pinctrl setup failed (%d)", ret);
		return ret;
	}

	bitrate_cfg = i2c_map_dt_bitrate(drvcfg->clock_freq);
	if (bitrate_cfg == 0) {
		return -EINVAL;
	}

	soc_xec_pcr_sleep_en_clear(drvcfg->enc_pcr);

	/* Default configuration */
	ret = i2c_xec_v2_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret != 0) {
		return ret;
	}

	k_mutex_init(&data->mux);

#ifdef CONFIG_I2C_TARGET
	if (drvcfg->irq_config_func != NULL) {
		drvcfg->irq_config_func();
	}
#endif
	return 0;
}

#define XEC_I2C_GIRQ_DT(inst, idx)                                                                 \
	(uint8_t)MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, idx))
#define XEC_I2C_GIRQ_POS_DT(inst, idx)                                                             \
	(uint8_t)MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, idx))

#define I2C_XEC_DEVICE(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void i2c_xec_irq_config_func_##n(void);                                             \
	static struct i2c_xec_data i2c_xec_data_##n;                                               \
	static const struct i2c_xec_config i2c_xec_config_##n = {                                  \
		.base_addr = (mm_reg_t)DT_INST_REG_ADDR(n),                                        \
		.port_sel = DT_INST_PROP(n, port_sel),                                             \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.girq = XEC_I2C_GIRQ_DT(n, 0),                                                     \
		.girq_pos = XEC_I2C_GIRQ_POS_DT(n, 0),                                             \
		.enc_pcr = DT_INST_PROP(n, pcr_scr),                                               \
		.irq_config_func = i2c_xec_irq_config_func_##n,                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.sda_gpio = GPIO_DT_SPEC_INST_GET(n, sda_gpios),                                   \
		.scl_gpio = GPIO_DT_SPEC_INST_GET(n, scl_gpios),                                   \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_xec_v2_init, NULL, &i2c_xec_data_##n,                     \
				  &i2c_xec_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,      \
				  &i2c_xec_v2_driver_api);                                         \
                                                                                                   \
	static void i2c_xec_irq_config_func_##n(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_xec_v2_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_XEC_DEVICE)
