/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/hwinfo.h>
#include <string.h>

struct stm32_uid {
	u32_t id[3];
};

ssize_t z_impl_hwinfo_get_device_id(u8_t *buffer, size_t length)
{
	struct stm32_uid dev_id;

	dev_id.id[0] = LL_GetUID_Word0();
	dev_id.id[1] = LL_GetUID_Word1();
	dev_id.id[2] = LL_GetUID_Word2();

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}
