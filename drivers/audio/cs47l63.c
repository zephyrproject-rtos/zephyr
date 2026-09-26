/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63.c
 * @brief CS47L63 audio codec driver (SPI, audio_codec API)
 *
 * The part's DSP cores and loadable firmware are not supported; the DSP-memory
 * patch the vendor driver applies alongside the trim block is therefore not
 * applied here.
 */

#define DT_DRV_COMPAT cirrus_cs47l63

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "cs47l63.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cs47l63);

/* 32-bit register access over SPI. */

/** Marks the address word of a read transaction. */
#define CS47L63_READ_FLAG 0x80000000U

/** Address, padding and data are each one 32-bit word on the wire. */
#define CS47L63_WORD_BYTES 4

/**
 * @brief Wire layout of every register transaction: address, padding, data.
 *
 * The padding word is not optional and it is not read-only: the vendor BSP
 * declares `spi_pad_len = 4` (modules/hal/cirrus-logic/cs47l63/bsp/bsp_cs47l63.c)
 * and its transport clocks that many zero bytes between the address and the
 * data on writes exactly as it does on reads (common/platform_bsp/eestm32int/
 * platform_bsp.c, bsp_spi_write()). A write that omits it is not a corrupt
 * write - the part takes the data word for the padding, sees the frame end
 * before any data arrives and discards the whole transaction, silently and
 * with the SPI transfer itself reporting success.
 */
#define CS47L63_FRAME_BYTES (3 * CS47L63_WORD_BYTES)

/** Byte offsets of the three words in a transaction. */
#define CS47L63_ADDR_OFFS 0
#define CS47L63_PAD_OFFS  CS47L63_WORD_BYTES
#define CS47L63_DATA_OFFS (2 * CS47L63_WORD_BYTES)

bool cs47l63_bus_is_ready(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;

	return spi_is_ready_dt(&cfg->bus);
}

int cs47l63_bus_write_reg(const struct device *dev, uint32_t addr, uint32_t val)
{
	const struct cs47l63_config *cfg = dev->config;
	uint8_t tx[CS47L63_FRAME_BYTES] = {0};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	int ret;

	sys_put_be32(addr, &tx[CS47L63_ADDR_OFFS]);
	/* tx[CS47L63_PAD_OFFS] stays zero - the padding word. */
	sys_put_be32(val, &tx[CS47L63_DATA_OFFS]);

	ret = spi_write_dt(&cfg->bus, &tx_set);
	if (ret < 0) {
		LOG_ERR("Failed to write reg 0x%05x: %d", addr, ret);
	}

	return ret;
}

int cs47l63_bus_read_reg(const struct device *dev, uint32_t addr, uint32_t *val)
{
	const struct cs47l63_config *cfg = dev->config;
	uint8_t tx[2 * CS47L63_WORD_BYTES] = {0};
	uint8_t rx[CS47L63_WORD_BYTES];
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	/* Address then padding on the way out; the first buffer has no
	 * destination, so both are clocked out and dropped rather than landing
	 * in front of the data.
	 */
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = sizeof(tx)},
		{.buf = rx, .len = sizeof(rx)},
	};
	const struct spi_buf_set rx_set = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};
	int ret;

	if (val == NULL) {
		return -EINVAL;
	}

	/* The read marker is OR'd in, so an address that already has its top
	 * bit set is not corrupted - it is simply not an addressable register.
	 */
	sys_put_be32(addr | CS47L63_READ_FLAG, &tx[CS47L63_ADDR_OFFS]);

	ret = spi_transceive_dt(&cfg->bus, &tx_set, &rx_set);
	if (ret < 0) {
		LOG_ERR("Failed to read reg 0x%05x: %d", addr, ret);
		return ret;
	}

	*val = sys_get_be32(rx);

	return 0;
}

int cs47l63_bus_update_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t val)
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

int cs47l63_bus_poll_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t expected,
			 uint32_t interval_ms, uint32_t max_polls)
{
	uint32_t val;
	int ret;

	for (uint32_t i = 0; i < max_polls; i++) {
		ret = cs47l63_bus_read_reg(dev, addr, &val);
		if (ret < 0) {
			return ret;
		}

		if ((val & mask) == expected) {
			return 0;
		}

		k_msleep(interval_ms);
	}

	LOG_ERR("Reg 0x%05x never reached 0x%08x under mask 0x%08x", addr, expected, mask);

	return -ETIMEDOUT;
}

/* Reset, boot-done wait, identification and trim.
 *
 * The reset timing, the boot-done poll bound and the trim block are taken from
 * the Apache-2.0 vendor driver (modules/hal/cirrus-logic/cs47l63/cs47l63.c,
 * cs47l63_reset() and cs47l63_patch()).
 */

/** Settling time either side of the supply coming up with reset held low. */
#define CS47L63_RESET_SETTLE_MS 2

/** Boot-done poll: read every 10 ms, give up after 20 reads. */
#define CS47L63_BOOT_POLL_MS  10
#define CS47L63_BOOT_POLL_MAX 20

/** OTP variant whose trim block this driver knows how to write. */
#define CS47L63_OTPID_TRIMMED 0x8

/**
 * @brief The trim block for OTP variant 8.
 *
 * A fixed sequence of register writes with no meaning derivable from the field
 * names, carried from the vendor source as a unit. It sets the headphone
 * output interface and the over-current detector to their validated operating
 * point, so skipping it or "simplifying" it leaves both outside the envelope
 * the part was characterised in. It is not an optimisation and it is not to be
 * reasoned about or trimmed.
 *
 * The block sits behind the register-region lock, which the caller opens and
 * closes around it.
 */
static const struct {
	uint32_t addr;
	uint32_t val;
} k_trim_otpid_8[] = {
	{CS47L63_DAC_IF_CONTROL_1, 0x1DB10000},  {CS47L63_DAC_IF_TEST_1, 0x700249B8},
	{CS47L63_HP_OCD_CTRL1, 0x00010000},      {CS47L63_HP_OCD_TEST1, 0x000005FF},
	{CS47L63_MICBIAS_TST_CTRL1, 0x04150415}, {CS47L63_MICBIAS_TST_CTRL4, 0x00000415},
};

/** The two-write unlock, then lock, code pairs for the trim region. */
static const uint32_t k_key_unlock[] = {CS47L63_KEY_UNLOCK_CODE0, CS47L63_KEY_UNLOCK_CODE1};
static const uint32_t k_key_lock[] = {CS47L63_KEY_LOCK_CODE0, CS47L63_KEY_LOCK_CODE1};

