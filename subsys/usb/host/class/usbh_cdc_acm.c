/*
 * Copyright Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cdc.h>

#include <zephyr/drivers/usb/uhc.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_cdc_acm, CONFIG_USBH_CDC_ACM_LOG_LEVEL);

/*
 * Abstract Control Model serial emulation subset of PSTN120, gated on the ACM
 * bmCapabilities D1 bit: Set_Line_Coding, Set_Control_Line_State (DTR/RTS) and
 * the SerialState notification (only DCD and DSR are exposed). Send_Break (D2)
 * and the communication feature requests (D0) are not implemented.
 */

#define CDC_ACM_DEFAULT_LINECODING	{sys_cpu_to_le32(115200), 0, 0, 8}

/*
 * Abstract Control Management bmCapabilities D1: device supports the request
 * combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding and
 * the notification Serial_State (PSTN120.pdf, 5.3.2, Table 4).
 */
#define CDC_ACM_CAP_LINE_CODING		BIT(1)

/* Net buffer user data carrying the original (user) buffer pointer. */
struct cdc_acm_buf_ud {
	uint8_t *user_buf;
};

/*
 * Each instance can have a TX, an active RX and a pending RX (ping-pong)
 * buffer allocated at the same time.
 */
#define USBH_CDC_ACM_BUF_COUNT		(3 * CONFIG_USBH_CDC_ACM_INSTANCES_COUNT)

/*
 * Pool for wrapping DMA-able user buffers without copying. The buffers carry no
 * data of their own.
 */
NET_BUF_POOL_DEFINE(host_cdc_acm_pool_0,
		    USBH_CDC_ACM_BUF_COUNT, 0, sizeof(struct cdc_acm_buf_ud), NULL);

#if defined(CONFIG_USBH_CDC_ACM_BOUNCE)
/* Pool for bounce buffers, used when the submitted buffer is not DMA-able. */
USB_BUF_POOL_VAR_DEFINE(host_cdc_acm_bounce_pool,
			USBH_CDC_ACM_BUF_COUNT,
			CONFIG_USBH_CDC_ACM_INSTANCES_COUNT *
				CONFIG_USBH_CDC_ACM_BOUNCE_POOL_SIZE,
			sizeof(struct cdc_acm_buf_ud), NULL);
#endif

static struct k_work_q cdc_acm_work_q;
static K_KERNEL_STACK_DEFINE(cdc_acm_stack, CONFIG_USBH_CDC_ACM_STACK_SIZE);

struct cdc_acm_uart_config {
	/* DMA-able single byte buffers used by the poll API */
	uint8_t *const poll_tx_buf;
	uint8_t *const poll_rx_buf;
};

struct cdc_acm_uart_data {
	const struct device *dev;
	struct usbh_class_data *const c_data;
	/* Communication (control) interface number */
	uint8_t ctrl_iface;
	/* Communication class interrupt IN (notification) endpoint */
	uint8_t int_ep;
	/* Data class bulk IN endpoint */
	uint8_t in_ep;
	/* Data class bulk OUT endpoint */
	uint8_t out_ep;
	/* Abstract Control Management capabilities (bmCapabilities) */
	uint8_t acm_caps;
	/* Line Coding Structure */
	struct cdc_acm_line_coding line_coding;
	/* SetControlLineState bitmap */
	uint16_t line_state;
	/* Serial state bitmap */
	uint16_t serial_state;
	/* UART actual configuration */
	struct uart_config uart_cfg;
	/* UART actual RTS state */
	bool line_state_rts;
	/* UART actual DTR state */
	bool line_state_dtr;
	/* Hardware (RTS/CTS) flow control enabled */
	bool flow_ctrl;

	/* Mutex protecting the TX and RX state */
	struct k_mutex mutex;
	/* Active TX transfer, NULL when idle */
	struct uhc_transfer *tx_xfer;
	/* Delayable work aborting the active TX transfer on timeout */
	struct k_work_delayable tx_timeout_work;
	/* Work applying the default configuration on connect */
	struct k_work cfg_work;

	/* Active RX transfer, NULL when idle */
	struct uhc_transfer *rx_xfer;
	/* Next RX transfer provided in response to UART_RX_BUF_REQUEST */
	struct uhc_transfer *rx_next_xfer;
	/* RX enabled flag */
	bool rx_enabled;

	/* Active notification (interrupt IN) transfer */
	struct uhc_transfer *notif_xfer;

	/* Poll API completion signals */
	struct k_sem poll_tx_sync;
	struct k_sem poll_rx_sync;
	int poll_in_ret;
	/* A poll_out / poll_in is in progress (serializes each direction) */
	bool poll_out_active;
	bool poll_in_active;

	/* UART ASYNC API callback */
	uart_callback_t async_cb;
	/* UART ASYNC API callback data */
	void *async_cb_data;
};

static int cdc_acm_tx_cb(struct usb_device *const udev,
			 struct uhc_transfer *const xfer);

static int cdc_acm_rx_cb(struct usb_device *const udev,
			 struct uhc_transfer *const xfer);

static int cdc_acm_notif_cb(struct usb_device *const udev,
			    struct uhc_transfer *const xfer);

static void cdc_acm_poll_cb(const struct device *dev,
			    struct uart_event *const evt, void *const user_data);

static int cdc_acm_callback_set(const struct device *dev,
				const uart_callback_t callback, void *const user_data)
{
	struct cdc_acm_uart_data *const data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (data->async_cb == cdc_acm_poll_cb) {
		/* Callback (and ASYNC API) is currently used by the poll API. */
		ret = -EBUSY;
	} else {
		data->async_cb = callback;
		data->async_cb_data = user_data;
	}

	k_mutex_unlock(&data->mutex);

	return ret;
}

