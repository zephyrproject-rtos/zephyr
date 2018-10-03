/*******************************************************************************
 *
 * Copyright(c) 2015,2016 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/**
 * @file
 * @brief CDC ACM device class driver
 *
 * Driver for USB CDC ACM device class driver
 */

#include <kernel.h>
#include <init.h>
#include <uart.h>
#include <string.h>
#include <misc/byteorder.h>
#include <usb/class/usb_cdc.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb_descriptor.h>

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
#error "CONFIG_UART_INTERRUPT_DRIVEN must be set for CDC ACM driver"
#endif

/* definitions */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_CDC_ACM_LEVEL
#include <logging/sys_log.h>

#define DEV_DATA(dev)						\
	((struct cdc_acm_dev_data_t * const)(dev)->driver_data)

/* 115200bps, no parity, 1 stop bit, 8bit char */
#define CDC_ACM_DEFAUL_BAUDRATE {sys_cpu_to_le32(115200), 0, 0, 8}

/* Size of the internal buffer used for storing received data */
#define CDC_ACM_BUFFER_SIZE (2 * CONFIG_CDC_ACM_BULK_EP_MPS)


/* Max CDC ACM class request max data size */
#define CDC_CLASS_REQ_MAX_DATA_SIZE	8

/* Serial state notification timeout */
#define CDC_CONTROL_SERIAL_STATE_TIMEOUT_US 100000

#define CDC_ACM_INT_EP_ADDR		0x85
#define CDC_ACM_IN_EP_ADDR		0x84
#define CDC_ACM_OUT_EP_ADDR		0x03

#define ACM_INT_EP_IDX			0
#define ACM_OUT_EP_IDX			1
#define ACM_IN_EP_IDX			2

#define ACM_IF0_STRING			"ACM-CDC"

struct usb_cdc_acm_config {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	struct usb_association_descriptor iad_cdc;
#endif
	struct usb_if_descriptor if0;
	struct cdc_header_descriptor if0_header;
	struct cdc_cm_descriptor if0_cm;
	struct cdc_acm_descriptor if0_acm;
	struct cdc_union_descriptor if0_union;
	struct usb_ep_descriptor if0_int_ep;

	struct usb_if_descriptor if1;
	struct usb_ep_descriptor if1_in_ep;
	struct usb_ep_descriptor if1_out_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary) struct usb_cdc_acm_config cdc_acm_cfg = {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	.iad_cdc = {
		.bLength = sizeof(struct usb_association_descriptor),
		.bDescriptorType = USB_ASSOCIATION_DESC,
		.bFirstInterface = 0,
		.bInterfaceCount = 0x02,
		.bFunctionClass = COMMUNICATION_DEVICE_CLASS,
		.bFunctionSubClass = ACM_SUBCLASS,
		.bFunctionProtocol = 0,
		.iFunction = 0,
	},
#endif
	/* Interface descriptor */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = COMMUNICATION_DEVICE_CLASS,
		.bInterfaceSubClass = ACM_SUBCLASS,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	/* Header Functional Descriptor */
	.if0_header = {
		.bFunctionLength = sizeof(struct cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = HEADER_FUNC_DESC,
		.bcdCDC = sys_cpu_to_le16(USB_1_1),
	},
	/* Call Management Functional Descriptor */
	.if0_cm = {
		.bFunctionLength = sizeof(struct cdc_cm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = CALL_MANAGEMENT_FUNC_DESC,
		.bmCapabilities = 0x02,
		.bDataInterface = 1,
	},
	/* ACM Functional Descriptor */
	.if0_acm = {
		.bFunctionLength = sizeof(struct cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = ACM_FUNC_DESC,
		/* Device supports the request combination of:
		 *	Set_Line_Coding,
		 *	Set_Control_Line_State,
		 *	Get_Line_Coding
		 *	and the notification Serial_State
		 */
		.bmCapabilities = 0x02,
	},
	/* Union Functional Descriptor */
	.if0_union = {
		.bFunctionLength = sizeof(struct cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = UNION_FUNC_DESC,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	},
	/* Endpoint INT */
	.if0_int_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_ACM_INT_EP_ADDR,
		.bmAttributes = USB_DC_EP_INTERRUPT,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_ACM_INTERRUPT_EP_MPS),
		.bInterval = 0x0A,
	},
	/* Interface descriptor */
	.if1 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = COMMUNICATION_DEVICE_CLASS_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	/* First Endpoint IN */
	.if1_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_ACM_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_ACM_BULK_EP_MPS),
		.bInterval = 0x00,
	},
	/* Second Endpoint OUT */
	.if1_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_ACM_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_ACM_BULK_EP_MPS),
		.bInterval = 0x00,
	},
};

