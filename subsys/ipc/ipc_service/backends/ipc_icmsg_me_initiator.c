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

#define DT_DRV_COMPAT	zephyr_ipc_icmsg_me_initiator

#define NUM_EP        CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_NUM_EP
#define EP_NAME_LEN   CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_EP_NAME_LEN

struct backend_data_t {
	struct icmsg_me_data_t icmsg_me_data;

	struct k_mutex epts_mutex;
	icmsg_me_ept_id_t ids[NUM_EP];
};

static void bound(void *priv)
{
	const struct device *instance = priv;
	struct backend_data_t *dev_data = instance->data;

	icmsg_me_icmsg_bound(&dev_data->icmsg_me_data);
}

static void received(const void *data, size_t len, void *priv)
{
	const struct device *instance = priv;
	struct backend_data_t *dev_data = instance->data;

	const icmsg_me_ept_id_t *id = data;

	__ASSERT_NO_MSG(len > 0);

	if (*id == 0) {
		const struct ipc_ept_cfg *ept;
		int r;

		__ASSERT_NO_MSG(len > 1);

		id++;
		icmsg_me_ept_id_t ept_id = *id;

		r = icmsg_me_get_ept_cfg(&dev_data->icmsg_me_data, ept_id,
					 &ept);
		if (r < 0) {
			return;
		}

		if (ept && ept->cb.bound) {
			ept->cb.bound(ept->priv);
		}
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

static int store_id_for_token(struct backend_data_t *data, icmsg_me_ept_id_t id,
			      void **token)
{
	int i;

	for (i = 0; i < NUM_EP; i++) {
		if (data->ids[i] == 0) {
			break;
		}
	}
	__ASSERT_NO_MSG(i < NUM_EP);
	if (i >= NUM_EP) {
		return -ENOENT;
	}

	data->ids[i] = id;
	*token = &data->ids[i];

	return 0;
}

static int register_ept(const struct device *instance, void **token,
			const struct ipc_ept_cfg *cfg)
{
	const struct icmsg_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	int r = 0;
	icmsg_me_ept_id_t id;
	size_t name_len = strlen(cfg->name);

	if (name_len > EP_NAME_LEN) {
		return -EINVAL;
	}

	k_mutex_lock(&data->epts_mutex, K_FOREVER);

	r = icmsg_me_set_empty_ept_cfg_slot(&data->icmsg_me_data, cfg, &id);
	if (r < 0) {
		goto exit;
	}
	__ASSERT_NO_MSG(id > 0);
	if (id <= 0) {
		r = -ENOENT;
		goto reset_slot;
	}

	r = store_id_for_token(data, id, token);
	if (r < 0) {
		goto reset_slot;
	}

	uint8_t ep_disc_req[EP_NAME_LEN + 2 * sizeof(icmsg_me_ept_id_t)] = {
		0,  /* EP discovery endpoint id */
		id, /* Bound endpoint id */
	};
	memcpy(&ep_disc_req[2], cfg->name, name_len);

	icmsg_me_wait_for_icmsg_bind(&data->icmsg_me_data);

	r = icmsg_send(conf, &data->icmsg_me_data.icmsg_data, ep_disc_req,
		       2 * sizeof(icmsg_me_ept_id_t) + name_len);
	if (r < 0) {
		goto reset_slot;
	} else {
		r = 0;
	}

reset_slot:
	if (r < 0) {
		icmsg_me_reset_ept_cfg(&data->icmsg_me_data, id);
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
	icmsg_me_ept_id_t *id = token;

	return icmsg_me_send(conf, &dev_data->icmsg_me_data, *id, msg, len);
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

	k_mutex_init(&dev_data->epts_mutex);

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
