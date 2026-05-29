/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shim for sl_btctrl_hci_reset API
 */

#include <zephyr/sys/util.h>
#include <stdint.h>

void sl_btctrl_hci_reset(void)
{
	/* This function works as a callback during processing of the HCI Reset command, and allows
	 * for custom processing (such as fully resetting the device). This only makes sense when
	 * the LL runs on a separate device from the host stack. Do nothing.
	 */
}

void sl_btctrl_reset_set_custom_reason(uint32_t reason)
{
	ARG_UNUSED(reason);
}

uint32_t sl_btctrl_reset_get_custom_reason(void)
{
	return 0;
}

void sl_btctrl_reset_clear_custom_reason(void)
{
}
