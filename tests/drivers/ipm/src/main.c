/*
 * Copyright (c) 2015-2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/console/ipm_console.h>
#include <zephyr/device.h>
#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/init.h>
#include <stdio.h>

#include <zephyr/tc_util.h>
#include "ipm_dummy.h"

#define PRINTK_OUT      1

#if PRINTK_OUT
#define SOURCE  IPM_CONSOLE_STDOUT
#define DEST    IPM_CONSOLE_PRINTK
#else
#define SOURCE  IPM_CONSOLE_PRINTK
#define DEST    IPM_CONSOLE_STDOUT
#endif

#define INIT_PRIO_IPM_SEND 40
#define INIT_PRIO_IPM_RECV 50

extern struct ipm_driver_api ipm_dummy_api;

/* Set up the dummy IPM driver */
struct ipm_dummy_driver_data ipm_dummy0_driver_data;
DEVICE_DEFINE(ipm_dummy0, "ipm_dummy0", NULL,
		NULL, &ipm_dummy0_driver_data, NULL,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&ipm_dummy_api);

/* Sending side of the console IPM driver, will forward anything sent
 * to printf() since we selected IPM_CONSOLE_STDOUT
 */
static struct ipm_console_sender_config_info sender_config = {
	.bind_to = "ipm_dummy0",
	.flags = SOURCE
};
DEVICE_DEFINE(ipm_console_send0, "ipm_send0", ipm_console_sender_init,
	      NULL, NULL, &sender_config,
	      POST_KERNEL, INIT_PRIO_IPM_SEND, NULL);

/* Receiving side of the console IPM driver. These numbers are
 * more or less arbitrary
 */
#define LINE_BUF_SIZE           80
#define RING_BUF_SIZE32         8

static uint32_t ring_buf_data[RING_BUF_SIZE32];
static K_THREAD_STACK_DEFINE(thread_stack, IPM_CONSOLE_STACK_SIZE);
static char line_buf[LINE_BUF_SIZE];

/* Dump incoming messages to printk() */
static struct ipm_console_receiver_config_info receiver_config = {
	.bind_to = "ipm_dummy0",
	.thread_stack = thread_stack,
	.ring_buf_data = ring_buf_data,
	.rb_size32 = RING_BUF_SIZE32,
	.line_buf = line_buf,
	.lb_size = LINE_BUF_SIZE,
	.flags = DEST
};

struct ipm_console_receiver_runtime_data receiver_data;
DEVICE_DEFINE(ipm_console_recv0, "ipm_recv0", ipm_console_receiver_init,
	      NULL, &receiver_data, &receiver_config,
	      POST_KERNEL, INIT_PRIO_IPM_RECV, NULL);

static const char thestr[] = "everything is awesome\n";

int main(void)
{
	int rv, i;
	const struct device *ipm;

	rv = TC_PASS;

	TC_SUITE_START("test_ipm");
	ipm = device_get_binding("ipm_dummy0");
	if (ipm == NULL) {
		TC_ERROR("unable to get device 'ipm_dummy0'\n");
		rv = TC_FAIL;
	} else {
		/* Try sending a raw string to the IPM device to show that the
		 * receiver works
		 */
		int rc = 0;

		for (i = 0; i < strlen(thestr) && rc == 0; i++) {
			rc = ipm_send(ipm, 1, thestr[i], NULL, 0);
		}
		if (rc) {
			TC_ERROR("ipm_send() error=%u\n", rc);
			rv = TC_FAIL;
		} else {
			/* Now do this through printf() to exercise the sender */
			/* I will be split to lines of LINE_BUF_SIZE           */
			printf(LOREM_IPSUM_SHORT "\n");
		}
	}
	/* Twister Console Harness checks the output actually printed out for
	 * automation purposes.
	 */

	TC_END_RESULT(rv);
	TC_SUITE_END("test_ipm", rv);
	TC_END_REPORT(rv);
	return 0;
}
