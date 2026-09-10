/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_public_header.c
 * @brief The terminal IDs an application gets from the public header
 *
 * Includes only the public header. Mixer addresses and source codes are from DS1249F2
 * section 4.3.3.
 */

#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/audio/cs47l63.h>
#include <zephyr/ztest.h>

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_out))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_out))

#define OUT1L_INPUT1   0x00008100U
#define OUT1L_INPUT2   0x00008104U
#define MIXER_SRC_MASK 0x000001FFU
#define SRC_ASP1RX1    0x020U
#define SRC_ASP1RX2    0x021U

BUILD_ASSERT(CS47L63_INPUT_LINE == 0 && CS47L63_INPUT_PDM == 1);
BUILD_ASSERT(CS47L63_OUTPUT_HP == 0);

ZTEST(cs47l63_out, test_output_id_routes_both_slots_to_the_headphone)
{
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, CS47L63_OUTPUT_HP));

	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT1) & MIXER_SRC_MASK, SRC_ASP1RX1,
		      "CS47L63_OUTPUT_HP must mix ASP1 RX1 into OUT1L");
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT2) & MIXER_SRC_MASK, SRC_ASP1RX2,
		      "CS47L63_OUTPUT_HP must mix ASP1 RX2 into OUT1L");
}