struct device *cdc_acm_dev;

static struct k_sem poll_wait_sem;

/* Device data structure */
struct cdc_acm_dev_data_t {
	/* USB device status code */
	enum usb_dc_status_code usb_status;
	/* Callback function pointer/arg */
	uart_irq_callback_user_data_t cb;
	void *cb_data;
	/* Tx ready status. Signals when */
	u8_t tx_ready;
	u8_t rx_ready;                 /* Rx ready status */
	u8_t tx_irq_ena;               /* Tx interrupt enable status */
	u8_t rx_irq_ena;               /* Rx interrupt enable status */
	u8_t rx_buf[CDC_ACM_BUFFER_SIZE];/* Internal Rx buffer */
	u32_t rx_buf_head;             /* Head of the internal Rx buffer */
	u32_t rx_buf_tail;             /* Tail of the internal Rx buffer */
	/* Interface data buffer */
#ifndef CONFIG_USB_COMPOSITE_DEVICE
	u8_t interface_data[CDC_CLASS_REQ_MAX_DATA_SIZE];
#endif
	/* CDC ACM line coding properties. LE order */
	struct cdc_acm_line_coding line_coding;
	/* CDC ACM line state bitmap, DTE side */
	u8_t line_state;
	/* CDC ACM serial state bitmap, DCE side */
	u8_t serial_state;
	/* CDC ACM notification sent status */
	u8_t notification_sent;
};

/**
 * @brief Handler called for Class requests not handled by the USB stack.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int cdc_acm_class_handle_req(struct usb_setup_packet *pSetup,
			     s32_t *len, u8_t **data)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(cdc_acm_dev);

	switch (pSetup->bRequest) {
	case SET_LINE_CODING:
		memcpy(&dev_data->line_coding,
		       *data, sizeof(dev_data->line_coding));
		SYS_LOG_DBG("\nCDC_SET_LINE_CODING %d %d %d %d",
			    sys_le32_to_cpu(dev_data->line_coding.dwDTERate),
			    dev_data->line_coding.bCharFormat,
			    dev_data->line_coding.bParityType,
			    dev_data->line_coding.bDataBits);
		break;

	case SET_CONTROL_LINE_STATE:
		dev_data->line_state = (u8_t)sys_le16_to_cpu(pSetup->wValue);
		SYS_LOG_DBG("CDC_SET_CONTROL_LINE_STATE 0x%x",
			    dev_data->line_state);
		break;

	case GET_LINE_CODING:
		*data = (u8_t *)(&dev_data->line_coding);
		*len = sizeof(dev_data->line_coding);
		SYS_LOG_DBG("\nCDC_GET_LINE_CODING %d %d %d %d",
			    sys_le32_to_cpu(dev_data->line_coding.dwDTERate),
			    dev_data->line_coding.bCharFormat,
			    dev_data->line_coding.bParityType,
			    dev_data->line_coding.bDataBits);
		break;

	default:
		SYS_LOG_DBG("CDC ACM request 0x%x, value 0x%x",
		    pSetup->bRequest, pSetup->wValue);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief EP Bulk IN handler, used to send data to the Host
 *
 * @param ep        Endpoint address.
 * @param ep_status Endpoint status code.
 *
 * @return  N/A.
 */
