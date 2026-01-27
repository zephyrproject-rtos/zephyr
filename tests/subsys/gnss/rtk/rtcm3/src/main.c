/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <zephyr/gnss/rtk/decoder.h>

ZTEST_SUITE(rtk_decoder, NULL, NULL, NULL, NULL, NULL);

ZTEST(rtk_decoder, test_frame_is_detected)
{
	uint8_t cmd_rtcm3[] = {
		0xD3, /* Sync byte */
		0x00, 0x04, /* Length: 4 bytes */
		0x4C, 0xE0, 0x00, 0x80, /* Payload */
		0xED, 0xED, 0xD6 /* CRC */
	};
	uint8_t *data;
	size_t data_len;

	zassert_equal(0,
		      gnss_rtk_decoder_frame_get(cmd_rtcm3, sizeof(cmd_rtcm3), &data, &data_len));
	zassert_equal_ptr(cmd_rtcm3, data);
	zassert_equal(sizeof(cmd_rtcm3), data_len);
}

ZTEST(rtk_decoder, test_frame_is_detected_after_invalid_data)
{
	uint8_t cmd_rtcm3[] = {
		0xFF, 0xFF,
		0xD3, /* Sync byte */
		0x00, 0x04, /* Length: 4 bytes */
		0x4C, 0xE0, 0x00, 0x80, /* Payload */
		0xED, 0xED, 0xD6 /* CRC */
	};
	uint8_t *data;
	size_t data_len;

	zassert_equal(0,
		      gnss_rtk_decoder_frame_get(cmd_rtcm3, sizeof(cmd_rtcm3), &data, &data_len));
	zassert_equal_ptr(&cmd_rtcm3[2], data);
	zassert_equal(sizeof(cmd_rtcm3) - 2, data_len);
}

ZTEST(rtk_decoder, test_frame_with_invalid_crc_is_invalid_data)
{
	uint8_t cmd_rtcm3[] = {0xD3, 0x00, 0x01, 0xFF, 0x00, 0x01, 0x02};
	uint8_t *data;
	size_t data_len;

	zassert_equal(-ENOENT,
		      gnss_rtk_decoder_frame_get(cmd_rtcm3, sizeof(cmd_rtcm3), &data, &data_len));
}

ZTEST(rtk_decoder, test_frame_with_zero_payload_len_is_skipped)
{
	uint8_t cmd_rtcm3[] = {
		0xD3, /* Sync byte */
		0x00, 0x00, /* Length: 0 bytes (payload_len == 0) */
		0x00, 0x00, 0x00 /* CRC (dummy, frame will be skipped before CRC check) */
	};
	uint8_t *data;
	size_t data_len;

	zassert_equal(-ENOENT,
		      gnss_rtk_decoder_frame_get(cmd_rtcm3, sizeof(cmd_rtcm3), &data, &data_len));
}

ZTEST(rtk_decoder, test_frame_exceeding_remaining_bytes_is_skipped)
{
	/* payload_len = 1, so RTCM3_FRAME_SZ(1) = 1 + 6 = 7 bytes
	 * Buffer has only 6 bytes remaining (sync + header + 3 bytes)
	 * So 7 > 6, frame should be skipped
	 */
	uint8_t cmd_rtcm3[] = {
		0xD3, /* Sync byte */
		0x00, 0x01, /* Length: 1 byte */
		0xFF, 0xFF, 0xFF /* Only 3 bytes available, but need 4 (1 payload + 3 CRC) */
		/* Missing 1 byte - frame incomplete */
	};
	uint8_t *data;
	size_t data_len;

	zassert_equal(-ENOENT,
		      gnss_rtk_decoder_frame_get(cmd_rtcm3, sizeof(cmd_rtcm3), &data, &data_len));
}

ZTEST(rtk_decoder, test_frame_exceeding_remaining_bytes_after_invalid_data)
{
	uint8_t cmd_rtcm3[] = {
		0xFF, 0xFF, /* Invalid data */
		0xD3, /* Sync byte */
		0x00, 0x02, /* Length: 2 bytes, so RTCM3_FRAME_SZ(2) = 2 + 6 = 8 bytes */
		0xFF, 0xFF, 0xFF /* Only 3 bytes available, but need 5 (2 payload + 3 CRC) */
		/* Missing 2 bytes - frame incomplete */
	};
	uint8_t *data;
	size_t data_len;

	zassert_equal(-ENOENT,
		      gnss_rtk_decoder_frame_get(cmd_rtcm3, sizeof(cmd_rtcm3), &data, &data_len));
}
