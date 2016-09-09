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

#include "dns_pack.h"

#include <stdio.h>
#include <errno.h>

int dns_print_msg_header(uint8_t *header, int size)
{
	if (size < DNS_MSG_HEADER_SIZE) {
		return -ENOMEM;
	}

	printf("\nHeader\n");
	printf("\tFlags\n");
	printf("\t\tTransaction ID:\t\t%d\n", dns_header_id(header));
	printf("\t\tMessage type:\t\t%d (%s)\n", dns_header_qr(header),
	       (dns_header_qr(header) == DNS_QUERY ? "query" : "response"));
	printf("\t\tOperation code:\t\t%d (%s)\n", dns_header_opcode(header),
	       (dns_header_opcode(header) == DNS_QUERY ? "query" : "other"));
	printf("\t\tAuthoritative:\t\t%d (%s)\n", dns_header_aa(header),
	       (dns_header_aa(header) ? "true" : "false"));
	printf("\t\tTruncated:\t\t%d (%s)\n", dns_header_tc(header),
	       (dns_header_tc(header) ? "true" : "false"));
	printf("\t\tRecursion desired:\t%d (%s)\n", dns_header_rd(header),
	       (dns_header_rd(header) ? "true" : "false"));
	printf("\t\tRecursion avaialable:\t%d (%s)\n", dns_header_ra(header),
	       (dns_header_ra(header) ? "true" : "false"));
	printf("\t\tZ:\t\t\t%d\n", dns_header_z(header));
	printf("\t\tResponse code:\t\t%d (%s)\n", dns_header_rcode(header),
	       (dns_header_rcode(header) == 0 ? "no error" : "error"));
	printf("\tQuestion counter:\t%d\n", dns_header_qdcount(header));
	printf("\tAnswer counter:\t\t%d\n", dns_header_ancount(header));
	printf("\tNServer counter:\t%d\n", dns_header_nscount(header));
	printf("\tAdditional counter:\t%d\n", dns_header_arcount(header));

	return 0;
}

int dns_print_label(uint8_t *label, int size)
{
	int n;
	int i;

	i = 0;
	while (i < size) {
		n = label[i];
		if (n == 0) {
			break;
		} else if (n > 63) {
			break;
		} else if (i + n <= size) {

			if (i) {
				printf(".");
			}
			int j = i + 1; /* next char			*/

			while (j < i + 1 + n) {
				printf("%c", label[j]);
				j++;
			}

			i += n + 1;	/* content + octect size	*/
		} else {
			i = 0;		/* no memory!			*/
			break;
		}
	}

	if (i == 0) {
		return -ENOMEM;
	}

	return 0;
}

int dns_print_msg_query(uint8_t *qname, int qname_size, int qtype, int qclass)
{
	printf("\nQuery\n");
	printf("\tQuery name\n\t\tLabel size:\t%d\n\t\tDomain name:\t",
	       qname_size);
	dns_print_label(qname, qname_size);
	printf("\n");
	printf("\tQuery type:\t\t%d\n", qtype);
	printf("\tQuery class:\t\t%d\n", qclass);

	return 0;
}

int dns_print_readable_msg_label(int offset, uint8_t *buf, int size)
{
	int next;
	int i;
	int j;

	for (i = offset; i < size;) {
		if (buf[i] <= 63) {
			/* +1 because the null label or a pointer */
			if (i + buf[i] + 1 >= size) {
				return -ENOMEM;
			}

			for (j = 1; j <= buf[i]; j++) {
				printf("%c", buf[i + j]);
			}
			i += buf[i] + 1;
			if (buf[i] == 0) {
				break;
			}
			printf(".");

		} else {
			if (i + 1 >= size) {
				return -ENOMEM;
			}

			next = ((buf[i] & 0x3F) << 8) + buf[i + 1];
			if (next >= size) {
				return -ENOMEM;
			}
			i = next;
			offset = next;
		}
	}

	return 0;
}

int print_buf(uint8_t *buf, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++) {
		printf("%d ", buf[i]);
	}

	return 0;
}
