/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63.c
 * @brief CS47L63 SPI audio codec driver
 *
 * The part runs as a plain I2S codec: serial ports, mixers, FLL1, volume and routing. Audio
 * passes through without DSP processing; the DSP cores and their firmware are not used, so the
 * vendor's DSP-memory patch, cs47l63_common_patch(), is not applied.
 */

#define DT_DRV_COMPAT cirrus_cs47l63

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/audio/cs47l63.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/zassert.h>

#include "cs47l63.h"

LOG_MODULE_REGISTER(cs47l63, CONFIG_AUDIO_CODEC_LOG_LEVEL);
ZASSERT_MODULE(DEFAULT);

/** The part-wide levels, set through the parent. */
struct cs47l63_data {
	/* OUT1L_VOL code last requested, kept whether or not it is on the pins. */
	uint8_t out_vol_code;
	bool out_muted;
	/* Input level and mute last requested; a stop leaves them, a start restores them. */
	uint8_t in_vol_code;
	bool in_muted;
};

static struct cs47l63_chip *chip_of(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;

	return cfg->chip;
}

static struct cs47l63_port *port_of(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;

	return cfg->port;
}

static void chip_lock(struct cs47l63_chip *chip)
{
	int ret __maybe_unused = k_mutex_lock(&chip->lock, K_FOREVER);

	ZASSERT(ret == 0, "Failed to lock the codec: %d", ret);
}

static void chip_unlock(struct cs47l63_chip *chip)
{
	int ret __maybe_unused = k_mutex_unlock(&chip->lock);

	ZASSERT(ret == 0, "Failed to unlock the codec: %d", ret);
}

#define CS47L63_READ_FLAG 0x80000000U

#define CS47L63_WORD_BYTES 4U

/**
 * @brief Wire layout of every register transaction: address, padding, data.
 *
 * The vendor BSP clocks the zero padding word on writes as well as reads
 * (CirrusLogic/mcu-drivers, cs47l63/bsp/bsp_cs47l63.c, `spi_pad_len = 4`).
 */
#define CS47L63_FRAME_BYTES (3U * CS47L63_WORD_BYTES)

#define CS47L63_ADDR_OFFS 0U
#define CS47L63_PAD_OFFS  CS47L63_WORD_BYTES
#define CS47L63_DATA_OFFS (2U * CS47L63_WORD_BYTES)

#define CS47L63_SPI_WORD_BITS 8U

static int cs47l63_bus_write_reg(const struct device *dev, uint32_t addr, uint32_t val)
{
	const struct cs47l63_config *cfg = dev->config;
	uint8_t tx[CS47L63_FRAME_BYTES] = {0U};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1U};
	int ret;

	sys_put_be32(addr, &tx[CS47L63_ADDR_OFFS]);
	sys_put_be32(val, &tx[CS47L63_DATA_OFFS]);

	ret = spi_write_dt(&cfg->bus, &tx_set);
	if (ret < 0) {
		LOG_ERR("Failed to write reg 0x%05x: %d", addr, ret);
		return ret;
	}

	return 0;
}

static int cs47l63_bus_read_reg(const struct device *dev, uint32_t addr, uint32_t *val)
{
	const struct cs47l63_config *cfg = dev->config;
	uint8_t tx[CS47L63_DATA_OFFS] = {0U};
	uint8_t rx[CS47L63_WORD_BYTES];
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1U};
	/* A NULL receive buffer discards the address and padding words. */
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = sizeof(tx)},
		{.buf = rx, .len = sizeof(rx)},
	};
	const struct spi_buf_set rx_set = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};
	int ret;

	sys_put_be32(addr | CS47L63_READ_FLAG, &tx[CS47L63_ADDR_OFFS]);

	ret = spi_transceive_dt(&cfg->bus, &tx_set, &rx_set);
	if (ret < 0) {
		LOG_ERR("Failed to read reg 0x%05x: %d", addr, ret);
		return ret;
	}

	*val = sys_get_be32(rx);

	return 0;
}

static int cs47l63_bus_update_reg(const struct device *dev, uint32_t addr, uint32_t mask,
				  uint32_t val)
{
	uint32_t cur;
	uint32_t next;
	int ret;

	ret = cs47l63_bus_read_reg(dev, addr, &cur);
	if (ret < 0) {
		return ret;
	}

	next = (cur & ~mask) | (val & mask);
	if (next == cur) {
		return 0;
	}

	return cs47l63_bus_write_reg(dev, addr, next);
}

static int cs47l63_bus_poll_reg(const struct device *dev, uint32_t addr, uint32_t mask,
				uint32_t expected, uint32_t interval_ms, uint32_t max_polls,
				bool *reached)
{
	uint32_t val;
	int ret;

	*reached = false;

	for (uint32_t i = 0U; i < max_polls; i++) {
		ret = cs47l63_bus_read_reg(dev, addr, &val);
		if (ret < 0) {
			return ret;
		}

		if ((val & mask) == expected) {
			*reached = true;
			return 0;
		}

		(void)k_msleep((int32_t)interval_ms);
	}

	LOG_ERR("Reg 0x%05x never reached 0x%08x under mask 0x%08x", addr, expected, mask);

	return 0;
}

/* Reset timing, boot-done poll bound and trim block follow the vendor driver
 * (CirrusLogic/mcu-drivers, cs47l63/cs47l63.c, cs47l63_reset() and cs47l63_patch()).
 */

#define CS47L63_RESET_SETTLE_MS 2U

#define CS47L63_BOOT_POLL_MS  10U
#define CS47L63_BOOT_POLL_MAX 20U

#define CS47L63_OTPID_TRIMMED 0x8U

static void keep_first_error(int *first, int ret)
{
	if ((ret < 0) && (*first == 0)) {
		*first = ret;
	}
}

static uint32_t port_offs(const struct cs47l63_port *port, uint32_t stride)
{
	return ((uint32_t)port->asp - 1U) * stride;
}

static uint32_t port_bit(const struct cs47l63_port *port)
{
	return BIT((uint32_t)port->asp - 1U);
}

static uint16_t port_rx_src(const struct cs47l63_port *port, uint32_t asp1_src)
{
	return (uint16_t)(asp1_src + port_offs(port, CS47L63_ASP2_RX_SRC_OFFS));
}

