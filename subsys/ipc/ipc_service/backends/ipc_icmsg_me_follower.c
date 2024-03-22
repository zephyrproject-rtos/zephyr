/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/ipc/icmsg.h>
#include <zephyr/ipc/icmsg_me.h>
#include <zephyr/ipc/ipc_service_backend.h>

#define DT_DRV_COMPAT	zephyr_ipc_icmsg_me_follower

#define INVALID_EPT_ID 255
#define SEND_BUF_SIZE CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_SEND_BUF_SIZE
#define NUM_EP        CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_NUM_EP
#define EP_NAME_LEN   CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_EP_NAME_LEN

#define EVENT_BOUND 0x01

struct ept_disc_rmt_cache_t {
	icmsg_me_ept_id_t id;
	char name[EP_NAME_LEN];
};

struct backend_data_t {
	struct icmsg_me_data_t icmsg_me_data;

	struct k_mutex cache_mutex;
	const struct ipc_ept_cfg *ept_disc_loc_cache[NUM_EP];
	struct ept_disc_rmt_cache_t ept_disc_rmt_cache[NUM_EP];
};

static const struct ipc_ept_cfg *get_ept_cached_loc(
		const struct backend_data_t *data, const char *name, size_t len)
{
	for (int i = 0; i < NUM_EP; i++) {
		if (data->ept_disc_loc_cache[i] == NULL) {
			continue;
		}
		if (strncmp(data->ept_disc_loc_cache[i]->name, name,
				MIN(EP_NAME_LEN, len)) == 0) {
			return data->ept_disc_loc_cache[i];
		}
	}

	return NULL;
}

static struct ept_disc_rmt_cache_t *get_ept_cached_rmt(
		struct backend_data_t *data, const char *name, size_t len)
{
	size_t cmp_len = MIN(EP_NAME_LEN, len);

	for (int i = 0; i < NUM_EP; i++) {
		if (strncmp(data->ept_disc_rmt_cache[i].name, name,
				cmp_len) == 0 &&
		    strlen(data->ept_disc_rmt_cache[i].name) == len) {
			return &data->ept_disc_rmt_cache[i];
		}
	}

	return NULL;
}

static int cache_ept_loc(struct backend_data_t *data, const struct ipc_ept_cfg *ept)
{
	for (int i = 0; i < NUM_EP; i++) {
		if (data->ept_disc_loc_cache[i] == NULL) {
			data->ept_disc_loc_cache[i] = ept;
			return 0;
		}
	}

	return -ENOMEM;
}

static int cache_ept_rmt(struct backend_data_t *data, const char *name,
			 size_t len, icmsg_me_ept_id_t id)
{
	for (int i = 0; i < NUM_EP; i++) {
		if (!strlen(data->ept_disc_rmt_cache[i].name)) {
			size_t copy_len = MIN(EP_NAME_LEN - 1, len);

			strncpy(data->ept_disc_rmt_cache[i].name, name,
					copy_len);
			data->ept_disc_rmt_cache[i].name[copy_len] = '\0';
			data->ept_disc_rmt_cache[i].id = id;
			return 0;
		}
	}

	return -ENOMEM;
}

static int bind_ept(const struct icmsg_config_t *conf,
		struct backend_data_t *data, const struct ipc_ept_cfg *ept,
		icmsg_me_ept_id_t id)
{
	__ASSERT_NO_MSG(id <= NUM_EP);

	int r;
	const uint8_t confirmation[] = {
		0,  /* EP discovery endpoint id */
		id, /* Bound endpoint id */
	};

	r = icmsg_me_set_ept_cfg(&data->icmsg_me_data, id, ept);
	if (r < 0) {
		return r;
	}

	icmsg_me_wait_for_icmsg_bind(&data->icmsg_me_data);
	r = icmsg_send(conf, &data->icmsg_me_data.icmsg_data, confirmation,
		       sizeof(confirmation));
	if (r < 0) {
		return r;
	}

	if (ept->cb.bound) {
		ept->cb.bound(ept->priv);
	}

	return 0;
}

static void bound(void *priv)
{
	const struct device *instance = priv;
	struct backend_data_t *dev_data = instance->data;

	icmsg_me_icmsg_bound(&dev_data->icmsg_me_data);
}

