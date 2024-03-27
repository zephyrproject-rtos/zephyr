/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_UAOL_H__
#define __INTEL_DAI_DRIVER_UAOL_H__

#include <stdint.h>
#include <zephyr/drivers/dai.h>

#define INTEL_UAOL_STREAM_TYPE_NORMAL		0
#define INTEL_UAOL_STREAM_TYPE_FEEDBACK		1

#define INTEL_UAOL_STREAM_DIRECTION_OUT		0
#define INTEL_UAOL_STREAM_DIRECTION_IN		1

#define INTEL_UAOL_GPDMA_BURST_LENGTH		4

#define INTEL_UAOL_FEEDBACK_SAMPLE_SIZE		3

/* service interval cadence of PCM stream in microseconds */
#define INTEL_UAOL_SERVICE_INTERVAL_US		1000

/* Misc. timeout values - all in microseconds */
#define INTEL_XHCI_MSG_TIMEOUT			10000
#define INTEL_UAOL_LINK_POWER_CHANGE_TIMEOUT	32000
#define INTEL_UAOL_STREAM_STATE_CHANGE_TIMEOUT	32000
#define INTEL_UAOL_FRAME_ADJUST_TIMEOUT		32000

/* maximum PCM payload size when split_ep is on */
#define INTEL_UAOL_PCM_CTL_MPS_SPLIT_EP		188

/* base clock frequencies in Hz */
#define INTEL_XHCI_BUS_CLOCK_FREQ		480000000
#define INTEL_ADSP_XTAL_FREQ			19200000

/* UAOL PCM stream microframe frequency in Hz */
#define INTEL_UAOL_UFRAME_FREQ			8000

/* UAOL PCM stream microframe period in microseconds */
#define INTEL_UAOL_UFRAME_PERIOD_US		(1000000 / INTEL_UAOL_UFRAME_FREQ)

/* Number of UAOL PCM stream microframes in 1 millisecond */
#define INTEL_UAOL_UFRAMES_PER_MILLISEC		(1000 / INTEL_UAOL_UFRAME_PERIOD_US)

/* reference clock frequency for xHCI timestamps in Hz */
#define INTEL_XHCI_TIMESTAMP_CLK		(INTEL_XHCI_BUS_CLOCK_FREQ / 8)

/* reference clock frequency for UAOL timestamps in Hz */
#define INTEL_UAOL_TIMESTAMP_CLK		INTEL_ADSP_XTAL_FREQ

/* UAOL PCM stream microframe duration expressed in UAOL timestamp clocks */
#define INTEL_UAOL_CLK_PER_UFRAME		(INTEL_UAOL_TIMESTAMP_CLK / INTEL_UAOL_UFRAME_FREQ)

/* gets SIO pin number for a given stream */
#define INTEL_UAOL_STREAM_TO_SIO_PIN(stream)	(stream + 1)

struct intel_uaol_link_caps {
	uint32_t input_streams_supported         : 4;
	uint32_t output_streams_supported        : 4;
	uint32_t bidirectional_streams_supported : 5;
	uint32_t rsvd                            : 19;
	uint32_t max_tx_fifo_size;
	uint32_t max_rx_fifo_size;
};

struct intel_uaol_caps {
	uint32_t link_count;
	struct intel_uaol_link_caps link_caps[];
};

enum intel_uaol_xhci_msg_type {
	XHCI_MSG_TYPE_CMD  = 1,
	XHCI_MSG_TYPE_DATA = 2,
	XHCI_MSG_TYPE_RESP = 3,
};

enum intel_uaol_xhci_recipient {
	XHCI_CMD_RECIPIENT_DEVICE    = 0,
	XHCI_CMD_RECIPIENT_INTERFACE = 1,
	XHCI_CMD_RECIPIENT_ENDPOINT  = 2,
	XHCI_CMD_RECIPIENT_OTHER     = 3,
};

enum intel_uaol_xhci_cmd_type {
	XHCI_CMD_TYPE_STANDARD = 0,
	XHCI_CMD_TYPE_CLASS    = 1,
	XHCI_CMD_TYPE_VENDOR   = 2,
	XHCI_CMD_TYPE_RESERVED = 3,
};

enum intel_uaol_xhci_cmd_dir {
	XHCI_CMD_DIR_HOST_TO_DEVICE = 0,
	XHCI_CMD_DIR_DEVICE_TO_HOST = 1,
};

enum intel_uaol_xhci_cmd_request {
	XHCI_CMD_REQUEST_GET_EP_TABLE_ENTRY = 0x80,
	XHCI_CMD_REQUEST_SET_EP_TABLE_ENTRY = 0x81,
	XHCI_CMD_REQUEST_GET_HH_TIMESTAMP   = 0x82,
};

struct intel_uaol_xhci_cmd {
	uint8_t type;
	uint16_t length;
	struct {
		uint8_t recipient : 5;
		uint8_t type      : 2;
		uint8_t direction : 1;
		uint8_t bRequest;
		uint16_t wValue;
		uint16_t wIndex;
		uint16_t wLength;
	} __packed __aligned(1) payload;
} __packed __aligned(4);

struct intel_uaol_xhci_resp {
	uint8_t type;
	uint16_t length;
	uint8_t completion_code;
} __packed __aligned(4);

