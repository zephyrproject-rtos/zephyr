/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/drivers/firmware/scmi/shmem.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/ztest.h>

#define SHMEM_NODE DT_NODELABEL(scmi_shmem0)
#define SHMEM_SIZE DT_REG_SIZE(SHMEM_NODE)

#define TEST_HDR SCMI_MESSAGE_HDR_MAKE(0x3, SCMI_COMMAND, 0x14, 0x0)

#define SENTINEL 0xa5a5a5a5U

/* The shmem area: the layout, then the payload of the reply */
struct shmem_area {
	struct scmi_shmem_layout layout;
	uint32_t payload[3];
};

static const struct device *const shmem = DEVICE_DT_GET(SHMEM_NODE);
static volatile struct shmem_area *area;

/* Write a reply the way the platform does */
static void platform_reply(uint32_t hdr, uint32_t len, const uint32_t *payload, size_t words)
{
	area->layout.msg_hdr = hdr;
	area->layout.len = len;
	for (size_t i = 0; i < words; i++) {
		area->payload[i] = payload[i];
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset((void *)area, 0, SHMEM_SIZE);
}

static void *setup(void)
{
	mm_reg_t regmap;

	zassert_true(device_is_ready(shmem));
	device_map(&regmap, DT_REG_ADDR(SHMEM_NODE), SHMEM_SIZE, K_MEM_CACHE_NONE);
	area = (volatile struct shmem_area *)regmap;

	return NULL;
}

ZTEST(scmi_shmem, test_reply_exact_length)
{
	const uint32_t payload[] = {0, 0x1234};
	uint32_t buf[2] = {SENTINEL, SENTINEL};
	struct scmi_message msg = {.hdr = TEST_HDR, .len = sizeof(buf), .content = buf};

	platform_reply(TEST_HDR, sizeof(uint32_t) + sizeof(payload), payload, ARRAY_SIZE(payload));

	zassert_ok(scmi_shmem_read_message(shmem, &msg));
	zassert_equal(msg.len, sizeof(buf));
	zassert_equal(buf[0], 0);
	zassert_equal(buf[1], 0x1234);
}

ZTEST(scmi_shmem, test_reply_status_only)
{
	const uint32_t payload[] = {(uint32_t)SCMI_NOT_FOUND};
	uint32_t buf[2] = {SENTINEL, SENTINEL};
	struct scmi_message msg = {.hdr = TEST_HDR, .len = sizeof(buf), .content = buf};

	platform_reply(TEST_HDR, sizeof(uint32_t) + sizeof(payload), payload, ARRAY_SIZE(payload));

	zassert_ok(scmi_shmem_read_message(shmem, &msg));
	zassert_equal(msg.len, sizeof(uint32_t));
	zassert_equal((int32_t)buf[0], SCMI_NOT_FOUND);
	zassert_equal(buf[1], SENTINEL, "bytes past the reply must not be written");
}

ZTEST(scmi_shmem, test_reply_longer_than_buffer)
{
	const uint32_t payload[] = {0, 0x1234, 0x5678};
	uint32_t buf[3] = {SENTINEL, SENTINEL, SENTINEL};
	struct scmi_message msg = {.hdr = TEST_HDR, .len = 2 * sizeof(uint32_t), .content = buf};

	platform_reply(TEST_HDR, sizeof(uint32_t) + sizeof(payload), payload, ARRAY_SIZE(payload));

	zassert_ok(scmi_shmem_read_message(shmem, &msg));
	zassert_equal(msg.len, 2 * sizeof(uint32_t));
	zassert_equal(buf[1], 0x1234);
	zassert_equal(buf[2], SENTINEL, "reply must be truncated to the buffer");
}

ZTEST(scmi_shmem, test_reply_without_status)
{
	uint32_t buf[2];
	struct scmi_message msg = {.hdr = TEST_HDR, .len = sizeof(buf), .content = buf};

	platform_reply(TEST_HDR, sizeof(uint32_t), NULL, 0);
	zassert_equal(scmi_shmem_read_message(shmem, &msg), -EPROTO);

	platform_reply(TEST_HDR, 0, NULL, 0);
	zassert_equal(scmi_shmem_read_message(shmem, &msg), -EPROTO);
}

ZTEST(scmi_shmem, test_reply_exceeds_shmem)
{
	uint32_t buf[2];
	struct scmi_message msg = {.hdr = TEST_HDR, .len = sizeof(buf), .content = buf};

	platform_reply(TEST_HDR, SHMEM_SIZE, NULL, 0);
	zassert_equal(scmi_shmem_read_message(shmem, &msg), -EPROTO);
}

ZTEST(scmi_shmem, test_reply_header_mismatch)
{
	const uint32_t payload[] = {0};
	uint32_t buf[1];
	struct scmi_message msg = {.hdr = TEST_HDR, .len = sizeof(buf), .content = buf};

	platform_reply(TEST_HDR + 1, sizeof(uint32_t) + sizeof(payload), payload,
		       ARRAY_SIZE(payload));
	zassert_equal(scmi_shmem_read_message(shmem, &msg), -EINVAL);
}

ZTEST_SUITE(scmi_shmem, NULL, setup, before, NULL, NULL);
