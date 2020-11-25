/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on usbh_core.h from uC/Modbus Stack.
 *
 *                                uC/Modbus
 *                         The Embedded USB Host Stack
 *
 *      Copyright 2003-2020 Silicon Laboratories Inc. www.silabs.com
 *
 *                   SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef ZEPHYR_USBH_CORE_
#define ZEPHYR_USBH_CORE_
#include <usbh_cfg.h>
#include <zephyr.h>
#include <drivers/usbh/usbh_ll.h>
#include <usbh_structs.h>

#define USBH_LEN_DESC_HDR 0x02u
/* #define  USBH_LEN_DESC_DEV                              0x12u */
#define USBH_LEN_DESC_DEV_QUAL 0x0Au
#define USBH_LEN_DESC_CFG 0x09u
#define USBH_LEN_DESC_OTHERSPD_CFG 0x09u
#define USBH_LEN_DESC_IF_ASSOCIATION 0x08u
#define USBH_LEN_DESC_IF 0x09u
#define USBH_LEN_DESC_EP 0x07u
#define USBH_LEN_DESC_OTG 0x03u
#define USBH_LEN_SETUP_PKT 0x08u

#define USBH_HUB_TIMEOUT 5000u
#define USBH_HUB_DLY_DEV_RESET 100u
#define USBH_HUB_MAX_DESC_LEN 72u

#define USBH_HUB_LEN_HUB_DESC 0x48u
#define USBH_HUB_LEN_HUB_STATUS 0x04u
#define USBH_HUB_LEN_HUB_PORT_STATUS 0x04u

#define USBH_HC_NBR_NONE 0xFFu

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.3, Table 9-2.
 *
 *		(2) The 'bm_request_type' field of a setup request is
 *			a bit-mapped datum with three subfields :
 *
 *	(a) Bit  7  : Data transfer direction.
 *	(b) Bits 6-5: Type.
 *	(c) Bits 4-0: Recipient.
 */
#define USBH_REQ_DIR_MASK 0x80u
#define USBH_REQ_DIR_HOST_TO_DEV 0x00u
#define USBH_REQ_DIR_DEV_TO_HOST 0x80u

#define USBH_REQ_TYPE_STD 0x00u
#define USBH_REQ_TYPE_CLASS 0x20u
#define USBH_REQ_TYPE_VENDOR 0x40u
#define USBH_REQ_TYPE_RSVD 0x60u

#define USBH_REQ_RECIPIENT_DEV 0x00u
#define USBH_REQ_RECIPIENT_IF 0x01u
#define USBH_REQ_RECIPIENT_EP 0x02u
#define USBH_REQ_RECIPIENT_OTHER 0x03u

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		 Section 9.4, Table 9-4.
 *
 *		(2) The 'b_request' field of a standard setup
 *		request may contain one of these values.
 */
#define USBH_REQ_GET_STATUS 0x00u
#define USBH_REQ_CLR_FEATURE 0x01u
#define USBH_REQ_SET_FEATURE 0x03u
#define USBH_REQ_SET_ADDR 0x05u
#define USBH_REQ_GET_DESC 0x06u
#define USBH_REQ_SET_DESC 0x07u
#define USBH_REQ_GET_CFG 0x08u
#define USBH_REQ_SET_CFG 0x09u
#define USBH_REQ_GET_IF 0x0Au
#define USBH_REQ_SET_IF 0x0Bu
#define USBH_REQ_SYNCH_FRAME 0x0Cu

/*
 * Note(s) :	(1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.4, Table 9-5, and Section 9.4.3.
 *
 *		(2) For a 'get descriptor' setup request, the low byte
 *		of the 'w_value' field may contain one of these values.
 */
