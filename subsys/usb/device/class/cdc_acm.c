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

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart/cdc_acm.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>
#include <usb_work_q.h>

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
#error "CONFIG_UART_INTERRUPT_DRIVEN must be set for CDC ACM driver"
#endif

/* definitions */

#include <zephyr/logging/log.h>
/* Prevent endless recursive logging loop and warn user about it */
#if defined(CONFIG_USB_CDC_ACM_LOG_LEVEL) && CONFIG_USB_CDC_ACM_LOG_LEVEL != LOG_LEVEL_NONE
#define CHOSEN_CONSOLE DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart)
#define CHOSEN_SHELL   DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
#if (CHOSEN_CONSOLE && defined(CONFIG_LOG_BACKEND_UART)) || \
	(CHOSEN_SHELL && defined(CONFIG_SHELL_LOG_BACKEND))
#warning "USB_CDC_ACM_LOG_LEVEL forced to LOG_LEVEL_NONE"
#undef CONFIG_USB_CDC_ACM_LOG_LEVEL
#define CONFIG_USB_CDC_ACM_LOG_LEVEL LOG_LEVEL_NONE
#endif
#endif
LOG_MODULE_REGISTER(usb_cdc_acm, CONFIG_USB_CDC_ACM_LOG_LEVEL);

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
	struct usb_association_descriptor iad_cdc;
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

/* Device data structure */
struct cdc_acm_dev_data_t {
	/* Callback function pointer/arg */
	uart_irq_callback_user_data_t cb;
	void *cb_data;
	struct k_work cb_work;
#if defined(CONFIG_CDC_ACM_DTE_RATE_CALLBACK_SUPPORT)
	cdc_dte_rate_callback_t rate_cb;
#endif
	struct k_work_delayable tx_work;
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
	/* CDC ACM paused flag */
	bool rx_paused;
	/* When flow_ctrl is set, poll out is blocked when the buffer is full,
	 * roughly emulating flow control.
	 */
	bool flow_ctrl;

	struct usb_dev_data common;
};

static sys_slist_t cdc_acm_data_devlist;
static DEVICE_API(uart, cdc_acm_driver_api);

