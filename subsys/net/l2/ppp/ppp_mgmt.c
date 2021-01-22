/*
 * Copyright (c) 2020 Lemonbeat GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/ppp.h>

void ppp_mgmt_raise_carrier_on_event(struct net_if *iface)
{
	net_mgmt_event_notify(NET_EVENT_PPP_CARRIER_ON, iface);
}

void ppp_mgmt_raise_carrier_off_event(struct net_if *iface)
{
	net_mgmt_event_notify(NET_EVENT_PPP_CARRIER_OFF, iface);
}

void ppp_mgmt_raise_phase_running_event(struct net_if *iface)
{
	net_mgmt_event_notify(NET_EVENT_PPP_PHASE_RUNNING, iface);
}

void ppp_mgmt_raise_phase_dead_event(struct net_if *iface)
{
	net_mgmt_event_notify(NET_EVENT_PPP_PHASE_DEAD, iface);
}