#define USBH_DESC_TYPE_DEV 1u
#define USBH_DESC_TYPE_CFG 2u
#define USBH_DESC_TYPE_STR 3u
#define USBH_DESC_TYPE_IF 4u
#define USBH_DESC_TYPE_EP 5u
#define USBH_DESC_TYPE_DEV_QUALIFIER 6u
#define USBH_DESC_TYPE_OTHER_SPD_CONFIG 7u
#define USBH_DESC_TYPE_IF_PWR 8u
#define USBH_DESC_TYPE_OTG 9u
#define USBH_DESC_TYPE_IAD 11u

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.4, Table 9-6, and
 *		Section 9.4.1.
 *
 *		(2) For a 'clear feature' setup request, the 'w_value'
 *		field may contain one of these values.
 */
#define USBH_FEATURE_SEL_EP_HALT 0u
#define USBH_FEATURE_SEL_DEV_REMOTE_WAKEUP 1u
#define USBH_FEATURE_SEL_TEST_MODE 2u
#define USBH_FEATURE_SEL_B_HNP_EN 3u

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.6, Table 9-13
 *
 *		(2) In an endpoint descriptor, the 'bm_attributes' value
 *		is one of these values.
 */
#define USBH_EP_TYPE_MASK 0x03u
#define USBH_EP_TYPE_CTRL 0x00u
#define USBH_EP_TYPE_ISOC 0x01u
#define USBH_EP_TYPE_BULK 0x02u
#define USBH_EP_TYPE_INTR 0x03u
/* Synchronization Type (Bits(3..2)).                   */
#define USBH_EP_TYPE_SYNC_NONE 0x00u
#define USBH_EP_TYPE_SYNC_ASYNC 0x01u
#define USBH_EP_TYPE_SYNC_ADAPTIVE 0x02u
#define USBH_EP_TYPE_SYNC_SYNC 0x03u
/* Usage Type (Bits(5..4)).                             */
#define USBH_EP_TYPE_USAGE_DATA 0x00u
#define USBH_EP_TYPE_USAGE_FEEDBACK 0x01u
#define USBH_EP_TYPE_USAGE_IMPLICIT_FEEDACK_DATA 0x02u

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.6, Table 9-13
 *
 *		(2) In an endpoint descriptor, the upper bit of
 *		'b_endpoint_address' indicates direction.
 */
#define USBH_EP_DIR_MASK 0x80u
#define USBH_EP_DIR_OUT 0x00u
#define USBH_EP_DIR_IN 0x80u
#define USBH_EP_DIR_NONE 0x01u

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 5.5 to 5.8
 *		The values come from:
 *		+ CONTROL    : Section 5.5.3
 *		+ ISOCHRONOUS: Section 5.6.3
 *		+ INTERRUPT  : Section 5.7.3
 *		+ BULK       : Section 5.8.3
 *
 *		(2) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.6
 *		Bits 12..11 from w_max_packet_size indicates
 *		the number of transaction per microframe.
 *		Valid for high-speed isochronous and interrupt endpoint only.
 */
/* See Note #1 */
#define USBH_MAX_EP_SIZE_TYPE_CTRL_LS 8u
#define USBH_MAX_EP_SIZE_TYPE_CTRL_FS 64u
#define USBH_MAX_EP_SIZE_TYPE_CTRL_HS 64u
#define USBH_MAX_EP_SIZE_TYPE_BULK_FS 64u
#define USBH_MAX_EP_SIZE_TYPE_BULK_HS 512u
#define USBH_MAX_EP_SIZE_TYPE_INTR_LS 8u
#define USBH_MAX_EP_SIZE_TYPE_INTR_FS 64u
#define USBH_MAX_EP_SIZE_TYPE_INTR_HS 1024u
#define USBH_MAX_EP_SIZE_TYPE_ISOC_FS 1023u
#define USBH_MAX_EP_SIZE_TYPE_ISOC_HS 1024u
/* See Note #2 */
#define USBH_NBR_TRANSACTION_PER_UFRAME (BIT(12) | BIT(11))
#define USBH_1_TRANSACTION_PER_UFRAME 0u
#define USBH_2_TRANSACTION_PER_UFRAME 1u
#define USBH_3_TRANSACTION_PER_UFRAME 2u

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.3, Table 9-10
 *
 *		(2) In a configuration descriptor, the 'bm_attributes'
 *		value is a bitmap composed of these values.
 */
