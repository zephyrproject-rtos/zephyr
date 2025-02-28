/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include "../shmem_internal.h"

LOG_MODULE_REGISTER(arm_scmi_shmem_nxp);

int scmi_shmem_vendor_validate_message(const struct scmi_shmem_layout *layout)
{
#ifdef CONFIG_ARM_SCMI_SHMEM_USE_CRC
	/* crc match? */
	if (layout->res1[1] != crc32_ieee((const uint8_t *)&layout->msg_hdr, layout->len)) {
		LOG_ERR("bad message crc");
		return -EINVAL;
	}
#endif

	return 0;
}

void scmi_shmem_vendor_prepare_message(struct scmi_shmem_layout *layout)
{
#ifdef CONFIG_ARM_SCMI_SHMEM_USE_CRC
	layout->res1[1] = crc32_ieee((const uint8_t *)&layout->msg_hdr, layout->len);
#endif
}
