/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_boot.c
 * @brief CS47L63 reset, boot-done wait, identification and trim
 *
 * The reset timing, the boot-done poll bound and the trim block are taken from
 * the Apache-2.0 vendor driver (modules/hal/cirrus-logic/cs47l63/cs47l63.c,
 * cs47l63_reset() and cs47l63_patch()).
 */

#include "cs47l63_boot.h"

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "cs47l63_bus.h"
#include "cs47l63_priv.h"
#include "cs47l63_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(cs47l63);

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
static void hw_reset(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;

	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	k_msleep(CS47L63_RESET_SETTLE_MS);
	k_msleep(CS47L63_RESET_SETTLE_MS);
	gpio_pin_set_dt(&cfg->reset_gpio, 0);
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
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(k_trim_otpid_8); i++) {
		ret = cs47l63_bus_write_reg(dev, k_trim_otpid_8[i].addr, k_trim_otpid_8[i].val);
		if (ret < 0) {
			/* Leave the region locked even on the way out. */
			(void)write_key_pair(dev, k_key_lock);
			return ret;
		}
	}

	return write_key_pair(dev, k_key_lock);
}

int cs47l63_boot_bringup(const struct device *dev)
{
	int ret;

	hw_reset(dev);

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
