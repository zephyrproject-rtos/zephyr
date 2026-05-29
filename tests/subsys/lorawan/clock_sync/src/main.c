/*
 * Copyright (c) 2024 A Labs GmbH
 * Copyright (c) 2024 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/emul.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define CMD_PACKAGE_VERSION             (0x00)
#define CMD_APP_TIME                    (0x01)
#define CMD_DEVICE_APP_TIME_PERIODICITY (0x02)
#define CMD_FORCE_DEVICE_RESYNC         (0x03)

#define CLOCK_SYNC_PORT (202)
#define CLOCK_SYNC_ID

struct lorawan_msg {
	/* large enough buffer to fit maximum clock sync message length */
	uint8_t data[6];
	uint8_t len;
};

K_MSGQ_DEFINE(uplink_msgq, sizeof(struct lorawan_msg), 10, 4);

void uplink_handler(uint8_t port, uint8_t len, const uint8_t *data)
{
	struct lorawan_msg msg;
	int ret;

	zassert_equal(port, CLOCK_SYNC_PORT);

	zassert_true(len <= sizeof(msg.data));
	memcpy(msg.data, data, len);
	msg.len = len;

	ret = k_msgq_put(&uplink_msgq, &msg, K_NO_WAIT);
	zassert_equal(ret, 0);
}

ZTEST(clock_sync, test_package_version)
{
	struct lorawan_msg ans;
	uint8_t req_data[] = {CMD_PACKAGE_VERSION};
	int ret;

	k_msgq_purge(&uplink_msgq);

	lorawan_emul_send_downlink(CLOCK_SYNC_PORT, false, 0, 0, sizeof(req_data), req_data);

	ret = k_msgq_get(&uplink_msgq, &ans, K_MSEC(100));
	zassert_equal(ret, 0, "receiving PackageVersionAns timed out");
	zassert_equal(ans.len, 3);
	zassert_equal(ans.data[0], CMD_PACKAGE_VERSION);
	zassert_equal(ans.data[1], 1); /* PackageIdentifier */
	zassert_equal(ans.data[2], 2); /* PackageVersion */
}

ZTEST(clock_sync, test_app_time)
{
	uint8_t ans_data[6] = {CMD_APP_TIME};
	struct lorawan_msg req;
	uint32_t device_time;
	uint32_t gps_time;
	uint8_t token_req;
	int ret;

	k_msgq_purge(&uplink_msgq);

	/* wait for more than the default (=minimum) periodicity of 128s + 30s jitter */
	ret = k_msgq_get(&uplink_msgq, &req, K_SECONDS(128 + 30 + 1));
	zassert_equal(ret, 0, "receiving AppTimeReq timed out");
	zassert_equal(req.len, 6);
	zassert_equal(req.data[0], CMD_APP_TIME);

	device_time = sys_get_le32(req.data + 1);
	token_req = req.data[5] & 0xF;
	zassert_within((int)device_time, k_uptime_seconds(), 1);

	/* apply a time correction of 1000 seconds */
	sys_put_le32(1000, ans_data + 1);
	ans_data[5] = token_req;

	lorawan_emul_send_downlink(CLOCK_SYNC_PORT, false, 0, 0, sizeof(ans_data), ans_data);

	lorawan_clock_sync_get(&gps_time);
	zassert_within(gps_time, k_uptime_seconds() + 1000, 1);
}

ZTEST(clock_sync, test_device_app_time_periodicity)
{
	const uint8_t period = 1; /* actual periodicity in seconds: 128 * 2^period */
	uint8_t req_data[] = {
		CMD_DEVICE_APP_TIME_PERIODICITY,
		period & 0xF,
	};
	struct lorawan_msg app_time_req;
	struct lorawan_msg ans;
	uint32_t device_time;
	uint32_t gps_time;
	int ret;

	k_msgq_purge(&uplink_msgq);

	lorawan_emul_send_downlink(CLOCK_SYNC_PORT, false, 0, 0, sizeof(req_data), req_data);

	ret = k_msgq_get(&uplink_msgq, &ans, K_MSEC(100));
	zassert_equal(ret, 0, "receiving DeviceAppTimePeriodicityAns timed out");
	zassert_equal(ans.len, 6);
	zassert_equal(ans.data[0], CMD_DEVICE_APP_TIME_PERIODICITY);
	zassert_equal(ans.data[1], 0);

	device_time = sys_get_le32(ans.data + 2);
	lorawan_clock_sync_get(&gps_time);
	zassert_within(device_time, gps_time, 1);

	/* wait for more than the old periodicity of 128s + 30s jitter */
	ret = k_msgq_get(&uplink_msgq, &app_time_req, K_SECONDS(128 + 30 + 1));
	zassert_equal(ret, -EAGAIN, "received AppTimeReq too early");

	/* wait for another 128s to cover the new periodicity of 256s + 30s jitter */
	ret = k_msgq_get(&uplink_msgq, &app_time_req, K_SECONDS(128));
	zassert_equal(ret, 0, "receiving AppTimeReq timed out");
	zassert_equal(app_time_req.len, 6);
	zassert_equal(app_time_req.data[0], CMD_APP_TIME);

	/* reset to minimum periodicity */
	req_data[1] = 0;
	lorawan_emul_send_downlink(CLOCK_SYNC_PORT, false, 0, 0, sizeof(req_data), req_data);
	ret = k_msgq_get(&uplink_msgq, &ans, K_MSEC(100));
	zassert_equal(ret, 0, "receiving DeviceAppTimePeriodicityAns timed out");
	zassert_equal(ans.len, 6);
	zassert_equal(ans.data[0], CMD_DEVICE_APP_TIME_PERIODICITY);
}

ZTEST(clock_sync, test_force_device_resync)
{
	const uint8_t nb_transmissions = 2;
	uint8_t resync_req_data[] = {
		CMD_FORCE_DEVICE_RESYNC,
		nb_transmissions,
	};
	struct lorawan_msg app_time_req;
	int ret;

	k_msgq_purge(&uplink_msgq);

	lorawan_emul_send_downlink(CLOCK_SYNC_PORT, false, 0, 0, sizeof(resync_req_data),
				   resync_req_data);

	for (int i = 0; i < nb_transmissions; i++) {
		/* wait for more than CLOCK_RESYNC_DELAY of 10 secs */
		ret = k_msgq_get(&uplink_msgq, &app_time_req, K_SECONDS(11));
		zassert_equal(ret, 0, "receiving AppTimeReq #%d timed out", i + 1);
		zassert_equal(app_time_req.len, 6);
		zassert_equal(app_time_req.data[0], CMD_APP_TIME);
	}
}

static void *clock_sync_setup(void)
{
	const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	struct lorawan_join_config join_cfg = {0};
	int ret;

	zassert_true(device_is_ready(lora_dev), "LoRa device not ready");

	ret = lorawan_start();
	zassert_equal(ret, 0, "lorawan_start failed: %d", ret);

	ret = lorawan_join(&join_cfg);
	zassert_equal(ret, 0, "lorawan_join failed: %d", ret);

	lorawan_emul_register_uplink_callback(uplink_handler);

	ret = lorawan_clock_sync_run();
	zassert_equal(ret, 0, "clock_sync_run failed: %d", ret);

	/* wait for first messages to be processed in the background */
	k_sleep(K_SECONDS(1));

	return NULL;
}

ZTEST_SUITE(clock_sync, NULL, clock_sync_setup, NULL, NULL, NULL);
