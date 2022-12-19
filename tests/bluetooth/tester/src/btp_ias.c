/* btp_ias.c - Bluetooth IAS Server Tester */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/services/ias.h>

#include "btp/btp.h"
#include "zephyr/sys/byteorder.h"
#include <stdint.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_ias
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define CONTROLLER_INDEX 0

/* Immediate Alert Service */
static void alert_stop(void)
{
	struct ias_alert_action_ev ev;

	ev.alert_lvl = BT_IAS_ALERT_LVL_NO_ALERT;

	tester_send(BTP_SERVICE_ID_IAS, IAS_EV_OUT_ALERT_ACTION,
		    CONTROLLER_INDEX, (uint8_t *)&ev, sizeof(ev));
}

static void alert_start(void)
{
	struct ias_alert_action_ev ev;

	ev.alert_lvl = BT_IAS_ALERT_LVL_MILD_ALERT;

	tester_send(BTP_SERVICE_ID_IAS, IAS_EV_OUT_ALERT_ACTION,
		    CONTROLLER_INDEX, (uint8_t *)&ev.alert_lvl, sizeof(ev));
}

static void alert_high_start(void)
{
	struct ias_alert_action_ev ev;

	ev.alert_lvl = BT_IAS_ALERT_LVL_HIGH_ALERT;

	tester_send(BTP_SERVICE_ID_IAS, IAS_EV_OUT_ALERT_ACTION,
		    CONTROLLER_INDEX, (uint8_t *)&ev, sizeof(ev));
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.no_alert = alert_stop,
	.mild_alert = alert_start,
	.high_alert = alert_high_start,
};
