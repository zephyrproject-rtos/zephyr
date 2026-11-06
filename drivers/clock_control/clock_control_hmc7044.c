/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HMC7044 dual-PLL clock generator -- Zephyr clock_control driver.
 *
 * A 14-output, dual-PLL, SPI-programmed jitter-attenuating clock generator and
 * JESD204 SYSREF distributor. One device instance per devicetree node; the
 * PLL1/PLL2/VCO/output-divider/SYSREF configuration comes entirely from DT.
 *
 * INIT LEVEL: this is POST_KERNEL, not PRE_KERNEL_1 as clock providers
 * conventionally are, because it is SPI-attached and Zephyr SPI controllers
 * initialise at POST_KERNEL/CONFIG_SPI_INIT_PRIORITY. See the BUILD_ASSERT and
 * the comment at DEVICE_DT_INST_DEFINE below. Consumers of these rates must be
 * POST_KERNEL or later.
 *
 * The HMC7044 has NO chip-ID register, so the bus is proved by writing a
 * known byte to the scratchpad register (0x0008) and reading it back.
 */

#define DT_DRV_COMPAT adi_hmc7044

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#include <stdlib.h>

#include <zephyr/drivers/clock_control/hmc7044.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hmc7044, LOG_LEVEL_INF);


/* HMC7044 command word :
 *   bit 15    : 1 = read, 0 = write
 *   bits 14:13: (count - 1)      -- 1 byte here -> 0
 *   bits 11:0 : register address
 */
#define HMC7044_READ    (1U << 15)
#define HMC7044_WRITE   (0U << 15)
#define HMC7044_CNT(x)  (((x) - 1U) << 13)
#define HMC7044_ADDR(x) ((x) & 0xFFFU)

/* -------------------- HMC7044 registers / bitfields ---------------- */

/* Global Control */
#define HMC7044_REG_SOFT_RESET		0x0000
#define HMC7044_SOFT_RESET		BIT(0)

#define HMC7044_REG_REQ_MODE_0		0x0001
#define HMC7044_RESEED_REQ		BIT(7)
#define HMC7044_HIGH_PERF_DISTRIB_PATH	BIT(6)
#define HMC7044_HIGH_PERF_PLL_VCO	BIT(5)
#define HMC7044_FORCE_HOLDOVER		BIT(4)
#define HMC7044_MUTE_OUT_DIV		BIT(3)
#define HMC7044_PULSE_GEN_REQ		BIT(2)
#define HMC7044_RESTART_DIV_FSM		BIT(1)
#define HMC7044_SLEEP_MODE		BIT(0)

#define HMC7044_REG_EN_CTRL_0		0x0003
#define HMC7044_RF_RESEEDER_EN		BIT(5)
#define HMC7044_VCO_SEL(x)		(((x) & 0x3) << 3)
#define HMC7044_VCO_EXT			0
#define HMC7044_VCO_HIGH		1
#define HMC7044_VCO_LOW			2
#define HMC7044_SYSREF_TIMER_EN		BIT(2)
#define HMC7044_PLL2_EN			BIT(1)
#define HMC7044_PLL1_EN			BIT(0)

#define HMC7044_REG_GLOB_MODE		0x0005
#define HMC7044_REF_PATH_EN(x)		((x) & 0xf)
#define HMC7044_RFSYNC_EN		BIT(4)
#define HMC7044_VCOIN_MODE_EN		BIT(5)
#define HMC7044_SYNC_PIN_MODE(x)	(((x) & 0x3) << 6)

#define HMC7044_REG_SCRATCHPAD		0x0008
#define HMC7044_SCRATCH_PATTERN		0xAD

/* PLL1 */
#define HMC7044_REG_CLKIN0_BUF_CTRL	0x000A
#define HMC7044_REG_CLKIN1_BUF_CTRL	0x000B
#define HMC7044_REG_CLKIN2_BUF_CTRL	0x000C
#define HMC7044_REG_CLKIN3_BUF_CTRL	0x000D
#define HMC7044_REG_OSCIN_BUF_CTRL	0x000E

#define HMC7044_REG_PLL1_REF_PRIO_CTRL	0x0014

#define HMC7044_REG_PLL1_CP_CTRL	0x001A
#define HMC7044_PLL1_CP_CURRENT(x)	((x) & 0xf)

#define HMC7044_REG_CLKIN_PRESCALER(x)	(0x001C + (x))
#define HMC7044_REG_OSCIN_PRESCALER	0x0020

#define HMC7044_REG_PLL1_R_LSB		0x0021
#define HMC7044_REG_PLL1_R_MSB		0x0022
#define HMC7044_REG_PLL1_N_LSB		0x0026
#define HMC7044_REG_PLL1_N_MSB		0x0027

#define HMC7044_REG_PLL1_LOCK_DETECT	0x0028
#define HMC7044_LOCK_DETECT_TIMER(x)	((x) & 0x1f)

#define HMC7044_REG_PLL1_REF_SWITCH	0x0029
#define HMC7044_HOLDOVER_DAC		BIT(2)
#define HMC7044_AUTO_REVERT_SWITCH	BIT(1)
#define HMC7044_AUTO_MODE_SWITCH	BIT(0)

/* PLL2 */
#define HMC7044_REG_PLL2_FREQ_DOUBLER	0x0032
#define HMC7044_PLL2_FREQ_DOUBLER_DIS	BIT(0)

#define HMC7044_REG_PLL2_R_LSB		0x0033
#define HMC7044_R2_LSB(x)		((x) & 0xff)
#define HMC7044_REG_PLL2_R_MSB		0x0034
#define HMC7044_R2_MSB(x)		(((x) & 0xf00) >> 8)
#define HMC7044_REG_PLL2_N_LSB		0x0035
#define HMC7044_N2_LSB(x)		((x) & 0xff)
#define HMC7044_REG_PLL2_N_MSB		0x0036
#define HMC7044_N2_MSB(x)		(((x) & 0xff00) >> 8)

/* GPIO/SDATA Control */
#define HMC7044_REG_GPI_CTRL(x)		(0x0046 + (x))
#define HMC7044_REG_GPO_CTRL(x)		(0x0050 + (x))

/* SYSREF/SYNC Control */
#define HMC7044_REG_PULSE_GEN		0x005A
#define HMC7044_PULSE_GEN_MODE(x)	((x) & 0x7)

#define HMC7044_REG_SYNC		0x005B
#define HMC7044_SYNC_RETIME		BIT(2)

