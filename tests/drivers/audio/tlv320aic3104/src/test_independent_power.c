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

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_independent_power))

#define LEFT_ADC_PGA_GAIN   0x0F
#define RIGHT_ADC_PGA_GAIN  0x10
#define LINE1L_TO_LADC_CTRL 0x13
#define LINE1R_TO_RADC_CTRL 0x16

#define LEFT_DAC_VOL         0x2B
#define RIGHT_DAC_VOL        0x2C
#define DAC_L1_TO_HPLOUT_VOL 0x2F
#define DAC_R1_TO_HPROUT_VOL 0x40

#define SENTINEL 0x55

static void prime_rx_regs(void)
{
	tlv320aic3104_emul_set_val(0, LEFT_ADC_PGA_GAIN, SENTINEL);
	tlv320aic3104_emul_set_val(0, RIGHT_ADC_PGA_GAIN, SENTINEL);
	tlv320aic3104_emul_set_val(0, LINE1L_TO_LADC_CTRL, SENTINEL);
	tlv320aic3104_emul_set_val(0, LINE1R_TO_RADC_CTRL, SENTINEL);
}

static void prime_rx_line1_reset(void)
{
	tlv320aic3104_emul_set_val(0, LINE1L_TO_LADC_CTRL, 0x78);
	tlv320aic3104_emul_set_val(0, LINE1R_TO_RADC_CTRL, 0x78);
}

static void prime_tx_regs(void)
{
	tlv320aic3104_emul_set_val(0, LEFT_DAC_VOL, SENTINEL);
	tlv320aic3104_emul_set_val(0, RIGHT_DAC_VOL, SENTINEL);
	tlv320aic3104_emul_set_val(0, DAC_L1_TO_HPLOUT_VOL, SENTINEL);
	tlv320aic3104_emul_set_val(0, DAC_R1_TO_HPROUT_VOL, SENTINEL);
}

ZTEST(tlv320_independent_power, test_start_rx_only_leaves_tx_alone)
{
	tlv320aic3104_emul_reset();
	prime_tx_regs();

	prime_rx_line1_reset();

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x7C,
		      "left ADC powered");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x7C,
		      "right ADC powered");
	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), 0x00,
		      "left PGA unmuted 0dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_ADC_PGA_GAIN), 0x00,
		      "right PGA unmuted 0dB");

	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_DAC_VOL), SENTINEL, "TX untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_DAC_VOL), SENTINEL, "TX untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), SENTINEL,
		      "TX untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), SENTINEL,
		      "TX untouched");
}

ZTEST(tlv320_independent_power, test_start_tx_only_leaves_rx_alone)
{
	tlv320aic3104_emul_reset();
	prime_rx_regs();

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_TX));

	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_DAC_VOL), 0x00, "left DAC unmuted");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_DAC_VOL), 0x00, "right DAC unmuted");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x00,
		      "left HP restored");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x00,
		      "right HP restored");

	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), SENTINEL, "RX untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_ADC_PGA_GAIN), SENTINEL, "RX untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), SENTINEL,
		      "RX untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), SENTINEL,
		      "RX untouched");
}

ZTEST(tlv320_independent_power, test_start_txrx_powers_both)
{
	tlv320aic3104_emul_reset();

	prime_rx_line1_reset();

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_TXRX));

	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x7C, "ADC powered");
	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_DAC_VOL), 0x00, "DAC unmuted");
}

ZTEST(tlv320_independent_power, test_stop_rx_only_leaves_tx_alone)
{
	tlv320aic3104_emul_reset();
	prime_tx_regs();

	prime_rx_line1_reset();

	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), 0x80, "left PGA muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_ADC_PGA_GAIN), 0x80, "right PGA muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x78,
		      "left ADC powered down");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x78,
		      "right ADC powered down");

	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_DAC_VOL), SENTINEL, "TX untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), SENTINEL,
		      "TX untouched");
}

ZTEST(tlv320_independent_power, test_stop_tx_only_leaves_rx_alone)
{
	tlv320aic3104_emul_reset();
	prime_rx_regs();

	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_TX));

	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_DAC_VOL), 0x80, "left DAC muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_DAC_VOL), 0x80, "right DAC muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0xF6,
		      "left HP muted band");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0xF6,
		      "right HP muted band");

	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), SENTINEL, "RX untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), SENTINEL,
		      "RX untouched");
}

ZTEST_SUITE(tlv320_independent_power, NULL, NULL, NULL, NULL, NULL);
