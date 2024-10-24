/*
 * Copyright (c) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INTEL_DAI_DRIVER_UAOL_PARAMS_IPC4_H_
#define INTEL_DAI_DRIVER_UAOL_PARAMS_IPC4_H_

#include <stdint.h>

enum ipc4_uaol_aux_tlv_type {
	IPC4_UAOL_AUX_TLV_XHCI_CONTROLLER_BDF     = 0,
	IPC4_UAOL_AUX_TLV_UAOL_CONFIG             = 1,
	IPC4_UAOL_AUX_TLV_FIFO_SAO                = 2,
	IPC4_UAOL_AUX_TLV_USB_EP_INFO             = 3,
	IPC4_UAOL_AUX_TLV_USB_EP_FEEDBACK_INFO    = 4,
	IPC4_UAOL_AUX_TLV_USB_ART_DIVIDER         = 5,
};

enum ipc4_uaol_ioctl_tlv_type {
	IPC4_UAOL_IOCTL_TLV_SET_EP_TABLE          = 0,
	IPC4_UAOL_IOCTL_TLV_RESET_EP_TABLE        = 1,
	IPC4_UAOL_IOCTL_TLV_SET_EP_INFO           = 3,
	IPC4_UAOL_IOCTL_TLV_SET_EP_FEEDBACK_INFO  = 4,
	IPC4_UAOL_IOCTL_TLV_SET_FEEDBACK_PERIOD   = 5,
};

/* Top level container for AUX config. The config_data[] field
 * (of size config_length dwords) shall contain a TLV series of AUX type.
 */
struct ipc4_copier_gateway_cfg {
	uint32_t node_id;
	uint32_t dma_buffer_size;
	uint32_t config_length;
	uint32_t config_data[];
};

struct ipc4_uaol_tlv {
	uint32_t type;
	uint32_t length;
	uint32_t value[];
};

struct ipc4_uaol_xhci_controller_bdf {
	uint32_t bus      : 8;
	uint32_t device   : 5;
	uint32_t function : 3;
	uint32_t rsvd0    : 16;
};

struct ipc4_uaol_config {
	uint32_t link_idx        : 8;
	uint32_t stream_idx      : 8;
	uint32_t feedback_idx    : 8;
	uint32_t feedback_period : 8;
};

struct ipc4_uaol_fifo_sao {
	uint32_t tx0_fifo_sao : 12;
	uint32_t rsvd0        : 4;
	uint32_t tx1_fifo_sao : 12;
	uint32_t rsvd1        : 4;
	uint32_t rx0_fifo_sao : 12;
	uint32_t rsvd2        : 4;
	uint32_t rx1_fifo_sao : 12;
	uint32_t rsvd3        : 4;
};

struct ipc4_uaol_usb_ep_info {
	uint32_t direction          : 1;
	uint32_t usb_ep_number      : 4;
	uint32_t device_slot_number : 8;
	uint32_t split_ep           : 1;
	uint32_t device_speed       : 1;
	uint32_t rsvd               : 1;
	uint32_t usb_mps            : 16;
};

struct ipc4_uaol_usb_art_divider {
	uint16_t multiplier;
	uint16_t divider;
};

struct ipc4_uaol_set_ep_table {
	uint32_t link_idx;
	uint32_t stream_idx;
	struct ipc4_uaol_usb_ep_info entry;
};

#endif /* INTEL_DAI_DRIVER_UAOL_PARAMS_IPC4_H_ */
