/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_w91_trng

#include <zephyr/drivers/entropy.h>
#include <ipc/ipc_based_driver.h>

enum {
	IPC_DISPATCHER_TRNG_GET_ENTROPY = IPC_DISPATCHER_ENTROPY_TRNG,
};

struct entropy_w91_config {
	uint8_t instance_id;            /* instance id */
};

struct entropy_w91_data {
	struct ipc_based_driver ipc;    /* ipc driver part */
};

struct entropy_w91_trng_get_entropy_resp {
	int err;
	uint16_t length;
	uint8_t *buffer;
};

#define ENTROPY_TRNG_MAX_SIZE_IN_PACK                                                      \
((CONFIG_PBUF_RX_READ_BUF_SIZE) - sizeof(uint32_t) - sizeof(int) - sizeof(uint16_t))

/* API implementation: driver initialization */
static int entropy_w91_trng_init(const struct device *dev)
{
	struct entropy_w91_data *data = dev->data;

	ipc_based_driver_init(&data->ipc);

	return 0;
}

/* APIs implementation: get entropy */
static size_t pack_entropy_w91_trng_get_entropy(
	uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	uint16_t *p_length = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(*p_length);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_TRNG_GET_ENTROPY, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, *p_length);
	}

	return pack_data_len;
}

static void unpack_entropy_w91_trng_get_entropy(void *unpack_data,
		const uint8_t *pack_data, size_t pack_data_len)
{
	struct entropy_w91_trng_get_entropy_resp *p_trng_get_entropy_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_trng_get_entropy_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_trng_get_entropy_resp->length);

	size_t expect_len = sizeof(uint32_t) + sizeof(p_trng_get_entropy_resp->err) +
			sizeof(p_trng_get_entropy_resp->length) + p_trng_get_entropy_resp->length;

	if (expect_len != pack_data_len) {
		p_trng_get_entropy_resp->err = -EINVAL;
		return;
	}

	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_trng_get_entropy_resp->buffer,
			p_trng_get_entropy_resp->length);
}

static int entropy_w91_trng_get_entropy(const struct device *dev,
					uint8_t *buffer, uint16_t length)
{
	struct entropy_w91_trng_get_entropy_resp trng_get_entropy_resp = {
		.buffer	= buffer,
	};

	struct ipc_based_driver *ipc_data = &((struct entropy_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct entropy_w91_config *)dev->config)->instance_id;
	uint16_t entropy_length;

	while (length) {
		if (length > ENTROPY_TRNG_MAX_SIZE_IN_PACK) {
			entropy_length = ENTROPY_TRNG_MAX_SIZE_IN_PACK;
		} else {
			entropy_length = length;
		}

		IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
				entropy_w91_trng_get_entropy,
				&entropy_length, &trng_get_entropy_resp,
				CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

		if (trng_get_entropy_resp.err || (length <= ENTROPY_TRNG_MAX_SIZE_IN_PACK)) {
			break;
		}

		trng_get_entropy_resp.buffer += entropy_length;
		length -= entropy_length;
	}

	return trng_get_entropy_resp.err;
}

/* API implementation: get_entropy_isr */
static int entropy_w91_trng_get_entropy_isr(const struct device *dev,
					    uint8_t *buffer, uint16_t length,
					    uint32_t flags)
{
	/* No supported handling in case of running from ISR */
	return -ENOTSUP;
}

/* Entropy driver APIs structure */
static const struct entropy_driver_api entropy_w91_trng_api = {
	.get_entropy = entropy_w91_trng_get_entropy,
	.get_entropy_isr = entropy_w91_trng_get_entropy_isr
};

/* Entropy driver registration */
#define ENTROPY_W91_INIT(n)                                             \
	static const struct entropy_w91_config entropy_w91_config_##n = {   \
		.instance_id = n,                                               \
	};                                                                  \
                                                                        \
	static struct entropy_w91_data entropy_w91_data_##n;                \
                                                                        \
	DEVICE_DT_INST_DEFINE(n, entropy_w91_trng_init,                     \
			      NULL,                                                 \
			      &entropy_w91_data_##n,                                \
			      &entropy_w91_config_##n,                              \
			      POST_KERNEL,                                          \
			      CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,          \
			      &entropy_w91_trng_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_W91_INIT)
