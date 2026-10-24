/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Layer 3 (Network) — routes packets between L2 and L4.
 * For RX (.ind): looks up TSAP from group address table, then dispatches to L4.
 * For TX (.req): sets address type and delegates to knx_l_data_req.
 * For CON: propagates confirmation upward to L4.
 */

#include "layer3_network.h"
#include "layer2_data_link.h"
#include "layer4_transport.h"
#include "object_address_table.h"
#include "object_device.h"
#include <zephyr/knx/knx_pkt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_l3, CONFIG_KNX_STACK_LOG_LEVEL);

/* ============================================================================
 * N_Data_Individual service
 * ============================================================================
 */

void knx_n_data_individual_ind(struct knx_pkt *pkt)
{
	LOG_DBG("ind src=" KNX_ADDR_FMT " dst=" KNX_ADDR_FMT, KNX_ADDR_VAL(pkt->src),
		KNX_ADDR_VAL(pkt->dst));
	knx_t_data_individual_ind(pkt);
}

void knx_n_data_individual_con(struct knx_pkt *pkt)
{
	knx_t_data_individual_con(pkt);
}

void knx_n_data_individual_req(struct knx_pkt *pkt)
{
	pkt->addr_type = KNX_ADDR_TYPE_INDIVIDUAL;
	knx_l_data_req(pkt);
}

/* ============================================================================
 * N_Data_Group service
 * ============================================================================
 */

void knx_n_data_group_ind(struct knx_pkt *pkt)
{
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L3)
	LOG_DBG("ind src=" KNX_ADDR_FMT " dst=" KNX_GROUP_FMT, KNX_ADDR_VAL(pkt->src),
		KNX_GROUP_VAL(pkt->dst));
#endif

	uint16_t tsap = address_table_get_tsap(pkt->dst);

	if (tsap == 0xFFFF) {
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L3)
		LOG_DBG("group address not in table, dropping");
#endif
		knx_pkt_free(pkt);
		return;
	}

	pkt->tsap = tsap;
	knx_t_data_group_ind(pkt);
}

void knx_n_data_group_con(struct knx_pkt *pkt)
{
	knx_t_data_group_con(pkt);
}

void knx_n_data_group_req(struct knx_pkt *pkt)
{
	pkt->addr_type = KNX_ADDR_TYPE_GROUP;
	knx_l_data_req(pkt);
}

/* ============================================================================
 * N_Data_Broadcast service
 * ============================================================================
 */

void knx_n_data_broadcast_ind(struct knx_pkt *pkt)
{
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L3)
	LOG_DBG(".");
#endif
	knx_t_data_broadcast_ind(pkt);
}

void knx_n_data_broadcast_con(struct knx_pkt *pkt)
{
	knx_t_data_broadcast_con(pkt);
}

void knx_n_data_broadcast_req(struct knx_pkt *pkt)
{
	pkt->addr_type = KNX_ADDR_TYPE_GROUP;
	pkt->dst = 0;
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L3)
	LOG_DBG(".");
#endif
	knx_l_data_req(pkt);
}

/* ============================================================================
 * N_Data_SystemBroadcast service
 * ============================================================================
 */

void knx_n_data_system_broadcast_ind(struct knx_pkt *pkt)
{
	LOG_DBG("ind src=" KNX_ADDR_FMT, KNX_ADDR_VAL(pkt->src));
	knx_t_data_system_broadcast_ind(pkt);
}

void knx_n_data_system_broadcast_con(struct knx_pkt *pkt)
{
	knx_t_data_system_broadcast_con(pkt);
}

void knx_n_data_system_broadcast_req(struct knx_pkt *pkt)
{
	knx_l_system_broadcast_req(pkt);
}