#define USBH_CFG_DESC_SELF_POWERED 0xC0u
#define USBH_CFG_DESC_BUS_POWERED 0x80u
#define USBH_CFG_DESC_REMOTE_WAKEUP 0x20u

/*
 * Note(s) : (1) See http://www.usb.org/developers/docs/USB_LANGIDs.pdf
 */
#define USBH_LANG_ID_ARABIC_SAUDIARABIA 0x0401u
#define USBH_LANG_ID_CHINESE_TAIWAN 0x0404u
#define USBH_LANG_ID_ENGLISH_UNITEDSTATES 0x0409u
#define USBH_LANG_ID_ENGLISH_UNITEDKINGDOM 0x0809u
#define USBH_LANG_ID_FRENCH_STANDARD 0x040Cu
#define USBH_LANG_ID_GERMAN_STANDARD 0x0407u
#define USBH_LANG_ID_GREEK 0x0408u
#define USBH_LANG_ID_ITALIAN_STANDARD 0x0410u
#define USBH_LANG_ID_PORTUGUESE_STANDARD 0x0816u
#define USBH_LANG_ID_SANSKRIT 0x044Fu

/*
 * Note(s) : (1) See 'Universal Class Codes',
 *		www.usb.org/developers/defined_class.
 *
 *	(2) Except as noted, these should be used ONLY
 *		in interface descriptors.
 *
 *		(a) Can only be used in device descriptor.
 *
 *		(b) Can be used in either device or interface descriptor.
 *
 *	(4) Subclass & protocol codes are defined in the relevant class drivers.
 */
#define USBH_CLASS_CODE_USE_IF_DESC 0x00u       /* See Notes #2a.*/
#define USBH_CLASS_CODE_AUDIO 0x01u
#define USBH_CLASS_CODE_CDC_CTRL 0x02u          /* See Notes #2b.*/
#define USBH_CLASS_CODE_HID 0x03u
#define USBH_CLASS_CODE_PHYSICAL 0x05u
#define USBH_CLASS_CODE_IMAGE 0x06u
#define USBH_CLASS_CODE_PRINTER 0x07u
#define USBH_CLASS_CODE_MASS_STORAGE 0x08u
#define USBH_CLASS_CODE_HUB 0x09u /* See Notes #2a.*/
#define USBH_CLASS_CODE_CDC_DATA 0x0Au
#define USBH_CLASS_CODE_SMART_CARD 0x0Bu
#define USBH_CLASS_CODE_CONTENT_SECURITY 0x0Du
#define USBH_CLASS_CODE_VIDEO 0x0Eu
#define USBH_CLASS_CODE_PERSONAL_HEALTHCARE 0x0Fu
#define USBH_CLASS_CODE_DIAGNOSTIC_DEV 0xDCu    /* See Notes #2b*/
#define USBH_CLASS_CODE_WIRELESS_CTRLR 0xE0u
#define USBH_CLASS_CODE_MISCELLANEOUS 0xEFu     /* See Notes #2b.*/
#define USBH_CLASS_CODE_APP_SPECIFIC 0xFEu
#define USBH_CLASS_CODE_VENDOR_SPECIFIC 0xFFu   /* See Notes #2b.*/

/*
 * Note(s) : (1) See 'Universal Class Codes',
 *		www.usb.org/developers/defined_class.
 *
 *	(2) Except as noted, these should be used ONLY in interface descriptors.
 *
 *		(a) Can only be used in device descriptor.
 *
 *		(b) Can be used in either device or interface descriptor.
 *
 *	(4) Subclass & protocol codes are defined in the relevant class drivers.
 */
