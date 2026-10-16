/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ADI AXI System ID ROM decode helper.
 *
 * The AXI System ID core is exposed by Zephyr as a read-only OTP (the
 * transport). The ROM *format* — header, build-info records and checksums —
 * is decoded here, on top of the standard otp_read() API, so the ROM-format
 * knowledge lives with the consumer instead of inside the transport driver.
 */

#ifndef AD463X_SAMPLE_SYSID_DECODE_H_
#define AD463X_SAMPLE_SYSID_DECODE_H_

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>

#define SYSID_HEADER_V1 1
#define SYSID_HEADER_V2 2

struct __packed sysid_header_v1 {
	uint32_t version;
	uint32_t build_info_offs; /* word offset into ROM */
	uint32_t board_info_offs;
	uint32_t product_info_offs;
	uint32_t custom_info_offs;
	uint32_t pr_custom_info_offs;
	uint32_t padding[9];
	uint32_t crc;
};

struct __packed sysid_build_info_v1 {
	char git_hash[44];
	char git_clean_chk[4];
	char vadj_chk[4];
	char epoch[12];
	uint8_t padding[4];
	uint32_t crc;
};

struct __packed sysid_build_info_v1_1 {
	char git_branch[28];
	char git_hash[44];
	char git_clean_chk[4];
	char vadj_chk[4];
	char epoch[12];
	uint8_t padding[4];
	uint32_t crc;
};

struct sysid_board_info {
	uint32_t header_version;
	char board[64];
	char product[64];
	char git_hash[45];
	char git_branch[29];
	bool git_clean;
};

static inline uint8_t sysid_checksum(const uint8_t *ptr, size_t len)
{
	uint8_t sum = 0;

	while (len-- != 0) {
		sum -= *ptr++;
	}

	return sum;
}

/*
 * Read a NUL-terminated string starting at ROM byte offset into dst (bounded).
 * A zero offset means "absent"; returns 0 on success, negative errno on error.
 * otp_read() itself rejects reads past the ROM end (the driver bounds-checks
 * against the DT reg size), so no separate size query is needed here.
 */
static inline int sysid_read_str(const struct device *otp, off_t offset, char *dst, size_t dst_size)
{
	dst[0] = '\0';

	if (offset == 0) {
		return 0;
	}

	for (size_t i = 0; i < dst_size - 1; i++) {
		char c;
		int ret = otp_read(otp, offset + i, &c, 1);

		if (ret < 0) {
			return (i == 0) ? ret : 0;
		}
		if (c == '\0') {
			dst[i] = '\0';
			return 0;
		}
		dst[i] = c;
	}

	dst[dst_size - 1] = '\0';
	return 0;
}

/*
 * Decode board/product/git information from the System ID ROM behind the given
 * read-only OTP device. Returns 0 on success, negative errno on failure.
 */
static inline int sysid_get_board_info(const struct device *otp, struct sysid_board_info *info)
{
	struct sysid_header_v1 header;
	int ret;

	memset(info, 0, sizeof(*info));

	ret = otp_read(otp, 0, &header, sizeof(header));
	if (ret < 0) {
		return ret;
	}

	if (sysid_checksum((const uint8_t *)&header, sizeof(header)) != 0) {
		return -EFAULT;
	}

	if (header.version != SYSID_HEADER_V1 && header.version != SYSID_HEADER_V2) {
		return -ENOTSUP;
	}
	info->header_version = header.version;

	ret = sysid_read_str(otp, (off_t)header.board_info_offs * sizeof(uint32_t), info->board,
			     sizeof(info->board));
	if (ret < 0) {
		return ret;
	}

	ret = sysid_read_str(otp, (off_t)header.product_info_offs * sizeof(uint32_t), info->product,
			     sizeof(info->product));
	if (ret < 0) {
		return ret;
	}

	if (header.build_info_offs == 0) {
		return -EFAULT;
	}

	off_t build_off = (off_t)header.build_info_offs * sizeof(uint32_t);

	if (header.version == SYSID_HEADER_V2) {
		struct sysid_build_info_v1_1 build;

		ret = otp_read(otp, build_off, &build, sizeof(build));
		if (ret < 0) {
			return ret;
		}
		if (sysid_checksum((const uint8_t *)&build, sizeof(build)) != 0) {
			return -EFAULT;
		}
		memcpy(info->git_hash, build.git_hash, sizeof(build.git_hash));
		memcpy(info->git_branch, build.git_branch, sizeof(build.git_branch));
		info->git_clean = (build.git_clean_chk[0] == 't');
	} else {
		struct sysid_build_info_v1 build;

		ret = otp_read(otp, build_off, &build, sizeof(build));
		if (ret < 0) {
			return ret;
		}
		if (sysid_checksum((const uint8_t *)&build, sizeof(build)) != 0) {
			return -EFAULT;
		}
		memcpy(info->git_hash, build.git_hash, sizeof(build.git_hash));
		info->git_clean = (build.git_clean_chk[0] == 't');
	}

	return 0;
}

#endif /* AD463X_SAMPLE_SYSID_DECODE_H_ */
