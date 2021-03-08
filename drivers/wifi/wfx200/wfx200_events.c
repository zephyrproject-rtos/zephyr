/**
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME wifi_wfx200_event
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL

#include <sl_wfx.h>
#undef BIT

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr.h>
#include <net/wifi_mgmt.h>

#include <wfx200_internal.h>

void wfx200_event_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	struct wfx200_data *context = p1;
	struct wfx200_queue_event *event;

	k_thread_name_set(NULL, "wfx200_ev");

	while (true) {
		event = k_queue_get(&context->event_queue, K_FOREVER);

		switch (event->ev) {
		case WFX200_CONNECT_EVENT:
			if (context->state == WFX200_STATE_STA_MODE) {
				net_if_set_link_addr(context->iface, context->sl_context.mac_addr_0.octet,
						     sizeof(context->sl_context.mac_addr_0.octet),
						     NET_LINK_ETHERNET);
				net_eth_carrier_on(context->iface);
				wifi_mgmt_raise_connect_result_event(context->iface, 0);
			}
			if (IS_ENABLED(CONFIG_WIFI_WFX200_SLEEP) &&
			    !(context->sl_context.state & SL_WFX_AP_INTERFACE_UP)) {
				/* Enable the power save */
				sl_wfx_set_power_mode(IS_ENABLED(WIFI_WFX200_PM_MODE_PS) ?
						      WFM_PM_MODE_PS :
						      WFM_PM_MODE_DTIM,
						      IS_ENABLED(WIFI_WFX200_PM_POLL_STRATEGY_UAPSD) ?
						      WFM_PM_POLL_UAPSD :
						      WFM_PM_POLL_FAST_PS,
						      CONFIG_WIFI_WFX200_DTIM_SKIP_INTERVAL
						      );
				sl_wfx_enable_device_power_save();
			}
			break;
		case WFX200_CONNECT_FAILED_EVENT:
			if (context->state == WFX200_STATE_STA_MODE) {
				wifi_mgmt_raise_connect_result_event(context->iface, 1);
				context->state = WFX200_STATE_INTERFACE_INITIALIZED;
			}
			break;
		case WFX200_DISCONNECT_EVENT:
			if (context->state == WFX200_STATE_STA_MODE) {
				net_eth_carrier_off(context->iface);
				wifi_mgmt_raise_disconnect_result_event(context->iface, 0);
				context->state = WFX200_STATE_INTERFACE_INITIALIZED;
			}
			break;
		case WFX200_AP_START_EVENT:
			if (context->state == WFX200_STATE_AP_MODE) {
				net_if_set_link_addr(context->iface, context->sl_context.mac_addr_1.octet,
						     sizeof(context->sl_context.mac_addr_1.octet),
						     NET_LINK_ETHERNET);
				net_eth_carrier_on(context->iface);
			} else {
				LOG_WRN("AP interface came up but interface wasn't set up correctly");
			}
			if (IS_ENABLED(CONFIG_WIFI_WFX200_SLEEP)) {
				/* Power save always disabled when SoftAP mode enabled */
				sl_wfx_set_power_mode(WFM_PM_MODE_ACTIVE,
						      IS_ENABLED(WIFI_WFX200_PM_POLL_STRATEGY_UAPSD) ?
						      WFM_PM_POLL_UAPSD :
						      WFM_PM_POLL_FAST_PS,
						      0
						      );
				sl_wfx_disable_device_power_save();
			}
			break;
		case WFX200_AP_START_FAILED_EVENT:
			if (context->state == WFX200_STATE_AP_MODE) {
				context->state = WFX200_STATE_INTERFACE_INITIALIZED;
			}
			break;
		case WFX200_AP_STOP_EVENT:
			if (context->state == WFX200_STATE_AP_MODE) {
				net_eth_carrier_off(context->iface);
				context->state = WFX200_STATE_INTERFACE_INITIALIZED;
			}
			if (IS_ENABLED(CONFIG_WIFI_WFX200_SLEEP) &&
			    context->sl_context.state & SL_WFX_STA_INTERFACE_CONNECTED) {
				/* Enable the power save */
				sl_wfx_set_power_mode(IS_ENABLED(WIFI_WFX200_PM_MODE_PS) ?
						      WFM_PM_MODE_PS :
						      WFM_PM_MODE_DTIM,
						      IS_ENABLED(WIFI_WFX200_PM_POLL_STRATEGY_UAPSD) ?
						      WFM_PM_POLL_UAPSD :
						      WFM_PM_POLL_FAST_PS,
						      CONFIG_WIFI_WFX200_DTIM_SKIP_INTERVAL
						      );
				sl_wfx_enable_device_power_save();
			}
			break;
		}

		k_heap_free(&context->heap, event);
		event = NULL;
	}
}
