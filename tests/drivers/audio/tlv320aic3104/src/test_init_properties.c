/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_init_properties))

#define MICBIAS_CTRL 0x19

#define CODEC_FILTER 0x0C

ZTEST(tlv320_init_properties, test_micbias_and_hpf_applied_on_rx_start)
{
	tlv320aic3104_emul_reset();

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(tlv320aic3104_emul_last_val(0, MICBIAS_CTRL), 0x80, "MICBIAS 2.5V");
	zassert_equal(tlv320aic3104_emul_last_val(0, CODEC_FILTER), 0xF0,
		      "HPF code 3 both channels");
}

ZTEST_SUITE(tlv320_init_properties, NULL, NULL, NULL, NULL, NULL);
