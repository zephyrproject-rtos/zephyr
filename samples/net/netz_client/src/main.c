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

#include <netz.h>

#define STACK_SIZE		1024
uint8_t stack[STACK_SIZE];

/* Increment this value to avoid -ENOMEM errors				*/
#define BUF_SIZE		32
uint8_t rx_raw_buf[BUF_SIZE];
uint8_t tx_raw_buf[] = "Hello World!\n";

void print_buf(struct app_buf_t *buf);

void fiber(void)
{
	/* Buffers used for application level data processing		*/
	struct app_buf_t rx_buf = APP_BUF_INIT(rx_raw_buf,
					       sizeof(rx_raw_buf), 0);

	struct app_buf_t tx_buf = APP_BUF_INIT(tx_raw_buf,
					       sizeof(tx_raw_buf),
					       sizeof(tx_raw_buf));

	/* netz API context: it encapsulates Zephyr IP stack stuff	*/
	struct netz_ctx_t netz_ctx = NETZ_CTX_INIT;
	/* Using a network address initializer				*/
	struct net_addr remote = NET_ADDR_IPV4_INIT(192, 168, 1, 10);
	int rc;

	netz_host_ipv4(&netz_ctx, 192, 168, 1, 100);
	netz_netmask_ipv4(&netz_ctx, 255, 255, 255, 0);
	/* Another way to set a network address				*/
	netz_remote(&netz_ctx, &remote, 5555);

#ifdef CONFIG_NETWORKING_WITH_TCP
	rc = netz_tcp(&netz_ctx);
#else
	rc = netz_udp(&netz_ctx);
#endif
	if (rc != 0) {
		printf("[%s:%d] netz ctx error: %d\n",
		       __func__, __LINE__, rc);
		return;
	}

	do {
		printf("-------------------------------\n");

		rc = netz_tx(&netz_ctx, &tx_buf);
		if (rc != 0) {
			printf("[%s:%d] Unable to send data, error: %d\n",
			       __func__, __LINE__, rc);
		}

		rc = netz_rx(&netz_ctx, &rx_buf);
		if (rc == 0 || rc == -ENOMEM) {
			printf("Received: ");
			print_buf(&rx_buf);
			printf("\n");
		}
	} while (1);
}

void main(void)
{
	net_init();

	task_fiber_start(stack, STACK_SIZE, (nano_fiber_entry_t)fiber,
			 0, 0, 7, 0);
}

void print_buf(struct app_buf_t *buf)
{
	int i;

	printf("[%u] ", buf->length);
	for (i = 0; i < buf->length; i++) {
		printf("0x%02x ", buf->buf[i]);
	}
}
