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

ZTEST_SUITE(net_wifi_utils, NULL, NULL, NULL, NULL, NULL);