/**
 * @brief Handler called for Class requests not handled by the USB stack.
 *
 * @param setup     Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int cdc_acm_class_handle_req(struct usb_setup_packet *setup,
			     int32_t *len, uint8_t **data)
{
	struct cdc_acm_dev_data_t *dev_data;
	struct usb_dev_data *common;
	uint32_t rate;
	uint32_t new_rate;

	common = usb_get_dev_data_by_iface(&cdc_acm_data_devlist,
					   (uint8_t)setup->wIndex);
	if (common == NULL) {
		LOG_WRN("Device data not found for interface %u",
			setup->wIndex);
		return -ENODEV;
	}

	dev_data = CONTAINER_OF(common, struct cdc_acm_dev_data_t, common);

	if (usb_reqtype_is_to_device(setup)) {
		switch (setup->bRequest) {
		case SET_LINE_CODING:
			rate = sys_le32_to_cpu(dev_data->line_coding.dwDTERate);
			memcpy(&dev_data->line_coding, *data,
			       sizeof(dev_data->line_coding));
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
			return 0;

		case SET_CONTROL_LINE_STATE:
			dev_data->line_state = (uint8_t)setup->wValue;
			LOG_DBG("CDC_SET_CONTROL_LINE_STATE 0x%x",
				dev_data->line_state);
			return 0;

		default:
			break;
		}
	} else {
		if (setup->bRequest == GET_LINE_CODING) {
			*data = (uint8_t *)(&dev_data->line_coding);
			*len = sizeof(dev_data->line_coding);
			LOG_DBG("CDC_GET_LINE_CODING %d %d %d %d",
				sys_le32_to_cpu(dev_data->line_coding.dwDTERate),
				dev_data->line_coding.bCharFormat,
				dev_data->line_coding.bParityType,
				dev_data->line_coding.bDataBits);
			return 0;
		}
	}

	LOG_DBG("CDC ACM bmRequestType 0x%02x bRequest 0x%02x unsupported",
		setup->bmRequestType, setup->bRequest);
	return -ENOTSUP;
}

static void cdc_acm_write_cb(uint8_t ep, int size, void *priv)
{
	struct cdc_acm_dev_data_t *dev_data = priv;

	LOG_DBG("ep %x: written %d bytes dev_data %p", ep, size, dev_data);

	dev_data->tx_ready = true;

	/* Call callback only if tx irq ena */
	if (dev_data->cb && dev_data->tx_irq_ena) {
		k_work_submit_to_queue(&USB_WORK_Q, &dev_data->cb_work);
	}

	/* If size is 0, we want to schedule tx work even if ringbuf is empty to
	 * ensure that actual payload will not be sent before initialization
	 * timeout passes.
	 */
	if (ring_buf_is_empty(dev_data->tx_ringbuf) && size) {
		LOG_DBG("tx_ringbuf is empty");
		return;
	}

	/* If size is 0, it means that host started polling IN data because it
	 * has read the ZLP we armed when interface was configured. This ZLP is
	 * probably the best indication that host has started to read the data.
	 * Wait initialization timeout before sending actual payload to make it
	 * possible for application to disable ECHO. The echo is long known
	 * problem related to the fact that POSIX defaults to ECHO ON and thus
	 * every application that opens tty device (on Linux) will have ECHO
	 * enabled in the short window between open() and ioctl() that disables
	 * the echo (if application wishes to disable the echo).
	 */
	k_work_schedule_for_queue(&USB_WORK_Q, &dev_data->tx_work, size ?
				  K_NO_WAIT : K_MSEC(CONFIG_CDC_ACM_TX_DELAY_MS));
}

static void tx_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct cdc_acm_dev_data_t *dev_data =
		CONTAINER_OF(dwork, struct cdc_acm_dev_data_t, tx_work);
	const struct device *dev = dev_data->common.dev;
	struct usb_cfg_data *cfg = (void *)dev->config;
	uint8_t ep = cfg->endpoint[ACM_IN_EP_IDX].ep_addr;
	uint8_t *data;
	size_t len;

	if (usb_transfer_is_busy(ep)) {
		LOG_DBG("Transfer is ongoing");
		return;
	}

	if (!dev_data->configured) {
		return;
	}

	len = ring_buf_get_claim(dev_data->tx_ringbuf, &data,
				 CONFIG_USB_CDC_ACM_RINGBUF_SIZE);

	if (!len) {
		LOG_DBG("Nothing to send");
		return;
	}

	dev_data->tx_ready = false;

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

	dev_data->rx_ready = true;

	/* Call callback only if rx irq ena */
	if (dev_data->cb && dev_data->rx_irq_ena) {
		k_work_submit_to_queue(&USB_WORK_Q, &dev_data->cb_work);
	}

	if (ring_buf_space_get(dev_data->rx_ringbuf) < sizeof(dev_data->rx_buf)) {
		dev_data->rx_paused = true;
		return;
	}

done:
	if (dev_data->configured) {
		usb_transfer(ep, dev_data->rx_buf, sizeof(dev_data->rx_buf),
			     USB_TRANS_READ, cdc_acm_read_cb, dev_data);
	}
}

