/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_tlv, CONFIG_PTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#include "tlv.h"

K_MEM_SLAB_DEFINE_STATIC(tlv_slab,
			 sizeof(struct ptp_tlv_container),
			 2 * CONFIG_PTP_MSG_POLL_SIZE,
			 8);

static inline void tlv_ntohs(void *ptr)
{
	uint16_t val = *(uint16_t *)ptr;

	val = ntohs(val);
	memcpy(ptr, &val, sizeof(val));
}

static inline void tlv_htons(void *ptr)
{
	uint16_t val = *(uint16_t *)ptr;

	val = htons(val);
	memcpy(ptr, &val, sizeof(val));
}

struct ptp_tlv_container *ptp_tlv_alloc(void)
{
	struct ptp_tlv_container *tlv_container = NULL;
	int ret = k_mem_slab_alloc(&tlv_slab, (void **)&tlv_container, K_FOREVER);

	if (ret) {
		LOG_ERR("Couldn't allocate memory for the TLV");
		return NULL;
	}

	memset(tlv_container, 0, sizeof(*tlv_container));
	return tlv_container;
}

void ptp_tlv_free(struct ptp_tlv_container *tlv_container)
{
	k_mem_slab_free(&tlv_slab, (void *)tlv_container);
}

enum ptp_tlv_type ptp_tlv_type(struct ptp_tlv *tlv)
{
	return (enum ptp_tlv_type)tlv->type;
}

int ptp_tlv_post_recv(struct ptp_tlv *tlv)
{
	int ret = 0;

	tlv->length = ntohs(tlv->length);
	tlv->type = ntohs(tlv->type);

	switch (ptp_tlv_type(tlv)) {
	case PTP_TLV_TYPE_MANAGEMENT:
	case PTP_TLV_TYPE_MANAGEMENT_ERROR_STATUS:
	case PTP_TLV_TYPE_REQUEST_UNICAST_TRANSMISSION:
	case PTP_TLV_TYPE_GRANT_UNICAST_TRANSMISSION:
	case PTP_TLV_TYPE_CANCEL_UNICAST_TRANSMISSION:
	case PTP_TLV_TYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:
	case PTP_TLV_TYPE_PATH_TRACE:
	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION:
	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION_PROPAGATE:
	case PTP_TLV_TYPE_ENHANCED_ACCURACY_METRICS:
	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION_DO_NOT_PROPAGATE:
	case PTP_TLV_TYPE_L1_SYNC:
	case PTP_TLV_TYPE_PORT_COMMUNICATION_AVAILABILITY:
	case PTP_TLV_TYPE_PROTOCOL_ADDRESS:
	case PTP_TLV_TYPE_SLAVE_RX_SYNC_TIMING_DATA:
	case PTP_TLV_TYPE_SLAVE_RX_SYNC_COMPUTED_DATA:
	case PTP_TLV_TYPE_SLAVE_TX_EVENT_TIMESTAMPS:
	case PTP_TLV_TYPE_CUMULATIVE_RATE_RATIO:
	case PTP_TLV_TYPE_PAD:
	case PTP_TLV_TYPE_AUTHENTICATION:
		break;
	default:
		break;
	}

	return ret;
}

void ptp_tlv_pre_send(struct ptp_tlv *tlv)
{
	switch (ptp_tlv_type(tlv)) {
	case PTP_TLV_TYPE_MANAGEMENT:
	case PTP_TLV_TYPE_MANAGEMENT_ERROR_STATUS:
	case PTP_TLV_TYPE_REQUEST_UNICAST_TRANSMISSION:
	case PTP_TLV_TYPE_GRANT_UNICAST_TRANSMISSION:
	case PTP_TLV_TYPE_CANCEL_UNICAST_TRANSMISSION:
	case PTP_TLV_TYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:
	case PTP_TLV_TYPE_PATH_TRACE:
	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION:
	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION_PROPAGATE:
	case PTP_TLV_TYPE_ENHANCED_ACCURACY_METRICS:
	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION_DO_NOT_PROPAGATE:
	case PTP_TLV_TYPE_L1_SYNC:
	case PTP_TLV_TYPE_PORT_COMMUNICATION_AVAILABILITY:
	case PTP_TLV_TYPE_PROTOCOL_ADDRESS:
	case PTP_TLV_TYPE_SLAVE_RX_SYNC_TIMING_DATA:
	case PTP_TLV_TYPE_SLAVE_RX_SYNC_COMPUTED_DATA:
	case PTP_TLV_TYPE_SLAVE_TX_EVENT_TIMESTAMPS:
	case PTP_TLV_TYPE_CUMULATIVE_RATE_RATIO:
	case PTP_TLV_TYPE_PAD:
	case PTP_TLV_TYPE_AUTHENTICATION:
		break;
	default:
		break;
	}

	tlv->length = htons(tlv->length);
	tlv->type = htons(tlv->type);
}
