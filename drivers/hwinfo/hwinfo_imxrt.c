/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

struct imxrt_uid {
	uint32_t id[2];
};

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct imxrt_uid dev_id;

#ifdef CONFIG_SOC_SERIES_IMX_RT11XX
	dev_id.id[0] = sys_cpu_to_be32(OCOTP->FUSEN[17].FUSE);
	dev_id.id[1] = sys_cpu_to_be32(OCOTP->FUSEN[16].FUSE);
#else
	dev_id.id[0] = sys_cpu_to_be32(OCOTP->CFG2);
	dev_id.id[1] = sys_cpu_to_be32(OCOTP->CFG1);
#endif

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}
