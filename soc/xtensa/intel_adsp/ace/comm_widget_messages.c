/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "comm_widget.h"
#include "pmc_interface.h"

/*
 * Report number of used HP-SRAM memory banks to the PMC, unit is 32 KB.
 */
int adsp_comm_widget_pmc_send_ipc(uint16_t banks)
{
	if (!cw_upstream_ready())
		return -EBUSY;

	uint32_t iface = FIELD_PREP(CW_PMC_IPC_OP_CODE, CW_PMC_OPC_SRAM_CONFIG) |
			 FIELD_PREP(CW_PMC_IPC_SRAM_USED_BANKS, banks) |
			 CW_PMC_IPC_BUSY;

	cw_sb_write(CW_PMC_DESTID_VALUE, 0, CW_PMC_MAILBOX3_INTERFACE_ADDRESS, iface);

	cw_upstream_wait_for_sent();
	return 0;
}