static void cdc_acm_bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(cdc_acm_dev);

	ARG_UNUSED(ep_status);
	ARG_UNUSED(ep);

	dev_data->tx_ready = 1;
	k_sem_give(&poll_wait_sem);
	/* Call callback only if tx irq ena */
	if (dev_data->cb && dev_data->tx_irq_ena) {
		dev_data->cb(dev_data->cb_data);
	}
}

/**
 * @brief EP Bulk OUT handler, used to read the data received from the Host
 *
 * @param ep        Endpoint address.
 * @param ep_status Endpoint status code.
 *
 * @return  N/A.
 */
static void cdc_acm_bulk_out(u8_t ep,
			     enum usb_dc_ep_cb_status_code ep_status)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(cdc_acm_dev);
	u32_t bytes_to_read, i, j, buf_head;
	u8_t tmp_buf[4];

	ARG_UNUSED(ep_status);

	/* Check how many bytes were received */
	usb_read(ep, NULL, 0, &bytes_to_read);

	buf_head = dev_data->rx_buf_head;

	/*
	 * Quark SE USB controller is always storing data
	 * in the FIFOs per 32-bit words.
	 */
	for (i = 0; i < bytes_to_read; i += 4) {
		usb_read(ep, tmp_buf, 4, NULL);

		for (j = 0; j < 4; j++) {
			if (i + j == bytes_to_read) {
				/* We read all the data */
				break;
			}

			if (((buf_head + 1) % CDC_ACM_BUFFER_SIZE) ==
			    dev_data->rx_buf_tail) {
				/* FIFO full, discard data */
				SYS_LOG_ERR("CDC buffer full!");
			} else {
				dev_data->rx_buf[buf_head] = tmp_buf[j];
				buf_head = (buf_head + 1) % CDC_ACM_BUFFER_SIZE;
			}
		}
	}

	dev_data->rx_buf_head = buf_head;
	dev_data->rx_ready = 1;
	/* Call callback only if rx irq ena */
	if (dev_data->cb && dev_data->rx_irq_ena) {
		dev_data->cb(dev_data->cb_data);
	}
}

/**
 * @brief EP Interrupt handler
 *
 * @param ep        Endpoint address.
 * @param ep_status Endpoint status code.
 *
 * @return  N/A.
 */
static void cdc_acm_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(cdc_acm_dev);

	ARG_UNUSED(ep_status);

	dev_data->notification_sent = 1;
	SYS_LOG_DBG("CDC_IntIN EP[%x]\r", ep);
}

/**
 * @brief Callback used to know the USB connection status
 *
 * @param status USB device status code.
 *
 * @return  N/A.
 */
static void cdc_acm_dev_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(cdc_acm_dev);

	ARG_UNUSED(param);

	/* Store the new status */
	dev_data->usb_status = status;

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		SYS_LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		SYS_LOG_DBG("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		SYS_LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		SYS_LOG_DBG("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		SYS_LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		SYS_LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		SYS_LOG_DBG("USB device resumed");
		break;
	case USB_DC_UNKNOWN:
	default:
		SYS_LOG_DBG("USB unknown state");
		break;
	}
}

static void cdc_interface_config(u8_t bInterfaceNumber)
{
	cdc_acm_cfg.if0.bInterfaceNumber = bInterfaceNumber;
	cdc_acm_cfg.if0_union.bControlInterface = bInterfaceNumber;
	cdc_acm_cfg.if1.bInterfaceNumber = bInterfaceNumber + 1;
	cdc_acm_cfg.if0_union.bSubordinateInterface0 = bInterfaceNumber + 1;
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	cdc_acm_cfg.iad_cdc.bFirstInterface = bInterfaceNumber;
#endif
}

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data cdc_acm_ep_data[] = {
	{
		.ep_cb	= cdc_acm_int_in,
		.ep_addr = CDC_ACM_INT_EP_ADDR
	},
	{
		.ep_cb	= cdc_acm_bulk_out,
		.ep_addr = CDC_ACM_OUT_EP_ADDR
	},
	{
		.ep_cb = cdc_acm_bulk_in,
		.ep_addr = CDC_ACM_IN_EP_ADDR
	}
};

