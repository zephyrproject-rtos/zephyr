/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_tlv, CONFIG_PTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#include "port.h"
#include "tlv.h"

#define TLV_MANUFACTURER_ID_LEN (3)
#define TLV_PROFILE_ID_LEN (6)
#define TLV_ADDR_LEN_MAX (16)

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

static int tlv_mgmt_post_recv(struct ptp_tlv_mgmt *mgmt_tlv, uint16_t length)
{
	struct ptp_tlv_mgmt_clock_desc *clock_desc;
	struct ptp_tlv_time_prop_ds *time_prop_ds;
	struct ptp_tlv_default_ds *default_ds;
	struct ptp_tlv_current_ds *current_ds;
	struct ptp_tlv_parent_ds *parent_ds;
	struct ptp_tlv_port_ds *port_ds;
	struct ptp_timestamp ts;

	enum ptp_mgmt_id id = (enum ptp_mgmt_id)mgmt_tlv->id;
	struct ptp_tlv_container *container;
	uint8_t *data;
	int32_t data_length;

	switch (id) {
	case PTP_MGMT_NULL_PTP_MANAGEMENT:
		__fallthrough;
	case PTP_MGMT_SAVE_IN_NON_VOLATILE_STORAGE:
		__fallthrough;
	case PTP_MGMT_RESET_NON_VOLATILE_STORAGE:
		__fallthrough;
	case PTP_MGMT_FAULT_LOG_RESET:
		__fallthrough;
	case PTP_MGMT_ENABLE_PORT:
		__fallthrough;
	case PTP_MGMT_DISABLE_PORT:
		if (length != 0) {
			return -EBADMSG;
		}
		break;
	case PTP_MGMT_CLOCK_DESCRIPTION:
		container = CONTAINER_OF((void *)mgmt_tlv, struct ptp_tlv_container, tlv);

		clock_desc = &container->clock_desc;
		data = mgmt_tlv->data;
		data_length = length;

		clock_desc->type = (uint16_t *)data;
		data += sizeof(*clock_desc->type);
		data_length -= sizeof(*clock_desc->type);
		if (data_length < 0) {
			return -EBADMSG;
		}
		tlv_ntohs(&clock_desc->type);

		clock_desc->phy_protocol = (struct ptp_text *)data;
		data += sizeof(*clock_desc->phy_protocol);
		data_length -= sizeof(*clock_desc->phy_protocol);
		if (data_length < 0) {
			return -EBADMSG;
		}
		data += clock_desc->phy_protocol->length;
		data_length -= clock_desc->phy_protocol->length;
		if (data_length < 0) {
			return -EBADMSG;
		}

		clock_desc->phy_addr_len = (uint16_t *)data;
		data += sizeof(*clock_desc->phy_addr_len);
		data_length -= sizeof(*clock_desc->phy_addr_len);
		if (data_length < 0) {
			return -EBADMSG;
		}
		tlv_ntohs(&clock_desc->phy_addr_len);
		if (*clock_desc->phy_addr_len > TLV_ADDR_LEN_MAX) {
			return -EBADMSG;
		}

		clock_desc->phy_addr = data;
		data += *clock_desc->phy_addr_len;
		data_length -= *clock_desc->phy_addr_len;
		if (data_length < 0) {
			return -EBADMSG;
		}

		clock_desc->protocol_addr = (struct ptp_port_addr *)data;
		data += sizeof(*clock_desc->protocol_addr);
		data_length -= sizeof(*clock_desc->protocol_addr);
		if (data_length < 0) {
			return -EBADMSG;
		}
		tlv_ntohs(&clock_desc->protocol_addr->protocol);
		tlv_ntohs(&clock_desc->protocol_addr->addr_len);
		if (clock_desc->protocol_addr->addr_len > TLV_ADDR_LEN_MAX) {
			return -EBADMSG;
		}

		data += clock_desc->protocol_addr->addr_len;
		data_length -= clock_desc->protocol_addr->addr_len;
		if (data_length < 0) {
			return -EBADMSG;
		}

		clock_desc->manufacturer_id = data;
		/* extra byte for reserved field - see IEEE 1588-2019 15.5.3.1.2 */
		data += TLV_MANUFACTURER_ID_LEN + 1;
		data_length -= TLV_MANUFACTURER_ID_LEN + 1;
		if (data_length < 0) {
			return -EBADMSG;
		}

		clock_desc->product_desc = (struct ptp_text *)data;
		data += sizeof(*clock_desc->product_desc);
		data_length -= sizeof(*clock_desc->product_desc);
		if (data_length < 0) {
			return -EBADMSG;
		}
		data += clock_desc->product_desc->length;
		data_length -= clock_desc->product_desc->length;
		if (data_length < 0) {
			return -EBADMSG;
		}

		clock_desc->revision_data = (struct ptp_text *)data;
		data += sizeof(*clock_desc->revision_data);
		data_length -= sizeof(*clock_desc->revision_data);
		if (data_length < 0) {
			return -EBADMSG;
		}
		data += clock_desc->revision_data->length;
		data_length -= clock_desc->revision_data->length;
		if (data_length < 0) {
			return -EBADMSG;
		}

		clock_desc->user_desc = (struct ptp_text *)data;
		data += sizeof(*clock_desc->user_desc);
		data_length -= sizeof(*clock_desc->user_desc);
		if (data_length < 0) {
			return -EBADMSG;
		}
		data += clock_desc->user_desc->length;
		data_length -= clock_desc->user_desc->length;
		if (data_length < 0) {
			return -EBADMSG;
		}

		clock_desc->profile_id = data;
		data += TLV_PROFILE_ID_LEN;
		data_length -= TLV_PROFILE_ID_LEN;

		break;
	case PTP_MGMT_USER_DESCRIPTION:
		container = CONTAINER_OF((void *)mgmt_tlv, struct ptp_tlv_container, tlv);

		if (length < sizeof(struct ptp_text)) {
			return -EBADMSG;
		}
		container->clock_desc.user_desc = (struct ptp_text *)mgmt_tlv->data;
		break;
	case PTP_MGMT_DEFAULT_DATA_SET:
		if (length != sizeof(struct ptp_tlv_default_ds)) {
			return -EBADMSG;
		}
		default_ds = (struct ptp_tlv_default_ds *)mgmt_tlv->data;

		default_ds->n_ports = ntohs(default_ds->n_ports);
		default_ds->clk_quality.offset_scaled_log_variance =
			ntohs(default_ds->clk_quality.offset_scaled_log_variance);
		break;
	case PTP_MGMT_CURRENT_DATA_SET:
		if (length != sizeof(struct ptp_tlv_current_ds)) {
			return -EBADMSG;
		}
		current_ds = (struct ptp_tlv_current_ds *)mgmt_tlv->data;

		current_ds->steps_rm = ntohs(current_ds->steps_rm);
		current_ds->offset_from_tt = ntohll(current_ds->offset_from_tt);
		current_ds->mean_delay = ntohll(current_ds->mean_delay);
		break;
	case PTP_MGMT_PARENT_DATA_SET:
		if (length != sizeof(struct ptp_tlv_parent_ds)) {
			return -EBADMSG;
		}
		parent_ds = (struct ptp_tlv_parent_ds *)mgmt_tlv->data;

		parent_ds->port_id.port_number = ntohs(parent_ds->port_id.port_number);
		parent_ds->obsreved_parent_offset_scaled_log_variance =
			ntohs(parent_ds->obsreved_parent_offset_scaled_log_variance);
		parent_ds->obsreved_parent_clk_phase_change_rate =
			ntohl(parent_ds->obsreved_parent_clk_phase_change_rate);
		parent_ds->gm_clk_quality.offset_scaled_log_variance =
			ntohs(parent_ds->gm_clk_quality.offset_scaled_log_variance);
		break;
	case PTP_MGMT_TIME_PROPERTIES_DATA_SET:
		if (length != sizeof(struct ptp_tlv_time_prop_ds)) {
			return -EBADMSG;
		}
		time_prop_ds = (struct ptp_tlv_time_prop_ds *)mgmt_tlv->data;

		time_prop_ds->current_utc_offset = ntohs(time_prop_ds->current_utc_offset);
		break;
	case PTP_MGMT_PORT_DATA_SET:
		if (length != sizeof(struct ptp_tlv_port_ds)) {
			return -EBADMSG;
		}
		port_ds = (struct ptp_tlv_port_ds *)mgmt_tlv->data;

		port_ds->id.port_number = ntohs(port_ds->id.port_number);
		port_ds->mean_link_delay = ntohll(port_ds->mean_link_delay);
		break;
	case PTP_MGMT_TIME:
		ts = *(struct ptp_timestamp *)mgmt_tlv->data;

		ts.seconds_high = ntohs(ts.seconds_high);
		ts.seconds_low = ntohl(ts.seconds_low);
		ts.nanoseconds = ntohl(ts.nanoseconds);

		memcpy(mgmt_tlv->data, &ts, sizeof(ts));
		break;
	default:
		break;
	}

	return 0;
}

