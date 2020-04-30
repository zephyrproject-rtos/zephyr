/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_gptp, CONFIG_NET_GPTP_LOG_LEVEL);

#include <ptp_clock.h>
#include <net/gptp.h>

#include "gptp_messages.h"
#include "gptp_data_set.h"

#include "net_private.h"

static sys_slist_t phase_dis_callbacks;

void gptp_register_phase_dis_cb(struct gptp_phase_dis_cb *phase_dis,
				gptp_phase_dis_callback_t cb)
{
	sys_slist_find_and_remove(&phase_dis_callbacks, &phase_dis->node);
	sys_slist_prepend(&phase_dis_callbacks, &phase_dis->node);

	phase_dis->cb = cb;
}

void gptp_unregister_phase_dis_cb(struct gptp_phase_dis_cb *phase_dis)
{
	sys_slist_find_and_remove(&phase_dis_callbacks, &phase_dis->node);
}

void gptp_call_phase_dis_cb(void)
{
	struct gptp_global_ds *global_ds;
	sys_snode_t *sn, *sns;
	uint8_t *gm_id;

	global_ds = GPTP_GLOBAL_DS();
	gm_id = &global_ds->gm_priority.root_system_id.grand_master_id[0];

	SYS_SLIST_FOR_EACH_NODE_SAFE(&phase_dis_callbacks, sn, sns) {
		struct gptp_phase_dis_cb *phase_dis =
			CONTAINER_OF(sn, struct gptp_phase_dis_cb, node);

		phase_dis->cb(gm_id,
			      &global_ds->gm_time_base_indicator,
			      &global_ds->clk_src_last_gm_phase_change,
			      &global_ds->clk_src_last_gm_freq_change);
	}
}

int gptp_event_capture(struct net_ptp_time *slave_time, bool *gm_present)
{
	int port, key;
	const struct device *clk;

	key = irq_lock();
	*gm_present =  GPTP_GLOBAL_DS()->gm_present;

	for (port = GPTP_PORT_START; port <= GPTP_PORT_END; port++) {
		/* Get first available clock, or slave clock if GM present. */
		if (!*gm_present || (GPTP_GLOBAL_DS()->selected_role[port] ==
				     GPTP_PORT_SLAVE)) {
			clk = net_eth_get_ptp_clock(GPTP_PORT_IFACE(port));
			if (clk) {
				ptp_clock_get(clk, slave_time);
				irq_unlock(key);
				return 0;
			}
		}
	}

	irq_unlock(key);
	return -EAGAIN;
}

char *gptp_sprint_clock_id(const uint8_t *clk_id, char *output, size_t output_len)
{
	return net_sprint_ll_addr_buf(clk_id, 8, output, output_len);
}

void gptp_clk_src_time_invoke(struct gptp_clk_src_time_invoke_params *arg)
{
	struct gptp_clk_master_sync_rcv_state *state;

	state = &GPTP_STATE()->clk_master_sync_receive;

	memcpy(&state->rcvd_clk_src_req, arg,
	       sizeof(struct gptp_clk_src_time_invoke_params));

	state->rcvd_clock_source_req = true;
}