static int write_key_pair(const struct device *dev, const uint32_t codes[CS47L63_KEY_CODES])
{
	int ret;

	for (uint32_t i = 0U; i < CS47L63_KEY_CODES; i++) {
		ret = cs47l63_bus_write_reg(dev, CS47L63_TEST_KEY_CTRL, codes[i]);
		if (ret < 0) {
			return ret;
		}
		ret = cs47l63_bus_write_reg(dev, CS47L63_USER_KEY_CTRL, codes[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static void port_reset(struct cs47l63_port *port)
{
	port->route = AUDIO_ROUTE_BYPASS;
	port->out_started = false;
	port->in_started = false;
	port->src1 = port_rx_src(port, CS47L63_MIXER_SRC_ASP1RX1);
	port->src2 = port_rx_src(port, CS47L63_MIXER_SRC_ASP1RX2);
	port->in_terminal = CS47L63_INPUT_LINE;
	port->in_src1 = CS47L63_MIXER_SRC_IN2L;
	port->in_src2 = CS47L63_MIXER_SRC_IN2R;
}

/* Reset is held low for both of the vendor's settles; the DCVDD enable the vendor makes
 * between them has no counterpart here.
 */
static int hw_reset(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;
	struct cs47l63_chip *chip = cfg->chip;
	int ret;

	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	chip->configured = 0U;
	chip->out_started = 0U;
	chip->out_running = false;
	(void)memset(chip->in_users, 0, sizeof(chip->in_users));
	for (size_t i = 0U; i < ARRAY_SIZE(chip->ports); i++) {
		if (chip->ports[i] != NULL) {
			port_reset(chip->ports[i]);
		}
	}

	(void)k_msleep((int32_t)(CS47L63_RESET_SETTLE_MS * 2U));

	return gpio_pin_set_dt(&cfg->reset_gpio, 0);
}

/* DEVID must read 0x047A63, the value DS1249F2 gives for R0 (table 4-69). */
static int identify(const struct device *dev)
{
	uint32_t devid;
	uint32_t revid;
	int ret;

	ret = cs47l63_bus_read_reg(dev, CS47L63_DEVID, &devid);
	if (ret < 0) {
		return ret;
	}
	devid &= CS47L63_DEVID_MASK;

	if (devid != CS47L63_DEVID_CS47L63) {
		LOG_ERR("Unexpected device ID 0x%06x - check wiring and SPI mode", devid);
		return -ENODEV;
	}

	ret = cs47l63_bus_read_reg(dev, CS47L63_REVID, &revid);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("CS47L63 devid 0x%06x, rev %u.%u", devid,
		(revid & CS47L63_REVID_AREVID_MASK) >> CS47L63_REVID_AREVID_SHIFT,
		revid & CS47L63_REVID_MTLREVID_MASK);

	return 0;
}

static int apply_trim(const struct device *dev)
{
	/* Trim block for OTP variant 8, as the vendor's cs47l63_otpid_8_patch() writes it. */
	static const struct {
		uint32_t addr;
		uint32_t val;
	} k_trim_otpid_8[] = {
		{CS47L63_DAC_IF_CONTROL_1, 0x1DB10000U},  {CS47L63_DAC_IF_TEST_1, 0x700249B8U},
		{CS47L63_HP_OCD_CTRL1, 0x00010000U},      {CS47L63_HP_OCD_TEST1, 0x000005FFU},
		{CS47L63_MICBIAS_TST_CTRL1, 0x04150415U}, {CS47L63_MICBIAS_TST_CTRL4, 0x00000415U},
	};
	static const uint32_t k_key_unlock[CS47L63_KEY_CODES] = {CS47L63_KEY_UNLOCK_CODE0,
								 CS47L63_KEY_UNLOCK_CODE1};
	static const uint32_t k_key_lock[CS47L63_KEY_CODES] = {CS47L63_KEY_LOCK_CODE0,
							       CS47L63_KEY_LOCK_CODE1};
	uint32_t otpid;
	int ret;

	ret = cs47l63_bus_read_reg(dev, CS47L63_OTPID, &otpid);
	if (ret < 0) {
		return ret;
	}
	otpid &= CS47L63_OTPID_MASK;

	if (otpid != CS47L63_OTPID_TRIMMED) {
		LOG_DBG("OTP variant %u needs no trim block", otpid);
		return 0;
	}

	ret = write_key_pair(dev, k_key_unlock);
	if (ret == 0) {
		for (size_t i = 0U; i < ARRAY_SIZE(k_trim_otpid_8); i++) {
			ret = cs47l63_bus_write_reg(dev, k_trim_otpid_8[i].addr,
						    k_trim_otpid_8[i].val);
			if (ret < 0) {
				break;
			}
		}
	}

	keep_first_error(&ret, write_key_pair(dev, k_key_lock));

	return ret;
}

static int apply_gpio_defaults(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;
	int ret;

	for (uint32_t i = 0U; i < cfg->gpio_defaults_len; i++) {
		if ((cfg->gpio_defaults[i] & CS47L63_GP_CTRL1_RSVD) != 0U) {
			continue;
		}
		ret = cs47l63_bus_write_reg(dev, CS47L63_GPIO1_CTRL1 + (i * CS47L63_WORD_BYTES),
					    cfg->gpio_defaults[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cs47l63_boot_bringup(const struct device *dev)
{
	bool reached;
	int ret;

	ret = hw_reset(dev);
	if (ret < 0) {
		LOG_ERR("Reset GPIO failed: %d", ret);
		return ret;
	}

	(void)k_msleep((int32_t)CS47L63_BOOT_POLL_MS);
	ret = cs47l63_bus_poll_reg(dev, CS47L63_IRQ1_EINT_2, CS47L63_BOOT_DONE_EINT1,
				   CS47L63_BOOT_DONE_EINT1, CS47L63_BOOT_POLL_MS,
				   CS47L63_BOOT_POLL_MAX, &reached);
	if (ret < 0) {
		return ret;
	}
	if (!reached) {
		return -ETIMEDOUT;
	}

	ret = identify(dev);
	if (ret < 0) {
		return ret;
	}

	ret = apply_trim(dev);
	if (ret < 0) {
		return ret;
	}

	return apply_gpio_defaults(dev);
}

/* FLL1 terms follow DS1249F2 section 4.10.7.4 and table 4-51, as the vendor's
 * cs47l63_fll_do_config() does, and the write order its cs47l63_fll_apply_config()
 * (CirrusLogic/mcu-drivers, cs47l63/cs47l63.c).
 */

#define CS47L63_FLL_LOW_THRESH 192000U
#define CS47L63_FLL_MID_THRESH 1152000U
#define CS47L63_FLL_MAX_THRESH 13000000U
#define CS47L63_FLL_LOW_GAINS  0x23F0U
#define CS47L63_FLL_MID_GAINS  0x22F2U
#define CS47L63_FLL_HIGH_GAINS 0x21F0U

#define CS47L63_FLL_LOW_LOCKDET_THR 0x2U
#define CS47L63_FLL_LOCKDET_THR     0x8U

#define CS47L63_FLL_LOW_INT_FB_DIV  4U
#define CS47L63_FLL_LOW_FRAC_FB_DIV 256U
#define CS47L63_FLL_MID_INT_FB_DIV  2U
#define CS47L63_FLL_MID_FRAC_FB_DIV 16U
#define CS47L63_FLL_HIGH_FB_DIV     1U
#define CS47L63_FLL_FB_DIV_STEP     2U
#define CS47L63_FLL_FB_DIV_MAX      (CS47L63_FLL1_FB_DIV_MASK >> CS47L63_FLL1_FB_DIV_SHIFT)

/* Below this the loop runs in its low-power integer mode. */
#define CS47L63_FLL_LP_INT_THRESH 100000U

#define CS47L63_FLL_INT_MIN_N  1U
#define CS47L63_FLL_INT_MAX_N  1023U
#define CS47L63_FLL_FRAC_MIN_N 2U
#define CS47L63_FLL_FRAC_MAX_N 255U

#define CS47L63_FLL_REFDIV_MAX_SHIFT 3U

int cs47l63_clock_solve(uint32_t fref_hz, struct cs47l63_fll_solution *out)
{
	uint32_t refdiv;
	uint32_t fref;
	uint32_t fbdiv;
	uint32_t int_fbdiv;
	uint32_t frac_fbdiv;
	uint32_t gains;
	uint32_t lockdet_thr;
	uint32_t hp;
	uint32_t min_n;
	uint32_t max_n;
	uint32_t ratio;
	uint32_t divisor;
	uint32_t num;
	uint32_t lambda;
	uint32_t n;
	uint32_t theta;
	bool frac;

	if (out == NULL) {
		return -EINVAL;
	}
	if (fref_hz == 0U) {
		return -EINVAL;
	}

	for (refdiv = 0U; refdiv < CS47L63_FLL_REFDIV_MAX_SHIFT; refdiv++) {
		if ((fref_hz >> refdiv) <= CS47L63_FLL_MAX_THRESH) {
			break;
		}
	}

	fref = fref_hz >> refdiv;
	if (fref > CS47L63_FLL_MAX_THRESH) {
		return -ENOTSUP;
	}
	if ((fref << refdiv) != fref_hz) {
		return -ENOTSUP;
	}

	if (fref < CS47L63_FLL_LOW_THRESH) {
		lockdet_thr = CS47L63_FLL_LOW_LOCKDET_THR;
		gains = CS47L63_FLL_LOW_GAINS;
		int_fbdiv = CS47L63_FLL_LOW_INT_FB_DIV;
		frac_fbdiv = CS47L63_FLL_LOW_FRAC_FB_DIV;
	} else if (fref < CS47L63_FLL_MID_THRESH) {
		lockdet_thr = CS47L63_FLL_LOCKDET_THR;
		gains = CS47L63_FLL_MID_GAINS;
		int_fbdiv = CS47L63_FLL_MID_INT_FB_DIV;
		frac_fbdiv = CS47L63_FLL_MID_FRAC_FB_DIV;
	} else {
		lockdet_thr = CS47L63_FLL_LOCKDET_THR;
		gains = CS47L63_FLL_HIGH_GAINS;
		int_fbdiv = CS47L63_FLL_HIGH_FB_DIV;
		frac_fbdiv = CS47L63_FLL_HIGH_FB_DIV;
	}

	/* The ratio at the loop includes FB_DIV, so the mode follows it, not the reference. */
	frac = (CS47L63_FLL_FOUT_HZ % (fref * int_fbdiv)) != 0U;
	fbdiv = frac ? frac_fbdiv : int_fbdiv;

	if (frac) {
		/* A fractional ratio needs the high-performance loop. */
		hp = CS47L63_FLL1_HP_FRAC;
		min_n = CS47L63_FLL_FRAC_MIN_N;
		max_n = CS47L63_FLL_FRAC_MAX_N;
	} else {
		hp = (fref < CS47L63_FLL_LP_INT_THRESH) ? CS47L63_FLL1_HP_LP_INT
							: CS47L63_FLL1_HP_INT;
		min_n = CS47L63_FLL_INT_MIN_N;
		max_n = CS47L63_FLL_INT_MAX_N;
	}

	ratio = CS47L63_FLL_FOUT_HZ / fref;
	while ((ratio / fbdiv) < min_n) {
		fbdiv /= CS47L63_FLL_FB_DIV_STEP;
		if (fbdiv < min_n) {
			return -ENOTSUP;
		}
	}
	while (frac && ((ratio / fbdiv) > max_n)) {
		fbdiv *= CS47L63_FLL_FB_DIV_STEP;
		if (fbdiv > CS47L63_FLL_FB_DIV_MAX) {
			return -ENOTSUP;
		}
	}

	/* N + theta/lambda is fout/(fref * fbdiv) in lowest terms. */
	divisor = sys_gcd(CS47L63_FLL_FOUT_HZ, fbdiv * fref);
	num = CS47L63_FLL_FOUT_HZ / divisor;
	lambda = (fref * fbdiv) / divisor;
	n = num / lambda;
	theta = num % lambda;

	if (!IN_RANGE(n, min_n, max_n)) {
		return -ENOTSUP;
	}
	if ((lambda > (uint32_t)UINT16_MAX) || (theta > (uint32_t)UINT16_MAX)) {
		return -ENOTSUP;
	}

	out->refclk_div = (uint8_t)refdiv;
	out->lockdet_thr = (uint8_t)lockdet_thr;
	out->hp = (uint8_t)hp;
	out->n = (uint16_t)n;
	out->fb_div = (uint16_t)fbdiv;
	out->lambda = (uint16_t)lambda;
	out->theta = (uint16_t)theta;
	out->gains = (uint16_t)gains;

	return 0;
}

/* Does not wait for lock: the reference, MCLK1, runs only while an I2S transfer does. */
static int cs47l63_clock_apply(const struct device *dev, const struct cs47l63_fll_solution *sol)
{
	const uint32_t lockdet = (uint32_t)sol->lockdet_thr << CS47L63_FLL1_LOCKDET_THR_SHIFT;
	const uint32_t refdiv = (uint32_t)sol->refclk_div << CS47L63_FLL1_REFCLK_DIV_SHIFT;
	const uint32_t n = (uint32_t)sol->n << CS47L63_FLL1_N_SHIFT;
	const uint32_t lambda = (uint32_t)sol->lambda << CS47L63_FLL1_LAMBDA_SHIFT;
	const uint32_t theta = (uint32_t)sol->theta << CS47L63_FLL1_THETA_SHIFT;
	const uint32_t gains = (uint32_t)sol->gains << CS47L63_FLL1_GAIN_SHIFT;
	const uint32_t hp = (uint32_t)sol->hp << CS47L63_FLL1_HP_SHIFT;
	const uint32_t fb_div = (uint32_t)sol->fb_div << CS47L63_FLL1_FB_DIV_SHIFT;
	const uint32_t ctrl2_mask = CS47L63_FLL1_LOCKDET_THR_MASK | CS47L63_FLL1_LOCKDET |
				    CS47L63_FLL1_PHASEDET | CS47L63_FLL1_REFDET |
				    CS47L63_FLL1_REFCLK_DIV_MASK | CS47L63_FLL1_N_MASK;
	const uint32_t ctrl2 = lockdet | CS47L63_FLL1_LOCKDET | CS47L63_FLL1_PHASEDET |
			       CS47L63_FLL1_REFDET | refdiv | n;
	const uint32_t ctrl4_mask =
		CS47L63_FLL1_GAIN_MASK | CS47L63_FLL1_HP_MASK | CS47L63_FLL1_FB_DIV_MASK;
	const uint32_t ctrl4 = gains | hp | fb_div;
	const uint32_t sysclk_mask = CS47L63_SYSCLK_FRAC | CS47L63_SYSCLK_FREQ_MASK |
				     CS47L63_SYSCLK_EN | CS47L63_SYSCLK_SRC_MASK;
	const uint32_t sysclk = (CS47L63_SYSCLK_FREQ_49M152 << CS47L63_SYSCLK_FREQ_SHIFT) |
				CS47L63_SYSCLK_EN | CS47L63_SYSCLK_SRC_FLL1;
	int ret;

	/* Held first, so a failure partway through leaves the loop held. */
	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_HOLD,
				     CS47L63_FLL1_HOLD);
	if (ret < 0) {
		return ret;
	}

	/* The vendor sets LOCKDET in cs47l63_syscfg_regs[], which this driver does not apply. */
	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL2, ctrl2_mask, ctrl2);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_write_reg(dev, CS47L63_FLL1_CONTROL3, lambda | theta);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL4, ctrl4_mask, ctrl4);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL2, CS47L63_FLL1_REFCLK_SRC_MASK,
				     CS47L63_FLL_SRC_MCLK1 << CS47L63_FLL1_REFCLK_SRC_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_EN, CS47L63_FLL1_EN);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_CTRL_UPD,
				     CS47L63_FLL1_CTRL_UPD);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_HOLD, 0U);
	if (ret < 0) {
		return ret;
	}

	return cs47l63_bus_update_reg(dev, CS47L63_SYSTEM_CLOCK1, sysclk_mask, sysclk);
}

/* FLL1_LOCK_STS1 is live, not latched: read it after CS47L63_FLL_LOCK_SETTLE_MS. */
static int cs47l63_clock_locked(const struct device *dev, bool *locked)
{
	uint32_t sts;
	int ret;

	ret = cs47l63_bus_read_reg(dev, CS47L63_IRQ1_STS_6, &sts);
	if (ret < 0) {
		return ret;
	}

	*locked = (sts & CS47L63_FLL1_LOCK_STS1) != 0U;

	return 0;
}

static int rate_code(uint32_t frame_clk_hz, uint8_t *code)
{
	static const struct {
		uint32_t hz;
		uint8_t code;
	} k_rates[] = {
		{48000U, CS47L63_SAMPLE_RATE_48K},
		{24000U, CS47L63_SAMPLE_RATE_24K},
		{16000U, CS47L63_SAMPLE_RATE_16K},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(k_rates); i++) {
		if (k_rates[i].hz == frame_clk_hz) {
			*code = k_rates[i].code;
			return 0;
		}
	}

	return -ENOTSUP;
}

static int check_word_size(uint8_t word_size)
{
	static const uint8_t k_word_sizes[] = {16U, 24U, 32U};

	for (size_t i = 0U; i < ARRAY_SIZE(k_word_sizes); i++) {
		if (k_word_sizes[i] == word_size) {
			return 0;
		}
	}

	return -ENOTSUP;
}

#define CS47L63_CHANNELS_MONO   1U
#define CS47L63_CHANNELS_STEREO 2U

int cs47l63_dai_solve(audio_dai_type_t dai_type, const struct i2s_config *i2s,
		      struct cs47l63_dai_solution *out)
{
	uint8_t code;
	int ret;

	if ((i2s == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	if (dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("Only I2S is implemented, got DAI type %d", dai_type);
		return -ENOTSUP;
	}

	if ((uint32_t)i2s->format != (uint32_t)I2S_FMT_DATA_FORMAT_I2S) {
		LOG_ERR("Only standard I2S is implemented, got format 0x%x", (uint32_t)i2s->format);
		return -ENOTSUP;
	}

	/* The host drives both clocks, so the codec must be the target of both. */
	if ((i2s->options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) !=
	    (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) {
		LOG_ERR("Codec as clock controller requested; it is always the target of both "
			"clocks here");
		return -ENOTSUP;
	}

	ret = check_word_size(i2s->word_size);
	if (ret < 0) {
		LOG_ERR("Unsupported word size %u", (uint32_t)i2s->word_size);
		return ret;
	}

	ret = rate_code(i2s->frame_clk_freq, &code);
	if (ret < 0) {
		LOG_ERR("No rate-field encoding for %u Hz", i2s->frame_clk_freq);
		return ret;
	}

	if (i2s->channels == CS47L63_CHANNELS_MONO) {
		out->enables = CS47L63_ASP1_RX1_EN;
		out->tx_enables = CS47L63_ASP1_TX1_EN;
	} else if (i2s->channels == CS47L63_CHANNELS_STEREO) {
		out->enables = CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN;
		out->tx_enables = CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN;
	} else {
		LOG_ERR("Unsupported channel count %u", (uint32_t)i2s->channels);
		return -ENOTSUP;
	}

	out->rate_code = code;
	out->word_len = (uint8_t)i2s->word_size;

	return 0;
}

/* rate is false when another port runs the part and SAMPLE_RATE1 already holds it. */
static int port_dai_apply(const struct device *dev, const struct cs47l63_port *port,
			  const struct cs47l63_dai_solution *sol, bool rate)
{
	/** The four pads that carry ASP1, in pin order. */
	static const struct {
		uint32_t addr;
		uint32_t val;
	} k_asp1_pads[] = {
		/* The codec drives DOUT; the host drives the other three, so they are inputs. */
		{CS47L63_GPIO1_CTRL1, CS47L63_GP_CTRL1_ASP_PAD},
		{CS47L63_GPIO2_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
		{CS47L63_GPIO3_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
		{CS47L63_GPIO4_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
	};
	const uint32_t block = port_offs(port, CS47L63_ASP2_BLOCK_OFFS);
	const uint32_t pads = port_offs(port, CS47L63_ASP2_PAD_OFFS);
	const uint32_t width = ((uint32_t)sol->word_len << CS47L63_ASP1_RX_WIDTH_SHIFT) |
			       ((uint32_t)sol->word_len << CS47L63_ASP1_TX_WIDTH_SHIFT);
	const uint32_t fmt = CS47L63_ASP1_FMT_I2S << CS47L63_ASP1_FMT_SHIFT;
	const uint32_t ctrl2_mask = CS47L63_ASP1_RX_WIDTH_MASK | CS47L63_ASP1_TX_WIDTH_MASK |
				    CS47L63_ASP1_FMT_MASK | CS47L63_ASP1_BCLK_INV |
				    CS47L63_ASP1_BCLK_MSTR | CS47L63_ASP1_FSYNC_INV |
				    CS47L63_ASP1_FSYNC_MSTR;
	int ret;

	for (size_t i = 0U; i < ARRAY_SIZE(k_asp1_pads); i++) {
		ret = cs47l63_bus_write_reg(dev, k_asp1_pads[i].addr + pads, k_asp1_pads[i].val);
		if (ret < 0) {
			return ret;
		}
	}

	if (rate) {
		ret = cs47l63_bus_update_reg(dev, CS47L63_SAMPLE_RATE1, CS47L63_SAMPLE_RATE_MASK,
					     sol->rate_code);
		if (ret < 0) {
			return ret;
		}
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_CONTROL1 + block, CS47L63_ASP1_RATE_MASK,
				     CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1 << CS47L63_ASP1_RATE_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_CONTROL2 + block, ctrl2_mask, width | fmt);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_DATA_CONTROL1 + block,
				     CS47L63_ASP1_TX_WL_MASK, sol->word_len);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_DATA_CONTROL5 + block,
				     CS47L63_ASP1_RX_WL_MASK, sol->word_len);
	if (ret < 0) {
		return ret;
	}

	return cs47l63_bus_write_reg(dev, CS47L63_ASP1_ENABLES1 + block, sol->enables);
}

#define CS47L63_EINT_1_WATCHED                                                                     \
	(CS47L63_OUT1L_SC_EINT1 | CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)

#define CS47L63_EINT_6_WATCHED (CS47L63_FLL1_REF_LOST_EINT1 | CS47L63_FLL1_LOCK_FALL_EINT1)

static int cs47l63_fault_register_callback(const struct device *dev,
					   audio_codec_error_callback_t cb)
{
	struct cs47l63_chip *chip = chip_of(dev);
	const uint32_t idx = (uint32_t)port_of(dev)->asp - 1U;

	chip_lock(chip);
	chip->fault_cb[idx] = cb;
	chip->fault_dev[idx] = dev;
	chip_unlock(chip);

	return 0;
}

static void cs47l63_fault_raise(const struct device *dev, uint32_t errors)
{
	struct cs47l63_chip *chip = chip_of(dev);
	uint32_t fresh;

	fresh = errors & ~chip->fault_errors;
	chip->fault_errors |= errors;

	if (fresh != 0U) {
		const uint32_t reported = chip->fault_errors;

		LOG_WRN("CS47L63 fault, errors 0x%02x", reported);
		for (size_t i = 0U; i < ARRAY_SIZE(chip->fault_cb); i++) {
			if (chip->fault_cb[i] != NULL) {
				chip->fault_cb[i](chip->fault_dev[i], reported);
			}
		}
	}
}

/* Polled from start_output and output volume or mute sets, and run by the IRQ work. */
static int cs47l63_fault_check(const struct device *dev)
{
	uint32_t eint1;
	uint32_t eint6;
	uint32_t errors = 0U;
	int ret;

	ret = cs47l63_bus_read_reg(dev, CS47L63_IRQ1_EINT_1, &eint1);
	if (ret < 0) {
		return ret;
	}
	eint1 &= CS47L63_EINT_1_WATCHED;

	ret = cs47l63_bus_read_reg(dev, CS47L63_IRQ1_EINT_6, &eint6);
	if (ret < 0) {
		return ret;
	}
	eint6 &= CS47L63_EINT_6_WATCHED;

	if ((eint1 & CS47L63_OUT1L_SC_EINT1) != 0U) {
		errors |= (uint32_t)AUDIO_CODEC_ERROR_OVERCURRENT;
	}

	/* The clock latches are a fault only while the output is running. */
	if (chip_of(dev)->out_running &&
	    (((eint1 & (CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)) != 0U) ||
	     (eint6 != 0U))) {
		errors |= CS47L63_ERROR_CLOCK;
	}

	/* Cleared before reporting: the flags are edge latches, so a present fault re-latches. */
	if (eint1 != 0U) {
		ret = cs47l63_bus_write_reg(dev, CS47L63_IRQ1_EINT_1, eint1);
	}
	if (eint6 != 0U) {
		keep_first_error(&ret, cs47l63_bus_write_reg(dev, CS47L63_IRQ1_EINT_6, eint6));
	}

	cs47l63_fault_raise(dev, errors);

	return ret;
}

/* Also clears the part's sticky flags, or the next poll re-reports the same fault. */
static int cs47l63_fault_clear(const struct device *dev)
{
	struct cs47l63_chip *chip = chip_of(dev);
	int ret;

	chip_lock(chip);
	chip->fault_errors = 0U;

	ret = cs47l63_bus_write_reg(chip->dev, CS47L63_IRQ1_EINT_1, CS47L63_EINT_1_WATCHED);
	if (ret == 0) {
		ret = cs47l63_bus_write_reg(chip->dev, CS47L63_IRQ1_EINT_6,
					    CS47L63_EINT_6_WATCHED);
	}
	chip_unlock(chip);

	return ret;
}

static void cs47l63_irq_disarm(struct cs47l63_chip *chip)
{
	const struct cs47l63_config *cfg = chip->dev->config;
	int ret;

	if (cfg->irq_gpio.port == NULL) {
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable the IRQ interrupt: %d", ret);
	}
	chip->irq_armed = false;
	(void)k_work_cancel(&chip->irq_work);
}

static int cs47l63_irq_arm(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;
	struct cs47l63_chip *chip = cfg->chip;
	int ret;

	if (cfg->irq_gpio.port == NULL) {
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret == -ENOTSUP) {
		LOG_WRN_ONCE("IRQ line has no edge interrupt, faults are polled only");
		return 0;
	}
	if (ret < 0) {
		LOG_ERR("Failed to enable the IRQ interrupt: %d", ret);
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_IRQ1_MASK_2, CS47L63_BOOT_DONE_EINT1,
				     CS47L63_BOOT_DONE_EINT1);
	if (ret == 0) {
		ret = cs47l63_bus_update_reg(dev, CS47L63_IRQ1_MASK_1, CS47L63_EINT_1_WATCHED,
					     0U);
	}
	if (ret == 0) {
		ret = cs47l63_bus_update_reg(dev, CS47L63_IRQ1_MASK_6, CS47L63_EINT_6_WATCHED,
					     0U);
	}
	if (ret == 0) {
		chip->irq_armed = true;
		ret = gpio_pin_get_dt(&cfg->irq_gpio);
		if (ret > 0) {
			ret = k_work_submit(&chip->irq_work);
		}
	}
	if (ret < 0) {
		LOG_ERR("Failed to arm the IRQ line: %d", ret);
		cs47l63_irq_disarm(chip);
		return ret;
	}

	return 0;
}

static void cs47l63_irq_isr(const struct device *port, struct gpio_callback *cb,
			    gpio_port_pins_t pins)
{
	struct cs47l63_chip *chip = CONTAINER_OF(cb, struct cs47l63_chip, irq_cb);
	int ret;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	ret = k_work_submit(&chip->irq_work);
	if (ret < 0) {
		LOG_ERR("Failed to queue the IRQ service: %d", ret);
	}
}

static void cs47l63_irq_work(struct k_work *work)
{
	struct cs47l63_chip *chip = CONTAINER_OF(work, struct cs47l63_chip, irq_work);
	const struct cs47l63_config *cfg = chip->dev->config;
	bool released = false;
	int ret = 0;

	chip_lock(chip);
	if (!chip->irq_armed) {
		chip_unlock(chip);
		return;
	}

	for (uint32_t pass = 0U; (pass < CS47L63_IRQ_PASSES_MAX) && !released; pass++) {
		ret = cs47l63_fault_check(chip->dev);
		if (ret < 0) {
			LOG_ERR("Failed to service the IRQ line: %d", ret);
			break;
		}

		ret = gpio_pin_get_dt(&cfg->irq_gpio);
		if (ret < 0) {
			LOG_ERR("Failed to read the IRQ line: %d", ret);
			break;
		}
		released = (ret == 0);
	}

	if (ret > 0) {
		LOG_WRN("IRQ line stays asserted, faults are polled only until the next configure");
		cs47l63_irq_disarm(chip);
	}
	chip_unlock(chip);
}

#define CS47L63_IN2_EN_BOTH (CS47L63_IN2L_EN | CS47L63_IN2R_EN)

#define CS47L63_IN1_EN_BOTH (CS47L63_IN1L_EN | CS47L63_IN1R_EN)

#define CS47L63_ASP1_TX_EN_BOTH (CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN)

#define CS47L63_MICB1B_ON (CS47L63_MICB1B_EN | CS47L63_MICB1B_SRC)

#define CS47L63_OUT1L_INPUT_STRIDE (CS47L63_OUT1L_INPUT2 - CS47L63_OUT1L_INPUT1)

static int write_mixers(const struct device *dev, const struct cs47l63_port *port, bool tx,
			uint16_t src1, uint16_t src2)
{
	const uint32_t mix_vol = (uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT;
	const uint16_t src[] = {src1, src2};
	uint32_t addr;
	uint32_t stride;
	int ret;

	if (tx) {
		addr = CS47L63_ASP1TX1_INPUT1 + port_offs(port, CS47L63_ASP2_TX_MIX_OFFS);
		stride = CS47L63_ASP1TX2_INPUT1 - CS47L63_ASP1TX1_INPUT1;
	} else {
		addr = CS47L63_OUT1L_INPUT1 +
		       (((uint32_t)port->out_mix_input - 1U) * CS47L63_OUT1L_INPUT_STRIDE);
		stride = CS47L63_OUT1L_INPUT_STRIDE;
	}

	for (uint32_t i = 0U; i < ARRAY_SIZE(src); i++) {
		ret = cs47l63_bus_update_reg(dev, addr + (i * stride),
					     CS47L63_OUT1LMIX_VOL_MASK | CS47L63_OUT1L_SRC_MASK,
					     mix_vol | (uint32_t)src[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/* An update, never a write: the receive slots share this register. */
static int write_tx_slots(const struct device *dev, const struct cs47l63_port *port, bool on)
{
	return cs47l63_bus_update_reg(
		dev, CS47L63_ASP1_ENABLES1 + port_offs(port, CS47L63_ASP2_BLOCK_OFFS),
		CS47L63_ASP1_TX_EN_BOTH, on ? port->tx_enables : 0U);
}

static int write_in_level(const struct device *dev, enum cs47l63_input input, bool mute)
{
	static const uint32_t k_control2[CS47L63_INPUT_COUNT][CS47L63_CHANNELS_STEREO] = {
		[CS47L63_INPUT_LINE] = {CS47L63_IN2L_CONTROL2, CS47L63_IN2R_CONTROL2},
		[CS47L63_INPUT_PDM] = {CS47L63_IN1L_CONTROL2, CS47L63_IN1R_CONTROL2},
	};
	const struct cs47l63_data *data = dev->data;
	uint32_t mask = CS47L63_IN2_MUTE | CS47L63_IN2_VOL_MASK;
	uint32_t val = (uint32_t)data->in_vol_code << CS47L63_IN2_VOL_SHIFT;
	int ret;

	if (input == CS47L63_INPUT_LINE) {
		mask |= CS47L63_IN2_PGA_VOL_MASK;
		val |= (uint32_t)CS47L63_IN2_PGA_VOL_0DB << CS47L63_IN2_PGA_VOL_SHIFT;
	}

	if (mute) {
		val |= CS47L63_IN2_MUTE;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(k_control2[input]); i++) {
		ret = cs47l63_bus_update_reg(dev, k_control2[input][i], mask, val);
		if (ret < 0) {
			return ret;
		}
	}

	return cs47l63_bus_write_reg(dev, CS47L63_INPUT_CONTROL3, CS47L63_IN_VU);
}

/* MICB_SC_EINT1 is masked across the disable and cleared. */
static int micbias_off(const struct device *dev)
{
	uint32_t ctrl5;
	uint32_t mask1;
	int ret;

	ret = cs47l63_bus_read_reg(dev, CS47L63_MICBIAS_CTRL5, &ctrl5);
	if (ret < 0) {
		return ret;
	}
	if ((ctrl5 & CS47L63_MICB1B_EN) == 0U) {
		return 0;
	}

	ret = cs47l63_bus_read_reg(dev, CS47L63_IRQ1_MASK_1, &mask1);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_IRQ1_MASK_1, CS47L63_MICB_SC_MASK1,
				     CS47L63_MICB_SC_MASK1);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_MICBIAS_CTRL5, CS47L63_MICB1B_ON, 0U);
	if (ret < 0) {
		return ret;
	}

	keep_first_error(&ret,
			 cs47l63_bus_write_reg(dev, CS47L63_IRQ1_EINT_1, CS47L63_MICB_SC_EINT1));
	keep_first_error(&ret,
			 cs47l63_bus_update_reg(dev, CS47L63_IRQ1_MASK_1, CS47L63_MICB_SC_MASK1,
						mask1 & CS47L63_MICB_SC_MASK1));
	keep_first_error(&ret,
			 cs47l63_bus_update_reg(dev, CS47L63_CLOCK32K, CS47L63_CLK_32K_EN, 0U));

	return ret;
}

static bool in_running(const struct cs47l63_chip *chip)
{
	return (chip->in_users[CS47L63_INPUT_LINE] | chip->in_users[CS47L63_INPUT_PDM]) != 0U;
}

static int write_captured_levels(const struct device *dev, bool mute)
{
	const struct cs47l63_chip *chip = chip_of(dev);
	int ret;

	if (chip->in_users[CS47L63_INPUT_LINE] != 0U) {
		ret = write_in_level(dev, CS47L63_INPUT_LINE, mute);
		if (ret < 0) {
			return ret;
		}
	}

	if (chip->in_users[CS47L63_INPUT_PDM] != 0U) {
		ret = write_in_level(dev, CS47L63_INPUT_PDM, mute);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int enable_in_hpf(const struct device *dev)
{
	static const uint32_t k_control1[] = {
		CS47L63_IN1L_CONTROL1,
		CS47L63_IN1R_CONTROL1,
		CS47L63_IN2L_CONTROL1,
		CS47L63_IN2R_CONTROL1,
	};
	int ret;

	for (size_t i = 0U; i < ARRAY_SIZE(k_control1); i++) {
		ret = cs47l63_bus_update_reg(dev, k_control1[i], CS47L63_IN_HPF, CS47L63_IN_HPF);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cs47l63_in_init(const struct device *dev, struct cs47l63_port *port)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->in_vol_code = CS47L63_IN_VOL_0DB;
	data->in_muted = false;

	ret = write_tx_slots(dev, port, false);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL,
				     CS47L63_IN2_EN_BOTH | CS47L63_IN1_EN_BOTH, 0U);
	if (ret < 0) {
		return ret;
	}

	ret = write_mixers(dev, port, true, CS47L63_MIXER_SRC_NONE, CS47L63_MIXER_SRC_NONE);
	if (ret < 0) {
		return ret;
	}

	ret = write_in_level(dev, CS47L63_INPUT_LINE, true);
	if (ret < 0) {
		return ret;
	}

	ret = write_in_level(dev, CS47L63_INPUT_PDM, true);
	if (ret < 0) {
		return ret;
	}

	ret = enable_in_hpf(dev);
	if (ret < 0) {
		return ret;
	}

	return micbias_off(dev);
}

static int line_start(const struct device *dev, const struct cs47l63_port *port)
{
	const struct cs47l63_data *data = dev->data;
	int ret;

	/* The rate field's reset value selects SAMPLE_RATE1 (DS1249F2 table 4-20). */
	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT2_CONTROL1,
				     CS47L63_IN2_OSR_MASK | CS47L63_IN2_MODE,
				     (uint32_t)CS47L63_IN2_OSR_3M072 << CS47L63_IN2_OSR_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_IN2L_CONTROL1, CS47L63_IN2_SRC_MASK,
				     (uint32_t)CS47L63_IN2_SRC_SINGLE_ENDED
					     << CS47L63_IN2_SRC_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_IN2R_CONTROL1, CS47L63_IN2_SRC_MASK,
				     (uint32_t)CS47L63_IN2_SRC_SINGLE_ENDED
					     << CS47L63_IN2_SRC_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = write_mixers(dev, port, true, port->in_src1, port->in_src2);
	if (ret < 0) {
		return ret;
	}

	ret = write_in_level(dev, CS47L63_INPUT_LINE, data->in_muted);
	if (ret < 0) {
		return ret;
	}

	return cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL, CS47L63_IN2_EN_BOTH,
				      CS47L63_IN2_EN_BOTH);
}

static int pdm_start_path(const struct device *dev, const struct cs47l63_port *port)
{
	const struct cs47l63_data *data = dev->data;
	int ret;

	ret = cs47l63_bus_update_reg(
		dev, CS47L63_INPUT1_CONTROL1, CS47L63_IN1_OSR_MASK | CS47L63_IN1_MODE,
		((uint32_t)CS47L63_IN1_OSR_3M072 << CS47L63_IN1_OSR_SHIFT) | CS47L63_IN1_MODE);
	if (ret < 0) {
		return ret;
	}

	ret = write_in_level(dev, CS47L63_INPUT_PDM, data->in_muted);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL, CS47L63_IN1_EN_BOTH,
				     CS47L63_IN1_EN_BOTH);
	if (ret < 0) {
		return ret;
	}

	return write_mixers(dev, port, true, port->in_src1, port->in_src2);
}

/* The 32 kHz clock goes on first: MICBIAS short-circuit detection runs from it
 * (DS1249F2 section 4.10.4).
 */
static int pdm_start(const struct device *dev, const struct cs47l63_port *port)
{
	int ret;

	ret = cs47l63_bus_update_reg(dev, CS47L63_CLOCK32K,
				     CS47L63_CLK_32K_EN | CS47L63_CLK_32K_SRC_MASK,
				     CS47L63_CLK_32K_EN | CS47L63_CLK_32K_SRC_SYSCLK);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_MICBIAS_CTRL5, CS47L63_MICB1B_ON,
				     CS47L63_MICB1B_ON);
	if (ret == 0) {
		ret = pdm_start_path(dev, port);
	}
	if (ret < 0) {
		int err = micbias_off(dev);

		keep_first_error(&err, cs47l63_bus_update_reg(dev, CS47L63_CLOCK32K,
							      CS47L63_CLK_32K_EN, 0U));
		if (err < 0) {
			LOG_ERR("PDM start cleanup failed (%d)", err);
		}
	}

	return ret;
}

/* The front end comes down with the last port capturing from it; every step runs despite
 * a bus error.
 */
static int port_in_stop(const struct device *dev, struct cs47l63_port *port)
{
	struct cs47l63_chip *chip = chip_of(dev);
	bool last;
	int err = 0;

	port->in_started = false;
	chip->in_users[port->in_terminal] &= ~port_bit(port);
	last = chip->in_users[port->in_terminal] == 0U;

	keep_first_error(&err, write_tx_slots(dev, port, false));

	if (last) {
		if (port->in_terminal == CS47L63_INPUT_PDM) {
			keep_first_error(&err, write_in_level(dev, CS47L63_INPUT_PDM, true));
			keep_first_error(&err, cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL,
								      CS47L63_IN1_EN_BOTH, 0U));
			keep_first_error(&err, micbias_off(dev));
		} else {
			keep_first_error(&err, write_in_level(dev, CS47L63_INPUT_LINE, true));
			keep_first_error(&err, cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL,
								      CS47L63_IN2_EN_BOTH, 0U));
		}
	}

	keep_first_error(&err, write_mixers(dev, port, true, CS47L63_MIXER_SRC_NONE,
					    CS47L63_MIXER_SRC_NONE));

	return err;
}

static int port_in_start(const struct device *dev, struct cs47l63_port *port)
{
	struct cs47l63_chip *chip = chip_of(dev);
	const uint32_t bit = port_bit(port);
	int ret;

	if ((chip->in_users[port->in_terminal] & ~bit) != 0U) {
		/* The other port already runs this front end. */
		ret = write_mixers(dev, port, true, port->in_src1, port->in_src2);
	} else if (port->in_terminal == CS47L63_INPUT_PDM) {
		ret = pdm_start(dev, port);
	} else {
		ret = line_start(dev, port);
	}
	if (ret == 0) {
		ret = write_tx_slots(dev, port, true);
	}
	if (ret < 0) {
		const int err = port_in_stop(dev, port);

		if (err < 0) {
			LOG_ERR("Input start cleanup failed (%d)", err);
		}
		return ret;
	}

	chip->in_users[port->in_terminal] |= bit;
	port->in_started = true;

	return 0;
}

static int port_route_input(const struct device *dev, struct cs47l63_port *port,
			    audio_channel_t channel, uint32_t input)
{
	enum cs47l63_input terminal;
	uint16_t left;
	uint16_t right;
	int ret = 0;

	if (input == (uint32_t)CS47L63_INPUT_LINE) {
		terminal = CS47L63_INPUT_LINE;
		left = CS47L63_MIXER_SRC_IN2L;
		right = CS47L63_MIXER_SRC_IN2R;
	} else if (input == (uint32_t)CS47L63_INPUT_PDM) {
		terminal = CS47L63_INPUT_PDM;
		left = CS47L63_MIXER_SRC_IN1L;
		right = CS47L63_MIXER_SRC_IN1R;
	} else {
		LOG_ERR("No input terminal %u on this part", input);
		return -ENOTSUP;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		/* A mono source goes to both slots of the stereo frame. */
		right = left;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		left = right;
		break;
	case AUDIO_CHANNEL_ALL:
		break;
	default:
		LOG_ERR("Channel %d is not routable on this part", channel);
		ret = -ENOTSUP;
		break;
	}
	if (ret < 0) {
		return ret;
	}

	if (port->in_started && (terminal != port->in_terminal)) {
		ret = port_in_stop(dev, port);
		if (ret < 0) {
			return ret;
		}

		port->in_terminal = terminal;
		port->in_src1 = left;
		port->in_src2 = right;

		return port_in_start(dev, port);
	}

	port->in_terminal = terminal;
	port->in_src1 = left;
	port->in_src2 = right;

	if (!port->in_started) {
		return 0;
	}

	return write_mixers(dev, port, true, port->in_src1, port->in_src2);
}

static int cs47l63_in_route_input(const struct device *dev, audio_channel_t channel,
				  uint32_t input)
{
	struct cs47l63_chip *chip = chip_of(dev);
	int ret;

	chip_lock(chip);
	ret = port_route_input(chip->dev, port_of(dev), channel, input);
	chip_unlock(chip);

	return ret;
}

static uint8_t db_to_code(uint8_t code_0db, int32_t vol_db, int32_t min_db, int32_t max_db)
{
	const int32_t db = CLAMP(vol_db, min_db, max_db);
	const int32_t code = (int32_t)code_0db + (db * CS47L63_VOL_CODES_PER_DB);
	const int32_t clamped = CLAMP(code, 0, (int32_t)UINT8_MAX);

	return (uint8_t)clamped;
}

static int cs47l63_in_set_volume(const struct device *dev, int vol)
{
	struct cs47l63_data *data = dev->data;

	data->in_vol_code = db_to_code(CS47L63_IN_VOL_0DB, vol, CS47L63_IN_VOLUME_MIN_DB,
				       CS47L63_IN_VOLUME_MAX_DB);

	if (!in_running(chip_of(dev))) {
		return 0;
	}

	return write_captured_levels(dev, data->in_muted);
}

static int cs47l63_in_set_mute(const struct device *dev, bool mute)
{
	struct cs47l63_data *data = dev->data;

	data->in_muted = mute;

	if (!in_running(chip_of(dev))) {
		return 0;
	}

	return write_captured_levels(dev, mute);
}

/* OUT1L_EN starts a pop-suppressed enable sequence (DS1249F2 section 4.9). */

#define CS47L63_OUT_POLL_MS  2U
#define CS47L63_OUT_POLL_MAX 50U

static int write_volume_reg(const struct device *dev, uint8_t code, bool mute)
{
	uint32_t val = CS47L63_OUT_VU | (uint32_t)code;

	if (mute) {
		val |= CS47L63_OUT1L_MUTE;
	}

	return cs47l63_bus_write_reg(dev, CS47L63_OUT1L_VOLUME_1, val);
}

static int apply_cached_volume(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;

	if (!chip_of(dev)->out_running) {
		return 0;
	}

	return write_volume_reg(dev, data->out_vol_code, data->out_muted);
}

static int cs47l63_out_init(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->out_muted = false;
	data->out_vol_code = CS47L63_OUT1L_VOL_0DB;

	ret = cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0U);
	if (ret < 0) {
		return ret;
	}

	return write_volume_reg(dev, data->out_vol_code, true);
}

static int port_route_output(const struct device *dev, struct cs47l63_port *port,
			     audio_channel_t channel, uint32_t output)
{
	const uint16_t rx1 = port_rx_src(port, CS47L63_MIXER_SRC_ASP1RX1);
	const uint16_t rx2 = port_rx_src(port, CS47L63_MIXER_SRC_ASP1RX2);
	uint16_t src1;
	uint16_t src2;
	int ret = 0;

	if (output != (uint32_t)CS47L63_OUTPUT_HP) {
		LOG_ERR("This part has only the headphone terminal");
		return -ENOTSUP;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		src1 = rx1;
		src2 = CS47L63_MIXER_SRC_NONE;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		src1 = rx2;
		src2 = CS47L63_MIXER_SRC_NONE;
		break;
	case AUDIO_CHANNEL_ALL:
		src1 = rx1;
		src2 = rx2;
		break;
	default:
		LOG_ERR("Channel %d is not routable on this part", channel);
		ret = -ENOTSUP;
		break;
	}
	if (ret < 0) {
		return ret;
	}

	port->src1 = src1;
	port->src2 = src2;

	if (!port->out_started && (chip_of(dev)->out_started > 0U)) {
		/* Written at this port's start; writing it now would play it through the
		 * other port's output.
		 */
		return 0;
	}

	return write_mixers(dev, port, false, src1, src2);
}

static int cs47l63_out_route_output(const struct device *dev, audio_channel_t channel,
				    uint32_t output)
{
	struct cs47l63_chip *chip = chip_of(dev);
	int ret;

	chip_lock(chip);
	ret = port_route_output(chip->dev, port_of(dev), channel, output);
	chip_unlock(chip);

	return ret;
}

static int cs47l63_out_start(const struct device *dev)
{
	const struct device *bus = chip_of(dev)->dev;
	const struct cs47l63_port *port = port_of(dev);
	int ret;

	ret = write_mixers(bus, port, false, port->src1, port->src2);
	if (ret < 0) {
		return ret;
	}

	return cs47l63_bus_update_reg(bus, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN,
				      CS47L63_OUT1L_EN);
}

static int cs47l63_out_confirm_start(const struct device *dev, bool *up)
{
	int ret;

	ret = cs47l63_bus_poll_reg(dev, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS,
				   CS47L63_OUT1L_EN_STS, CS47L63_OUT_POLL_MS, CS47L63_OUT_POLL_MAX,
				   up);
	if (ret < 0) {
		return ret;
	}
	if (!*up) {
		LOG_ERR("Headphone amplifier never reported enabled");
		return 0;
	}

	chip_of(dev)->out_running = true;

	return apply_cached_volume(dev);
}

static int cs47l63_out_stop(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	bool locked;
	bool reached;
	int ret;
	int err;

	/* Muted first, so the stage is silent while it goes down. */
	ret = write_volume_reg(dev, data->out_vol_code, true);

	chip_of(dev)->out_running = false;

	err = cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0U);
	keep_first_error(&ret, err);
	if (err < 0) {
		return ret;
	}

	/* Without FLL1 lock nothing clocks OUT1L_EN_STS, so the wait is skipped. */
	err = cs47l63_clock_locked(dev, &locked);
	if (err < 0) {
		keep_first_error(&ret, err);
		return ret;
	}

	if (!locked) {
		return ret;
	}

	err = cs47l63_bus_poll_reg(dev, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS, 0U,
				   CS47L63_OUT_POLL_MS, CS47L63_OUT_POLL_MAX, &reached);
	if ((err == 0) && !reached) {
		LOG_ERR("Headphone amplifier never reported disabled");
		err = -ETIMEDOUT;
	}
	keep_first_error(&ret, err);

	return ret;
}

static int cs47l63_out_set_volume(const struct device *dev, int vol)
{
	struct cs47l63_data *data = dev->data;

	data->out_vol_code = db_to_code(CS47L63_OUT1L_VOL_0DB, vol, CS47L63_OUT_VOLUME_MIN_DB,
					CS47L63_OUT_VOLUME_MAX_DB);

	return apply_cached_volume(dev);
}

static int cs47l63_out_set_mute(const struct device *dev, bool mute)
{
	struct cs47l63_data *data = dev->data;

	data->out_muted = mute;

	return apply_cached_volume(dev);
}

/* A stop, or a stop and a new start, may come too late to cancel a running
 * check: either ends it.
 */
static bool start_check_current(struct cs47l63_chip *chip)
{
	const uint32_t busy = (uint32_t)k_work_delayable_busy_get(&chip->start_check);

	return (chip->out_started > 0U) && ((busy & (uint32_t)K_WORK_DELAYED) == 0U);
}

/* Deferred because FLL1's reference, MCLK1, starts only with the caller's I2S
 * transfer, after start_output returns.
 */
static void check_out_start(struct cs47l63_chip *chip)
{
	const struct device *dev = chip->dev;
	bool locked;
	bool up;
	int ret;

	if (!start_check_current(chip)) {
		return;
	}

	ret = cs47l63_clock_locked(dev, &locked);
	if (ret < 0) {
		LOG_ERR("Failed to read FLL1 lock state: %d", ret);
	}
	if ((ret == 0) && !locked) {
		LOG_ERR("FLL1 still unlocked %u ms after output start - no MCLK1 reference; "
			"the output stage is running on a clock that never started",
			CS47L63_FLL_LOCK_SETTLE_MS);
		cs47l63_fault_raise(dev, CS47L63_ERROR_CLOCK);
		if (!start_check_current(chip)) {
			return;
		}
	}

	/* Attempted even without lock: OUT1L_EN_STS shows the stage state directly. */
	ret = cs47l63_out_confirm_start(dev, &up);
	if (ret < 0) {
		LOG_ERR("Failed to confirm the output start: %d", ret);
	}
	if ((ret == 0) && !up) {
		cs47l63_fault_raise(dev, CS47L63_ERROR_OUTPUT);
	}
}

static void start_check_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct cs47l63_chip *chip = CONTAINER_OF(dwork, struct cs47l63_chip, start_check);

	chip_lock(chip);
	check_out_start(chip);
	chip_unlock(chip);
}

static int codec_initialize(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;
	int ret;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	cfg->chip->dev = dev;
	ret = k_mutex_init(&cfg->chip->lock);
	if (ret < 0) {
		return ret;
	}
	cfg->chip->ports[cfg->port->asp - 1U] = cfg->port;
	k_work_init_delayable(&cfg->chip->start_check, start_check_work);
	k_work_init(&cfg->chip->irq_work, cs47l63_irq_work);

	if (cfg->irq_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
			LOG_ERR("IRQ GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
		if (ret < 0) {
			return ret;
		}

		gpio_init_callback(&cfg->chip->irq_cb, cs47l63_irq_isr, BIT(cfg->irq_gpio.pin));
		ret = gpio_add_callback_dt(&cfg->irq_gpio, &cfg->chip->irq_cb);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

__maybe_unused static int child_initialize(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;

	if (!device_is_ready(cfg->chip->dev)) {
		LOG_ERR("Parent codec not ready");
		return -ENODEV;
	}

	cfg->chip->ports[cfg->port->asp - 1U] = cfg->port;

	return 0;
}

/* A failed configure leaves the amplifier disabled. */
static int configure_failed(const struct device *dev, int ret)
{
	const int err = cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0U);

	if (err < 0) {
		LOG_ERR("Amplifier disable after a failed configure failed (%d)", err);
	}

	return ret;
}

static int port_out_stop(struct cs47l63_chip *chip, struct cs47l63_port *port)
{
	if (port->out_started && (chip->out_started > 0U)) {
		chip->out_started--;
	}
	port->out_started = false;

	if (chip->out_started > 0U) {
		return write_mixers(chip->dev, port, false, CS47L63_MIXER_SRC_NONE,
				    CS47L63_MIXER_SRC_NONE);
	}

	(void)k_work_cancel_delayable(&chip->start_check);

	return cs47l63_out_stop(chip->dev);
}

/** Configures a port while another runs the part: its own registers only, no reset,
 * clock or rate write.
 */
static int port_join(const struct device *dev, struct cs47l63_chip *chip, struct cs47l63_port *port,
		     const struct cs47l63_dai_solution *sol)
{
	int ret;

	if (port->out_started) {
		ret = port_out_stop(chip, port);
		if (ret < 0) {
			return ret;
		}
	}

	if (port->in_started) {
		ret = port_in_stop(dev, port);
		if (ret < 0) {
			return ret;
		}
	}

	ret = port_dai_apply(dev, port, sol, false);
	if (ret < 0) {
		return ret;
	}

	port_reset(port);

	ret = write_mixers(dev, port, false, CS47L63_MIXER_SRC_NONE, CS47L63_MIXER_SRC_NONE);
	if (ret < 0) {
		return ret;
	}

	return write_mixers(dev, port, true, CS47L63_MIXER_SRC_NONE, CS47L63_MIXER_SRC_NONE);
}

static int port_configure(struct cs47l63_chip *chip, struct cs47l63_port *port,
			  const struct audio_codec_cfg *cfg)
{
	const struct device *dev = chip->dev;
	const uint32_t bit = port_bit(port);
	const bool shared = (chip->configured & ~bit) != 0U;
	struct cs47l63_fll_solution clock_sol;
	struct cs47l63_dai_solution dai_sol;
	int ret = 0;

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
	case AUDIO_ROUTE_CAPTURE:
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		break;
	default:
		LOG_ERR("Unsupported route %d", cfg->dai_route);
		ret = -ENOTSUP;
		break;
	}
	if (ret < 0) {
		return ret;
	}

	/* Every port runs from the one FLL1 and the one SAMPLE_RATE1 slot. */
	if (shared && ((cfg->mclk_freq != chip->mclk_freq) ||
		       (cfg->dai_cfg.i2s.frame_clk_freq != chip->rate))) {
		LOG_ERR("Another port runs at MCLK %u Hz, %u Hz", chip->mclk_freq, chip->rate);
		return -ENOTSUP;
	}

	/* Both solvers are pure, so a refused request leaves the part untouched. */
	ret = cs47l63_clock_solve(cfg->mclk_freq, &clock_sol);
	if (ret < 0) {
		LOG_ERR("No FLL1 solution for MCLK %u Hz: %d", cfg->mclk_freq, ret);
		return ret;
	}

	ret = cs47l63_dai_solve(cfg->dai_type, &cfg->dai_cfg.i2s, &dai_sol);
	if (ret < 0) {
		return ret;
	}

	if (shared) {
		/* Stays unstartable unless the join completes. */
		port->route = AUDIO_ROUTE_BYPASS;
		chip->configured &= ~bit;

		ret = port_join(dev, chip, port, &dai_sol);
		if (ret == 0) {
			port->route = cfg->dai_route;
			port->tx_enables = dai_sol.tx_enables;
			chip->configured |= bit;
		}

		return ret;
	}

	/* A start check armed before this configure would otherwise run against
	 * the new configuration and unmute an output nobody has started.
	 */
	(void)k_work_cancel_delayable(&chip->start_check);

	cs47l63_irq_disarm(chip);

	ret = cs47l63_boot_bringup(dev);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_clock_apply(dev, &clock_sol);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	ret = port_dai_apply(dev, port, &dai_sol, true);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	ret = cs47l63_out_init(dev);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	if (cfg->dai_route != AUDIO_ROUTE_CAPTURE) {
		ret = write_mixers(dev, port, false, port->src1, port->src2);
		if (ret < 0) {
			return configure_failed(dev, ret);
		}
	}

	ret = cs47l63_in_init(dev, port);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	ret = cs47l63_fault_clear(dev);
	if (ret == 0) {
		ret = cs47l63_irq_arm(dev);
	}
	if (ret == 0) {
		port->route = cfg->dai_route;
		port->tx_enables = dai_sol.tx_enables;
		chip->configured = bit;
		chip->mclk_freq = cfg->mclk_freq;
		chip->rate = cfg->dai_cfg.i2s.frame_clk_freq;
	}

	return ret;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct cs47l63_chip *chip = chip_of(dev);
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}

	chip_lock(chip);
	ret = port_configure(chip, port_of(dev), cfg);
	chip_unlock(chip);

	return ret;
}

static int port_out_start(const struct device *dev)
{
	struct cs47l63_chip *chip = chip_of(dev);
	struct cs47l63_port *port = port_of(dev);
	const bool amp_up = port->out_started ? (chip->out_started > 1U) : (chip->out_started > 0U);
	int ret;

	for (size_t i = 0U; i < ARRAY_SIZE(chip->ports); i++) {
		const struct cs47l63_port *other = chip->ports[i];

		if ((other != NULL) && (other != port) && !other->out_started) {
			ret = write_mixers(chip->dev, other, false, CS47L63_MIXER_SRC_NONE,
					   CS47L63_MIXER_SRC_NONE);
			if (ret < 0) {
				return ret;
			}
		}
	}

	/* The amplifier and its start check belong to the first port started. */
	if (amp_up) {
		ret = write_mixers(chip->dev, port, false, port->src1, port->src2);
		if ((ret < 0) && !port->out_started) {
			const int err = write_mixers(chip->dev, port, false, CS47L63_MIXER_SRC_NONE,
						     CS47L63_MIXER_SRC_NONE);

			if (err < 0) {
				LOG_ERR("Output start cleanup failed (%d)", err);
			}
		}
	} else {
		ret = cs47l63_out_start(dev);
	}
	if (ret < 0) {
		return ret;
	}

	if (!port->out_started) {
		port->out_started = true;
		chip->out_started++;
	}

	if (amp_up) {
		return 0;
	}

	ret = k_work_reschedule(&chip->start_check, K_MSEC(CS47L63_FLL_LOCK_SETTLE_MS));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void codec_start_output(const struct device *dev)
{
	struct cs47l63_chip *chip = chip_of(dev);
	const struct cs47l63_port *port = port_of(dev);
	int ret;

	chip_lock(chip);

	if ((port->route != AUDIO_ROUTE_PLAYBACK) &&
	    (port->route != AUDIO_ROUTE_PLAYBACK_CAPTURE)) {
		LOG_ERR("No playback on route %d", port->route);
	} else {
		ret = port_out_start(dev);
		if (ret < 0) {
			LOG_ERR("Failed to start output: %d", ret);
		}

		ret = cs47l63_fault_check(chip->dev);
		if (ret < 0) {
			LOG_ERR("Failed to poll faults: %d", ret);
		}
	}

	chip_unlock(chip);
}

static void codec_stop_output(const struct device *dev)
{
	struct cs47l63_chip *chip = chip_of(dev);
	int ret;

	chip_lock(chip);
	ret = port_out_stop(chip, port_of(dev));
	chip_unlock(chip);

	if (ret < 0) {
		LOG_ERR("Failed to stop output: %d", ret);
	}
}

static void poll_faults_or_log(const struct device *dev)
{
	const int ret = cs47l63_fault_check(dev);

	if (ret < 0) {
		LOG_ERR("Failed to poll faults: %d", ret);
	}
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	struct cs47l63_chip *chip = chip_of(dev);
	int ret;

	if (channel != AUDIO_CHANNEL_ALL) {
		LOG_ERR("Only AUDIO_CHANNEL_ALL is addressable on this part");
		return -EINVAL;
	}

	chip_lock(chip);

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		ret = cs47l63_out_set_volume(dev, val.vol);
		poll_faults_or_log(dev);
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		ret = cs47l63_out_set_mute(dev, val.mute);
		poll_faults_or_log(dev);
		break;
	case AUDIO_PROPERTY_INPUT_VOLUME:
		ret = cs47l63_in_set_volume(dev, val.vol);
		break;
	case AUDIO_PROPERTY_INPUT_MUTE:
		ret = cs47l63_in_set_mute(dev, val.mute);
		break;
	default:
		LOG_ERR("Property %d is not supported", property);
		ret = -ENOTSUP;
		break;
	}

	chip_unlock(chip);

	return ret;
}

/* Nothing to commit: every property is latched by its update strobe as it is written. */
static int codec_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	struct cs47l63_chip *chip = chip_of(dev);
	struct cs47l63_port *port = port_of(dev);
	bool in_was_started;
	int ret = 0;

	chip_lock(chip);
	in_was_started = port->in_started;

	if ((port->route == AUDIO_ROUTE_BYPASS) ||
	    (((dir & AUDIO_DAI_DIR_RX) != 0U) && (port->route == AUDIO_ROUTE_PLAYBACK)) ||
	    (((dir & AUDIO_DAI_DIR_TX) != 0U) && (port->route == AUDIO_ROUTE_CAPTURE))) {
		LOG_ERR("Direction 0x%x is outside route %d", (uint32_t)dir, port->route);
		ret = -EINVAL;
	}

	if ((ret == 0) && ((dir & AUDIO_DAI_DIR_RX) != 0U)) {
		ret = port_in_start(chip->dev, port);
	}

	if ((ret == 0) && ((dir & AUDIO_DAI_DIR_TX) != 0U)) {
		ret = port_out_start(dev);
		if ((ret < 0) && ((dir & AUDIO_DAI_DIR_RX) != 0U) && !in_was_started) {
			const int err = port_in_stop(chip->dev, port);

			if (err < 0) {
				LOG_ERR("Input rollback failed (%d)", err);
			}
		}

		poll_faults_or_log(chip->dev);
	}
	chip_unlock(chip);

	return ret;
}

static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct cs47l63_chip *chip = chip_of(dev);
	int ret = 0;

	chip_lock(chip);

	if ((dir & AUDIO_DAI_DIR_RX) != 0U) {
		ret = port_in_stop(chip->dev, port_of(dev));
	}

	if ((dir & AUDIO_DAI_DIR_TX) != 0U) {
		keep_first_error(&ret, port_out_stop(chip, port_of(dev)));
	}
	chip_unlock(chip);

	return ret;
}

static DEVICE_API(audio_codec, codec_driver_api) = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
	.clear_errors = cs47l63_fault_clear,
	.register_error_callback = cs47l63_fault_register_callback,
	.route_input = cs47l63_in_route_input,
	.route_output = cs47l63_out_route_output,
	.start = codec_start,
	.stop = codec_stop,
};

