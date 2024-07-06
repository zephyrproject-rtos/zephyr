/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_id

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

struct sam0_uid {
	uint32_t id[4];
};

int z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct sam0_uid dev_id;

	dev_id.id[0] = sys_cpu_to_be32(*(const uint32_t *)
				       DT_INST_REG_ADDR_BY_IDX(0, 0));
	dev_id.id[1] = sys_cpu_to_be32(*(const uint32_t *)
				       DT_INST_REG_ADDR_BY_IDX(0, 1));
	dev_id.id[2] = sys_cpu_to_be32(*(const uint32_t *)
				       DT_INST_REG_ADDR_BY_IDX(0, 2));
	dev_id.id[3] = sys_cpu_to_be32(*(const uint32_t *)
				       DT_INST_REG_ADDR_BY_IDX(0, 3));

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}
