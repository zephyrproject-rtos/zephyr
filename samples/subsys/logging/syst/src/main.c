/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#include <sys/printk.h>

#define DATA_MAX_DLEN 8
#define LOG_MODULE_NAME syst
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

struct test_frame {
	uint32_t id_type : 1;
	uint32_t rtr     : 1;
	/* Message identifier */
	union {
		uint32_t std_id  : 11;
		uint32_t ext_id  : 29;
	};
	/* The length of the message (max. 8) in byte */
	uint8_t dlc;
	/* The message data */
	union {
		uint8_t data[8];
		uint32_t data_32[2];
	};
} __packed;

void main(void)
{
	struct test_frame frame = { 0 };
	const uint8_t data[DATA_MAX_DLEN] = { 0x01, 0x02, 0x03, 0x04,
					0x05, 0x06, 0x07, 0x08 };
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_INTERNAL_RAW_STRING,
		.source_id = 0, /* not used as level indicates raw string. */
		.domain_id = 0, /* not used as level indicates raw string. */
	};


	/* standard print */
	LOG_ERR("Error message example.");
	LOG_WRN("Warning message example.");
	LOG_INF("Info message example.");
	LOG_DBG("Debug message example.");

	LOG_DBG("Debug message example, %d", 1);
	LOG_DBG("Debug message example, %d, %d", 1, 2);
	LOG_DBG("Debug message example, %d, %d, %d", 1, 2, 3);
	LOG_DBG("Debug message example, %d, %d, %d, 0x%x", 1, 2, 3, 4);

	/* hexdump */
	frame.rtr = 1U;
	frame.id_type = 1U;
	frame.std_id = 1234U;
	frame.dlc = sizeof(data);
	memcpy(frame.data, data, sizeof(data));

	LOG_HEXDUMP_ERR((const uint8_t *)&frame, sizeof(frame), "frame");
	LOG_HEXDUMP_WRN((const uint8_t *)&frame, sizeof(frame), "frame");
	LOG_HEXDUMP_INF((const uint8_t *)&frame, sizeof(frame), "frame");
	LOG_HEXDUMP_DBG((const uint8_t *)&frame, sizeof(frame), "frame");

	/* raw string */
	printk("hello sys-t on board %s\n", CONFIG_BOARD);

	/* log output string */
	log_string_sync(src_level, "%s", "log string sync");
}
