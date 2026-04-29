/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal I2C role-switch echo sample for MCXN947 Flexcomm9.
 *
 * Two boards are used:
 * - requester: periodically sends payloads to responder and checks echoed data
 * - responder: receives payloads in target mode and echoes them back using
 *   controller-mode transmission on the same I2C peripheral
 *
 * This sample validates transient controller-mode transfer support in the
 * underlying I2C driver while the device remains logically target-attached.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(role_switch_echo, LOG_LEVEL_INF);

#if DT_NODE_HAS_STATUS(DT_ALIAS(role_switch_i2c), okay)
#define I2C_DEV_NODE DT_ALIAS(role_switch_i2c)
#else
#error "role_switch_i2c alias not defined"
#endif

#if !DT_NODE_HAS_STATUS(I2C_DEV_NODE, okay)
#error "flexcomm9_lpi2c device is not enabled in the overlay"
#endif

#define LOCAL_ADDR  CONFIG_ROLE_SWITCH_ECHO_LOCAL_ADDR
#define REMOTE_ADDR CONFIG_ROLE_SWITCH_ECHO_REMOTE_ADDR
#define PAYLOAD_LEN CONFIG_ROLE_SWITCH_ECHO_PAYLOAD_SIZE
#define LOOP_COUNT  CONFIG_ROLE_SWITCH_ECHO_COUNT
#define INTERVAL_MS CONFIG_ROLE_SWITCH_ECHO_INTERVAL_MS

#define ECHO_TIMEOUT_MS 2000

BUILD_ASSERT(PAYLOAD_LEN <= 32, "Payload size must be <= 32");

enum payload_pattern {
	PAYLOAD_PATTERN_INCREMENT = 0,
	PAYLOAD_PATTERN_XOR,
};

struct echo_ctx {
	struct i2c_target_config target_cfg;
	struct k_sem rx_sem;
	uint8_t rx_buf[PAYLOAD_LEN];
	size_t rx_len;
	bool rx_ready;
};

static const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
static struct echo_ctx g_ctx;

static void fill_payload(uint8_t *buf, size_t len, uint32_t seq,
			 enum payload_pattern pattern)
{
	switch (pattern) {
	case PAYLOAD_PATTERN_INCREMENT:
		for (size_t i = 0; i < len; i++) {
			buf[i] = (uint8_t)(seq + i);
		}
		break;

	case PAYLOAD_PATTERN_XOR:
		for (size_t i = 0; i < len; i++) {
			buf[i] = (uint8_t)(0xA5U ^ (uint8_t)seq ^ (uint8_t)i);
		}
		break;

	default:
		for (size_t i = 0; i < len; i++) {
			buf[i] = 0U;
		}
		break;
	}
}

static void dump_buf(const char *tag, const uint8_t *buf, size_t len)
{
	printk("%s", tag);
	for (size_t i = 0; i < len; i++) {
		printk("%02x ", buf[i]);
	}
	printk("\n");
}

static int echo_target_write_requested(struct i2c_target_config *cfg)
{
	struct echo_ctx *ctx = CONTAINER_OF(cfg, struct echo_ctx, target_cfg);

	ctx->rx_len = 0U;
	ctx->rx_ready = false;

	return 0;
}

static int echo_target_write_received(struct i2c_target_config *cfg, uint8_t val)
{
	struct echo_ctx *ctx = CONTAINER_OF(cfg, struct echo_ctx, target_cfg);

	if (ctx->rx_len >= sizeof(ctx->rx_buf)) {
		return -ENOSPC;
	}

	ctx->rx_buf[ctx->rx_len++] = val;
	return 0;
}

static int echo_target_stop(struct i2c_target_config *cfg)
{
	struct echo_ctx *ctx = CONTAINER_OF(cfg, struct echo_ctx, target_cfg);

	if (ctx->rx_len == PAYLOAD_LEN) {
		ctx->rx_ready = true;
		k_sem_give(&ctx->rx_sem);
	}

	return 0;
}

static const struct i2c_target_callbacks echo_target_callbacks = {
	.write_requested = echo_target_write_requested,
	.write_received = echo_target_write_received,
	.stop = echo_target_stop,
};

static int register_target(uint16_t addr)
{
	g_ctx.target_cfg.address = addr;
	g_ctx.target_cfg.callbacks = &echo_target_callbacks;
	return i2c_target_register(i2c_dev, &g_ctx.target_cfg);
}

