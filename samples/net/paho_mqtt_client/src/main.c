/*
 * Copyright (c) 2016 Intel Corporation
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
#include <misc/printk.h>

#include "config.h"
#include "tcp.h"
#include "mqtt.h"

#define RC_MSG(rc)		(rc) == 0 ? "success" : "failure"
#define STACK_SIZE		1024

uint8_t stack[STACK_SIZE];

struct net_context *ctx;

void fiber(void)
{
	char *client_name = "zephyr_client";
	char *topic = "zephyr";
	char *msg = "Hello World from Zephyr!";
	/* it is assumed that these buffers have enough space to
	 * store the incoming topic and msg!
	 */
	char received_topic[32];
	char received_msg[64];
	int rc;

	do {
		rc = mqtt_connect(ctx, client_name);
		printk("Connect: %s\n", RC_MSG(rc));

		fiber_sleep(APP_SLEEP_TICKS);
	} while (rc != 0);

	do {

		rc = mqtt_subscribe(ctx, topic);
		printk("Subscribe: %s\n", RC_MSG(rc));

		fiber_sleep(APP_SLEEP_TICKS);
	} while (rc != 0);

	do {
		rc = mqtt_pingreq(ctx);
		printk("Pingreq: %s\n", RC_MSG(rc));

		rc = mqtt_publish(ctx, topic, msg);
		printk("Publish: %s\n", RC_MSG(rc));

		rc = mqtt_publish_read(ctx, received_topic, received_msg);
		if (rc == 0) {
			printk("\n\tReceived topic: %s, msg: %s\n",
			       received_topic, received_msg);
		}
		printk("Publish read: %s\n", RC_MSG(rc));

		fiber_sleep(APP_SLEEP_TICKS);
	} while (1);
}

void main(void)
{
	net_init();
	tcp_init(&ctx);

	task_fiber_start(stack, STACK_SIZE, (nano_fiber_entry_t)fiber,
			 0, 0, 7, 0);
}
