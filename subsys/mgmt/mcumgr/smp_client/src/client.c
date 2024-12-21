/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** SMP - Simple Management Client Protocol. */

#include <string.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <mgmt/mcumgr/transport/smp_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcumgr_smp_client, CONFIG_MCUMGR_SMP_CLIENT_LOG_LEVEL);

struct smp_client_cmd_req {
	sys_snode_t node;
	struct net_buf *nb;
	struct smp_client_object *smp_client;
	void *user_data;
	smp_client_res_fn cb;
	int64_t timestamp;
	int retry_cnt;
};

struct smp_client_data_base {
	struct k_work_delayable work_delay;
	sys_slist_t cmd_free_list;
	sys_slist_t cmd_list;
};

static struct smp_client_cmd_req smp_cmd_req_bufs[CONFIG_SMP_CLIENT_CMD_MAX];
static struct smp_client_data_base smp_client_data;

static void smp_read_hdr(const struct net_buf *nb, struct smp_hdr *dst_hdr);
static void smp_client_cmd_req_free(struct smp_client_cmd_req *cmd_req);

/**
 * Send all SMP client request packets.
 */
static void smp_client_handle_reqs(struct k_work *work)
{
	struct smp_client_object *smp_client;
	struct smp_transport *smpt;
	struct net_buf *nb;

	smp_client = (void *)work;
	smpt = smp_client->smpt;

	while ((nb = k_fifo_get(&smp_client->tx_fifo, K_NO_WAIT)) != NULL) {
		smpt->functions.output(nb);
	}
}

static void smp_header_init(struct smp_hdr *header, uint16_t group, uint8_t id, uint8_t op,
			    uint16_t payload_len, uint8_t seq, enum smp_mcumgr_version_t version)
{
	/* Pre config SMP header structure */
	memset(header, 0, sizeof(struct smp_hdr));
	header->nh_version = version;
	header->nh_op = op;
	header->nh_len = sys_cpu_to_be16(payload_len);
	header->nh_group = sys_cpu_to_be16(group);
	header->nh_id = id;
	header->nh_seq = seq;
}

static void smp_client_transport_work_fn(struct k_work *work)
{
	struct smp_client_cmd_req *entry, *tmp;
	smp_client_res_fn cb;
	void *user_data;
	int backoff_ms = CONFIG_SMP_CMD_RETRY_TIME;
	int64_t time_stamp_cmp;
	int64_t time_stamp_ref;
	int64_t time_stamp_delta;

	ARG_UNUSED(work);

	if (sys_slist_is_empty(&smp_client_data.cmd_list)) {
		/* No more packet for Transport */
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&smp_client_data.cmd_list, entry, tmp, node) {
		time_stamp_ref = entry->timestamp;
		/* Check Time delta and get current time to reference */
		time_stamp_delta = k_uptime_delta(&time_stamp_ref);

		if (time_stamp_delta < 0) {
			time_stamp_cmp = entry->timestamp - time_stamp_ref;
			if (time_stamp_cmp < CONFIG_SMP_CMD_RETRY_TIME &&
			    time_stamp_cmp < backoff_ms) {
				/* Update new shorter shedule */
				backoff_ms = time_stamp_cmp;
			}
			continue;
		} else if (entry->retry_cnt) {
			/* Increment reference for re-transmission */
			entry->nb = net_buf_ref(entry->nb);
			entry->retry_cnt--;
			entry->timestamp = time_stamp_ref + CONFIG_SMP_CMD_RETRY_TIME;
			k_fifo_put(&entry->smp_client->tx_fifo, entry->nb);
			k_work_submit_to_queue(smp_get_wq(), &entry->smp_client->work);
			continue;
		}

		cb = entry->cb;
		user_data = entry->user_data;
		smp_client_cmd_req_free(entry);
		if (cb) {
			cb(NULL, user_data);
		}
	}

	if (!sys_slist_is_empty(&smp_client_data.cmd_list)) {
		/* Re-schedule new timeout to next */
		k_work_reschedule_for_queue(smp_get_wq(), &smp_client_data.work_delay,
					    K_MSEC(backoff_ms));
	}
}

static int smp_client_init(void)
{
	k_work_init_delayable(&smp_client_data.work_delay, smp_client_transport_work_fn);
	sys_slist_init(&smp_client_data.cmd_list);
	sys_slist_init(&smp_client_data.cmd_free_list);
	for (int i = 0; i < CONFIG_SMP_CLIENT_CMD_MAX; i++) {
		sys_slist_append(&smp_client_data.cmd_free_list, &smp_cmd_req_bufs[i].node);
	}
	return 0;
}

static struct smp_client_cmd_req *smp_client_cmd_req_allocate(void)
{
	sys_snode_t *cmd_node;
	struct smp_client_cmd_req *req;

	cmd_node = sys_slist_get(&smp_client_data.cmd_free_list);
	if (!cmd_node) {
		return NULL;
	}

	req = SYS_SLIST_CONTAINER(cmd_node, req, node);

	return req;
}

static void smp_cmd_add_to_list(struct smp_client_cmd_req *cmd_req)
{
	if (sys_slist_is_empty(&smp_client_data.cmd_list)) {
		/* Enable timer */
		k_work_reschedule_for_queue(smp_get_wq(), &smp_client_data.work_delay,
					    K_MSEC(CONFIG_SMP_CMD_RETRY_TIME));
	}
	sys_slist_append(&smp_client_data.cmd_list, &cmd_req->node);
}

