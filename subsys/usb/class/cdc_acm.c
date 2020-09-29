/*******************************************************************************
 *
 * Copyright(c) 2015-2019 Intel Corporation.
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
#include <drivers/uart/cdc_acm.h>
#include <drivers/uart.h>
#include <string.h>
#include <sys/ring_buffer.h>
#include <sys/byteorder.h>
#include <usb/class/usb_cdc.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb_descriptor.h>
#include <usb_work_q.h>

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
#error "CONFIG_UART_INTERRUPT_DRIVEN must be set for CDC ACM driver"
#endif

/* definitions */

#define LOG_LEVEL CONFIG_USB_CDC_ACM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_cdc_acm);

#define DEV_DATA(dev)						\
	((struct cdc_acm_dev_data_t * const)(dev)->data)

/* 115200bps, no parity, 1 stop bit, 8bit char */
#define CDC_ACM_DEFAULT_BAUDRATE {sys_cpu_to_le32(115200), 0, 0, 8}

/* Size of the internal buffer used for storing received data */
#define CDC_ACM_BUFFER_SIZE (CONFIG_CDC_ACM_BULK_EP_MPS)

/* Serial state notification timeout */
#define CDC_CONTROL_SERIAL_STATE_TIMEOUT_US 100000

#define ACM_INT_EP_IDX			0
#define ACM_OUT_EP_IDX			1
#define ACM_IN_EP_IDX			2

struct usb_cdc_acm_config {
#if (CONFIG_USB_COMPOSITE_DEVICE || CONFIG_CDC_ACM_IAD)
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

#define INITIALIZER_IAD							\
	{								\
		.bLength = sizeof(struct usb_association_descriptor),	\
		.bDescriptorType = USB_ASSOCIATION_DESC,		\
		.bFirstInterface = 0,					\
		.bInterfaceCount = 0x02,				\
		.bFunctionClass = COMMUNICATION_DEVICE_CLASS,		\
		.bFunctionSubClass = ACM_SUBCLASS,			\
		.bFunctionProtocol = 0,					\
		.iFunction = 0,						\
	}

#define INITIALIZER_IF(iface_num, num_ep, class, subclass)		\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_INTERFACE_DESC,			\
		.bInterfaceNumber = iface_num,				\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = num_ep,				\
		.bInterfaceClass = class,				\
		.bInterfaceSubClass = subclass,				\
		.bInterfaceProtocol = 0,				\
		.iInterface = 0,					\
	}

#define INITIALIZER_IF_HDR						\
	{								\
		.bFunctionLength = sizeof(struct cdc_header_descriptor),\
		.bDescriptorType = USB_CS_INTERFACE_DESC,		\
		.bDescriptorSubtype = HEADER_FUNC_DESC,			\
		.bcdCDC = sys_cpu_to_le16(USB_1_1),			\
	}

#define INITIALIZER_IF_CM						\
	{								\
		.bFunctionLength = sizeof(struct cdc_cm_descriptor),	\
		.bDescriptorType = USB_CS_INTERFACE_DESC,		\
		.bDescriptorSubtype = CALL_MANAGEMENT_FUNC_DESC,	\
		.bmCapabilities = 0x02,					\
		.bDataInterface = 1,					\
	}

/* Device supports the request combination of:
 *	Set_Line_Coding,
 *	Set_Control_Line_State,
 *	Get_Line_Coding
 *	and the notification Serial_State
 */
#define INITIALIZER_IF_ACM						\
	{								\
		.bFunctionLength = sizeof(struct cdc_acm_descriptor),	\
		.bDescriptorType = USB_CS_INTERFACE_DESC,		\
		.bDescriptorSubtype = ACM_FUNC_DESC,			\
		.bmCapabilities = 0x02,					\
	}

#define INITIALIZER_IF_UNION						\
	{								\
		.bFunctionLength = sizeof(struct cdc_union_descriptor),	\
		.bDescriptorType = USB_CS_INTERFACE_DESC,		\
		.bDescriptorSubtype = UNION_FUNC_DESC,			\
		.bControlInterface = 0,					\
		.bSubordinateInterface0 = 1,				\
	}

