/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RA_BATTERY_BACKUP_H_
#define ZEPHYR_SOC_RENESAS_RA_BATTERY_BACKUP_H_

#include <zephyr/device.h>
#include <bsp_api.h>

bool is_backup_domain_reset_happen(void);
void battery_backup_init(void);

#endif /* ZEPHYR_SOC_RENESAS_RA_BATTERY_BACKUP_H_ */