#define HMC7044_REG_SYSREF_TIMER_LSB	0x005C
#define HMC7044_SYSREF_TIMER_LSB(x)	((x) & 0xff)
#define HMC7044_REG_SYSREF_TIMER_MSB	0x005D
#define HMC7044_SYSREF_TIMER_MSB(x)	(((x) & 0xf00) >> 8)

#define HMC7044_CLK_INPUT_CTRL		0x0064
#define HMC7044_LOW_FREQ_INPUT_MODE	BIT(0)

/* Status and Alarm readback */
#define HMC7044_REG_ALARM_READBACK	0x007D
#define HMC7044_REG_PLL1_STATUS		0x0082

#define HMC7044_PLL1_FSM_STATE(x)	((x) & 0x7)
#define HMC7044_PLL1_ACTIVE_CLKIN(x)	(((x) >> 3) & 0x3)
#define HMC7044_PLL2_LOCK_DETECT(x)	((x) & 0x1)

/* PLL1 FSM state 2 is "Locked". */
#define HMC7044_PLL1_FSM_LOCKED		2

/* Other Controls */
#define HMC7044_REG_CLK_OUT_DRV_LOW_PW	0x009F
#define HMC7044_REG_CLK_OUT_DRV_HIGH_PW	0x00A0
#define HMC7044_REG_PLL1_DELAY		0x00A5
#define HMC7044_REG_PLL1_HOLDOVER	0x00A8
#define HMC7044_REG_VTUNE_PRESET	0x00B0

/* Clock Distribution */
#define HMC7044_REG_CH_OUT_CRTL_0(ch)	(0x00C8 + 0xA * (ch))
#define HMC7044_HI_PERF_MODE		BIT(7)
#define HMC7044_SYNC_EN			BIT(6)
#define HMC7044_CH_OUT_RESERVED_SET	BIT(4)	/* reserved, POR default 1 */
#define HMC7044_START_UP_MODE_DYN_EN	(BIT(3) | BIT(2))
#define HMC7044_CH_EN			BIT(0)

#define HMC7044_REG_CH_OUT_CRTL_1(ch)	(0x00C9 + 0xA * (ch))
#define HMC7044_DIV_LSB(x)		((x) & 0xFF)
#define HMC7044_REG_CH_OUT_CRTL_2(ch)	(0x00CA + 0xA * (ch))
#define HMC7044_DIV_MSB(x)		(((x) >> 8) & 0xFF)
#define HMC7044_REG_CH_OUT_CRTL_3(ch)	(0x00CB + 0xA * (ch))
#define HMC7044_FINE_ANALOG_DELAY(x)	((x) & 0x1F)
#define HMC7044_REG_CH_OUT_CRTL_4(ch)	(0x00CC + 0xA * (ch))
#define HMC7044_COARSE_DIGITAL_DELAY(x)	((x) & 0x1F)
#define HMC7044_REG_CH_OUT_CRTL_7(ch)	(0x00CF + 0xA * (ch))
#define HMC7044_OUT_MUX_SEL(x)		((x) & 0x3)
#define HMC7044_REG_CH_OUT_CRTL_8(ch)	(0x00D0 + 0xA * (ch))
#define HMC7044_DRIVER_MODE(x)		(((x) & 0x3) << 3)
#define HMC7044_DRIVER_Z_MODE(x)	(((x) & 0x3) << 0)
#define HMC7044_DYN_DRIVER_EN		BIT(5)
#define HMC7044_FORCE_MUTE_EN		BIT(7)

#define HMC7044_NUM_CHAN	HMC7044_NUM_CLK_OUT

#define HMC7044_LOW_VCO_MIN	2150000
#define HMC7044_LOW_VCO_MAX	2880000
#define HMC7044_HIGH_VCO_MIN	2650000
#define HMC7044_HIGH_VCO_MAX	3200000

#define HMC7044_RECOMM_LCM_MIN	30000
#define HMC7044_RECOMM_LCM_MAX	70000
#define HMC7044_RECOMM_PFD1	10000
#define HMC7044_MIN_PFD1	1
#define HMC7044_MAX_PFD1	50000

#define HMC7044_CP_CURRENT_STEP	120
#define HMC7044_CP_CURRENT_MIN	120
#define HMC7044_CP_CURRENT_MAX	1920
#define HMC7044_CP_CURRENT_DEF	1080

#define HMC7044_R1_MAX		65535
#define HMC7044_N1_MAX		65535

#define HMC7044_R2_MIN		1
#define HMC7044_R2_MAX		4095
#define HMC7044_N2_MIN		8
#define HMC7044_N2_MAX		65535

#define HMC7044_OUT_DIV_MIN	1
#define HMC7044_OUT_DIV_MAX	4094

static const char *const pll1_fsm_states[] = {
	"Reset",
	"Acquisition",
	"Locked",
	"Invalid",
	"Holdover",
	"DAC assisted holdover exit",
	"Invalid",
	"Invalid",
};

/* --------------------------- config / data -------------------------------- */

/* One output, entirely from devicetree -- no field is written at runtime. */
struct hmc7044_chan_config {
	uint8_t num;
	const char *name;
	uint16_t divider;
	uint8_t driver_mode;
	uint8_t driver_impedance;
	uint8_t coarse_delay;
	uint8_t fine_delay;
	uint8_t out_mux_mode;
	bool high_performance_mode_dis;
	bool start_up_mode_dynamic_enable;
	bool dynamic_driver_enable;
	bool force_mute_enable;
	bool is_sysref;
};

struct hmc7044_config {
	struct spi_dt_spec spi;

	uint32_t clkin_freq[4];
	uint32_t vcxo_freq;
	uint32_t pll2_freq;
	uint32_t pll1_loop_bw;
	/* pfd1_limit in kHz, already clamped */
	uint32_t pfd1_limit;
	uint32_t pll1_cp_current;
	uint32_t sysref_timer_div;
	uint32_t pulse_gen_mode;
	uint8_t pll1_ref_prio_ctrl;
	uint8_t sync_pin_mode;
	bool pll1_ref_autorevert_en;
	bool clkin0_rfsync_en;
	bool clkin1_vcoin_en;
	bool high_performance_mode_clock_dist_en;
	bool rf_reseeder_en;
	uint8_t in_buf_mode[5];
	uint8_t gpi_ctrl[4];
	uint8_t gpo_ctrl[4];

	const struct hmc7044_chan_config *channels;
	uint8_t num_channels;
};