#define USBH_SUBCLASS_CODE_USE_IF_DESC 0x00u            /* See Notes #2a.*/
#define USBH_SUBCLASS_CODE_USE_COMMON_CLASS 0x02u       /* See Notes #2a.*/
#define USBH_SUBCLASS_CODE_VENDOR_SPECIFIC 0xFFu        /* See Notes #2b.*/

/*
 * Note(s) : (1) See 'Universal Class Codes',
 *		www.usb.org/developers/defined_class.
 *
 *	(2) Except as noted, these should be used ONLY in interface descriptors.
 *
 *		(a) Can only be used in device descriptor.
 *
 *		(b) See "USB Interface Association Descriptor
 *		Device Class Code and Use Model" Document at
 *		www.usb.org/developers/whitepapers/iadclasscode_r10.pdf.
 *
 *		(c) Can be used in either device or interface descriptor.
 *
 *	(4) Subclass & protocol codes are defined in the relevant class drivers.
 */
#define USBH_PROTOCOL_CODE_USE_IF_DESC 0x00u            /* See Notes #2a.*/
#define USBH_PROTOCOL_CODE_USE_IAD 0x01u                /* See Notes #2b.*/
#define USBH_PROTOCOL_CODE_VENDOR_SPECIFIC 0xFFu        /* See Notes #2c.*/

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 9.6.1, Table 9-8.
 *
 *		(2) The field "bcd_usb" is part of the device descriptor and
 *		indicates the release number of the
 *		USB specification to which the device complies.
 */
#define USBH_SPEC_RELEASE_NBR_1_0 0x0100u
#define USBH_SPEC_RELEASE_NBR_1_1 0x0110u
#define USBH_SPEC_RELEASE_NBR_2_0 0x0200u

#define USBO_OTG_DESC_HNP 0x01u /* Device is HNP capable*/
#define USBO_OTG_DESC_SRP 0x02u /* Device is SRP capable*/

/*
 * DEFAULT LANGUAGE ID
 */
#define USBH_STRING_DESC_LANGID 0x00u

/*
 * HOST STATE
 */
#define USBH_HOST_STATE_NONE 0u
#define USBH_HOST_STATE_READY 1u
#define USBH_HOST_STATE_SUSPENDED 2u
#define USBH_HOST_STATE_RESUMED 3u

/*
 * urb STATE
 */
#define USBH_URB_STATE_NONE 0u
#define USBH_URB_STATE_SCHEDULED 1u
#define USBH_URB_STATE_QUEUED 2u
#define USBH_URB_STATE_ABORTED 3u

/*
 * USB TOKEN
 */
#define USBH_TOKEN_SETUP 0u
#define USBH_TOKEN_OUT 1u
#define USBH_TOKEN_IN 2u

/*
 * USB DEV + RH DEV
 */

/* #define  USBH_MAX_NBR_DEVS USBH_CFG_MAX_NBR_DEVS + USBH_CFG_MAX_NBR_HC */

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 11.24.2, Table 11-16.
 *
 *		(2) The 'b_request' field of a class-specific setup
 *		request may contain one of these values.
 */
#define USBH_HUB_REQ_GET_STATUS 0x00u
#define USBH_HUB_REQ_CLR_FEATURE 0x01u
#define USBH_HUB_REQ_SET_FEATURE 0x03u
#define USBH_HUB_REQ_GET_DESC 0x06u
#define USBH_HUB_REQ_SET_DESC 0x07u
#define USBH_HUB_REQ_CLR_TT_BUF 0x08u
#define USBH_HUB_REQ_RESET_TT 0x09u
#define USBH_HUB_REQ_GET_TT_STATE 0x0Au
#define USBH_HUB_REQ_STOP_TT 0x0Bu

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 11.23.2, Table 11-13.
 *
 *           (2) For a 'get descriptor' setup request,
 *		the low byte of the 'w_value' field may contain
 *		one of these values.
 */
#define USBH_HUB_DESC_TYPE_HUB 0x29u

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 11.24.2, Table 11-17.
 *
 *		(2) For a 'clear feature' setup request,
 *		the 'w_value' field may contain one of these values.
 */
