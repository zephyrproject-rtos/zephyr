/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * AD9523(-1) clock generator -- Zephyr clock_control driver.
 *
 * A 14-output, dual-PLL (PLL1 + PLL2/VCO), SPI-programmed clock generator. One
 * device instance per devicetree node; the PLL1/PLL2/VCO/output-divider
 * configuration comes entirely from DT.
 *
 * INIT LEVEL: this is POST_KERNEL, not PRE_KERNEL_1 as clock providers
 * conventionally are, because it is SPI-attached and Zephyr SPI controllers
 * initialise at POST_KERNEL/CONFIG_SPI_INIT_PRIORITY (see the BUILD_ASSERT at the
 * end of the file). Consumers of these rates must be POST_KERNEL or later.
 *
 * The AD9523 has no chip-ID register that returns a fixed constant, so the bus
 * is proved by write-verifying the EEPROM customer-version register (0x006).
 */

#define DT_DRV_COMPAT adi_ad9523_1

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/clock_control/ad9523.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ad9523, LOG_LEVEL_INF);

/*
 * Register addressing. Every register constant carries its transfer length in
 * bits [17:16] (R1B/R2B/R3B) and its 12-bit address in the low bits. The SPI
 * helpers below walk the multi-byte access one byte per frame, decrementing the
 * address, MSB first.
 */
#define AD9523_READ    (1U << 15)
#define AD9523_WRITE   (0U << 15)
#define AD9523_CNT(x)  (((x) - 1U) << 13)
#define AD9523_ADDR(x) ((x) & 0xFFFU)

#define AD9523_R1B           (1U << 16)
#define AD9523_R2B           (2U << 16)
#define AD9523_R3B           (3U << 16)
#define AD9523_TRANSF_LEN(x) ((x) >> 16)

/* -------------------- AD9523 registers / bitfields ---------------- */

#define AD9523_SERIAL_PORT_CONFIG (AD9523_R1B | 0x0)
#define AD9523_VERSION_REGISTER   (AD9523_R1B | 0x2)
#define AD9523_PART_REGISTER      (AD9523_R1B | 0x3)
#define AD9523_READBACK_CTRL      (AD9523_R1B | 0x4)

#define AD9523_EEPROM_CUSTOMER_VERSION_ID (AD9523_R2B | 0x6)

#define AD9523_PLL1_REF_A_DIVIDER        (AD9523_R2B | 0x11)
#define AD9523_PLL1_REF_B_DIVIDER        (AD9523_R2B | 0x13)
#define AD9523_PLL1_REF_TEST_DIVIDER     (AD9523_R1B | 0x14)
#define AD9523_PLL1_FEEDBACK_DIVIDER     (AD9523_R2B | 0x17)
#define AD9523_PLL1_CHARGE_PUMP_CTRL     (AD9523_R2B | 0x19)
#define AD9523_PLL1_INPUT_RECEIVERS_CTRL (AD9523_R1B | 0x1A)
#define AD9523_PLL1_REF_CTRL             (AD9523_R1B | 0x1B)
#define AD9523_PLL1_MISC_CTRL            (AD9523_R1B | 0x1C)
#define AD9523_PLL1_LOOP_FILTER_CTRL     (AD9523_R1B | 0x1D)

#define AD9523_PLL2_CHARGE_PUMP         (AD9523_R1B | 0xF0)
#define AD9523_PLL2_FEEDBACK_DIVIDER_AB (AD9523_R1B | 0xF1)
#define AD9523_PLL2_CTRL                (AD9523_R1B | 0xF2)
#define AD9523_PLL2_VCO_CTRL            (AD9523_R1B | 0xF3)
#define AD9523_PLL2_VCO_DIVIDER         (AD9523_R1B | 0xF4)
#define AD9523_PLL2_LOOP_FILTER_CTRL    (AD9523_R2B | 0xF6)
#define AD9523_PLL2_R2_DIVIDER          (AD9523_R1B | 0xF7)

#define AD9523_CHANNEL_CLOCK_DIST(ch) (AD9523_R3B | (0x192 + 3 * (ch)))

#define AD9523_PLL1_OUTPUT_CTRL         (AD9523_R1B | 0x1BA)
#define AD9523_PLL1_OUTPUT_CHANNEL_CTRL (AD9523_R1B | 0x1BB)

#define AD9523_READBACK_0 (AD9523_R1B | 0x22C)
#define AD9523_READBACK_1 (AD9523_R1B | 0x22D)

#define AD9523_STATUS_SIGNALS  (AD9523_R3B | 0x232)
#define AD9523_POWER_DOWN_CTRL (AD9523_R1B | 0x233)
#define AD9523_IO_UPDATE       (AD9523_R1B | 0x234)

/* AD9523_SERIAL_PORT_CONFIG */
#define AD9523_SER_CONF_SDO_ACTIVE ((1U << 7) | (1U << 0))
#define AD9523_SER_CONF_SOFT_RESET ((1U << 5) | (1U << 2))

/* AD9523_READBACK_CTRL */
#define AD9523_READBACK_CTRL_READ_BUFFERED (1U << 0)

/* AD9523_PLL1_CHARGE_PUMP_CTRL */
#define AD9523_PLL1_CHARGE_PUMP_CURRENT_nA(x) (((x) / 500) & 0x7F)
#define AD9523_PLL1_CHARGE_PUMP_TRISTATE      (1U << 7)
#define AD9523_PLL1_CHARGE_PUMP_MODE_NORMAL   (3U << 8)
#define AD9523_PLL1_BACKLASH_PW_MIN           (0U << 10)

/* AD9523_PLL1_INPUT_RECEIVERS_CTRL */
#define AD9523_PLL1_REFB_DIFF_RCV_EN       (1U << 6)
#define AD9523_PLL1_REFA_DIFF_RCV_EN       (1U << 5)
#define AD9523_PLL1_REFB_RCV_EN            (1U << 4)
#define AD9523_PLL1_REFA_RCV_EN            (1U << 3)
#define AD9523_PLL1_REFA_REFB_PWR_CTRL_EN  (1U << 2)
#define AD9523_PLL1_OSC_IN_CMOS_NEG_INP_EN (1U << 1)
#define AD9523_PLL1_OSC_IN_DIFF_EN         (1U << 0)

/* AD9523_PLL1_REF_CTRL */
#define AD9523_PLL1_BYPASS_FEEDBACK_DIV_EN (1U << 6)
#define AD9523_PLL1_ZERO_DELAY_MODE_INT    (1U << 5)
#define AD9523_PLL1_OSC_IN_PLL_FEEDBACK_EN (1U << 4)
#define AD9523_PLL1_ZD_IN_CMOS_NEG_INP_EN  (1U << 3)
#define AD9523_PLL1_ZD_IN_DIFF_EN          (1U << 2)
#define AD9523_PLL1_REFB_CMOS_NEG_INP_EN   (1U << 1)
#define AD9523_PLL1_REFA_CMOS_NEG_INP_EN   (1U << 0)

/* AD9523_PLL1_MISC_CTRL */
#define AD9523_PLL1_REFB_INDEP_DIV_CTRL_EN (1U << 7)
#define AD9523_PLL1_REF_MODE(x)            ((x) << 2)

