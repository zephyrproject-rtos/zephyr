/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_fault.c
 * @brief CS47L63 fault decode, sticky fault state and error callback
 *
 * The flag bits and their registers are from the Apache-2.0 vendor header
 * modules/hal/cirrus-logic/cs47l63/cs47l63_spec.h (IRQ1_EINT_1 and
 * IRQ1_EINT_6). Every EINT bit on this part is write-1-to-clear.
 */

#include "cs47l63_fault.h"

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include "cs47l63_bus.h"
#include "cs47l63_priv.h"
#include "cs47l63_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(cs47l63);

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
	 * deferred start check in cs47l63.c reads the lock bit directly once
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
