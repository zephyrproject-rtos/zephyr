/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 * Copyright (c) 2022 tado GmbH
 *
 * Parts of this implementation were inspired by LmhpClockSync.c from the
 * LoRaMac-node firmware repository https://github.com/Lora-net/LoRaMac-node
 * written by Miguel Luis (Semtech).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lorawan_services.h"

#include <LoRaMac.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(lorawan_clock_sync, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

/**
 * Version of LoRaWAN Application Layer Clock Synchronization Specification
 *
 * This implementation only supports TS003-2.0.0, as the previous revision TS003-1.0.0
 * requested to temporarily disable ADR and set nb_trans to 1. This causes issues on the
 * server side and is not recommended anymore.
 */
#define CLOCK_SYNC_PACKAGE_VERSION 2

/* Maximum length of clock sync answers */
#define MAX_CLOCK_SYNC_ANS_LEN 6

/* Delay between consecutive transmissions of AppTimeReq */
#define CLOCK_RESYNC_DELAY 10

enum clock_sync_commands {
	CLOCK_SYNC_CMD_PKG_VERSION                 = 0x00,
	CLOCK_SYNC_CMD_APP_TIME                    = 0x01,
	CLOCK_SYNC_CMD_DEVICE_APP_TIME_PERIODICITY = 0x02,
	CLOCK_SYNC_CMD_FORCE_DEVICE_RESYNC         = 0x03,
};

struct clock_sync_context {
	/** Work item for regular (re-)sync requests (uplink messages) */
	struct k_work_delayable resync_work;
	/** Continuously incremented token to map clock sync answers and requests */
	uint8_t req_token;
	/** Number of requested clock sync requests left to be transmitted */
	uint8_t nb_transmissions;
	/**
	 * Offset to be added to system uptime to get GPS time (as used by LoRaWAN)
	 */
	uint32_t time_offset;
	/**
	 * AppTimeReq retransmission interval in seconds
	 *
	 * Valid range between 128 (0x80) and 8388608 (0x800000)
	 */
	uint32_t periodicity;
	/** Indication if at least one valid time correction was received */
	bool synchronized;
};

static struct clock_sync_context ctx;

/**
 * Writes the DeviceTime into the buffer.
 *
 * @returns number of bytes written or -ENOSPC in case of error
 */
static int clock_sync_serialize_device_time(uint8_t *buf, size_t size)
{
	uint32_t device_time = k_uptime_seconds() + ctx.time_offset;

	if (size < sizeof(uint32_t)) {
		return -ENOSPC;
	}

	buf[0] = (device_time >> 0) & 0xFF;
	buf[1] = (device_time >> 8) & 0xFF;
	buf[2] = (device_time >> 16) & 0xFF;
	buf[3] = (device_time >> 24) & 0xFF;

	return sizeof(uint32_t);
}

static inline k_timeout_t clock_sync_calc_periodicity(void)
{
	/* add +-30s jitter to nominal periodicity as required by the spec */
	return K_SECONDS(ctx.periodicity - 30 + sys_rand32_get() % 61);
}