struct intel_uaol_xhci_ep_table_entry_data {
	uint8_t type;
	uint16_t length;
	struct {
		uint16_t usb_ep_address     : 5;
		uint16_t device_slot_number : 8;
		uint16_t split_ep           : 1;
		uint16_t rsvd0              : 1;
		uint16_t valid              : 1;
		uint8_t sio_pin_num;
		uint8_t rsvd;
	} __packed __aligned(1) payload;
} __packed __aligned(4);

struct intel_uaol_xhci_hh_timestamp_resp {
	uint8_t type;
	uint16_t length;
	struct {
		uint16_t cmfb  : 13;
		uint16_t rsvd0 : 3;
		uint16_t cmfi  : 14;
		uint16_t rsvd1 : 2;
		uint64_t global_time;
	} __packed __aligned(1) payload;
} __packed __aligned(4);

struct intel_uaol_aux_config_blob {
	uint32_t length;
	uint32_t data[];
};

struct intel_uaol_xhci_controller_bdf {
	uint32_t bus      : 8;
	uint32_t device   : 5;
	uint32_t function : 3;
	uint32_t rsvd0    : 16;
};

struct intel_uaol_config {
	uint32_t link_idx        : 8;
	uint32_t stream_idx      : 8;
	uint32_t feedback_idx    : 8;
	uint32_t feedback_period : 8;
};

struct intel_uaol_fifo_sao {
	uint32_t tx0_fifo_sao : 12;
	uint32_t rsvd0        : 4;
	uint32_t tx1_fifo_sao : 12;
	uint32_t rsvd1        : 4;
	uint32_t rx0_fifo_sao : 12;
	uint32_t rsvd2        : 4;
	uint32_t rx1_fifo_sao : 12;
	uint32_t rsvd3        : 4;
};

struct intel_uaol_usb_ep_info {
	uint32_t direction          : 1;
	uint32_t usb_ep_number      : 4;
	uint32_t device_slot_number : 8;
	uint32_t split_ep           : 1;
	uint32_t device_speed       : 1;
	uint32_t rsvd               : 1;
	uint32_t usb_mps            : 16;
};

struct intel_uaol_usb_art_divider {
	uint16_t multiplier;
	uint16_t divider;
};

struct intel_uaol_ep_table_entry {
	uint32_t link_idx;
	uint32_t stream_idx;
	struct intel_uaol_usb_ep_info ep_info;
};

enum intel_uaol_aux_tlv_id {
	INTEL_UAOL_AUX_XHCI_CONTROLLER_BDF  = 0,
	INTEL_UAOL_AUX_UAOL_CONFIG          = 1,
	INTEL_UAOL_AUX_FIFO_SAO             = 2,
	INTEL_UAOL_AUX_USB_EP_INFO          = 3,
	INTEL_UAOL_AUX_USB_EP_FEEDBACK_INFO = 4,
	INTEL_UAOL_AUX_USB_ART_DIVIDER      = 5,
};

struct intel_uaol_aux_tlv {
	uint32_t type;
	uint32_t length;
	union {
		struct intel_uaol_xhci_controller_bdf xhci_controller_bdf;
		struct intel_uaol_config uaol_config;
		struct intel_uaol_fifo_sao uaol_fifo_sao;
		struct intel_uaol_usb_ep_info usb_ep_info;
		struct intel_uaol_usb_art_divider art_divider;
	} value;
};

enum intel_uaol_ioctl_tlv_id {
	INTEL_UAOL_IOCTL_SET_EP_TABLE_ENTRY   = 0,
	INTEL_UAOL_IOCTL_RESET_EP_TABLE_ENTRY = 1,
	INTEL_UAOL_IOCTL_SET_EP_INFO          = 3,
	INTEL_UAOL_IOCTL_SET_EP_FEEDBACK_INFO = 4,
	INTEL_UAOL_IOCTL_SET_FEEDBACK_PERIOD  = 5,
	INTEL_UAOL_IOCTL_GET_HW_CAPS          = 10,
};

struct intel_uaol_ioctl_tlv {
	uint32_t type;
	uint32_t length;
	union {
		struct intel_uaol_ep_table_entry ep_table_entry;
		struct intel_uaol_usb_ep_info usb_ep_info;
		uint32_t feedback_period;
	} value;
};

struct intel_uaol_ioctl_resp_tlv {
	uint32_t type;
	uint32_t length;
	union {
		struct intel_uaol_caps uaol_caps;
	} value;
};

struct intel_uaol_link {
	mem_addr_t ip_base;
	mem_addr_t shim_base;
#if CONFIG_SOC_INTEL_ACE20_LNL
	mem_addr_t hdaml_base;
#endif
	uint32_t link_idx;
	int acquire_count;
	bool is_initialized;
	struct intel_uaol_xhci_controller_bdf bdf;
	struct intel_uaol_fifo_sao fifo_sao;
	struct intel_uaol_usb_art_divider art_divider;
};

struct dai_intel_uaol {
	struct dai_config config;
	struct dai_properties props;
	enum dai_state state;
	bool in_use;
	struct intel_uaol_link *link;
	uint32_t link_idx;
	uint32_t stream_idx;
	uint32_t stream_type;
	struct intel_uaol_usb_ep_info ep_info;
	uint32_t service_interval;
#if CONFIG_SOC_INTEL_ACE15_MTPM
	uint32_t alh_stream;
	uint32_t dma_handshake;
#endif
};

#endif /* __INTEL_DAI_DRIVER_UAOL_H__ */
