/*
 * Copyright (c) 2024 A Labs GmbH
 * Copyright (c) 2024 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/lorawan/emul.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/ztest.h>

#include "frag_encoder.h"

#define FRAG_SIZE     CONFIG_LORAWAN_FRAG_TRANSPORT_MAX_FRAG_SIZE
#define FIRMWARE_SIZE (FRAG_SIZE * 100 + 1) /* not divisible by frag size to test padding */
#define UNCODED_FRAGS (DIV_ROUND_UP(FIRMWARE_SIZE, FRAG_SIZE))
#define REDUNDANT_FRAGS                                                                            \
	(DIV_ROUND_UP(UNCODED_FRAGS * CONFIG_LORAWAN_FRAG_TRANSPORT_MAX_REDUNDANCY, 100))
#define PADDING (UNCODED_FRAGS * FRAG_SIZE - FIRMWARE_SIZE)

#define CMD_FRAG_SESSION_SETUP (0x02)
#define CMD_DATA_FRAGMENT      (0x08)
#define FRAG_TRANSPORT_PORT    (201)
#define FRAG_SESSION_INDEX     (1)

#define TARGET_IMAGE_AREA FIXED_PARTITION_ID(slot1_partition)

/* below array would normally hold the actual firmware binary */
static uint8_t fw_uncoded[FIRMWARE_SIZE];

/* enough space for redundancy of up to 100% */
static uint8_t fw_coded[(UNCODED_FRAGS + REDUNDANT_FRAGS) * FRAG_SIZE];

static const struct flash_area *fa;

static struct k_sem fuota_finished_sem;

static void fuota_finished(void)
{
	k_sem_give(&fuota_finished_sem);
}

uint8_t frag_session_setup_req[] = {
	CMD_FRAG_SESSION_SETUP,
	0x1f,
	UNCODED_FRAGS & 0xFF,
	(UNCODED_FRAGS >> 8) & 0xFF,
	FRAG_SIZE,
	0x01,
	PADDING,
	0x00,
	0x00,
	0x00,
	0x00,
};

static void run_test(size_t lost_packets, bool expected_success)
{
	uint8_t buf[256]; /* maximum size of one LoRaWAN message */
	int ret;
	size_t num_packets = sizeof(fw_coded) / FRAG_SIZE;
	size_t packets_to_lose[num_packets];
	size_t tmp;
	bool skip;

	for (size_t i = 0; i < num_packets; i++) {
		packets_to_lose[i] = i;
	}
	/* Shuffle array */
	for (size_t i = num_packets - 1; i > 0; i--) {
		int j = sys_rand32_get() % (i + 1);

		if (i != j) {
			tmp = packets_to_lose[j];
			packets_to_lose[j] = packets_to_lose[i];
			packets_to_lose[i] = tmp;
		}
	}

	k_sem_reset(&fuota_finished_sem);
	lorawan_emul_send_downlink(FRAG_TRANSPORT_PORT, false, 0, 0, sizeof(frag_session_setup_req),
				   frag_session_setup_req);

	for (size_t i = 0; i < num_packets; i++) {
		skip = false;
		for (int j = 0; j < lost_packets; j++) {
			if (packets_to_lose[j] == i) {
				skip = true;
				break;
			}
		}
		if (skip) {
			/* lose packet */
			continue;
		}
		buf[0] = CMD_DATA_FRAGMENT;
		buf[1] = (i + 1) & 0xFF;
		buf[2] = (FRAG_SESSION_INDEX << 6) | ((i + 1) >> 8);
		memcpy(buf + 3, fw_coded + i * FRAG_SIZE, FRAG_SIZE);
		lorawan_emul_send_downlink(FRAG_TRANSPORT_PORT, false, 0, 0, FRAG_SIZE + 3, buf);
	}

	ret = k_sem_take(&fuota_finished_sem, K_MSEC(100));
	if (expected_success) {
		zassert_equal(ret, 0, "FUOTA finish timed out");
		for (int i = 0; i < UNCODED_FRAGS; i++) {
			size_t num_bytes =
				(i == UNCODED_FRAGS - 1) ? (FRAG_SIZE - PADDING) : FRAG_SIZE;

			flash_area_read(fa, i * FRAG_SIZE, buf, num_bytes);
			zassert_mem_equal(buf, fw_coded + i * FRAG_SIZE, num_bytes,
					  "fragment %d invalid", i + 1);
		}
	} else {
		zassert_not_equal(ret, 0, "FUOTA should have failed");
	}
}

ZTEST(frag_decoder, test_frag_transport_lose_none)
{
	run_test(0, true);
}

ZTEST(frag_decoder, test_frag_transport_lose_one)
{
	run_test(1, true);
}

ZTEST(frag_decoder, test_frag_transport_lose_close_to_max_redundancy)
{
	run_test(REDUNDANT_FRAGS * 0.95, true);
}

ZTEST(frag_decoder, test_frag_transport_lose_more_than_max_redundancy)
{
	run_test(REDUNDANT_FRAGS + 1, false);
}

static void *frag_decoder_setup(void)
{
	const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	struct lorawan_join_config join_cfg = {0};
	int ret;

	/* populate firmware image with random data */
	sys_rand_get(fw_uncoded, sizeof(fw_uncoded));

	/* create coded data (including redundant fragments) from firmware image */
	ret = lorawan_frag_encoder(fw_uncoded, sizeof(fw_uncoded), fw_coded, sizeof(fw_coded),
				   FRAG_SIZE, REDUNDANT_FRAGS);
	zassert_equal(ret, 0, "creating coded data failed: %d", ret);

	k_sem_init(&fuota_finished_sem, 0, 1);

	ret = flash_area_open(TARGET_IMAGE_AREA, &fa);
	zassert_equal(ret, 0, "opening flash area failed: %d", ret);

	zassert_true(device_is_ready(lora_dev), "LoRa device not ready");

	ret = lorawan_start();
	zassert_equal(ret, 0, "lorawan_start failed: %d", ret);

	ret = lorawan_join(&join_cfg);
	zassert_equal(ret, 0, "lorawan_join failed: %d", ret);

	lorawan_frag_transport_run(fuota_finished);

	return NULL;
}

ZTEST_SUITE(frag_decoder, NULL, frag_decoder_setup, NULL, NULL, NULL);
