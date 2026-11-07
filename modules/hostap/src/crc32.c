/*
 * Copyright (c) 2026 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Replacement for hostap's utils/crc32.c, using the Zephyr CRC implementation */

#include <zephyr/sys/crc.h>

#include "includes.h"
#include "common.h"
#include "utils/crc32.h"

u32 ieee80211_crc32(const u8 *frame, size_t frame_len)
{
	return crc32_ieee(frame, frame_len);
}
