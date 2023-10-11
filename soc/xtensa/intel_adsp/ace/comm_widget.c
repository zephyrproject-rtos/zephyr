/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "comm_widget.h"

/*
 * Operation codes
 */
#define CW_OPCODE_CRRD 0x06
#define CW_OPCODE_CRWR 0x07

void cw_sb_write(uint32_t dest, uint32_t func, uint16_t address, uint32_t data)
{
	cw_upstream_set_attr(dest, func, CW_OPCODE_CRWR, 0, 0);
	cw_upstream_set_address16(address);
	cw_upstream_set_data(data);
	cw_upstream_do_pw();
}