/* AD9523_PLL1_LOOP_FILTER_CTRL */
#define AD9523_PLL1_LOOP_FILTER_RZERO(x) ((x) & 0xF)

/* AD9523_PLL2_CHARGE_PUMP */
#define AD9523_PLL2_CHARGE_PUMP_CURRENT_nA(x) ((x) / 3500)

/* AD9523_PLL2_FEEDBACK_DIVIDER_AB */
#define AD9523_PLL2_FB_NDIV_A_CNT(x) (((x) & 0x3) << 6)
#define AD9523_PLL2_FB_NDIV_B_CNT(x) (((x) & 0x3F) << 0)
#define AD9523_PLL2_FB_NDIV(a, b)    (4 * (b) + (a))

/* AD9523_PLL2_CTRL */
#define AD9523_PLL2_CHARGE_PUMP_MODE_NORMAL (3U << 0)
#define AD9523_PLL2_BACKLASH_CTRL_EN        (1U << 4)
#define AD9523_PLL2_FREQ_DOUBLER_EN         (1U << 5)

/* AD9523_PLL2_VCO_CTRL */
#define AD9523_PLL2_VCO_CALIBRATE (1U << 1)

/* AD9523_PLL2_VCO_DIVIDER */
#define AD9523_PLL2_VCO_DIV_M1(x)          ((((x) - 3) & 0x3) << 0)
#define AD9523_PLL2_VCO_DIV_M2(x)          ((((x) - 3) & 0x3) << 4)
#define AD9523_PLL2_VCO_DIV_M1_PWR_DOWN_EN (1U << 2)
#define AD9523_PLL2_VCO_DIV_M2_PWR_DOWN_EN (1U << 6)

/* AD9523_PLL2_LOOP_FILTER_CTRL */
#define AD9523_PLL2_LOOP_FILTER_CPOLE1(x)       (((x) & 0x7) << 0)
#define AD9523_PLL2_LOOP_FILTER_RZERO(x)        (((x) & 0x7) << 3)
#define AD9523_PLL2_LOOP_FILTER_RPOLE2(x)       (((x) & 0x7) << 6)
#define AD9523_PLL2_LOOP_FILTER_RZERO_BYPASS_EN (1U << 8)

/* AD9523_PLL2_R2_DIVIDER */
#define AD9523_PLL2_R2_DIVIDER_VAL(x) (((x) & 0x1F) << 0)

/* AD9523_CHANNEL_CLOCK_DIST */
#define AD9523_CLK_DIST_DIV_PHASE(x)      (((x) & 0x3F) << 18)
#define AD9523_CLK_DIST_DIV(x)            ((((x) - 1) & 0x3FF) << 8)
#define AD9523_CLK_DIST_INV_DIV_OUTPUT_EN (1U << 7)
#define AD9523_CLK_DIST_IGNORE_SYNC_EN    (1U << 6)
#define AD9523_CLK_DIST_PWR_DOWN_EN       (1U << 5)
#define AD9523_CLK_DIST_LOW_PWR_MODE_EN   (1U << 4)
#define AD9523_CLK_DIST_DRIVER_MODE(x)    (((x) & 0xF) << 0)

/* AD9523_PLL1_OUTPUT_CTRL: CH4..CH6 select VCO2 (M2) rather than VCO1 (M1). */
#define AD9523_PLL1_OUTP_CTRL_VCO_DIV_SEL_CH4_M2 (1U << 5)

/* AD9523_PLL1_OUTPUT_CHANNEL_CTRL */
#define AD9523_PLL1_OUTP_CH_CTRL_VCO_DIV_SEL_CH7_M2 (1U << 4)
#define AD9523_PLL1_OUTP_CH_CTRL_VCXO_SRC_SEL_CH0   (1U << 0)

/* AD9523_READBACK_0 */
#define AD9523_READBACK_0_STAT_PLL2_REF_CLK (1U << 7)
#define AD9523_READBACK_0_STAT_PLL2_FB_CLK  (1U << 6)
#define AD9523_READBACK_0_STAT_VCXO         (1U << 5)
#define AD9523_READBACK_0_STAT_REF_TEST     (1U << 4)
#define AD9523_READBACK_0_STAT_REFB         (1U << 3)
#define AD9523_READBACK_0_STAT_REFA         (1U << 2)
#define AD9523_READBACK_0_STAT_PLL2_LD      (1U << 1)
#define AD9523_READBACK_0_STAT_PLL1_LD      (1U << 0)

/* AD9523_READBACK_1 */
#define AD9523_READBACK_1_VCO_CALIB_IN_PROGRESS (1U << 0)

/* AD9523_STATUS_SIGNALS */
#define AD9523_STATUS_SIGNALS_SYNC_MAN_CTRL   (1U << 16)
#define AD9523_STATUS_MONITOR_01_PLL12_LOCKED (0x302)

/* AD9523_IO_UPDATE */
#define AD9523_IO_UPDATE_EN (1U << 0)

/* Output driver mode value that parks a lane (tristate). */
#define AD9523_DRIVER_MODE_TRISTATE 0

/*
 * Arbitrary token written to the EEPROM customer-version register to verify the
 * bus; the original contents are restored afterwards.
 */
#define AD9523_SPI_CHECK_PATTERN 0xAD95

/* Clock-source indices into ad9523_data.vco_out_freq[]. */
#define AD9523_VCO1        0
#define AD9523_VCO2        1
#define AD9523_VCXO        2
#define AD9523_NUM_CLK_SRC 3

/*
 * PLL2 VCO band for the AD9523-1 variant, 2.94-3.1 GHz per the datasheet. Used
 * only by the derive path.
 */
#define AD9523_1_VCO_MIN 2940000000U
#define AD9523_1_VCO_MAX 3100000000U

/* PLL2 feedback N-divider and R2 limits (a in 0..3, b in 0..63, r2 in 1..31). */
#define AD9523_PLL2_NDIV_MAX 255
#define AD9523_PLL2_R2_MAX   31

/* --------------------------- config / data -------------------------------- */

/* One output, entirely from devicetree -- no field is written at runtime. */
struct ad9523_chan_config {
	uint8_t num;
	const char *name;
	uint8_t driver_mode;
	uint8_t divider_phase;
	uint16_t channel_divider;
	bool use_alt_clock_src;
	bool output_dis;
	bool sync_ignore_en;
	bool low_power_mode_en;
	bool divider_output_invert_en;
};

struct ad9523_config {
	struct spi_dt_spec spi;

	/** SYNC pin, optional; parked high at init to release the output dividers. */
	struct gpio_dt_spec sync;

	uint32_t vcxo_freq;

	/*
	 * 3-wire SPI (SDIO bidirectional, no separate SDO). When set, the driver
	 * does NOT assert SDO_ACTIVE in SERIAL_PORT_CONFIG, so the chip returns
	 * read data on SDIO. Must match the board wiring: a 4-wire config on a
	 * 3-wire board makes the chip drive an unrouted SDO pin and every readback
	 * returns zero.
	 */
	bool spi3wire;