struct hmc7044_data {
	/* Solved at init; reported by hmc7044_get_status(). PFD1 rate in kHz. */
	uint32_t pll1_pfd;
	/* Set by the scratchpad check. Gates read-modify-write on this chip. */
	bool read_write_confirmed;
	/* Bit N set once output N has been programmed and enabled. */
	uint16_t enabled;
};

/* ------------------------------- helper math ------------------------------ */

/* Reduce num/den to lowest terms; zero both outputs if they exceed the caps. */
static void hmc7044_rational_best_approximation(uint32_t num, uint32_t den,
						uint32_t max_num, uint32_t max_den,
						uint32_t *best_num, uint32_t *best_den)
{
	uint32_t gcd = sys_gcd(num, den);

	*best_num = num / gcd;
	*best_den = den / gcd;

	if ((*best_num > max_num) || (*best_den > max_den)) {
		*best_num = 0;
		*best_den = 0;
	}
}

/* ------------------------------ SPI helpers ------------------------------- */

/* Write one 8-bit register: [ cmd_hi, cmd_lo, val ]. */
static int hmc7044_write(const struct device *dev, uint16_t reg, uint8_t val)
{
	const struct hmc7044_config *config = dev->config;
	uint16_t cmd = HMC7044_WRITE | HMC7044_CNT(1) | HMC7044_ADDR(reg);
	uint8_t tx[3] = {
		cmd >> 8,
		cmd & 0xFF,
		val,
	};
	const struct spi_buf txb = { .buf = tx, .len = sizeof(tx) };
	const struct spi_buf_set txs = { .buffers = &txb, .count = 1 };

	return spi_write_dt(&config->spi, &txs);
}

/* Read one 8-bit register. Data returns in the third byte. */
static int hmc7044_read(const struct device *dev, uint16_t reg, uint8_t *val)
{
	const struct hmc7044_config *config = dev->config;
	uint16_t cmd = HMC7044_READ | HMC7044_CNT(1) | HMC7044_ADDR(reg);
	uint8_t tx[3] = {
		cmd >> 8,
		cmd & 0xFF,
		0x00,
	};
	uint8_t rx[3] = { 0 };
	const struct spi_buf txb = { .buf = tx, .len = sizeof(tx) };
	const struct spi_buf rxb = { .buf = rx, .len = sizeof(rx) };
	const struct spi_buf_set txs = { .buffers = &txb, .count = 1 };
	const struct spi_buf_set rxs = { .buffers = &rxb, .count = 1 };
	int ret;

	ret = spi_transceive_dt(&config->spi, &txs, &rxs);
	if (ret == 0) {
		*val = rx[2];
	}
	return ret;
}

/*
 * Confirm the SPI path reads back what it writes, via the scratchpad register.
 * A bus transfer error is a hard failure and propagates; a successful transfer
 * that returns the wrong value only means reads are unreliable, so the chip is
 * driven write-only and this returns 0.
 */
static int hmc7044_read_write_check(const struct device *dev)
{
	struct hmc7044_data *data = dev->data;
	uint8_t val = 0;
	int ret;

	ret = hmc7044_write(dev, HMC7044_REG_SCRATCHPAD, HMC7044_SCRATCH_PATTERN);
	if (ret < 0) {
		return ret;
	}

	ret = hmc7044_read(dev, HMC7044_REG_SCRATCHPAD, &val);
	if (ret < 0) {
		return ret;
	}

	data->read_write_confirmed = (val == HMC7044_SCRATCH_PATTERN);

	if (!data->read_write_confirmed) {
		LOG_WRN("Read/Write check failed (0x%X)", val);
	}

	return 0;
}

/* Set the masked bits, then clear them, with an optional settling delay. */
static int hmc7044_toggle_bit(const struct device *dev, uint16_t reg,
			      uint8_t mask, uint32_t us_delay)
{
	struct hmc7044_data *data = dev->data;
	uint8_t val;
	int ret;

	if (data->read_write_confirmed) {
		ret = hmc7044_read(dev, reg, &val);
		if (ret < 0) {
			return ret;
		}
	} else {
		val = 0;
	}

	ret = hmc7044_write(dev, reg, val | mask);
	if (ret < 0) {
		return ret;
	}

	val &= ~mask;

	ret = hmc7044_write(dev, reg, val);
	if (ret < 0) {
		return ret;
	}

	if (us_delay) {
		k_busy_wait(us_delay);
	}

	return 0;
}

/* ------------------------------ channels ---------------------------------- */

static const struct hmc7044_chan_config *hmc7044_chan_find(const struct device *dev,
							   uint8_t num)
{
	const struct hmc7044_config *config = dev->config;

	for (uint8_t i = 0; i < config->num_channels; i++) {
		if (config->channels[i].num == num) {
			return &config->channels[i];
		}
	}

	return NULL;
}

/*
 * Write a channel's CH_OUT_CRTL_0. This register carries the enable bit plus
 * the mode bits, so enabling or disabling an output means rewriting the whole
 * word from the devicetree configuration rather than a read-modify-write --
 * which also keeps clock_control_on() idempotent.
 */
static int hmc7044_chan_set_enable(const struct device *dev,
				   const struct hmc7044_chan_config *chan, bool on)
{
	struct hmc7044_data *data = dev->data;
	int ret;

	ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_0(chan->num),
			    (chan->start_up_mode_dynamic_enable ?
			     HMC7044_START_UP_MODE_DYN_EN : 0) |
			    HMC7044_CH_OUT_RESERVED_SET |
			    (chan->high_performance_mode_dis ?
			     0 : HMC7044_HI_PERF_MODE) | HMC7044_SYNC_EN |
			    (on ? HMC7044_CH_EN : 0));
	if (ret) {
		return ret;
	}

	WRITE_BIT(data->enabled, chan->num, on);

	return 0;
}

/* --------------------------- clock_control ops ---------------------------- */

/*
 * Translate a subsystem handle to an output config. HMC7044_CLK_OUT() biases the
 * output number by one so that output 0 is distinguishable from
 * CLOCK_CONTROL_SUBSYS_ALL, which is NULL.
 */
static int hmc7044_subsys_to_chan(const struct device *dev,
				  clock_control_subsys_t sys,
				  const struct hmc7044_chan_config **chan)
{
	uintptr_t handle = (uintptr_t)sys;

	if (handle == 0 || handle > HMC7044_NUM_CLK_OUT) {
		return -EINVAL;
	}

	*chan = hmc7044_chan_find(dev, (uint8_t)(handle - 1));
	if (*chan == NULL) {
		/* A real output number, but it has no child node in DT. */
		return -ENODEV;
	}

	return 0;
}

