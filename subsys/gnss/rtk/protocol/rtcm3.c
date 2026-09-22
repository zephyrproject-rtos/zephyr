/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/gnss/rtk/decoder.h>
#include <zephyr/sys/crc.h>

#define RTCM3_FRAME_SYNC_SZ		1
#define RTCM3_FRAME_HDR_SZ		2
#define RTCM3_FRAME_CHECKSUM_SZ		3
#define RTCM3_FRAME_OVERHEAD		(RTCM3_FRAME_SYNC_SZ + RTCM3_FRAME_HDR_SZ + \
					 RTCM3_FRAME_CHECKSUM_SZ)

#define RTCM3_SYNC_BYTE 0xD3

#define RTCM3_FRAME_PAYLOAD_SZ(hdr)	(sys_be16_to_cpu(hdr) & BIT_MASK(10))
#define RTCM3_FRAME_SZ(payload_len) ((payload_len) + RTCM3_FRAME_OVERHEAD)

struct rtcm3_frame {
	uint8_t sync_frame;
	uint16_t hdr;
	uint8_t payload[];
} __packed;

int gnss_rtk_decoder_frame_get(uint8_t *buf, size_t buf_len,
			       uint8_t **data, size_t *data_len)
{
	for (size_t i = 0 ; (i + RTCM3_FRAME_OVERHEAD - 1) < buf_len ; i++) {
		if (buf[i] != RTCM3_SYNC_BYTE) {
			continue;
		}

		struct rtcm3_frame *frame = (struct rtcm3_frame *)&buf[i];
		uint16_t payload_len = RTCM3_FRAME_PAYLOAD_SZ(frame->hdr);
		uint16_t remaining_bytes = buf_len - i;

		if (payload_len == 0 ||
		    RTCM3_FRAME_SZ(payload_len) > remaining_bytes) {
			continue;
		}

		if (crc24q_rtcm3((const uint8_t *)frame,
				 RTCM3_FRAME_SZ(payload_len)) == 0) {
			*data = (uint8_t *)frame;
			*data_len = RTCM3_FRAME_SZ(payload_len);

			return 0;
		}
	}

	return -ENOENT;
}
