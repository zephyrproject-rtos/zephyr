/*
 * Copyright 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>

static uint32_t sample_rate;
static uint8_t channels;

void board_codec_init(void)
{
}

void board_codec_configure(uint32_t sample_rate, uint8_t channels)
{
}

void board_codec_deconfigure(void)
{
}

void board_codec_start(void)
{
}

void board_codec_stop(void)
{
}

void board_codec_media_play(uint8_t *data, uint32_t length)
{
}
