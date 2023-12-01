/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap_service_sample);

#include <zephyr/net/coap_mgmt.h>
#include <zephyr/net/coap_service.h>

#define COAP_EVENTS_SET (NET_EVENT_COAP_OBSERVER_ADDED | NET_EVENT_COAP_OBSERVER_REMOVED |	\
			 NET_EVENT_COAP_SERVICE_STARTED | NET_EVENT_COAP_SERVICE_STOPPED)

void coap_event_handler(uint32_t mgmt_event, struct net_if *iface,
			void *info, size_t info_length, void *user_data)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(user_data);

	switch (mgmt_event) {
	case NET_EVENT_COAP_OBSERVER_ADDED:
		LOG_INF("CoAP observer added");
		break;
	case NET_EVENT_COAP_OBSERVER_REMOVED:
		LOG_INF("CoAP observer removed");
		break;
	case NET_EVENT_COAP_SERVICE_STARTED:
		if (info != NULL && info_length == sizeof(struct net_event_coap_service)) {
			struct net_event_coap_service *net_event = info;

			LOG_INF("CoAP service %s started", net_event->service->name);
		} else {
			LOG_INF("CoAP service started");
		}
		break;
	case NET_EVENT_COAP_SERVICE_STOPPED:
		if (info != NULL && info_length == sizeof(struct net_event_coap_service)) {
			struct net_event_coap_service *net_event = info;

			LOG_INF("CoAP service %s stopped", net_event->service->name);
		} else {
			LOG_INF("CoAP service stopped");
		}
		break;
	}
}

NET_MGMT_REGISTER_EVENT_HANDLER(coap_events, COAP_EVENTS_SET, coap_event_handler, NULL);
