/*
 * Copyright (c) 2025 Quectel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <string.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/userial/quectel/quec_uhc_app.h>

#include "quec_ringbuffer.h"

/*===========================================================================
 * 								define
 ===========================================================================*/
#define UHC_PORT_INVALID				0xFF

#define QUEC_SYSTEM_PORT 				0
#define QUEC_AT_INTF_NUM				2
#define QUEC_MODEM_INTF_NUM				3

#define DEVICE_DESC_PRE_SIZE			8
#define CFG_DESC_MAX_SIZE				512

#define USB_DESC_TYPE_DEVICE            0x01
#define USB_DESC_TYPE_CONFIGURATION     0x02
#define USB_DESC_TYPE_STRING            0x03
#define USB_DESC_TYPE_INTERFACE         0x04
#define USB_DESC_TYPE_ENDPOINT          0x05
#define USB_DESC_TYPE_DEVICEQUALIFIER   0x06
#define USB_DESC_TYPE_OTHERSPEED        0x07
#define USB_DESC_TYPE_IAD               0x0b
#define USB_DESC_TYPE_HID               0x21
#define USB_DESC_TYPE_REPORT            0x22
#define USB_DESC_TYPE_PHYSICAL          0x23
#define USB_DESC_TYPE_HUB               0x29

#define USB_EP_ATTR_CONTROL             0x00
#define USB_EP_ATTR_ISOC                0x01
#define USB_EP_ATTR_BULK                0x02
#define USB_EP_ATTR_INT                 0x03
#define USB_EP_ATTR_TYPE_MASK           0x03

#define USB_DIR_OUT                     0x00
#define USB_DIR_IN                      0x80

#define USB_REQ_TYPE_DIR_OUT            0x00
#define USB_REQ_TYPE_DIR_IN             0x80

#define USB_REQ_TYPE_DEVICE             0x00
#define USB_REQ_TYPE_INTERFACE          0x01
#define USB_REQ_TYPE_ENDPOINT           0x02
#define USB_REQ_TYPE_OTHER              0x03
#define USB_REQ_TYPE_RECIPIENT_MASK     0x1f

#define USB_REQ_TYPE_STANDARD           0x00
#define USB_REQ_TYPE_CLASS              0x20
#define USB_REQ_TYPE_VENDOR             0x40
#define USB_REQ_TYPE_MASK               0x60

#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_FEATURE             0x03
#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0A
#define USB_REQ_SET_INTERFACE           0x0B
#define USB_REQ_SYNCH_FRAME             0x0C
#define USB_REQ_SET_ENCRYPTION          0x0D
#define USB_REQ_GET_ENCRYPTION          0x0E
#define USB_REQ_RPIPE_ABORT             0x0E
#define USB_REQ_SET_HANDSHAKE           0x0F
#define USB_REQ_RPIPE_RESET             0x0F
#define USB_REQ_GET_HANDSHAKE           0x10
#define USB_REQ_SET_CONNECTION          0x11
#define USB_REQ_SET_SECURITY_DATA       0x12
#define USB_REQ_GET_SECURITY_DATA       0x13
#define USB_REQ_SET_WUSB_DATA           0x14
#define USB_REQ_LOOPBACK_DATA_WRITE     0x15
#define USB_REQ_LOOPBACK_DATA_READ      0x16
#define USB_REQ_SET_INTERFACE_DS        0x17
#define USB_REQ_SET_LINE_STATE        	0x22

#define USB_TRANS_ID					11388

#define USBH_PID_SETUP                  0x00
#define USBH_PID_DATA                   0x01

#define USB_FIFO_SIZE				(4 * 1024)
#define USB_RX_TRIG_LEVEL			(4 * 1024)

#define USB_RX_TRIG_TIMEOUT			(50) //50ms
#define USB_FS_PKT_SIZE				64

#define	QUEC_RX_STACK_SIZE			(8 * 1024)
#define	QUEC_TX_STACK_SIZE			(8 * 1024)


/*===========================================================================
 * 								ENUM
 ===========================================================================*/
typedef enum
{
	QUEC_PORT_STATUS_INVALID = 0,
	QUEC_PORT_STATUS_FREE,
	QUEC_PORT_STATUS_OPEN
}quec_port_status_e;


/*===========================================================================
 * 								Struct
 ===========================================================================*/
struct usb_intf_ep_desc
{
	struct usb_if_descriptor intf_desc;
	struct usb_ep_descriptor ctrl_ep_desc;
	struct usb_ep_descriptor in_ep_desc;
	struct usb_ep_descriptor out_ep_desc;
} __packed;