static int responder_main(void)
{
	int rc;
	uint32_t echo_count = 0U;

	k_sem_init(&g_ctx.rx_sem, 0, 1);

	rc = register_target(LOCAL_ADDR);
	if (rc) {
		LOG_ERR("failed to register target at 0x%02x: %d", LOCAL_ADDR, rc);
		return rc;
	}

	LOG_INF("responder started: local=0x%02x remote=0x%02x payload=%d",
		LOCAL_ADDR, REMOTE_ADDR, PAYLOAD_LEN);

	while (1) {
		rc = k_sem_take(&g_ctx.rx_sem, K_FOREVER);
		if (rc) {
			LOG_ERR("semaphore wait failed: %d", rc);
			continue;
		}

		if (!g_ctx.rx_ready) {
			continue;
		}

		rc = i2c_write(i2c_dev, g_ctx.rx_buf, g_ctx.rx_len, REMOTE_ADDR);
		if (rc) {
			LOG_ERR("controller TX failed rc=%d", rc);
			dump_buf("responder RX: ", g_ctx.rx_buf, g_ctx.rx_len);
		} else {
			echo_count++;
			LOG_INF("echoed packet %u len=%u", echo_count,
				(unsigned int)g_ctx.rx_len);
		}

		g_ctx.rx_ready = false;
		g_ctx.rx_len = 0U;
	}
}

static int requester_wait_for_echo(const uint8_t *expected, size_t len)
{
	int rc;

	rc = k_sem_take(&g_ctx.rx_sem, K_MSEC(ECHO_TIMEOUT_MS));
	if (rc) {
		return rc;
	}

	if (!g_ctx.rx_ready) {
		return -EIO;
	}

	if (g_ctx.rx_len != len) {
		return -EMSGSIZE;
	}

	if (memcmp(g_ctx.rx_buf, expected, len) != 0) {
		return -EIO;
	}

	g_ctx.rx_ready = false;
	g_ctx.rx_len = 0U;
	return 0;
}

static int requester_run_pattern(enum payload_pattern pattern, const char *name,
				 uint32_t *pass_out, uint32_t *fail_out)
{
	int rc;
	uint8_t tx_buf[PAYLOAD_LEN];
	uint32_t pass = 0U;
	uint32_t fail = 0U;

	LOG_INF("starting pattern: %s", name);

	for (uint32_t seq = 0; seq < LOOP_COUNT; seq++) {
		fill_payload(tx_buf, sizeof(tx_buf), seq, pattern);

		rc = i2c_write(i2c_dev, tx_buf, sizeof(tx_buf), REMOTE_ADDR);
		if (rc) {
			fail++;
			LOG_ERR("TX failed pattern=%s seq=%u rc=%d", name, seq, rc);
			k_msleep(INTERVAL_MS);
			continue;
		}

		rc = requester_wait_for_echo(tx_buf, sizeof(tx_buf));
		if (rc) {
			fail++;
			LOG_ERR("echo mismatch/timeout pattern=%s seq=%u rc=%d",
				name, seq, rc);
			if (g_ctx.rx_len > 0U) {
				dump_buf("expected: ", tx_buf, sizeof(tx_buf));
				dump_buf("received: ", g_ctx.rx_buf, g_ctx.rx_len);
			}
		} else {
			pass++;
			LOG_INF("PASS pattern=%s seq=%u", name, seq);
		}

		k_msleep(INTERVAL_MS);
	}

	*pass_out += pass;
	*fail_out += fail;

	LOG_INF("pattern done: %s pass=%u fail=%u", name, pass, fail);
	return 0;
}

static int requester_main(void)
{
	int rc;
	uint32_t total_pass = 0U;
	uint32_t total_fail = 0U;

	k_sem_init(&g_ctx.rx_sem, 0, 1);

	rc = register_target(LOCAL_ADDR);
	if (rc) {
		LOG_ERR("failed to register local target at 0x%02x: %d",
			LOCAL_ADDR, rc);
		return rc;
	}

	LOG_INF("requester started: local=0x%02x remote=0x%02x payload=%d count=%d",
		LOCAL_ADDR, REMOTE_ADDR, PAYLOAD_LEN, LOOP_COUNT);

	requester_run_pattern(PAYLOAD_PATTERN_INCREMENT, "increment",
			      &total_pass, &total_fail);
	requester_run_pattern(PAYLOAD_PATTERN_XOR, "xor",
			      &total_pass, &total_fail);

	LOG_INF("summary: total_pass=%u total_fail=%u", total_pass, total_fail);

	return (total_fail == 0U) ? 0 : -EIO;
}

int main(void)
{
	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("I2C device %s is not ready", i2c_dev->name);
		return -ENODEV;
	}

#if defined(CONFIG_ROLE_SWITCH_ECHO_ROLE_REQUESTER)
	return requester_main();
#elif defined(CONFIG_ROLE_SWITCH_ECHO_ROLE_RESPONDER)
	return responder_main();
#else
#error "Select requester or responder role"
#endif
}
