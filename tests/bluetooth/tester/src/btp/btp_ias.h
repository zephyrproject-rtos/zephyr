/* btp_ias.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

/* events */
#define BTP_IAS_EV_OUT_ALERT_ACTION 0x80
struct btp_ias_alert_action_ev {
	uint8_t alert_lvl;
} __packed;