static int child_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(property);
	ARG_UNUSED(channel);
	ARG_UNUSED(val);

	return -ENOTSUP;
}

static int child_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static DEVICE_API(audio_codec, child_driver_api) = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = child_set_property,
	.apply_properties = child_apply_properties,
	.clear_errors = cs47l63_fault_clear,
	.register_error_callback = cs47l63_fault_register_callback,
	.route_input = cs47l63_in_route_input,
	.route_output = cs47l63_out_route_output,
	.start = codec_start,
	.stop = codec_stop,
};

#define CS47L63_GPIO_DEFAULT_BIT(node_id, prop, idx)                                               \
	((((uint32_t)DT_PROP_BY_IDX(node_id, prop, idx) & CS47L63_GP_CTRL1_RSVD) == 0U) ? BIT(idx) \
											: 0U) |

/* Bit n - 1 set for every GPIOn that cirrus,gpio-defaults writes. */
#define CS47L63_GPIO_DEFAULTS_MASK(inst)                                                           \
	(COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, cirrus_gpio_defaults),                            \
		     (DT_INST_FOREACH_PROP_ELEM(inst, cirrus_gpio_defaults,                        \
						CS47L63_GPIO_DEFAULT_BIT)),                        \
		     ()) 0U)

#define CS47L63_ASP_PAD_COUNT 4U

