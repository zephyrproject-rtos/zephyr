/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/ipc/icmsg.h>
#include <zephyr/ipc/ipc_service_backend.h>

#define DT_DRV_COMPAT	zephyr_ipc_icmsg_me_follower

#define INVALID_EPT_ID 255
#define SEND_BUF_SIZE CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_SEND_BUF_SIZE
#define NUM_EP        CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_NUM_EP
#define EP_NAME_LEN   CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_EP_NAME_LEN

#define EVENT_BOUND 0x01

/* If more bytes than 1 was used for endpoint id, endianness should be
 * considered.
 */
typedef uint8_t ept_id_t;

struct ept_disc_rmt_cache_t {
	ept_id_t id;
	char name[EP_NAME_LEN];
};

struct backend_data_t {
	struct icmsg_data_t icmsg_data;
	struct ipc_ept_cfg ept_cfg;

	struct k_event event;

	struct k_mutex epts_mutex;
	struct k_mutex send_mutex;
	const struct ipc_ept_cfg *epts[NUM_EP];

	const struct ipc_ept_cfg *ept_disc_loc_cache[NUM_EP];
	struct ept_disc_rmt_cache_t ept_disc_rmt_cache[NUM_EP];

	uint8_t send_buffer[SEND_BUF_SIZE] __aligned(4);
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
			 size_t len, ept_id_t id)
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
		ept_id_t id)
{
	__ASSERT_NO_MSG(id <= NUM_EP);

	int r;
	int i = id - 1;
	const uint8_t confirmation[] = {
		0,  /* EP discovery endpoint id */
		id, /* Bound endpoint id */
	};

	data->epts[i] = ept;

	k_event_wait(&data->event, EVENT_BOUND, false, K_FOREVER);
	r = icmsg_send(conf, &data->icmsg_data, confirmation,
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

	k_event_post(&dev_data->event, EVENT_BOUND);
}

static void received(const void *data, size_t len, void *priv)
{
	const struct device *instance = priv;
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;

	const ept_id_t *id = data;

	__ASSERT_NO_MSG(len > 0);

	if (*id == 0) {
		__ASSERT_NO_MSG(len > 1);

		id++;
		ept_id_t ept_id = *id;
		const char *name = id + 1;
		size_t name_len = len - 2 * sizeof(ept_id_t);

		k_mutex_lock(&dev_data->epts_mutex, K_FOREVER);

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

		k_mutex_unlock(&dev_data->epts_mutex);
	} else {
		int i = *id - 1;

		if (i >= NUM_EP) {
			return;
		}
		if (dev_data->epts[i] == NULL) {
			return;
		}
		if (dev_data->epts[i]->cb.received) {
			dev_data->epts[i]->cb.received(id + 1,
					len - sizeof(ept_id_t),
					dev_data->epts[i]->priv);
		}
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

	dev_data->ept_cfg.cb = cb;
	dev_data->ept_cfg.priv = (void *)instance;

	return icmsg_open(conf, &dev_data->icmsg_data, &dev_data->ept_cfg.cb,
		dev_data->ept_cfg.priv);
}

static int register_ept(const struct device *instance, void **token,
			const struct ipc_ept_cfg *cfg)
{
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	struct ept_disc_rmt_cache_t *rmt_cache_entry;
	int r;

	k_mutex_lock(&data->epts_mutex, K_FOREVER);

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
		ept_id_t ept_id = rmt_cache_entry->id;

		if (ept_id == INVALID_EPT_ID) {
			r = -EAGAIN;
			goto exit;
		}

		*token = &rmt_cache_entry->id;
		r = bind_ept(conf, data, cfg, ept_id);
	}

exit:
	k_mutex_unlock(&data->epts_mutex);
	return r;
}

static int send(const struct device *instance, void *token,
		const void *msg, size_t len)
{
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;
	ept_id_t *id = token;
	int r;
	int sent_bytes;

	if (*id == INVALID_EPT_ID) {
		return -ENOTCONN;
	}

	if (len >= SEND_BUF_SIZE - sizeof(ept_id_t)) {
		return -EBADMSG;
	}

	k_mutex_lock(&dev_data->send_mutex, K_FOREVER);

	/* TODO: Optimization: How to avoid this copying? */
	/* Scatter list supported by icmsg? */
	dev_data->send_buffer[0] = *id;
	memcpy(dev_data->send_buffer + sizeof(ept_id_t), msg, len);

	r = icmsg_send(conf, &dev_data->icmsg_data, dev_data->send_buffer,
			len + sizeof(ept_id_t));

	if (r > 0) {
		sent_bytes = r - 1;
	}

	k_mutex_unlock(&dev_data->send_mutex);

	if (r > 0) {
		return sent_bytes;
	} else {
		return r;
	}
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

	k_event_init(&dev_data->event);
	k_mutex_init(&dev_data->epts_mutex);
	k_mutex_init(&dev_data->send_mutex);

	return icmsg_init(conf, &dev_data->icmsg_data);
}

#define BACKEND_CONFIG_POPULATE(i)						\
	{									\
		.tx_shm_size = DT_REG_SIZE(DT_INST_PHANDLE(i, tx_region)),	\
		.tx_shm_addr = DT_REG_ADDR(DT_INST_PHANDLE(i, tx_region)),	\
		.rx_shm_size = DT_REG_SIZE(DT_INST_PHANDLE(i, rx_region)),	\
		.rx_shm_addr = DT_REG_ADDR(DT_INST_PHANDLE(i, rx_region)),	\
		.mbox_tx = MBOX_DT_CHANNEL_GET(DT_DRV_INST(i), tx),		\
		.mbox_rx = MBOX_DT_CHANNEL_GET(DT_DRV_INST(i), rx),		\
	}

#define DEFINE_BACKEND_DEVICE(i)						\
	static const struct icmsg_config_t backend_config_##i =			\
		BACKEND_CONFIG_POPULATE(i);					\
	static struct backend_data_t backend_data_##i;				\
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

#if defined(CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_SHMEM_RESET)
#define BACKEND_CONFIG_DEFINE(i) BACKEND_CONFIG_POPULATE(i),
static int shared_memory_prepare(const struct device *arg)
{
	const struct icmsg_config_t *backend_config;
	const struct icmsg_config_t backend_configs[] = {
		DT_INST_FOREACH_STATUS_OKAY(BACKEND_CONFIG_DEFINE)
	};

	for (backend_config = backend_configs;
	     backend_config < backend_configs + ARRAY_SIZE(backend_configs);
	     backend_config++) {
		icmsg_clear_tx_memory(backend_config);
		icmsg_clear_rx_memory(backend_config);
	}

	return 0;
}

SYS_INIT(shared_memory_prepare, PRE_KERNEL_1, 1);
#endif /* CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_SHMEM_RESET */
