/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>

#define NUM_STD_CODECS 3
static const struct bt_hci_std_codec_info_v2 std_codecs[NUM_STD_CODECS] = {
	{ BT_HCI_CODING_FORMAT_ALAW_LOG,
	  (BT_HCI_CODEC_TRANSPORT_MASK_BREDR_ACL |
	   BT_HCI_CODEC_TRANSPORT_MASK_BREDR_SCO) },
	{ BT_HCI_CODING_FORMAT_TRANSPARENT,
	  BT_HCI_CODEC_TRANSPORT_MASK_LE_CIS },
	{ BT_HCI_CODING_FORMAT_LINEAR_PCM,
	  BT_HCI_CODEC_TRANSPORT_MASK_LE_BIS },
};

#define NUM_VS_CODECS 2
static const struct bt_hci_vs_codec_info_v2 vs_codecs[NUM_VS_CODECS] = {
	{ BT_COMP_ID_LF, 23,
	  BT_HCI_CODEC_TRANSPORT_MASK_LE_CIS },
	{ BT_COMP_ID_LF, 42,
	  (BT_HCI_CODEC_TRANSPORT_MASK_LE_CIS |
	   BT_HCI_CODEC_TRANSPORT_MASK_LE_BIS) },
};


uint8_t hci_vendor_read_std_codecs(
	const struct bt_hci_std_codec_info_v2 **codecs)
{
	*codecs = std_codecs;
	return NUM_STD_CODECS;
}

uint8_t hci_vendor_read_vs_codecs(
	const struct bt_hci_vs_codec_info_v2 **codecs)
{
	*codecs = vs_codecs;
	return NUM_VS_CODECS;
}

ZTEST_SUITE(test_hci_codecs_info, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_hci_codecs_info, test_read_codecs)
{
	int err;

	/* Initialize bluetooth subsystem */
	bt_enable(NULL);

	/*
	 * An LE controller shall no longer support
	 * HCI_Read_Local_Supported_Codecs [v1]
	 * according to BT Core 5.3.
	 */


	/* Read Local Supported Codecs */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_CODECS, NULL, NULL);
	zassert_not_equal(err, 0, "Reading local supported codecs failed");

}

ZTEST(test_hci_codecs_info, test_read_codecs_v2)
{
	const struct bt_hci_rp_read_codecs_v2 *codecs;
	struct net_buf *rsp;
	uint8_t num_std_codecs;
	uint8_t num_vs_codecs;
	const uint8_t *ptr;
	uint8_t i;
	int err;

	/* Initialize bluetooth subsystem */
	bt_enable(NULL);

	/* Read Local Supported Codecs */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_CODECS_V2, NULL, &rsp);
	zassert_equal(err, 0, "Reading local supported codecs v2 failed");

	/* Check returned data */
	codecs = (const struct bt_hci_rp_read_codecs_v2 *)rsp->data;
	zassert_equal(codecs->status, 0,
		      "Reading local supported codecs v2 status failed");

	ptr = (uint8_t *)&codecs->status + sizeof(codecs->status);
	num_std_codecs = *ptr++;
	zassert_equal(num_std_codecs, NUM_STD_CODECS,
		      "Reading std codecs count failed");

	for (i = 0; i < num_std_codecs; i++) {
		const struct bt_hci_std_codec_info_v2 *codec;

		codec = (const struct bt_hci_std_codec_info_v2 *)ptr;
		ptr += sizeof(*codec);
		zassert_equal(codec->codec_id, std_codecs[i].codec_id,
			      "Reading std codecs codec_id %d failed", i);
		zassert_equal(codec->transports, std_codecs[i].transports,
			      "Reading std codecs transports %d failed", i);
	}

	num_vs_codecs = *ptr++;
	zassert_equal(num_vs_codecs, NUM_VS_CODECS,
		      "Reading vendor codecs count failed");
	for (i = 0; i < num_vs_codecs; i++) {
		const struct bt_hci_vs_codec_info_v2 *codec;

		codec = (const struct bt_hci_vs_codec_info_v2 *)ptr;
		ptr += sizeof(*codec);
		zassert_equal(codec->company_id,
			      sys_cpu_to_le16(vs_codecs[i].company_id),
			      "Reading vendor codecs company_id %d failed", i);
		zassert_equal(codec->codec_id,
			      sys_cpu_to_le16(vs_codecs[i].codec_id),
			      "Reading vendor codecs codec_id %d failed", i);
		zassert_equal(codec->transports, vs_codecs[i].transports,
			      "Reading vendor codecs transports %d failed", i);
	}

	net_buf_unref(rsp);
}