struct uhc_cfg_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
	uint8_t data[512];
} __packed;

typedef struct usb_desc_header usb_desc_head_t;
typedef struct usb_device_descriptor usb_device_desc_t;
typedef struct usb_cfg_descriptor usb_cfg_desc_t;
typedef struct usb_if_descriptor usb_intf_desc_t;
typedef struct usb_ep_descriptor usb_endp_desc_t;
typedef struct usb_intf_ep_desc usb_intf_ep_desc_t;
typedef struct uhc_cfg_descriptor uhc_cfg_descriptor_t;

typedef void(*quec_uhc_drv_cb_t)(uint32_t event, void *ctx);

typedef void(*quec_uhc_trans_cb_t)(void *ctx);

typedef struct
{
	usb_endp_desc_t 	*ep_desc;
	uint16_t			trans_id;
	uint8_t				cdc_num;
	uint8_t				port_num;
	uint8_t 			token;
	uint8_t 			*buffer;
	int 				nbytes;
	int					actual;
	int					cached;
	int 				timeouts;
	int					status;
	quec_uhc_trans_cb_t	callback;
}quec_uhc_xfer_t;

typedef struct
{
	usb_endp_desc_t		ep_desc;
	uint8_t				port_num;
	uint8_t				cache[64];
	ring_buffer_t		fifo;
	quec_uhc_xfer_t		xfer;
	uint8_t				is_busy;
	
	uint8_t 			*task_stack;
	struct k_msgq 		*msgq;
	struct k_thread		thread;
}quec_uhc_pmg_t;

typedef struct
{
	bool				occupied;
	quec_uhc_xfer_t		*xfer;
}quec_udrv_port_t;

typedef struct
{
	HCD_HandleTypeDef 				hcd;
	uint32_t						port_index;
	uint8_t							status;
	uint8_t							dev_address;
	quec_uhc_drv_cb_t				callback;
	quec_udrv_port_t				port[16];
}quec_udrv_mgr_t;

typedef struct
{
	uint32_t event_id;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
}quec_uhc_msg_t;

typedef struct
{
    uint8_t  request_type;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
}quec_uhc_req_t;

typedef struct
{
	int (*init)(const struct device *dev, quec_uhc_drv_cb_t callback);
	int (*deinit)(const struct device *dev);
	int (*reset)(const struct device *dev);
	int (*set_address)(const struct device *dev, uint8_t address);
	int (*ep_enable)(const struct device *dev, usb_endp_desc_t *port);
	int (*ep_disable)(const struct device *dev, uint8_t port);	
	int (*enqueue)(const struct device *dev, quec_uhc_xfer_t *xfer);
}quec_udrv_api_t;

typedef struct
{
	uint8_t		cdc_num;
	uint32_t	status;
	uint32_t	size;
}quec_trans_status_t;

typedef struct
{
	uint8_t				intf_num;
	quec_port_status_e	status;
	quec_uhc_pmg_t		rx_port;
	quec_uhc_pmg_t		tx_port;
	quec_uhc_pmg_t		ctl_port;;
}quec_uhc_dev_t;

typedef struct
{
	const struct device *device;
	quec_udrv_api_t		*api;
	uint16_t			trans;
	uint8_t				dev_address;
	uint8_t				status;
	quec_uhc_pmg_t		sys_port;	
	quec_uhc_callback	user_callback;
	quec_uhc_dev_t		dev[QUEC_PORT_MAX];
}quec_uhc_mgr_t;


/*===========================================================================
 * 								Function
 ===========================================================================*/
int quec_uhc_enum_process(quec_uhc_mgr_t *dev, usb_device_desc_t *dev_desc, uhc_cfg_descriptor_t *cfg_desc);

int quec_uhc_parse_config_desc(uhc_cfg_descriptor_t *cfg_desc, int intf_num, usb_intf_ep_desc_t *desc);

int quec_uhc_open(const struct device *dev, quec_cdc_port_e port_id);

int quec_uhc_set_line_state(quec_uhc_mgr_t *udev, uint16_t intf, uint16_t value);

int quec_uhc_set_interface(quec_uhc_mgr_t *udev, uint16_t intf);

void quec_uhc_cdc_memory_init(quec_uhc_dev_t *dev, uint8_t port);

void quec_uhc_sys_memory_init(quec_uhc_pmg_t *port);

int quec_uhc_sio_deinit(quec_uhc_mgr_t *udev);

int quec_uhc_msg_put(struct k_msgq *msgq, uint32_t event_id, uint32_t param1, uint32_t param2);