#define USBH_HUB_FEATURE_SEL_C_HUB_LOCAL_PWR 0u
#define USBH_HUB_FEATURE_SEL_C_HUB_OVER_CUR 1u

#define USBH_HUB_FEATURE_SEL_PORT_CONN 0u
#define USBH_HUB_FEATURE_SEL_PORT_EN 1u
#define USBH_HUB_FEATURE_SEL_PORT_SUSPEND 2u
#define USBH_HUB_FEATURE_SEL_PORT_OVER_CUR 3u
#define USBH_HUB_FEATURE_SEL_PORT_RESET 4u
#define USBH_HUB_FEATURE_SEL_PORT_PWR 8u
#define USBH_HUB_FEATURE_SEL_PORT_LOW_SPD 9u
#define USBH_HUB_FEATURE_SEL_C_PORT_CONN 16u
#define USBH_HUB_FEATURE_SEL_C_PORT_EN 17u
#define USBH_HUB_FEATURE_SEL_C_PORT_SUSPEND 18u
#define USBH_HUB_FEATURE_SEL_C_PORT_OVER_CUR 19u
#define USBH_HUB_FEATURE_SEL_C_PORT_RESET 20u
#define USBH_HUB_FEATURE_SEL_PORT_TEST 21u
#define USBH_HUB_FEATURE_SEL_PORT_INDICATOR 22u

/*
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 11.24.2.7.1, Table 11-21.
 */
#define USBH_HUB_STATUS_PORT_CONN 0x0001u
#define USBH_HUB_STATUS_PORT_EN 0x0002u
#define USBH_HUB_STATUS_PORT_SUSPEND 0x0004u
#define USBH_HUB_STATUS_PORT_OVER_CUR 0x0008u
#define USBH_HUB_STATUS_PORT_RESET 0x0010u
#define USBH_HUB_STATUS_PORT_PWR 0x0100u
#define USBH_HUB_STATUS_PORT_LOW_SPD 0x0200u
#define USBH_HUB_STATUS_PORT_FULL_SPD 0x0000u
#define USBH_HUB_STATUS_PORT_HIGH_SPD 0x0400u
#define USBH_HUB_STATUS_PORT_TEST 0x0800u
#define USBH_HUB_STATUS_PORT_INDICATOR 0x1000u

/*
 * HUB PORT STATUS CHANGE BITS
 * Note(s) : (1) See 'Universal Serial Bus Specification Revision 2.0',
 *		Section 11.24.2.7.2, Table 11-22.
 */
#define USBH_HUB_STATUS_C_PORT_CONN 0x0001u
#define USBH_HUB_STATUS_C_PORT_EN 0x0002u
#define USBH_HUB_STATUS_C_PORT_SUSPEND 0x0004u
#define USBH_HUB_STATUS_C_PORT_OVER_CUR 0x0008u
#define USBH_HUB_STATUS_C_PORT_RESET 0x0010u

/*
 * XFER COMPLETE NOTIFICATION FNCT
 */
typedef void (*usbh_isoc_cmpl_fnct)(struct usbh_ep *p_ep, uint8_t *p_buf,
				    uint32_t buf_len, uint32_t cur_xfer_len,
				    uint32_t start_frm, uint32_t nbr_frm,
				    uint16_t *p_frm_len, int *p_frm_err,
				    void *p_arg, int err);

typedef void (*usbh_xfer_cmpl_fnct)(struct usbh_ep *p_ep, void *p_buf,
				    uint32_t buf_len, uint32_t xfer_len,
				    void *p_arg, int err);

#define USBH_OS_SEM_REQUIRED					      \
	(3u + (((USBH_CFG_MAX_NBR_EPS * USBH_CFG_MAX_NBR_IFS) + 1u) * \
	       USBH_CFG_MAX_NBR_DEVS))