	/* PLL1 */
	uint16_t refa_r_div;
	uint16_t refb_r_div;
	uint16_t pll1_feedback_div;
	uint16_t pll1_charge_pump_current_nA;
	uint8_t pll1_loop_filter_rzero;
	uint8_t ref_mode;
	bool pll1_bypass_en;
	bool zero_delay_mode_internal_en;
	bool osc_in_feedback_en;
	bool refa_diff_rcv_en;
	bool refb_diff_rcv_en;
	bool zd_in_diff_en;
	bool osc_in_diff_en;
	bool refa_cmos_neg_inp_en;
	bool refb_cmos_neg_inp_en;
	bool zd_in_cmos_neg_inp_en;
	bool osc_in_cmos_neg_inp_en;

	/* PLL2 -- explicit divider path (used when pll2_m1_freq == 0). */
	uint32_t pll2_charge_pump_current_nA;
	uint8_t pll2_ndiv_a_cnt;
	uint8_t pll2_ndiv_b_cnt;
	uint8_t pll2_r2_div;
	uint8_t pll2_vco_diff_m1;
	uint8_t pll2_vco_diff_m2;
	bool pll2_freq_doubler_en;

	/* PLL2 loop filter */
	uint8_t rpole2;
	uint8_t rzero;
	uint8_t cpole1;
	bool rzero_bypass_en;

	/*
	 * Derive input. Non-zero selects the derive path: the driver solves
	 * r2/ndiv(a,b)/m1/doubler at init to hit this VCO1 output frequency (Hz)
	 * and the explicit pll2_* fields above are ignored. A BUILD_ASSERT
	 * forbids setting both on one node.
	 */
	uint32_t pll2_m1_freq;

	const struct ad9523_chan_config *channels;
	uint8_t num_channels;
};

struct ad9523_data {
	/* PLL2 VCO frequency, solved at init. */
	uint32_t vco_freq;
	/* Frequencies of the three clock sources, indexed by AD9523_VCO1 etc. */
	uint32_t vco_out_freq[AD9523_NUM_CLK_SRC];
	/* Per-output source selection (AD9523_VCO1/VCO2/VCXO), from setup. */
	uint8_t vco_out_map[AD9523_NUM_CLK_OUT];
	/* Bit N set once output N has been programmed and enabled. */
	uint16_t enabled;
	/* Set by the setup write-verify. Gates status reads on this chip. */
	bool spi_ok;
};

/* ------------------------------ divider math ------------------------------ */

static uint32_t ad9523_gcd(uint32_t a, uint32_t b)
{
	uint32_t rem;

	if ((a == 0) || (b == 0)) {
		return MAX(a, b);
	}

	while (b != 0) {
		rem = a % b;
		a = b;
		b = rem;
	}

	return a;
}

static void ad9523_rational_best_approximation(uint32_t num, uint32_t den, uint32_t max_num,
					       uint32_t max_den, uint32_t *best_num,
					       uint32_t *best_den)
{
	uint32_t gcd = ad9523_gcd(num, den);

	*best_num = num / gcd;
	*best_den = den / gcd;

	if ((*best_num > max_num) || (*best_den > max_den)) {
		*best_num = 0;
		*best_den = 0;
	}
}

/* ------------------------------ SPI helpers ------------------------------- */

/*
 * Read a register. The multi-byte access is one 3-byte frame per data byte,
 * MSB first, decrementing the address each byte. reg_addr carries the transfer
 * length in bits [17:16].
 */
static int ad9523_spi_read(const struct device *dev, uint32_t reg_addr, uint32_t *reg_data)
{
	const struct ad9523_config *config = dev->config;
	uint32_t len = AD9523_TRANSF_LEN(reg_addr);

	if (reg_data == NULL) {
		return -EINVAL;
	}

	*reg_data = 0;
	for (uint32_t i = 0; i < len; i++) {
		uint8_t tx[3] = {
			0x80 | (reg_addr >> 8),
			reg_addr & 0xFF,
			0x00,
		};
		uint8_t rx[3] = {0};
		const struct spi_buf txb = {.buf = tx, .len = sizeof(tx)};
		const struct spi_buf rxb = {.buf = rx, .len = sizeof(rx)};
		const struct spi_buf_set txs = {.buffers = &txb, .count = 1};
		const struct spi_buf_set rxs = {.buffers = &rxb, .count = 1};
		int ret = spi_transceive_dt(&config->spi, &txs, &rxs);

		if (ret < 0) {
			return ret;
		}

		reg_addr--;
		*reg_data = (*reg_data << 8) | rx[2];
	}

	return 0;
}

/*
 * Write a register. One 3-byte frame per data byte, MSB first, decrementing the
 * address each byte.
 */
static int ad9523_spi_write(const struct device *dev, uint32_t reg_addr, uint32_t reg_data)
{
	const struct ad9523_config *config = dev->config;
	uint32_t len = AD9523_TRANSF_LEN(reg_addr);

	for (uint32_t i = 0; i < len; i++) {
		uint8_t tx[3] = {
			reg_addr >> 8,
			reg_addr & 0xFF,
			(reg_data >> ((len - i - 1) * 8)) & 0xFF,
		};
		const struct spi_buf txb = {.buf = tx, .len = sizeof(tx)};
		const struct spi_buf_set txs = {.buffers = &txb, .count = 1};
		int ret = spi_write_dt(&config->spi, &txs);

		if (ret < 0) {
			return ret;
		}

		reg_addr--;
	}

	return 0;
}

/* Latch the buffered/hold registers into effect. */
static int ad9523_io_update(const struct device *dev)
{
	return ad9523_spi_write(dev, AD9523_IO_UPDATE, AD9523_IO_UPDATE_EN);
}

/* --------------------------- clock source map ----------------------------- */

/*
 * Route an output to its clock source. CH0..CH3 can draw from the VCXO,
 * CH4..CH9 from VCO2 (M2); everything else stays on VCO1 (M1). The result is
 * recorded in vco_out_map[] so get_rate() knows which source frequency to
 * divide.
 */
static int ad9523_vco_out_map(const struct device *dev, uint32_t ch, uint32_t out)
{
	struct ad9523_data *data = dev->data;
	uint32_t mask;
	uint32_t reg_data;
	int ret;

	switch (ch) {
	case 0 ... 3:
		ret = ad9523_spi_read(dev, AD9523_PLL1_OUTPUT_CHANNEL_CTRL, &reg_data);
		if (ret < 0) {
			return ret;
		}
		mask = AD9523_PLL1_OUTP_CH_CTRL_VCXO_SRC_SEL_CH0 << ch;
		if (out) {
			reg_data |= mask;
			out = AD9523_VCXO;
		} else {
			reg_data &= ~mask;
		}
		ret = ad9523_spi_write(dev, AD9523_PLL1_OUTPUT_CHANNEL_CTRL, reg_data);
		break;
	case 4 ... 6:
		ret = ad9523_spi_read(dev, AD9523_PLL1_OUTPUT_CTRL, &reg_data);
		if (ret < 0) {
			return ret;
		}
		mask = AD9523_PLL1_OUTP_CTRL_VCO_DIV_SEL_CH4_M2 << (ch - 4);
		if (out) {
			reg_data |= mask;
		} else {
			reg_data &= ~mask;
		}
		ret = ad9523_spi_write(dev, AD9523_PLL1_OUTPUT_CTRL, reg_data);
		break;
	case 7 ... 9:
		ret = ad9523_spi_read(dev, AD9523_PLL1_OUTPUT_CHANNEL_CTRL, &reg_data);
		if (ret < 0) {
			return ret;
		}
		mask = AD9523_PLL1_OUTP_CH_CTRL_VCO_DIV_SEL_CH7_M2 << (ch - 7);
		if (out) {
			reg_data |= mask;
		} else {
			reg_data &= ~mask;
		}
		ret = ad9523_spi_write(dev, AD9523_PLL1_OUTPUT_CHANNEL_CTRL, reg_data);
		break;
	default:
		/*
		 * CH10..CH13 have no source-select register; they run off VCO1.
		 * Leave vco_out_map[ch] at its zero-init (AD9523_VCO1) by
		 * returning before the store below.
		 */
		return 0;
	}

	if (ret < 0) {
		return ret;
	}

	data->vco_out_map[ch] = out;

	return 0;
}

