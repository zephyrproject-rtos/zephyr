/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/shmem.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(arm_scmi_shmem_nxp);

#define SMT_CRC_NONE  0U
#define SMT_CRC_XOR   1U /* Unsupported */
#define SMT_CRC_J1850 2U /* Unsupported */
#define SMT_CRC_CRC32 3U

int scmi_shmem_vendor_read_message(const struct scmi_shmem_layout *layout)
{
	uint32_t validation_type = layout->res1[0];

	if (validation_type == SMT_CRC_CRC32) {
		if (layout->res1[1] != crc32_ieee((const uint8_t *)&layout->msg_hdr, layout->len)) {
			LOG_ERR("bad message crc");
			return -EBADMSG;
		}
	} else if (validation_type == SMT_CRC_NONE) {
		/* do nothing */
	} else {
		LOG_ERR("unsupported validation type 0x%x", validation_type);
		return -EINVAL;
	}

	return 0;
}

int scmi_shmem_vendor_write_message(struct scmi_shmem_layout *layout)
{
	uint32_t validation_type = layout->res1[0];

	if (validation_type == SMT_CRC_CRC32) {
		layout->res1[1] = crc32_ieee((const uint8_t *)&layout->msg_hdr, layout->len);
	} else if (validation_type == SMT_CRC_NONE) {
		/* do nothing */
	} else {
		LOG_ERR("unsupported validation type 0x%x", validation_type);
		return -EINVAL;
	}

	return 0;
}