#define DEF_BIT_IS_SET(val, mask) (((val & mask) == mask) ? true : false)
#define DEF_BIT_IS_CLR(val, bit) !DEF_BIT_IS_SET(val, bit)

/*  USB HOST STACK GENERAL FUNCTIONS  */
uint32_t usbh_version_get(void);

int usbh_init(void);

int usbh_suspend(void);

int usbh_resume(void);

/*  HOST CONTROLLER FUNCTIONS  */
uint8_t usbh_hc_add(int *p_err);

int usbh_hc_start(uint8_t hc_nbr);

int usbh_hc_stop(uint8_t hc_nbr);

int usbh_hc_port_en(uint8_t hc_nbr, uint8_t port_nbr);

int usbh_hc_port_dis(uint8_t hc_nbr, uint8_t port_nbr);

uint32_t usbh_hc_frame_nbr_get(uint8_t hc_nbr, int *p_err);

/*  DEVICE CONTROL FUNCTIONS  */
int usbh_dev_conn(struct usbh_dev *p_dev);

void usbh_dev_disconn(struct usbh_dev *p_dev);

uint8_t usbh_dev_cfg_nbr_get(struct usbh_dev *p_dev);

void usbh_dev_desc_get(struct usbh_dev *p_dev,
		       struct usbh_dev_desc *p_dev_desc);

/*  DEVICE CONFIGURATION FUNCTIONS  */
int usbh_cfg_set(struct usbh_dev *p_dev, uint8_t cfg_nbr);

struct usbh_cfg *usbh_cfg_get(struct usbh_dev *p_dev, uint8_t cfg_ix);

uint8_t usbh_cfg_if_nbr_get(struct usbh_cfg *p_cfg);

int usbh_cfg_desc_get(struct usbh_cfg *p_cfg, struct usbh_cfg_desc *p_cfg_desc);

struct usbh_desc_hdr *usbh_cfg_extra_desc_get(struct usbh_cfg *p_cfg,
					      int *p_err);

/*  DEVICE INTERFACE CONTROL FUNCTIONS  */
int usbh_if_set(struct usbh_if *p_if, uint8_t alt_nbr);

struct usbh_if *usbu_if_get(struct usbh_cfg *p_cfg, uint8_t if_ix);

uint8_t usbh_if_alt_nbr_get(struct usbh_if *p_if);

uint8_t usbh_if_nbr_get(struct usbh_if *p_if);

uint8_t usbh_if_ep_nbr_get(struct usbh_if *p_if, uint8_t alt_ix);

int usbh_if_desc_get(struct usbh_if *p_if, uint8_t alt_ix,
		     struct usbh_if_desc *p_if_desc);

uint8_t *usbh_if_extra_desc_get(struct usbh_if *p_if, uint8_t alt_ix,
				uint16_t *p_data_len);

/*  DEVICE ENDPOINT OPEN FUNCTIONS  */
int usbh_bulk_in_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		      struct usbh_ep *p_ep);

int usbh_bulk_out_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		       struct usbh_ep *p_ep);

int usbh_intr_in_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		      struct usbh_ep *p_ep);

int usbh_intr_out_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		       struct usbh_ep *p_ep);

int usbh_isoc_in_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		      struct usbh_ep *p_ep);

int usbh_isoc_out_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		       struct usbh_ep *p_ep);

/*  DEVICE ENDPOINT TRANSFER FUNCTIONS  */
uint16_t usbh_ctrl_tx(struct usbh_dev *p_dev, uint8_t b_req,
		      uint8_t bm_req_type, uint16_t w_val, uint16_t w_ix,
		      void *p_data, uint16_t w_len, uint32_t timeout_ms,
		      int *p_err);

uint16_t usbh_ctrl_rx(struct usbh_dev *p_dev, uint8_t b_req,
		      uint8_t bm_req_type, uint16_t w_val, uint16_t w_ix,
		      void *p_data, uint16_t w_len, uint32_t timeout_ms,
		      int *p_err);

uint32_t usbh_bulk_tx(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		      uint32_t timeout_ms, int *p_err);