static int hmc7044_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct hmc7044_chan_config *chan;
	int ret;

	ret = hmc7044_subsys_to_chan(dev, sys, &chan);
	if (ret) {
		return ret;
	}

	return hmc7044_chan_set_enable(dev, chan, true);
}

static int hmc7044_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct hmc7044_chan_config *chan;
	int ret;

	ret = hmc7044_subsys_to_chan(dev, sys, &chan);
	if (ret) {
		return ret;
	}

	return hmc7044_chan_set_enable(dev, chan, false);
}

static int hmc7044_clk_get_rate(const struct device *dev,
				clock_control_subsys_t sys, uint32_t *rate)
{
	const struct hmc7044_config *config = dev->config;
	const struct hmc7044_chan_config *chan;
	int ret;

	if (rate == NULL) {
		return -EINVAL;
	}

	/*
	 * There is no single rate for the whole chip -- 14 outputs run at
	 * pll2_freq/divider. Report the PLL2 VCO frequency for SUBSYS_ALL, which
	 * is the one rate every output derives from.
	 */
	if (sys == CLOCK_CONTROL_SUBSYS_ALL) {
		*rate = config->pll2_freq;
		return 0;
	}

	ret = hmc7044_subsys_to_chan(dev, sys, &chan);
	if (ret) {
		return ret;
	}

	*rate = config->pll2_freq / chan->divider;

	return 0;
}

/*
 * Status semantics: an output is ON when it has been programmed and enabled AND
 * both PLLs are locked. A divider running off an unlocked PLL2 is producing
 * something, but not the rate get_rate() reports, so reporting it as ON would be
 * a lie in exactly the case a caller is checking for.
 */
