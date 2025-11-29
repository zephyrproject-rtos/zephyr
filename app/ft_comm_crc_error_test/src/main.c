/*
 * Fault Tolerance Test - Communication CRC Error
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ft_comm_crc_test, LOG_LEVEL_INF);

#define TEST_DELAY_MS  2000

static volatile bool test_completed = false;

struct crc_context {
	const char *protocol;
	uint32_t expected_crc;
	uint32_t received_crc;
	uint32_t packet_id;
	size_t packet_size;
};

static ft_recovery_result_t crc_error_recovery(const ft_event_t *event)
{
	struct crc_context *ctx = NULL;

	LOG_WRN("=== CRC ERROR RECOVERY HANDLER ===");
	LOG_WRN("Fault Kind: %s", ft_kind_to_string(event->kind));

	if (event->context != NULL && event->context_size >= sizeof(struct crc_context)) {
		ctx = (struct crc_context *)event->context;
		LOG_WRN("Protocol: %s", ctx->protocol);
		LOG_WRN("Expected CRC: 0x%08x", ctx->expected_crc);
		LOG_WRN("Received CRC: 0x%08x", ctx->received_crc);
		LOG_WRN("Packet ID: %u", ctx->packet_id);
		LOG_WRN("Packet Size: %u bytes", ctx->packet_size);
	}

	LOG_INF("Requesting packet retransmission...");
	test_completed = true;
	return FT_RECOVERY_SUCCESS;
}

int main(void)
{
	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: CRC Error");
	LOG_INF("========================================");

	ft_init();
	ft_register_handler(FT_COMM_CRC_ERROR, crc_error_recovery);
	ft_enable_detection(FT_COMM_CRC_ERROR);

	k_msleep(TEST_DELAY_MS);

	LOG_WRN("=== INITIATING CRC ERROR TEST ===");

	struct crc_context ctx = {
		.protocol = "UART",
		.expected_crc = 0x12345678,
		.received_crc = 0x12345600,
		.packet_id = 42,
		.packet_size = 128
	};

	ft_event_t event = {
		.kind = FT_COMM_CRC_ERROR,
		.severity = FT_SEV_WARNING,
		.domain = FT_DOMAIN_COMMUNICATION,
		.code = 0x7000,
		.timestamp = k_uptime_get(),
		.thread_id = k_current_get(),
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	ft_report_fault(&event);
	k_msleep(100);

	if (test_completed) {
		LOG_INF("=== TEST RESULT: PASS ===");
	}

	return 0;
}