int usbh_bulk_tx_async(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		       usbh_xfer_cmpl_fnct fnct, void *p_fnct_arg);

uint32_t usbh_bulk_rx(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		      uint32_t timeout_ms, int *p_err);

int usbh_bulk_rx_async(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		       usbh_xfer_cmpl_fnct fnct, void *p_fnct_arg);

uint32_t usbh_intr_tx(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		      uint32_t timeout_ms, int *p_err);

int usbh_intr_tx_async(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		       usbh_xfer_cmpl_fnct fnct, void *p_fnct_arg);

uint32_t usbh_intr_rx(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		      uint32_t timeout_ms, int *p_err);

int usbh_intr_rx_async(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		       usbh_xfer_cmpl_fnct fnct, void *p_fnct_arg);

uint32_t usbh_isoc_tx(struct usbh_ep *p_ep, uint8_t *p_buf, uint32_t buf_len,
		      uint32_t start_frm, uint32_t nbr_frm, uint16_t *p_frm_len,
		      int *p_frm_err, uint32_t timeout_ms, int *p_err);

int usbh_isoc_tx_async(struct usbh_ep *p_ep, uint8_t *p_buf, uint32_t buf_len,
		       uint32_t start_frm, uint32_t nbr_frm,
		       uint16_t *p_frm_len, int *p_frm_err,
		       usbh_isoc_cmpl_fnct fnct, void *p_fnct_arg);

uint32_t usbh_isoc_rx(struct usbh_ep *p_ep, uint8_t *p_buf, uint32_t buf_len,
		      uint32_t start_frm, uint32_t nbr_frm, uint16_t *p_frm_len,
		      int *p_frm_err, uint32_t timeout_ms, int *p_err);

int usbh_isoc_rx_async(struct usbh_ep *p_ep, uint8_t *p_buf, uint32_t buf_len,
		       uint32_t start_frm, uint32_t nbr_frm,
		       uint16_t *p_frm_len, int *p_frm_err,
		       usbh_isoc_cmpl_fnct fnct, void *p_fnct_arg);

/*  DEVICE ENDPOINT FUNCTIONS  */
uint8_t usbh_ep_log_nbr_get(struct usbh_ep *p_ep);

uint8_t usbh_ep_dir_get(struct usbh_ep *p_ep);

uint16_t usbh_ep_max_pkt_size_get(struct usbh_ep *p_ep);

uint8_t usbh_ep_type_get(struct usbh_ep *p_ep);

int usbh_ep_get(struct usbh_if *p_if, uint8_t alt_ix, uint8_t ep_ix,
		struct usbh_ep *p_ep);

int usbh_ep_stall_set(struct usbh_ep *p_ep);

int usbh_ep_stall_clr(struct usbh_ep *p_ep);

int usbh_ep_reset(struct usbh_dev *p_dev, struct usbh_ep *p_ep);

int usbh_ep_close(struct usbh_ep *p_ep);

/*  USB REQUEST BLOCK FUNCTIONS  */
void usbh_urb_done(struct usbh_urb *p_urb);

int usbh_urb_complete(struct usbh_urb *p_urb);

/*  MISCELLANEOUS FUNCTIONS  */
uint32_t usbh_str_get(struct usbh_dev *p_dev, uint8_t desc_ix, uint16_t lang_id,
		      uint8_t *p_buf, uint32_t buf_len, int *p_err);

#ifndef USBH_CFG_MAX_NBR_DEVS
#error "USBH_CFG_MAX_NBR_DEVS not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_NBR_DEVS < 1u)
#error "USBH_CFG_MAX_NBR_DEVS illegally #define'd in 'usbh_cfg.h'"
#error " w[MUST be >= 1]"
#elif (USBH_CFG_MAX_NBR_DEVS > 127u)
#error "USBH_CFG_MAX_NBR_DEVS illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be <= 127]"
#endif