/* Configuration of the CDC-ACM Device send to the USB Driver */
USBD_CFG_DATA_DEFINE(cdc_acm) struct usb_cfg_data cdc_acm_config = {
	.usb_device_description = NULL,
	.interface_config = cdc_interface_config,
	.interface_descriptor = &cdc_acm_cfg.if0,
	.cb_usb_status = cdc_acm_dev_status_cb,
	.interface = {
		.class_handler = cdc_acm_class_handle_req,
		.custom_handler = NULL,
		.payload_data = NULL,
	},
	.num_endpoints = ARRAY_SIZE(cdc_acm_ep_data),
	.endpoint = cdc_acm_ep_data
};

/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UART.
 *
 * @param dev             CDC ACM device struct.
 * @param baudrate        Baud rate.
 *
 * @return N/A.
 */
static void cdc_acm_baudrate_set(struct device *dev, u32_t baudrate)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->line_coding.dwDTERate = sys_cpu_to_le32(baudrate);
}

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev CDC ACM device struct.
 *
 * @return 0 always.
 */
static int cdc_acm_init(struct device *dev)
{
	cdc_acm_dev = dev;

#ifndef CONFIG_USB_COMPOSITE_DEVICE
	int ret;
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	cdc_acm_config.interface.payload_data = dev_data->interface_data;
	cdc_acm_config.usb_device_description = usb_get_device_descriptor();
	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&cdc_acm_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&cdc_acm_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}
#endif
	k_sem_init(&poll_wait_sem, 0, UINT_MAX);

	return 0;
}

/**
 * @brief Fill FIFO with data
 *
 * @param dev     CDC ACM device struct.
 * @param tx_data Data to transmit.
 * @param len     Number of bytes to send.
 *
 * @return Number of bytes sent.
 */
static int cdc_acm_fifo_fill(struct device *dev,
			     const u8_t *tx_data, int len)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);
	u32_t wrote = 0;
	int err;

	if (dev_data->usb_status != USB_DC_CONFIGURED) {
		return 0;
	}

	dev_data->tx_ready = 0;

	/* FIXME: On Quark SE Family processor, restrict writing more than
	 * 4 bytes into TX USB Endpoint. When more than 4 bytes are written,
	 * sometimes (freq ~1/3000) first 4 bytes are  repeated.
	 * (example: abcdef prints as abcdabcdef) (refer Jira GH-3515).
	 * Application should handle partial data transfer while writing
	 * into USB TX Endpoint.
	 */
#ifdef CONFIG_SOC_SERIES_QUARK_SE
	len = len > sizeof(u32_t) ? sizeof(u32_t) : len;
#endif

	err = usb_write(cdc_acm_ep_data[ACM_IN_EP_IDX].ep_addr,
			tx_data, len, &wrote);
	if (err != 0) {
		return err;
	}
	return wrote;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev     CDC ACM device struct.
 * @param rx_data Pointer to data container.
 * @param size    Container size.
 *
 * @return Number of bytes read.
 */