#define CS47L63_ASP_PADS_MASK(asp)                                                                 \
	(BIT_MASK(CS47L63_ASP_PAD_COUNT) << (CS47L63_ASP_PAD_COUNT * ((uint32_t)(asp) - 1U)))

#define CS47L63_CHILD_DEFINE(node_id, inst)                                                        \
	BUILD_ASSERT(DT_PROP(node_id, cirrus_asp) != DT_INST_PROP(inst, cirrus_asp),               \
		     "cs47l63: " DT_NODE_PATH(node_id) " is on its parent's cirrus,asp");          \
	BUILD_ASSERT((CS47L63_GPIO_DEFAULTS_MASK(inst) &                                           \
		      CS47L63_ASP_PADS_MASK(DT_PROP(node_id, cirrus_asp))) == 0U,                  \
		     "cs47l63: cirrus,gpio-defaults sets an ASP pad of " DT_NODE_PATH(node_id));   \
	static struct cs47l63_port cs47l63_port_##node_id = {                                      \
		.asp = DT_PROP(node_id, cirrus_asp),                                               \
		.out_mix_input = CS47L63_OUT1L_MIX_INPUT_CHILD,                                    \
	};                                                                                         \
	static const struct cs47l63_config cs47l63_config_##node_id = {                            \
		.chip = &cs47l63_chip_##inst,                                                      \
		.port = &cs47l63_port_##node_id,                                                   \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, child_initialize, NULL, NULL, &cs47l63_config_##node_id,         \
			 POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &child_driver_api);

