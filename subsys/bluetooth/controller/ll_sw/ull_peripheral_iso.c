/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_peripheral_iso
#include "common/log.h"
#include "hal/debug.h"

uint8_t ll_cis_accept(uint16_t handle)
{
	ARG_UNUSED(handle);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cis_reject(uint16_t handle, uint8_t reason)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(reason);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

int ull_peripheral_iso_init(void)
{
	return 0;
}

int ull_peripheral_iso_reset(void)
{
	return 0;
}