static int write_key_pair(const struct device *dev, const uint32_t codes[2])
{
	int ret;

	for (size_t i = 0; i < 2; i++) {
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

/**
 * @brief Drive the reset line through the vendor's sequence.
 *
 * Reset low, settle, settle again (the vendor brings DCVDD up in this window;
 * on this board that rail is not under software control and is already on),
 * then release. The delays are the vendor's own, not rounded to something
 * convenient.
 */
static int hw_reset(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;
	int ret;

	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	k_msleep(CS47L63_RESET_SETTLE_MS);
	k_msleep(CS47L63_RESET_SETTLE_MS);

	return gpio_pin_set_dt(&cfg->reset_gpio, 0);
}

/**
 * @brief Read and sanity-check DEVID and REVID.
 *
 * The Apache-2.0 vendor sources record the device-ID register but never state
 * the value the part answers with, so this checks for the two readings a
 * mis-wired or mis-clocked bus produces - all zeros and all ones - rather than
 * comparing against a number nobody in this tree can attest. Both IDs are
 * logged so a bench session can read the real value off the board and tighten
 * this into an equality check.
 */
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

	if (devid == 0 || devid == CS47L63_DEVID_MASK) {
		LOG_ERR("Implausible device ID 0x%06x - check wiring and SPI mode", devid);
		return -ENODEV;
	}

	ret = cs47l63_bus_read_reg(dev, CS47L63_REVID, &revid);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("CS47L63 devid 0x%06x, rev %u.%u", devid, (revid & CS47L63_REVID_AREVID_MASK) >> 4,
		revid & CS47L63_REVID_MTLREVID_MASK);

	return 0;
}

/**
 * @brief Apply the trim block if this part's OTP variant needs it.
 *
 * A bus error here aborts init rather than continuing into a part whose
 * headphone driver and over-current detector are half-trimmed.
 */
static int apply_trim(const struct device *dev)
{
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

	for (size_t i = 0; ret >= 0 && i < ARRAY_SIZE(k_trim_otpid_8); i++) {
		ret = cs47l63_bus_write_reg(dev, k_trim_otpid_8[i].addr, k_trim_otpid_8[i].val);
	}

	if (ret < 0) {
		/* Leave the region locked even on the way out. */
		(void)write_key_pair(dev, k_key_lock);
		return ret;
	}

	return write_key_pair(dev, k_key_lock);
}

int cs47l63_boot_bringup(const struct device *dev)
{
	int ret;

	ret = hw_reset(dev);
	if (ret < 0) {
		LOG_ERR("Reset GPIO failed: %d", ret);
		return ret;
	}

	/* Boot-done is an edge flag, so the first read comes after a delay
	 * rather than before it. Bounded by construction: it gives up with
	 * -ETIMEDOUT and never loops.
	 */
	k_msleep(CS47L63_BOOT_POLL_MS);
	ret = cs47l63_bus_poll_reg(dev, CS47L63_IRQ1_EINT_2, CS47L63_BOOT_DONE_EINT1,
				   CS47L63_BOOT_DONE_EINT1, CS47L63_BOOT_POLL_MS,
				   CS47L63_BOOT_POLL_MAX);
	if (ret < 0) {
		return ret;
	}

	ret = identify(dev);
	if (ret < 0) {
		return ret;
	}

	return apply_trim(dev);
}

/* FLL1 solver and apply path.
 *
 * The solver reproduces the FLLHJ term selection of the Apache-2.0 vendor
 * driver (modules/hal/cirrus-logic/cs47l63/cs47l63.c, cs47l63_fll_do_config()):
 * its thresholds, gain words, feedback dividers and N/theta/lambda arithmetic
 * are properties of the loop hardware, not of any one implementation. The
 * register order in cs47l63_clock_apply() is that source's
 * cs47l63_fll_apply_config() / cs47l63_fll_enable().
 */

/* Reference-frequency bands. Each picks a lock-detect threshold, a loop-gain
 * word and a starting feedback divider.
 */
#define CS47L63_FLL_LOW_THRESH 192000U
#define CS47L63_FLL_MID_THRESH 1152000U
#define CS47L63_FLL_MAX_THRESH 13000000U
#define CS47L63_FLL_LOW_GAINS  0x23F0
#define CS47L63_FLL_MID_GAINS  0x22F2
#define CS47L63_FLL_HIGH_GAINS 0x21F0

/* Below this the loop runs in its low-power integer mode. */
#define CS47L63_FLL_LP_INT_THRESH 100000U

/* N range, per mode. */
#define CS47L63_FLL_INT_MIN_N  1U
#define CS47L63_FLL_INT_MAX_N  1023U
#define CS47L63_FLL_FRAC_MIN_N 2U
#define CS47L63_FLL_FRAC_MAX_N 255U

/** Largest output the loop can produce. */
#define CS47L63_FLL_MAX_FOUT 50000000U

/** The reference divider is a shift of 0 to 3. */
#define CS47L63_FLL_REFDIV_MAX_SHIFT 3U

static uint32_t gcd_u32(uint32_t a, uint32_t b)
{
	while (b != 0) {
		uint32_t t = a % b;

		a = b;
		b = t;
	}

	return a;
}

/**
 * @brief The SYSCLK_FREQ code for an FLL output, or -ENOTSUP.
 *
 * An FLL that feeds SYSCLK is not free to run at any of the frequencies the
 * SYSCLK_FREQ field can name: DS1249F2 section 4.10.7 requires exactly
 * 49.152 MHz for the 48 kHz sample-rate family, or 45.1584 MHz for the
 * 44.1 kHz one. SYSCLK_FRAC is pinned to 0 throughout this driver, which
 * selects the first of those two, so one output frequency is legal here and
 * anything else has to be refused rather than programmed.
 */
static int sysclk_freq_code(uint32_t fout_hz, uint8_t *code)
{
	if (fout_hz != CS47L63_FLL_FOUT_HZ) {
		return -ENOTSUP;
	}

	*code = CS47L63_SYSCLK_FREQ_49M152;

	return 0;
}

int cs47l63_clock_solve(uint32_t fref_hz, uint32_t fout_hz, uint8_t refclk_src,
			struct cs47l63_fll_solution *out)
{
	uint32_t refdiv;
	uint32_t fref;
	uint32_t fbdiv;
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
	uint8_t freq_code;
	bool frac;
	int ret;

	if (out == NULL) {
		return -EINVAL;
	}
	if (fref_hz == 0 || fout_hz == 0) {
		return -EINVAL;
	}
	if (fout_hz > CS47L63_FLL_MAX_FOUT) {
		return -ENOTSUP;
	}

	/* SYSCLK is part of the solution, not something the caller is left to
	 * remember: an fout the SYSCLK mux cannot name is unreachable, and
	 * saying so here keeps a failed solve from writing anything.
	 */
	ret = sysclk_freq_code(fout_hz, &freq_code);
	if (ret < 0) {
		return ret;
	}

	/* Bring the reference into the loop's input range with the /1 /2 /4 /8
	 * predivider. A reference still above the maximum at /8 is out of
	 * range and is refused rather than approximated.
	 */
	for (refdiv = 0; refdiv <= CS47L63_FLL_REFDIV_MAX_SHIFT; refdiv++) {
		if ((fref_hz >> refdiv) <= CS47L63_FLL_MAX_THRESH) {
			break;
		}
	}
	refdiv = MIN(refdiv, CS47L63_FLL_REFDIV_MAX_SHIFT);
	fref = fref_hz >> refdiv;
	if (fref > CS47L63_FLL_MAX_THRESH) {
		return -ENOTSUP;
	}
	if (fref > fout_hz) {
		return -ENOTSUP;
	}

	frac = (fout_hz % fref) != 0;

	if (fref < CS47L63_FLL_LOW_THRESH) {
		lockdet_thr = 2;
		gains = CS47L63_FLL_LOW_GAINS;
		fbdiv = frac ? 256 : 4;
	} else if (fref < CS47L63_FLL_MID_THRESH) {
		lockdet_thr = 8;
		gains = CS47L63_FLL_MID_GAINS;
		fbdiv = frac ? 16 : 2;
	} else {
		lockdet_thr = 8;
		gains = CS47L63_FLL_HIGH_GAINS;
		fbdiv = 1;
	}

	if (frac) {
		/* A fractional ratio needs the high-performance loop. */
		hp = 0x3;
		min_n = CS47L63_FLL_FRAC_MIN_N;
		max_n = CS47L63_FLL_FRAC_MAX_N;
	} else {
		hp = (fref < CS47L63_FLL_LP_INT_THRESH) ? 0x0 : 0x1;
		min_n = CS47L63_FLL_INT_MIN_N;
		max_n = CS47L63_FLL_INT_MAX_N;
	}

	/* Walk the feedback divider until N lands inside the range the mode
	 * allows. Either bound running out means the request is unreachable.
	 */
	ratio = fout_hz / fref;
	while (ratio / fbdiv < min_n) {
		fbdiv /= 2;
		if (fbdiv < min_n) {
			return -ENOTSUP;
		}
	}
	while (frac && (ratio / fbdiv > max_n)) {
		fbdiv *= 2;
		if (fbdiv >= 1024) {
			return -ENOTSUP;
		}
	}

	/* N + theta/lambda is fout/(fref * fbdiv) in lowest terms. */
	divisor = gcd_u32(fout_hz, fbdiv * fref);
	num = fout_hz / divisor;
	lambda = (fref * fbdiv) / divisor;
	if (lambda == 0) {
		return -ENOTSUP;
	}
	n = num / lambda;
	theta = num % lambda;

	if (n < min_n || n > max_n) {
		return -ENOTSUP;
	}
	if (fbdiv < 1 || (frac && fbdiv >= 1024) || (!frac && fbdiv >= 256)) {
		return -ENOTSUP;
	}
	if (lambda > UINT16_MAX || theta > UINT16_MAX) {
		return -ENOTSUP;
	}

	out->refclk_src = refclk_src;
	out->refclk_div = (uint8_t)refdiv;
	out->lockdet_thr = (uint8_t)lockdet_thr;
	out->hp = (uint8_t)hp;
	out->n = (uint16_t)n;
	out->fb_div = (uint16_t)fbdiv;
	out->lambda = (uint16_t)lambda;
	out->theta = (uint16_t)theta;
	out->gains = (uint16_t)gains;
	out->sysclk_freq = freq_code;

	return 0;
}

int cs47l63_clock_apply(const struct device *dev, const struct cs47l63_fll_solution *sol)
{
	int ret;

	if (sol == NULL) {
		return -EINVAL;
	}

	/* Hold first. Every write below happens with the loop held, which is
	 * also the state a failure partway through leaves behind.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_HOLD,
				     CS47L63_FLL1_HOLD);
	if (ret < 0) {
		return ret;
	}

	/* LOCKDET and REFDET are not part of the vendor driver's FLL path,
	 * because its BSP hands cs47l63_syscfg_regs[] to cs47l63_boot() and that
	 * block writes FLL1_CONTROL2 = 0x28601177 - both detectors enabled -
	 * before any FLL call runs. This driver applies no such block, so the
	 * two bits are set here instead. Without LOCKDET the lock detector is
	 * off and FLL1_LOCK_STS1 cannot rise however well the loop is running,
	 * which is indistinguishable from a loop that never locked.
	 */
	ret = cs47l63_bus_update_reg(
		dev, CS47L63_FLL1_CONTROL2,
		CS47L63_FLL1_LOCKDET_THR_MASK | CS47L63_FLL1_LOCKDET | CS47L63_FLL1_PHASEDET |
			CS47L63_FLL1_REFDET | CS47L63_FLL1_REFCLK_DIV_MASK | CS47L63_FLL1_N_MASK,
		((uint32_t)sol->lockdet_thr << CS47L63_FLL1_LOCKDET_THR_SHIFT) |
			CS47L63_FLL1_LOCKDET | CS47L63_FLL1_PHASEDET | CS47L63_FLL1_REFDET |
			((uint32_t)sol->refclk_div << CS47L63_FLL1_REFCLK_DIV_SHIFT) |
			((uint32_t)sol->n << CS47L63_FLL1_N_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_write_reg(dev, CS47L63_FLL1_CONTROL3,
				    ((uint32_t)sol->lambda << CS47L63_FLL1_LAMBDA_SHIFT) |
					    ((uint32_t)sol->theta << CS47L63_FLL1_THETA_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL4,
				     CS47L63_FLL1_GAIN_MASK | CS47L63_FLL1_HP_MASK |
					     CS47L63_FLL1_FB_DIV_MASK,
				     ((uint32_t)sol->gains << CS47L63_FLL1_GAIN_SHIFT) |
					     ((uint32_t)sol->hp << CS47L63_FLL1_HP_SHIFT) |
					     ((uint32_t)sol->fb_div << CS47L63_FLL1_FB_DIV_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL2, CS47L63_FLL1_REFCLK_SRC_MASK,
				     (uint32_t)sol->refclk_src << CS47L63_FLL1_REFCLK_SRC_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_EN, CS47L63_FLL1_EN);
	if (ret < 0) {
		return ret;
	}

	/* Latch the whole set, then let the loop run. */
	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_CTRL_UPD,
				     CS47L63_FLL1_CTRL_UPD);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_HOLD, 0);
	if (ret < 0) {
		return ret;
	}

	/* No wait for lock here. The reference is the SoC's I2S master clock,
	 * and that pin only runs while an I2S transfer does - it is dead at
	 * configure time, so a loop that has not locked yet is the normal
	 * outcome of this function rather than a fault. Lock is observed later,
	 * once the stream is running (cs47l63_clock_locked()).
	 */
	return cs47l63_bus_update_reg(dev, CS47L63_SYSTEM_CLOCK1,
				      CS47L63_SYSCLK_FRAC | CS47L63_SYSCLK_FREQ_MASK |
					      CS47L63_SYSCLK_EN | CS47L63_SYSCLK_SRC_MASK,
				      ((uint32_t)sol->sysclk_freq << CS47L63_SYSCLK_FREQ_SHIFT) |
					      CS47L63_SYSCLK_EN | CS47L63_SYSCLK_SRC_FLL1);
}

int cs47l63_clock_locked(const struct device *dev, bool *locked)
{
	uint32_t sts;
	int ret;

	if (locked == NULL) {
		return -EINVAL;
	}

	ret = cs47l63_bus_read_reg(dev, CS47L63_IRQ1_STS_6, &sts);
	if (ret < 0) {
		return ret;
	}

	*locked = (sts & CS47L63_FLL1_LOCK_STS1) != 0;

	return 0;
}

/* ASP1 serial-port solver and apply path.
 *
 * The rate code, the I2S format code and the ASP1 pin pad configuration are the
 * ones the Apache-2.0 vendor bring-up script states in words
 * (modules/hal/cirrus-logic/cs47l63/config/wisce_init.txt); the field masks are
 * from cs47l63_spec.h.
 */

/** The four pads that carry ASP1 on this board, in pin order. */
static const struct {
	uint32_t addr;
	uint32_t val;
} k_asp1_pads[] = {
	/* ASP1DOUT is driven by the codec; the other three are driven by the
	 * nRF and are inputs here.
	 */
	{CS47L63_GPIO1_CTRL1, CS47L63_GP_CTRL1_ASP_PAD},
	{CS47L63_GPIO2_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
	{CS47L63_GPIO3_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
	{CS47L63_GPIO4_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
};

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

	for (size_t i = 0; i < ARRAY_SIZE(k_rates); i++) {
		if (k_rates[i].hz == frame_clk_hz) {
			*code = k_rates[i].code;
			return 0;
		}
	}

	return -ENOTSUP;
}

int cs47l63_dai_solve(audio_dai_type_t dai_type, const struct i2s_config *i2s,
		      struct cs47l63_dai_solution *out)
{
	uint8_t code;
	int ret;

	if (i2s == NULL || out == NULL) {
		return -EINVAL;
	}

	if (dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("Only I2S is implemented on ASP1, got DAI type %d", dai_type);
		return -ENOTSUP;
	}

	/* Refused, never corrected: the nRF drives both clocks on this board,
	 * and a second driver on either net is a hardware fault.
	 */
	if ((i2s->options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) != 0) {
		LOG_ERR("Codec-as-master requested; ASP1 is always a slave here");
		return -ENOTSUP;
	}

	switch (i2s->word_size) {
	case 16:
	case 24:
	case 32:
		break;
	default:
		LOG_ERR("Unsupported word size %u", i2s->word_size);
		return -ENOTSUP;
	}

	ret = rate_code(i2s->frame_clk_freq, &code);
	if (ret < 0) {
		LOG_ERR("No rate-field encoding for %u Hz", i2s->frame_clk_freq);
		return ret;
	}

	switch (i2s->channels) {
	case 1:
		out->enables = CS47L63_ASP1_RX1_EN;
		break;
	case 2:
		out->enables = CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN;
		break;
	default:
		LOG_ERR("Unsupported channel count %u", i2s->channels);
		return -ENOTSUP;
	}

	out->rate_code = code;
	out->fmt = CS47L63_ASP1_FMT_I2S;
	out->slot_width = (uint8_t)i2s->word_size;
	out->word_len = (uint8_t)i2s->word_size;

	return 0;
}

int cs47l63_dai_apply(const struct device *dev, const struct cs47l63_dai_solution *sol)
{
	int ret;

	if (sol == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(k_asp1_pads); i++) {
		ret = cs47l63_bus_write_reg(dev, k_asp1_pads[i].addr, k_asp1_pads[i].val);
		if (ret < 0) {
			return ret;
		}
	}

	/* One decision, two registers: the global slot carries the rate and the
	 * port is pointed at that slot. Computing the rate twice is how the two
	 * silently disagree.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_SAMPLE_RATE1, CS47L63_SAMPLE_RATE_MASK,
				     sol->rate_code);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_CONTROL1, CS47L63_ASP1_RATE_MASK,
				     CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1 << CS47L63_ASP1_RATE_SHIFT);
	if (ret < 0) {
		return ret;
	}

	/* Slot widths, format, and the four master/inversion bits written
	 * explicitly to zero rather than left at whatever reset or a previous
	 * configure left behind.
	 */
	ret = cs47l63_bus_update_reg(
		dev, CS47L63_ASP1_CONTROL2,
		CS47L63_ASP1_RX_WIDTH_MASK | CS47L63_ASP1_TX_WIDTH_MASK | CS47L63_ASP1_FMT_MASK |
			CS47L63_ASP1_BCLK_INV | CS47L63_ASP1_BCLK_MSTR | CS47L63_ASP1_FSYNC_INV |
			CS47L63_ASP1_FSYNC_MSTR,
		((uint32_t)sol->slot_width << CS47L63_ASP1_RX_WIDTH_SHIFT) |
			((uint32_t)sol->slot_width << CS47L63_ASP1_TX_WIDTH_SHIFT) |
			((uint32_t)sol->fmt << CS47L63_ASP1_FMT_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_DATA_CONTROL1, CS47L63_ASP1_TX_WL_MASK,
				     sol->word_len);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_DATA_CONTROL5, CS47L63_ASP1_RX_WL_MASK,
				     sol->word_len);
	if (ret < 0) {
		return ret;
	}

	return cs47l63_bus_write_reg(dev, CS47L63_ASP1_ENABLES1, sol->enables);
}

/* Fault decode, sticky fault state and error callback.
 *
 * The flag bits and their registers are from the Apache-2.0 vendor header
 * modules/hal/cirrus-logic/cs47l63/cs47l63_spec.h (IRQ1_EINT_1 and
 * IRQ1_EINT_6). Every EINT bit on this part is write-1-to-clear.
 */

/** Every flag this driver reads out of IRQ1_EINT_1. */
#define CS47L63_EINT_1_WATCHED                                                                     \
	(CS47L63_OUT1L_SC_EINT1 | CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)

/** Every flag this driver reads out of IRQ1_EINT_6. */
#define CS47L63_EINT_6_WATCHED (CS47L63_FLL1_REF_LOST_EINT1 | CS47L63_FLL1_LOCK_FALL_EINT1)

int cs47l63_fault_register_callback(const struct device *dev, audio_codec_error_callback_t cb)
{
	struct cs47l63_data *data = dev->data;

	data->fault_cb = cb;

	return 0;
}

/** @brief Write the observed flags back to clear them on the part. */
static int clear_flags(const struct device *dev, uint32_t addr, uint32_t flags)
{
	if (flags == 0) {
		return 0;
	}

	return cs47l63_bus_write_reg(dev, addr, flags);
}

int cs47l63_fault_check(const struct device *dev)
{
	uint32_t eint1;
	uint32_t eint6;
	uint32_t errors = 0;
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

	if ((eint1 & CS47L63_OUT1L_SC_EINT1) != 0) {
		errors |= AUDIO_CODEC_ERROR_OVERCURRENT;
	}

	/* The clock latches are evidence of a fault only while the output is
	 * meant to be running. FLL1's reference is MCLK1, which the SoC drives
	 * only for as long as the I2S transfer does, so stopping a route takes
	 * the reference away by design: REF_LOST, LOCK_FALL and the SYSCLK
	 * flags all latch on the way down and mean nothing more than that the
	 * stream stopped. They are still cleared below, because a latch left
	 * set is reported at the next check instead - after the output is back
	 * up and the condition it describes is long gone.
	 *
	 * A reference that genuinely never arrives is not missed by this: the
	 * deferred start check, start_check_work(), reads the lock bit directly once
	 * the stream has had time to come up, and raises the same error.
	 */
	if (((struct cs47l63_data *)dev->data)->output_running &&
	    (((eint1 & (CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)) != 0) ||
	     eint6 != 0)) {
		errors |= CS47L63_ERROR_CLOCK;
	}

	/* Clear on the part before reporting. The flags are edge latches: a
	 * condition that is still present sets them again, and one that has
	 * passed stops re-arming them - which is what makes "once per
	 * occurrence" rather than "once per poll" possible at all.
	 */
	ret = clear_flags(dev, CS47L63_IRQ1_EINT_1, eint1);
	if (ret < 0) {
		return ret;
	}
	ret = clear_flags(dev, CS47L63_IRQ1_EINT_6, eint6);
	if (ret < 0) {
		return ret;
	}

	cs47l63_fault_raise(dev, errors);

	return 0;
}

void cs47l63_fault_raise(const struct device *dev, uint32_t errors)
{
	struct cs47l63_data *data = dev->data;
	uint32_t fresh;

	/* Only bits not already latched reach the callback. The shadow is OR'd
	 * into and zeroed only by cs47l63_fault_clear(), so a fault latched
	 * earlier survives a later poll that no longer sees it.
	 */
	fresh = errors & ~data->fault_errors;
	data->fault_errors |= errors;

	if (fresh != 0) {
		LOG_WRN("CS47L63 fault, errors 0x%02x", data->fault_errors);
		if (data->fault_cb != NULL) {
			data->fault_cb(dev, data->fault_errors);
		}
	}
}

int cs47l63_fault_clear(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->fault_errors = 0;

	ret = cs47l63_bus_write_reg(dev, CS47L63_IRQ1_EINT_1, CS47L63_EINT_1_WATCHED);
	if (ret < 0) {
		return ret;
	}

	return cs47l63_bus_write_reg(dev, CS47L63_IRQ1_EINT_6, CS47L63_EINT_6_WATCHED);
}

/* Analog line input and the ASP1 transmit mixers.
 *
 * Field layouts come from the Apache-2.0 vendor register map
 * (modules/hal/cirrus-logic/cs47l63/cs47l63_spec.h); the two transmit mixers
 * are laid out identically to the OUT1L mixer slots, which is why
 * write_tx_mixer_slot() is the same shape as write_mixer_slot() rather than
 * shared with it - the masks belong to different registers and pretending they
 * are one control is how a change to the output path silently reaches the
 * input.
 */

/** Both halves of the line pair, as one enable mask. */
#define CS47L63_IN2_EN_BOTH (CS47L63_IN2L_EN | CS47L63_IN2R_EN)

/** Both ASP1 transmit slots, as one enable mask. */
#define CS47L63_ASP1_TX_EN_BOTH (CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN)

/**
 * @brief Write one transmit mixer slot: a source and its 0 dB mix volume.
 */
static int write_tx_mixer_slot(const struct device *dev, uint32_t addr, uint16_t src)
{
	return cs47l63_bus_update_reg(
		dev, addr, CS47L63_OUT1LMIX_VOL_MASK | CS47L63_OUT1L_SRC_MASK,
		((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | src);
}

/**
 * @brief Point both transmit mixers at the cached sources.
 */
static int write_in_route(const struct device *dev)
{
	const struct cs47l63_data *data = dev->data;
	int ret;

	ret = write_tx_mixer_slot(dev, CS47L63_ASP1TX1_INPUT1, data->in_src1);
	if (ret < 0) {
		return ret;
	}

	return write_tx_mixer_slot(dev, CS47L63_ASP1TX2_INPUT1, data->in_src2);
}

/**
 * @brief Set both halves of the line pair muted or unmuted at 0 dB.
 *
 * IN_VU is strobed afterwards without exception: it is what latches the two
 * volume fields, so a write without it leaves them changed and the front end
 * still at the previous level.
 */
static int write_input_level(const struct device *dev, bool mute)
{
	static const uint32_t k_control2[] = {CS47L63_IN2L_CONTROL2, CS47L63_IN2R_CONTROL2};
	uint32_t val = ((uint32_t)CS47L63_IN2_VOL_0DB << CS47L63_IN2_VOL_SHIFT) |
		       ((uint32_t)CS47L63_IN2_PGA_VOL_0DB << CS47L63_IN2_PGA_VOL_SHIFT);
	int ret;

	if (mute) {
		val |= CS47L63_IN2_MUTE;
	}

	for (size_t i = 0; i < ARRAY_SIZE(k_control2); i++) {
		ret = cs47l63_bus_update_reg(
			dev, k_control2[i],
			CS47L63_IN2_MUTE | CS47L63_IN2_VOL_MASK | CS47L63_IN2_PGA_VOL_MASK, val);
		if (ret < 0) {
			return ret;
		}
	}

	return cs47l63_bus_write_reg(dev, CS47L63_INPUT_CONTROL3, CS47L63_IN_VU);
}

int cs47l63_in_init(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->in_running = false;
	data->in_src1 = CS47L63_MIXER_SRC_IN2L;
	data->in_src2 = CS47L63_MIXER_SRC_IN2R;

	/* Boot quiet: the transmit slots off, the front end disabled, the two
	 * mixers pointed at nothing. Nothing reaches ASP1DOUT until a start.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_ENABLES1, CS47L63_ASP1_TX_EN_BOTH, 0);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL, CS47L63_IN2_EN_BOTH, 0);
	if (ret < 0) {
		return ret;
	}

	ret = write_tx_mixer_slot(dev, CS47L63_ASP1TX1_INPUT1, CS47L63_MIXER_SRC_NONE);
	if (ret < 0) {
		return ret;
	}

	ret = write_tx_mixer_slot(dev, CS47L63_ASP1TX2_INPUT1, CS47L63_MIXER_SRC_NONE);
	if (ret < 0) {
		return ret;
	}

	return write_input_level(dev, true);
}

int cs47l63_in_start(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	/* Analog, not PDM, at the oversampling the 48 kHz family runs. The rate
	 * field of IN2nCONTROL1 is left alone: its reset value already selects
	 * SAMPLE_RATE1, which is the slot cs47l63_dai_apply() programs.
	 */
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

	ret = write_in_route(dev);
	if (ret < 0) {
		return ret;
	}

	ret = write_input_level(dev, false);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL, CS47L63_IN2_EN_BOTH,
				     CS47L63_IN2_EN_BOTH);
	if (ret < 0) {
		return ret;
	}

	/* An update, never a write: the receive slots cs47l63_dai_apply() enabled
	 * live in this same register, and a full write would take the playback
	 * path down with it.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_ENABLES1, CS47L63_ASP1_TX_EN_BOTH,
				     CS47L63_ASP1_TX_EN_BOTH);
	if (ret < 0) {
		return ret;
	}

	data->in_running = true;

	return 0;
}

int cs47l63_in_stop(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->in_running = false;

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_ENABLES1, CS47L63_ASP1_TX_EN_BOTH, 0);
	if (ret < 0) {
		return ret;
	}

	ret = write_input_level(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL, CS47L63_IN2_EN_BOTH, 0);
	if (ret < 0) {
		return ret;
	}

	ret = write_tx_mixer_slot(dev, CS47L63_ASP1TX1_INPUT1, CS47L63_MIXER_SRC_NONE);
	if (ret < 0) {
		return ret;
	}

	return write_tx_mixer_slot(dev, CS47L63_ASP1TX2_INPUT1, CS47L63_MIXER_SRC_NONE);
}

int cs47l63_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	struct cs47l63_data *data = dev->data;
	uint16_t src1;
	uint16_t src2;

	if ((enum cs47l63_input)input != CS47L63_INPUT_LINE) {
		LOG_ERR("Only the analog line pair is brought up by this driver");
		return -ENOTSUP;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		/* One pin into both slots, so a mono source still arrives on
		 * both halves of the stereo frame the serial port carries.
		 */
		src1 = CS47L63_MIXER_SRC_IN2L;
		src2 = CS47L63_MIXER_SRC_IN2L;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		src1 = CS47L63_MIXER_SRC_IN2R;
		src2 = CS47L63_MIXER_SRC_IN2R;
		break;
	case AUDIO_CHANNEL_ALL:
		src1 = CS47L63_MIXER_SRC_IN2L;
		src2 = CS47L63_MIXER_SRC_IN2R;
		break;
	default:
		return -ENOTSUP;
	}

	data->in_src1 = src1;
	data->in_src2 = src2;

	if (!data->in_running) {
		/* Remembered, not written: a route chosen while stopped is
		 * applied by the next start, like the output path does.
		 */
		return 0;
	}

	return write_in_route(dev);
}

/* Output mixer, headphone amplifier, volume and mute.
 *
 * The enable path follows the Apache-2.0 vendor sources rather than the field
 * names: the amplifier is brought up by setting OUT1L_EN and then waiting for
 * OUT1L_EN_STS, because the analog stage takes time the register write does not
 * account for, and driving audio into it before it reports ready is not visible
 * from any symbol in the map.
 *
 * The two halves of that are split across @ref cs47l63_out_start and
 * @ref cs47l63_out_confirm_start, because the analog stage cannot report ready
 * until SYSCLK runs and SYSCLK does not run until the caller has started the
 * I2S transfer that feeds FLL1 - which happens after start_output returns. The
 * level is put on the pins by the confirming half, so it is never applied to a
 * stage that never came up.
 */

/** Amplifier enable/disable poll: read every 2 ms, give up after 50 reads. */
#define CS47L63_OUT_POLL_MS  2
#define CS47L63_OUT_POLL_MAX 50

/** The volume field is 0.5 dB per step, so one dB is two codes. */
#define CS47L63_VOL_CODES_PER_DB 2

/**
 * @brief Convert a dB level to an OUT1L_VOL code, clamped to the envelope.
 */
static uint8_t vol_db_to_code(int vol_db)
{
	int db = CLAMP(vol_db, CS47L63_VOLUME_MIN_DB, CS47L63_VOLUME_MAX_DB);

	return (uint8_t)(CS47L63_OUT1L_VOL_0DB + (db * CS47L63_VOL_CODES_PER_DB));
}

/**
 * @brief Write OUT1L_VOLUME_1 with the update strobe set.
 *
 * OUT_VU is set on every write without exception: it is what latches the level
 * and the mute bit together, so a write without it leaves the field changed and
 * the output still at the previous value.
 */
static int write_volume_reg(const struct device *dev, uint8_t code, bool mute)
{
	uint32_t val = CS47L63_OUT_VU | code;

	if (mute) {
		val |= CS47L63_OUT1L_MUTE;
	}

	return cs47l63_bus_write_reg(dev, CS47L63_OUT1L_VOLUME_1, val);
}

/**
 * @brief Put the cached level on the pins if it belongs there.
 *
 * While the output is stopped the cache is updated and nothing is written, so
 * a level set before playback holds until the start applies it, and a stopped
 * output is never brought up to a level by a bare property set.
 */
static int apply_cached_volume(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;

	if (!data->output_running) {
		return 0;
	}

	return write_volume_reg(dev, data->vol_code, data->output_muted);
}

int cs47l63_out_init(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->output_running = false;
	data->output_muted = false;
	data->vol_code = CS47L63_OUT1L_VOL_0DB;
	data->src1 = CS47L63_MIXER_SRC_ASP1RX1;
	data->src2 = CS47L63_MIXER_SRC_ASP1RX2;

	/* Boot silent and unpowered: the amplifier off, the volume field at its
	 * cached level but muted. Nothing reaches the jack until start_output.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0);
	if (ret < 0) {
		return ret;
	}

	return write_volume_reg(dev, data->vol_code, true);
}

/** @brief Write one mixer input slot: a source and its 0 dB mix volume. */
static int write_mixer_slot(const struct device *dev, uint32_t addr, uint16_t src)
{
	return cs47l63_bus_update_reg(
		dev, addr, CS47L63_OUT1LMIX_VOL_MASK | CS47L63_OUT1L_SRC_MASK,
		((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | src);
}

static int write_out_route(const struct device *dev)
{
	const struct cs47l63_data *data = dev->data;
	int ret;

	ret = write_mixer_slot(dev, CS47L63_OUT1L_INPUT1, data->src1);
	if (ret < 0) {
		return ret;
	}

	return write_mixer_slot(dev, CS47L63_OUT1L_INPUT2, data->src2);
}

int cs47l63_out_route_output(const struct device *dev, audio_channel_t channel, uint32_t output)
{
	struct cs47l63_data *data = dev->data;
	uint16_t src1;
	uint16_t src2;

	if ((enum cs47l63_output)output != CS47L63_OUTPUT_HP) {
		LOG_ERR("This part has only the headphone terminal");
		return -ENOTSUP;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		src1 = CS47L63_MIXER_SRC_ASP1RX1;
		src2 = CS47L63_MIXER_SRC_NONE;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		src1 = CS47L63_MIXER_SRC_ASP1RX2;
		src2 = CS47L63_MIXER_SRC_NONE;
		break;
	case AUDIO_CHANNEL_ALL:
		/* One physical channel, two slots: both receive slots are mixed
		 * into it.
		 */
		src1 = CS47L63_MIXER_SRC_ASP1RX1;
		src2 = CS47L63_MIXER_SRC_ASP1RX2;
		break;
	default:
		return -ENOTSUP;
	}

	data->src1 = src1;
	data->src2 = src2;

	return write_out_route(dev);
}

int cs47l63_out_start(const struct device *dev)
{
	int ret;

	/* Re-arm at the route and level already chosen rather than redoing
	 * configure()'s work.
	 */
	ret = write_out_route(dev);
	if (ret < 0) {
		return ret;
	}

	/* The enable is written here and confirmed later. The analog stage
	 * needs SYSCLK to come up at all, SYSCLK needs FLL1 locked, and FLL1's
	 * reference is the SoC's I2S master clock - which the caller starts
	 * after it has told this codec to play. Waiting for OUT1L_EN_STS at
	 * this point therefore cannot succeed on a cold boot, and the wait
	 * belongs where the clock exists: cs47l63_out_confirm_start().
	 */
	return cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN,
				      CS47L63_OUT1L_EN);
}

int cs47l63_out_confirm_start(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	ret = cs47l63_bus_poll_reg(dev, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS,
				   CS47L63_OUT1L_EN_STS, CS47L63_OUT_POLL_MS, CS47L63_OUT_POLL_MAX);
	if (ret < 0) {
		LOG_ERR("Headphone amplifier never reported enabled");
		return ret;
	}

	/* Only now is the output live, so only now may the cached level go on
	 * the pins: this is the point the synchronous wait used to reach.
	 */
	data->output_running = true;

	return apply_cached_volume(dev);
}

int cs47l63_out_stop(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	bool locked;
	int ret;

	/* Mute before the amplifier goes down, so the stage is silent while it
	 * collapses rather than after.
	 */
	ret = write_volume_reg(dev, data->vol_code, true);
	if (ret < 0) {
		return ret;
	}

	data->output_running = false;

	ret = cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0);
	if (ret < 0) {
		return ret;
	}

	/* The status bit is updated by the part's own clock domain, and by the
	 * time a route tears down, the I2S transfer that gives FLL1 its
	 * reference has usually already stopped. The amplifier does go down -
	 * the enable bit above is what takes it down - but nothing is left
	 * running to report that it did, so waiting can only ever time out.
	 * Wait while there is a clock to be waited on, and take the write's
	 * word for it when there is not.
	 */
	ret = cs47l63_clock_locked(dev, &locked);
	if (ret < 0) {
		return ret;
	}

	if (!locked) {
		return 0;
	}

	ret = cs47l63_bus_poll_reg(dev, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS, 0,
				   CS47L63_OUT_POLL_MS, CS47L63_OUT_POLL_MAX);
	if (ret < 0) {
		LOG_ERR("Headphone amplifier never reported disabled");
	}

	return ret;
}

int cs47l63_out_set_volume(const struct device *dev, int vol)
{
	struct cs47l63_data *data = dev->data;

	data->vol_code = vol_db_to_code(vol);

	return apply_cached_volume(dev);
}

int cs47l63_out_set_mute(const struct device *dev, bool mute)
{
	struct cs47l63_data *data = dev->data;

	data->output_muted = mute;

	return apply_cached_volume(dev);
}

/* Class API glue. */

/**
 * @brief Read the FLL lock and finish the amplifier enable, once both can work.
 *
 * Deferred rather than done inline because the FLL's reference is the SoC's
 * I2S master clock: that pin starts with the I2S transfer, which the caller
 * sets up after it has told this codec to play. At every point the class API
 * is entered there is therefore no clock at all - nothing for the loop to lock
 * to, and no SYSCLK for the output stage to come up on. The only honest moment
 * for either is a while after start_output returned.
 *
 * A locked loop and an enabled amplifier say nothing. An unlocked loop is the
 * whole diagnostic for a reference that never arrived; an amplifier that is
 * still not enabled once the clock is there is a dead output stage. Both are
 * latched as faults, so neither can pass as silence with no explanation.
 */
static void start_check_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct cs47l63_data *data = CONTAINER_OF(dwork, struct cs47l63_data, start_check);
	const struct device *dev = data->dev;
	bool locked;
	int ret;

	ret = cs47l63_clock_locked(dev, &locked);
	if (ret < 0) {
		LOG_ERR("Failed to read FLL1 lock state: %d", ret);
	} else if (!locked) {
		LOG_ERR("FLL1 still unlocked %d ms after output start - no MCLK1 reference; "
			"the output stage is running on a clock that never started",
			CS47L63_FLL_LOCK_SETTLE_MS);
		cs47l63_fault_raise(dev, CS47L63_ERROR_CLOCK);
	}

	/* Attempted whatever the lock said: the amplifier is what the listener
	 * hears, and its own status bit is a better witness than an inference
	 * from the loop's.
	 */
	ret = cs47l63_out_confirm_start(dev);
	if (ret < 0) {
		LOG_ERR("Output stage never came up %d ms after start: %d",
			CS47L63_FLL_LOCK_SETTLE_MS, ret);
		cs47l63_fault_raise(dev, CS47L63_ERROR_OUTPUT);
	}
}

static int codec_initialize(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;
	struct cs47l63_data *data = dev->data;

	if (!cs47l63_bus_is_ready(dev)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	data->dev = dev;
	k_work_init_delayable(&data->start_check, start_check_work);

	return 0;
}

/**
 * @brief Leave the output stage disabled after a failed configure.
 *
 * A configure that gives up partway must not leave a live amplifier behind on
 * a part whose clock or serial port is in an unknown state.
 */
static int configure_failed(const struct device *dev, int ret)
{
	(void)cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0);

	return ret;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct cs47l63_data *data = dev->data;
	struct cs47l63_fll_solution clock_sol;
	struct cs47l63_dai_solution dai_sol;
	struct k_work_sync sync;
	bool playback;
	bool capture;
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
	case AUDIO_ROUTE_CAPTURE:
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		break;
	default:
		LOG_ERR("Unsupported route %d", cfg->dai_route);
		return -ENOTSUP;
	}

	playback = cfg->dai_route != AUDIO_ROUTE_CAPTURE;
	capture = cfg->dai_route != AUDIO_ROUTE_PLAYBACK;

	/* Solve the clock and the serial port before touching any hardware.
	 * Both are pure computation, so an unreachable rate or a request the
	 * part cannot express is refused here - with the solver's own errno
	 * intact - and the part is left exactly as it was.
	 */
	ret = cs47l63_clock_solve(cfg->mclk_freq, CS47L63_FLL_FOUT_HZ, CS47L63_FLL_SRC_MCLK1,
				  &clock_sol);
	if (ret < 0) {
		LOG_ERR("No FLL1 solution for MCLK %u Hz: %d", cfg->mclk_freq, ret);
		return ret;
	}

	ret = cs47l63_dai_solve(cfg->dai_type, &cfg->dai_cfg.i2s, &dai_sol);
	if (ret < 0) {
		return ret;
	}

	/* A start check armed before this configure would otherwise run against
	 * the new configuration and unmute an output nobody has started.
	 */
	(void)k_work_cancel_delayable_sync(&data->start_check, &sync);

	ret = cs47l63_boot_bringup(dev);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_clock_apply(dev, &clock_sol);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	ret = cs47l63_dai_apply(dev, &dai_sol);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	/* Output configured, not started: the mixer is routed and the level
	 * cached, the amplifier stays down until start_output.
	 */
	ret = cs47l63_out_init(dev);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	if (playback) {
		ret = cs47l63_out_route_output(dev, AUDIO_CHANNEL_ALL, CS47L63_OUTPUT_HP);
		if (ret < 0) {
			return configure_failed(dev, ret);
		}
	}

	/* Same treatment for the capture side: the line pair is left disabled
	 * and muted, so an image that only plays back still leaves the ADC
	 * pins quiet rather than at their reset defaults.
	 */
	ret = cs47l63_in_init(dev);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	if (capture) {
		ret = cs47l63_in_route_input(dev, AUDIO_CHANNEL_ALL, CS47L63_INPUT_LINE);
		if (ret < 0) {
			return configure_failed(dev, ret);
		}
	}

	return cs47l63_fault_clear(dev);
}

/**
 * @brief Enable the output and arm the check that will finish the job.
 *
 * Shared by the two class entry points that start audio, because the deferred
 * half is not optional: without it the amplifier is enabled on the part but
 * the driver never learns it came up, so the output stays muted forever.
 */
static int start_output_tx(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret = cs47l63_out_start(dev);

	if (ret < 0) {
		return ret;
	}

	/* By the time this runs the caller's stream - and with it the FLL's
	 * reference, SYSCLK and the amplifier - has had time to come up.
	 */
	(void)k_work_reschedule(&data->start_check, K_MSEC(CS47L63_FLL_LOCK_SETTLE_MS));

	return 0;
}

/**
 * @brief Drop a pending start check and take the output down.
 *
 * The cancel is synchronous: a check that ran to completion after the stop
 * would mark the output running and unmute a stage the caller has just
 * disabled.
 */
static int stop_output_tx(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	struct k_work_sync sync;

	(void)k_work_cancel_delayable_sync(&data->start_check, &sync);

	return cs47l63_out_stop(dev);
}

static void codec_start_output(const struct device *dev)
{
	int ret = start_output_tx(dev);

	if (ret < 0) {
		LOG_ERR("Failed to start output: %d", ret);
	}

	/* Opportunistic fault poll: the driver does not service the part's
	 * interrupt line, so the class operations that already touch the output
	 * stage stand in for one. Best-effort - a bus error here must not
	 * change what start_output did.
	 */
	(void)cs47l63_fault_check(dev);
}

static void codec_stop_output(const struct device *dev)
{
	int ret = stop_output_tx(dev);

	if (ret < 0) {
		LOG_ERR("Failed to stop output: %d", ret);
	}
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	int ret;

	/* One physical output channel, fed by both receive slots. A per-side
	 * level or mute cannot be expressed, so it is refused rather than
	 * applied to the whole output - which is indistinguishable, at the far
	 * end, from the driver having got the channel wrong.
	 */
	if (channel != AUDIO_CHANNEL_ALL) {
		LOG_ERR("Only AUDIO_CHANNEL_ALL is addressable on this part");
		return -EINVAL;
	}

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		ret = cs47l63_out_set_volume(dev, val.vol);
		(void)cs47l63_fault_check(dev);
		return ret;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		ret = cs47l63_out_set_mute(dev, val.mute);
		(void)cs47l63_fault_check(dev);
		return ret;
	default:
		break;
	}

	/* Refused, not accepted and dropped: a silently ignored property is
	 * indistinguishable from a hardware fault at the far end.
	 */
	return -ENOTSUP;
}

/**
 * @brief Class API @c apply_properties.
 *
 * Nothing to commit: every property this driver accepts is latched on the part
 * by the volume-update strobe as it is written, so there is no batch waiting
 * here for a commit.
 */
static int codec_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;

	if ((dir & AUDIO_DAI_DIR_RX) != 0) {
		ret = cs47l63_in_start(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if ((dir & AUDIO_DAI_DIR_TX) != 0) {
		return start_output_tx(dev);
	}

	return 0;
}

static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;

	if ((dir & AUDIO_DAI_DIR_RX) != 0) {
		ret = cs47l63_in_stop(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if ((dir & AUDIO_DAI_DIR_TX) != 0) {
		return stop_output_tx(dev);
	}

	return 0;
}

/* .write and .register_done_callback are left unset: this part has no
 * streaming-write path in the class API contract this driver implements.
 */
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

#define CS47L63_DEFINE(inst)                                                                       \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, reset_gpios),                                     \
		     "cs47l63: reset-gpios is required, boot starts with a hardware reset");       \
	static struct cs47l63_data cs47l63_data_##inst;                                            \
	static const struct cs47l63_config cs47l63_config_##inst = {                               \
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB),             \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		.gpio9_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, gpio9_gpios, {0}),                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, codec_initialize, NULL, &cs47l63_data_##inst,                  \
			      &cs47l63_config_##inst, POST_KERNEL,                                 \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CS47L63_DEFINE)
