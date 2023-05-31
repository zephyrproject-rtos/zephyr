/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_FPGA_BRIDGE_INTEL_H_
#define ZEPHYR_SUBSYS_FPGA_BRIDGE_INTEL_H_

#include <zephyr/kernel.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>

/* Mask for FPGA-HPS bridges */
#define BRIDGE_MASK					0x0F

/*
 * Bit(2) - Error detected during the process.
 * Bit(25)- FPGA not configured
 */
#define FPGA_CONFIG_NOT_DONE_ERROR	0x02000004

enum smc_cmd_code {
	/* SMC COMMAND ID to disable all the bridges */
	FPGA_ALL_BRIDGE_DISABLE = 0x00,
	/* SMC COMMAND ID to enable all the bridges */
	FPGA_ALL_BRIDGE_ENABLE = 0x01
};

enum smc_request {
	/* SMC request parameter a2 index*/
	SMC_REQUEST_A2_INDEX = 0x00,
	/* SMC request parameter a3 index */
	SMC_REQUEST_A3_INDEX = 0x01
};

/* SIP SVC response private data */
struct private_data_t {
	struct sip_svc_response response;
	struct k_sem smc_sem;
};

#endif