/* ------------------------------ channels ---------------------------------- */

static const struct ad9523_chan_config *ad9523_chan_find(const struct device *dev, uint8_t num)
{
	const struct ad9523_config *config = dev->config;

	for (uint8_t i = 0; i < config->num_channels; i++) {
		if (config->channels[i].num == num) {
			return &config->channels[i];
		}
	}

	return NULL;
}

/*
 * Build a channel's CHANNEL_CLOCK_DIST word from devicetree. This carries the
 * divider, phase, driver mode and the power-down bit, so enabling or disabling
 * an output means rewriting the whole word rather than a read-modify-write --
 * which keeps clock_control_on()/off() idempotent. A disabled output is parked
 * tristate and powered down.
 */
static int ad9523_chan_set_enable(const struct device *dev, const struct ad9523_chan_config *chan,
				  bool on)
{
	struct ad9523_data *data = dev->data;
	uint32_t word;
	int ret;

	if (on) {
		word = AD9523_CLK_DIST_DRIVER_MODE(chan->driver_mode) |
		       AD9523_CLK_DIST_DIV(chan->channel_divider) |
		       AD9523_CLK_DIST_DIV_PHASE(chan->divider_phase) |
		       (chan->sync_ignore_en ? AD9523_CLK_DIST_IGNORE_SYNC_EN : 0) |
		       (chan->divider_output_invert_en ? AD9523_CLK_DIST_INV_DIV_OUTPUT_EN : 0) |
		       (chan->low_power_mode_en ? AD9523_CLK_DIST_LOW_PWR_MODE_EN : 0) |
		       (chan->output_dis ? AD9523_CLK_DIST_PWR_DOWN_EN : 0);
	} else {
		word = AD9523_CLK_DIST_DRIVER_MODE(AD9523_DRIVER_MODE_TRISTATE) |
		       AD9523_CLK_DIST_PWR_DOWN_EN;
	}

	ret = ad9523_spi_write(dev, AD9523_CHANNEL_CLOCK_DIST(chan->num), word);
	if (ret < 0) {
		return ret;
	}

	/* The distribution registers are buffered; latch the change. */
	ret = ad9523_io_update(dev);
	if (ret < 0) {
		return ret;
	}

	WRITE_BIT(data->enabled, chan->num, on && !chan->output_dis);

	return 0;
}

/* --------------------------- clock_control ops ---------------------------- */

/*
 * Translate a subsystem handle to an output config. AD9523_CLK_OUT() biases the
 * output number by one so that output 0 is distinguishable from
 * CLOCK_CONTROL_SUBSYS_ALL, which is NULL.
 */
static int ad9523_subsys_to_chan(const struct device *dev, clock_control_subsys_t sys,
				 const struct ad9523_chan_config **chan)
{
	uintptr_t handle = (uintptr_t)sys;

	if (handle == 0 || handle > AD9523_NUM_CLK_OUT) {
		return -EINVAL;
	}

	*chan = ad9523_chan_find(dev, (uint8_t)(handle - 1));
	if (*chan == NULL) {
		/* A real output number, but it has no child node in DT. */
		return -ENODEV;
	}

	return 0;
}

static int ad9523_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct ad9523_chan_config *chan;
	int ret;

	ret = ad9523_subsys_to_chan(dev, sys, &chan);
	if (ret) {
		return ret;
	}

	return ad9523_chan_set_enable(dev, chan, true);
}

static int ad9523_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct ad9523_chan_config *chan;
	int ret;

	ret = ad9523_subsys_to_chan(dev, sys, &chan);
	if (ret) {
		return ret;
	}

	return ad9523_chan_set_enable(dev, chan, false);
}

static int ad9523_clk_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	struct ad9523_data *data = dev->data;
	const struct ad9523_chan_config *chan;
	int ret;

	if (rate == NULL) {
		return -EINVAL;
	}

	/*
	 * There is no single rate for the whole chip -- 14 outputs each run at
	 * their source frequency / divider. Report the PLL2 VCO frequency for
	 * SUBSYS_ALL, which is the rate the VCO1/VCO2 outputs derive from.
	 */
	if (sys == CLOCK_CONTROL_SUBSYS_ALL) {
		*rate = data->vco_freq;
		return 0;
	}

	ret = ad9523_subsys_to_chan(dev, sys, &chan);
	if (ret) {
		return ret;
	}

	*rate = data->vco_out_freq[data->vco_out_map[chan->num]] / chan->channel_divider;

	return 0;
}

/*
 * Status semantics: an output is ON when it has been programmed and enabled AND
 * the PLLs are locked. A divider running off an unlocked PLL2 is producing
 * something, but not the rate get_rate() reports, so reporting it as ON would be
 * a lie in exactly the case a caller is checking for.
 */
