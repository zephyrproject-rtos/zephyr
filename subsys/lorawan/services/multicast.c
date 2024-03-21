/*
 * Copyright (c) 2022-2024 Martin Jäger <martin@libre.solar>
 * Copyright (c) 2022-2024 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lorawan_services.h"
#include "../lw_priv.h"

#include <LoRaMac.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include <stdio.h>

LOG_MODULE_REGISTER(lorawan_multicast, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

/**
 * Version of LoRaWAN Remote Multicast Setup Specification
 *
 * This implementation only supports TS005-1.0.0.
 */
#define MULTICAST_PACKAGE_VERSION 1

/**
 * Maximum expected number of multicast commands per packet
 *
 * The standard states "A message MAY carry more than one command". Even though this was not
 * observed during testing, space for up to 3 packages is reserved.
 */
#define MAX_MULTICAST_CMDS_PER_PACKAGE 3

/** Maximum length of multicast answers */
#define MAX_MULTICAST_ANS_LEN 5

enum multicast_commands {
	MULTICAST_CMD_PKG_VERSION = 0x00,
	MULTICAST_CMD_MC_GROUP_STATUS = 0x01,
	MULTICAST_CMD_MC_GROUP_SETUP = 0x02,
	MULTICAST_CMD_MC_GROUP_DELETE = 0x03,
	MULTICAST_CMD_MC_CLASS_C_SESSION = 0x04,
	MULTICAST_CMD_MC_CLASS_B_SESSION = 0x05,
};

struct multicast_context {
	struct k_work_delayable session_start_work;
	struct k_work_delayable session_stop_work;
};

static struct multicast_context ctx[LORAMAC_MAX_MC_CTX];

static void multicast_session_start(struct k_work *work)
{
	int ret;

	ret = lorawan_services_class_c_start();
	if (ret < 0) {
		LOG_WRN("Failed to switch to class C: %d. Retrying in 1s.", ret);
		lorawan_services_reschedule_work(k_work_delayable_from_work(work), K_SECONDS(1));
	}
}

static void multicast_session_stop(struct k_work *work)
{
	int ret;

	ret = lorawan_services_class_c_stop();
	if (ret < 0) {
		LOG_WRN("Failed to revert to class A: %d. Retrying in 1s.", ret);
		lorawan_services_reschedule_work(k_work_delayable_from_work(work), K_SECONDS(1));
	}
}

/**
 * Schedule Class C session if valid timing is found
 *
 * @returns time to start (negative in case of missed start)
 */
static int32_t multicast_schedule_class_c_session(uint8_t id, uint32_t session_time,
						  uint32_t session_timeout)
{
	uint32_t current_time;
	int32_t time_to_start;
	int err;

	err = lorawan_clock_sync_get(&current_time);
	time_to_start = session_time - current_time;

	if (err != 0 || time_to_start > 0xFFFFFF) {
		LOG_ERR("Clocks not synchronized, cannot schedule class C session");

		/* truncate value to indicates that clocks are out of sync */
		time_to_start = 0xFFFFFF;
	} else if (time_to_start >= 0) {
		LOG_DBG("Starting class C session in %d s", time_to_start);

		lorawan_services_reschedule_work(&ctx[id].session_start_work,
						 K_SECONDS(time_to_start));

		lorawan_services_reschedule_work(&ctx[id].session_stop_work,
						 K_SECONDS(time_to_start + session_timeout));
	}

	return time_to_start;
}

