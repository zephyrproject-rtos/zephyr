/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>

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

void log_msgs(void)
{
	struct test_frame frame = { 0 };
	const uint8_t data[DATA_MAX_DLEN] = { 0x01, 0x02, 0x03, 0x04,
					0x05, 0x06, 0x07, 0x08 };

	char c = '!';
	const char *s = "static str";
	const char *s1 = "c str";
	char vs0[32];
	char vs1[32];

	/* standard print */
	LOG_ERR("Error message example.");
	LOG_WRN("Warning message example.");
	LOG_INF("Info message example.");
	LOG_DBG("Debug message example.");

	LOG_DBG("Debug message example, %d", 1);
	LOG_DBG("Debug message example, %d, %d", 1, 2);
	LOG_DBG("Debug message example, %d, %d, %d", 1, 2, 3);
	LOG_DBG("Debug message example, %d, %d, %d, 0x%x", 1, 2, 3, 4);

	memset(vs0, 0, sizeof(vs0));
	snprintk(&vs0[0], sizeof(vs0), "%s", "dynamic str");

	memset(vs1, 0, sizeof(vs1));
	snprintk(&vs1[0], sizeof(vs1), "%s", "another dynamic str");

	LOG_DBG("char %c", c);
	LOG_DBG("s str %s %s", s, s1);

	LOG_DBG("d str %s", vs0);
	LOG_DBG("mixed str %s %s %s %s %s %s %s", vs0, "---", vs0, "---", vs1, "---", vs1);
	LOG_DBG("mixed c/s %c %s %s %s %c", c, s, vs0, s, c);

	LOG_DBG("Debug message example, %f", 3.14159265359);

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

#if CONFIG_LOG_MODE_DEFERRED
	/*
	 * When deferred logging is enabled, the work is being performed by
	 * another thread. This k_sleep() gives that thread time to process
	 * those messages.
	 */

	k_sleep(K_TICKS(10));
#endif
}

int main(void)
{
	log_msgs();

	uint32_t log_type = LOG_OUTPUT_TEXT;

	log_format_set_all_active_backends(log_type);

	log_msgs();

	log_type = LOG_OUTPUT_SYST;
	log_format_set_all_active_backends(log_type);

	log_msgs();

	log_type = LOG_OUTPUT_TEXT;
	log_format_set_all_active_backends(log_type);

	/* raw string */
	printk("SYST Sample Execution Completed\n");
	return 0;
}
