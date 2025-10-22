/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RA_COLD_START_H_
#define ZEPHYR_SOC_RENESAS_RA_COLD_START_H_

#include <zephyr/device.h>
#include <bsp_api.h>
#include "battery_backup.h"

bool is_power_on_reset_happen(void);
void cold_start_handler(void);

#endif /* ZEPHYR_SOC_RENESAS_RA_COLD_START_H_ */