static enum clock_control_status hmc7044_clk_get_status(const struct device *dev,
							clock_control_subsys_t sys)
{
	struct hmc7044_data *data = dev->data;
	const struct hmc7044_chan_config *chan;
	struct hmc7044_status status;

	if (sys != CLOCK_CONTROL_SUBSYS_ALL) {
		int ret = hmc7044_subsys_to_chan(dev, sys, &chan);

		if (ret) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}

		if ((data->enabled & BIT(chan->num)) == 0) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	}

	if (hmc7044_get_status(dev, &status) != 0) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (!data->read_write_confirmed) {
		/* Write-only bus: the chip is programmed but unverifiable. */
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (status.pll1_locked && status.pll2_locked) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	/*
	 * PLL1 reset/acquisition and PLL2 calibration are transient states that
	 * resolve without further programming -- report STATUS_STARTING. Holdover
	 * (state > locked) means PLL1 lost its reference and is not starting up.
	 */
	if (status.pll1_fsm_state <= HMC7044_PLL1_FSM_LOCKED) {
		return CLOCK_CONTROL_STATUS_STARTING;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, hmc7044_api) = {
	.on = hmc7044_clk_on,
	.off = hmc7044_clk_off,
	.get_rate = hmc7044_clk_get_rate,
	.get_status = hmc7044_clk_get_status,
};

/* ------------------------------ extension API ----------------------------- */

int hmc7044_get_status(const struct device *dev, struct hmc7044_status *status)
{
	const struct hmc7044_config *config;
	struct hmc7044_data *data;
	uint8_t alarm_stat = 0, pll1_stat = 0;
	uint8_t fsm_state;
	int ret;

	if (dev == NULL || status == NULL) {
		return -EINVAL;
	}
	if (!DEVICE_API_IS(clock_control, dev) || dev->api != &hmc7044_api) {
		return -EINVAL;
	}

	config = dev->config;
	data = dev->data;

	if (!data->read_write_confirmed) {
		/*
		 * Reads are not coming back, so PLL state is unknowable. Report
		 * what is still true -- the programmed frequencies -- and leave
		 * both locked flags false rather than inventing a state.
		 */
		*status = (struct hmc7044_status){
			.pll1_fsm_state_str = "unreadable",
			.pll1_pfd_khz = data->pll1_pfd,
			.pll2_freq = config->pll2_freq,
		};
		return 0;
	}

	ret = hmc7044_read(dev, HMC7044_REG_PLL1_STATUS, &pll1_stat);
	if (ret < 0) {
		return ret;
	}

	if (HMC7044_PLL1_FSM_STATE(pll1_stat) != HMC7044_PLL1_FSM_LOCKED) {
		/*
		 * Give PLL1 one loop-bandwidth-derived settling window before
		 * calling it unlocked.
		 */
		k_msleep(DIV_ROUND_UP(5000, config->pll1_loop_bw));

		ret = hmc7044_read(dev, HMC7044_REG_PLL1_STATUS, &pll1_stat);
		if (ret < 0) {
			return ret;
		}
	}

	ret = hmc7044_read(dev, HMC7044_REG_ALARM_READBACK, &alarm_stat);
	if (ret < 0) {
		return ret;
	}

	fsm_state = HMC7044_PLL1_FSM_STATE(pll1_stat);

	status->pll1_fsm_state = fsm_state;
	status->pll1_fsm_state_str = pll1_fsm_states[fsm_state];
	status->pll1_active_clkin = HMC7044_PLL1_ACTIVE_CLKIN(pll1_stat);
	status->pll1_active_clkin_freq = config->clkin_freq[status->pll1_active_clkin];
	status->pll1_pfd_khz = data->pll1_pfd;
	status->pll1_locked = (fsm_state == HMC7044_PLL1_FSM_LOCKED);
	status->pll2_locked = HMC7044_PLL2_LOCK_DETECT(alarm_stat) != 0;
	status->pll2_freq = config->pll2_freq;

	return 0;
}

int hmc7044_sysref_request(const struct device *dev)
{
	if (dev == NULL || !DEVICE_API_IS(clock_control, dev) ||
	    dev->api != &hmc7044_api) {
		return -EINVAL;
	}

	/*
	 * Toggle PULSE_GEN_REQ with no delay. The pulse count comes from
	 * adi,pulse-generator-mode.
	 */
	return hmc7044_toggle_bit(dev, HMC7044_REG_REQ_MODE_0,
				  HMC7044_PULSE_GEN_REQ, 0);
}

/* -------------------------------- setup ----------------------------------- */

/* Log PLL1/PLL2 lock state and reference selection at INFO level. */
static int hmc7044_log_status(const struct device *dev)
{
	struct hmc7044_data *data = dev->data;
	struct hmc7044_status status;
	int ret;

	if (!data->read_write_confirmed) {
		LOG_INF("Probed, SPI read support failed");
		return 0;
	}

	ret = hmc7044_get_status(dev, &status);
	if (ret) {
		return ret;
	}

	LOG_INF("PLL1: %s, CLKIN%u @ %u Hz, PFD: %u kHz - PLL2: %s @ %u.%06u MHz",
		status.pll1_fsm_state_str, status.pll1_active_clkin,
		status.pll1_active_clkin_freq, status.pll1_pfd_khz,
		status.pll2_locked ? "Locked" : "Unlocked",
		status.pll2_freq / 1000000, status.pll2_freq % 1000000);

	return 0;
}

/* Program the full PLL1/PLL2/VCO/output-divider/SYSREF sequence. */
static int hmc7044_setup(const struct device *dev)
{
	const struct hmc7044_config *config = dev->config;
	struct hmc7044_data *data = dev->data;
	const struct hmc7044_chan_config *chan;
	bool high_vco_en;
	bool pll2_freq_doubler_en;
	uint32_t vcxo_freq, pll2_freq;
	uint32_t clkin_freq[4];
	uint32_t lcm_freq;
	uint32_t in_prescaler[5];
	uint32_t pll1_lock_detect;
	uint32_t n1, r1;
	uint32_t n, r;
	uint32_t pfd1_freq;
	uint32_t vco_limit;
	uint32_t n2[2], r2[2];
	uint32_t i, ref_en = 0;
	int ret;

	vcxo_freq = config->vcxo_freq / 1000;
	pll2_freq = config->pll2_freq / 1000;

	lcm_freq = vcxo_freq;
	for (i = 0; i < ARRAY_SIZE(clkin_freq); i++) {
		clkin_freq[i] = config->clkin_freq[i] / 1000;

		if (clkin_freq[i]) {
			lcm_freq = sys_gcd(clkin_freq[i], lcm_freq);
			ref_en |= BIT(i);
		}
	}

	while (lcm_freq > HMC7044_RECOMM_LCM_MAX) {
		lcm_freq /= 2;
	}

	for (i = 0; i < ARRAY_SIZE(clkin_freq); i++) {
		if (clkin_freq[i]) {
			in_prescaler[i] = clkin_freq[i] / lcm_freq;
		} else {
			in_prescaler[i] = 1;
		}
	}
	in_prescaler[4] = vcxo_freq / lcm_freq;

	pll1_lock_detect = LOG2((lcm_freq * 4000) / config->pll1_loop_bw);

	/* fVCXO / N1 = fLCM / R1 */
	hmc7044_rational_best_approximation(vcxo_freq, lcm_freq,
					    HMC7044_N1_MAX, HMC7044_R1_MAX,
					    &n1, &r1);

	pfd1_freq = vcxo_freq / n1;

	n = n1;
	r = r1;
	while (pfd1_freq > config->pfd1_limit) {
		do {
			n++;
		} while (((vcxo_freq % n) || (lcm_freq * n % vcxo_freq)) &&
			 (n <= HMC7044_N1_MAX));
		r = lcm_freq * n / vcxo_freq;

		if ((n > HMC7044_N1_MAX) || (r > HMC7044_R1_MAX)) {
			break;
		}

		n1 = n;
		r1 = r;
		pfd1_freq = vcxo_freq / n1;
	}

	data->pll1_pfd = pfd1_freq;

	if (pll2_freq < HMC7044_LOW_VCO_MIN || pll2_freq > HMC7044_HIGH_VCO_MAX) {
		LOG_ERR("adi,pll2-output-frequency %u Hz is outside both VCO ranges",
			config->pll2_freq);
		return -EINVAL;
	}

	vco_limit = (HMC7044_LOW_VCO_MAX + HMC7044_HIGH_VCO_MIN) / 2;
	high_vco_en = (pll2_freq >= vco_limit);

	/* fVCO / N2 = fVCXO * doubler / R2 */
	pll2_freq_doubler_en = true;
	hmc7044_rational_best_approximation(pll2_freq, vcxo_freq * 2,
					    HMC7044_N2_MAX, HMC7044_R2_MAX,
					    &n2[0], &r2[0]);

	if (pll2_freq != vcxo_freq * n2[0] / r2[0]) {
		hmc7044_rational_best_approximation(pll2_freq, vcxo_freq,
						    HMC7044_N2_MAX, HMC7044_R2_MAX,
						    &n2[1], &r2[1]);

		if (abs((int)pll2_freq - (int)(vcxo_freq * 2 * n2[0] / r2[0])) >
		    abs((int)pll2_freq - (int)(vcxo_freq * n2[1] / r2[1]))) {
			n2[0] = n2[1];
			r2[0] = r2[1];
			pll2_freq_doubler_en = false;
		}
	}

	while ((n2[0] < HMC7044_N2_MIN) && (r2[0] <= HMC7044_R2_MAX / 2)) {
		n2[0] *= 2;
		r2[0] *= 2;
	}
	if (n2[0] < HMC7044_N2_MIN) {
		LOG_ERR("no valid PLL2 N2/R2 pair for %u Hz from a %u Hz VCXO",
			config->pll2_freq, config->vcxo_freq);
		return -EINVAL;
	}

	LOG_INF("PLL1 N1=%u R1=%u PFD1=%u kHz; PLL2 N2=%u R2=%u doubler=%d VCO=%s",
		n1, r1, pfd1_freq, n2[0], r2[0], pll2_freq_doubler_en,
		high_vco_en ? "high" : "low");

	/* Resets all registers to default values */
	ret = hmc7044_toggle_bit(dev, HMC7044_REG_SOFT_RESET,
				 HMC7044_SOFT_RESET, 100);
	if (ret) {
		return ret;
	}

	ret = hmc7044_read_write_check(dev);
	if (ret) {
		return ret;
	}

	/* Disable all channels */
	data->enabled = 0;
	for (i = 0; i < HMC7044_NUM_CHAN; i++) {
		ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_0(i), 0);
		if (ret) {
			return ret;
		}
	}

	/* Load the configuration updates (provided by Analog Devices) */
	ret = hmc7044_write(dev, HMC7044_REG_CLK_OUT_DRV_LOW_PW, 0x4d);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_CLK_OUT_DRV_HIGH_PW, 0xdf);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_DELAY, 0x06);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_HOLDOVER, 0x06);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_VTUNE_PRESET, 0x04);
	if (ret) {
		return ret;
	}

	ret = hmc7044_write(dev, HMC7044_REG_GLOB_MODE,
			    HMC7044_SYNC_PIN_MODE(config->sync_pin_mode) |
			    (config->clkin0_rfsync_en ? HMC7044_RFSYNC_EN : 0) |
			    (config->clkin1_vcoin_en ? HMC7044_VCOIN_MODE_EN : 0) |
			    HMC7044_REF_PATH_EN(ref_en));
	if (ret) {
		return ret;
	}

	/* Program PLL2 -- select the VCO range */
	ret = hmc7044_write(dev, HMC7044_REG_EN_CTRL_0,
			    (config->rf_reseeder_en ? HMC7044_RF_RESEEDER_EN : 0) |
			    HMC7044_VCO_SEL(high_vco_en ? HMC7044_VCO_HIGH :
					    HMC7044_VCO_LOW) |
			    HMC7044_SYSREF_TIMER_EN | HMC7044_PLL2_EN |
			    HMC7044_PLL1_EN);
	if (ret) {
		return ret;
	}

	/* Program the PLL2 dividers */
	ret = hmc7044_write(dev, HMC7044_REG_PLL2_R_LSB, HMC7044_R2_LSB(r2[0]));
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL2_R_MSB, HMC7044_R2_MSB(r2[0]));
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL2_N_LSB, HMC7044_N2_LSB(n2[0]));
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL2_N_MSB, HMC7044_N2_MSB(n2[0]));
	if (ret) {
		return ret;
	}

	/* Program the reference doubler */
	ret = hmc7044_write(dev, HMC7044_REG_PLL2_FREQ_DOUBLER,
			    pll2_freq_doubler_en ? 0 : HMC7044_PLL2_FREQ_DOUBLER_DIS);
	if (ret) {
		return ret;
	}

	/* Program PLL1 */
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_CP_CTRL,
			    HMC7044_PLL1_CP_CURRENT(config->pll1_cp_current /
						    HMC7044_CP_CURRENT_STEP - 1));
	if (ret) {
		return ret;
	}
	/* Set the lock detect timer threshold */
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_LOCK_DETECT,
			    HMC7044_LOCK_DETECT_TIMER(pll1_lock_detect));
	if (ret) {
		return ret;
	}

	/* Set the LCM */
	for (i = 0; i < ARRAY_SIZE(clkin_freq); i++) {
		ret = hmc7044_write(dev, HMC7044_REG_CLKIN_PRESCALER(i),
				    in_prescaler[i]);
		if (ret) {
			return ret;
		}
	}
	ret = hmc7044_write(dev, HMC7044_REG_OSCIN_PRESCALER, in_prescaler[4]);
	if (ret) {
		return ret;
	}

	/*
	 * Program the PLL1 dividers. The PLL2 R2_/N2_ field macros are reused
	 * deliberately -- PLL1 R1/N1 share the same LSB/MSB register split.
	 */
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_R_LSB, HMC7044_R2_LSB(r1));
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_R_MSB, HMC7044_R2_MSB(r1));
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_N_LSB, HMC7044_N2_LSB(n1));
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_N_MSB, HMC7044_N2_MSB(n1));
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_REF_PRIO_CTRL,
			    config->pll1_ref_prio_ctrl);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_PLL1_REF_SWITCH,
			    HMC7044_HOLDOVER_DAC |
			    (config->pll1_ref_autorevert_en ?
			     HMC7044_AUTO_REVERT_SWITCH : 0) |
			    HMC7044_AUTO_MODE_SWITCH);
	if (ret) {
		return ret;
	}

	/* Program the SYSREF timer divide ratio */
	ret = hmc7044_write(dev, HMC7044_REG_SYSREF_TIMER_LSB,
			    HMC7044_SYSREF_TIMER_LSB(config->sysref_timer_div));
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_SYSREF_TIMER_MSB,
			    HMC7044_SYSREF_TIMER_MSB(config->sysref_timer_div));
	if (ret) {
		return ret;
	}

	/* Set the pulse generator mode configuration */
	ret = hmc7044_write(dev, HMC7044_REG_PULSE_GEN,
			    HMC7044_PULSE_GEN_MODE(config->pulse_gen_mode));
	if (ret) {
		return ret;
	}

	/* Enable the input buffers */
	ret = hmc7044_write(dev, HMC7044_REG_CLKIN0_BUF_CTRL, config->in_buf_mode[0]);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_CLKIN1_BUF_CTRL, config->in_buf_mode[1]);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_CLKIN2_BUF_CTRL, config->in_buf_mode[2]);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_CLKIN3_BUF_CTRL, config->in_buf_mode[3]);
	if (ret) {
		return ret;
	}
	ret = hmc7044_write(dev, HMC7044_REG_OSCIN_BUF_CTRL, config->in_buf_mode[4]);
	if (ret) {
		return ret;
	}

	/* Set GPIOs */
	for (i = 0; i < ARRAY_SIZE(config->gpi_ctrl); i++) {
		ret = hmc7044_write(dev, HMC7044_REG_GPI_CTRL(i), config->gpi_ctrl[i]);
		if (ret) {
			return ret;
		}
	}
	for (i = 0; i < ARRAY_SIZE(config->gpo_ctrl); i++) {
		ret = hmc7044_write(dev, HMC7044_REG_GPO_CTRL(i), config->gpo_ctrl[i]);
		if (ret) {
			return ret;
		}
	}

	k_msleep(10);

	/* Program the output channels */
	for (i = 0; i < config->num_channels; i++) {
		chan = &config->channels[i];

		ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_1(chan->num),
				    HMC7044_DIV_LSB(chan->divider));
		if (ret) {
			return ret;
		}
		ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_2(chan->num),
				    HMC7044_DIV_MSB(chan->divider));
		if (ret) {
			return ret;
		}
		ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_8(chan->num),
				    HMC7044_DRIVER_MODE(chan->driver_mode) |
				    HMC7044_DRIVER_Z_MODE(chan->driver_impedance) |
				    (chan->dynamic_driver_enable ?
				     HMC7044_DYN_DRIVER_EN : 0) |
				    (chan->force_mute_enable ?
				     HMC7044_FORCE_MUTE_EN : 0));
		if (ret) {
			return ret;
		}
		ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_3(chan->num),
				    HMC7044_FINE_ANALOG_DELAY(chan->fine_delay));
		if (ret) {
			return ret;
		}
		ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_4(chan->num),
				    HMC7044_COARSE_DIGITAL_DELAY(chan->coarse_delay));
		if (ret) {
			return ret;
		}
		ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_7(chan->num),
				    HMC7044_OUT_MUX_SEL(chan->out_mux_mode));
		if (ret) {
			return ret;
		}

		ret = hmc7044_chan_set_enable(dev, chan, true);
		if (ret) {
			return ret;
		}

		LOG_DBG("out%u %-14s div=%-4u -> %u Hz%s", chan->num,
			chan->name ? chan->name : "", chan->divider,
			config->pll2_freq / chan->divider,
			chan->is_sysref ? " (SYSREF)" : "");
	}
	k_msleep(10);

	/* Do a restart to reset the system and initiate calibration */
	ret = hmc7044_toggle_bit(dev, HMC7044_REG_REQ_MODE_0,
				 HMC7044_RESTART_DIV_FSM, 10000);
	if (ret) {
		return ret;
	}

	ret = hmc7044_toggle_bit(dev, HMC7044_REG_REQ_MODE_0,
				 HMC7044_RESEED_REQ, 1000);
	if (ret) {
		return ret;
	}

	ret = hmc7044_write(dev, HMC7044_REG_REQ_MODE_0,
			    (config->high_performance_mode_clock_dist_en ?
			     HMC7044_HIGH_PERF_DISTRIB_PATH : 0));
	if (ret) {
		return ret;
	}

	return hmc7044_log_status(dev);
}

