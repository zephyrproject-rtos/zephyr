/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libmctp.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pmci/mctp/mctp_i2c_smbus_target.h>

#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(mctp_i2c_smbus_echo, LOG_LEVEL_INF);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mctp_i2c), okay)
#define MCTP_BINDING_NODE DT_NODELABEL(mctp_i2c)
#else
#error "mctp_i2c node is not defined"
#endif

#define REMOTE_EID  CONFIG_MCTP_I2C_SMBUS_ECHO_REMOTE_EID
#define PAYLOAD_LEN CONFIG_MCTP_I2C_SMBUS_ECHO_PAYLOAD_SIZE
#define LOOP_COUNT  CONFIG_MCTP_I2C_SMBUS_ECHO_COUNT
#define INTERVAL_MS CONFIG_MCTP_I2C_SMBUS_ECHO_INTERVAL_MS

#define ECHO_TIMEOUT_MS 2000

BUILD_ASSERT(PAYLOAD_LEN <= MCTP_BODY_SIZE(MCTP_I2C_SMBUS_MAX_MCTP_BYTES),
	     "Payload size exceeds the MCTP SMBus block payload capacity");

MCTP_I2C_SMBUS_TARGET_DT_DEFINE(mctp_smbus, MCTP_BINDING_NODE);

enum payload_pattern {
	PAYLOAD_PATTERN_INCREMENT = 0,
	PAYLOAD_PATTERN_XOR,
};

struct mctp_i2c_smbus_echo_ctx {
	struct k_sem rx_sem;
	uint8_t from_eid;
	uint8_t msg_tag;
	bool tag_owner;
	uint8_t rx_buf[PAYLOAD_LEN];
	size_t rx_len;
	bool rx_ready;
};

static struct mctp *mctp_ctx;
static struct mctp_i2c_smbus_echo_ctx echo_ctx;

#if defined(CONFIG_MCTP_I2C_SMBUS_ECHO_ROLE_REQUESTER)
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
			buf[i] = (uint8_t)(0xa5U ^ (uint8_t)seq ^ (uint8_t)i);
		}
		break;

	default:
		memset(buf, 0, len);
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
#endif

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag,
		       void *data, void *msg, size_t len)
{
	ARG_UNUSED(data);

	if (len > sizeof(echo_ctx.rx_buf)) {
		LOG_WRN("MCTP RX too large: len=%u", (unsigned int)len);
		return;
	}

	memcpy(echo_ctx.rx_buf, msg, len);
	echo_ctx.rx_len = len;
	echo_ctx.from_eid = eid;
	echo_ctx.msg_tag = msg_tag;
	echo_ctx.tag_owner = tag_owner;
	echo_ctx.rx_ready = true;

	LOG_INF("MCTP RX from EID %u len=%u", eid, (unsigned int)len);
	k_sem_give(&echo_ctx.rx_sem);
}

static int mctp_echo_start(void)
{
	int rc;

	k_sem_init(&echo_ctx.rx_sem, 0, 1);

	mctp_ctx = mctp_init();
	if (mctp_ctx == NULL) {
		LOG_ERR("mctp_init failed");
		return -ENOMEM;
	}

	rc = mctp_register_bus(mctp_ctx, &mctp_smbus.binding,
			       mctp_smbus.endpoint_id);
	if (rc) {
		LOG_ERR("mctp_register_bus failed: %d", rc);
		return rc;
	}

	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	LOG_INF("MCTP SMBus binding started: local-eid=%u local-i2c=0x%02x remote-i2c=0x%02x",
		mctp_smbus.endpoint_id, mctp_smbus.ep_i2c_addr,
		mctp_smbus.remote_i2c_addr);

	return 0;
}

#if defined(CONFIG_MCTP_I2C_SMBUS_ECHO_ROLE_REQUESTER)
static int wait_for_echo(const uint8_t *expected, size_t len)
{
	int rc;

	rc = k_sem_take(&echo_ctx.rx_sem, K_MSEC(ECHO_TIMEOUT_MS));
	if (rc) {
		return rc;
	}

	if (!echo_ctx.rx_ready || (echo_ctx.rx_len != len)) {
		return -EIO;
	}

	if (memcmp(echo_ctx.rx_buf, expected, len) != 0) {
		return -EIO;
	}

	echo_ctx.rx_ready = false;
	echo_ctx.rx_len = 0U;
	return 0;
}

