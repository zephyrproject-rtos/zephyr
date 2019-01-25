/* hci_vs.h - Bluetooth Host Control Interface Vendor Specific definitions */

/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HCI_VS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HCI_VS_H_

#include <bluetooth/hci.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_HCI_VS_HW_PLAT_INTEL                 0x0001
#define BT_HCI_VS_HW_PLAT_NORDIC                0x0002
#define BT_HCI_VS_HW_PLAT_NXP                   0x0003

#define BT_HCI_VS_HW_VAR_NORDIC_NRF51X          0x0001
#define BT_HCI_VS_HW_VAR_NORDIC_NRF52X          0x0002

#define BT_HCI_VS_FW_VAR_STANDARD_CTLR          0x0001
#define BT_HCI_VS_FW_VAR_VS_CTLR                0x0002
#define BT_HCI_VS_FW_VAR_FW_LOADER              0x0003
#define BT_HCI_VS_FW_VAR_RESCUE_IMG             0x0004
#define BT_HCI_OP_VS_READ_VERSION_INFO		BT_OP(BT_OGF_VS, 0x0001)
struct bt_hci_rp_vs_read_version_info {
	u8_t  status;
	u16_t hw_platform;
	u16_t hw_variant;
	u8_t  fw_variant;
	u8_t  fw_version;
	u16_t fw_revision;
	u32_t fw_build;
} __packed;

#define BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS	BT_OP(BT_OGF_VS, 0x0002)
struct bt_hci_rp_vs_read_supported_commands {
	u8_t  status;
	u8_t  commands[64];
} __packed;

#define BT_HCI_OP_VS_READ_SUPPORTED_FEATURES	BT_OP(BT_OGF_VS, 0x0003)
struct bt_hci_rp_vs_read_supported_features {
	u8_t  status;
	u8_t  features[8];
} __packed;

#define BT_HCI_OP_VS_SET_EVENT_MASK             BT_OP(BT_OGF_VS, 0x0004)
struct bt_hci_cp_vs_set_event_mask {
	u8_t  event_mask[8];
} __packed;

#define BT_HCI_VS_RESET_SOFT                    0x00
#define BT_HCI_VS_RESET_HARD                    0x01
#define BT_HCI_OP_VS_RESET                      BT_OP(BT_OGF_VS, 0x0005)
struct bt_hci_cp_vs_reset {
	u8_t  type;
} __packed;