/* --------------------------------- init ----------------------------------- */

/*
 * Prove the SPI path before programming anything. The HMC7044 has no chip-ID
 * register, so this writes 0xAD to the scratchpad and reads it back.
 */
static int hmc7044_probe(const struct device *dev)
{
	const struct hmc7044_config *config = dev->config;
	uint8_t val = 0;
	int ret;

	LOG_INF("HMC7044 scratchpad probe over %s", config->spi.bus->name);

	ret = hmc7044_write(dev, HMC7044_REG_SCRATCHPAD, HMC7044_SCRATCH_PATTERN);
	if (ret) {
		LOG_ERR("scratchpad write failed (%d)", ret);
		return ret;
	}

	ret = hmc7044_read(dev, HMC7044_REG_SCRATCHPAD, &val);
	if (ret) {
		LOG_ERR("scratchpad read failed (%d)", ret);
		return ret;
	}

	LOG_INF("scratchpad readback = 0x%02x (wrote 0x%02x)",
		val, HMC7044_SCRATCH_PATTERN);

	if (val != HMC7044_SCRATCH_PATTERN) {
		LOG_ERR("Scratchpad mismatch (got 0x%02x, expected 0x%02x)",
			val, HMC7044_SCRATCH_PATTERN);
		LOG_ERR("If 0x00/0xff: check CS wiring (CLK_CS=0 on SPI1), "
			"SPI ref clock, or 3-/4-wire mode");
		return -ENODEV;
	}

	return 0;
}

