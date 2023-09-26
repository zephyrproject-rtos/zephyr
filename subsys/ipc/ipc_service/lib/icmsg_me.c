/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/icmsg_me.h>

#include <string.h>

#define SEND_BUF_SIZE CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_SEND_BUF_SIZE
#define NUM_EP        CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_NUM_EP

#define EVENT_BOUND 0x01

#define HEADER_SIZE (sizeof(icmsg_me_ept_id_t))

static void *icmsg_buffer_to_user_buffer(const void *icmsg_buffer)
{
	return (void *)(((char *)icmsg_buffer) + HEADER_SIZE);
}

static void *user_buffer_to_icmsg_buffer(const void *user_buffer)
{
	return (void *)(((char *)user_buffer) - HEADER_SIZE);
}

static size_t icmsg_buffer_len_to_user_buffer_len(size_t icmsg_buffer_len)
{
	return icmsg_buffer_len - HEADER_SIZE;
}

static size_t user_buffer_len_to_icmsg_buffer_len(size_t user_buffer_len)
{
	return user_buffer_len + HEADER_SIZE;
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

	r = icmsg_me_get_ept_cfg(data, id, &ept);
	if (r < 0) {
		return;
	}

	if (ept == NULL) {
		return;
	}

	if (ept->cb.received) {
		ept->cb.received(icmsg_buffer_to_user_buffer(msg),
				 icmsg_buffer_len_to_user_buffer_len(len),
				 ept->priv);
	}
}

int icmsg_me_send(const struct icmsg_config_t *conf,
		  struct icmsg_me_data_t *data, icmsg_me_ept_id_t id,
		  const void *msg, size_t len)
{
	int r;
	int sent_bytes = 0;

	if (user_buffer_len_to_icmsg_buffer_len(len) >= SEND_BUF_SIZE) {
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
		       user_buffer_len_to_icmsg_buffer_len(len));
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

static size_t get_buffer_length_to_pass(size_t icmsg_buffer_len)
{
	if (icmsg_buffer_len >= HEADER_SIZE) {
		return icmsg_buffer_len_to_user_buffer_len(icmsg_buffer_len);
	} else {
		return 0;
	}
}

int icmsg_me_get_tx_buffer(const struct icmsg_config_t *conf,
			   struct icmsg_me_data_t *data,
			   void **buffer, uint32_t *user_len, k_timeout_t wait)
{
	void *icmsg_buffer;
	int r;
	size_t icmsg_len;

	if (!K_TIMEOUT_EQ(wait, K_NO_WAIT)) {
		return -ENOTSUP;
	}

	if (*user_len) {
		icmsg_len = user_buffer_len_to_icmsg_buffer_len(*user_len);
	} else {
		icmsg_len = 0;
	}

	r = icmsg_get_tx_buffer(conf, &data->icmsg_data,
				&icmsg_buffer, &icmsg_len);
	if (r == -ENOMEM) {
		*user_len = get_buffer_length_to_pass(icmsg_len);
		return -ENOMEM;
	}

	if (r < 0) {
		return r;
	}

	/* If requested max buffer length (*len == 0) allocated buffer might be
	 * shorter than HEADER_SIZE. In such circumstances drop the buffer
	 * and return error.
	 */
	*user_len = get_buffer_length_to_pass(icmsg_len);

	if (!(*user_len)) {
		r = icmsg_drop_tx_buffer(conf, &data->icmsg_data, icmsg_buffer);
		__ASSERT_NO_MSG(!r);
		return -ENOBUFS;
	}

	*buffer = icmsg_buffer_to_user_buffer(icmsg_buffer);
	return 0;

}

int icmsg_me_drop_tx_buffer(const struct icmsg_config_t *conf,
			    struct icmsg_me_data_t *data,
			    const void *buffer)
{
	const void *buffer_to_drop = user_buffer_to_icmsg_buffer(buffer);

	return icmsg_drop_tx_buffer(conf, &data->icmsg_data, buffer_to_drop);
}

int icmsg_me_send_nocopy(const struct icmsg_config_t *conf,
			 struct icmsg_me_data_t *data, icmsg_me_ept_id_t id,
			 const void *msg, size_t len)
{
	void *buffer_to_send;
	size_t len_to_send;
	int r;
	int sent_bytes;

	buffer_to_send = user_buffer_to_icmsg_buffer(msg);
	len_to_send = user_buffer_len_to_icmsg_buffer_len(len);

	set_ept_id_in_send_buffer(buffer_to_send, id);

	r = icmsg_send_nocopy(conf, &data->icmsg_data,
			      buffer_to_send, len_to_send);

	if (r < 0) {
		return r;
	}

	__ASSERT_NO_MSG(r >= HEADER_SIZE);
	if (r < HEADER_SIZE) {
		return 0;
	}

	sent_bytes = icmsg_buffer_len_to_user_buffer_len(r);

	return sent_bytes;
}

#ifdef CONFIG_IPC_SERVICE_ICMSG_ME_NOCOPY_RX
int icmsg_me_hold_rx_buffer(const struct icmsg_config_t *conf,
			    struct icmsg_me_data_t *data, void *buffer)
{
	void *icmsg_buffer = user_buffer_to_icmsg_buffer(buffer);

	return icmsg_hold_rx_buffer(conf, &data->icmsg_data, icmsg_buffer);
}

int icmsg_me_release_rx_buffer(const struct icmsg_config_t *conf,
			       struct icmsg_me_data_t *data, void *buffer)
{
	void *icmsg_buffer = user_buffer_to_icmsg_buffer(buffer);

	return icmsg_release_rx_buffer(conf, &data->icmsg_data, icmsg_buffer);
}
#endif /* CONFIG_IPC_SERVICE_ICMSG_ME_NOCOPY_RX */
