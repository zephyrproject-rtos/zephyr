/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_LORA_BASICS_MODEM_LBM_LORAWAN_H_
#define ZEPHYR_SUBSYS_LORAWAN_LORA_BASICS_MODEM_LBM_LORAWAN_H_

#include <smtc_modem_api.h>

#define LBM_STACK_ID 0

void lbm_lorawan_event(const smtc_modem_event_t *event);

#endif /* ZEPHYR_SUBSYS_LORAWAN_LORA_BASICS_MODEM_LBM_LORAWAN_H_ */
