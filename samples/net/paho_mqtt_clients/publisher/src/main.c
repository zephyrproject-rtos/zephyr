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
#include <stdio.h>
#include <errno.h>

/* Network for Zephyr API - netz					*/
#include <netz.h>

#include "mqtt.h"

#define STACK_SIZE		2048
uint8_t stack[STACK_SIZE];

/* Change this value to modify the size of the tx and rx buffers	*/
#define BUF_SIZE		256
uint8_t tx_raw_buf[BUF_SIZE];
uint8_t rx_raw_buf[BUF_SIZE];

#define SLEEP_TIME		30

#define RC_STR(rc)		(rc == 0 ? "OK" : "ERROR")

void fiber(void)
{
	/* tx_buf and rx_buf are application-level buffers		*/
	struct app_buf_t tx_buf = APP_BUF_INIT(tx_raw_buf,
					       sizeof(tx_raw_buf), 0);
	struct app_buf_t rx_buf = APP_BUF_INIT(rx_raw_buf,
					       sizeof(rx_raw_buf), 0);

	/* netz context is initialized with default values. See netz.h	*/
	struct netz_ctx_t netz_ctx = NETZ_CTX_INIT;

	/* The MQTT Client Context - only MQTT-related stuff here	*/
	struct mqtt_client_ctx_t client_ctx = MQTT_CLIENT_CTX_INIT("zephyr");

	/* The MQTT Application Context: networking and mqtt stuff	*/
	struct mqtt_app_ctx_t app_ctx = MQTT_APP_INIT(&client_ctx, &netz_ctx,
						      &tx_buf, &rx_buf);
	/* MQTT message for publishing					*/
	struct mqtt_msg_t msg = MQTT_MSG_INIT;
	int retained = 0;
	int rc;

	/* First we configure network related stuff			*/
	netz_host_ipv4(&netz_ctx, 192, 168, 1, 110);
	netz_netmask_ipv4(&netz_ctx, 255, 255, 255, 0);
	/* MQTT server address and port					*/
	netz_remote_ipv4(&netz_ctx, 192, 168, 1, 10, 1883);

	/* MQTT context runtime initialization				*/
	mqtt_init(&app_ctx);
	/* Network buffers are now handled by the MQTT context		*/
	mqtt_buffers(&app_ctx, &tx_buf, &rx_buf);
	/* Now we let the MQTT context to handle the network context	*/
	mqtt_network(&app_ctx, &netz_ctx);
	/* Once everything is configured, we connect to the MQTT server	*/
	rc = mqtt_connect(&app_ctx);
	if (rc != 0) {
		printf("[%s:%d] Unable to connect: %d\n",
		       __func__, __LINE__, rc);
		return;
	}
	fiber_sleep(SLEEP_TIME);

	/* PUBLISH topic - we will use the same topic on future messages,
	 * so it makes sense to just assign it once.
	 */
	mqtt_msg_topic(&msg, "sensors");

	do {
		printf("\n--------------------------------\n");

		rc = mqtt_pingreq(&app_ctx);
		printf("Pingreq, rc: %s\n", RC_STR(rc));
		fiber_sleep(SLEEP_TIME);

		/* Message published with QoS0				*/
		mqtt_msg_payload_str(&msg, "OK QoS0");
		rc = mqtt_publish(&app_ctx, &msg, MQTT_QoS0, retained);
		printf("Publish QoS0, rc: %s\n", RC_STR(rc));
		fiber_sleep(SLEEP_TIME);

		/* Message published with QoS1				*/
		mqtt_msg_payload_str(&msg, "OK QoS1");
		rc = mqtt_publish(&app_ctx, &msg, MQTT_QoS1, retained);
		printf("Publish QoS1, rc: %s\n", RC_STR(rc));
		fiber_sleep(SLEEP_TIME);

		/* Message published with QoS2				*/
		mqtt_msg_payload_str(&msg, "OK QoS2");
		rc = mqtt_publish(&app_ctx, &msg, MQTT_QoS2, retained);
		printf("Publish QoS2, rc: %s\n", RC_STR(rc));
		fiber_sleep(SLEEP_TIME);

	} while (1);

}

void main(void)
{
	net_init();

	task_fiber_start(stack, STACK_SIZE, (nano_fiber_entry_t)fiber,
			 0, 0, 7, 0);
}
