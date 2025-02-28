/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct scmi_shmem_layout {
	volatile uint32_t res0;
	volatile uint32_t chan_status;
	volatile uint32_t res1[2];
	volatile uint32_t chan_flags;
	volatile uint32_t len;
	volatile uint32_t msg_hdr;
};

int scmi_shmem_vendor_validate_message(const struct scmi_shmem_layout *layout);
void scmi_shmem_vendor_prepare_message(struct scmi_shmem_layout *layout);