/**
 * @brief EP Interrupt handler
 *
 * @param ep        Endpoint address.
 * @param ep_status Endpoint status code.
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
	dev_data->configured = false;
	dev_data->suspended = false;
	dev_data->rx_ready = false;
	dev_data->tx_ready = false;
	dev_data->line_coding = (struct cdc_acm_line_coding)
				CDC_ACM_DEFAULT_BAUDRATE;
	dev_data->serial_state = 0;
	dev_data->line_state = 0;
	dev_data->rx_paused = false;
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
			dev_data->configured = true;
			cdc_acm_read_cb(cfg->endpoint[ACM_OUT_EP_IDX].ep_addr, 0,
					dev_data);
			/* Queue ZLP on IN endpoint so we know when host starts polling */
			if (!dev_data->tx_ready) {
				usb_transfer(cfg->endpoint[ACM_IN_EP_IDX].ep_addr, NULL, 0,
					     USB_TRANS_WRITE, cdc_acm_write_cb, dev_data);
			}
		}
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
	desc->iad_cdc.bFirstInterface = bInterfaceNumber;
}

/**
 * @brief Call the IRQ function callback.
 *
 * This routine is called from the system work queue to signal an UART
 * IRQ.
 *
 * @param work Address of work item.
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
	struct cdc_acm_dev_data_t * const dev_data = dev->data;
	int ret = 0;

	dev_data->common.dev = dev;
	sys_slist_append(&cdc_acm_data_devlist, &dev_data->common.node);

	LOG_DBG("Device dev %p dev_data %p cfg %p added to devlist %p",
		dev, dev_data, dev->config, &cdc_acm_data_devlist);

	k_work_init(&dev_data->cb_work, cdc_acm_irq_callback_work_handler);
	k_work_init_delayable(&dev_data->tx_work, tx_work_handler);

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
	struct cdc_acm_dev_data_t * const dev_data = dev->data;
	unsigned int lock;
	size_t wrote;

	LOG_DBG("dev_data %p len %d tx_ringbuf space %u",
		dev_data, len, ring_buf_space_get(dev_data->tx_ringbuf));

	lock = irq_lock();
	wrote = ring_buf_put(dev_data->tx_ringbuf, tx_data, len);
	irq_unlock(lock);
	LOG_DBG("Wrote %zu of %d bytes to TX ringbuffer", wrote, len);

	if (wrote) {
		k_work_schedule_for_queue(&USB_WORK_Q, &dev_data->tx_work, K_NO_WAIT);
	}

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
	struct cdc_acm_dev_data_t * const dev_data = dev->data;
	uint32_t len;

	LOG_DBG("dev %p size %d rx_ringbuf space %u",
		dev, size, ring_buf_space_get(dev_data->rx_ringbuf));

	len = ring_buf_get(dev_data->rx_ringbuf, rx_data, size);

	if (dev_data->rx_paused == true) {
		if (ring_buf_space_get(dev_data->rx_ringbuf) >= CDC_ACM_BUFFER_SIZE) {
			struct usb_cfg_data *cfg = (void *)dev->config;

			if (dev_data->configured) {
				cdc_acm_read_cb(cfg->endpoint[ACM_OUT_EP_IDX].ep_addr, 0, dev_data);
			}
			dev_data->rx_paused = false;
		}
	}

	return len;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev CDC ACM device struct.
 */
static void cdc_acm_irq_tx_enable(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

	dev_data->tx_irq_ena = true;

	if (dev_data->cb && dev_data->tx_ready) {
		k_work_submit_to_queue(&USB_WORK_Q, &dev_data->cb_work);
	}
}

/**
 * @brief Disable TX interrupt
 *
 * @param dev CDC ACM device struct.
 */
static void cdc_acm_irq_tx_disable(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

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
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

	if (dev_data->tx_irq_ena && dev_data->tx_ready) {
		return ring_buf_space_get(dev_data->tx_ringbuf);
	}

	return 0;
}

/**
 * @brief Enable RX interrupt
 *
 * @param dev CDC ACM device struct.
 */
static void cdc_acm_irq_rx_enable(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

	dev_data->rx_irq_ena = true;

	if (dev_data->cb && dev_data->rx_ready) {
		k_work_submit_to_queue(&USB_WORK_Q, &dev_data->cb_work);
	}
}

/**
 * @brief Disable RX interrupt
 *
 * @param dev CDC ACM device struct.
 */