#define CS47L63_DEFINE(inst)                                                                       \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, reset_gpios),                                     \
		     "cs47l63: reset-gpios is required, boot starts with a hardware reset");       \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(inst) <= 1,                                     \
		     "cs47l63: " DT_NODE_PATH(DT_DRV_INST(inst)) " has more than one child");      \
	BUILD_ASSERT((uint32_t)DT_INST_PROP_LEN_OR(inst, cirrus_gpio_defaults, 0) <=               \
			     CS47L63_GPIO_COUNT,                                                   \
		     "cs47l63: cirrus,gpio-defaults has more entries than the part has GPIOs");    \
	BUILD_ASSERT((CS47L63_GPIO_DEFAULTS_MASK(inst) &                                           \
		      CS47L63_ASP_PADS_MASK(DT_INST_PROP(inst, cirrus_asp))) == 0U,                \
		     "cs47l63: cirrus,gpio-defaults sets an ASP pad of the parent");               \
	static const uint32_t cs47l63_gpio_defaults_##inst[] =                                     \
		DT_INST_PROP_OR(inst, cirrus_gpio_defaults, {0U});                                 \
	static struct cs47l63_data cs47l63_data_##inst;                                            \
	static struct cs47l63_chip cs47l63_chip_##inst;                                            \
	static struct cs47l63_port cs47l63_port_##inst = {                                         \
		.asp = DT_INST_PROP(inst, cirrus_asp),                                             \
		.out_mix_input = CS47L63_OUT1L_MIX_INPUT_PARENT,                                   \
	};                                                                                         \
	static const struct cs47l63_config cs47l63_config_##inst = {                               \
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(CS47L63_SPI_WORD_BITS) |            \
							  SPI_TRANSFER_MSB),                       \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		.gpio_defaults = cs47l63_gpio_defaults_##inst,                                     \
		.gpio_defaults_len = DT_INST_PROP_LEN_OR(inst, cirrus_gpio_defaults, 0),           \
		.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),                        \
		.chip = &cs47l63_chip_##inst,                                                      \
		.port = &cs47l63_port_##inst,                                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, codec_initialize, NULL, &cs47l63_data_##inst,                  \
			      &cs47l63_config_##inst, POST_KERNEL,                                 \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);                \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(inst, CS47L63_CHILD_DEFINE, inst)

DT_INST_FOREACH_STATUS_OKAY(CS47L63_DEFINE)
