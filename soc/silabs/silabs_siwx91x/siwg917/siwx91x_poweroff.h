/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_POWEROFF_H
#define SIWX91X_POWEROFF_H

/**
 * @brief Put the NWP into deep sleep without RAM retention.
 *
 * Requests the shutdown power profile from the Network Processor so that a
 * subsequent sys_poweroff() only has to perform M4-local register work.
 *
 * Reaching the NWP requires a command round trip handled by the WiSeConnect
 * command engine thread, so this function blocks and must be called from
 * thread context with the scheduler running. It cannot be called from
 * z_sys_poweroff(), from an ISR, or with interrupts locked.
 *
 * The Wi-Fi interface must not be associated when this is called. Issue
 * NET_REQUEST_WIFI_DISCONNECT and wait for the disconnect event first.
 *
 * Once this returns successfully the NWP is no longer usable: the WiSeConnect
 * driver marks the device uninitialized, and recovering requires a full
 * sl_wifi_init(). The only supported continuation is sys_poweroff().
 *
 * @return 0 on success, -ENODEV if the NWP is not initialized, -EIO if the
 *         NWP rejected the request.
 */
int siwx91x_nwp_prepare_poweroff(void);

#endif /* SIWX91X_POWEROFF_H */
