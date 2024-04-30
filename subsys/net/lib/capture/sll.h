/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Defines for Linux cooked mode capture (SLL) */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

/* Useful information about SLL header format can be found here
 * https://www.tcpdump.org/linktypes/LINKTYPE_LINUX_SLL.html
 */

#define SLL_HDR_LEN  16 /* Total header length         */
#define SLL_ADDRLEN   8 /* Length of the address field */

struct sll_header {
	uint16_t sll_pkttype;           /* Packet type               */
	uint16_t sll_hatype;            /* Link-layer address type   */
	uint16_t sll_halen;             /* Link-layer address length */
	uint8_t  sll_addr[SLL_ADDRLEN]; /* Link-layer address        */
	uint16_t sll_protocol;          /* Protocol                  */
};

BUILD_ASSERT(sizeof(struct sll_header) == SLL_HDR_LEN);

#define SLL2_HDR_LEN 20 /* Total header length         */

struct sll2_header {
	uint16_t sll2_protocol;          /* Protocol                  */
	uint16_t sll2_reserved_mbz;      /* Reserved - must be zero   */
	uint32_t sll2_if_index;          /* 1-based interface index   */
	uint16_t sll2_hatype;            /* Link-layer address type   */
	uint8_t  sll2_pkttype;           /* Packet type               */
	uint8_t  sll2_halen;             /* Link-layer address length */
	uint8_t  sll2_addr[SLL_ADDRLEN]; /* Link-layer address        */
};

BUILD_ASSERT(sizeof(struct sll2_header) == SLL2_HDR_LEN);

#define SLL_HOST       0 /* packet was sent to us by somebody else */
#define SLL_BROADCAST  1 /* packet was broadcast by somebody else */
#define SLL_MULTICAST  2 /* packet was multicast, but not broadcast, by somebody else */
#define SLL_OTHERHOST  3 /* packet was sent by somebody else to somebody else */
#define SLL_OUTGOING   4 /* packet was sent by us */
