/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/pm/evtc.h>
#include <timeout_q.h>

LOG_MODULE_REGISTER(pm_evt, CONFIG_PM_EVTC_LOG_LEVEL);

#define UPD_UPTKS_NONE INT64_MAX

static uint32_t get_lat_ns_max(const struct pm_evtc *evtc)
{
	return evtc->lats_ns[0];
}

static uint32_t floor_lat_ns(const struct pm_evtc *evtc, uint32_t max_lat_ns)
{
	uint32_t lat_ns;
	uint8_t i;

	i = 0;
	do {
		lat_ns = evtc->lats_ns[i];

		if (lat_ns <= max_lat_ns) {
			break;
		}

		i++;
	} while (i < evtc->lats_ns_size);

	return lat_ns;
}

static void add_evt(const struct pm_evtc *evtc, struct pm_evt *evt)
{
	struct pm_evtc_rt *rt = evtc->rt;

	sys_slist_append(&rt->evt_list, &evt->node);
}

static void remove_evt(const struct pm_evtc *evtc, struct pm_evt *evt)
{
	struct pm_evtc_rt *rt = evtc->rt;

	(void)sys_slist_find_and_remove(&rt->evt_list, &evt->node);
}

static int64_t get_evt_act_uptks(const struct pm_evt *evt)
{
	const struct pm_evtc *evtc = evt->evtc;

	return evt->uptks - evtc->req_lat_tks - 1;
}

static bool evt_is_act(const struct pm_evt *evt, int64_t uptks)
{
	const struct pm_evtc *evtc = evt->evtc;

	return get_evt_act_uptks(evt) <= uptks + evtc->req_lat_tks + 1;
}

static bool lat_req_is_required(const struct pm_evtc *evtc, uint32_t lat_ns)
{
	struct pm_evtc_rt *rt = evtc->rt;

	return rt->req_lat_ns != lat_ns;
}

static bool lat_req_is_idle(const struct pm_evtc *evtc, int64_t uptks)
{
	struct pm_evtc_rt *rt = evtc->rt;

	if (rt->req_uptks == -1) {
		return false;
	}

	return rt->req_uptks < uptks - evtc->req_lat_tks;
}

static int64_t get_lat_req_idle_uptks(const struct pm_evtc *evtc)
{
	struct pm_evtc_rt *rt = evtc->rt;

	return rt->req_uptks + evtc->req_lat_tks + 1;
}

static void req_lat(const struct pm_evtc *evtc, int64_t lat_ns, int64_t ticks)
{
	struct pm_evtc_rt *rt = evtc->rt;

	rt->req_lat_ns = lat_ns;
	rt->req_uptks = ticks;

	evtc->req(evtc->dev, rt->req_lat_ns);
}

static bool upd_to_is_required(int64_t upd_uptks)
{
	return upd_uptks != UPD_UPTKS_NONE;
}

static void upd(const struct pm_evtc *evtc, int64_t uptks);

static void upd_to_handler(struct _timeout *upd_to)
{
	struct pm_evtc_rt *rt = CONTAINER_OF(upd_to, struct pm_evtc_rt, upd_to);
	k_spinlock_key_t key;

	key = k_spin_lock(&rt->lock);
	upd(rt->evtc, k_uptime_ticks());
	k_spin_unlock(&rt->lock, key);
}

static void set_upd_to(const struct pm_evtc *evtc, int64_t upd_uptks)
{
	struct pm_evtc_rt *rt = evtc->rt;

	(void)z_abort_timeout(&rt->upd_to);
	z_add_timeout(&rt->upd_to, upd_to_handler, K_TIMEOUT_ABS_TICKS(upd_uptks));
}

static void clear_upd_to(const struct pm_evtc *evtc)
{
	struct pm_evtc_rt *rt = evtc->rt;

	(void)z_abort_timeout(&rt->upd_to);
}

static bool evt_lat_is_act(const struct pm_evt *evt, uint32_t lat_ns)
{
	const struct pm_evtc *evtc = evt->evtc;
	struct pm_evtc_rt *rt = evtc->rt;

	return rt->req_lat_ns <= lat_ns;
}

