/* btp_ias.c - Bluetooth IAS Server Tester */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/services/ias.h>

#include "btp/btp.h"
#include <zephyr/sys/byteorder.h>
#include <stdint.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_ias
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static bool initialized;


/* Immediate Alert Service */
static void alert_stop(void)
{
	struct btp_ias_alert_action_ev ev;

	if (!initialized) {
		return;
	}

	ev.alert_lvl = BT_IAS_ALERT_LVL_NO_ALERT;

	tester_event(BTP_SERVICE_ID_IAS, BTP_IAS_EV_OUT_ALERT_ACTION,
		    (uint8_t *)&ev, sizeof(ev));
}

static void alert_start(void)
{
	struct btp_ias_alert_action_ev ev;

	if (!initialized) {
		return;
	}

	ev.alert_lvl = BT_IAS_ALERT_LVL_MILD_ALERT;

	tester_event(BTP_SERVICE_ID_IAS, BTP_IAS_EV_OUT_ALERT_ACTION, &ev, sizeof(ev));
}

static void alert_high_start(void)
{
	struct btp_ias_alert_action_ev ev;

	if (!initialized) {
		return;
	}

	ev.alert_lvl = BT_IAS_ALERT_LVL_HIGH_ALERT;

	tester_event(BTP_SERVICE_ID_IAS, BTP_IAS_EV_OUT_ALERT_ACTION, &ev, sizeof(ev));
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.no_alert = alert_stop,
	.mild_alert = alert_start,
	.high_alert = alert_high_start,
};

uint8_t tester_init_ias(void)
{
	initialized = true;

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_ias(void)
{
	initialized = false;

	return BTP_STATUS_SUCCESS;
}