#define INITIALIZER_IF_EP(addr, attr, mps, interval)			\
	{								\
		.bLength = sizeof(struct usb_ep_descriptor),		\
		.bDescriptorType = USB_ENDPOINT_DESC,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = interval,					\
	}

/* Device data structure */
struct cdc_acm_dev_data_t {
	/* Callback function pointer/arg */
	uart_irq_callback_user_data_t cb;
	void *cb_data;
	struct k_sem poll_wait_sem;
	struct k_work cb_work;
#if defined(CONFIG_CDC_ACM_DTE_RATE_CALLBACK_SUPPORT)
	cdc_dte_rate_callback_t rate_cb;
#endif
	struct k_work tx_work;
	/* Tx ready status. Signals when */
	bool tx_ready;
	bool rx_ready;				/* Rx ready status */
	bool tx_irq_ena;			/* Tx interrupt enable status */
	bool rx_irq_ena;			/* Rx interrupt enable status */
	uint8_t rx_buf[CDC_ACM_BUFFER_SIZE];	/* Internal RX buffer */
	struct ring_buf *rx_ringbuf;
	struct ring_buf *tx_ringbuf;
	/* Interface data buffer */
	/* CDC ACM line coding properties. LE order */
	struct cdc_acm_line_coding line_coding;
	/* CDC ACM line state bitmap, DTE side */
	uint8_t line_state;
	/* CDC ACM serial state bitmap, DCE side */
	uint8_t serial_state;
	/* CDC ACM notification sent status */
	uint8_t notification_sent;
	/* CDC ACM configured flag */
	bool configured;
	/* CDC ACM suspended flag */
	bool suspended;

	struct usb_dev_data common;
};

static sys_slist_t cdc_acm_data_devlist;
static const struct uart_driver_api cdc_acm_driver_api;

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
			     int32_t *len, uint8_t **data)
{
	struct cdc_acm_dev_data_t *dev_data;
	struct usb_dev_data *common;
	uint32_t rate;
	uint32_t new_rate;

	common = usb_get_dev_data_by_iface(&cdc_acm_data_devlist,
					   (uint8_t)pSetup->wIndex);
	if (common == NULL) {
		LOG_WRN("Device data not found for interface %u",
			pSetup->wIndex);
		return -ENODEV;
	}

	dev_data = CONTAINER_OF(common, struct cdc_acm_dev_data_t, common);

	switch (pSetup->bRequest) {
	case SET_LINE_CODING:
		rate = sys_le32_to_cpu(dev_data->line_coding.dwDTERate);
		memcpy(&dev_data->line_coding,
		       *data, sizeof(dev_data->line_coding));
		new_rate = sys_le32_to_cpu(dev_data->line_coding.dwDTERate);
		LOG_DBG("CDC_SET_LINE_CODING %d %d %d %d",
			new_rate,
			dev_data->line_coding.bCharFormat,
			dev_data->line_coding.bParityType,
			dev_data->line_coding.bDataBits);
#if defined(CONFIG_CDC_ACM_DTE_RATE_CALLBACK_SUPPORT)
		if (rate != new_rate && dev_data->rate_cb != NULL) {
			dev_data->rate_cb(common->dev, new_rate);
		}
#endif
		break;

	case SET_CONTROL_LINE_STATE:
		dev_data->line_state = (uint8_t)pSetup->wValue;
		LOG_DBG("CDC_SET_CONTROL_LINE_STATE 0x%x",
			dev_data->line_state);
		break;

	case GET_LINE_CODING:
		*data = (uint8_t *)(&dev_data->line_coding);
		*len = sizeof(dev_data->line_coding);
		LOG_DBG("CDC_GET_LINE_CODING %d %d %d %d",
			sys_le32_to_cpu(dev_data->line_coding.dwDTERate),
			dev_data->line_coding.bCharFormat,
			dev_data->line_coding.bParityType,
			dev_data->line_coding.bDataBits);
		break;

	default:
		LOG_DBG("CDC ACM request 0x%x, value 0x%x",
			pSetup->bRequest, pSetup->wValue);
		return -EINVAL;
	}

	return 0;
}