static void cdc_acm_irq_rx_disable(const struct device *dev)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

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
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

	if (dev_data->rx_ready && dev_data->rx_irq_ena) {
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
	if (cdc_acm_irq_rx_ready(dev) || cdc_acm_irq_tx_ready(dev)) {
		return 1;
	}

	return 0;
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
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

	if (!ring_buf_space_get(dev_data->tx_ringbuf)) {
		dev_data->tx_ready = false;
	}

	if (ring_buf_is_empty(dev_data->rx_ringbuf)) {
		dev_data->rx_ready = false;
	}

	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev CDC ACM device struct.
 * @param cb  Callback function pointer.
 */
static void cdc_acm_irq_callback_set(const struct device *dev,
				     uart_irq_callback_user_data_t cb,
				     void *cb_data)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

#if defined(CONFIG_CDC_ACM_DTE_RATE_CALLBACK_SUPPORT)
int cdc_acm_dte_rate_callback_set(const struct device *dev,
				  cdc_dte_rate_callback_t callback)
{
	struct cdc_acm_dev_data_t *const dev_data = dev->data;

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
 */
static void cdc_acm_baudrate_set(const struct device *dev, uint32_t baudrate)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

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
 * @retval 0 on success.
 * @retval -EIO if timed out.
 */
static int cdc_acm_send_notification(const struct device *dev,
				     uint16_t serial_state)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;
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
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

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
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

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

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int cdc_acm_configure(const struct device *dev,
			     const struct uart_config *cfg)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		dev_data->flow_ctrl = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		dev_data->flow_ctrl = true;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int cdc_acm_config_get(const struct device *dev,
			      struct uart_config *cfg)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;

	cfg->baudrate = sys_le32_to_cpu(dev_data->line_coding.dwDTERate);

	switch (dev_data->line_coding.bCharFormat) {
	case USB_CDC_LINE_CODING_STOP_BITS_1:
		cfg->stop_bits = UART_CFG_STOP_BITS_1;
		break;
	case USB_CDC_LINE_CODING_STOP_BITS_1_5:
		cfg->stop_bits = UART_CFG_STOP_BITS_1_5;
		break;
	case USB_CDC_LINE_CODING_STOP_BITS_2:
	default:
		cfg->stop_bits = UART_CFG_STOP_BITS_2;
		break;
	};

	switch (dev_data->line_coding.bParityType) {
	case USB_CDC_LINE_CODING_PARITY_NO:
	default:
		cfg->parity = UART_CFG_PARITY_NONE;
		break;
	case USB_CDC_LINE_CODING_PARITY_ODD:
		cfg->parity = UART_CFG_PARITY_ODD;
		break;
	case USB_CDC_LINE_CODING_PARITY_EVEN:
		cfg->parity = UART_CFG_PARITY_EVEN;
		break;
	case USB_CDC_LINE_CODING_PARITY_MARK:
		cfg->parity = UART_CFG_PARITY_MARK;
		break;
	case USB_CDC_LINE_CODING_PARITY_SPACE:
		cfg->parity = UART_CFG_PARITY_SPACE;
		break;
	};

	switch (dev_data->line_coding.bDataBits) {
	case USB_CDC_LINE_CODING_DATA_BITS_5:
		cfg->data_bits = UART_CFG_DATA_BITS_5;
		break;
	case USB_CDC_LINE_CODING_DATA_BITS_6:
		cfg->data_bits = UART_CFG_DATA_BITS_6;
		break;
	case USB_CDC_LINE_CODING_DATA_BITS_7:
		cfg->data_bits = UART_CFG_DATA_BITS_7;
		break;
	case USB_CDC_LINE_CODING_DATA_BITS_8:
	default:
		cfg->data_bits = UART_CFG_DATA_BITS_8;
		break;
	};

	cfg->flow_ctrl = dev_data->flow_ctrl ? UART_CFG_FLOW_CTRL_RTS_CTS :
					       UART_CFG_FLOW_CTRL_NONE;

	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */


/*
 * @brief Poll the device for input.
 */
static int cdc_acm_poll_in(const struct device *dev, unsigned char *c)
{
	int ret = cdc_acm_fifo_read(dev, c, 1);

	return ret == 1 ? 0 : -1;
}

/*
 * @brief Output a character in polled mode.
 *
 * According to the UART API, the implementation of this routine should block
 * if the transmitter is full. But blocking when the USB subsystem is not ready
 * is considered highly undesirable behavior. Blocking may also be undesirable
 * when CDC ACM UART is used as a logging backend.
 *
 * The behavior of CDC ACM poll out is:
 *  - Block if the TX ring buffer is full, hw_flow_control property is enabled,
 *    and called from a non-ISR context.
 *  - Do not block if the USB subsystem is not ready, poll out implementation
 *    is called from an ISR context, or hw_flow_control property is disabled.
 *
 */
static void cdc_acm_poll_out(const struct device *dev, unsigned char c)
{
	struct cdc_acm_dev_data_t * const dev_data = dev->data;
	unsigned int lock;
	uint32_t wrote;

	dev_data->tx_ready = false;

	while (true) {
		lock = irq_lock();
		wrote = ring_buf_put(dev_data->tx_ringbuf, &c, 1);
		irq_unlock(lock);
		if (wrote == 1) {
			break;
		}
		if (k_is_in_isr() || !dev_data->flow_ctrl) {
			LOG_WRN_ONCE("Ring buffer full, discard data");
			break;
		}

		k_msleep(1);
	}

	/* Schedule with minimal timeout to make it possible to send more than
	 * one byte per USB transfer. The latency increase is negligible while
	 * the increased throughput and reduced CPU usage is easily observable.
	 */
	k_work_schedule_for_queue(&USB_WORK_Q, &dev_data->tx_work, K_MSEC(1));
}

static DEVICE_API(uart, cdc_acm_driver_api) = {
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
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = cdc_acm_configure,
	.config_get = cdc_acm_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
};

#define INITIALIZER_IAD							\
	.iad_cdc = {							\
		.bLength = sizeof(struct usb_association_descriptor),	\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,		\
		.bFirstInterface = 0,					\
		.bInterfaceCount = 0x02,				\
		.bFunctionClass = USB_BCC_CDC_CONTROL,			\
		.bFunctionSubClass = ACM_SUBCLASS,			\
		.bFunctionProtocol = 0,					\
		.iFunction = 0,						\
	},

#define INITIALIZER_IF(iface_num, num_ep, class, subclass)		\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE,			\
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
		.bDescriptorType = USB_DESC_CS_INTERFACE,		\
		.bDescriptorSubtype = HEADER_FUNC_DESC,			\
		.bcdCDC = sys_cpu_to_le16(USB_SRN_1_1),			\
	}

#define INITIALIZER_IF_CM						\
	{								\
		.bFunctionLength = sizeof(struct cdc_cm_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,		\
		.bDescriptorSubtype = CALL_MANAGEMENT_FUNC_DESC,	\
		.bmCapabilities = 0x02,					\
		.bDataInterface = 1,					\
	}

#define INITIALIZER_IF_ACM						\
	{								\
		.bFunctionLength = sizeof(struct cdc_acm_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,		\
		.bDescriptorSubtype = ACM_FUNC_DESC,			\
		.bmCapabilities = 0x02,					\
	}

#define INITIALIZER_IF_UNION						\
	{								\
		.bFunctionLength = sizeof(struct cdc_union_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,		\
		.bDescriptorSubtype = UNION_FUNC_DESC,			\
		.bControlInterface = 0,					\
		.bSubordinateInterface0 = 1,				\
	}

#define INITIALIZER_IF_EP(addr, attr, mps, interval)			\
	{								\
		.bLength = sizeof(struct usb_ep_descriptor),		\
		.bDescriptorType = USB_DESC_ENDPOINT,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = interval,					\
	}

#define CDC_ACM_CFG_AND_DATA_DEFINE(x)					\
	USBD_CLASS_DESCR_DEFINE(primary, x)				\
	struct usb_cdc_acm_config cdc_acm_cfg_##x = {			\
		INITIALIZER_IAD						\
		.if0 = INITIALIZER_IF(0, 1,				\
				USB_BCC_CDC_CONTROL,			\
				ACM_SUBCLASS),				\
		.if0_header = INITIALIZER_IF_HDR,			\
		.if0_cm = INITIALIZER_IF_CM,				\
		.if0_acm = INITIALIZER_IF_ACM,				\
		.if0_union = INITIALIZER_IF_UNION,			\
		.if0_int_ep = INITIALIZER_IF_EP(AUTO_EP_IN,		\
				USB_DC_EP_INTERRUPT,			\
				CONFIG_CDC_ACM_INTERRUPT_EP_MPS,	\
				0x0A),					\
		.if1 = INITIALIZER_IF(1, 2,				\
				USB_BCC_CDC_DATA,			\
				0),					\
		.if1_in_ep = INITIALIZER_IF_EP(AUTO_EP_IN,		\
				USB_DC_EP_BULK,				\
				CONFIG_CDC_ACM_BULK_EP_MPS,		\
				0x00),					\
		.if1_out_ep = INITIALIZER_IF_EP(AUTO_EP_OUT,		\
				USB_DC_EP_BULK,				\
				CONFIG_CDC_ACM_BULK_EP_MPS,		\
				0x00),					\
	};								\
									\
	static struct usb_ep_cfg_data cdc_acm_ep_data_##x[] = {		\
		{							\
			.ep_cb = cdc_acm_int_in,			\
			.ep_addr = AUTO_EP_IN,				\
		},							\
		{							\
			.ep_cb = usb_transfer_ep_callback,		\
			.ep_addr = AUTO_EP_OUT,				\
		},							\
		{							\
			.ep_cb = usb_transfer_ep_callback,		\
			.ep_addr = AUTO_EP_IN,				\
		},							\
	};								\
									\
	USBD_DEFINE_CFG_DATA(cdc_acm_config_##x) = {			\
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
	};								\
									\
	RING_BUF_DECLARE(cdc_acm_rx_rb_##x,				\
			 CONFIG_USB_CDC_ACM_RINGBUF_SIZE);		\
	RING_BUF_DECLARE(cdc_acm_tx_rb_##x,				\
			 CONFIG_USB_CDC_ACM_RINGBUF_SIZE);		\
	static struct cdc_acm_dev_data_t cdc_acm_dev_data_##x = {	\
		.line_coding = CDC_ACM_DEFAULT_BAUDRATE,		\
		.rx_ringbuf = &cdc_acm_rx_rb_##x,			\
		.tx_ringbuf = &cdc_acm_tx_rb_##x,			\
		.flow_ctrl = DT_INST_PROP(x, hw_flow_control),		\
	};

#define DT_DRV_COMPAT zephyr_cdc_acm_uart

#define CDC_ACM_DT_DEVICE_DEFINE(idx)					\
	BUILD_ASSERT(DT_INST_ON_BUS(idx, usb),				\
		     "node " DT_NODE_PATH(DT_DRV_INST(idx))		\
		     " is not assigned to a USB device controller");	\
	CDC_ACM_CFG_AND_DATA_DEFINE(idx)				\
									\
	DEVICE_DT_INST_DEFINE(idx, cdc_acm_init, NULL,			\
		&cdc_acm_dev_data_##idx, &cdc_acm_config_##idx,		\
		PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		\
		&cdc_acm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CDC_ACM_DT_DEVICE_DEFINE);