static int cdc_acm_fifo_read(struct device *dev, u8_t *rx_data,
			     const int size)
{
	u32_t avail_data, bytes_read, i;
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	avail_data = (CDC_ACM_BUFFER_SIZE + dev_data->rx_buf_head -
		      dev_data->rx_buf_tail) % CDC_ACM_BUFFER_SIZE;
	if (avail_data > size) {
		bytes_read = size;
	} else {
		bytes_read = avail_data;
	}

	for (i = 0; i < bytes_read; i++) {
		rx_data[i] = dev_data->rx_buf[(dev_data->rx_buf_tail + i) %
					      CDC_ACM_BUFFER_SIZE];
	}

	dev_data->rx_buf_tail = (dev_data->rx_buf_tail + bytes_read) %
		CDC_ACM_BUFFER_SIZE;

	if (dev_data->rx_buf_tail == dev_data->rx_buf_head) {
		/* Buffer empty */
		dev_data->rx_ready = 0;
	}

	return bytes_read;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev CDC ACM device struct.
 *
 * @return N/A.
 */
static void cdc_acm_irq_tx_enable(struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->tx_irq_ena = 1;
}

/**
 * @brief Disable TX interrupt
 *
 * @param dev CDC ACM device struct.
 *
 * @return N/A.
 */
static void cdc_acm_irq_tx_disable(struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->tx_irq_ena = 0;
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev CDC ACM device struct.
 *
 * @return 1 if a Tx IRQ is pending, 0 otherwise.
 */
static int cdc_acm_irq_tx_ready(struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->tx_ready) {
		dev_data->tx_ready = 0;
		return 1;
	}

	return 0;
}

/**
 * @brief Enable RX interrupt
 *
 * @param dev CDC ACM device struct.
 *
 * @return N/A
 */
static void cdc_acm_irq_rx_enable(struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->rx_irq_ena = 1;
}

/**
 * @brief Disable RX interrupt
 *
 * @param dev CDC ACM device struct.
 *
 * @return N/A.
 */
static void cdc_acm_irq_rx_disable(struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->rx_irq_ena = 0;
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev CDC ACM device struct.
 *
 * @return 1 if an IRQ is ready, 0 otherwise.
 */
static int cdc_acm_irq_rx_ready(struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->rx_ready) {
		dev_data->rx_ready = 0;
		return 1;
	}

	return 0;
}

/**
 * @brief Check if Tx or Rx IRQ is pending
 *
 * @param dev CDC ACM device struct.
 *
 * @return 1 if a Tx or Rx IRQ is pending, 0 otherwise.
 */
static int cdc_acm_irq_is_pending(struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->tx_ready && dev_data->tx_irq_ena) {
		return 1;
	} else if (dev_data->rx_ready && dev_data->rx_irq_ena) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Update IRQ status
 *
 * @param dev CDC ACM device struct.
 *
 * @return Always 1
 */
static int cdc_acm_irq_update(struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev CDC ACM device struct.
 * @param cb  Callback function pointer.
 *
 * @return N/A
 */
static void cdc_acm_irq_callback_set(struct device *dev,
				     uart_irq_callback_user_data_t cb,
				     void *cb_data)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

#ifdef CONFIG_UART_LINE_CTRL

/**
 * @brief Send serial line state notification to the Host
 *
 * This routine sends asynchronous notification of UART status
 * on the interrupt endpoint
 *
 * @param dev CDC ACM device struct.
 * @param ep_status Endpoint status code.
 *
 * @return  N/A.
 */
static int cdc_acm_send_notification(struct device *dev, u16_t serial_state)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);
	struct cdc_acm_notification notification;
	u32_t cnt = 0;

	notification.bmRequestType = 0xA1;
	notification.bNotificationType = 0x20;
	notification.wValue = 0;
	notification.wIndex = 0;
	notification.wLength = sys_cpu_to_le16(sizeof(serial_state));
	notification.data = sys_cpu_to_le16(serial_state);

	dev_data->notification_sent = 0;
	usb_write(cdc_acm_ep_data[ACM_INT_EP_IDX].ep_addr,
		  (const u8_t *)&notification, sizeof(notification), NULL);

	/* Wait for notification to be sent */
	while (!((volatile u8_t)dev_data->notification_sent)) {
		k_busy_wait(1);

		if (++cnt > CDC_CONTROL_SERIAL_STATE_TIMEOUT_US) {
			SYS_LOG_DBG("CDC ACM notification timeout!");
			return -EIO;
		}
	}

	return 0;
}

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev CDC ACM device struct
 * @param ctrl The line control to be manipulated
 * @param val Value to set the line control
 *
 * @return 0 if successful, failed otherwise.
 */