static void cdc_acm_write_cb(uint8_t ep, int size, void *priv)
{
	struct cdc_acm_dev_data_t *dev_data = priv;

	LOG_DBG("ep %x: written %d bytes dev_data %p", ep, size, dev_data);

	dev_data->tx_ready = true;

	k_sem_give(&dev_data->poll_wait_sem);

	/* Call callback only if tx irq ena */
	if (dev_data->cb && dev_data->tx_irq_ena) {
		k_work_submit_to_queue(&USB_WORK_Q, &dev_data->cb_work);
	}

	if (ring_buf_is_empty(dev_data->tx_ringbuf)) {
		LOG_DBG("tx_ringbuf is empty");
		return;
	}

	k_work_submit_to_queue(&USB_WORK_Q, &dev_data->tx_work);
}

static void tx_work_handler(struct k_work *work)
{
	struct cdc_acm_dev_data_t *dev_data =
		CONTAINER_OF(work, struct cdc_acm_dev_data_t, tx_work);
	const struct device *dev = dev_data->common.dev;
	struct usb_cfg_data *cfg = (void *)dev->config;
	uint8_t ep = cfg->endpoint[ACM_IN_EP_IDX].ep_addr;
	uint8_t *data;
	size_t len;

	if (usb_transfer_is_busy(ep)) {
		LOG_DBG("Transfer is ongoing");
		return;
	}

	len = ring_buf_get_claim(dev_data->tx_ringbuf, &data,
				 CONFIG_USB_CDC_ACM_RINGBUF_SIZE);

	if (!len) {
		LOG_DBG("Nothing to send");
		return;
	}

	/*
	 * Transfer less data to avoid zero-length packet. The application
	 * running on the host may conclude that there is no more data to be
	 * received (i.e. the transaction has completed), hence not triggering
	 * another I/O Request Packet (IRP).
	 */
	if (!(len % CONFIG_CDC_ACM_BULK_EP_MPS)) {
		len -= 1;
	}

	LOG_DBG("Got %zd bytes from ringbuffer send to ep %x", len, ep);

	usb_transfer(ep, data, len, USB_TRANS_WRITE,
		     cdc_acm_write_cb, dev_data);

	ring_buf_get_finish(dev_data->tx_ringbuf, len);
}

static void cdc_acm_read_cb(uint8_t ep, int size, void *priv)
{
	struct cdc_acm_dev_data_t *dev_data = priv;
	size_t wrote;

	LOG_DBG("ep %x size %d dev_data %p rx_ringbuf space %u",
		ep, size, dev_data, ring_buf_space_get(dev_data->rx_ringbuf));

	if (size <= 0) {
		goto done;
	}

	wrote = ring_buf_put(dev_data->rx_ringbuf, dev_data->rx_buf, size);
	if (wrote < size) {
		LOG_ERR("Ring buffer full, drop %zd bytes", size - wrote);
	}

done:
	dev_data->rx_ready = true;

	/* Call callback only if rx irq ena */
	if (dev_data->cb && dev_data->rx_irq_ena) {
		k_work_submit_to_queue(&USB_WORK_Q, &dev_data->cb_work);
	}

	usb_transfer(ep, dev_data->rx_buf, sizeof(dev_data->rx_buf),
		     USB_TRANS_READ, cdc_acm_read_cb, dev_data);

}

/**
 * @brief EP Interrupt handler
 *
 * @param ep        Endpoint address.
 * @param ep_status Endpoint status code.
 *
 * @return  N/A.
 */