static void tlv_mgmt_pre_send(struct ptp_tlv_mgmt *mgmt_tlv)
{
	enum ptp_mgmt_id id = (enum ptp_mgmt_id)mgmt_tlv->id;
	struct ptp_tlv_mgmt_clock_desc *clock_desc;
	struct ptp_tlv_time_prop_ds *time_prop_ds;
	struct ptp_tlv_default_ds *default_ds;
	struct ptp_tlv_current_ds *current_ds;
	struct ptp_tlv_container *container;
	struct ptp_tlv_parent_ds *parent_ds;
	struct ptp_tlv_port_ds *port_ds;
	struct ptp_timestamp ts;

	switch (id) {
	case PTP_MGMT_CLOCK_DESCRIPTION:
		container = CONTAINER_OF((void *)mgmt_tlv, struct ptp_tlv_container, tlv);
		clock_desc = &container->clock_desc;

		tlv_htons(&clock_desc->type);
		tlv_htons(&clock_desc->phy_addr_len);
		tlv_htons(&clock_desc->protocol_addr->protocol);
		tlv_htons(&clock_desc->protocol_addr->addr_len);
		break;
	case PTP_MGMT_DEFAULT_DATA_SET:
		default_ds = (struct ptp_tlv_default_ds *)mgmt_tlv->data;

		default_ds->n_ports = htons(default_ds->n_ports);
		default_ds->clk_quality.offset_scaled_log_variance =
			htons(default_ds->clk_quality.offset_scaled_log_variance);
		break;
	case PTP_MGMT_CURRENT_DATA_SET:
		current_ds = (struct ptp_tlv_current_ds *)mgmt_tlv->data;

		current_ds->steps_rm = htons(current_ds->steps_rm);
		current_ds->offset_from_tt = htonll(current_ds->offset_from_tt);
		current_ds->mean_delay = htonll(current_ds->mean_delay);
		break;
	case PTP_MGMT_PARENT_DATA_SET:
		parent_ds = (struct ptp_tlv_parent_ds *)mgmt_tlv->data;

		parent_ds->port_id.port_number = htons(parent_ds->port_id.port_number);
		parent_ds->obsreved_parent_offset_scaled_log_variance =
			htons(parent_ds->obsreved_parent_offset_scaled_log_variance);
		parent_ds->obsreved_parent_clk_phase_change_rate =
			htons(parent_ds->obsreved_parent_clk_phase_change_rate);
		parent_ds->gm_clk_quality.offset_scaled_log_variance =
			htons(parent_ds->gm_clk_quality.offset_scaled_log_variance);
		break;
	case PTP_MGMT_TIME_PROPERTIES_DATA_SET:
		time_prop_ds = (struct ptp_tlv_time_prop_ds *)mgmt_tlv->data;

		time_prop_ds->current_utc_offset = htons(time_prop_ds->current_utc_offset);
		break;
	case PTP_MGMT_PORT_DATA_SET:
		port_ds = (struct ptp_tlv_port_ds *)mgmt_tlv->data;

		port_ds->id.port_number = htons(port_ds->id.port_number);
		port_ds->mean_link_delay = htonll(port_ds->mean_link_delay);
		break;
	case PTP_MGMT_TIME:
		ts = *(struct ptp_timestamp *)mgmt_tlv->data;

		ts.seconds_high = htons(ts.seconds_high);
		ts.seconds_low = htonl(ts.seconds_low);
		ts.nanoseconds = htonl(ts.nanoseconds);

		memcpy(mgmt_tlv->data, &ts, sizeof(ts));
		break;
	default:
		break;
	}
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

enum ptp_mgmt_op ptp_mgmt_action(struct ptp_msg *msg)
{
	return (enum ptp_mgmt_op)msg->management.action;
}

enum ptp_tlv_type ptp_tlv_type(struct ptp_tlv *tlv)
{
	return (enum ptp_tlv_type)tlv->type;
}

int ptp_tlv_post_recv(struct ptp_tlv *tlv)
{
	struct ptp_tlv_mgmt_err *mgmt_err;
	struct ptp_tlv_mgmt *mgmt;
	int ret = 0;

	switch (ptp_tlv_type(tlv)) {
	case PTP_TLV_TYPE_MANAGEMENT:
		if (tlv->length < (sizeof(struct ptp_tlv_mgmt) - sizeof(struct ptp_tlv))) {
			return -EBADMSG;
		}
		mgmt = (struct ptp_tlv_mgmt *)tlv;
		mgmt->id = ntohs(mgmt->id);

		/* Value of length is 2 + N, where N is length of data field
		 * based on IEEE 1588-2019 Section 15.5.2.2.
		 */
		if (tlv->length > sizeof(mgmt->id)) {
			ret = tlv_mgmt_post_recv(mgmt, tlv->length - 2);
		}
		break;
	case PTP_TLV_TYPE_MANAGEMENT_ERROR_STATUS:
		if (tlv->length < (sizeof(struct ptp_tlv_mgmt_err) - sizeof(struct ptp_tlv))) {
			return -EBADMSG;
		}
		mgmt_err = (struct ptp_tlv_mgmt_err *)tlv;
		mgmt_err->err_id = ntohs(mgmt_err->err_id);
		mgmt_err->id = ntohs(mgmt_err->id);
		break;
	default:
		break;
	}

	return ret;
}

void ptp_tlv_pre_send(struct ptp_tlv *tlv)
{
	struct ptp_tlv_mgmt_err *mgmt_err;
	struct ptp_tlv_mgmt *mgmt;

	switch (ptp_tlv_type(tlv)) {
	case PTP_TLV_TYPE_MANAGEMENT:
		mgmt = (struct ptp_tlv_mgmt *)tlv;

		/* Check if management TLV contains data */
		if (tlv->length > sizeof(mgmt->id)) {
			tlv_mgmt_pre_send(mgmt);
		}
		mgmt->id = htons(mgmt->id);
		break;
	case PTP_TLV_TYPE_MANAGEMENT_ERROR_STATUS:
		mgmt_err = (struct ptp_tlv_mgmt_err *)tlv;

		mgmt_err->err_id = htons(mgmt_err->err_id);
		mgmt_err->id = htons(mgmt_err->id);
		break;
	default:
		break;
	}

	tlv->length = htons(tlv->length);
	tlv->type = htons(tlv->type);
}