static void multicast_package_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
				       uint8_t len, const uint8_t *rx_buf)
{
	uint8_t tx_buf[MAX_MULTICAST_CMDS_PER_PACKAGE * MAX_MULTICAST_ANS_LEN];
	uint8_t tx_pos = 0;
	uint8_t rx_pos = 0;

	__ASSERT(port == LORAWAN_PORT_MULTICAST_SETUP, "Wrong port %d", port);

	while (rx_pos < len) {
		uint8_t command_id = rx_buf[rx_pos++];

		if (sizeof(tx_buf) - tx_pos < MAX_MULTICAST_ANS_LEN) {
			LOG_ERR("insufficient tx_buf size, some requests discarded");
			break;
		}

		switch (command_id) {
		case MULTICAST_CMD_PKG_VERSION:
			tx_buf[tx_pos++] = MULTICAST_CMD_PKG_VERSION;
			tx_buf[tx_pos++] = LORAWAN_PACKAGE_ID_REMOTE_MULTICAST_SETUP;
			tx_buf[tx_pos++] = MULTICAST_PACKAGE_VERSION;
			LOG_DBG("PackageVersionReq");
			break;
		case MULTICAST_CMD_MC_GROUP_STATUS:
			LOG_ERR("McGroupStatusReq not implemented");
			return;
		case MULTICAST_CMD_MC_GROUP_SETUP: {
			uint8_t id = rx_buf[rx_pos++] & 0x03;
			McChannelParams_t channel = {
				.IsRemotelySetup = true,
				.IsEnabled = true,
				.GroupID = (AddressIdentifier_t)id,
				.RxParams = {0},
			};

			channel.Address = sys_get_le32(rx_buf + rx_pos);
			rx_pos += sizeof(uint32_t);

			/* the key is copied in LoRaMacMcChannelSetup (cast to discard const) */
			channel.McKeys.McKeyE = (uint8_t *)rx_buf + rx_pos;
			rx_pos += 16;

			channel.FCountMin = sys_get_le32(rx_buf + rx_pos);
			rx_pos += sizeof(uint32_t);

			channel.FCountMax = sys_get_le32(rx_buf + rx_pos);
			rx_pos += sizeof(uint32_t);

			LOG_DBG("McGroupSetupReq id: %u, addr: 0x%.8X, fcnt_min: %u, fcnt_max: %u",
				id, channel.Address, channel.FCountMin, channel.FCountMax);

			LoRaMacStatus_t ret = LoRaMacMcChannelSetup(&channel);

			tx_buf[tx_pos++] = MULTICAST_CMD_MC_GROUP_SETUP;
			if (ret == LORAMAC_STATUS_OK) {
				tx_buf[tx_pos++] = id;
			} else if (ret == LORAMAC_STATUS_MC_GROUP_UNDEFINED) {
				/* set IDerror flag */
				tx_buf[tx_pos++] = (1U << 2) | id;
			} else {
				LOG_ERR("McGroupSetupReq failed: %s", lorawan_status2str(ret));
				return;
			}
			break;
		}
		case MULTICAST_CMD_MC_GROUP_DELETE: {
			uint8_t id = rx_buf[rx_pos++] & 0x03;

			LoRaMacStatus_t ret = LoRaMacMcChannelDelete((AddressIdentifier_t)id);

			LOG_DBG("McGroupDeleteReq id: %d", id);

			tx_buf[tx_pos++] = MULTICAST_CMD_MC_GROUP_DELETE;
			if (ret == LORAMAC_STATUS_OK) {
				tx_buf[tx_pos++] = id;
			} else if (ret == LORAMAC_STATUS_MC_GROUP_UNDEFINED) {
				/* set McGroupUndefined flag */
				tx_buf[tx_pos++] = (1U << 2) | id;
			} else {
				LOG_ERR("McGroupDeleteReq failed: %s", lorawan_status2str(ret));
				return;
			}
			break;
		}
		case MULTICAST_CMD_MC_CLASS_C_SESSION: {
			uint32_t session_time;
			uint32_t session_timeout;
			uint8_t status = 0x00;
			uint8_t id = rx_buf[rx_pos++] & 0x03;
			McRxParams_t rx_params;

			session_time = sys_get_le32(rx_buf + rx_pos);
			rx_pos += sizeof(uint32_t);

			session_timeout = 1U << (rx_buf[rx_pos++] & 0x0F);

			rx_params.Class = CLASS_C;

			rx_params.Params.ClassC.Frequency = sys_get_le24(rx_buf + rx_pos) * 100;
			rx_pos += 3;

			rx_params.Params.ClassC.Datarate = rx_buf[rx_pos++];

			LOG_DBG("McClassCSessionReq time: %u, timeout: %u, freq: %u, DR: %d",
				session_time, session_timeout, rx_params.Params.ClassC.Frequency,
				rx_params.Params.ClassC.Datarate);

			LoRaMacStatus_t ret = LoRaMacMcChannelSetupRxParams((AddressIdentifier_t)id,
									    &rx_params, &status);

			tx_buf[tx_pos++] = MULTICAST_CMD_MC_CLASS_C_SESSION;
			if (ret == LORAMAC_STATUS_OK) {
				int32_t time_to_start;

				time_to_start = multicast_schedule_class_c_session(id, session_time,
										   session_timeout);
				if (time_to_start >= 0) {
					tx_buf[tx_pos++] = status;
					sys_put_le24(time_to_start, tx_buf + tx_pos);
					tx_pos += 3;
				} else {
					LOG_ERR("Missed class C session start at %d in %d s",
						session_time, time_to_start);
					/* set StartMissed flag */
					tx_buf[tx_pos++] = (1U << 5) | status;
				}
			} else {
				LOG_ERR("McClassCSessionReq failed: %s", lorawan_status2str(ret));
				if (ret == LORAMAC_STATUS_MC_GROUP_UNDEFINED) {
					/* set McGroupUndefined flag */
					tx_buf[tx_pos++] = (1U << 4) | status;
				} else if (ret == LORAMAC_STATUS_FREQ_AND_DR_INVALID) {
					/* set FreqError and DR Error flags */
					tx_buf[tx_pos++] = (3U << 2) | status;
					return;
				}
			}
			break;
		}
		case MULTICAST_CMD_MC_CLASS_B_SESSION:
			LOG_ERR("McClassBSessionReq not implemented");
			return;
		default:
			return;
		}
	}

	if (tx_pos > 0) {
		/* Random delay 2+-1 seconds according to RP002-1.0.3, chapter 2.3 */
		uint32_t delay = 1 + sys_rand32_get() % 3;

		lorawan_services_schedule_uplink(LORAWAN_PORT_MULTICAST_SETUP, tx_buf, tx_pos,
						 delay);
	}
}

static struct lorawan_downlink_cb downlink_cb = {
	.port = (uint8_t)LORAWAN_PORT_MULTICAST_SETUP,
	.cb = multicast_package_callback,
};

static int multicast_init(void)
{
	for (int i = 0; i < ARRAY_SIZE(ctx); i++) {
		k_work_init_delayable(&ctx[i].session_start_work, multicast_session_start);
		k_work_init_delayable(&ctx[i].session_stop_work, multicast_session_stop);
	}

	lorawan_register_downlink_callback(&downlink_cb);

	return 0;
}

/* initialization must be after lorawan_init in lorawan.c */
SYS_INIT(multicast_init, POST_KERNEL, 1);
