/* btp_ias.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/bluetooth/services/ias.h"
#include <stdint.h>

/* events */
#define IAS_EV_OUT_ALERT_ACTION       0x80
struct ias_alert_action_ev {
	uint8_t alert_lvl;
} __packed;
