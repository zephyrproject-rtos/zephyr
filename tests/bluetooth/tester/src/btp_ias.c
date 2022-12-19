#include <zephyr/bluetooth/services/ias.h>

#include "btp/btp.h"
#include "zephyr/sys/byteorder.h"

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_ias
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define CONTROLLER_INDEX 0

static int g_ias_alert_lvl;

/* Immediate Alert Service */
static void alert_stop(void)
{
	struct ias_alert_action_ev ev;
	g_ias_alert_lvl = 0;

	ev.alert_lvl = sys_cpu_to_le32(g_ias_alert_lvl);

	tester_send(BTP_SERVICE_ID_IAS, IAS_EV_OUT_ALERT_ACTION,
				CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void alert_start(void)
{
	struct ias_alert_action_ev ev;
	g_ias_alert_lvl = 1;

	ev.alert_lvl = sys_cpu_to_le32(g_ias_alert_lvl);

	tester_send(BTP_SERVICE_ID_IAS, IAS_EV_OUT_ALERT_ACTION,
				CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void alert_high_start(void)
{
	struct ias_alert_action_ev ev;
	g_ias_alert_lvl = 2;

	ev.alert_lvl = sys_cpu_to_le32(g_ias_alert_lvl);

	tester_send(BTP_SERVICE_ID_IAS, IAS_EV_OUT_ALERT_ACTION,
				CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
		.no_alert = alert_stop,
		.mild_alert = alert_start,
		.high_alert = alert_high_start,
};
