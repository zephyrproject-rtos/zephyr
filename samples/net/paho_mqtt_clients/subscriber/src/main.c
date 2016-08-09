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

#include "netz.h"
#include "mqtt.h"

#define STACK_SIZE		2048
uint8_t stack[STACK_SIZE];

/* Change this value to modify the size of the tx and rx buffers	*/
#define BUF_SIZE		256
uint8_t tx_raw_buf[BUF_SIZE];
uint8_t rx_raw_buf[BUF_SIZE];

#define SLEEP_TIME		30

#define RC_STR(rc)		(rc == 0 ? "OK" : "ERROR")

/*
 * This is a callback function. Used to process a received PUBLISH message.
 * It must be installed before starting receiving messages. See mqtt_pack.h
 * for more information about mqtt_msg_t.
 *
 * Callback return codes are catched and returned by mqtt_read.
 */
int cb_publish(struct mqtt_msg_t *msg);

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
	struct mqtt_client_ctx_t client_ctx = MQTT_CLIENT_CTX_INIT("zephyr2");

	/* The MQTT Application Context: networking and mqtt stuff	*/
	struct mqtt_app_ctx_t app_ctx = MQTT_APP_INIT(&client_ctx, &netz_ctx,
						      &tx_buf, &rx_buf);
	int i = 0;
	int rc;

	/* First we configure network related stuff			*/
	netz_host_ipv4(&netz_ctx, 192, 168, 1, 111);
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
		printf("[%s:%d] Unable to connect: %s\n",
		       __func__, __LINE__, RC_STR(rc));
		return;
	}
	fiber_sleep(SLEEP_TIME);

	/* Subscribe returns on success Qos0, Qos1 or Qos2, or a
	 * negative value indicating that an error was found.
	 */
	rc = mqtt_subscribe(&app_ctx, "sensors", MQTT_QoS2);
	if (rc < 0) {
		printf("[%s:%d] Unable to subscribe: %s\n",
		       __func__, __LINE__, RC_STR(rc));
		return;
	}

	/* cb_publish will be called once a PUBLISH message arrives	*/
	mqtt_cb_publish(&app_ctx, cb_publish);

	do {
		printf("\n--------------------------------\n");

		mqtt_read(&app_ctx);
		fiber_sleep(SLEEP_TIME);

	} while (++i < 50);

	rc = mqtt_unsubscribe(&app_ctx, "sensors");
	if (rc < 0) {
		printf("[%s:%d] Unable to unsubscribe: %s\n",
		       __func__, __LINE__, RC_STR(rc));
		return;
	}

	rc = mqtt_disconnect(&app_ctx);
	if (rc < 0) {
		printf("[%s:%d] Unable to disconnect: %s\n",
		       __func__, __LINE__, RC_STR(rc));
		return;
	}

	printf("Bye!\n");
}

void main(void)
{
	net_init();

	task_fiber_start(stack, STACK_SIZE, (nano_fiber_entry_t)fiber,
			 0, 0, 7, 0);
}

int cb_publish(struct mqtt_msg_t *msg)
{
	static uint8_t helper_buf[BUF_SIZE];

	printf("[%s:%d] Received msg\n", __func__, __LINE__);

	if (msg->topic.length >= BUF_SIZE || msg->payload.length >= BUF_SIZE) {
		return -ENOMEM;
	}

	memcpy(helper_buf, msg->topic.buf, msg->topic.length);
	helper_buf[msg->topic.length] = '\0';
	printf("[%s:%d] Topic: %s\n", __func__, __LINE__, helper_buf);

	memcpy(helper_buf, msg->payload.buf, msg->payload.length);
	helper_buf[msg->payload.length] = '\0';
	printf("[%s:%d] Msg: %s\n", __func__, __LINE__, helper_buf);

	printf("[%s:%d] QoS: %d\n", __func__, __LINE__, msg->qos);
	if (msg->qos != MQTT_QoS0) {
		printf("[%s:%d] Pkt id: %d\n", __func__, __LINE__, msg->pkt_id);
	}

	return 0;
}