static void cdc_acm_int_in(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	struct cdc_acm_dev_data_t *dev_data;
	struct usb_dev_data *common;

	ARG_UNUSED(ep_status);

	common = usb_get_dev_data_by_ep(&cdc_acm_data_devlist, ep);
	if (common == NULL) {
		LOG_WRN("Device data not found for endpoint %u", ep);
		return;
	}

	dev_data = CONTAINER_OF(common, struct cdc_acm_dev_data_t, common);

	dev_data->notification_sent = 1U;
	LOG_DBG("CDC_IntIN EP[%x]\r", ep);
}

static void cdc_acm_reset_port(struct cdc_acm_dev_data_t *dev_data)
{
	k_sem_give(&dev_data->poll_wait_sem);
	dev_data->configured = false;
	dev_data->suspended = false;
	dev_data->rx_ready = false;
	dev_data->tx_ready = false;
	dev_data->line_coding = (struct cdc_acm_line_coding)
				CDC_ACM_DEFAULT_BAUDRATE;
	dev_data->serial_state = 0;
	dev_data->line_state = 0;
	memset(&dev_data->rx_buf, 0, CDC_ACM_BUFFER_SIZE);
}

static void cdc_acm_do_cb(struct cdc_acm_dev_data_t *dev_data,
			  enum usb_dc_status_code status,
			  const uint8_t *param)
{
	const struct device *dev = dev_data->common.dev;
	struct usb_cfg_data *cfg = (void *)dev->config;

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("Device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("Device reset detected");
		cdc_acm_reset_port(dev_data);
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("Device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_INF("Device configured");
		if (!dev_data->configured) {
			cdc_acm_read_cb(cfg->endpoint[ACM_OUT_EP_IDX].ep_addr, 0,
					dev_data);
		}
		dev_data->configured = true;
		dev_data->tx_ready = true;
		break;
	case USB_DC_DISCONNECTED:
		LOG_INF("Device disconnected");
		cdc_acm_reset_port(dev_data);
		break;
	case USB_DC_SUSPEND:
		LOG_INF("Device suspended");
		dev_data->suspended = true;
		break;
	case USB_DC_RESUME:
		LOG_INF("Device resumed");
		if (dev_data->suspended) {
			LOG_INF("from suspend");
			dev_data->suspended = false;
			if (dev_data->configured) {
				cdc_acm_read_cb(cfg->endpoint[ACM_OUT_EP_IDX].ep_addr,
					0, dev_data);
			}
		} else {
			LOG_DBG("Spurious resume event");
		}
		break;
	case USB_DC_SOF:
	case USB_DC_INTERFACE:
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("Unknown event");
		break;
	}
}

static void cdc_acm_dev_status_cb(struct usb_cfg_data *cfg,
				  enum usb_dc_status_code status,
				  const uint8_t *param)
{
	struct cdc_acm_dev_data_t *dev_data;
	struct usb_dev_data *common;

	LOG_DBG("cfg %p status %d", cfg, status);

	common = usb_get_dev_data_by_cfg(&cdc_acm_data_devlist, cfg);
	if (common == NULL) {
		LOG_WRN("Device data not found for cfg %p", cfg);
		return;
	}

	dev_data = CONTAINER_OF(common, struct cdc_acm_dev_data_t, common);

	cdc_acm_do_cb(dev_data, status, param);
}

static void cdc_interface_config(struct usb_desc_header *head,
				 uint8_t bInterfaceNumber)
{
	struct usb_if_descriptor *if_desc = (struct usb_if_descriptor *) head;
	struct usb_cdc_acm_config *desc =
		CONTAINER_OF(if_desc, struct usb_cdc_acm_config, if0);

	desc->if0.bInterfaceNumber = bInterfaceNumber;
	desc->if0_union.bControlInterface = bInterfaceNumber;
	desc->if1.bInterfaceNumber = bInterfaceNumber + 1;
	desc->if0_union.bSubordinateInterface0 = bInterfaceNumber + 1;
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	desc->iad_cdc.bFirstInterface = bInterfaceNumber;
#endif
}

/**
 * @brief Call the IRQ function callback.
 *
 * This routine is called from the system work queue to signal an UART
 * IRQ.
 *
 * @param work Address of work item.
 *
 * @return N/A.
 */
static void cdc_acm_irq_callback_work_handler(struct k_work *work)
{
	struct cdc_acm_dev_data_t *dev_data;

	dev_data = CONTAINER_OF(work, struct cdc_acm_dev_data_t, cb_work);

	dev_data->cb(dev_data->common.dev, dev_data->cb_data);
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
static int cdc_acm_init(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);
	int ret = 0;

	dev_data->common.dev = dev;
	sys_slist_append(&cdc_acm_data_devlist, &dev_data->common.node);

	LOG_DBG("Device dev %p dev_data %p cfg %p added to devlist %p",
		dev, dev_data, dev->config, &cdc_acm_data_devlist);

	k_sem_init(&dev_data->poll_wait_sem, 0, UINT_MAX);
	k_work_init(&dev_data->cb_work, cdc_acm_irq_callback_work_handler);
	k_work_init(&dev_data->tx_work, tx_work_handler);

	return ret;
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
static int cdc_acm_fifo_fill(const struct device *dev,
			     const uint8_t *tx_data, int len)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);
	size_t wrote;

	LOG_DBG("dev_data %p len %d tx_ringbuf space %u",
		dev_data, len, ring_buf_space_get(dev_data->tx_ringbuf));

	if (!dev_data->configured || dev_data->suspended) {
		LOG_WRN("Device not configured or suspended, drop %d bytes",
			len);
		return 0;
	}

	dev_data->tx_ready = false;

	wrote = ring_buf_put(dev_data->tx_ringbuf, tx_data, len);
	if (wrote < len) {
		LOG_WRN("Ring buffer full, drop %zd bytes", len - wrote);
	}

	k_work_submit_to_queue(&USB_WORK_Q, &dev_data->tx_work);

	/* Return written to ringbuf data len */
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
static int cdc_acm_fifo_read(const struct device *dev, uint8_t *rx_data,
			     const int size)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);
	uint32_t len;

	LOG_DBG("dev %p size %d rx_ringbuf space %u",
		dev, size, ring_buf_space_get(dev_data->rx_ringbuf));

	len = ring_buf_get(dev_data->rx_ringbuf, rx_data, size);

	if (ring_buf_is_empty(dev_data->rx_ringbuf)) {
		dev_data->rx_ready = false;
	}

	return len;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev CDC ACM device struct.
 *
 * @return N/A.
 */
