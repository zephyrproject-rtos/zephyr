/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file polled_only.h
 * @brief Checks shared by the parts that never arm the IRQ line
 */

#ifndef CS47L63_TEST_POLLED_ONLY_H_
#define CS47L63_TEST_POLLED_ONLY_H_

#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define POLLED_SETTLE_MS 100U

static atomic_t s_cb_errors;
static atomic_t s_cb_calls;

static void polled_fault_cb(const struct device *dev, uint32_t errors)
{
	ARG_UNUSED(dev);
	(void)atomic_set(&s_cb_errors, (atomic_val_t)errors);
	(void)atomic_inc(&s_cb_calls);
}

static void polled_setup(const struct device *codec, const struct emul *emul)
{
	cs47l63_emul_reset(emul);
	zassert_ok(cs47l63_test_configure(codec, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	(void)atomic_set(&s_cb_errors, 0);
	(void)atomic_set(&s_cb_calls, 0);
	zassert_ok(audio_codec_register_error_callback(codec, polled_fault_cb));
}

static void polled_teardown(const struct device *codec)
{
	zassert_ok(audio_codec_register_error_callback(codec, NULL));
}

static void polled_check_masks_untouched(const struct emul *emul)
{
	zassert_equal(cs47l63_emul_write_count(emul, CS47L63_IRQ1_MASK_1), 0U);
	zassert_equal(cs47l63_emul_write_count(emul, CS47L63_IRQ1_MASK_2), 0U);
	zassert_equal(cs47l63_emul_write_count(emul, CS47L63_IRQ1_MASK_6), 0U);
}

static void polled_check_reported_by_the_poll(const struct device *codec,
					      const struct emul *emul)
{
	audio_property_value_t unmute = {.mute = false};
	const uint32_t reads = cs47l63_emul_read_count(emul, CS47L63_IRQ1_EINT_1);

	cs47l63_emul_set_reg(emul, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	(void)k_msleep((int32_t)POLLED_SETTLE_MS);

	zassert_equal(cs47l63_emul_read_count(emul, CS47L63_IRQ1_EINT_1), reads,
		      "no service ran: the interrupt was never enabled");
	zassert_equal(atomic_get(&s_cb_calls), 0);

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute));

	zassert_equal(atomic_get(&s_cb_calls), 1, "the poll still reports it");
	zassert_equal((uint32_t)atomic_get(&s_cb_errors), (uint32_t)AUDIO_CODEC_ERROR_OVERCURRENT);
}

#endif /* CS47L63_TEST_POLLED_ONLY_H_ */