#define NUM_CAPABILITIES 2
#define CODEC_CAPAB_0 'Z', 'e', 'p', 'h', 'y', 'r'
#define CODEC_CAPAB_1 'C', 'o', 'd', 'e', 'c'
static const uint8_t codec_capabilities[] = {
	sizeof((uint8_t []){CODEC_CAPAB_0}), CODEC_CAPAB_0,
	sizeof((uint8_t []){CODEC_CAPAB_1}), CODEC_CAPAB_1,
};

#define READ_CAPABS_CODING_FMT 0xff
#define READ_CAPABS_COMPANY_ID 0x1234
#define READ_CAPABS_VS_CODEC_ID 0x5678
#define READ_CAPABS_TRANSPORT BT_HCI_LOGICAL_TRANSPORT_TYPE_LE_CIS
#define READ_CAPABS_DIRECTION BT_HCI_DATAPATH_DIR_CTLR_TO_HOST

uint8_t hci_vendor_read_codec_capabilities(uint8_t coding_format,
					   uint16_t company_id,
					   uint16_t vs_codec_id,
					   uint8_t transport,
					   uint8_t direction,
					   uint8_t *num_capabilities,
					   size_t *capabilities_bytes,
					   const uint8_t **capabilities)
{
	/* Check input parameters */
	zassert_equal(coding_format, READ_CAPABS_CODING_FMT,
		      "Reading codec capabilities passed wrong coding_format");
	zassert_equal(company_id, READ_CAPABS_COMPANY_ID,
		      "Reading codec capabilities passed wrong company_id");
	zassert_equal(vs_codec_id, READ_CAPABS_VS_CODEC_ID,
		      "Reading codec capabilities passed wrong vs_codec_id");
	zassert_equal(transport, READ_CAPABS_TRANSPORT,
		      "Reading codec capabilities passed wrong transport");
	zassert_equal(direction, READ_CAPABS_DIRECTION,
		      "Reading codec capabilities passed wrong direction");

	/* Fill in response data */
	*num_capabilities = NUM_CAPABILITIES;
	*capabilities_bytes = sizeof(codec_capabilities);
	*capabilities = codec_capabilities;

	return 0;
}

ZTEST(test_hci_codecs_info, test_read_codec_capabilities)
{
	struct bt_hci_cp_read_codec_capabilities *cp;
	struct bt_hci_rp_read_codec_capabilities *rp;
	struct net_buf *buf;
	struct net_buf *rsp;
	const uint8_t *ptr;
	int err;

	/* Initialize bluetooth subsystem */
	bt_enable(NULL);

	/* Read Local Supported Codec Capabilities */
	buf = bt_hci_cmd_create(BT_HCI_OP_READ_CODEC_CAPABILITIES, sizeof(*cp));
	cp = net_buf_add(buf, sizeof(*cp));

	cp->codec_id.coding_format = READ_CAPABS_CODING_FMT;
	cp->codec_id.company_id = sys_cpu_to_le16(READ_CAPABS_COMPANY_ID);
	cp->codec_id.vs_codec_id = sys_cpu_to_le16(READ_CAPABS_VS_CODEC_ID);
	cp->transport = READ_CAPABS_TRANSPORT;
	cp->direction = READ_CAPABS_DIRECTION;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_CODEC_CAPABILITIES,
				   buf, &rsp);
	zassert_equal(err, 0,
		      "Reading local supported codec capabilities failed");

	/* Check returned data */
	rp = (struct bt_hci_rp_read_codec_capabilities *)rsp->data;
	zassert_equal(rp->status, 0,
		      "Reading codec capabilities status failed");
	zassert_equal(rp->num_capabilities, NUM_CAPABILITIES,
		      "Reading codec capabilities count failed");
	ptr = &rp->capabilities[0];
	zassert_mem_equal(ptr, codec_capabilities, sizeof(codec_capabilities),
			  0, "Reading codec capabilities content failed");
}