static int hmc7044_init(const struct device *dev)
{
	const struct hmc7044_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	ret = hmc7044_probe(dev);
	if (ret) {
		return ret;
	}
	LOG_INF("SUCCESS: HMC7044 scratchpad read/write confirmed");

	LOG_INF("HMC7044 clock setup over %s", config->spi.bus->name);

	ret = hmc7044_setup(dev);
	if (ret) {
		LOG_ERR("hmc7044_setup failed (%d)", ret);
		return ret;
	}
	LOG_INF("SUCCESS: HMC7044 clock tree configured");

	return 0;
}

/* ------------------------------ DT plumbing ------------------------------- */

/*
 * pfd1_limit is in kHz internally, while the devicetree property is in Hz.
 * Absent means the recommended 10 MHz.
 */
#define HMC7044_PFD1_LIMIT(n)                                                              \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, adi_pfd1_maximum_limit_frequency_hz),          \
		    (CLAMP(DT_INST_PROP(n, adi_pfd1_maximum_limit_frequency_hz) / 1000,     \
			   HMC7044_MIN_PFD1, HMC7044_MAX_PFD1)),                            \
		    (HMC7044_RECOMM_PFD1))

#define HMC7044_CHAN_CONFIG(child)                                                         \
	{                                                                                  \
		.num = DT_REG_ADDR(child),                                                 \
		.name = DT_PROP_OR(child, adi_extended_name, NULL),                        \
		.divider = DT_PROP(child, adi_divider),                                    \
		.driver_mode = DT_PROP(child, adi_driver_mode),                            \
		.driver_impedance = DT_PROP(child, adi_driver_impedance_mode),              \
		.coarse_delay = DT_PROP(child, adi_coarse_digital_delay),                   \
		.fine_delay = DT_PROP(child, adi_fine_analog_delay),                        \
		.out_mux_mode = DT_PROP(child, adi_output_mux_mode),                        \
		.high_performance_mode_dis =                                               \
			DT_PROP(child, adi_high_performance_mode_disable),                  \
		.start_up_mode_dynamic_enable =                                            \
			DT_PROP(child, adi_startup_mode_dynamic_enable),                    \
		.dynamic_driver_enable = DT_PROP(child, adi_dynamic_driver_enable),         \
		.force_mute_enable = DT_PROP(child, adi_force_mute_enable),                 \
		.is_sysref = DT_PROP(child, adi_jesd204_sysref_chan),                       \
	},

/*
 * Per-output range checks. These have to be a separate FOREACH pass rather than
 * riding along inside HMC7044_CHAN_CONFIG: that macro expands inside an array
 * initializer, which is an expression context, and _Static_assert is a
 * declaration.
 */
#define HMC7044_CHAN_ASSERTS(child)                                                        \
	BUILD_ASSERT(DT_REG_ADDR(child) < HMC7044_NUM_CLK_OUT,                             \
		     "HMC7044 output number out of range (0-13)");                         \
	BUILD_ASSERT(DT_PROP(child, adi_divider) >= HMC7044_OUT_DIV_MIN &&                 \
		     DT_PROP(child, adi_divider) <= HMC7044_OUT_DIV_MAX,                   \
		     "HMC7044 adi,divider out of range (1-4094)");

