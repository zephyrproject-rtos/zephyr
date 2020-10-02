/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sync_iso
#include "common/log.h"
#include "hal/debug.h"

uint8_t ll_big_sync_create(uint8_t big_handle, uint16_t sync_handle,
			   uint8_t encryption, uint8_t *bcode, uint8_t mse,
			   uint16_t sync_timeout, uint8_t num_bis,
			   uint8_t *bis)
{
	/* TODO: Implement */
	ARG_UNUSED(big_handle);
	ARG_UNUSED(sync_handle);
	ARG_UNUSED(encryption);
	ARG_UNUSED(bcode);
	ARG_UNUSED(mse);
	ARG_UNUSED(sync_timeout);
	ARG_UNUSED(num_bis);
	ARG_UNUSED(bis);

	return BT_HCI_ERR_CMD_DISALLOWED;
}


uint8_t ll_big_sync_terminate(uint8_t big_handle)
{
	/* TODO: Implement */
	ARG_UNUSED(big_handle);

	return BT_HCI_ERR_CMD_DISALLOWED;
}