static int cdc_acm_line_ctrl_set(struct device *dev,
				 u32_t ctrl, u32_t val)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	switch (ctrl) {
	case LINE_CTRL_BAUD_RATE:
		cdc_acm_baudrate_set(dev, val);
		return 0;
	case LINE_CTRL_DCD:
		dev_data->serial_state &= ~SERIAL_STATE_RX_CARRIER;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_RX_CARRIER;
		}

		cdc_acm_send_notification(dev, SERIAL_STATE_RX_CARRIER);
		return 0;
	case LINE_CTRL_DSR:
		dev_data->serial_state &= ~SERIAL_STATE_TX_CARRIER;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_TX_CARRIER;
		}

		cdc_acm_send_notification(dev, dev_data->serial_state);
		return 0;
	default:
		return -ENODEV;
	}

	return -ENOTSUP;
}

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev CDC ACM device struct
 * @param ctrl The line control to be manipulated
 * @param val Value to set the line control
 *
 * @return 0 if successful, failed otherwise.
 */
static int cdc_acm_line_ctrl_get(struct device *dev,
				 u32_t ctrl, u32_t *val)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	switch (ctrl) {
	case LINE_CTRL_BAUD_RATE:
		*val = sys_le32_to_cpu(dev_data->line_coding.dwDTERate);
		return 0;
	case LINE_CTRL_RTS:
		*val = (dev_data->line_state &
			SET_CONTROL_LINE_STATE_RTS) ? 1 : 0;
		return 0;
	case LINE_CTRL_DTR:
		*val = (dev_data->line_state &
			SET_CONTROL_LINE_STATE_DTR) ? 1 : 0;
		return 0;
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_LINE_CTRL */

/*
 * @brief Poll the device for input.
 *
 * @return -ENOTSUP Since underlying USB device controller always uses
 * interrupts, polled mode UART APIs are not implemented for the UART interface
 * exported by CDC ACM driver. Apps should use fifo_read API instead.
 */

static int cdc_acm_poll_in(struct device *dev, unsigned char *c)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(c);

	return -ENOTSUP;
}

/*
 * @brief Output a character in polled mode.
 *
 * The UART poll method for USB UART is simulated by waiting till
 * we get the next BULK In upcall from the USB device controller or 100 ms.
 *
 * @return the same character which is sent
 */
static unsigned char cdc_acm_poll_out(struct device *dev,
				      unsigned char c)
{
	cdc_acm_fifo_fill(dev, &c, 1);
	k_sem_take(&poll_wait_sem, K_MSEC(100));

	return c;
}

static const struct uart_driver_api cdc_acm_driver_api = {
	.poll_in = cdc_acm_poll_in,
	.poll_out = cdc_acm_poll_out,
	.fifo_fill = cdc_acm_fifo_fill,
	.fifo_read = cdc_acm_fifo_read,
	.irq_tx_enable = cdc_acm_irq_tx_enable,
	.irq_tx_disable = cdc_acm_irq_tx_disable,
	.irq_tx_ready = cdc_acm_irq_tx_ready,
	.irq_rx_enable = cdc_acm_irq_rx_enable,
	.irq_rx_disable = cdc_acm_irq_rx_disable,
	.irq_rx_ready = cdc_acm_irq_rx_ready,
	.irq_is_pending = cdc_acm_irq_is_pending,
	.irq_update = cdc_acm_irq_update,
	.irq_callback_set = cdc_acm_irq_callback_set,
#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = cdc_acm_line_ctrl_set,
	.line_ctrl_get = cdc_acm_line_ctrl_get,
#endif /* CONFIG_UART_LINE_CTRL */
};

static struct cdc_acm_dev_data_t cdc_acm_dev_data = {
	.usb_status = USB_DC_UNKNOWN,
	.line_coding = CDC_ACM_DEFAUL_BAUDRATE,
};

DEVICE_AND_API_INIT(cdc_acm, CONFIG_CDC_ACM_PORT_NAME, &cdc_acm_init,
		    &cdc_acm_dev_data, NULL,
		    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &cdc_acm_driver_api);