static void received(const void *data, size_t len, void *priv)
{
	const struct device *instance = priv;
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;

	const icmsg_me_ept_id_t *id = data;

	__ASSERT_NO_MSG(len > 0);

	if (*id == 0) {
		__ASSERT_NO_MSG(len > 1);

		id++;
		icmsg_me_ept_id_t ept_id = *id;
		const char *name = id + 1;
		size_t name_len = len - 2 * sizeof(icmsg_me_ept_id_t);

		k_mutex_lock(&dev_data->cache_mutex, K_FOREVER);

		const struct ipc_ept_cfg *ept =
				get_ept_cached_loc(dev_data, name, name_len);
		if (ept == NULL) {
			cache_ept_rmt(dev_data, name, name_len, ept_id);
		} else {
			/* Remote cache entry should be already filled during
			 * the local cache entry registration. Update its
			 * id for correct token value.
			 */
			struct ept_disc_rmt_cache_t *rmt_cache_entry;

			rmt_cache_entry = get_ept_cached_rmt(dev_data, name,
							     name_len);

			__ASSERT_NO_MSG(rmt_cache_entry != NULL);
			__ASSERT_NO_MSG(rmt_cache_entry->id == INVALID_EPT_ID);
			rmt_cache_entry->id = ept_id;

			bind_ept(conf, dev_data, ept, ept_id);
		}

		k_mutex_unlock(&dev_data->cache_mutex);
	} else {
		icmsg_me_received_data(&dev_data->icmsg_me_data, *id,
				       data, len);
	}
}

static const struct ipc_service_cb cb = {
	.bound = bound,
	.received = received,
	.error = NULL,
};

static int open(const struct device *instance)
{
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;

	return icmsg_me_open(conf, &dev_data->icmsg_me_data, &cb,
			     (void *)instance);
}

static int register_ept(const struct device *instance, void **token,
			const struct ipc_ept_cfg *cfg)
{
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	struct ept_disc_rmt_cache_t *rmt_cache_entry;
	int r;

	k_mutex_lock(&data->cache_mutex, K_FOREVER);

	rmt_cache_entry = get_ept_cached_rmt(data, cfg->name,
					     strlen(cfg->name));
	if (rmt_cache_entry == NULL) {
		r = cache_ept_loc(data, cfg);
		if (r) {
			goto exit;
		}

		/* Register remote cache entry alongside the local one to
		 * make available endpoint id storage required for token.
		 */
		r = cache_ept_rmt(data, cfg->name, strlen(cfg->name),
				INVALID_EPT_ID);
		if (r) {
			goto exit;
		}

		rmt_cache_entry = get_ept_cached_rmt(data, cfg->name,
						     strlen(cfg->name));
		__ASSERT_NO_MSG(rmt_cache_entry != NULL);
		*token = &rmt_cache_entry->id;
	} else {
		icmsg_me_ept_id_t ept_id = rmt_cache_entry->id;

		if (ept_id == INVALID_EPT_ID) {
			r = -EAGAIN;
			goto exit;
		}

		*token = &rmt_cache_entry->id;
		r = bind_ept(conf, data, cfg, ept_id);
	}

exit:
	k_mutex_unlock(&data->cache_mutex);
	return r;
}

static int send(const struct device *instance, void *token,
		const void *msg, size_t user_len)
{
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;
	icmsg_me_ept_id_t *id = token;

	if (*id == INVALID_EPT_ID) {
		return -ENOTCONN;
	}

	return icmsg_me_send(conf, &dev_data->icmsg_me_data, *id, msg,
			     user_len);
}

const static struct ipc_service_backend backend_ops = {
	.open_instance = open,
	.register_endpoint = register_ept,
	.send = send,
};

static int backend_init(const struct device *instance)
{
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;

	k_mutex_init(&dev_data->cache_mutex);

	return icmsg_me_init(conf, &dev_data->icmsg_me_data);
}

#define DEFINE_BACKEND_DEVICE(i)						\
	static const struct icmsg_config_t backend_config_##i = {		\
		.mbox_tx = MBOX_DT_SPEC_INST_GET(i, tx),			\
		.mbox_rx = MBOX_DT_SPEC_INST_GET(i, rx),			\
	};									\
										\
	PBUF_DEFINE(tx_pb_##i,							\
			DT_REG_ADDR(DT_INST_PHANDLE(i, tx_region)),		\
			DT_REG_SIZE(DT_INST_PHANDLE(i, tx_region)),		\
			DT_INST_PROP_OR(i, dcache_alignment, 0));		\
	PBUF_DEFINE(rx_pb_##i,							\
			DT_REG_ADDR(DT_INST_PHANDLE(i, rx_region)),		\
			DT_REG_SIZE(DT_INST_PHANDLE(i, rx_region)),		\
			DT_INST_PROP_OR(i, dcache_alignment, 0));		\
										\
	static struct backend_data_t backend_data_##i = {			\
		.icmsg_me_data = {						\
			.icmsg_data = {						\
				.tx_pb = &tx_pb_##i,				\
				.rx_pb = &rx_pb_##i,				\
			}							\
		}								\
	};									\
										\
	DEVICE_DT_INST_DEFINE(i,						\
			 &backend_init,						\
			 NULL,							\
			 &backend_data_##i,					\
			 &backend_config_##i,					\
			 POST_KERNEL,						\
			 CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY,		\
			 &backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_DEVICE)
