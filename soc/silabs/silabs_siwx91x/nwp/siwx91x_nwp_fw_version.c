/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "siwx91x_nwp_fw_version.h"

/* This value needs to be updated when the Wiseconnect SDK (hal_silabs/wiseconnect) is updated. */
const struct siwx91x_nwp_version siwx91x_nwp_fw_expected_version = {
	.rom_id = 0x0B,
	.major = 2,
	.minor = 15,
	.security_version = 5,
	.patch_num = 0,
	.customer_id = 0,
	.build_num = 12,
};
