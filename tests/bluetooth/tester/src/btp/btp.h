/* bttester.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>

#include "bttester.h"
#include "btp_core.h"
#include "btp_gap.h"
#include "btp_gatt.h"
#include "btp_l2cap.h"
#include "btp_mesh.h"
#include "btp_vcs.h"
#include "btp_aics.h"
#include "btp_vocs.h"
#include "btp_ias.h"
#include "btp_pacs.h"
#include "btp_ascs.h"
#include "btp_bap.h"
#include "btp_has.h"
#include "btp_csis.h"
#include "btp_micp.h"
#include "btp_mics.h"
#include "btp_ccp.h"
#include "btp_vcp.h"
#include "btp_cas.h"
#include "btp_mcp.h"
#include "btp_mcs.h"
#include "btp_hap.h"
#include "btp_csip.h"
#include "btp_cap.h"
#include "btp_tbs.h"
#include "btp_tmap.h"
#include "btp_ots.h"

#define BTP_MTU 1024
#define BTP_DATA_MAX_SIZE (BTP_MTU - sizeof(struct btp_hdr))

#define BTP_INDEX_NONE		0xff
#define BTP_INDEX		0x00

#define BTP_SERVICE_ID_CORE	0
#define BTP_SERVICE_ID_GAP	1
#define BTP_SERVICE_ID_GATT	2
#define BTP_SERVICE_ID_L2CAP	3
#define BTP_SERVICE_ID_MESH	4
#define BTP_SERVICE_ID_MESH_MDL	5
#define BTP_SERVICE_GATT_CLIENT	6
#define BTP_SERVICE_GATT_SERVER	7
#define BTP_SERVICE_ID_VCS	8
#define BTP_SERVICE_ID_IAS	9
#define BTP_SERVICE_ID_AICS	10
#define BTP_SERVICE_ID_VOCS	11
#define BTP_SERVICE_ID_PACS	12
#define BTP_SERVICE_ID_ASCS	13
#define BTP_SERVICE_ID_BAP	14
#define BTP_SERVICE_ID_HAS	15
#define BTP_SERVICE_ID_MICP	16
#define BTP_SERVICE_ID_CSIS	17
#define BTP_SERVICE_ID_MICS	18
#define BTP_SERVICE_ID_CCP	19
#define BTP_SERVICE_ID_VCP	20
#define BTP_SERVICE_ID_CAS	21
#define BTP_SERVICE_ID_MCP	22
#define BTP_SERVICE_ID_GMCS	23
#define BTP_SERVICE_ID_HAP	24
#define BTP_SERVICE_ID_CSIP	25
#define BTP_SERVICE_ID_CAP	26
#define BTP_SERVICE_ID_TBS	27
#define BTP_SERVICE_ID_TMAP	28
#define BTP_SERVICE_ID_OTS	29

#define BTP_SERVICE_ID_MAX	BTP_SERVICE_ID_OTS

#define BTP_STATUS_SUCCESS	0x00
#define BTP_STATUS_FAILED	0x01
#define BTP_STATUS_UNKNOWN_CMD	0x02
#define BTP_STATUS_NOT_READY	0x03
#define BTP_STATUS_NOT_SUPPORT	0x04

#define BTP_STATUS_VAL(err) (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS

/* TODO indicate delay response, should be removed when all commands are
 * converted to cmd+status+ev pattern
 */
#define BTP_STATUS_DELAY_REPLY	0xFF

struct btp_hdr {
	uint8_t  service;
	uint8_t  opcode;
	uint8_t  index;
	uint16_t len;
	uint8_t  data[];
} __packed;

#define BTP_STATUS		0x00
struct btp_status {
	uint8_t code;
} __packed;
