/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <cybsp.h>
#include <sys/__assert.h>
#include <init.h>



static int board_init(const struct device *dev)
{
	cy_rslt_t result;

	/* Initializes the board support package */
	result = cybsp_init();

	__ASSERT(result == CY_RSLT_SUCCESS,
		 "cybsp_init failed [error %d]", result);

	return ((result == CY_RSLT_SUCCESS) ? 0 : -1);
}

SYS_INIT(board_init, PRE_KERNEL_1, 2);