/*
 * Two notes on the instance definition below, kept out here because a comment
 * inside the macro body would need a line continuation on every line.
 *
 * SPI_DT_SPEC_INST_GET takes no CS-delay argument: that parameter is deprecated
 * in favour of the spi-cs-setup-delay-ns / spi-cs-hold-delay-ns DT properties.
 *
 * The init level is POST_KERNEL, not PRE_KERNEL_1 as clock providers
 * conventionally use: this one is SPI-attached and Zephyr SPI controllers
 * initialise at POST_KERNEL. Every consumer of these rates must therefore also
 * be POST_KERNEL or later, or it would query an unprogrammed chip. The priority
 * must stay above CONFIG_SPI_INIT_PRIORITY -- see the BUILD_ASSERT that follows
 * the macro.
 */
#define HMC7044_DEFINE(n)                                                                  \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, adi_pll1_clkin_frequencies) == 4,                  \
		     "adi,pll1-clkin-frequencies must have 4 entries");                     \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, adi_gpi_controls) == 4,                            \
		     "adi,gpi-controls must have 4 entries");                               \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, adi_gpo_controls) == 4,                            \
		     "adi,gpo-controls must have 4 entries");                               \
	BUILD_ASSERT(DT_INST_PROP(n, adi_pll1_charge_pump_current_ua) >=                    \
			     HMC7044_CP_CURRENT_MIN &&                                      \
		     DT_INST_PROP(n, adi_pll1_charge_pump_current_ua) <=                    \
			     HMC7044_CP_CURRENT_MAX,                                        \
		     "adi,pll1-charge-pump-current-ua out of range (120-1920)");            \
	BUILD_ASSERT(!DT_INST_PROP(n, adi_clkin1_vco_in_enable),                            \
		     "adi,clkin1-vco-in-enable (external-VCO bypass) is not implemented");   \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(n) > 0,                                 \
		     "an HMC7044 with no enabled output child nodes does nothing");         \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, HMC7044_CHAN_ASSERTS)                          \
                                                                                           \
	static const struct hmc7044_chan_config hmc7044_channels_##n[] = {                  \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, HMC7044_CHAN_CONFIG)                   \
	};                                                                                 \
                                                                                           \
	static const struct hmc7044_config hmc7044_config_##n = {                           \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_WORD_SET(8) | SPI_TRANSFER_MSB |          \
						   SPI_OP_MODE_MASTER),                     \
		.clkin_freq = {                                                            \
			DT_INST_PROP_BY_IDX(n, adi_pll1_clkin_frequencies, 0),              \
			DT_INST_PROP_BY_IDX(n, adi_pll1_clkin_frequencies, 1),              \
			DT_INST_PROP_BY_IDX(n, adi_pll1_clkin_frequencies, 2),              \
			DT_INST_PROP_BY_IDX(n, adi_pll1_clkin_frequencies, 3),              \
		},                                                                         \
		.vcxo_freq = DT_INST_PROP(n, adi_vcxo_frequency),                           \
		.pll2_freq = DT_INST_PROP(n, adi_pll2_output_frequency),                    \
		.pll1_loop_bw = DT_INST_PROP(n, adi_pll1_loop_bandwidth_hz),                \
		.pfd1_limit = HMC7044_PFD1_LIMIT(n),                                       \
		.pll1_cp_current = DT_INST_PROP(n, adi_pll1_charge_pump_current_ua),        \
		.sysref_timer_div = DT_INST_PROP(n, adi_sysref_timer_divider),              \
		.pulse_gen_mode = DT_INST_PROP(n, adi_pulse_generator_mode),                \
		.pll1_ref_prio_ctrl = DT_INST_PROP(n, adi_pll1_ref_prio_ctrl),              \
		.sync_pin_mode = DT_INST_PROP(n, adi_sync_pin_mode),                        \
		.pll1_ref_autorevert_en =                                                  \
			DT_INST_PROP(n, adi_pll1_ref_autorevert_enable),                    \
		.clkin0_rfsync_en = DT_INST_PROP(n, adi_clkin0_rf_sync_enable),             \
		.clkin1_vcoin_en = DT_INST_PROP(n, adi_clkin1_vco_in_enable),               \
		.high_performance_mode_clock_dist_en =                                     \
			DT_INST_PROP(n, adi_high_performance_mode_clock_dist_enable),        \
		/* Inverted sense: the DT property disables the reseeder. */                \
		.rf_reseeder_en = !DT_INST_PROP(n, adi_rf_reseeder_disable),                \
		.in_buf_mode = {                                                           \
			DT_INST_PROP(n, adi_clkin0_buffer_mode),                            \
			DT_INST_PROP(n, adi_clkin1_buffer_mode),                            \
			DT_INST_PROP(n, adi_clkin2_buffer_mode),                            \
			DT_INST_PROP(n, adi_clkin3_buffer_mode),                            \
			DT_INST_PROP(n, adi_oscin_buffer_mode),                             \
		},                                                                         \
		.gpi_ctrl = {                                                              \
			DT_INST_PROP_BY_IDX(n, adi_gpi_controls, 0),                        \
			DT_INST_PROP_BY_IDX(n, adi_gpi_controls, 1),                        \
			DT_INST_PROP_BY_IDX(n, adi_gpi_controls, 2),                        \
			DT_INST_PROP_BY_IDX(n, adi_gpi_controls, 3),                        \
		},                                                                         \
		.gpo_ctrl = {                                                              \
			DT_INST_PROP_BY_IDX(n, adi_gpo_controls, 0),                        \
			DT_INST_PROP_BY_IDX(n, adi_gpo_controls, 1),                        \
			DT_INST_PROP_BY_IDX(n, adi_gpo_controls, 2),                        \
			DT_INST_PROP_BY_IDX(n, adi_gpo_controls, 3),                        \
		},                                                                         \
		.channels = hmc7044_channels_##n,                                          \
		.num_channels = ARRAY_SIZE(hmc7044_channels_##n),                          \
	};                                                                                 \
                                                                                           \
	static struct hmc7044_data hmc7044_data_##n;                                        \
                                                                                           \
	DEVICE_DT_INST_DEFINE(n, hmc7044_init, NULL, &hmc7044_data_##n,                     \
			      &hmc7044_config_##n, POST_KERNEL,                             \
			      CONFIG_CLOCK_CONTROL_HMC7044_INIT_PRIORITY,                   \
			      &hmc7044_api);

BUILD_ASSERT(CONFIG_CLOCK_CONTROL_HMC7044_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "The HMC7044 is SPI-attached, so it must initialise after its SPI controller");

DT_INST_FOREACH_STATUS_OKAY(HMC7044_DEFINE)
