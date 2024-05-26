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

#define FRAG_SIZE       CONFIG_LORAWAN_FRAG_TRANSPORT_MAX_FRAG_SIZE
#define FIRMWARE_SIZE   (FRAG_SIZE * 100 + 1) /* not divisible by frag size to test padding */
#define UNCODED_FRAGS   (DIV_ROUND_UP(FIRMWARE_SIZE, FRAG_SIZE))
#define REDUNDANT_FRAGS \
	(DIV_ROUND_UP(UNCODED_FRAGS * CONFIG_LORAWAN_FRAG_TRANSPORT_MAX_REDUNDANCY, 100))
#define PADDING         (UNCODED_FRAGS * FRAG_SIZE - FIRMWARE_SIZE)

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

ZTEST(frag_decoder, test_frag_transport)
{
	uint8_t buf[256]; /* maximum size of one LoRaWAN message */
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
	int ret;

	k_sem_reset(&fuota_finished_sem);

	lorawan_emul_send_downlink(FRAG_TRANSPORT_PORT, false, 0, 0, sizeof(frag_session_setup_req),
				   frag_session_setup_req);

	for (int i = 0; i < sizeof(fw_coded) / FRAG_SIZE; i++) {
		if (i % 10 == 9) {
			/* loose every 10th packet */
			continue;
		}
		buf[0] = CMD_DATA_FRAGMENT;
		buf[1] = (i + 1) & 0xFF;
		buf[2] = (FRAG_SESSION_INDEX << 6) | ((i + 1) >> 8);
		memcpy(buf + 3, fw_coded + i * FRAG_SIZE, FRAG_SIZE);
		lorawan_emul_send_downlink(FRAG_TRANSPORT_PORT, false, 0, 0, FRAG_SIZE + 3, buf);
	}

	for (int i = 0; i < UNCODED_FRAGS; i++) {
		size_t num_bytes = (i == UNCODED_FRAGS - 1) ? (FRAG_SIZE - PADDING) : FRAG_SIZE;

		flash_area_read(fa, i * FRAG_SIZE, buf, num_bytes);
		zassert_mem_equal(buf, fw_coded + i * FRAG_SIZE, num_bytes, "fragment %d invalid",
				  i + 1);
	}

	ret = k_sem_take(&fuota_finished_sem, K_MSEC(100));
	zassert_equal(ret, 0, "FUOTA finish timed out");
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
