/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shim for sl_btctrl_hci_reset API
 */

#include <stdbool.h>

bool sl_btctrl_hci_reset_reason_is_sys_reset(void)
{
	/* If this function returns true, the LL will emit command complete for HCI Reset during
	 * init. This only makes sense when the LL runs on a separate device from the host stack.
	 * Always return false.
	 */
	return false;
}

void sl_btctrl_hci_reset(void)
{
	/* This function works as a callback during processing of the HCI Reset command, and allows
	 * for custom processing (such as fully resetting the device). This only makes sense when
	 * the LL runs on a separate device from the host stack. Do nothing.
	 */
}