static void smp_client_cmd_req_free(struct smp_client_cmd_req *cmd_req)
{
	smp_client_buf_free(cmd_req->nb);
	cmd_req->nb = NULL;
	sys_slist_find_and_remove(&smp_client_data.cmd_list, &cmd_req->node);
	/* Add to free list */
	sys_slist_append(&smp_client_data.cmd_free_list, &cmd_req->node);

	if (sys_slist_is_empty(&smp_client_data.cmd_list)) {
		/* cancel delay */
		k_work_cancel_delayable(&smp_client_data.work_delay);
	}
}

static struct smp_client_cmd_req *smp_client_response_discover(const struct smp_hdr *res_hdr)
{
	struct smp_hdr smp_header;
	enum mcumgr_op_t response;
	struct smp_client_cmd_req *cmd_req;

	if (sys_slist_is_empty(&smp_client_data.cmd_list)) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&smp_client_data.cmd_list, cmd_req, node) {
		smp_read_hdr(cmd_req->nb, &smp_header);
		if (smp_header.nh_op == MGMT_OP_READ) {
			response = MGMT_OP_READ_RSP;
		} else {
			response = MGMT_OP_WRITE_RSP;
		}

		if (smp_header.nh_seq != res_hdr->nh_seq) {
			continue;
		} else if (res_hdr->nh_op != response) {
			continue;
		}

		return cmd_req;
	}
	return NULL;
}

int smp_client_object_init(struct smp_client_object *smp_client, int smp_type)
{
	smp_client->smpt = smp_client_transport_get(smp_type);
	if (!smp_client->smpt) {
		return MGMT_ERR_EINVAL;
	}

	/* Init TX FIFO */
	k_work_init(&smp_client->work, smp_client_handle_reqs);
	k_fifo_init(&smp_client->tx_fifo);

	return MGMT_ERR_EOK;
}

int smp_client_single_response(struct net_buf *nb, const struct smp_hdr *res_hdr)
{
	struct smp_client_cmd_req *cmd_req;
	smp_client_res_fn cb;
	void *user_data;

	/* Discover request for incoming response */
	cmd_req = smp_client_response_discover(res_hdr);
	LOG_DBG("Response Header len %d, flags %d OP: %d group %d id %d seq %d", res_hdr->nh_len,
		res_hdr->nh_flags, res_hdr->nh_op, res_hdr->nh_group, res_hdr->nh_id,
		res_hdr->nh_seq);

	if (cmd_req) {
		cb = cmd_req->cb;
		user_data = cmd_req->user_data;
		smp_client_cmd_req_free(cmd_req);
		if (cb) {
			cb(nb, user_data);
			return MGMT_ERR_EOK;
		}
	}

	return MGMT_ERR_ENOENT;
}

struct net_buf *smp_client_buf_allocation(struct smp_client_object *smp_client, uint16_t group,
					  uint8_t command_id, uint8_t op,
					  enum smp_mcumgr_version_t version)
{
	struct net_buf *nb;
	struct smp_hdr smp_header;

	nb = smp_packet_alloc();

	if (nb) {
		/* Write SMP header with payload length 0 */
		smp_header_init(&smp_header, group, command_id, op, 0, smp_client->smp_seq++,
				version);
		memcpy(nb->data, &smp_header, sizeof(smp_header));
		nb->len = sizeof(smp_header);
	}
	return nb;
}

void smp_client_buf_free(struct net_buf *nb)
{
	smp_packet_free(nb);
}

static void smp_read_hdr(const struct net_buf *nb, struct smp_hdr *dst_hdr)
{
	memcpy(dst_hdr, nb->data, sizeof(*dst_hdr));
	dst_hdr->nh_len = sys_be16_to_cpu(dst_hdr->nh_len);
	dst_hdr->nh_group = sys_be16_to_cpu(dst_hdr->nh_group);
}

int smp_client_send_cmd(struct smp_client_object *smp_client, struct net_buf *nb,
			smp_client_res_fn cb, void *user_data, int timeout_in_sec)
{
	struct smp_hdr smp_header;
	struct smp_client_cmd_req *cmd_req;

	if (timeout_in_sec > 30) {
		LOG_ERR("Command timeout can't be over 30 seconds");
		return MGMT_ERR_EINVAL;
	}

	if (timeout_in_sec == 0) {
		timeout_in_sec = CONFIG_SMP_CMD_DEFAULT_LIFE_TIME;
	}

	smp_read_hdr(nb, &smp_header);
	if (nb->len < sizeof(smp_header)) {
		return MGMT_ERR_EINVAL;
	}
	/* Update Length */
	smp_header.nh_len = sys_cpu_to_be16(nb->len - sizeof(smp_header));
	smp_header.nh_group = sys_cpu_to_be16(smp_header.nh_group),
	memcpy(nb->data, &smp_header, sizeof(smp_header));

	cmd_req = smp_client_cmd_req_allocate();
	if (!cmd_req) {
		return MGMT_ERR_ENOMEM;
	}

	LOG_DBG("Command send Header flags %d OP: %d group %d id %d seq %d", smp_header.nh_flags,
		smp_header.nh_op, sys_be16_to_cpu(smp_header.nh_group), smp_header.nh_id,
		smp_header.nh_seq);
	cmd_req->nb = nb;
	cmd_req->cb = cb;
	cmd_req->smp_client = smp_client;
	cmd_req->user_data = user_data;
	cmd_req->retry_cnt = timeout_in_sec * (1000 / CONFIG_SMP_CMD_RETRY_TIME);
	cmd_req->timestamp = k_uptime_get() + CONFIG_SMP_CMD_RETRY_TIME;
	/* Increment reference for re-transmission and read smp header */
	nb = net_buf_ref(nb);
	smp_cmd_add_to_list(cmd_req);
	k_fifo_put(&smp_client->tx_fifo, nb);
	k_work_submit_to_queue(smp_get_wq(), &smp_client->work);
	return MGMT_ERR_EOK;
}

SYS_INIT(smp_client_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