static int64_t get_evt_eff_uptks(const struct pm_evt *evt,
				 int64_t uptks)
{
	const struct pm_evtc *evtc = evt->evtc;

	if (evt_lat_is_act(evt, evt->lat_ns)) {
		return MAX(evt->uptks, uptks);
	}

	if (lat_req_is_idle(evtc, uptks)) {
		return MAX(evt->uptks, uptks + evtc->req_lat_tks + 1);
	}

	return MAX(evt->uptks, get_lat_req_idle_uptks(evtc) + evtc->req_lat_tks + 1);
}

static void upd(const struct pm_evtc *evtc, int64_t uptks)
{
	struct pm_evtc_rt *rt = evtc->rt;
	uint32_t req_lat_ns;
	int64_t upd_uptks;
	struct pm_evt *evt;

	req_lat_ns = get_lat_ns_max(evtc);
	upd_uptks = UPD_UPTKS_NONE;

	SYS_SLIST_FOR_EACH_CONTAINER(&rt->evt_list, evt, node) {
		if (evt_is_act(evt, uptks)) {
			req_lat_ns = MIN(evt->lat_ns, req_lat_ns);
		} else {
			upd_uptks = MIN(get_evt_act_uptks(evt), upd_uptks);
		}
	}

	if (lat_req_is_required(evtc, req_lat_ns)) {
		if (lat_req_is_idle(evtc, uptks)) {
			req_lat(evtc, req_lat_ns, uptks);
		} else {
			upd_uptks = get_lat_req_idle_uptks(evtc);
		}
	}

	if (upd_to_is_required(upd_uptks)) {
		set_upd_to(evtc, upd_uptks);
	} else {
		clear_upd_to(evtc);
	}
}

void pm_evtc_init(const struct pm_evtc *evtc)
{
	req_lat(evtc, get_lat_ns_max(evtc), k_uptime_ticks());
}

int64_t pm_evtc_schedule_evt(const struct pm_evtc *evtc,
			     struct pm_evt *evt,
			     uint32_t max_latency_ns,
			     int64_t uptime_ticks)
{
	struct pm_evtc_rt *rt = evtc->rt;
	k_spinlock_key_t key;
	int64_t uptks;
	int64_t eff_uptks;

	evt->evtc = evtc;
	evt->lat_ns = floor_lat_ns(evtc, max_latency_ns);
	evt->uptks = uptime_ticks;

	key = k_spin_lock(&rt->lock);
	uptks = k_uptime_ticks();
	eff_uptks = get_evt_eff_uptks(evt, uptks);
	add_evt(evtc, evt);
	upd(evtc, uptks);
	k_spin_unlock(&rt->lock, key);

	return eff_uptks;
}

int64_t pm_evtc_reschedule_evt(struct pm_evt *evt, int64_t uptime_ticks)
{
	const struct pm_evtc *evtc = evt->evtc;
	struct pm_evtc_rt *rt = evtc->rt;
	k_spinlock_key_t key;
	int64_t uptks;
	int64_t eff_uptks;

	key = k_spin_lock(&rt->lock);
	evt->uptks = uptime_ticks;
	uptks = k_uptime_ticks();
	eff_uptks = get_evt_eff_uptks(evt, uptks);
	upd(evt->evtc, k_uptime_ticks());
	k_spin_unlock(&rt->lock, key);

	return eff_uptks;
}

int64_t pm_evtc_request_evt(const struct pm_evtc *evtc,
			    struct pm_evt *evt,
			    uint32_t max_latency_ns)
{
	return pm_evtc_schedule_evt(evtc, evt, max_latency_ns, 0);
}

void pm_evtc_release_evt(struct pm_evt *evt)
{
	const struct pm_evtc *evtc = evt->evtc;
	struct pm_evtc_rt *rt = evtc->rt;
	k_spinlock_key_t key;

	key = k_spin_lock(&rt->lock);
	remove_evt(evtc, evt);
	upd(evtc, k_uptime_ticks());
	k_spin_unlock(&rt->lock, key);
}
