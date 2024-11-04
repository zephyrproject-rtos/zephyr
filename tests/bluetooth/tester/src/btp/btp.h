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

#define BTP_SERVICE_ID_CORE     0x00
#define BTP_SERVICE_ID_GAP      0x01
#define BTP_SERVICE_ID_GATT     0x02
#define BTP_SERVICE_ID_L2CAP    0x03
#define BTP_SERVICE_ID_MESH     0x04
#define BTP_SERVICE_ID_MESH_MDL 0x05
#define BTP_SERVICE_GATT_CLIENT 0x06
#define BTP_SERVICE_GATT_SERVER 0x07
#define BTP_SERVICE_ID_VCS      0x08
#define BTP_SERVICE_ID_IAS      0x09
#define BTP_SERVICE_ID_AICS     0x0a
#define BTP_SERVICE_ID_VOCS     0x0b
#define BTP_SERVICE_ID_PACS     0x0c
#define BTP_SERVICE_ID_ASCS     0x0d
#define BTP_SERVICE_ID_BAP      0x0e
#define BTP_SERVICE_ID_HAS      0x0f
#define BTP_SERVICE_ID_MICP     0x10
#define BTP_SERVICE_ID_CSIS     0x11
#define BTP_SERVICE_ID_MICS     0x12
#define BTP_SERVICE_ID_CCP      0x13
#define BTP_SERVICE_ID_VCP      0x14
#define BTP_SERVICE_ID_CAS      0x15
#define BTP_SERVICE_ID_MCP      0x16
#define BTP_SERVICE_ID_GMCS     0x17
#define BTP_SERVICE_ID_HAP      0x18
#define BTP_SERVICE_ID_CSIP     0x19
#define BTP_SERVICE_ID_CAP      0x1a
#define BTP_SERVICE_ID_TBS      0x1b
#define BTP_SERVICE_ID_TMAP     0x1c
#define BTP_SERVICE_ID_OTS      0x1d

#define BTP_SERVICE_ID_MAX	BTP_SERVICE_ID_OTS

#define BTP_STATUS_SUCCESS	0x00
#define BTP_STATUS_FAILED	0x01
#define BTP_STATUS_UNKNOWN_CMD	0x02
#define BTP_STATUS_NOT_READY	0x03

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