static enum clock_control_status ad9523_clk_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	struct ad9523_data *data = dev->data;
	const struct ad9523_chan_config *chan;
	struct ad9523_status status;

	if (sys != CLOCK_CONTROL_SUBSYS_ALL) {
		int ret = ad9523_subsys_to_chan(dev, sys, &chan);

		if (ret) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}

		if ((data->enabled & BIT(chan->num)) == 0) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	}

	if (!data->spi_ok) {
		/* Bus never verified: the chip is programmed but unreadable. */
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (ad9523_get_status(dev, &status) != 0) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (status.vcxo_locked && status.pll2_locked &&
	    (status.pll1_bypassed || status.pll1_locked)) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	/*
	 * VCXO up but a PLL not yet locked is a transient calibration state that
	 * resolves without further programming -- STATUS_STARTING. No VCXO at
	 * all is not going to resolve on its own.
	 */
	if (status.vcxo_locked) {
		return CLOCK_CONTROL_STATUS_STARTING;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, ad9523_api) = {
	.on = ad9523_clk_on,
	.off = ad9523_clk_off,
	.get_rate = ad9523_clk_get_rate,
	.get_status = ad9523_clk_get_status,
};

/* ------------------------------ extension API ----------------------------- */

int ad9523_get_status(const struct device *dev, struct ad9523_status *status)
{
	const struct ad9523_config *config;
	struct ad9523_data *data;
	uint32_t reg_data = 0;
	int ret;

	if (dev == NULL || status == NULL) {
		return -EINVAL;
	}
	if (!DEVICE_API_IS(clock_control, dev) || dev->api != &ad9523_api) {
		return -EINVAL;
	}

	config = dev->config;
	data = dev->data;

	ret = ad9523_spi_read(dev, AD9523_READBACK_0, &reg_data);
	if (ret < 0) {
		return ret;
	}

	status->vcxo_locked = (reg_data & AD9523_READBACK_0_STAT_VCXO) != 0;
	status->pll2_locked = (reg_data & AD9523_READBACK_0_STAT_PLL2_LD) != 0;
	status->pll1_locked = (reg_data & AD9523_READBACK_0_STAT_PLL1_LD) != 0;
	status->pll1_bypassed = config->pll1_bypass_en;
	status->vco_freq = data->vco_freq;

	return 0;
}

/* Toggle SYNC_MAN_CTRL, io_update on each edge. */
static int ad9523_do_sync(const struct device *dev)
{
	uint32_t reg_data;
	int ret;

	ret = ad9523_spi_read(dev, AD9523_STATUS_SIGNALS, &reg_data);
	if (ret < 0) {
		return ret;
	}

	reg_data |= AD9523_STATUS_SIGNALS_SYNC_MAN_CTRL;
	ret = ad9523_spi_write(dev, AD9523_STATUS_SIGNALS, reg_data);
	if (ret < 0) {
		return ret;
	}

	ret = ad9523_io_update(dev);
	if (ret < 0) {
		return ret;
	}

	reg_data &= ~AD9523_STATUS_SIGNALS_SYNC_MAN_CTRL;
	ret = ad9523_spi_write(dev, AD9523_STATUS_SIGNALS, reg_data);
	if (ret < 0) {
		return ret;
	}

	return ad9523_io_update(dev);
}

int ad9523_sync(const struct device *dev)
{
	if (dev == NULL || !DEVICE_API_IS(clock_control, dev) || dev->api != &ad9523_api) {
		return -EINVAL;
	}

	return ad9523_do_sync(dev);
}

/* -------------------------------- setup ----------------------------------- */

/*
 * Derive PLL2 dividers to hit config->pll2_m1_freq (a VCO1 output frequency).
 *
 * UNVERIFIED: opt-in via adi,pll2-m1-freq, unused when a node ships explicit
 * dividers instead.
 *
 * Method: for each supported M1 in {3,4,5}, the target VCO is m1_freq * M1; the
 * first that lands in the AD9523-1 VCO band (2.94-3.1 GHz) and factors exactly
 * as vcxo * NDIV / R2 (doubler off) within the divider limits is taken. VCO2
 * (M2) is left off -- the derive path serves single-VCO1 configurations.
 */
static int ad9523_derive_pll2(const struct ad9523_config *config, uint32_t *r2, uint32_t *ndiv_a,
			      uint32_t *ndiv_b, uint32_t *m1, uint32_t *m2, bool *doubler)
{
	static const uint32_t m1_candidates[] = {3, 4, 5};

	for (uint32_t i = 0; i < ARRAY_SIZE(m1_candidates); i++) {
		uint32_t mm = m1_candidates[i];
		uint64_t vco = (uint64_t)config->pll2_m1_freq * mm;
		uint32_t ndiv, rr, a, b;

		if (vco < AD9523_1_VCO_MIN || vco > AD9523_1_VCO_MAX) {
			continue;
		}

		/* vco fits uint32 once inside the band. NDIV/R2 = vco/vcxo. */
		ad9523_rational_best_approximation((uint32_t)vco, config->vcxo_freq,
						   AD9523_PLL2_NDIV_MAX, AD9523_PLL2_R2_MAX, &ndiv,
						   &rr);
		if (ndiv == 0 || rr == 0) {
			continue;
		}

		/* Require an exact integer relationship -- no fractional VCO. */
		if ((uint64_t)config->vcxo_freq * ndiv != vco * rr) {
			continue;
		}

		a = ndiv % 4;
		b = ndiv / 4;
		if (b > 63) {
			continue;
		}

		*r2 = rr;
		*ndiv_a = a;
		*ndiv_b = b;
		*m1 = mm;
		*m2 = 0;
		*doubler = false;

		return 0;
	}

	LOG_ERR("adi,pll2-m1-freq %u Hz not reachable from a %u Hz VCXO", config->pll2_m1_freq,
		config->vcxo_freq);
	return -EINVAL;
}

/* Kick a PLL2 VCO calibration and poll for done. */
static int ad9523_calibrate(const struct device *dev)
{
	uint32_t reg_data;
	int ret;

	ret = ad9523_spi_write(dev, AD9523_PLL2_VCO_CTRL, AD9523_PLL2_VCO_CALIBRATE);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_io_update(dev);
	if (ret < 0) {
		return ret;
	}

	for (uint32_t timeout = 0; timeout < 100; timeout++) {
		k_msleep(1);
		ret = ad9523_spi_read(dev, AD9523_READBACK_1, &reg_data);
		if (ret < 0) {
			return ret;
		}
		if ((reg_data & AD9523_READBACK_1_VCO_CALIB_IN_PROGRESS) == 0) {
			break;
		}
	}

	ret = ad9523_spi_read(dev, AD9523_READBACK_1, &reg_data);
	if (ret < 0) {
		return ret;
	}
	if ((reg_data & AD9523_READBACK_1_VCO_CALIB_IN_PROGRESS) != 0) {
		LOG_ERR("VCO calibration failed (0x%06x)", reg_data);
		return -EIO;
	}

	return 0;
}

/* Poll VCXO + PLL lock, warn on failure. */
static int ad9523_status_wait(const struct device *dev)
{
	const struct ad9523_config *config = dev->config;
	uint32_t reg_data = 0;
	uint32_t want;
	int ret = 0;

	/* VCXO and PLL2 must always be present. */
	want = AD9523_READBACK_0_STAT_VCXO | AD9523_READBACK_0_STAT_PLL2_LD;

	if (!config->pll1_bypass_en) {
		want |= AD9523_READBACK_0_STAT_PLL2_REF_CLK | AD9523_READBACK_0_STAT_PLL2_FB_CLK |
			AD9523_READBACK_0_STAT_REF_TEST | AD9523_READBACK_0_STAT_REFB |
			AD9523_READBACK_0_STAT_REFA | AD9523_READBACK_0_STAT_PLL1_LD;
	}

	for (uint32_t timeout = 0; timeout < 100; timeout++) {
		k_msleep(1);
		ret = ad9523_spi_read(dev, AD9523_READBACK_0, &reg_data);
		if (ret < 0) {
			return ret;
		}
		if ((reg_data & want) == want) {
			break;
		}
	}

	ret = 0;
	if ((reg_data & AD9523_READBACK_0_STAT_VCXO) == 0) {
		LOG_ERR("VCXO status error (0x%06x)", reg_data);
		ret = -EIO;
	}
	if ((reg_data & AD9523_READBACK_0_STAT_PLL2_LD) == 0) {
		LOG_ERR("PLL2 not locked (0x%06x)", reg_data);
		ret = -EIO;
	}

	return ret;
}

/* Program the full clock tree: reset, bus check, PLL1, PLL2, outputs, sync. */
static int ad9523_setup(const struct device *dev)
{
	const struct ad9523_config *config = dev->config;
	struct ad9523_data *data = dev->data;
	const struct ad9523_chan_config *chan;
	uint32_t active_mask = 0;
	uint32_t reg_data, version_id;
	uint32_t r2, ndiv_a, ndiv_b, m1, m2;
	bool doubler;
	int ret;

	/* Choose the PLL2 divider set: derived or explicit. */
	if (config->pll2_m1_freq) {
		ret = ad9523_derive_pll2(config, &r2, &ndiv_a, &ndiv_b, &m1, &m2, &doubler);
		if (ret) {
			return ret;
		}
	} else {
		r2 = config->pll2_r2_div;
		ndiv_a = config->pll2_ndiv_a_cnt;
		ndiv_b = config->pll2_ndiv_b_cnt;
		m1 = config->pll2_vco_diff_m1;
		m2 = config->pll2_vco_diff_m2;
		doubler = config->pll2_freq_doubler_en;
	}

	/* Soft reset. */
	ret = ad9523_spi_write(dev, AD9523_SERIAL_PORT_CONFIG,
			       AD9523_SER_CONF_SOFT_RESET |
				       (config->spi3wire ? 0 : AD9523_SER_CONF_SDO_ACTIVE));
	if (ret < 0) {
		return ret;
	}
	k_msleep(1);

	/* Buffered readback, so a read returns the programmed (not live) value. */
	ret = ad9523_spi_write(dev, AD9523_READBACK_CTRL, AD9523_READBACK_CTRL_READ_BUFFERED);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_io_update(dev);
	if (ret < 0) {
		return ret;
	}

	/* Prove the bus: save, write a token, read it back, restore. */
	ret = ad9523_spi_read(dev, AD9523_EEPROM_CUSTOMER_VERSION_ID, &version_id);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(dev, AD9523_EEPROM_CUSTOMER_VERSION_ID, AD9523_SPI_CHECK_PATTERN);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_read(dev, AD9523_EEPROM_CUSTOMER_VERSION_ID, &reg_data);
	if (ret < 0) {
		return ret;
	}
	if (reg_data != AD9523_SPI_CHECK_PATTERN) {
		LOG_ERR("SPI write-verify failed (0x%06x)", reg_data);
		LOG_ERR("check CS wiring, SPI ref clock, or 3-/4-wire mode");
		return -ENODEV;
	}
	data->spi_ok = true;
	ret = ad9523_spi_write(dev, AD9523_EEPROM_CUSTOMER_VERSION_ID, version_id);
	if (ret < 0) {
		return ret;
	}
	LOG_INF("SUCCESS: AD9523 SPI write-verify confirmed");

	/*
	 * PLL1 setup.
	 */
	ret = ad9523_spi_write(dev, AD9523_PLL1_REF_A_DIVIDER, config->refa_r_div);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(dev, AD9523_PLL1_REF_B_DIVIDER, config->refb_r_div);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(dev, AD9523_PLL1_FEEDBACK_DIVIDER, config->pll1_feedback_div);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(
		dev, AD9523_PLL1_CHARGE_PUMP_CTRL,
		config->pll1_bypass_en
			? AD9523_PLL1_CHARGE_PUMP_TRISTATE
			: (AD9523_PLL1_CHARGE_PUMP_CURRENT_nA(config->pll1_charge_pump_current_nA) |
			   AD9523_PLL1_CHARGE_PUMP_MODE_NORMAL | AD9523_PLL1_BACKLASH_PW_MIN));
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(
		dev, AD9523_PLL1_INPUT_RECEIVERS_CTRL,
		config->pll1_bypass_en
			? (AD9523_PLL1_REFA_REFB_PWR_CTRL_EN |
			   (config->osc_in_diff_en ? AD9523_PLL1_OSC_IN_DIFF_EN : 0) |
			   (config->osc_in_cmos_neg_inp_en ? AD9523_PLL1_OSC_IN_CMOS_NEG_INP_EN
							   : 0))
			: ((config->refa_diff_rcv_en ? AD9523_PLL1_REFA_RCV_EN : 0) |
			   (config->refb_diff_rcv_en ? AD9523_PLL1_REFB_RCV_EN : 0) |
			   (config->osc_in_diff_en ? AD9523_PLL1_OSC_IN_DIFF_EN : 0) |
			   (config->osc_in_cmos_neg_inp_en ? AD9523_PLL1_OSC_IN_CMOS_NEG_INP_EN
							   : 0) |
			   (config->refa_diff_rcv_en ? AD9523_PLL1_REFA_DIFF_RCV_EN : 0) |
			   (config->refb_diff_rcv_en ? AD9523_PLL1_REFB_DIFF_RCV_EN : 0)));
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(
		dev, AD9523_PLL1_REF_CTRL,
		config->pll1_bypass_en
			? (AD9523_PLL1_BYPASS_FEEDBACK_DIV_EN | AD9523_PLL1_ZERO_DELAY_MODE_INT)
			: ((config->zd_in_diff_en ? AD9523_PLL1_ZD_IN_DIFF_EN : 0) |
			   (config->zd_in_cmos_neg_inp_en ? AD9523_PLL1_ZD_IN_CMOS_NEG_INP_EN : 0) |
			   (config->zero_delay_mode_internal_en ? AD9523_PLL1_ZERO_DELAY_MODE_INT
								: 0) |
			   (config->osc_in_feedback_en ? AD9523_PLL1_OSC_IN_PLL_FEEDBACK_EN : 0) |
			   (config->refa_cmos_neg_inp_en ? AD9523_PLL1_REFA_CMOS_NEG_INP_EN : 0) |
			   (config->refb_cmos_neg_inp_en ? AD9523_PLL1_REFB_CMOS_NEG_INP_EN : 0)));
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(dev, AD9523_PLL1_MISC_CTRL,
			       AD9523_PLL1_REFB_INDEP_DIV_CTRL_EN |
				       AD9523_PLL1_REF_MODE(config->ref_mode));
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(dev, AD9523_PLL1_LOOP_FILTER_CTRL,
			       AD9523_PLL1_LOOP_FILTER_RZERO(config->pll1_loop_filter_rzero));
	if (ret < 0) {
		return ret;
	}
	LOG_INF("SUCCESS: AD9523 PLL1 configured");

	/*
	 * PLL2 setup.
	 */
	ret = ad9523_spi_write(
		dev, AD9523_PLL2_CHARGE_PUMP,
		AD9523_PLL2_CHARGE_PUMP_CURRENT_nA(config->pll2_charge_pump_current_nA));
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(dev, AD9523_PLL2_FEEDBACK_DIVIDER_AB,
			       AD9523_PLL2_FB_NDIV_A_CNT(ndiv_a) |
				       AD9523_PLL2_FB_NDIV_B_CNT(ndiv_b));
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(dev, AD9523_PLL2_CTRL,
			       AD9523_PLL2_CHARGE_PUMP_MODE_NORMAL | AD9523_PLL2_BACKLASH_CTRL_EN |
				       (doubler ? AD9523_PLL2_FREQ_DOUBLER_EN : 0));
	if (ret < 0) {
		return ret;
	}

	/* fVCO = fVCXO * doubler / R2 * NDIV. */
	data->vco_freq =
		(config->vcxo_freq * (doubler ? 2 : 1) / r2) * AD9523_PLL2_FB_NDIV(ndiv_a, ndiv_b);

	ret = ad9523_spi_write(dev, AD9523_PLL2_VCO_CTRL, AD9523_PLL2_VCO_CALIBRATE);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(dev, AD9523_PLL2_VCO_DIVIDER,
			       AD9523_PLL2_VCO_DIV_M1(m1) | AD9523_PLL2_VCO_DIV_M2(m2) |
				       (m1 ? 0 : AD9523_PLL2_VCO_DIV_M1_PWR_DOWN_EN) |
				       (m2 ? 0 : AD9523_PLL2_VCO_DIV_M2_PWR_DOWN_EN));
	if (ret < 0) {
		return ret;
	}

	if (m1) {
		data->vco_out_freq[AD9523_VCO1] = data->vco_freq / m1;
	}
	if (m2) {
		data->vco_out_freq[AD9523_VCO2] = data->vco_freq / m2;
	}
	data->vco_out_freq[AD9523_VCXO] = config->vcxo_freq;

	ret = ad9523_spi_write(dev, AD9523_PLL2_R2_DIVIDER, AD9523_PLL2_R2_DIVIDER_VAL(r2));
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_spi_write(
		dev, AD9523_PLL2_LOOP_FILTER_CTRL,
		AD9523_PLL2_LOOP_FILTER_CPOLE1(config->cpole1) |
			AD9523_PLL2_LOOP_FILTER_RZERO(config->rzero) |
			AD9523_PLL2_LOOP_FILTER_RPOLE2(config->rpole2) |
			(config->rzero_bypass_en ? AD9523_PLL2_LOOP_FILTER_RZERO_BYPASS_EN : 0));
	if (ret < 0) {
		return ret;
	}
	LOG_INF("SUCCESS: AD9523 PLL2 configured (VCO %u.%06u MHz)", data->vco_freq / 1000000,
		data->vco_freq % 1000000);

	/*
	 * Output channels.
	 */
	for (uint32_t i = 0; i < config->num_channels; i++) {
		chan = &config->channels[i];

		active_mask |= BIT(chan->num);

		ret = ad9523_spi_write(
			dev, AD9523_CHANNEL_CLOCK_DIST(chan->num),
			AD9523_CLK_DIST_DRIVER_MODE(chan->driver_mode) |
				AD9523_CLK_DIST_DIV(chan->channel_divider) |
				AD9523_CLK_DIST_DIV_PHASE(chan->divider_phase) |
				(chan->sync_ignore_en ? AD9523_CLK_DIST_IGNORE_SYNC_EN : 0) |
				(chan->divider_output_invert_en ? AD9523_CLK_DIST_INV_DIV_OUTPUT_EN
								: 0) |
				(chan->low_power_mode_en ? AD9523_CLK_DIST_LOW_PWR_MODE_EN : 0) |
				(chan->output_dis ? AD9523_CLK_DIST_PWR_DOWN_EN : 0));
		if (ret < 0) {
			return ret;
		}

		ret = ad9523_vco_out_map(dev, chan->num, chan->use_alt_clock_src);
		if (ret < 0) {
			return ret;
		}

		WRITE_BIT(data->enabled, chan->num, !chan->output_dis);

		LOG_DBG("out%u %-16s div=%-4u -> %u Hz", chan->num, chan->name ? chan->name : "",
			chan->channel_divider,
			data->vco_out_freq[data->vco_out_map[chan->num]] / chan->channel_divider);
	}

	/* Park every unused output tristate + powered down. */
	for (uint32_t i = 0; i < AD9523_NUM_CLK_OUT; i++) {
		if (!(active_mask & BIT(i))) {
			ret = ad9523_spi_write(
				dev, AD9523_CHANNEL_CLOCK_DIST(i),
				AD9523_CLK_DIST_DRIVER_MODE(AD9523_DRIVER_MODE_TRISTATE) |
					AD9523_CLK_DIST_PWR_DOWN_EN);
			if (ret < 0) {
				return ret;
			}
		}
	}
	LOG_INF("SUCCESS: AD9523 %u outputs configured", config->num_channels);

	/* Power up all blocks. */
	ret = ad9523_spi_write(dev, AD9523_POWER_DOWN_CTRL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Route the status pins to the PLL1+PLL2 combined-lock monitor. */
	ret = ad9523_spi_write(dev, AD9523_STATUS_SIGNALS, AD9523_STATUS_MONITOR_01_PLL12_LOCKED);
	if (ret < 0) {
		return ret;
	}

	ret = ad9523_io_update(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ad9523_do_sync(dev);
	if (ret < 0) {
		return ret;
	}

	/*
	 * VCO calibration needs all registers out of the hold/buffered state, so
	 * switch readback back to live, latch, calibrate, then re-sync.
	 */
	ret = ad9523_spi_write(dev, AD9523_READBACK_CTRL, 0x0);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_io_update(dev);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_calibrate(dev);
	if (ret < 0) {
		return ret;
	}
	ret = ad9523_do_sync(dev);
	if (ret < 0) {
		return ret;
	}

	return ad9523_status_wait(dev);
}

/* --------------------------------- init ----------------------------------- */

static int ad9523_log_status(const struct device *dev)
{
	struct ad9523_status status;
	int ret;

	ret = ad9523_get_status(dev, &status);
	if (ret) {
		return ret;
	}

	LOG_INF("VCXO: %s - PLL1: %s - PLL2: %s @ VCO %u.%06u MHz",
		status.vcxo_locked ? "OK" : "FAIL",
		status.pll1_bypassed ? "bypassed" : (status.pll1_locked ? "Locked" : "Unlocked"),
		status.pll2_locked ? "Locked" : "Unlocked", status.vco_freq / 1000000,
		status.vco_freq % 1000000);

	return 0;
}

static int ad9523_init(const struct device *dev)
{
	const struct ad9523_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	/* SYNC held asserted freezes all output dividers; drive it high to release. */
	if (config->sync.port != NULL) {
		if (!gpio_is_ready_dt(&config->sync)) {
			LOG_ERR("SYNC GPIO %s not ready", config->sync.port->name);
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->sync, GPIO_OUTPUT_HIGH);
		if (ret < 0) {
			return ret;
		}
		LOG_INF("AD9523 SYNC released (driven high)");
	}

	LOG_INF("AD9523 clock setup over %s", config->spi.bus->name);

	ret = ad9523_setup(dev);
	if (ret) {
		LOG_ERR("ad9523_setup failed (%d)", ret);
		return ret;
	}
	LOG_INF("SUCCESS: AD9523 clock tree configured");

	return ad9523_log_status(dev);
}

/* ------------------------------ DT plumbing ------------------------------- */

#define AD9523_CHAN_CONFIG(child)                                                                  \
	{                                                                                          \
		.num = DT_REG_ADDR(child),                                                         \
		.name = DT_PROP_OR(child, adi_extended_name, NULL),                                \
		.driver_mode = DT_PROP(child, adi_driver_mode),                                    \
		.divider_phase = DT_PROP(child, adi_divider_phase),                                \
		.channel_divider = DT_PROP(child, adi_channel_divider),                            \
		.use_alt_clock_src = DT_PROP(child, adi_use_alt_clock_src),                        \
		.output_dis = DT_PROP(child, adi_output_dis),                                      \
		.sync_ignore_en = DT_PROP(child, adi_sync_ignore_enable),                          \
		.low_power_mode_en = DT_PROP(child, adi_low_power_mode_enable),                    \
		.divider_output_invert_en = DT_PROP(child, adi_divider_output_invert_enable),      \
	},

/*
 * Per-output range checks. A separate FOREACH pass from AD9523_CHAN_CONFIG:
 * that macro expands inside an array initializer (an expression context), while
 * BUILD_ASSERT is a declaration.
 */
#define AD9523_CHAN_ASSERTS(child)                                                                 \
	BUILD_ASSERT(DT_REG_ADDR(child) < AD9523_NUM_CLK_OUT,                                      \
		     "AD9523 output number out of range (0-13)");                                  \
	BUILD_ASSERT(DT_PROP(child, adi_channel_divider) >= 1 &&                                   \
			     DT_PROP(child, adi_channel_divider) <= 1024,                          \
		     "AD9523 adi,channel-divider out of range (1-1024)");

/*
 * The explicit PLL2 divider path and the derive path (adi,pll2-m1-freq) are
 * mutually exclusive: a node may set one or the other, never both and never
 * neither. Enforcing this at build time is CLAUDE.md's "derived params must not
 * be independently settable" -- it stops a hand-set divider from silently
 * drifting from a derived one.
 */
#define AD9523_HAS_EXPLICIT_PLL2(n)                                                                \
	(DT_INST_NODE_HAS_PROP(n, adi_pll2_r2_div) ||                                              \
	 DT_INST_NODE_HAS_PROP(n, adi_pll2_ndiv_a_cnt) ||                                          \
	 DT_INST_NODE_HAS_PROP(n, adi_pll2_ndiv_b_cnt) ||                                          \
	 DT_INST_NODE_HAS_PROP(n, adi_pll2_vco_div_m1) ||                                          \
	 DT_INST_NODE_HAS_PROP(n, adi_pll2_vco_div_m2))

#define AD9523_DEFINE(n)                                                                           \
	BUILD_ASSERT(!(DT_INST_NODE_HAS_PROP(n, adi_pll2_m1_freq) && AD9523_HAS_EXPLICIT_PLL2(n)), \
		     "set either adi,pll2-m1-freq (derived) or the explicit "                      \
		     "adi,pll2-* dividers, not both");                                             \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, adi_pll2_m1_freq) || AD9523_HAS_EXPLICIT_PLL2(n),    \
		     "set adi,pll2-m1-freq (derived) or the explicit adi,pll2-* "                  \
		     "dividers");                                                                  \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(n) > 0,                                         \
		     "an AD9523 with no enabled output child nodes does nothing");                 \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, AD9523_CHAN_ASSERTS)                                  \
                                                                                                   \
	static const struct ad9523_chan_config ad9523_channels_##n[] = {                           \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, AD9523_CHAN_CONFIG)};                         \
                                                                                                   \
	static const struct ad9523_config ad9523_config_##n = {                                    \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_WORD_SET(8) | SPI_TRANSFER_MSB |                \
						       SPI_OP_MODE_MASTER),                        \
		.sync = GPIO_DT_SPEC_INST_GET_OR(n, sync_gpios, {0}),                              \
		.vcxo_freq = DT_INST_PROP(n, adi_vcxo_freq),                                       \
		.spi3wire = DT_INST_PROP(n, adi_spi_3wire_enable),                                 \
		.refa_r_div = DT_INST_PROP(n, adi_refa_r_divider),                                 \
		.refb_r_div = DT_INST_PROP(n, adi_refb_r_divider),                                 \
		.pll1_feedback_div = DT_INST_PROP(n, adi_pll1_feedback_divider),                   \
		.pll1_charge_pump_current_nA = DT_INST_PROP(n, adi_pll1_charge_pump_current_na),   \
		.pll1_loop_filter_rzero = DT_INST_PROP(n, adi_pll1_loopfilter_rzero),              \
		.ref_mode = DT_INST_PROP(n, adi_ref_mode),                                         \
		.pll1_bypass_en = DT_INST_PROP(n, adi_pll1_bypass_enable),                         \
		.zero_delay_mode_internal_en =                                                     \
			DT_INST_PROP(n, adi_zero_delay_mode_internal_enable),                      \
		.osc_in_feedback_en = DT_INST_PROP(n, adi_osc_in_feedback_enable),                 \
		.refa_diff_rcv_en = DT_INST_PROP(n, adi_refa_diff_rcv_enable),                     \
		.refb_diff_rcv_en = DT_INST_PROP(n, adi_refb_diff_rcv_enable),                     \
		.zd_in_diff_en = DT_INST_PROP(n, adi_zd_in_diff_enable),                           \
		.osc_in_diff_en = DT_INST_PROP(n, adi_osc_in_diff_enable),                         \
		.refa_cmos_neg_inp_en = DT_INST_PROP(n, adi_refa_cmos_neg_inp_enable),             \
		.refb_cmos_neg_inp_en = DT_INST_PROP(n, adi_refb_cmos_neg_inp_enable),             \
		.zd_in_cmos_neg_inp_en = DT_INST_PROP(n, adi_zd_in_cmos_neg_inp_enable),           \
		.osc_in_cmos_neg_inp_en = DT_INST_PROP(n, adi_osc_in_cmos_neg_inp_enable),         \
		.pll2_charge_pump_current_nA = DT_INST_PROP(n, adi_pll2_charge_pump_current_na),   \
		.pll2_ndiv_a_cnt = DT_INST_PROP_OR(n, adi_pll2_ndiv_a_cnt, 0),                     \
		.pll2_ndiv_b_cnt = DT_INST_PROP_OR(n, adi_pll2_ndiv_b_cnt, 0),                     \
		.pll2_r2_div = DT_INST_PROP_OR(n, adi_pll2_r2_div, 0),                             \
		.pll2_vco_diff_m1 = DT_INST_PROP_OR(n, adi_pll2_vco_div_m1, 0),                    \
		.pll2_vco_diff_m2 = DT_INST_PROP_OR(n, adi_pll2_vco_div_m2, 0),                    \
		.pll2_freq_doubler_en = DT_INST_PROP(n, adi_pll2_freq_doubler_enable),             \
		.rpole2 = DT_INST_PROP(n, adi_rpole2),                                             \
		.rzero = DT_INST_PROP(n, adi_rzero),                                               \
		.cpole1 = DT_INST_PROP(n, adi_cpole1),                                             \
		.rzero_bypass_en = DT_INST_PROP(n, adi_rzero_bypass_enable),                       \
		.pll2_m1_freq = DT_INST_PROP_OR(n, adi_pll2_m1_freq, 0),                           \
		.channels = ad9523_channels_##n,                                                   \
		.num_channels = ARRAY_SIZE(ad9523_channels_##n),                                   \
	};                                                                                         \
                                                                                                   \
	static struct ad9523_data ad9523_data_##n;                                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ad9523_init, NULL, &ad9523_data_##n, &ad9523_config_##n,          \
			      POST_KERNEL, CONFIG_CLOCK_CONTROL_AD9523_INIT_PRIORITY,              \
			      &ad9523_api);

BUILD_ASSERT(CONFIG_CLOCK_CONTROL_AD9523_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "The AD9523 is SPI-attached, so it must initialise after its SPI controller");

DT_INST_FOREACH_STATUS_OKAY(AD9523_DEFINE)
