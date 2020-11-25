/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_USBH_STRUCTS_H_
#define ZEPHYR_USBH_STRUCTS_H_

#include <zephyr.h>
#include <zephyr/types.h>
#include <usbh_cfg.h>

#define USBH_MAX_NBR_DEVS (USBH_CFG_MAX_NBR_DEVS + USBH_CFG_MAX_NBR_HC)
#define USBH_LEN_DESC_DEV 0x12u

enum usbh_device_speed {
	USBH_UNKNOWN_SPEED = 0, /* enumerating */
	USBH_LOW_SPEED,
	USBH_FULL_SPEED,        /* usb 1.1 */
	USBH_HIGH_SPEED,        /* usb 2.0 */
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *	Section 11.24.2.7.
 */

struct usbh_hub_port_status {
	uint16_t w_port_status;
	uint16_t w_port_change;
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 11.23.2.1.
 */
struct usbh_hub_desc {
	uint8_t b_desc_length;
	uint8_t b_desc_type;
	uint8_t b_nbr_ports;
	uint16_t w_hub_characteristics;
	uint8_t b_pwr_on_to_pwr_good;
	uint8_t b_hub_contr_current;
	uint8_t device_removable;
	uint32_t port_pwr_ctrl_mask[USBH_CFG_MAX_HUB_PORTS];
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 11.24.2.6.
 */

struct usbh_hub_status {
	uint16_t w_hub_status;
	uint16_t w_hub_change;
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.3, Table 9-2.
 */

struct usbh_setup_req {
	uint8_t bm_request_type;
	uint8_t b_request;
	uint16_t w_value;
	uint16_t w_index;
	uint16_t w_length;
};

struct usbh_desc_hdr {
	uint8_t b_length;
	uint8_t b_desc_type;
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.1, Table 9-8.
 */

struct usbh_dev_desc {
	uint8_t b_length;
	uint8_t b_desc_type;
	uint16_t bcd_usb;
	uint8_t b_device_class;
	uint8_t b_device_sub_class;
	uint8_t b_device_protocol;
	uint8_t b_max_packet_size_zero;
	uint16_t id_vendor;
	uint16_t id_product;
	uint16_t bcd_device;
	uint8_t i_manufacturer;
	uint8_t i_product;
	uint8_t i_serial_number;
	uint8_t b_nbr_configs;
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.2, Table 9-9.
 */

struct usbh_dev_qualifier_desc {
	uint8_t b_length;
	uint8_t b_desc_type;
	uint16_t bcd_usb;
	uint8_t b_device_class;
	uint8_t b_device_sub_class;
	uint8_t b_device_protocol;
	uint8_t b_max_packet_size_zero;
	uint8_t b_nbr_configs;
	uint8_t b_reserved;
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.3, Table 9-10.
 */

struct usbh_cfg_desc {
	uint8_t b_length;
	uint8_t b_desc_type;
	uint16_t w_total_length;
	uint8_t b_nbr_interfaces;
	uint8_t b_cfg_value;
	uint8_t i_cfg;
	uint8_t bm_attributes;
	uint8_t b_max_pwr;
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.4, Table 9-11.
 */

struct usbh_other_spd_cfg_desc {
	uint8_t b_length;
	uint8_t b_desc_type;
	uint16_t w_total_length;
	uint8_t b_nbr_interfaces;
	uint8_t b_cfg_value;
	uint8_t i_cfg;
	uint8_t bm_attributes;
	uint8_t b_max_pwr;
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.5, Table 9-12.
 */

struct usbh_if_desc {
	uint8_t b_length;
	uint8_t b_desc_type;
	uint8_t b_if_nbr;
	uint8_t b_alt_setting;
	uint8_t b_nbr_endpoints;
	uint8_t b_if_class;
	uint8_t b_if_sub_class;
	uint8_t b_if_protocol;
	uint8_t i_interface;
};

/*
 * Note(s) :
 * (1) See 'www.usb.org/developers/doc/InterfaceAssociationDescriptor_ecn.pdf',
 * Section 9.X.Y, Table 9-Z.
 */

struct usbh_if_association_desc {
	uint8_t b_length;
	uint8_t b_desc_type;
	uint8_t b_first_if;
	uint8_t b_if_cnt;
	uint8_t b_fnct_class;
	uint8_t b_fnct_sub_class;
	uint8_t b_fnct_protocol;
	uint8_t i_function;
};

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.6, Table 9-14.
 */

struct usbh_ep_desc {
	uint8_t b_length;
	uint8_t b_desc_type;
	uint8_t b_endpoint_address;
	uint8_t bm_attributes;
	uint16_t w_max_packet_size;
	uint8_t b_interval;
	uint8_t b_refresh;
	uint8_t b_sync_address;
};

/*
 * Note(s) : (1) See 'On-The-Go Specification Revision 1.3',
 *		Section 6.4, Table 6-1.
 */

struct usbh_otg_desc {
	uint8_t b_length;
	uint8_t b_desc_type;
	uint8_t bm_attributes;
};

struct usbh_isoc_desc {
	uint8_t *buf_ptr;
	uint32_t buf_len;
	uint32_t start_frm;
	uint32_t nbr_frm;
	uint16_t *frm_len;
	int *frm_err;
};

struct usbh_urb {
	/* state of urb.*/
	volatile uint8_t state;
	/* EP the urb belongs to.*/
	struct usbh_ep *ep_ptr;
	/* The status of urb completion. */
	volatile int err;
	/* Ptr to buf supplied by app. */
	void *userbuf_ptr;
	/* Buf len in bytes. */
	uint32_t uberbuf_len;
	/* DMA buf ptr used by DMA HW. */
	void *dma_buf_ptr;
	/* DMA buf len. */
	int32_t dma_buf_len;
	/* Actual len xfer'd by ctrlr. */
	uint32_t xfer_len;
	/* Isoc xfer desc. */
	struct usbh_isoc_desc *isoc_desc_ptr;
	/* Fnct ptr, called when I/O is completed. */
	void *fnct_ptr;
	/* Fnct context. */
	void *fnct_arg_ptr;
	/* HCD private data. */
	void *arg_ptr;
	/* token (SETUP, IN, or OUT). */
	uint8_t token;

