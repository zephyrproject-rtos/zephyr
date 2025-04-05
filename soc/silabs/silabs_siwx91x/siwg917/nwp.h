/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWG917_NWP_H
#define SIWG917_NWP_H

#include "sl_wifi.h"

#define SIWX91X_INTERFACE_MASK (0x03)

typedef struct siwx91x_boot_config {
	uint8_t oper_mode;
	bool hidden_ssid;
	uint8_t max_num_sta;
} siwx91x_boot_config_t;

/**
 * @brief Switch the Wi-Fi operating modes.
 *
 * This function switches the mode. If the requested mode is already active,
 * no action is performed. Otherwise, it reinitializes the Wi-Fi subsystem to
 * apply the new mode with configured features.
 *
 * @param[in] boot_cfg  Wi-Fi boot config feature:
 *
 * @return 0 on success, or a negative error code on failure:
 */
int siwx91x_nwp_mode_switch(siwx91x_boot_config_t boot_cfg);

#endif