#define READ_DELAY_CODING_FMT 0xff
#define READ_DELAY_COMPANY_ID 0x9abc
#define READ_DELAY_VS_CODEC_ID 0xdef0
#define READ_DELAY_TRANSPORT BT_HCI_LOGICAL_TRANSPORT_TYPE_LE_BIS
#define READ_DELAY_DIRECTION BT_HCI_DATAPATH_DIR_HOST_TO_CTLR
const uint8_t read_delay_codec_config[] = { 17, 23, 42, 18, 86 };

#define MIN_CTLR_DELAY 0x12
#define MAX_CTLR_DELAY 0x3456

uint8_t hci_vendor_read_ctlr_delay(uint8_t coding_format,
				   uint16_t company_id,
				   uint16_t vs_codec_id,
				   uint8_t transport,
				   uint8_t direction,
				   uint8_t codec_config_len,
				   const uint8_t *codec_config,
				   uint32_t *min_delay,
				   uint32_t *max_delay)
{
	/* Check input parameters */
	zassert_equal(coding_format, READ_DELAY_CODING_FMT,
		      "Reading controller delay passed wrong coding_format");
	zassert_equal(company_id, READ_DELAY_COMPANY_ID,
		      "Reading controller delay passed wrong company_id");
	zassert_equal(vs_codec_id, READ_DELAY_VS_CODEC_ID,
		      "Reading controller delay passed wrong vs_codec_id");
	zassert_equal(transport, READ_DELAY_TRANSPORT,
		      "Reading controller delay passed wrong transport");
	zassert_equal(direction, READ_DELAY_DIRECTION,
		      "Reading controller delay passed wrong direction");
	zassert_equal(codec_config_len, sizeof(read_delay_codec_config),
		      "Reading controller delay passed wrong config length");
	zassert_equal(memcmp(codec_config, read_delay_codec_config,
			     codec_config_len), 0,
		      "Reading controller delay passed wrong config data");

	/* Fill in response data */
	*min_delay = MIN_CTLR_DELAY;
	*max_delay = MAX_CTLR_DELAY;

	return 0;
}

ZTEST(test_hci_codecs_info, test_read_ctlr_delay)
{
	struct bt_hci_cp_read_ctlr_delay *cp;
	struct bt_hci_rp_read_ctlr_delay *rp;
	struct net_buf *buf;
	struct net_buf *rsp;
	int err;

	/* Initialize bluetooth subsystem */
	bt_enable(NULL);

	/* Read Local Supported Controller Delay */
	buf = bt_hci_cmd_create(BT_HCI_OP_READ_CTLR_DELAY, sizeof(*cp));
	cp = net_buf_add(buf, sizeof(*cp) + sizeof(read_delay_codec_config));

	cp->codec_id.coding_format = READ_DELAY_CODING_FMT;
	cp->codec_id.company_id = sys_cpu_to_le16(READ_DELAY_COMPANY_ID);
	cp->codec_id.vs_codec_id = sys_cpu_to_le16(READ_DELAY_VS_CODEC_ID);
	cp->transport = READ_DELAY_TRANSPORT;
	cp->direction = READ_DELAY_DIRECTION;
	cp->codec_config_len = sizeof(read_delay_codec_config);
	memcpy(cp->codec_config, read_delay_codec_config,
	       sizeof(read_delay_codec_config));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_CTLR_DELAY, buf, &rsp);
	zassert_equal(err, 0,
		      "Reading local supported controller delay failed");

	/* Check returned data */
	rp = (struct bt_hci_rp_read_ctlr_delay *)rsp->data;
	zassert_equal(rp->status, 0,
		      "Reading controller delay status failed");
	zassert_equal(sys_get_le24(rp->min_ctlr_delay), MIN_CTLR_DELAY,
		      "Reading controller min delay failed");
	zassert_equal(sys_get_le24(rp->max_ctlr_delay), MAX_CTLR_DELAY,
		      "Reading controller max delay failed");
}