static void cdc_acm_irq_tx_enable(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->tx_irq_ena = true;

	if (dev_data->cb && dev_data->tx_ready) {
		k_work_submit_to_queue(&USB_WORK_Q, &dev_data->cb_work);
	}
}

/**
 * @brief Disable TX interrupt
 *
 * @param dev CDC ACM device struct.
 *
 * @return N/A.
 */
static void cdc_acm_irq_tx_disable(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->tx_irq_ena = false;
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev CDC ACM device struct.
 *
 * @return 1 if a Tx IRQ is pending, 0 otherwise.
 */
static int cdc_acm_irq_tx_ready(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->tx_ready) {
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
static void cdc_acm_irq_rx_enable(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->rx_irq_ena = true;

	if (dev_data->cb && dev_data->rx_ready) {
		k_work_submit_to_queue(&USB_WORK_Q, &dev_data->cb_work);
	}
}

/**
 * @brief Disable RX interrupt
 *
 * @param dev CDC ACM device struct.
 *
 * @return N/A.
 */
static void cdc_acm_irq_rx_disable(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->rx_irq_ena = false;
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev CDC ACM device struct.
 *
 * @return 1 if an IRQ is ready, 0 otherwise.
 */
static int cdc_acm_irq_rx_ready(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->rx_ready) {
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
static int cdc_acm_irq_is_pending(const struct device *dev)
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
static int cdc_acm_irq_update(const struct device *dev)
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
static void cdc_acm_irq_callback_set(const struct device *dev,
				     uart_irq_callback_user_data_t cb,
				     void *cb_data)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

#if defined(CONFIG_CDC_ACM_DTE_RATE_CALLBACK_SUPPORT)
int cdc_acm_dte_rate_callback_set(const struct device *dev,
				  cdc_dte_rate_callback_t callback)
{
	struct cdc_acm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev->api != &cdc_acm_driver_api) {
		return -EINVAL;
	}

	dev_data->rate_cb = callback;

	return 0;
}
#endif

#ifdef CONFIG_UART_LINE_CTRL

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
static void cdc_acm_baudrate_set(const struct device *dev, uint32_t baudrate)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->line_coding.dwDTERate = sys_cpu_to_le32(baudrate);
}

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
static int cdc_acm_send_notification(const struct device *dev,
				     uint16_t serial_state)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);
	struct usb_cfg_data * const cfg = (void *)dev->config;
	struct cdc_acm_notification notification;
	uint32_t cnt = 0U;

	notification.bmRequestType = 0xA1;
	notification.bNotificationType = 0x20;
	notification.wValue = 0U;
	notification.wIndex = 0U;
	notification.wLength = sys_cpu_to_le16(sizeof(serial_state));
	notification.data = sys_cpu_to_le16(serial_state);

	dev_data->notification_sent = 0U;

	usb_write(cfg->endpoint[ACM_INT_EP_IDX].ep_addr,
		  (const uint8_t *)&notification, sizeof(notification), NULL);

	/* Wait for notification to be sent */
	while (!((volatile uint8_t)dev_data->notification_sent)) {
		k_busy_wait(1);

		if (++cnt > CDC_CONTROL_SERIAL_STATE_TIMEOUT_US) {
			LOG_DBG("CDC ACM notification timeout!");
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
static int cdc_acm_line_ctrl_set(const struct device *dev,
				 uint32_t ctrl, uint32_t val)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	switch (ctrl) {
	case USB_CDC_LINE_CTRL_BAUD_RATE:
		cdc_acm_baudrate_set(dev, val);
		return 0;
	case USB_CDC_LINE_CTRL_DCD:
		dev_data->serial_state &= ~SERIAL_STATE_RX_CARRIER;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_RX_CARRIER;
		}
		cdc_acm_send_notification(dev, SERIAL_STATE_RX_CARRIER);
		return 0;
	case USB_CDC_LINE_CTRL_DSR:
		dev_data->serial_state &= ~SERIAL_STATE_TX_CARRIER;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_TX_CARRIER;
		}
		cdc_acm_send_notification(dev, dev_data->serial_state);
		return 0;
	case USB_CDC_LINE_CTRL_BREAK:
		dev_data->serial_state &= ~SERIAL_STATE_BREAK;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_BREAK;
		}
		cdc_acm_send_notification(dev, dev_data->serial_state);
		return 0;
	case USB_CDC_LINE_CTRL_RING_SIGNAL:
		dev_data->serial_state &= ~SERIAL_STATE_RING_SIGNAL;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_RING_SIGNAL;
		}
		cdc_acm_send_notification(dev, dev_data->serial_state);
		return 0;
	case USB_CDC_LINE_CTRL_FRAMING:
		dev_data->serial_state &= ~SERIAL_STATE_FRAMING;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_FRAMING;
		}
		cdc_acm_send_notification(dev, dev_data->serial_state);
		return 0;
	case USB_CDC_LINE_CTRL_PARITY:
		dev_data->serial_state &= ~SERIAL_STATE_PARITY;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_PARITY;
		}
		cdc_acm_send_notification(dev, dev_data->serial_state);
		return 0;
	case USB_CDC_LINE_CTRL_OVER_RUN:
		dev_data->serial_state &= ~SERIAL_STATE_OVER_RUN;

		if (val) {
			dev_data->serial_state |= SERIAL_STATE_OVER_RUN;
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
static int cdc_acm_line_ctrl_get(const struct device *dev,
				 uint32_t ctrl, uint32_t *val)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		*val = sys_le32_to_cpu(dev_data->line_coding.dwDTERate);
		return 0;
	case UART_LINE_CTRL_RTS:
		*val = (dev_data->line_state &
			SET_CONTROL_LINE_STATE_RTS) ? 1 : 0;
		return 0;
	case UART_LINE_CTRL_DTR:
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

static int cdc_acm_poll_in(const struct device *dev, unsigned char *c)
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
 */
static void cdc_acm_poll_out(const struct device *dev,
				      unsigned char c)
{
	struct cdc_acm_dev_data_t * const dev_data = DEV_DATA(dev);

	cdc_acm_fifo_fill(dev, &c, 1);
	k_sem_take(&dev_data->poll_wait_sem, K_MSEC(100));
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

#define INITIALIZER_EP_DATA(cb, addr)					\
	{								\
		.ep_cb = cb,						\
		.ep_addr = addr,					\
	}

#define DEFINE_CDC_ACM_EP(x, int_ep_addr, out_ep_addr, in_ep_addr)	\
	static struct usb_ep_cfg_data cdc_acm_ep_data_##x[] = {		\
		INITIALIZER_EP_DATA(cdc_acm_int_in, int_ep_addr),	\
		INITIALIZER_EP_DATA(usb_transfer_ep_callback,		\
				    out_ep_addr),			\
		INITIALIZER_EP_DATA(usb_transfer_ep_callback,		\
				    in_ep_addr),			\
	}

#define DEFINE_CDC_ACM_CFG_DATA(x, _)					\
	USBD_CFG_DATA_DEFINE(primary, cdc_acm)				\
	struct usb_cfg_data cdc_acm_config_##x = {			\
		.usb_device_description = NULL,				\
		.interface_config = cdc_interface_config,		\
		.interface_descriptor = &cdc_acm_cfg_##x.if0,		\
		.cb_usb_status = cdc_acm_dev_status_cb,			\
		.interface = {						\
			.class_handler = cdc_acm_class_handle_req,	\
			.custom_handler = NULL,				\
		},							\
		.num_endpoints = ARRAY_SIZE(cdc_acm_ep_data_##x),	\
		.endpoint = cdc_acm_ep_data_##x,			\
	};

#if (CONFIG_USB_COMPOSITE_DEVICE || CONFIG_CDC_ACM_IAD)
#define DEFINE_CDC_ACM_DESCR(x, int_ep_addr, out_ep_addr, in_ep_addr)	\
	USBD_CLASS_DESCR_DEFINE(primary, x)				\
	struct usb_cdc_acm_config cdc_acm_cfg_##x = {			\
	.iad_cdc = INITIALIZER_IAD,					\
	.if0 = INITIALIZER_IF(0, 1, COMMUNICATION_DEVICE_CLASS,		\
			      ACM_SUBCLASS),				\
	.if0_header = INITIALIZER_IF_HDR,				\
	.if0_cm = INITIALIZER_IF_CM,					\
	.if0_acm = INITIALIZER_IF_ACM,					\
	.if0_union = INITIALIZER_IF_UNION,				\
	.if0_int_ep = INITIALIZER_IF_EP(int_ep_addr,			\
					USB_DC_EP_INTERRUPT,		\
					CONFIG_CDC_ACM_INTERRUPT_EP_MPS,\
					0x0A),				\
	.if1 = INITIALIZER_IF(1, 2, COMMUNICATION_DEVICE_CLASS_DATA, 0),\
	.if1_in_ep = INITIALIZER_IF_EP(in_ep_addr,			\
				       USB_DC_EP_BULK,			\
				       CONFIG_CDC_ACM_BULK_EP_MPS,	\
				       0x00),				\
	.if1_out_ep = INITIALIZER_IF_EP(out_ep_addr,			\
					USB_DC_EP_BULK,			\
					CONFIG_CDC_ACM_BULK_EP_MPS,	\
					0x00),				\
}
#else /* (CONFIG_USB_COMPOSITE_DEVICE || CONFIG_CDC_ACM_IAD) */
#define DEFINE_CDC_ACM_DESCR(x, int_ep_addr, out_ep_addr, in_ep_addr)	\
	USBD_CLASS_DESCR_DEFINE(primary, x)				\
	struct usb_cdc_acm_config cdc_acm_cfg_##x = {			\
	.if0 = INITIALIZER_IF(0, 1, COMMUNICATION_DEVICE_CLASS,		\
			      ACM_SUBCLASS),				\
	.if0_header = INITIALIZER_IF_HDR,				\
	.if0_cm = INITIALIZER_IF_CM,					\
	.if0_acm = INITIALIZER_IF_ACM,					\
	.if0_union = INITIALIZER_IF_UNION,				\
	.if0_int_ep = INITIALIZER_IF_EP(int_ep_addr,			\
					USB_DC_EP_INTERRUPT,		\
					CONFIG_CDC_ACM_INTERRUPT_EP_MPS,\
					0x0A),				\
	.if1 = INITIALIZER_IF(1, 2, COMMUNICATION_DEVICE_CLASS_DATA, 0),\
	.if1_in_ep = INITIALIZER_IF_EP(in_ep_addr,			\
				       USB_DC_EP_BULK,			\
				       CONFIG_CDC_ACM_BULK_EP_MPS,	\
				       0x00),				\
	.if1_out_ep = INITIALIZER_IF_EP(out_ep_addr,			\
					USB_DC_EP_BULK,			\
					CONFIG_CDC_ACM_BULK_EP_MPS,	\
					0x00),				\
}
#endif /* (CONFIG_USB_COMPOSITE_DEVICE || CONFIG_CDC_ACM_IAD) */

