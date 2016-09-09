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
#include <string.h>
#include <errno.h>

#include "tcp_config.h"
#include "tcp.h"

#include "dns_utils.h"
#include "dns_pack.h"

#define STACK_SIZE	1024
uint8_t stack[STACK_SIZE];

#define BUF_SIZE	1024
uint8_t tx_buf[BUF_SIZE];
uint8_t rx_buf[BUF_SIZE];

size_t tx_len;
size_t rx_len;

struct net_context *ctx;

#define RC_STR(rc)	(rc == 0 ? "OK" : "ERROR")

char *domains[] = {"not_a_real_domain_name",
		   "linuxfoundation.org", "www.linuxfoundation.org",
		   "wikipedia.org", "www.wikipedia.org",
		   "zephyrproject.org", "www.zephyrproject.org",
		   NULL};

/* this function creates the DNS query					*/
int dns_query(uint8_t *buf, size_t *len, size_t size,
	      char *str, uint16_t id, enum dns_rr_type qtype);

/* this function parses the DNS server response				*/
int dns_response(uint8_t *buf, size_t size, int *response_type, int src_id);

void fiber(void)
{
	int response_type;
	int counter;
	char *name;
	int rc;

	rc = tcp_init(&ctx);
	if (rc != 0) {
		printk("[%s:%d] Unable to init net_context\n",
		       __func__, __LINE__);
		return;
	}

	counter = 0;
	do {
		printf("\n-----------------------------------------\n");

		name = domains[counter];
		if (name == NULL) {
			counter = 0;
			continue;
		}

		printf("Domain name: %s\n", name);

		rc = dns_query(tx_buf, &tx_len, sizeof(tx_buf), name, counter,
			       DNS_RR_TYPE_A);
		printf("[%s:%d] DNS Query: %s, ID: %d\n",
		       __func__, __LINE__, RC_STR(rc), counter);

		rc = tcp_tx(ctx, tx_buf, tx_len);
		printf("[%s:%d] TX: %s\n", __func__, __LINE__, RC_STR(rc));

		fiber_sleep(APP_SLEEP_TICKS);

		rc = tcp_rx(ctx, rx_buf, &rx_len, sizeof(rx_buf));
		printf("[%s:%d] RX: %s\n", __func__, __LINE__, RC_STR(rc));

		if (rc == 0) {
			rc = dns_response(rx_buf, rx_len,
					  &response_type, counter);
			printf("[%s:%d] DNS Response: %s %s\n",
			       __func__, __LINE__, RC_STR(rc),
			       (rc != 0 && counter == 0 ? "<- :)" : ""));
		}

		counter += 1;
		fiber_sleep(APP_SLEEP_TICKS);
	} while (1);

}

void main(void)
{
	net_init();

	task_fiber_start(stack, STACK_SIZE, (nano_fiber_entry_t)fiber,
			 0, 0, 7, 0);
}

/* Next versions must handle the transaction id internally		*/
int dns_query(uint8_t *buf, size_t *len, size_t size,
	      char *str, uint16_t id, enum dns_rr_type qtype)
{
	return dns_msg_pack_query(buf, len, size, str, id, qtype);
}

/* See dns_unpack_answer, and also see:
 * https://tools.ietf.org/html/rfc1035#section-4.1.2
 */
#define DNS_QUERY_POS	0x0c

int dns_response(uint8_t *buf, size_t size, int *response_type, int src_id)
{
	struct dns_msg_t dns_msg = DNS_MSG_INIT(buf, size);
	int ptr;
	int rc;
	int i;

	rc = dns_unpack_response_header(&dns_msg, src_id);
	if (rc != 0) {
		return -EINVAL;
	}

	if (dns_header_qdcount(dns_msg.msg) != 1) {
		return -EINVAL;
	}

	rc = dns_unpack_response_query(&dns_msg);
	if (rc != 0) {
		return -EINVAL;
	}

	i = 0;
	ptr = DNS_QUERY_POS;
	while (i < dns_header_ancount(dns_msg.msg)) {

		printf("\n****** DNS ANSWER: %d ******\n", i);

		rc = dns_unpack_answer(&dns_msg, ptr);
		if (rc != 0) {
			return -EINVAL;
		}

		switch (dns_msg.response_type) {
		case DNS_RESPONSE_IP:
			printf("Response: IP address\t\tSize: %d:\t",
			       dns_msg.response_length);
			print_buf(dns_msg.msg + dns_msg.response_position,
				  dns_msg.response_length);
			printf("\n");
			break;

		case DNS_RESPONSE_CNAME_NO_IP:
			printf("Response: CNAME NO IP address");
			printf("\nCNAME: ");
			dns_print_readable_msg_label(dns_msg.response_position,
						     dns_msg.msg,
						     dns_msg.msg_size);
			printf("\n");
			ptr = dns_msg.response_position;

			break;

		case DNS_RESPONSE_CNAME_WITH_IP:
			printf("Response: CNAME WITH IP addr\t\tSize: %d:\t",
			       dns_msg.response_length);
			break;
		}

		dns_msg.answer_offset = dns_msg.answer_offset + 12 +
					dns_msg.response_length;
		++i;
	}

	*response_type = dns_msg.response_type;

	return 0;
}
