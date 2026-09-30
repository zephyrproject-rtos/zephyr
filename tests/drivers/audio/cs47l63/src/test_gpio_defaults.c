/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_gpio_defaults.c
 * @brief cirrus,gpio-defaults: GPIOn_CTRL1 values written each time the driver resets the part
 *
 * The node lists GPIO5 = 0x61000001, GPIO6 = 0 and GPIO10 = 0x410001fa. GPIO8 sets
 * reserved bit 23, GPIO9 reserved bit 11, GPIO1-4 and 7 are 0xffffffff, and GPIO11-12
 * have no entry.
 * Addresses are DS1249F2 table 4-56, not derived from the driver's names.
 */

#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_gpio))

#define GPIO_CTRL1(n) (0x00000C08U + 4U * ((n) - 1U))

#define GPIO5_DEFAULT  0x61000001U
#define GPIO6_DEFAULT  0x00000000U
#define GPIO10_DEFAULT 0x410001FAU

#define ASP_PAD_OUT 0x61000000U
#define ASP_PAD_IN  0xE1000000U

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
}

ZTEST_SUITE(cs47l63_gpio_defaults, NULL, NULL, before_each, NULL, NULL);

ZTEST(cs47l63_gpio_defaults, test_listed_entries_are_written_at_bringup)
{
	uint32_t val;

	zassert_ok(cs47l63_boot_bringup(CODEC));

	zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_CTRL1(10)), 1);
	zassert_true(cs47l63_emul_nth_write(EMUL, GPIO_CTRL1(10), 0, &val));
	zassert_equal(val, GPIO10_DEFAULT, "GPIO10_CTRL1 got 0x%08x", val);

	zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_CTRL1(5)), 1);
	zassert_true(cs47l63_emul_nth_write(EMUL, GPIO_CTRL1(5), 0, &val));
	zassert_equal(val, GPIO5_DEFAULT, "GPIO5_CTRL1 got 0x%08x", val);

	zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_CTRL1(6)), 1);
	zassert_true(cs47l63_emul_nth_write(EMUL, GPIO_CTRL1(6), 0, &val));
	zassert_equal(val, GPIO6_DEFAULT, "GPIO6_CTRL1 got 0x%08x", val);
}

ZTEST(cs47l63_gpio_defaults, test_reserved_bit_and_unlisted_entries_are_untouched)
{
	zassert_ok(cs47l63_boot_bringup(CODEC));

	for (uint32_t n = 1U; n <= 12U; n++) {
		if (n == 5U || n == 6U || n == 10U) {
			continue;
		}
		zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_CTRL1(n)), 0,
			      "GPIO%u_CTRL1 has no default and must stay at reset", n);
	}
}

ZTEST(cs47l63_gpio_defaults, test_written_only_once_the_part_has_booted)
{
	int devid_idx;
	int gpio_idx;

	zassert_ok(cs47l63_boot_bringup(CODEC));

	devid_idx = cs47l63_emul_read_index(EMUL, CS47L63_DEVID, 0);
	gpio_idx = cs47l63_emul_write_index(EMUL, GPIO_CTRL1(10), 0);

	zassert_true(devid_idx >= 0);
	zassert_true(gpio_idx > devid_idx, "a write before identification is lost to boot");
}

ZTEST(cs47l63_gpio_defaults, test_not_written_into_a_part_that_never_booted)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_2, 0);

	zassert_not_ok(cs47l63_boot_bringup(CODEC));

	zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_CTRL1(10)), 0);
}

ZTEST(cs47l63_gpio_defaults, test_every_reset_writes_them_again)
{
	zassert_ok(cs47l63_boot_bringup(CODEC));
	zassert_ok(cs47l63_boot_bringup(CODEC));

	zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_CTRL1(10)), 2,
		      "a reset returns GPIO10_CTRL1 to its reset value");
	zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_CTRL1(5)), 2);
	zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_CTRL1(6)), 2);
}

ZTEST(cs47l63_gpio_defaults, test_configure_keeps_defaults_and_asp_pads)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 6144000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE,
		.dai_cfg.i2s.word_size = 16,
		.dai_cfg.i2s.channels = 2,
		.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S,
		.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET,
		.dai_cfg.i2s.frame_clk_freq = 48000,
	};

	zassert_ok(audio_codec_configure(CODEC, &cfg));

	zassert_equal(cs47l63_emul_get_reg(EMUL, GPIO_CTRL1(10)), GPIO10_DEFAULT);
	zassert_equal(cs47l63_emul_get_reg(EMUL, GPIO_CTRL1(5)), GPIO5_DEFAULT,
		      "GPIO5 is an ASP2 pad, and ASP2 is not in use here");
	zassert_equal(cs47l63_emul_get_reg(EMUL, GPIO_CTRL1(1)), ASP_PAD_OUT);
	for (uint32_t n = 2U; n <= 4U; n++) {
		zassert_equal(cs47l63_emul_get_reg(EMUL, GPIO_CTRL1(n)), ASP_PAD_IN,
			      "GPIO%u is an ASP1 pad", n);
	}
}
