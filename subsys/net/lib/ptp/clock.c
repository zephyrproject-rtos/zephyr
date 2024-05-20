/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_clock, CONFIG_PTP_LOG_LEVEL);

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/slist.h>

#include "clock.h"
#include "ddt.h"
#include "port.h"

#define MIN_NSEC_TO_TIMEINTERVAL (0xFFFF800000000000ULL)
#define MAX_NSEC_TO_TIMEINTERVAL (0x00007FFFFFFFFFFFULL)

/**
 * @brief PTP Clock structure.
 */
struct ptp_clock {
	const struct device	    *phc;
	struct ptp_default_ds	    default_ds;
	struct ptp_current_ds	    current_ds;
	struct ptp_parent_ds	    parent_ds;
	struct ptp_time_prop_ds	    time_prop_ds;
	struct ptp_dataset	    dataset;
	struct ptp_foreign_tt_clock *best;
	sys_slist_t		    ports_list;
	struct zsock_pollfd	    pollfd[2 * CONFIG_PTP_NUM_PORTS];
	bool			    pollfd_valid;
	uint8_t			    time_src;
	struct {
		uint64_t	    t1;
		uint64_t	    t2;
		uint64_t	    t3;
		uint64_t	    t4;
	} timestamp;			/* latest timestamps in nanoseconds */
};

static struct ptp_clock clock = { 0 };
char str_clock_id[] = "FF:FF:FF:FF:FF:FF:FF:FF";

static int clock_generate_id(ptp_clk_id *clock_id, struct net_if *iface)
{
	struct net_linkaddr *addr = net_if_get_link_addr(iface);

	if (addr) {
		clock_id->id[0] = addr->addr[0];
		clock_id->id[1] = addr->addr[1];
		clock_id->id[2] = addr->addr[2];
		clock_id->id[3] = 0xFF;
		clock_id->id[4] = 0xFE;
		clock_id->id[5] = addr->addr[3];
		clock_id->id[6] = addr->addr[4];
		clock_id->id[7] = addr->addr[5];
		return 0;
	}
	return -1;
}

static const char *clock_id_str(ptp_clk_id *clock_id)
{
	uint8_t *cid = clock_id->id;

	snprintk(str_clock_id, sizeof(str_clock_id), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
		 cid[0],
		 cid[1],
		 cid[2],
		 cid[3],
		 cid[4],
		 cid[5],
		 cid[6],
		 cid[7]);

	return str_clock_id;
}

static ptp_timeinterval clock_ns_to_timeinterval(int64_t val)
{
	if (val < (int64_t)MIN_NSEC_TO_TIMEINTERVAL) {
		val = MIN_NSEC_TO_TIMEINTERVAL;
	} else if (val > (int64_t)MAX_NSEC_TO_TIMEINTERVAL) {
		val = MAX_NSEC_TO_TIMEINTERVAL;
	}

	return (uint64_t)val << 16;
}

