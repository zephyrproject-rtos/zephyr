/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWG917_NWP_H
#define SIWG917_NWP_H

#include "sl_wifi.h"

#define SIWX91X_INTERFACE_MASK (0x03)

/**
 * @brief Retrieve the default configuration based on the Wi-Fi operating mode.
 *
 * This function populates the provided configuration structure with the appropriate
 * settings for the specified Wi-Fi operating mode (Client or Access Point).
 *
 * @param[in]  wifi_oper_mode   Wi-Fi operating mode.
 * @param[out] get_config       Pointer to the structure where the configuration will be stored.
 *
 * @return 0 on success, -EINVAL if an invalid mode is provided.
 */
int siwg91x_get_nwp_config(int wifi_oper_mode, sl_wifi_device_configuration_t *get_config);

/**
 * @brief Switch the Wi-Fi operating modes.
 *
 * This function switches the mode. If the requested mode is already active,
 * no action is performed. Otherwise, it reinitializes the Wi-Fi subsystem to
 * apply the new mode.
 *
 * @param[in] oper_mode  Desired Wi-Fi operating mode:
 *
 * @return 0 on success, or a negative error code on failure:
 */
int siwx91x_nwp_mode_switch(uint8_t oper_mode);

#endif