static void cdc_acm_uart_evt(struct cdc_acm_uart_data *const data,
			     struct uart_event *const evt)
{
	if (data->async_cb != NULL) {
		data->async_cb(data->dev, evt, data->async_cb_data);
	}
}

static void cdc_acm_update_uart_cfg(struct cdc_acm_uart_data *const data)
{
	struct uart_config *const cfg = &data->uart_cfg;

	cfg->baudrate = sys_le32_to_cpu(data->line_coding.dwDTERate);

	switch (data->line_coding.bCharFormat) {
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
	}

	switch (data->line_coding.bParityType) {
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
	case USB_CDC_LINE_CODING_PARITY_NO:
	default:
		cfg->parity = UART_CFG_PARITY_NONE;
		break;
	}

	switch (data->line_coding.bDataBits) {
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
	}

	cfg->flow_ctrl = data->flow_ctrl ? UART_CFG_FLOW_CTRL_RTS_CTS :
					   UART_CFG_FLOW_CTRL_NONE;
}

static int cdc_acm_set_line_coding(struct cdc_acm_uart_data *const data,
				   const struct cdc_acm_line_coding *const line_coding)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				      (USB_REQTYPE_TYPE_CLASS << 5) |
				      (USB_REQTYPE_RECIPIENT_INTERFACE << 0);
	struct usb_device *const udev = data->c_data->udev;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, sizeof(*line_coding));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, line_coding, sizeof(*line_coding));

	ret = usbh_req_setup(udev, bmRequestType, SET_LINE_CODING, 0,
			     data->ctrl_iface, sizeof(*line_coding), buf);
	if (ret != 0) {
		LOG_ERR("Failed to set line coding: %d", ret);
	}

	usbh_xfer_buf_free(udev, buf);

	return ret;
}

static int cdc_acm_set_control_line_state(struct cdc_acm_uart_data *const data,
					  const uint16_t line_state)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				      (USB_REQTYPE_TYPE_CLASS << 5) |
				      (USB_REQTYPE_RECIPIENT_INTERFACE << 0);
	struct usb_device *const udev = data->c_data->udev;
	int ret;

	ret = usbh_req_setup(udev, bmRequestType, SET_CONTROL_LINE_STATE,
			     line_state, data->ctrl_iface, 0, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to set control line state: %d", ret);
	}

	return ret;
}

/* Caller must hold data->mutex. */
static int cdc_acm_notif_submit(const struct device *const dev)
{
	struct cdc_acm_uart_data *const data = dev->data;
	struct usb_device *const udev = data->c_data->udev;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	xfer = usbh_xfer_alloc(udev, data->int_ep, cdc_acm_notif_cb, data);
	if (xfer == NULL) {
		LOG_ERR("Failed to allocate transfer");
		return -ENOBUFS;
	}

	buf = usbh_xfer_buf_alloc(udev, xfer->mps);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		usbh_xfer_free(udev, xfer);
		return -ENOBUFS;
	}

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret != 0) {
		LOG_ERR("Failed to setup transfer");
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		return ret;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue transfer");
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		return ret;
	}

	data->notif_xfer = xfer;

	return 0;
}

static void cdc_acm_cfg_work(struct k_work *const work)
{
	struct cdc_acm_uart_data *const data =
		CONTAINER_OF(work, struct cdc_acm_uart_data, cfg_work);
	struct cdc_acm_line_coding line_coding;
	uint16_t line_state;

	if (data->c_data->udev == NULL) {
		return;
	}

	if (!(data->acm_caps & CDC_ACM_CAP_LINE_CODING)) {
		return;
	}

	/*
	 * The line_coding and line_state are passed by value so that the
	 * blocking control transfer does not read the shared configuration
	 * without holding data->mutex.
	 */
	k_mutex_lock(&data->mutex, K_FOREVER);
	line_coding = data->line_coding;
	line_state = data->line_state;
	k_mutex_unlock(&data->mutex);

	(void)cdc_acm_set_line_coding(data, &line_coding);
	(void)cdc_acm_set_control_line_state(data, line_state);

	/* Start monitoring the serial state notifications. */
	k_mutex_lock(&data->mutex, K_FOREVER);
	(void)cdc_acm_notif_submit(data->dev);
	k_mutex_unlock(&data->mutex);
}

static void cdc_acm_tx_timeout_work(struct k_work *const work)
{
	struct k_work_delayable *const dwork = k_work_delayable_from_work(work);
	struct cdc_acm_uart_data *const data =
		CONTAINER_OF(dwork, struct cdc_acm_uart_data, tx_timeout_work);
	struct usbh_class_data *const c_data = data->c_data;
	struct usb_device *const udev = c_data->udev;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (data->tx_xfer != NULL) {
		LOG_DBG("TX timeout, abort transfer %p", (void *)data->tx_xfer);
		(void)usbh_xfer_dequeue(udev, data->tx_xfer);
	}

	k_mutex_unlock(&data->mutex);
}

/*
 * Allocate memory for a transfer of the user buffer. A DMA-able user buffer
 * is used without copying, otherwise a bounce buffer is allocated. For TX, the
 * data is copied into it here, for RX it is copied back on completion. The
 * original user buffer pointer is stored in the net_buf user data.
 */
