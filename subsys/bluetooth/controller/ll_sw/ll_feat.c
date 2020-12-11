/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ll_feat
#include "common/log.h"
#include "hal/debug.h"

uint8_t ll_set_host_feature(uint8_t bit_number, uint8_t bit_value)
{
	ARG_UNUSED(bit_number);
	ARG_UNUSED(bit_value);

	return BT_HCI_ERR_CMD_DISALLOWED;
}
