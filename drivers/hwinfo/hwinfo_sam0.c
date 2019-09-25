/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/hwinfo.h>
#include <string.h>

struct sam0_uid {
	u32_t id[4];
};

ssize_t z_impl_hwinfo_get_device_id(u8_t *buffer, size_t length)
{
	struct sam0_uid dev_id;

	dev_id.id[0] = *(const u32_t *) DT_INST_0_ATMEL_SAM0_ID_BASE_ADDRESS_0;
	dev_id.id[1] = *(const u32_t *) DT_INST_0_ATMEL_SAM0_ID_BASE_ADDRESS_1;
	dev_id.id[2] = *(const u32_t *) DT_INST_0_ATMEL_SAM0_ID_BASE_ADDRESS_2;
	dev_id.id[3] = *(const u32_t *) DT_INST_0_ATMEL_SAM0_ID_BASE_ADDRESS_3;

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}