const struct ptp_clock *ptp_clock_init(void)
{
	struct ptp_default_ds *dds = &clock.default_ds;
	struct ptp_current_ds *cds = &clock.current_ds;
	struct ptp_parent_ds *pds  = &clock.parent_ds;
	struct ptp_time_prop_ds *tpds = &clock.time_prop_ds;
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));

	clock.time_src = (enum ptp_time_src)PTP_TIME_SRC_INTERNAL_OSC;

	/* Initialize Default Dataset. */
	int ret = clock_generate_id(&dds->clk_id, iface);

	if (ret) {
		LOG_ERR("Couldn't assign Clock Identity.");
		return NULL;
	}

	dds->type = (enum ptp_clock_type)CONFIG_PTP_CLOCK_TYPE;
	dds->n_ports = 0;
	dds->time_receiver_only = IS_ENABLED(CONFIG_PTP_TIME_RECEIVER_ONLY) ? true : false;

	dds->clk_quality.class = dds->time_receiver_only ? 255 : 248;
	dds->clk_quality.accuracy = CONFIG_PTP_CLOCK_ACCURACY;
	/* 0xFFFF means that value has not been computed - IEEE 1588-2019 7.6.3.3 */
	dds->clk_quality.offset_scaled_log_variance = 0xFFFF;

	dds->max_steps_rm = 255;

	dds->priority1 = CONFIG_PTP_PRIORITY1;
	dds->priority2 = CONFIG_PTP_PRIORITY2;

	/* Initialize Parent Dataset. */
	memset(cds, 0, sizeof(struct ptp_current_ds));
	memcpy(&pds->port_id.clk_id, &cds->clk_id, sizeof(ptp_clk_id));
	memcpy(&pds->gm_id, &dds->clk_id, sizeof(ptp_clk_id));
	pds->port_id.port_number = 0;
	pds->gm_clk_quality = dds->clk_quality;
	pds->gm_priority1 = dds->priority1;
	pds->gm_priority2 = dds->priority2;

	tpds->current_utc_offset = 37; // IEEE 1588-2019 9.4
	tpds->time_src = clock.time_src;
	tpds->flags = 0;

	pds->obsreved_parent_offset_scaled_log_variance = 0xFFFF;
	pds->obsreved_parent_clk_phase_change_rate = 0x7FFFFFFF;
	/* Parent statistics haven't been measured - IEEE 1588-2019 7.6.4.2 */
	pds->stats = false;

	clock.phc = net_eth_get_ptp_clock(iface);
	if (!clock.phc) {
		LOG_ERR("Couldn't get PTP HW Clock for the interface.");
		return NULL;
	}

	sys_slist_init(&clock.ports_list);
	LOG_DBG("PTP Clock %s initialized", clock_id_str(&dds->clk_id));
	return &clock;
}

void ptp_clock_synchronize(uint64_t ingress, uint64_t egress)
{
	int64_t offset;
	uint64_t delay = clock.current_ds.mean_delay >> 16;

	clock.timestamp.t1 = egress;
	clock.timestamp.t2 = ingress;

	if (!clock.current_ds.mean_delay) {
		return;
	}

	offset = clock.timestamp.t2 - clock.timestamp.t1 - delay;

	/* If diff is too big, clock needs to be set first. */
	if (offset > NSEC_PER_SEC || offset < -NSEC_PER_SEC) {
		struct net_ptp_time current;

		LOG_WRN("Clock offset exceeds 1 second.");

		ptp_clock_get(clock.phc, &current);

		current.second -= (uint64_t)(offset / NSEC_PER_SEC);
		current.nanosecond -= (uint32_t)(offset % NSEC_PER_SEC);

		ptp_clock_set(clock.phc, &current);
		return;
	}

	LOG_DBG("Offset %lldns", offset);
	clock.current_ds.offset_from_tt = clock_ns_to_timeinterval(offset);

	ptp_clock_adjust(clock.phc, offset);
}

void ptp_clock_delay(uint64_t egress, uint64_t ingress)
{
	int64_t delay;

	clock.timestamp.t3 = egress;
	clock.timestamp.t4 = ingress;

	delay = ((clock.timestamp.t2 - clock.timestamp.t3) +
		 (clock.timestamp.t4 - clock.timestamp.t1)) / 2;

	LOG_DBG("Delay %lldns", delay);
	clock.current_ds.mean_delay = clock_ns_to_timeinterval(delay);
}

const struct ptp_default_ds *ptp_clock_default_ds(void)
{
	return &clock.default_ds;
}

const struct ptp_parent_ds *ptp_clock_parent_ds(void)
{
	return &clock.parent_ds;
}

void ptp_clock_pollfd_invalidate(void)
{
	clock.pollfd_valid = false;
}

void ptp_clock_port_add(struct ptp_port *port)
{
	clock.default_ds.n_ports++;
	sys_slist_append(&clock.ports_list, &port->node);
}

const struct ptp_foreign_tt_clock *ptp_clock_best_time_transmitter(void)
{
	return clock.best;
}