#define BT_HCI_OP_VS_WRITE_BD_ADDR              BT_OP(BT_OGF_VS, 0x0006)
struct bt_hci_cp_vs_write_bd_addr {
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_VS_TRACE_DISABLED                0x00
#define BT_HCI_VS_TRACE_ENABLED                 0x01

#define BT_HCI_VS_TRACE_HCI_EVTS                0x00
#define BT_HCI_VS_TRACE_VDC                     0x01
#define BT_HCI_OP_VS_SET_TRACE_ENABLE           BT_OP(BT_OGF_VS, 0x0007)
struct bt_hci_cp_vs_set_trace_enable {
	u8_t  enable;
	u8_t  type;
} __packed;

#define BT_HCI_OP_VS_READ_BUILD_INFO            BT_OP(BT_OGF_VS, 0x0008)
struct bt_hci_rp_vs_read_build_info {
	u8_t  status;
	u8_t  info[0];
} __packed;

struct bt_hci_vs_static_addr {
	bt_addr_t bdaddr;
	u8_t      ir[16];
} __packed;

#define BT_HCI_OP_VS_READ_STATIC_ADDRS          BT_OP(BT_OGF_VS, 0x0009)
struct bt_hci_rp_vs_read_static_addrs {
	u8_t   status;
	u8_t   num_addrs;
	struct bt_hci_vs_static_addr a[0];
} __packed;

#define BT_HCI_OP_VS_READ_KEY_HIERARCHY_ROOTS   BT_OP(BT_OGF_VS, 0x000a)
struct bt_hci_rp_vs_read_key_hierarchy_roots {
	u8_t  status;
	u8_t  ir[16];
	u8_t  er[16];
} __packed;

#define BT_HCI_OP_VS_READ_CHIP_TEMP             BT_OP(BT_OGF_VS, 0x000b)
struct bt_hci_rp_vs_read_chip_temp {
	u8_t  status;
	s8_t  temps;
} __packed;

struct bt_hci_vs_cmd {
	u16_t vendor_id;
	u16_t opcode_base;
} __packed;

#define BT_HCI_VS_VID_ANDROID                   0x0001
#define BT_HCI_VS_VID_MICROSOFT                 0x0002
#define BT_HCI_OP_VS_READ_HOST_STACK_CMDS       BT_OP(BT_OGF_VS, 0x000c)
struct bt_hci_rp_vs_read_host_stack_cmds {
	u8_t   status;
	u8_t   num_cmds;
	struct bt_hci_vs_cmd c[0];
} __packed;

#define BT_HCI_VS_SCAN_REQ_REPORTS_DISABLED     0x00
#define BT_HCI_VS_SCAN_REQ_REPORTS_ENABLED      0x01
#define BT_HCI_OP_VS_SET_SCAN_REQ_REPORTS       BT_OP(BT_OGF_VS, 0x000d)
struct bt_hci_cp_vs_set_scan_req_reports {
	u8_t  enable;
} __packed;

/* Events */

struct bt_hci_evt_vs {
	u8_t  subevent;
} __packed;

#define BT_HCI_EVT_VS_FATAL_ERROR              0x02
struct bt_hci_evt_vs_fatal_error {
	u64_t pc;
	u8_t  err_info[0];
} __packed;

#define BT_HCI_VS_TRACE_LMP_TX                 0x01
#define BT_HCI_VS_TRACE_LMP_RX                 0x02
#define BT_HCI_VS_TRACE_LLCP_TX                0x03
#define BT_HCI_VS_TRACE_LLCP_RX                0x04
#define BT_HCI_VS_TRACE_LE_CONN_IND            0x05
#define BT_HCI_EVT_VS_TRACE_INFO               0x03
struct bt_hci_evt_vs_trace_info {
	u8_t  type;
	u8_t  data[0];
} __packed;

#define BT_HCI_EVT_VS_SCAN_REQ_RX              0x04
struct bt_hci_evt_vs_scan_req_rx {
	bt_addr_le_t addr;
	s8_t         rssi;
} __packed;

/* Event mask bits */

#define BT_EVT_MASK_VS_FATAL_ERROR             BT_EVT_BIT(1)
#define BT_EVT_MASK_VS_TRACE_INFO              BT_EVT_BIT(2)
#define BT_EVT_MASK_VS_SCAN_REQ_RX             BT_EVT_BIT(3)

/* Mesh HCI commands */
#define BT_HCI_MESH_REVISION                   0x01

#define BT_HCI_OP_VS_MESH                      BT_OP(BT_OGF_VS, 0x0042)
#define BT_HCI_MESH_EVT_PREFIX                 0xF0

struct bt_hci_cp_mesh {
	u8_t         opcode;
} __packed;

#define BT_HCI_OC_MESH_GET_OPTS                0x00
struct bt_hci_rp_mesh_get_opts {
	u8_t      status;
	u8_t      opcode;
	u8_t      revision;
	u8_t      ch_map;
	s8_t      min_tx_power;
	s8_t      max_tx_power;
	u8_t      max_scan_filter;
	u8_t      max_filter_pattern;
	u8_t      max_adv_slot;
	u8_t      max_tx_window;
	u8_t      evt_prefix_len;
	u8_t      evt_prefix;
} __packed;

#define BT_HCI_MESH_PATTERN_LEN_MAX            0x0f

#define BT_HCI_OC_MESH_SET_SCAN_FILTER         0x01
struct bt_hci_mesh_pattern {
	u8_t pattern_len;
	u8_t pattern[0];
} __packed;

struct bt_hci_cp_mesh_set_scan_filter {
	u8_t      scan_filter;
	u8_t      filter_dup;
	u8_t      num_patterns;
	struct    bt_hci_mesh_pattern patterns[0];
} __packed;
struct bt_hci_rp_mesh_set_scan_filter {
	u8_t      status;
	u8_t      opcode;
	u8_t      scan_filter;
} __packed;

#define BT_HCI_OC_MESH_ADVERTISE               0x02
struct bt_hci_cp_mesh_advertise {
	u8_t      adv_slot;
	u8_t      own_addr_type;
	bt_addr_t random_addr;
	u8_t      ch_map;
	s8_t      tx_power;
	u8_t      min_tx_delay;
	u8_t      max_tx_delay;
	u8_t      retx_count;
	u8_t      retx_interval;
	u8_t      scan_delay;
	u16_t     scan_duration;
	u8_t      scan_filter;
	u8_t      data_len;
	u8_t      data[31];
} __packed;
struct bt_hci_rp_mesh_advertise {
	u8_t      status;
	u8_t      opcode;
	u8_t      adv_slot;
} __packed;

#define BT_HCI_OC_MESH_ADVERTISE_TIMED         0x03
struct bt_hci_cp_mesh_advertise_timed {
	u8_t      adv_slot;
	u8_t      own_addr_type;
	bt_addr_t random_addr;
	u8_t      ch_map;
	s8_t      tx_power;
	u8_t      retx_count;
	u8_t      retx_interval;
	u32_t     instant;
	u16_t     tx_delay;
	u16_t     tx_window;
	u8_t      data_len;
	u8_t      data[31];
} __packed;
struct bt_hci_rp_mesh_advertise_timed {
	u8_t      status;
	u8_t      opcode;
	u8_t      adv_slot;
} __packed;

#define BT_HCI_OC_MESH_ADVERTISE_CANCEL        0x04
struct bt_hci_cp_mesh_advertise_cancel {
	u8_t      adv_slot;
} __packed;
struct bt_hci_rp_mesh_advertise_cancel {
	u8_t      status;
	u8_t      opcode;
	u8_t      adv_slot;
} __packed;

#define BT_HCI_OC_MESH_SET_SCANNING            0x05
struct bt_hci_cp_mesh_set_scanning {
	u8_t      enable;
	u8_t      ch_map;
	u8_t      scan_filter;
} __packed;
struct bt_hci_rp_mesh_set_scanning {
	u8_t      status;
	u8_t      opcode;
} __packed;

/* Events */
struct bt_hci_evt_mesh {
	u8_t  prefix;
	u8_t  subevent;
} __packed;

#define BT_HCI_EVT_MESH_ADV_COMPLETE           0x00
struct bt_hci_evt_mesh_adv_complete {
	u8_t         adv_slot;
} __packed;

#define BT_HCI_EVT_MESH_SCANNING_REPORT        0x01
struct bt_hci_evt_mesh_scan_report {
	bt_addr_le_t addr;
	u8_t         chan;
	s8_t         rssi;
	u32_t        instant;
	u8_t         data_len;
	u8_t         data[0];
} __packed;
struct bt_hci_evt_mesh_scanning_report {
	u8_t num_reports;
	struct bt_hci_evt_mesh_scan_report reports[0];
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HCI_VS_H_ */