	bool urb_done_signal;
	/* Ptr to next urb (if any). */
	struct usbh_urb *async_urb_nxt_ptr;
	/* Used for urb chained list in async task. */
	struct usbh_urb *nxt_ptr;
	/* sem to wait on I/O completion. */
	struct k_sem sem;
};

struct usbh_ep {
	/* USB dev spd. */
	enum usbh_device_speed dev_spd;
	/* USB dev addr. */
	uint8_t dev_addr;
	/* Ptr to USB dev struct. */
	struct usbh_dev *dev_ptr;
	/* EP desc. */
	struct usbh_ep_desc desc;
	/* EP interval. */
	uint16_t interval;
	/* Initial HC ref frame nbr. */
	uint32_t hc_ref_frame;
	/* HCD private data. */
	void *arg_ptr;
	/* urb used for data xfer on this endpoint. */
	struct usbh_urb urb;
	/* mutex for I/O access serialization on this EP. */
	struct k_mutex mutex;
	/* EP state. */
	bool is_open;
	/* nbr of urb(s) in progress. Used for async omm. */
	uint32_t xfer_nbr_in_prog;
	/* EP Data Toggle PID tracker. */
	uint8_t data_pid;
};

struct usbh_if {
	/* Ptr to USB dev. */
	struct usbh_dev *dev_ptr;
	/* Selected alternate setting ix. */
	uint8_t alt_ix_sel;
	/* Ptr to class dev created by class drv. */
	void *class_dev_ptr;
	/* Ptr to class drv registered for this IF. */
	struct usbh_class_drv_reg *class_drv_reg_ptr;
	/* Buf pointer containing IF data. */
	uint8_t *if_data_ptr;
	/* Buf len. */
	uint16_t if_data_len;
};

struct usbh_cfg {
	/* Buf containing cfg desc data. */
	uint8_t cfg_data[USBH_CFG_MAX_CFG_DATA_LEN];
	/* Cfg desc data len. */
	uint16_t cfg_data_len;
	/* Device IFs. */
	struct usbh_if if_list[USBH_CFG_MAX_NBR_IFS];
};

struct usbh_dev {
	/* Ptr to HC struct. */
	struct usbh_hc *hc_ptr;
	/* USB dev addr assigned by host. */
	uint8_t dev_addr;
	/* Dev spd (low, full or high). */
	enum usbh_device_speed dev_spd;
	/* Dflt ctrl EP. */
	struct usbh_ep dflt_ep;
	/* Dev dflt EP mutex. */
	struct k_mutex dflt_ep_mutex;
	/* Language ID used by the str desc. */
	uint16_t lang_id;
	/* Ptr to class dev created by class drv. */
	void *class_dev_ptr;
	/* Ptr to class drv managing this dev. */
	struct usbh_class_drv_reg *class_drv_reg_ptr;
	/* Dev desc. */
	uint8_t dev_desc[USBH_LEN_DESC_DEV];
	/* Dev cfg. */
	struct usbh_cfg cfg_list[USBH_CFG_MAX_NBR_CFGS];
	/* Selected dev cfg nbr. */
	uint8_t sel_cfg;
	/* Ptr to up stream hub dev struct. */
	struct usbh_dev *hub_dev_ptr;
	/* Port nbr to which this dev is connected. */
	uint32_t port_nbr;
	/* Indicate if this is a RH dev. */
	bool is_root_hub;
	/* Ptr to prev HS Hub. */
	struct usbh_hub_dev *hub_hs_ptr;
};

struct usbh_hub_dev {
	/* Intr EP to recv events from hub. */
	struct usbh_ep intr_ep;
	/* Hub desc. */
	struct usbh_hub_desc desc;
	/* Ptrs to USB devs connected to this hub. */
	struct usbh_dev *dev_ptr_list[USBH_CFG_MAX_HUB_PORTS];
	/* USB dev ptr of the hub IF. */
	struct usbh_dev *dev_ptr;
	/* HUB IF ptr. */
	struct usbh_if *if_ptr;
	/* Buf to recv hub events. */
	uint8_t hub_intr_buf[64];
	uint32_t err_cnt;
	uint8_t state;
	uint8_t ref_cnt;
	struct usbh_hub_dev *nxt_ptr;
	/* Re-connection counter */
	uint8_t conn_cnt;
};

struct usbh_hc_drv {
	/* HC nbr. */
	uint8_t nbr;
	/* Drv's data. */
	void *data_ptr;
	/* Ptr to RH dev struct. */
	struct usbh_dev *rh_dev_ptr;
	/* Ptr to HC drv API struct. */
	const struct usbh_hc_drv_api *api_ptr;
	/* Ptr to RH drv API struct. */
	const struct usbh_hc_rh_api *rh_api_ptr;
};

struct usbh_hc {
	/* Host Controller driver (HCD) info. */
	struct usbh_hc_drv hc_drv;
	/* Host structure. */
	struct usbh_host *host_ptr;
	/* Root Hub class device pointer. */
	struct usbh_hub_dev *rh_class_dev_ptr;
	/* mutex to sync access to HCD. */
	struct k_mutex hcd_mutex;
	/* Indicate if RH is virtual. */
	bool is_vir_rh;
};

struct usbh_host {
	/* state of USB host stack. */
	uint8_t state;
	/* List of USB dev connected. */
	struct usbh_dev dev_list[USBH_MAX_NBR_DEVS];
	/* Pool for mem mgmt of USB devs. */
	int8_t dev_cnt;
	int8_t isoc_cnt;
	struct usbh_isoc_desc isoc_desc[USBH_CFG_MAX_ISOC_DESC];
	/* Pool of extra urb when using async comm. */
	struct k_mem_pool async_urb_pool;
	/* Array of HC structs. */
	struct usbh_hc hc_tbl[USBH_CFG_MAX_NBR_HC];
	uint8_t hc_nbr_next;
	/* Async task handle. */
	struct k_thread h_async_task;
	/* Hub event task handle. */
	struct k_thread h_hub_task;
};

#endif /*  ZEPHYR_USBH_STRUCTS_H_ */
