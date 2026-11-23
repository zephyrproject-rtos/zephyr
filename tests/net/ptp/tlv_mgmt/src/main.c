/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/ztest.h>

#include "tlv.h"

/* Bytes after the TLV that the parser must not touch */
#define GUARD_LEN  16
#define GUARD_BYTE 0xa5

/* MANAGEMENT TLV with a CLOCK_DESCRIPTION, as received (network byte order) */
static const uint8_t clock_desc_tlv[] = {
	0x00, 0x01,                         /* tlvType: MANAGEMENT */
	0x00, 0x2c,                         /* lengthField: 44 */
	0x00, 0x01,                         /* managementId: CLOCK_DESCRIPTION */
	0x80, 0x00,                         /* clockType: ordinary clock */
	0x03, 'p',  'h',  'y',              /* physicalLayerProtocol */
	0x00, 0x06,                         /* physicalAddressLength */
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, /* physicalAddress */
	0x00, 0x01, 0x00, 0x04,             /* protocolAddress: UDP/IPv4, 4 bytes */
	192,  168,  1,    10,               /* protocolAddress: address */
	0x00, 0x04, 0x9f, 0x00,             /* manufacturerIdentity, reserved */
	0x03, 'p',  'r',  'd',              /* productDescription */
	0x01, '1',                          /* revisionData */
	0x03, 'u',  's',  'r',              /* userDescription */
	0x00, 0x1b, 0x19, 0x00, 0x01, 0x00, /* profileIdentity */
};

/* MANAGEMENT TLV with a USER_DESCRIPTION, as received */
static const uint8_t user_desc_tlv[] = {
	0x00, 0x01,           /* tlvType: MANAGEMENT */
	0x00, 0x06,           /* lengthField: 6 */
	0x00, 0x02,           /* managementId: USER_DESCRIPTION */
	0x03, 'a',  'b', 'c', /* userDescription */
};

static uint8_t rx_buf[64 + GUARD_LEN] __aligned(4);

/* What msg.c does before it hands a TLV to ptp_tlv_post_recv() */
static struct ptp_tlv_container *receive_tlv(const uint8_t *wire, size_t len)
{
	struct ptp_tlv_container *container = ptp_tlv_alloc();

	zassert_not_null(container, "no TLV container");
	zassert_true(len + GUARD_LEN <= sizeof(rx_buf), "test TLV too long");

	memset(rx_buf, GUARD_BYTE, sizeof(rx_buf));
	memcpy(rx_buf, wire, len);

	container->tlv = (struct ptp_tlv *)rx_buf;
	container->tlv->type = net_ntohs(container->tlv->type);
	container->tlv->length = net_ntohs(container->tlv->length);

	return container;
}

static void assert_guard_intact(size_t len)
{
	for (size_t i = len; i < sizeof(rx_buf); i++) {
		zassert_equal(rx_buf[i], GUARD_BYTE, "byte %zu after the TLV was written", i - len);
	}
}

/*
 * The parsed fields are kept in the container that holds the TLV, and point into the
 * received bytes. The received bytes change only where they are converted to host order.
 */
ZTEST(ptp_tlv_mgmt, test_clock_description_is_parsed_into_container)
{
	struct ptp_tlv_container *container = receive_tlv(clock_desc_tlv, sizeof(clock_desc_tlv));
	struct ptp_tlv_mgmt_clock_desc *desc = &container->clock_desc;
	int ret = ptp_tlv_post_recv(&container->tlv);

	zassert_equal(ret, 0, "parse failed with %d", ret);

	zassert_equal_ptr(desc->type, &rx_buf[6], "clockType not in the received TLV");
	zassert_equal(*desc->type, 0x8000, "clockType 0x%04x", *desc->type);
	zassert_equal_ptr(desc->phy_protocol, &rx_buf[8], "physicalLayerProtocol");
	zassert_equal(desc->phy_protocol->length, 3, "physicalLayerProtocol length");
	zassert_equal(*desc->phy_addr_len, 6, "physicalAddressLength %u", *desc->phy_addr_len);
	zassert_equal_ptr(desc->phy_addr, &rx_buf[14], "physicalAddress");
	zassert_equal(desc->protocol_addr->protocol, 1, "networkProtocol");
	zassert_equal(desc->protocol_addr->addr_len, 4, "addressLength");
	zassert_equal_ptr(desc->manufacturer_id, &rx_buf[28], "manufacturerIdentity");
	zassert_equal(desc->product_desc->length, 3, "productDescription length");
	zassert_equal(desc->revision_data->length, 1, "revisionData length");
	zassert_mem_equal(desc->user_desc->text, "usr", 3, "userDescription");
	zassert_equal_ptr(desc->profile_id, &rx_buf[42], "profileIdentity");

	/* Text fields are not converted, so they must be exactly as received */
	zassert_mem_equal(&rx_buf[8], &clock_desc_tlv[8], 4, "physicalLayerProtocol changed");
	zassert_mem_equal(&rx_buf[28], &clock_desc_tlv[28], 20, "trailing fields changed");
	assert_guard_intact(sizeof(clock_desc_tlv));

	ptp_tlv_free(container);
}

/* A received CLOCK_DESCRIPTION goes back to the same bytes when it is sent on */
ZTEST(ptp_tlv_mgmt, test_clock_description_round_trip)
{
	struct ptp_tlv_container *container = receive_tlv(clock_desc_tlv, sizeof(clock_desc_tlv));
	int ret = ptp_tlv_post_recv(&container->tlv);

	zassert_equal(ret, 0, "parse failed with %d", ret);

	ptp_tlv_pre_send(&container->tlv);

	zassert_mem_equal(rx_buf, clock_desc_tlv, sizeof(clock_desc_tlv),
			  "TLV differs from the received one after pre_send");
	assert_guard_intact(sizeof(clock_desc_tlv));

	ptp_tlv_free(container);
}

ZTEST(ptp_tlv_mgmt, test_user_description_is_parsed_into_container)
{
	struct ptp_tlv_container *container = receive_tlv(user_desc_tlv, sizeof(user_desc_tlv));
	int ret = ptp_tlv_post_recv(&container->tlv);

	zassert_equal(ret, 0, "parse failed with %d", ret);
	zassert_equal_ptr(container->clock_desc.user_desc, &rx_buf[6], "userDescription");
	zassert_equal(container->clock_desc.user_desc->length, 3, "userDescription length");
	assert_guard_intact(sizeof(user_desc_tlv));

	ptp_tlv_free(container);
}

ZTEST_SUITE(ptp_tlv_mgmt, NULL, NULL, NULL, NULL, NULL);
