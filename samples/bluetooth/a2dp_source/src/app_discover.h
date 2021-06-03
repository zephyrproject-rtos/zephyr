/*
 * Copyright 2020 - 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __APP_DISCOVER_H__
#define __APP_DISCOVER_H__

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * API
 ******************************************************************************/

void app_discover(void);

uint8_t *app_get_addr(uint8_t select);

#endif /* __APP_DISCOVER_H__ */