#define DEFINE_CDC_ACM_DEV_DATA(x, _)					\
	RING_BUF_DECLARE(rx_ringbuf_##x,				\
			 CONFIG_USB_CDC_ACM_RINGBUF_SIZE);		\
	RING_BUF_DECLARE(tx_ringbuf_##x,				\
			 CONFIG_USB_CDC_ACM_RINGBUF_SIZE);		\
	static struct cdc_acm_dev_data_t cdc_acm_dev_data_##x = {	\
		.line_coding = CDC_ACM_DEFAULT_BAUDRATE,		\
		.rx_ringbuf = &rx_ringbuf_##x,				\
		.tx_ringbuf = &tx_ringbuf_##x,				\
	};

#define DEFINE_CDC_ACM_DEVICE(x, _)					\
	DEVICE_AND_API_INIT(cdc_acm_##x,				\
			    CONFIG_USB_CDC_ACM_DEVICE_NAME "_" #x,	\
			    &cdc_acm_init, &cdc_acm_dev_data_##x,	\
			    &cdc_acm_config_##x,			\
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &cdc_acm_driver_api);

#define DEFINE_CDC_ACM_DESCR_AUTO(x, _) \
	DEFINE_CDC_ACM_DESCR(x, AUTO_EP_IN, AUTO_EP_OUT, AUTO_EP_IN);

#define DEFINE_CDC_ACM_EP_AUTO(x, _) \
	DEFINE_CDC_ACM_EP(x, AUTO_EP_IN, AUTO_EP_OUT, AUTO_EP_IN);

UTIL_LISTIFY(CONFIG_USB_CDC_ACM_DEVICE_COUNT, DEFINE_CDC_ACM_DESCR_AUTO, _)
UTIL_LISTIFY(CONFIG_USB_CDC_ACM_DEVICE_COUNT, DEFINE_CDC_ACM_EP_AUTO, _)
UTIL_LISTIFY(CONFIG_USB_CDC_ACM_DEVICE_COUNT, DEFINE_CDC_ACM_CFG_DATA, _)
UTIL_LISTIFY(CONFIG_USB_CDC_ACM_DEVICE_COUNT, DEFINE_CDC_ACM_DEV_DATA, _)
UTIL_LISTIFY(CONFIG_USB_CDC_ACM_DEVICE_COUNT, DEFINE_CDC_ACM_DEVICE, _)
