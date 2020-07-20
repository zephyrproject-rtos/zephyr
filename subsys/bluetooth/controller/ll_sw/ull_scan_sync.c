/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <bluetooth/hci.h>

uint8_t ll_scan_sync_create(uint8_t options, uint8_t sid, uint8_t adv_addr_type,
			    uint8_t *adv_addr, uint16_t skip,
			    uint16_t sync_timeout, uint8_t sync_cte_type)
{
	/* TODO: */
	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_scan_sync_create_cancel(void)
{
	/* TODO: */
	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_scan_sync_terminate(uint16_t handle)
{
	/* TODO: */
	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_scan_sync_recv_enable(uint16_t handle, uint8_t enable)
{
	/* TODO: */
	return BT_HCI_ERR_CMD_DISALLOWED;
}