static int run_pattern(enum payload_pattern pattern, const char *name,
		       uint32_t *pass_out, uint32_t *fail_out)
{
	uint8_t tx_buf[PAYLOAD_LEN];
	uint32_t pass = 0U;
	uint32_t fail = 0U;
	int rc;

	LOG_INF("starting MCTP pattern: %s", name);

	for (uint32_t seq = 0; seq < LOOP_COUNT; seq++) {
		fill_payload(tx_buf, sizeof(tx_buf), seq, pattern);

		rc = mctp_message_tx(mctp_ctx, REMOTE_EID, false, 0,
				     tx_buf, sizeof(tx_buf));
		if (rc) {
			fail++;
			LOG_ERR("MCTP TX failed pattern=%s seq=%u rc=%d",
				name, seq, rc);
			k_msleep(INTERVAL_MS);
			continue;
		}

		rc = wait_for_echo(tx_buf, sizeof(tx_buf));
		if (rc) {
			fail++;
			LOG_ERR("MCTP echo mismatch/timeout pattern=%s seq=%u rc=%d",
				name, seq, rc);
			if (echo_ctx.rx_len > 0U) {
				dump_buf("expected: ", tx_buf, sizeof(tx_buf));
				dump_buf("received: ", echo_ctx.rx_buf,
					 echo_ctx.rx_len);
			}
		} else {
			pass++;
			LOG_INF("MCTP PASS pattern=%s seq=%u", name, seq);
		}

		k_msleep(INTERVAL_MS);
	}

	*pass_out += pass;
	*fail_out += fail;

	LOG_INF("MCTP pattern done: %s pass=%u fail=%u", name, pass, fail);
	return 0;
}

static int requester_main(void)
{
	uint32_t total_pass = 0U;
	uint32_t total_fail = 0U;
	int rc;

	rc = mctp_echo_start();
	if (rc) {
		return rc;
	}

	LOG_INF("MCTP requester started: local-eid=%u remote-eid=%u payload=%d count=%d",
		mctp_smbus.endpoint_id, REMOTE_EID, PAYLOAD_LEN, LOOP_COUNT);

	run_pattern(PAYLOAD_PATTERN_INCREMENT, "increment",
		    &total_pass, &total_fail);
	run_pattern(PAYLOAD_PATTERN_XOR, "xor", &total_pass, &total_fail);

	LOG_INF("MCTP summary: total_pass=%u total_fail=%u",
		total_pass, total_fail);

	return (total_fail == 0U) ? 0 : -EIO;
}
#endif

#if defined(CONFIG_MCTP_I2C_SMBUS_ECHO_ROLE_RESPONDER)
static int responder_main(void)
{
	uint32_t echo_count = 0U;
	int rc;

	rc = mctp_echo_start();
	if (rc) {
		return rc;
	}

	LOG_INF("MCTP responder started: local-eid=%u", mctp_smbus.endpoint_id);

	while (1) {
		rc = k_sem_take(&echo_ctx.rx_sem, K_FOREVER);
		if (rc || !echo_ctx.rx_ready) {
			continue;
		}

		rc = mctp_message_tx(mctp_ctx, echo_ctx.from_eid, false, 0,
				     echo_ctx.rx_buf, echo_ctx.rx_len);
		if (rc) {
			LOG_ERR("MCTP echo TX failed to EID %u rc=%d",
				echo_ctx.from_eid, rc);
		} else {
			echo_count++;
			LOG_INF("MCTP echoed message %u to EID %u len=%u",
				echo_count, echo_ctx.from_eid,
				(unsigned int)echo_ctx.rx_len);
		}

		echo_ctx.rx_ready = false;
		echo_ctx.rx_len = 0U;
	}
}
#endif

int main(void)
{
#if defined(CONFIG_MCTP_I2C_SMBUS_ECHO_ROLE_REQUESTER)
	return requester_main();
#elif defined(CONFIG_MCTP_I2C_SMBUS_ECHO_ROLE_RESPONDER)
	return responder_main();
#else
#error "Select requester or responder role"
#endif
}
