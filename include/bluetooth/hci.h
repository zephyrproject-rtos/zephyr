/* hci.h - Bluetooth Host Control Interface definitions */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BT_HCI_H
#define __BT_HCI_H

struct bt_hci_evt_hdr {
	uint8_t  evt;
	uint8_t  len;
} PACK_STRUCT;

#define bt_acl_handle(h)		((h) & 0x0fff)

struct bt_hci_acl_hdr {
	uint16_t handle;
	uint16_t len;
} PACK_STRUCT;

struct bt_hci_cmd_hdr {
	uint16_t opcode;
	uint8_t  param_len;
} PACK_STRUCT;

/* LMP features */
#define BT_LMP_NO_BREDR				0x20
#define BT_LMP_LE				0x40

/* LE features */
#define BT_HCI_LE_ENCRYPTION			0x01

/* OpCode Group Fields */
#define BT_OGF_BASEBAND				0x03
#define BT_OGF_INFO				0x04
#define BT_OGF_LE				0x08

/* Construct OpCode from OGF and OCF */
#define BT_OP(ogf, ocf)				((ocf) | ((ogf) << 10))

#define BT_HCI_OP_SET_EVENT_MASK		BT_OP(BT_OGF_BASEBAND, 0x0001)
struct bt_hci_cp_set_event_mask {
	uint8_t  events[8];
} PACK_STRUCT;

#define BT_HCI_OP_RESET				BT_OP(BT_OGF_BASEBAND, 0x0003)

#define BT_HCI_OP_LE_WRITE_LE_HOST_SUPP		BT_OP(BT_OGF_BASEBAND, 0x006d)
struct bt_hci_cp_write_le_host_supp {
	uint8_t  le;
	uint8_t  simul;
} PACK_STRUCT;

#define BT_HCI_OP_READ_LOCAL_VERSION_INFO	BT_OP(BT_OGF_INFO, 0x0001)
struct bt_hci_rp_read_local_version_info {
	uint8_t  status;
	uint8_t  hci_version;
	uint16_t hci_revision;
	uint8_t  lmp_version;
	uint16_t manufacturer;
	uint16_t lmp_subversion;
} PACK_STRUCT;

#define BT_HCI_OP_READ_LOCAL_FEATURES		BT_OP(BT_OGF_INFO, 0x0003)
struct bt_hci_rp_read_local_features {
	uint8_t  status;
	uint8_t  features[8];
} PACK_STRUCT;

#define BT_HCI_OP_READ_BUFFER_SIZE		BT_OP(BT_OGF_INFO, 0x0005)
struct bt_hci_rp_read_buffer_size {
	uint8_t  status;
	uint16_t acl_max_len;
	uint8_t  sco_max_len;
	uint16_t acl_max_num;
	uint16_t sco_max_num;
} PACK_STRUCT;

#define BT_HCI_OP_READ_BD_ADDR			BT_OP(BT_OGF_INFO, 0x0009)
struct bt_hci_rp_read_bd_addr {
	uint8_t  status;
	uint8_t  bdaddr[6];
} PACK_STRUCT;

#define BT_HCI_OP_LE_READ_BUFFER_SIZE		BT_OP(BT_OGF_LE, 0x0002)
struct bt_hci_rp_le_read_buffer_size {
	uint8_t  status;
	uint16_t le_max_len;
	uint8_t  le_max_num;
} PACK_STRUCT;

#define BT_HCI_OP_LE_READ_LOCAL_FEATURES	BT_OP(BT_OGF_LE, 0x0003)
struct bt_hci_rp_le_read_local_features {
	uint8_t  status;
	uint8_t  features[8];
} PACK_STRUCT;

/* Event definitions */

#define BT_HCI_EVT_CMD_COMPLETE			0x0e
struct hci_evt_cmd_complete {
	uint8_t  ncmd;
	uint16_t opcode;
} PACK_STRUCT;

#define BT_HCI_EVT_CMD_STATUS			0x0f
struct bt_hci_evt_cmd_status {
	uint8_t  status;
	uint8_t  ncmd;
	uint16_t opcode;
} PACK_STRUCT;

#endif /* __BT_HCI_H */