#ifndef USBH_CFG_MAX_NBR_CFGS
#error "USBH_CFG_MAX_NBR_CFGS not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_NBR_CFGS < 1u)
#error "USBH_CFG_MAX_NBR_CFGS illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1] "
#endif

#ifndef USBH_CFG_MAX_NBR_IFS
#error "USBH_CFG_MAX_NBR_IFS not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_NBR_IFS < 1u)
#error "USBH_CFG_MAX_NBR_IFS illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1] "
#endif

#ifndef USBH_CFG_MAX_NBR_EPS
#error "USBH_CFG_MAX_NBR_EPS not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_NBR_EPS < 1u)
#error "USBH_CFG_MAX_NBR_EPS illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1] "
#endif

#ifndef USBH_CFG_MAX_NBR_CLASS_DRVS
#error "USBH_CFG_MAX_NBR_CLASS_DRVS not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_NBR_CLASS_DRVS < 2u)
#error "USBH_CFG_MAX_NBR_CLASS_DRVS illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 2] "
#endif

#ifndef USBH_CFG_MAX_CFG_DATA_LEN
#error "USBH_CFG_MAX_CFG_DATA_LEN not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_CFG_DATA_LEN < 18u)
#error "USBH_CFG_MAX_CFG_DATA_LEN illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 18] "
#endif

#ifndef USBH_CFG_MAX_HUBS
#error "USBH_CFG_MAX_HUBS not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_HUBS < 1u)
#error "USBH_CFG_MAX_HUBS illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1 (min value = 1 root hub)}"
#endif

#ifndef USBH_CFG_MAX_HUB_PORTS
#error "USBH_CFG_MAX_HUB_PORTS not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_HUB_PORTS < 1u)
#error "USBH_CFG_MAX_HUB_PORTS illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1] "
#elif (USBH_CFG_MAX_HUB_PORTS > 7u)
#error "USBH_CFG_MAX_HUB_PORTS illegally #define'd in 'usbh_cfg.h'"
#error "USBH_CFG_MAX_HUB_PORTS cannot exceed 7 ports per external hub. "
#endif

#ifndef USBH_CFG_STD_REQ_RETRY
#error "USBH_CFG_STD_REQ_RETRY not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_STD_REQ_RETRY < 1u)
#error "USBH_CFG_STD_REQ_RETRY illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1] "
#endif

#ifndef USBH_CFG_STD_REQ_TIMEOUT
#error "USBH_CFG_STD_REQ_TIMEOUT not #define'd in 'usbh_cfg.h'"
#endif

#ifndef USBH_CFG_MAX_STR_LEN
#error "USBH_CFG_MAX_STR_LEN not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_STR_LEN > 256u)
#error "USBH_CFG_MAX_STR_LEN illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be <= 256] "
#endif

#ifndef USBH_CFG_MAX_NBR_HC
#error "USBH_CFG_MAX_NBR_HC not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_NBR_HC < 1u)
#error "USBH_CFG_MAX_NBR_HC illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1] "
#endif

#ifndef USBH_CFG_MAX_ISOC_DESC
#error "USBH_CFG_MAX_ISOC_DESC not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_ISOC_DESC < 1u)
#error "USBH_CFG_MAX_ISOC_DESC illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1] "
#endif

#ifndef USBH_CFG_MAX_EXTRA_URB_PER_DEV
#error "USBH_CFG_MAX_EXTRA_URB_PER_DEV not #define'd in 'usbh_cfg.h'"
#elif (USBH_CFG_MAX_EXTRA_URB_PER_DEV < 1u)
#error "USBH_CFG_MAX_EXTRA_URB_PER_DEV illegally #define'd in 'usbh_cfg.h'"
#error " [MUST be >= 1] "
#endif

#ifndef USBH_CFG_MAX_NUM_DEV_RECONN
#error "USBH_CFG_MAX_NUM_DEV_RECONN not #define'd in 'usbh_cfg.h'"
#endif

#endif
