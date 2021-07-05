/*
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper macros to make it easier to work with endpoint numbers */
#define EP_ADDR2IDX(ep)         ((ep) & ~USB_EP_DIR_MASK)
#define EP_ADDR2DIR(ep)         ((ep) & USB_EP_DIR_MASK)
#define EP_UDINT_MASK()         (0x000FF000)

#define NUM_OF_EP_MAX           DT_INST_PROP(0, num_bidir_endpoints)
#define USBC_RAM_ADDR           DT_REG_ADDR(DT_NODELABEL(sram1))
#define USBC_RAM_SIZE           DT_REG_SIZE(DT_NODELABEL(sram1))
#define USBC_is_frozen_clk()    (USBC->USBCON & USBC_USBCON_FRZCLK)
#define USBC_freeze_clk()       (USBC->USBCON |= USBC_USBCON_FRZCLK)
#define USBC_unfreeze_clk()     do {					     \
					USBC->USBCON &= ~USBC_USBCON_FRZCLK; \
									     \
					while (USBC->USBCON &		     \
					       USBC_USBCON_FRZCLK) {	     \
					};				     \
				} while (0)
#define USBC_is_freeze_clk()    (USBC->USBCON & USBC_USBCON_FRZCLK)

/**
 * @brief USB Driver Control Endpoint Finite State Machine states
 *
 * FSM states to keep tracking of control endpoint hiden states.
 */
enum usb_dc_epctrl_state {
	/* Wait a SETUP packet */
	USB_EPCTRL_SETUP,
	/* Wait a OUT data packet */
	USB_EPCTRL_DATA_OUT,
	/* Wait a IN data packet */
	USB_EPCTRL_DATA_IN,
	/* Wait a IN ZLP packet */
	USB_EPCTRL_HANDSHAKE_WAIT_IN_ZLP,
	/* Wait a OUT ZLP packet */
	USB_EPCTRL_HANDSHAKE_WAIT_OUT_ZLP,
	/* STALL enabled on IN & OUT packet */
	USB_EPCTRL_STALL_REQ,
};

struct sam_usbc_udesc_sizes_t {
	uint32_t byte_count:15;
	uint32_t reserved:1;
	uint32_t multi_packet_size:15;
	uint32_t auto_zlp:1;
};

struct sam_usbc_udesc_bk_ctrl_stat_t {
	uint32_t stallrq:1;
	uint32_t reserved1:15;
	uint32_t crcerri:1;
	uint32_t overfi:1;
	uint32_t underfi:1;
	uint32_t reserved2:13;
};

struct sam_usbc_udesc_ep_ctrl_stat_t {
	uint32_t pipe_dev_addr:7;
	uint32_t reserved1:1;
	uint32_t pipe_num:4;
	uint32_t pipe_error_cnt_max:4;
	uint32_t pipe_error_status:8;
	uint32_t reserved2:8;
};

struct sam_usbc_desc_table_t {
	uint8_t *ep_pipe_addr;
	union {
		uint32_t sizes;
		struct sam_usbc_udesc_sizes_t SIZES;
	};
	union {
		uint32_t bk_ctrl_stat;
		struct sam_usbc_udesc_bk_ctrl_stat_t BK_CTRL_STAT;
	};
	union {
		uint32_t ep_ctrl_stat;
		struct sam_usbc_udesc_ep_ctrl_stat_t EP_CTRL_STAT;
	};
};

struct usb_device_ep_data {
	usb_dc_ep_callback cb_in;
	usb_dc_ep_callback cb_out;
	uint16_t mps;
	bool mps_x2;
	bool is_configured;
	uint32_t out_at;
};

struct usb_device_data {
	usb_dc_status_callback status_cb;
	struct usb_device_ep_data ep_data[NUM_OF_EP_MAX];
};
