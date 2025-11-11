/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <zephyr/ipc/icmsg_me.h>
#include <zephyr/sys/math_extras.h>

#include <string.h>

#define SEND_BUF_SIZE CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_SEND_BUF_SIZE
#define NUM_EP        CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_NUM_EP

#define EVENT_BOUND 0x01

#define HEADER_SIZE (sizeof(icmsg_me_ept_id_t))

static void *icmsg_buffer_to_user_buffer(const void *icmsg_buffer)
{
	return (void *)(((char *)icmsg_buffer) + HEADER_SIZE);
}

static ssize_t icmsg_buffer_len_to_user_buffer_len(size_t icmsg_buffer_len)
{
	if (icmsg_buffer_len < HEADER_SIZE) {
		return -EINVAL;
	}

	return (ssize_t)(icmsg_buffer_len - HEADER_SIZE);
}

static ssize_t user_buffer_len_to_icmsg_buffer_len(size_t user_buffer_len)
{
	size_t ret;

	if (size_add_overflow(user_buffer_len, HEADER_SIZE, &ret)) {
		return -EINVAL;
	}

	return (ssize_t)ret;
}

static void set_ept_id_in_send_buffer(uint8_t *send_buffer,
				      icmsg_me_ept_id_t ept_id)
{
	send_buffer[0] = ept_id;
}

int icmsg_me_init(const struct icmsg_config_t *conf,
		  struct icmsg_me_data_t *data)
{
	k_event_init(&data->event);
	k_mutex_init(&data->send_mutex);

	return 0;
}

int icmsg_me_open(const struct icmsg_config_t *conf,
		  struct icmsg_me_data_t *data,
		  const struct ipc_service_cb *cb,
		  void *ctx)
{
	data->ept_cfg.cb = *cb;
	data->ept_cfg.priv = ctx;

	return icmsg_open(conf, &data->icmsg_data, &data->ept_cfg.cb,
			  data->ept_cfg.priv);
}

void icmsg_me_icmsg_bound(struct icmsg_me_data_t *data)
{
	k_event_post(&data->event, EVENT_BOUND);
}

void icmsg_me_wait_for_icmsg_bind(struct icmsg_me_data_t *data)
{
	k_event_wait(&data->event, EVENT_BOUND, false, K_FOREVER);
}

int icmsg_me_set_empty_ept_cfg_slot(struct icmsg_me_data_t *data,
				    const struct ipc_ept_cfg *ept_cfg,
				    icmsg_me_ept_id_t *id)
{
	int i;

	for (i = 0; i < NUM_EP; i++) {
		if (data->epts[i] == NULL) {
			break;
		}
	}

	if (i >= NUM_EP) {
		return -ENOMEM;
	}

	data->epts[i] = ept_cfg;
	*id = i + 1;
	return 0;
}

static int get_ept_cfg_index(icmsg_me_ept_id_t id)
{
	int i = id - 1;

	if (i >= NUM_EP || i < 0) {
		return -ENOENT;
	}

	return i;
}

int icmsg_me_set_ept_cfg(struct icmsg_me_data_t *data, icmsg_me_ept_id_t id,
			 const struct ipc_ept_cfg *ept_cfg)
{
	int i = get_ept_cfg_index(id);

	if (i < 0) {
		return i;
	}

	data->epts[i] = ept_cfg;
	return 0;
}

int icmsg_me_get_ept_cfg(struct icmsg_me_data_t *data, icmsg_me_ept_id_t id,
			 const struct ipc_ept_cfg **ept_cfg)
{
	int i = get_ept_cfg_index(id);

	if (i < 0) {
		return i;
	}

	*ept_cfg = data->epts[i];
	return 0;
}

void icmsg_me_reset_ept_cfg(struct icmsg_me_data_t *data, icmsg_me_ept_id_t id)
{
	int i = get_ept_cfg_index(id);

	if (i < 0) {
		return;
	}

	data->epts[i] = NULL;
}

void icmsg_me_received_data(struct icmsg_me_data_t *data, icmsg_me_ept_id_t id,
			    const void *msg, size_t len)
{
	int r;
	const struct ipc_ept_cfg *ept;
	ssize_t user_buffer_len;

	r = icmsg_me_get_ept_cfg(data, id, &ept);
	if (r < 0) {
		return;
	}

	if (ept == NULL) {
		return;
	}

	user_buffer_len = icmsg_buffer_len_to_user_buffer_len(len);
	if (user_buffer_len < 0) {
		return;
	}

	if (ept->cb.received) {
		ept->cb.received(icmsg_buffer_to_user_buffer(msg),
				 user_buffer_len, ept->priv);
	}
}

int icmsg_me_send(const struct icmsg_config_t *conf,
		  struct icmsg_me_data_t *data, icmsg_me_ept_id_t id,
		  const void *msg, size_t len)
{
	int r;
	int sent_bytes = 0;
	ssize_t icmsg_buffer_len = user_buffer_len_to_icmsg_buffer_len(len);

	if ((icmsg_buffer_len < 0) || (icmsg_buffer_len >= SEND_BUF_SIZE)) {
		return -EBADMSG;
	}

	k_mutex_lock(&data->send_mutex, K_FOREVER);

	/* TODO: Optimization: How to avoid this copying? */
	/* We could implement scatter list for icmsg_send, but it would require
	 * scatter list also for SPSC buffer implementation.
	 */
	set_ept_id_in_send_buffer(data->send_buffer, id);
	memcpy(icmsg_buffer_to_user_buffer(data->send_buffer), msg, len);

	r = icmsg_send(conf, &data->icmsg_data, data->send_buffer,
		       icmsg_buffer_len);
	if (r > 0) {
		sent_bytes = icmsg_buffer_len_to_user_buffer_len(r);
	}

	k_mutex_unlock(&data->send_mutex);

	if (r < 0) {
		return r;
	}

	__ASSERT_NO_MSG(r >= HEADER_SIZE);
	if (r < HEADER_SIZE) {
		return 0;
	}

	return sent_bytes;
}