static void clock_sync_package_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr,
					uint8_t len, const uint8_t *rx_buf)
{
	uint8_t tx_buf[3 * MAX_CLOCK_SYNC_ANS_LEN];
	uint8_t tx_pos = 0;
	uint8_t rx_pos = 0;

	__ASSERT(port == LORAWAN_PORT_CLOCK_SYNC, "Wrong port %d", port);

	while (rx_pos < len) {
		uint8_t command_id = rx_buf[rx_pos++];

		if (sizeof(tx_buf) - tx_pos < MAX_CLOCK_SYNC_ANS_LEN) {
			LOG_ERR("insufficient tx_buf size, some requests discarded");
			break;
		}

		switch (command_id) {
		case CLOCK_SYNC_CMD_PKG_VERSION:
			tx_buf[tx_pos++] = CLOCK_SYNC_CMD_PKG_VERSION;
			tx_buf[tx_pos++] = LORAWAN_PACKAGE_ID_CLOCK_SYNC;
			tx_buf[tx_pos++] = CLOCK_SYNC_PACKAGE_VERSION;
			LOG_DBG("PackageVersionReq");
			break;
		case CLOCK_SYNC_CMD_APP_TIME: {
			/* answer from application server */
			int32_t time_correction;

			ctx.nb_transmissions = 0;

			time_correction = rx_buf[rx_pos++];
			time_correction	+= rx_buf[rx_pos++] << 8;
			time_correction	+= rx_buf[rx_pos++] << 16;
			time_correction	+= rx_buf[rx_pos++] << 24;

			uint8_t token = rx_buf[rx_pos++] & 0x0F;

			if (token == ctx.req_token) {
				ctx.time_offset += time_correction;
				ctx.req_token = (ctx.req_token + 1) % 16;
				ctx.synchronized = true;

				LOG_DBG("AppTimeAns time_correction %d (token %d)",
					time_correction, token);
			} else {
				LOG_WRN("AppTimeAns with outdated token %d", token);
			}
			break;
		}
		case CLOCK_SYNC_CMD_DEVICE_APP_TIME_PERIODICITY: {
			uint8_t period = rx_buf[rx_pos++] & 0x0F;

			ctx.periodicity = 1U << (period + 7);

			tx_buf[tx_pos++] = CLOCK_SYNC_CMD_DEVICE_APP_TIME_PERIODICITY;
			tx_buf[tx_pos++] = 0x00; /* Status: OK */

			tx_pos += clock_sync_serialize_device_time(tx_buf + tx_pos,
								   sizeof(tx_buf) - tx_pos);

			lorawan_services_reschedule_work(&ctx.resync_work,
							 clock_sync_calc_periodicity());

			LOG_DBG("DeviceAppTimePeriodicityReq period: %u", period);
			break;
		}
		case CLOCK_SYNC_CMD_FORCE_DEVICE_RESYNC: {
			uint8_t nb_transmissions = rx_buf[rx_pos++] & 0x07;

			if (nb_transmissions != 0) {
				ctx.nb_transmissions = nb_transmissions;
				lorawan_services_reschedule_work(&ctx.resync_work, K_NO_WAIT);
			}

			LOG_DBG("ForceDeviceResyncCmd nb_transmissions: %u", nb_transmissions);
			break;
		}
		default:
			return;
		}
	}

	if (tx_pos > 0) {
		lorawan_services_schedule_uplink(LORAWAN_PORT_CLOCK_SYNC, tx_buf, tx_pos, 0);
	}
}

static int clock_sync_app_time_req(void)
{
	uint8_t tx_pos = 0;
	uint8_t tx_buf[6];

	if (lorawan_services_class_c_active() > 0) {
		/* avoid disturbing the session and causing potential package loss */
		LOG_DBG("AppTimeReq not sent because of active class C session");
		return -EBUSY;
	}

	tx_buf[tx_pos++] = CLOCK_SYNC_CMD_APP_TIME;
	tx_pos += clock_sync_serialize_device_time(tx_buf + tx_pos,
						       sizeof(tx_buf) - tx_pos);

	/* Param: AnsRequired = 0 | TokenReq */
	tx_buf[tx_pos++] = ctx.req_token;

	LOG_DBG("Sending clock sync AppTimeReq (token %d)", ctx.req_token);

	lorawan_services_schedule_uplink(LORAWAN_PORT_CLOCK_SYNC, tx_buf, tx_pos, 0);

	return 0;
}

static void clock_sync_resync_handler(struct k_work *work)
{
	clock_sync_app_time_req();

	if (ctx.nb_transmissions > 0) {
		ctx.nb_transmissions--;
		lorawan_services_reschedule_work(&ctx.resync_work, K_SECONDS(CLOCK_RESYNC_DELAY));
	} else {
		lorawan_services_reschedule_work(&ctx.resync_work,
						 clock_sync_calc_periodicity());
	}
}

int lorawan_clock_sync_get(uint32_t *gps_time)
{
	__ASSERT(gps_time != NULL, "gps_time parameter is required");

	if (ctx.synchronized) {
		*gps_time = k_uptime_seconds() + ctx.time_offset;
		return 0;
	} else {
		return -EAGAIN;
	}
}

static struct lorawan_downlink_cb downlink_cb = {
	.port = (uint8_t)LORAWAN_PORT_CLOCK_SYNC,
	.cb = clock_sync_package_callback
};

int lorawan_clock_sync_run(void)
{
	ctx.periodicity = CONFIG_LORAWAN_APP_CLOCK_SYNC_PERIODICITY;

	lorawan_register_downlink_callback(&downlink_cb);

	k_work_init_delayable(&ctx.resync_work, clock_sync_resync_handler);
	lorawan_services_reschedule_work(&ctx.resync_work, K_NO_WAIT);

	return 0;
}
