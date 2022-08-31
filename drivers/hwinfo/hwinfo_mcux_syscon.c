/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_uid

#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

#define UID_WORD_COUNT (DT_INST_REG_SIZE(0) / sizeof(uint32_t))

struct uid {
	uint32_t id[UID_WORD_COUNT];
};

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	volatile const uint32_t * const uid_addr = (uint32_t *) DT_INST_REG_ADDR(0);
	struct uid dev_id;

	if (buffer == NULL) {
		return 0;
	}

	for (size_t i = 0 ; i < UID_WORD_COUNT ; i++) {
		dev_id.id[i] = sys_cpu_to_be32(uid_addr[i]);
	}

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}
