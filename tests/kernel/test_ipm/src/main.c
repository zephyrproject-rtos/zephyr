/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <ipm.h>
#include <drivers/console/ipm_console.h>
#include <device.h>
#include <init.h>
#include <stdio.h>

#include <tc_util.h>
#include "ipm_dummy.h"

#define PRINTK_OUT	1

#if PRINTK_OUT
#define SOURCE	IPM_CONSOLE_STDOUT
#define DEST	IPM_CONSOLE_PRINTK
#else
#define SOURCE	IPM_CONSOLE_PRINTK
#define DEST	IPM_CONSOLE_STDOUT
#endif

#define INIT_PRIO_IPM_SEND 50

/* Set up the dummy IPM driver */
struct ipm_dummy_driver_data ipm_dummy0_driver_data;
DEVICE_INIT(ipm_dummy0, "ipm_dummy0", ipm_dummy_init,
				&ipm_dummy0_driver_data, NULL,
				SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* Sending side of the console IPM driver, will forward anything sent
 * to printf() since we selected IPM_CONSOLE_STDOUT
 */
static struct ipm_console_sender_config_info sender_config = {
	.bind_to = "ipm_dummy0",
	.flags = SOURCE
};
DEVICE_INIT(ipm_console_send0, "ipm_send0", ipm_console_sender_init,
				NULL, &sender_config,
				NANOKERNEL, INIT_PRIO_IPM_SEND);

/* Receiving side of the console IPM driver. These numbers are
 * more or less arbitrary
 */
#define LINE_BUF_SIZE		80
#define RING_BUF_SIZE32		8

static uint32_t ring_buf_data[RING_BUF_SIZE32];
static char __stack thread_stack[IPM_CONSOLE_STACK_SIZE];
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
DEVICE_INIT(ipm_console_recv0, "ipm_recv0", ipm_console_receiver_init,
				&receiver_data, &receiver_config,
				NANOKERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static const char thestr[] = "everything is awesome\n";

void main(void)
{
	int rv, i;
	struct device *ipm;

	TC_START("Test IPM");
	ipm = device_get_binding("ipm_dummy0");

	/* Try sending a raw string to the IPM device to show that the
	 * receiver works
	 */
	for (i = 0; i < strlen(thestr); i++) {
		ipm_send(ipm, 1, thestr[i], NULL, 0);
	}

	/* Now do this through printf() to exercise the sender */
	printf("Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
	       "sed do eiusmod tempor incididunt ut labore et dolore magna "
	       "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
	       "ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis "
	       "aute irure dolor in reprehenderit in voluptate velit esse "
	       "cillum dolore eu fugiat nulla pariatur. Excepteur sint "
	       "occaecat cupidatat non proident, sunt in culpa qui officia "
	       "deserunt mollit anim id est laborum.\n");

	/* XXX how to tell if something was actually printed out for
	 * automation purposes?
	 */

	rv = TC_PASS;
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}


