/*
 * Copyright (c) 2024 A Labs GmbH
 * Copyright (c) 2024 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/lorawan/emul.h>
#include <zephyr/lorawan/lorawan.h>

#include <errno.h>

#include <LoRaMac.h>
#include <Region.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lorawan_emul, CONFIG_LORAWAN_LOG_LEVEL);

static bool lorawan_adr_enable;

static sys_slist_t dl_callbacks;

static DeviceClass_t current_class;

static lorawan_battery_level_cb_t battery_level_cb;
static lorawan_dr_changed_cb_t dr_changed_cb;
static lorawan_uplink_cb_t uplink_cb;

/* implementation required by the soft-se (software secure element) */
void BoardGetUniqueId(uint8_t *id)
{
	/* Do not change the default value */
}

void lorawan_emul_send_downlink(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
				uint8_t len, const uint8_t *data)
{
	struct lorawan_downlink_cb *cb;

	/* Iterate over all registered downlink callbacks */
	SYS_SLIST_FOR_EACH_CONTAINER(&dl_callbacks, cb, node) {
		if ((cb->port == LW_RECV_PORT_ANY) || (cb->port == port)) {
			cb->cb(port, data_pending, rssi, snr, len, data);
		}
	}
}

int lorawan_join(const struct lorawan_join_config *join_cfg)
{
	return 0;
}

int lorawan_set_class(enum lorawan_class dev_class)
{
	switch (dev_class) {
	case LORAWAN_CLASS_A:
		current_class = CLASS_A;
		break;
	case LORAWAN_CLASS_B:
		LOG_ERR("Class B not supported yet!");
		return -ENOTSUP;
	case LORAWAN_CLASS_C:
		current_class = CLASS_C;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int lorawan_set_datarate(enum lorawan_datarate dr)
{
	ARG_UNUSED(dr);

	/* Bail out if using ADR */
	if (lorawan_adr_enable) {
		return -EINVAL;
	}

	return 0;
}

void lorawan_get_payload_sizes(uint8_t *max_next_payload_size, uint8_t *max_payload_size)
{
	LoRaMacTxInfo_t tx_info;

	/* QueryTxPossible cannot fail */
	(void)LoRaMacQueryTxPossible(0, &tx_info);

	*max_next_payload_size = tx_info.MaxPossibleApplicationDataSize;
	*max_payload_size = tx_info.CurrentPossiblePayloadSize;
}

enum lorawan_datarate lorawan_get_min_datarate(void)
{
	return LORAWAN_DR_0;
}

void lorawan_enable_adr(bool enable)
{
	lorawan_adr_enable = enable;
}

int lorawan_set_conf_msg_tries(uint8_t tries)
{
	return 0;
}

int lorawan_send(uint8_t port, uint8_t *data, uint8_t len, enum lorawan_message_type type)
{
	if (data == NULL) {
		return -EINVAL;
	}

	if (uplink_cb != NULL) {
		uplink_cb(port, len, data);
	}

	return 0;
}

void lorawan_register_battery_level_callback(lorawan_battery_level_cb_t cb)
{
	battery_level_cb = cb;
}

void lorawan_register_downlink_callback(struct lorawan_downlink_cb *cb)
{
	sys_slist_append(&dl_callbacks, &cb->node);
}

void lorawan_register_dr_changed_callback(lorawan_dr_changed_cb_t cb)
{
	dr_changed_cb = cb;
}

int lorawan_start(void)
{
	return 0;
}

static int lorawan_init(void)
{
	sys_slist_init(&dl_callbacks);

	return 0;
}

void lorawan_emul_register_uplink_callback(lorawan_uplink_cb_t cb)
{
	uplink_cb = cb;
}

SYS_INIT(lorawan_init, POST_KERNEL, 0);
