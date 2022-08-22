/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 * Copyright (c) 2022 tado GmbH
 *
 * Parts of this implementation were inspired by LmhpRemoteMcastSetup.c from the
 * LoRaMac-node firmware repository https://github.com/Lora-net/LoRaMac-node
 * written by Miguel Luis (Semtech).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lorawan_services.h"
#include "../lw_priv.h"

#include <LoRaMac.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/rand32.h>
#include <zephyr/settings/settings.h>

#include <stdio.h>

LOG_MODULE_REGISTER(lorawan_multicast, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

/* maximum length of multicast answers */
#define MAX_MULTICAST_ANS_LEN 5

#define MULTICAST_SETTINGS_BASE "lorawan/multicast"

enum multicast_commands {
	MULTICAST_CMD_PKG_VERSION        = 0x00,
	MULTICAST_CMD_MC_GROUP_STATUS    = 0x01,
	MULTICAST_CMD_MC_GROUP_SETUP     = 0x02,
	MULTICAST_CMD_MC_GROUP_DELETE    = 0x03,
	MULTICAST_CMD_MC_CLASS_C_SESSION = 0x04,
	MULTICAST_CMD_MC_CLASS_B_SESSION = 0x05,
};

struct multicast_context {
	/* below group-specific settings have to be stored in non-volatile memory */
	struct {
		/**
		 * McAddr: multicast group network address
		 */
		uint32_t addr;
		/**
		 * McKey_encrypted: encrypted multicast group key used to derive
		 * McAppSKey and McNetSKey
		 */
		uint8_t key_encrypted[16];
		/**
		 * minMcFCount: next frame counter value of the multicast downlink to be sent
		 */
		uint32_t fcnt_min;
		/**
		 * maxMcFCount: lifetime of this multicast group expressed as a maximum number
		 * of frames
		 */
		uint32_t fcnt_max;
	} mc_group;
	/** Start of the Class C window as GPS epoch modulo 2^32 */
	uint32_t session_time;
	/** Maximum duration of MC session before device reverts to class A */
	uint32_t session_timeout;
	/** Receive parameters for MC session */
	McRxParams_t rx_params;

	struct k_work_delayable session_start_work;
	struct k_work_delayable session_stop_work;
};

static struct multicast_context ctx[LORAMAC_MAX_MC_CTX];

static struct k_work_q *workq;

static void multicast_settings_store(uint8_t group_id)
{
	char path[30];

	snprintf(path, sizeof(path), "%s/groups/%u", MULTICAST_SETTINGS_BASE, group_id);

	settings_save_one(path, &ctx[group_id].mc_group, sizeof(ctx[group_id].mc_group));
}

static int multicast_settings_load(const char *name, size_t len,
                        	   settings_read_cb read_cb, void *cb_arg)
{
	const char *remainder;
	int rc, group_id;

	if (settings_name_steq(name, "groups", &remainder)) {
		if (len != sizeof(ctx[0].mc_group)) {
			return -EINVAL;
		}

		group_id = remainder[0] - '0';
		if (group_id < 0 || group_id > ARRAY_SIZE(ctx)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &ctx[group_id].mc_group, len);
		if (rc >= 0) {
			return 0;
		}

		return rc;
	}

	return -ENOENT;
}

static struct settings_handler settings_conf = {
	.name = MULTICAST_SETTINGS_BASE,
	.h_set = multicast_settings_load,
};

static void multicast_session_start(struct k_work *work)
{
	int ret;

	ret = lorawan_services_class_c_start();
	if (ret < 0) {
		LOG_WRN("Failed to switch to class C: %d. Retrying in 1s.", ret);
		k_work_reschedule_for_queue(workq,
					    k_work_delayable_from_work(work), K_SECONDS(1));
	}
}

static void multicast_session_stop(struct k_work *work)
{
	int ret;

	ret = lorawan_services_class_c_stop();
	if (ret < 0) {
		LOG_WRN("Failed to revert to class A: %d. Retrying in 1s.", ret);
		k_work_reschedule_for_queue(workq,
					    k_work_delayable_from_work(work), K_SECONDS(1));
	}
}

static inline LoRaMacStatus_t multicast_group_setup(uint8_t id)
{
	__ASSERT_NO_MSG(id < ARRAY_SIZE(ctx));

	McChannelParams_t channel = {
		.IsRemotelySetup = true,
		.IsEnabled = true,
		.GroupID = (AddressIdentifier_t)id,
		.Address = ctx[id].mc_group.addr,
		.McKeys.McKeyE = ctx[id].mc_group.key_encrypted,
		.FCountMin = ctx[id].mc_group.fcnt_min,
		.FCountMax = ctx[id].mc_group.fcnt_max,
		.RxParams = {0}
	};

	return LoRaMacMcChannelSetup(&channel);
}

static void multicast_package_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
				       uint8_t len, const uint8_t *rx_buf)
{
	uint8_t tx_buf[3 * MAX_MULTICAST_ANS_LEN];
	uint8_t tx_pos = 0;
	uint8_t rx_pos = 0;

	__ASSERT(port == LORAWAN_PORT_MULTICAST, "Wrong port %d", port);

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
			tx_buf[tx_pos++] = CONFIG_LORAWAN_REMOTE_MULTICAST_VERSION;
			LOG_DBG("PackageVersionReq");
			break;
		case MULTICAST_CMD_MC_GROUP_STATUS:
			LOG_ERR("McGroupStatusReq not implemented");
			return;
		case MULTICAST_CMD_MC_GROUP_SETUP: {
			uint8_t id = rx_buf[rx_pos++] & 0x03;

			ctx[id].mc_group.addr = rx_buf[rx_pos++];
			ctx[id].mc_group.addr += (rx_buf[rx_pos++] << 8);
			ctx[id].mc_group.addr += (rx_buf[rx_pos++] << 16);
			ctx[id].mc_group.addr += (rx_buf[rx_pos++] << 24);

			for (int i = 0; i < 16; i++) {
				ctx[id].mc_group.key_encrypted[i] = rx_buf[rx_pos++];
			}

			ctx[id].mc_group.fcnt_min = rx_buf[rx_pos++];
			ctx[id].mc_group.fcnt_min += (rx_buf[rx_pos++] << 8);
			ctx[id].mc_group.fcnt_min += (rx_buf[rx_pos++] << 16);
			ctx[id].mc_group.fcnt_min += (rx_buf[rx_pos++] << 24);

			ctx[id].mc_group.fcnt_max = rx_buf[rx_pos++];
			ctx[id].mc_group.fcnt_max += (rx_buf[rx_pos++] << 8);
			ctx[id].mc_group.fcnt_max += (rx_buf[rx_pos++] << 16);
			ctx[id].mc_group.fcnt_max += (rx_buf[rx_pos++] << 24);

			LOG_DBG("McGroupSetupReq id: %u, addr: 0x%.8X, "
				"fcnt_min: %u, fcnt_max: %u", id, ctx[id].mc_group.addr,
				ctx[id].mc_group.fcnt_min, ctx[id].mc_group.fcnt_max);

			LoRaMacStatus_t ret = multicast_group_setup(id);

			tx_buf[tx_pos++] = MULTICAST_CMD_MC_GROUP_SETUP;
			if (ret == LORAMAC_STATUS_OK) {
				tx_buf[tx_pos++] = id;
				multicast_settings_store(id);
			} else if (ret == LORAMAC_STATUS_MC_GROUP_UNDEFINED) {
				/* set IDerror flag */
				tx_buf[tx_pos++] = (1U << 2) | id;
			} else {
				LOG_ERR("McGroupSetupReq failed: %s",
					lorawan_status2str(ret));
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
				multicast_settings_store(id);
			} else if (ret == LORAMAC_STATUS_MC_GROUP_UNDEFINED) {
				/* set McGroupUndefined flag */
				tx_buf[tx_pos++] = (1U << 2) | id;
			} else {
				LOG_ERR("McGroupDeleteReq failed: %s",
					lorawan_status2str(ret));
				return;
			}
			break;
		}
		case MULTICAST_CMD_MC_CLASS_C_SESSION: {
			uint8_t status = 0x00;
			uint8_t id = rx_buf[rx_pos++] & 0x03;

			ctx[id].session_time = rx_buf[rx_pos++];
			ctx[id].session_time += rx_buf[rx_pos++] << 8;
			ctx[id].session_time += rx_buf[rx_pos++] << 16;
			ctx[id].session_time += rx_buf[rx_pos++] << 24;

			ctx[id].session_timeout = 1U << (rx_buf[rx_pos++] & 0x0F);

			ctx[id].rx_params.Class = CLASS_C;

			ctx[id].rx_params.Params.ClassC.Frequency = rx_buf[rx_pos++];
			ctx[id].rx_params.Params.ClassC.Frequency += rx_buf[rx_pos++] << 8;
			ctx[id].rx_params.Params.ClassC.Frequency += rx_buf[rx_pos++] << 16;
			ctx[id].rx_params.Params.ClassC.Frequency *= 100;

			ctx[id].rx_params.Params.ClassC.Datarate = rx_buf[rx_pos++];

			LOG_DBG("McClassCSessionReq time: %u, timeout: %u, freq: %u, DR: %d",
				ctx[id].session_time, ctx[id].session_timeout,
				ctx[id].rx_params.Params.ClassC.Frequency,
				ctx[id].rx_params.Params.ClassC.Datarate);

			LoRaMacStatus_t ret = LoRaMacMcChannelSetupRxParams(
				(AddressIdentifier_t)id, &ctx[id].rx_params, &status);

			tx_buf[tx_pos++] = MULTICAST_CMD_MC_CLASS_C_SESSION;
			if (ret == LORAMAC_STATUS_OK) {
				uint32_t current_time;
				int32_t time_to_start;
				bool synchronized;

				synchronized = (lorawan_clock_sync_get(&current_time) == 0);
				time_to_start =	ctx[id].session_time - current_time;

				if (time_to_start >= 0) {
					if (time_to_start > 0xFFFFFF || !synchronized) {
						LOG_ERR("Clocks not synchronized, cannot "
							"schedule class C session");
						/* truncated value indicates that clocks are out
						 * of sync
						 */
						time_to_start = 0xFFFFFF;
					} else {
						LOG_DBG("Starting class C session in %d s",
							time_to_start);

						k_work_reschedule_for_queue(workq,
							&ctx[id].session_start_work,
							K_SECONDS(time_to_start));

						k_work_reschedule_for_queue(workq,
							&ctx[id].session_stop_work,
							K_SECONDS(time_to_start +
								  ctx[id].session_timeout));
					}

					tx_buf[tx_pos++] = status;
					tx_buf[tx_pos++] = (time_to_start >> 0) & 0xFF;
					tx_buf[tx_pos++] = (time_to_start >> 8) & 0xFF;
					tx_buf[tx_pos++] = (time_to_start >> 16) & 0xFF;
				} else {
					LOG_ERR("Missed class C session start at %d in %d s",
						ctx[id].session_time, time_to_start);
					/* set StartMissed flag */
					tx_buf[tx_pos++] = (1U << 5) | status;
				}
			} else {
				LOG_ERR("McClassCSessionReq failed: %s",
					lorawan_status2str(ret));
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

		lorawan_services_schedule_uplink(LORAWAN_PORT_MULTICAST, tx_buf, tx_pos,
						 K_SECONDS(delay));
	}
}

static struct lorawan_downlink_cb downlink_cb = {
	.port = (uint8_t)LORAWAN_PORT_MULTICAST,
	.cb = multicast_package_callback
};

int lorawan_remote_multicast_run(void)
{
	workq = lorawan_services_get_work_queue();

	settings_subsys_init();
	settings_register(&settings_conf);
	settings_load();

	for (int i = 0; i < ARRAY_SIZE(ctx); i++) {
		multicast_group_setup(i);
		LOG_DBG("mc_group %u init, addr: 0x%.8X, fcnt_min: %u, fcnt_max: %u", i,
			ctx[i].mc_group.addr, ctx[i].mc_group.fcnt_min,	ctx[i].mc_group.fcnt_max);
		k_work_init_delayable(&ctx[i].session_start_work, multicast_session_start);
		k_work_init_delayable(&ctx[i].session_stop_work, multicast_session_stop);
	}

	lorawan_register_downlink_callback(&downlink_cb);

	return 0;
}
