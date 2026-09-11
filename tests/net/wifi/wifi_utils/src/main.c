/* main.c - Wi-Fi utility function tests */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_utils.h>

struct chan_to_band_test {
	uint16_t chan;
	enum wifi_frequency_bands band;
};

static const struct chan_to_band_test chan_to_band_tests[] = {
	/* Not valid in any band. An interface that is not associated
	 * reports channel 0 and must not be claimed to be on 2.4 GHz.
	 */
	{0, WIFI_FREQ_BAND_UNKNOWN},
	{15, WIFI_FREQ_BAND_UNKNOWN},
	{234, WIFI_FREQ_BAND_UNKNOWN},

	/* 2.4 GHz. These are also valid 6 GHz channel numbers, the
	 * lower band wins.
	 */
	{1, WIFI_FREQ_BAND_2_4_GHZ},
	{6, WIFI_FREQ_BAND_2_4_GHZ},
	{14, WIFI_FREQ_BAND_2_4_GHZ},

	/* 5 GHz. Channel 108 is the one from the original bug report. */
	{36, WIFI_FREQ_BAND_5_GHZ},
	{108, WIFI_FREQ_BAND_5_GHZ},
	{165, WIFI_FREQ_BAND_5_GHZ},

	/* 6 GHz, i.e. above 14 and not a valid 5 GHz channel. */
	{17, WIFI_FREQ_BAND_6_GHZ},
	{33, WIFI_FREQ_BAND_6_GHZ},
	{233, WIFI_FREQ_BAND_6_GHZ},
};

ZTEST(net_wifi_utils, test_chan_to_band)
{
	for (int i = 0; i < ARRAY_SIZE(chan_to_band_tests); i++) {
		const struct chan_to_band_test *test = &chan_to_band_tests[i];

		zexpect_equal(wifi_utils_chan_to_band(test->chan), test->band,
			      "Channel %u mapped to band %d, expected %d", test->chan,
			      wifi_utils_chan_to_band(test->chan), test->band);
	}
}

ZTEST(net_wifi_utils, test_chan_to_band_agrees_with_validators)
{
	/* Whatever band is returned, the channel must be valid in it, and
	 * an unknown band must mean no band accepts the channel.
	 */
	for (uint16_t chan = 0; chan <= 300; chan++) {
		enum wifi_frequency_bands band = wifi_utils_chan_to_band(chan);

		if (band == WIFI_FREQ_BAND_UNKNOWN) {
			zexpect_false(wifi_utils_validate_chan_2g(chan),
				      "Channel %u is valid in 2.4 GHz", chan);
			zexpect_false(wifi_utils_validate_chan_5g(chan),
				      "Channel %u is valid in 5 GHz", chan);
			zexpect_false(wifi_utils_validate_chan_6g(chan),
				      "Channel %u is valid in 6 GHz", chan);
		} else {
			zexpect_true(wifi_utils_validate_chan(band, chan),
				     "Channel %u is not valid in band %d", chan, band);
		}
	}
}

struct chan_to_freq_test {
	enum wifi_frequency_bands band;
	uint16_t chan;
	uint16_t freq;
};

static const struct chan_to_freq_test chan_to_freq_tests[] = {
	/* 2.4 GHz, including channel 14 which breaks the 5 MHz spacing. */
	{WIFI_FREQ_BAND_2_4_GHZ, 1, 2412},
	{WIFI_FREQ_BAND_2_4_GHZ, 13, 2472},
	{WIFI_FREQ_BAND_2_4_GHZ, 14, 2484},

	/* 5 GHz, spanning the low and high parts of the band. */
	{WIFI_FREQ_BAND_5_GHZ, 36, 5180},
	{WIFI_FREQ_BAND_5_GHZ, 108, 5540},
	{WIFI_FREQ_BAND_5_GHZ, 165, 5825},

	/* 6 GHz, where channel 2 is the odd one out and 233 is the top. */
	{WIFI_FREQ_BAND_6_GHZ, 1, 5955},
	{WIFI_FREQ_BAND_6_GHZ, 2, 5935},
	{WIFI_FREQ_BAND_6_GHZ, 233, 7115},

	/* Channels that are not valid in the band asked for. */
	{WIFI_FREQ_BAND_2_4_GHZ, 0, 0},
	{WIFI_FREQ_BAND_2_4_GHZ, 15, 0},
	{WIFI_FREQ_BAND_5_GHZ, 14, 0},
	{WIFI_FREQ_BAND_5_GHZ, 37, 0},
	{WIFI_FREQ_BAND_6_GHZ, 234, 0},
	{WIFI_FREQ_BAND_UNKNOWN, 1, 0},
	{WIFI_FREQ_BAND_SUB_1_GHZ, 1, 0},
};

ZTEST(net_wifi_utils, test_chan_to_freq)
{
	for (int i = 0; i < ARRAY_SIZE(chan_to_freq_tests); i++) {
		const struct chan_to_freq_test *t = &chan_to_freq_tests[i];

		zexpect_equal(wifi_utils_chan_to_freq(t->band, t->chan), t->freq,
			      "Channel %u in band %d is not %u MHz", t->chan, t->band, t->freq);
	}
}

ZTEST(net_wifi_utils, test_chan_to_freq_agrees_with_validators)
{
	/* A channel that is valid in a band must produce a frequency, and one
	 * that is not must produce none.
	 */
	static const enum wifi_frequency_bands bands[] = {
		WIFI_FREQ_BAND_2_4_GHZ,
		WIFI_FREQ_BAND_5_GHZ,
		WIFI_FREQ_BAND_6_GHZ,
	};

	for (int i = 0; i < ARRAY_SIZE(bands); i++) {
		for (uint16_t chan = 0; chan <= 300; chan++) {
			uint16_t freq = wifi_utils_chan_to_freq(bands[i], chan);

			if (wifi_utils_validate_chan(bands[i], chan)) {
				zexpect_not_equal(freq, 0, "Channel %u in band %d has no frequency",
						  chan, bands[i]);
			} else {
				zexpect_equal(freq, 0, "Channel %u in band %d has a frequency",
					      chan, bands[i]);
			}
		}
	}
}

ZTEST_SUITE(net_wifi_utils, NULL, NULL, NULL, NULL, NULL);