static struct net_buf *cdc_acm_buf_alloc(uint8_t *const user_buf,
					 const size_t len, const bool tx)
{
	struct cdc_acm_buf_ud *ud;
	struct net_buf *buf;

	if (IS_USB_BUF_ALIGNED(user_buf)) {
		buf = net_buf_alloc_with_data(&host_cdc_acm_pool_0,
					      user_buf, len, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return NULL;
		}

		if (!tx) {
			/* Reset length to receive into the wrapped buffer. */
			net_buf_reset(buf);
		}
	} else {
#if defined(CONFIG_USBH_CDC_ACM_BOUNCE)
		buf = net_buf_alloc_len(&host_cdc_acm_bounce_pool, len, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate bounce buffer");
			return NULL;
		}

		if (tx) {
			net_buf_add_mem(buf, user_buf, len);
		}
#else
		/* Unaligned buffers are rejected at the API entry points. */
		return NULL;
#endif
	}

	ud = net_buf_user_data(buf);
	ud->user_buf = user_buf;

	return buf;
}

static int cdc_acm_tx(const struct device *dev, const uint8_t *const tx_buf,
		      const size_t len, const int32_t timeout)
{
	struct cdc_acm_uart_data *const data = dev->data;
	struct usbh_class_data *const c_data = data->c_data;
	struct usb_device *const udev = c_data->udev;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	if (k_is_in_isr()) {
		LOG_ERR("Called from ISR context, not supported");
		return -ENOTSUP;
	}

	if (!IS_ENABLED(CONFIG_USBH_CDC_ACM_BOUNCE) && !IS_USB_BUF_ALIGNED(tx_buf)) {
		LOG_ERR("Buffer is not aligned");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (udev == NULL) {
		ret = -ENODEV;
		goto unlock;
	}

	if (data->tx_xfer != NULL) {
		ret = -EBUSY;
		goto unlock;
	}

	buf = cdc_acm_buf_alloc((uint8_t *)tx_buf, len, true);
	if (buf == NULL) {
		ret = -ENOBUFS;
		goto unlock;
	}

	xfer = usbh_xfer_alloc(udev, data->out_ep, cdc_acm_tx_cb, data);
	if (xfer == NULL) {
		LOG_ERR("Failed to allocate transfer");
		net_buf_unref(buf);
		ret = -ENOBUFS;
		goto unlock;
	}

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret != 0) {
		LOG_ERR("Failed to setup transfer");
		net_buf_unref(buf);
		usbh_xfer_free(udev, xfer);
		goto unlock;
	}

	data->tx_xfer = xfer;

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue transfer");
		data->tx_xfer = NULL;
		net_buf_unref(buf);
		usbh_xfer_free(udev, xfer);
		goto unlock;
	}

	/* The timeout is only valid if flow control is enabled, otherwise the
	 * transfer is allowed to wait for the device to accept the data.
	 */
	if (data->flow_ctrl && timeout != SYS_FOREVER_US) {
		k_work_schedule_for_queue(&cdc_acm_work_q, &data->tx_timeout_work,
					  K_USEC(timeout));
	}

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int cdc_acm_tx_abort(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;
	struct usbh_class_data *const c_data = data->c_data;
	struct usb_device *const udev = c_data->udev;
	int ret;

	if (k_is_in_isr()) {
		LOG_ERR("Called from ISR context, not supported");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (data->tx_xfer == NULL || udev == NULL) {
		ret = -EFAULT;
		goto unlock;
	}

	ret = usbh_xfer_dequeue(udev, data->tx_xfer);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static struct uhc_transfer *cdc_acm_rx_xfer_alloc(struct cdc_acm_uart_data *const data,
						  uint8_t *const buf, const size_t len)
{
	struct usb_device *const udev = data->c_data->udev;
	struct uhc_transfer *xfer;
	struct net_buf *nbuf;
	int ret;

	nbuf = cdc_acm_buf_alloc(buf, len, false);
	if (nbuf == NULL) {
		return NULL;
	}

	xfer = usbh_xfer_alloc(udev, data->in_ep, cdc_acm_rx_cb, data);
	if (xfer == NULL) {
		LOG_ERR("Failed to allocate transfer");
		net_buf_unref(nbuf);
		return NULL;
	}

	ret = usbh_xfer_buf_add(udev, xfer, nbuf);
	if (ret != 0) {
		LOG_ERR("Failed to setup transfer");
		net_buf_unref(nbuf);
		usbh_xfer_free(udev, xfer);
		return NULL;
	}

	return xfer;
}

static int cdc_acm_rx_enable(const struct device *dev, uint8_t *const buf,
			     const size_t len, const int32_t timeout)
{
	struct cdc_acm_uart_data *const data = dev->data;
	struct usb_device *const udev = data->c_data->udev;
	struct uart_event evt;
	struct uhc_transfer *xfer;
	int ret;

	ARG_UNUSED(timeout);

	if (k_is_in_isr()) {
		LOG_ERR("Called from ISR context, not supported");
		return -ENOTSUP;
	}

	if (!IS_ENABLED(CONFIG_USBH_CDC_ACM_BOUNCE) && !IS_USB_BUF_ALIGNED(buf)) {
		LOG_ERR("Buffer is not aligned");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (udev == NULL) {
		ret = -ENODEV;
		goto unlock;
	}

	if (data->rx_enabled) {
		ret = -EBUSY;
		goto unlock;
	}

	xfer = cdc_acm_rx_xfer_alloc(data, buf, len);
	if (xfer == NULL) {
		ret = -ENOBUFS;
		goto unlock;
	}

	data->rx_xfer = xfer;
	data->rx_enabled = true;

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue transfer");
		data->rx_xfer = NULL;
		data->rx_enabled = false;
		net_buf_unref(xfer->buf);
		usbh_xfer_free(udev, xfer);
		goto unlock;
	}

	/* Request a second buffer */
	evt.type = UART_RX_BUF_REQUEST;
	cdc_acm_uart_evt(data, &evt);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int cdc_acm_rx_buf_rsp(const struct device *dev, uint8_t *const buf,
			      const size_t len)
{
	struct cdc_acm_uart_data *const data = dev->data;
	struct uhc_transfer *xfer;
	int ret = 0;

	if (k_is_in_isr()) {
		LOG_WRN("Called from ISR context, not supported");
		return -ENOTSUP;
	}

	if (!IS_ENABLED(CONFIG_USBH_CDC_ACM_BOUNCE) && !IS_USB_BUF_ALIGNED(buf)) {
		LOG_ERR("Buffer is not aligned");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (!data->rx_enabled) {
		ret = -EACCES;
		goto unlock;
	}

	if (data->rx_next_xfer != NULL) {
		ret = -EBUSY;
		goto unlock;
	}

	xfer = cdc_acm_rx_xfer_alloc(data, buf, len);
	if (xfer == NULL) {
		ret = -ENOBUFS;
		goto unlock;
	}

	data->rx_next_xfer = xfer;

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int cdc_acm_rx_disable(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;
	struct usb_device *const udev = data->c_data->udev;
	int ret = 0;

	if (k_is_in_isr()) {
		LOG_ERR("Called from ISR context, not supported");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (udev == NULL) {
		ret = -ENODEV;
		goto unlock;
	}

	if (!data->rx_enabled) {
		ret = -EFAULT;
		goto unlock;
	}

	data->rx_enabled = false;

	if (data->rx_xfer != NULL) {
		/* The completion handler releases the buffers and emits the
		 * UART_RX_DISABLED event.
		 */
		(void)usbh_xfer_dequeue(udev, data->rx_xfer);
	}

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/*
 * The poll API is implemented on top of the ASYNC API and is only available
 * when the application has not installed its own ASYNC API callback. While
 * polling, a private callback is installed to synchronously complete the
 * single byte transfer.
 */
static void cdc_acm_poll_cb(const struct device *dev, struct uart_event *const evt,
			    void *const user_data)
{
	struct cdc_acm_uart_data *const data = user_data;

	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		k_sem_give(&data->poll_tx_sync);
		break;
	case UART_RX_RDY:
		data->poll_in_ret = 0;
		break;
	case UART_RX_DISABLED:
		k_sem_give(&data->poll_rx_sync);
		break;
	default:
		break;
	}
}

static int cdc_acm_poll_in(const struct device *dev, unsigned char *const c)
{
	struct cdc_acm_uart_data *const data = dev->data;
	const struct cdc_acm_uart_config *const cfg = dev->config;
	int ret;

	if (k_is_in_isr()) {
		LOG_ERR("Called from ISR context, not supported");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if ((data->async_cb != NULL && data->async_cb != cdc_acm_poll_cb) ||
	    data->poll_in_active) {
		k_mutex_unlock(&data->mutex);
		return -EBUSY;
	}

	data->poll_in_active = true;
	data->async_cb = cdc_acm_poll_cb;
	data->async_cb_data = data;

	/*
	 * Receive a single byte. The buffer fills after one byte and reception
	 * is disabled because no further buffer is provided. The device may send
	 * a ZLP, which completes the transfer without delivering data, so
	 * retry until a byte is received or reception cannot be enabled.
	 */
	do {
		data->poll_in_ret = -1;

		ret = cdc_acm_rx_enable(dev, cfg->poll_rx_buf, 1, SYS_FOREVER_US);
		if (ret != 0) {
			break;
		}

		/* The mutex must be released before waiting for the completion. */
		k_mutex_unlock(&data->mutex);

		k_sem_take(&data->poll_rx_sync, K_FOREVER);

		k_mutex_lock(&data->mutex, K_FOREVER);

		ret = data->poll_in_ret;
	} while (ret != 0);

	data->poll_in_active = false;
	if (!data->poll_out_active) {
		data->async_cb = NULL;
		data->async_cb_data = NULL;
	}

	k_mutex_unlock(&data->mutex);

	if (ret == 0) {
		*c = cfg->poll_rx_buf[0];
	}

	return ret;
}

static void cdc_acm_poll_out(const struct device *dev, const unsigned char c)
{
	const struct cdc_acm_uart_config *const cfg = dev->config;
	struct cdc_acm_uart_data *const data = dev->data;
	int ret;

	if (k_is_in_isr()) {
		LOG_ERR("Called from ISR context, not supported");
		return;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if ((data->async_cb != NULL && data->async_cb != cdc_acm_poll_cb) ||
	    data->poll_out_active) {
		k_mutex_unlock(&data->mutex);
		LOG_ERR("Asynchronous API is in use");
		return;
	}

	data->poll_out_active = true;
	data->async_cb = cdc_acm_poll_cb;
	data->async_cb_data = data;

	cfg->poll_tx_buf[0] = c;
	ret = cdc_acm_tx(dev, cfg->poll_tx_buf, 1, SYS_FOREVER_US);

	/* The mutex must be released before waiting for the completion. */
	k_mutex_unlock(&data->mutex);

	if (ret == 0) {
		k_sem_take(&data->poll_tx_sync, K_FOREVER);
	} else {
		LOG_ERR("Failed to send byte (%d)", ret);
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	data->poll_out_active = false;
	if (!data->poll_in_active) {
		data->async_cb = NULL;
		data->async_cb_data = NULL;
	}

	k_mutex_unlock(&data->mutex);
}

#ifdef CONFIG_UART_LINE_CTRL
static int cdc_acm_set_baud_rate(struct cdc_acm_uart_data *const data,
				 const uint32_t baudrate)
{
	struct cdc_acm_line_coding line_coding;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);
	line_coding = data->line_coding;
	line_coding.dwDTERate = sys_cpu_to_le32(baudrate);
	k_mutex_unlock(&data->mutex);

	ret = cdc_acm_set_line_coding(data, &line_coding);
	if (ret == 0) {
		k_mutex_lock(&data->mutex, K_FOREVER);
		data->line_coding = line_coding;
		cdc_acm_update_uart_cfg(data);
		k_mutex_unlock(&data->mutex);
	}

	return ret;
}

static int cdc_acm_set_line_state(struct cdc_acm_uart_data *const data,
				  const uint16_t mask, const uint32_t val)
{
	uint16_t line_state;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);

	line_state = data->line_state;
	if (val) {
		line_state |= mask;
	} else {
		line_state &= ~mask;
	}

	k_mutex_unlock(&data->mutex);

	ret = cdc_acm_set_control_line_state(data, line_state);
	if (ret == 0) {
		k_mutex_lock(&data->mutex, K_FOREVER);
		data->line_state = line_state;
		data->line_state_rts = (line_state & SET_CONTROL_LINE_STATE_RTS) != 0;
		data->line_state_dtr = (line_state & SET_CONTROL_LINE_STATE_DTR) != 0;
		k_mutex_unlock(&data->mutex);
	}

	return ret;
}

static int cdc_acm_line_ctrl_set(const struct device *dev,
				 const uint32_t ctrl, const uint32_t val)
{
	struct cdc_acm_uart_data *const data = dev->data;

	if (k_is_in_isr()) {
		LOG_ERR("Called from ISR context, not supported");
		return -ENOTSUP;
	}

	if (data->c_data->udev == NULL) {
		return -ENODEV;
	}

	if (!(data->acm_caps & CDC_ACM_CAP_LINE_CODING)) {
		return -ENOTSUP;
	}

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		return cdc_acm_set_baud_rate(data, val);
	case UART_LINE_CTRL_RTS:
		return cdc_acm_set_line_state(data, SET_CONTROL_LINE_STATE_RTS, val);
	case UART_LINE_CTRL_DTR:
		return cdc_acm_set_line_state(data, SET_CONTROL_LINE_STATE_DTR, val);
	}

	return -ENOTSUP;
}

static int cdc_acm_line_ctrl_get(const struct device *dev,
				 const uint32_t ctrl, uint32_t *const val)
{
	struct cdc_acm_uart_data *const data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		*val = data->uart_cfg.baudrate;
		break;
	case UART_LINE_CTRL_RTS:
		*val = data->line_state_rts;
		break;
	case UART_LINE_CTRL_DTR:
		*val = data->line_state_dtr;
		break;
	case UART_LINE_CTRL_DCD:
		*val = (data->serial_state & USB_CDC_SERIAL_STATE_RXCARRIER) ? 1 : 0;
		break;
	case UART_LINE_CTRL_DSR:
		*val = (data->serial_state & USB_CDC_SERIAL_STATE_TXCARRIER) ? 1 : 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->mutex);

	return ret;
}
#endif

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int cdc_acm_configure(const struct device *dev,
			     const struct uart_config *const cfg)
{
	struct cdc_acm_uart_data *const data = dev->data;
	struct cdc_acm_line_coding line_coding;
	bool flow_ctrl;
	int ret;

	if (k_is_in_isr()) {
		LOG_ERR("Called from ISR context, not supported");
		return -ENOTSUP;
	}

	if (data->c_data->udev == NULL) {
		return -ENODEV;
	}

	if (!(data->acm_caps & CDC_ACM_CAP_LINE_CODING)) {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		flow_ctrl = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		flow_ctrl = true;
		break;
	default:
		return -ENOTSUP;
	}

	line_coding.dwDTERate = sys_cpu_to_le32(cfg->baudrate);

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		line_coding.bDataBits = USB_CDC_LINE_CODING_DATA_BITS_5;
		break;
	case UART_CFG_DATA_BITS_6:
		line_coding.bDataBits = USB_CDC_LINE_CODING_DATA_BITS_6;
		break;
	case UART_CFG_DATA_BITS_7:
		line_coding.bDataBits = USB_CDC_LINE_CODING_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		line_coding.bDataBits = USB_CDC_LINE_CODING_DATA_BITS_8;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		line_coding.bParityType = USB_CDC_LINE_CODING_PARITY_NO;
		break;
	case UART_CFG_PARITY_ODD:
		line_coding.bParityType = USB_CDC_LINE_CODING_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		line_coding.bParityType = USB_CDC_LINE_CODING_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_MARK:
		line_coding.bParityType = USB_CDC_LINE_CODING_PARITY_MARK;
		break;
	case UART_CFG_PARITY_SPACE:
		line_coding.bParityType = USB_CDC_LINE_CODING_PARITY_SPACE;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		line_coding.bCharFormat = USB_CDC_LINE_CODING_STOP_BITS_1;
		break;
	case UART_CFG_STOP_BITS_1_5:
		line_coding.bCharFormat = USB_CDC_LINE_CODING_STOP_BITS_1_5;
		break;
	case UART_CFG_STOP_BITS_2:
		line_coding.bCharFormat = USB_CDC_LINE_CODING_STOP_BITS_2;
		break;
	default:
		return -ENOTSUP;
	}

	ret = cdc_acm_set_line_coding(data, &line_coding);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	data->flow_ctrl = flow_ctrl;
	data->line_coding = line_coding;
	cdc_acm_update_uart_cfg(data);
	k_mutex_unlock(&data->mutex);

	return 0;
}

static int cdc_acm_config_get(const struct device *dev,
			      struct uart_config *const cfg)
{
	struct cdc_acm_uart_data *const data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
	memcpy(cfg, &data->uart_cfg, sizeof(struct uart_config));
	k_mutex_unlock(&data->mutex);

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static DEVICE_API(uart, cdc_acm_uart_api) = {
	.poll_in = cdc_acm_poll_in,
	.poll_out = cdc_acm_poll_out,
#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = cdc_acm_line_ctrl_set,
	.line_ctrl_get = cdc_acm_line_ctrl_get,
#endif
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = cdc_acm_configure,
	.config_get = cdc_acm_config_get,
#endif
	.callback_set = cdc_acm_callback_set,
	.tx = cdc_acm_tx,
	.tx_abort = cdc_acm_tx_abort,
	.rx_enable = cdc_acm_rx_enable,
	.rx_buf_rsp = cdc_acm_rx_buf_rsp,
	.rx_disable = cdc_acm_rx_disable,
};

static int cdc_acm_tx_cb(struct usb_device *const udev,
			 struct uhc_transfer *const xfer)
{
	struct cdc_acm_uart_data *const data = xfer->priv;
	struct net_buf *const buf = xfer->buf;
	const struct cdc_acm_buf_ud *const ud = net_buf_user_data(buf);
	struct uart_event evt;

	/*
	 * The controller consumes the OUT buffer, so the number of bytes
	 * transferred is the difference between the original buffer size and
	 * the data left in it.
	 */
	evt.data.tx.len = buf->size - buf->len;
	evt.data.tx.buf = ud->user_buf;

	if (xfer->err == 0) {
		LOG_DBG("Transfer %p finished", (void *)xfer);
		evt.type = UART_TX_DONE;
	} else {
		if (xfer->err != -ECONNRESET) {
			LOG_ERR("Transfer failed with error %d", xfer->err);
		}

		evt.type = UART_TX_ABORTED;
	}

	/* The event is emitted under the mutex, like the RX callback. */
	k_mutex_lock(&data->mutex, K_FOREVER);
	k_work_cancel_delayable(&data->tx_timeout_work);
	data->tx_xfer = NULL;
	cdc_acm_uart_evt(data, &evt);
	k_mutex_unlock(&data->mutex);

	net_buf_unref(buf);
	usbh_xfer_free(udev, xfer);

	return 0;
}

static int cdc_acm_rx_cb(struct usb_device *const udev,
			 struct uhc_transfer *const xfer)
{
	struct cdc_acm_uart_data *const data = xfer->priv;
	struct net_buf *const buf = xfer->buf;
	const struct cdc_acm_buf_ud *const ud = net_buf_user_data(buf);
	uint8_t *const user_buf = ud->user_buf;
	const int err = xfer->err;
	struct uart_event evt;

	/*
	 * ASYNC API events are emitted while holding the mutex so that the RX
	 * state stays consistent across the event sequence. The application may
	 * call uart_rx_buf_rsp() or uart_rx_disable() from the callback. This is
	 * safe because the mutex is recursive within the same thread.
	 */
	k_mutex_lock(&data->mutex, K_FOREVER);

	data->rx_xfer = NULL;

	/* Report the received data and release the completed buffer. */
	if (buf->len > 0) {
		/* Copy back to the user buffer when a bounce buffer was used. */
		if (buf->data != user_buf) {
			memcpy(user_buf, buf->data, buf->len);
		}

		evt.type = UART_RX_RDY;
		evt.data.rx.buf = user_buf;
		evt.data.rx.offset = 0;
		evt.data.rx.len = buf->len;
		cdc_acm_uart_evt(data, &evt);
	}

	evt.type = UART_RX_BUF_RELEASED;
	evt.data.rx_buf.buf = user_buf;
	cdc_acm_uart_evt(data, &evt);

	net_buf_unref(buf);
	usbh_xfer_free(udev, xfer);

	/* Continue with the next buffer if reception is still enabled. */
	if (err == 0 && data->rx_enabled && data->rx_next_xfer != NULL) {
		struct uhc_transfer *const next = data->rx_next_xfer;
		const struct cdc_acm_buf_ud *next_ud = net_buf_user_data(next->buf);

		data->rx_next_xfer = NULL;
		data->rx_xfer = next;

		if (usbh_xfer_enqueue(udev, next) == 0) {
			/* Request the next buffer */
			evt.type = UART_RX_BUF_REQUEST;
			cdc_acm_uart_evt(data, &evt);
			k_mutex_unlock(&data->mutex);
			return 0;
		}

		LOG_ERR("Failed to enqueue transfer");
		data->rx_xfer = NULL;

		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = next_ud->user_buf;
		cdc_acm_uart_evt(data, &evt);

		net_buf_unref(next->buf);
		usbh_xfer_free(udev, next);
	}

	/* Reception stopped, release any pending next buffer and disable. */
	if (data->rx_next_xfer != NULL) {
		const struct cdc_acm_buf_ud *next_ud =
			net_buf_user_data(data->rx_next_xfer->buf);

		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = next_ud->user_buf;
		cdc_acm_uart_evt(data, &evt);

		net_buf_unref(data->rx_next_xfer->buf);
		usbh_xfer_free(udev, data->rx_next_xfer);
		data->rx_next_xfer = NULL;
	}

	evt.type = UART_RX_DISABLED;
	cdc_acm_uart_evt(data, &evt);
	data->rx_enabled = false;

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int cdc_acm_notif_cb(struct usb_device *const udev,
			    struct uhc_transfer *const xfer)
{
	struct cdc_acm_uart_data *const data = xfer->priv;
	struct net_buf *const buf = xfer->buf;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (xfer->err == 0 && buf->len >= sizeof(struct cdc_acm_notification)) {
		const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
					      (USB_REQTYPE_TYPE_CLASS << 5) |
					      (USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		const struct cdc_acm_notification *const notif = (const void *)buf->data;

		if (notif->bmRequestType == bmRequestType &&
		    notif->bNotificationType == USB_CDC_SERIAL_STATE &&
		    sys_le16_to_cpu(notif->wIndex) == data->ctrl_iface) {
			data->serial_state = sys_le16_to_cpu(notif->data);
			LOG_DBG("Serial state 0x%04x", data->serial_state);
		}
	}

	/*
	 * Reuse the same transfer and buffer unless it was cancelled or USB
	 * device is being removed (notif_xfer cleared by cdc_acm_removed()).
	 */
	if (xfer->err == 0 && data->notif_xfer == xfer) {
		net_buf_reset(buf);

		if (usbh_xfer_enqueue(udev, xfer) == 0) {
			k_mutex_unlock(&data->mutex);
			return 0;
		}

		LOG_ERR("Failed to resubmit notification transfer");
	}

	if (data->notif_xfer == xfer) {
		data->notif_xfer = NULL;
	}

	k_mutex_unlock(&data->mutex);

	usbh_xfer_buf_free(udev, buf);
	usbh_xfer_free(udev, xfer);

	return 0;
}

static int cdc_acm_parse_descriptors(struct cdc_acm_uart_data *const data,
				     struct usb_device *const udev,
				     const uint8_t ctrl_iface)
{
	const struct usb_association_descriptor *iad;
	const struct cdc_union_descriptor *un_desc = NULL;
	const struct cdc_acm_descriptor *acm_desc = NULL;
	const struct usb_if_descriptor *if_desc;
	const struct usb_ep_descriptor *ep_desc;
	const struct usb_desc_header *desc;
	uint8_t data_iface;

	/*
	 * Start from a clean state so a partial parse cannot leave stale
	 * endpoints from an earlier probe.
	 */
	data->int_ep = 0;
	data->in_ep = 0;
	data->out_ep = 0;

	if_desc = usbh_desc_get_iface(udev, ctrl_iface);
	if (if_desc == NULL) {
		LOG_ERR("Communication interface %u not found", ctrl_iface);
		return -ENODEV;
	}

	/*
	 * Walk the communication interface descriptors to find the union and
	 * abstract control management functional descriptors and notification
	 * interrupt IN endpoint.
	 */
	desc = usbh_desc_get_next(if_desc);
	while (desc != NULL && desc->bDescriptorType != USB_DESC_INTERFACE) {
		if (desc->bDescriptorType == USB_DESC_CS_INTERFACE) {
			const struct cdc_header_descriptor *cs_desc = (const void *)desc;

			switch (cs_desc->bDescriptorSubtype) {
			case UNION_FUNC_DESC:
				un_desc = (const void *)desc;
				break;
			case ACM_FUNC_DESC:
				acm_desc = (const void *)desc;
				break;
			default:
				break;
			}
		}

		if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			uint8_t ep_type;

			ep_desc = (const void *)desc;
			ep_type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (ep_type == USB_EP_TYPE_INTERRUPT &&
			    USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
				data->int_ep = ep_desc->bEndpointAddress;
			} else {
				LOG_WRN("Found suspicious endpoint 0x%02x with type %u",
					ep_desc->bEndpointAddress, ep_type);
			}
		}

		desc = usbh_desc_get_next(desc);
	}

	if (un_desc == NULL) {
		LOG_ERR("Union functional descriptor not found");
		return -ENODEV;
	}

	if (acm_desc == NULL) {
		LOG_ERR("Abstract control management descriptor not found");
		return -ENODEV;
	}

	if (data->int_ep == 0) {
		LOG_ERR("Control interface endpoint not found");
		return -ENODEV;
	}

	LOG_DBG("Union descriptor bControlInterface %u bSubordinateInterface0 %u",
		un_desc->bControlInterface, un_desc->bSubordinateInterface0);

	if (un_desc->bControlInterface != ctrl_iface) {
		LOG_ERR("Union control interface %u does not match %u",
			un_desc->bControlInterface, ctrl_iface);
		return -ENODEV;
	}

	data_iface = un_desc->bSubordinateInterface0;

	/* Check IAD and verify control and data interfaces */
	iad = usbh_desc_get_iad(udev, ctrl_iface);
	if (iad != NULL &&
	    (un_desc->bControlInterface != iad->bFirstInterface ||
	     data_iface < iad->bFirstInterface ||
	     data_iface >= iad->bFirstInterface + iad->bInterfaceCount)) {
		LOG_ERR("Union interfaces do not match IAD %u..%u",
			iad->bFirstInterface,
			iad->bFirstInterface + iad->bInterfaceCount - 1);
		return -ENODEV;
	}

	/* Walk the data interface descriptors to find the bulk endpoints */
	if_desc = usbh_desc_get_iface(udev, data_iface);
	if (if_desc == NULL) {
		LOG_ERR("Data interface %u not found", data_iface);
		return -ENODEV;
	}

	desc = usbh_desc_get_next(if_desc);
	while (desc != NULL && desc->bDescriptorType != USB_DESC_INTERFACE) {
		uint8_t ep_type;

		if (desc->bDescriptorType != USB_DESC_ENDPOINT) {
			desc = usbh_desc_get_next(desc);
			continue;
		}

		ep_desc = (const void *)desc;
		ep_type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

		if (ep_type == USB_EP_TYPE_BULK) {
			if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
				data->in_ep = ep_desc->bEndpointAddress;
			} else {
				data->out_ep = ep_desc->bEndpointAddress;
			}
		} else {
			LOG_WRN("Found suspicious endpoint 0x%02x with type %u",
				ep_desc->bEndpointAddress, ep_type);
		}

		desc = usbh_desc_get_next(desc);
	}

	if (data->in_ep == 0 || data->out_ep == 0) {
		LOG_ERR("Bulk endpoints not found, in 0x%02x out 0x%02x",
			data->in_ep, data->out_ep);
		return -ENODEV;
	}

	data->ctrl_iface = ctrl_iface;
	data->acm_caps = acm_desc->bmCapabilities;
	LOG_DBG("Use control interface %u, endpoints 0x%02x 0x%02x 0x%02x",
		data->ctrl_iface, data->int_ep, data->in_ep, data->out_ep);

	return 0;
}

static int cdc_acm_probe(struct usbh_class_data *const c_data,
			 struct usb_device *const udev, const uint8_t iface)
{
	const struct device *const dev = c_data->priv;
	struct cdc_acm_uart_data *const data = dev->data;
	int ret;

	LOG_DBG("Probe %s bInterfaceNumber %u", c_data->name, iface);

	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		return -ENOTSUP;
	}

	ret = cdc_acm_parse_descriptors(data, udev, iface);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	cdc_acm_update_uart_cfg(data);
	k_mutex_unlock(&data->mutex);

	/*
	 * Apply the default line coding and control line state from a work
	 * to avoid blocking the thread on control transfers.
	 */
	k_work_submit_to_queue(&cdc_acm_work_q, &data->cfg_work);

	return 0;
}

static int cdc_acm_init(struct usbh_class_data *const c_data)
{
	LOG_DBG("%s: Init CDC ACM class driver", c_data->name);

	return 0;
}

static int cdc_acm_removed(struct usbh_class_data *const c_data)
{
	const struct device *const dev = c_data->priv;
	struct cdc_acm_uart_data *const data = dev->data;
	struct usb_device *const udev = c_data->udev;

	LOG_DBG("Removed %s", c_data->name);

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Stop the pending work before tearing down the transfers. */
	k_work_cancel_delayable(&data->tx_timeout_work);
	k_work_cancel(&data->cfg_work);

	/*
	 * Dequeue all submitted transfers. UHC driver and stack return them to
	 * the respective completion callbacks, which release the buffers and
	 * emit the corresponding UART events.
	 * Clearing rx_enabled and notif_xfer prevents the RX and notification
	 * callbacks from re-arming a transfer while the device is being removed.
	 */
	data->rx_enabled = false;

	if (data->tx_xfer != NULL) {
		(void)usbh_xfer_dequeue(udev, data->tx_xfer);
	}

	if (data->rx_xfer != NULL) {
		(void)usbh_xfer_dequeue(udev, data->rx_xfer);
	}

	if (data->notif_xfer != NULL) {
		(void)usbh_xfer_dequeue(udev, data->notif_xfer);
		data->notif_xfer = NULL;
	}

	data->ctrl_iface = 0;
	data->int_ep = 0;
	data->in_ep = 0;
	data->out_ep = 0;
	data->acm_caps = 0;

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int usbh_cdc_acm_init_wq(void)
{
	k_work_queue_init(&cdc_acm_work_q);
	k_work_queue_start(&cdc_acm_work_q, cdc_acm_stack,
			   K_KERNEL_STACK_SIZEOF(cdc_acm_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, NULL);
	k_thread_name_set(&cdc_acm_work_q.thread, "cdc_acm_work_q");

	return 0;
}

static int usbh_cdc_acm_preinit(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	k_mutex_init(&data->mutex);
	k_sem_init(&data->poll_tx_sync, 0, 1);
	k_sem_init(&data->poll_rx_sync, 0, 1);
	k_work_init_delayable(&data->tx_timeout_work, cdc_acm_tx_timeout_work);
	k_work_init(&data->cfg_work, cdc_acm_cfg_work);

	return 0;
}

static struct usbh_class_api usbh_cdc_acm_api = {
	.probe = cdc_acm_probe,
	.init = cdc_acm_init,
	.removed = cdc_acm_removed,
};

static struct usbh_class_filter cdc_acm_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_CDC_CONTROL,
		.sub = ACM_SUBCLASS,
		.proto = 0,
	},
	{0},
};

#define USBH_CDC_ACM_DEVICE_DEFINE(n, _)					\
	UDC_STATIC_BUF_DEFINE(poll_tx_buf_##n, 1);				\
	UDC_STATIC_BUF_DEFINE(poll_rx_buf_##n, 1);				\
										\
	static const struct cdc_acm_uart_config uart_config_##n = {		\
		.poll_tx_buf = poll_tx_buf_##n,					\
		.poll_rx_buf = poll_rx_buf_##n,					\
	};									\
										\
	static struct cdc_acm_uart_data uart_data_##n;				\
										\
	DEVICE_DEFINE(usbh_cdc_acm_##n, "usbh_cdc_acm_" #n,			\
		      usbh_cdc_acm_preinit, NULL,				\
		      &uart_data_##n, &uart_config_##n,				\
		      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		\
		      &cdc_acm_uart_api);					\
										\
	USBH_DEFINE_CLASS(cdc_acm_##n,						\
			  &usbh_cdc_acm_api,					\
			  (void *)DEVICE_GET(usbh_cdc_acm_##n),			\
			  cdc_acm_filters);					\
										\
	static struct cdc_acm_uart_data uart_data_##n = {			\
		.dev = DEVICE_GET(usbh_cdc_acm_##n),				\
		.c_data = &class_data_cdc_acm_##n,				\
		.line_coding = CDC_ACM_DEFAULT_LINECODING,			\
	};

LISTIFY(CONFIG_USBH_CDC_ACM_INSTANCES_COUNT, USBH_CDC_ACM_DEVICE_DEFINE, ())

SYS_INIT(usbh_cdc_acm_init_wq, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
