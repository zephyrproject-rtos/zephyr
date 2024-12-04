/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_cdc_acm_uart

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cdc.h>

#include <zephyr/drivers/usb/udc.h>

#include "usbd_msg.h"

#include <zephyr/logging/log.h>
/* Prevent endless recursive logging loop and warn user about it */
#if defined(CONFIG_USBD_CDC_ACM_LOG_LEVEL) && CONFIG_USBD_CDC_ACM_LOG_LEVEL != LOG_LEVEL_NONE
#define CHOSEN_CONSOLE DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart)
#define CHOSEN_SHELL   DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
#if (CHOSEN_CONSOLE && defined(CONFIG_LOG_BACKEND_UART)) || \
	(CHOSEN_SHELL && defined(CONFIG_SHELL_LOG_BACKEND))
#warning "USBD_CDC_ACM_LOG_LEVEL forced to LOG_LEVEL_NONE"
#undef CONFIG_USBD_CDC_ACM_LOG_LEVEL
#define CONFIG_USBD_CDC_ACM_LOG_LEVEL LOG_LEVEL_NONE
#endif
#endif
LOG_MODULE_REGISTER(usbd_cdc_acm, CONFIG_USBD_CDC_ACM_LOG_LEVEL);

UDC_BUF_POOL_DEFINE(cdc_acm_ep_pool,
		    DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 2,
		    512, sizeof(struct udc_buf_info), NULL);

#define CDC_ACM_DEFAULT_LINECODING	{sys_cpu_to_le32(115200), 0, 0, 8}
#define CDC_ACM_DEFAULT_INT_EP_MPS	16
#define CDC_ACM_INTERVAL_DEFAULT	10000UL
#define CDC_ACM_FS_INT_EP_INTERVAL	USB_FS_INT_EP_INTERVAL(10000U)
#define CDC_ACM_HS_INT_EP_INTERVAL	USB_HS_INT_EP_INTERVAL(10000U)

#define CDC_ACM_CLASS_ENABLED		0
#define CDC_ACM_CLASS_SUSPENDED		1
#define CDC_ACM_IRQ_RX_ENABLED		2
#define CDC_ACM_IRQ_TX_ENABLED		3
#define CDC_ACM_RX_FIFO_BUSY		4
#define CDC_ACM_TX_FIFO_BUSY		5

static struct k_work_q cdc_acm_work_q;
static K_KERNEL_STACK_DEFINE(cdc_acm_stack,
			     CONFIG_USBD_CDC_ACM_STACK_SIZE);

struct cdc_acm_uart_fifo {
	struct ring_buf *rb;
	bool irq;
	bool altered;
};

struct usbd_cdc_acm_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct cdc_header_descriptor if0_header;
	struct cdc_cm_descriptor if0_cm;
	struct cdc_acm_descriptor if0_acm;
	struct cdc_union_descriptor if0_union;
	struct usb_ep_descriptor if0_int_ep;
	struct usb_ep_descriptor if0_hs_int_ep;

	struct usb_if_descriptor if1;
	struct usb_ep_descriptor if1_in_ep;
	struct usb_ep_descriptor if1_out_ep;
	struct usb_ep_descriptor if1_hs_in_ep;
	struct usb_ep_descriptor if1_hs_out_ep;

	struct usb_desc_header nil_desc;
};

struct cdc_acm_uart_data {
	/* Pointer to the associated USBD class node */
	struct usbd_class_data *c_data;
	/* Pointer to the class interface descriptors */
	struct usbd_cdc_acm_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
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
	/* UART API IRQ callback */
	uart_irq_callback_user_data_t cb;
	/* UART API user callback data */
	void *cb_data;
	/* UART API IRQ callback work */
	struct k_work irq_cb_work;
	struct cdc_acm_uart_fifo rx_fifo;
	struct cdc_acm_uart_fifo tx_fifo;
	/* When flow_ctrl is set, poll out is blocked when the buffer is full,
	 * roughly emulating flow control.
	 */
	bool flow_ctrl;
	/* USBD CDC ACM TX fifo work */
	struct k_work_delayable tx_fifo_work;
	/* USBD CDC ACM RX fifo work */
	struct k_work rx_fifo_work;
	atomic_t state;
	struct k_sem notif_sem;
};

static void cdc_acm_irq_rx_enable(const struct device *dev);

struct net_buf *cdc_acm_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&cdc_acm_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	return buf;
}

static ALWAYS_INLINE int cdc_acm_work_submit(struct k_work *work)
{
	return k_work_submit_to_queue(&cdc_acm_work_q, work);
}

static ALWAYS_INLINE int cdc_acm_work_schedule(struct k_work_delayable *work,
					       k_timeout_t delay)
{
	return k_work_schedule_for_queue(&cdc_acm_work_q, work, delay);
}

static ALWAYS_INLINE bool check_wq_ctx(const struct device *dev)
{
	return k_current_get() == k_work_queue_thread_get(&cdc_acm_work_q);
}

static uint8_t cdc_acm_get_int_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;
	struct usbd_cdc_acm_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_int_ep.bEndpointAddress;
	}

	return desc->if0_int_ep.bEndpointAddress;
}

static uint8_t cdc_acm_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;
	struct usbd_cdc_acm_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if1_hs_in_ep.bEndpointAddress;
	}

	return desc->if1_in_ep.bEndpointAddress;
}

static uint8_t cdc_acm_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;
	struct usbd_cdc_acm_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if1_hs_out_ep.bEndpointAddress;
	}

	return desc->if1_out_ep.bEndpointAddress;
}

static size_t cdc_acm_get_bulk_mps(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return 512U;
	}

	return 64U;
}

static int usbd_cdc_acm_request(struct usbd_class_data *const c_data,
				struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);
	if (err) {
		if (err == -ECONNABORTED) {
			LOG_WRN("request ep 0x%02x, len %u cancelled",
				bi->ep, buf->len);
		} else {
			LOG_ERR("request ep 0x%02x, len %u failed",
				bi->ep, buf->len);
		}

		if (bi->ep == cdc_acm_get_bulk_out(c_data)) {
			atomic_clear_bit(&data->state, CDC_ACM_RX_FIFO_BUSY);
		}

		if (bi->ep == cdc_acm_get_bulk_in(c_data)) {
			atomic_clear_bit(&data->state, CDC_ACM_TX_FIFO_BUSY);
		}

		goto ep_request_error;
	}

	if (bi->ep == cdc_acm_get_bulk_out(c_data)) {
		/* RX transfer completion */
		size_t done;

		LOG_HEXDUMP_INF(buf->data, buf->len, "");
		done = ring_buf_put(data->rx_fifo.rb, buf->data, buf->len);
		if (done && data->cb) {
			cdc_acm_work_submit(&data->irq_cb_work);
		}

		atomic_clear_bit(&data->state, CDC_ACM_RX_FIFO_BUSY);
		cdc_acm_work_submit(&data->rx_fifo_work);
	}

	if (bi->ep == cdc_acm_get_bulk_in(c_data)) {
		/* TX transfer completion */
		if (data->cb) {
			cdc_acm_work_submit(&data->irq_cb_work);
		}

		atomic_clear_bit(&data->state, CDC_ACM_TX_FIFO_BUSY);

		if (!ring_buf_is_empty(data->tx_fifo.rb)) {
			/* Queue pending TX data on IN endpoint */
			cdc_acm_work_schedule(&data->tx_fifo_work, K_NO_WAIT);
		}

	}

	if (bi->ep == cdc_acm_get_int_in(c_data)) {
		k_sem_give(&data->notif_sem);
	}

ep_request_error:
	return usbd_ep_buf_free(uds_ctx, buf);
}

static void usbd_cdc_acm_update(struct usbd_class_data *const c_data,
				uint8_t iface, uint8_t alternate)
{
	LOG_DBG("New configuration, interface %u alternate %u",
		iface, alternate);
}

static void usbd_cdc_acm_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;

	atomic_set_bit(&data->state, CDC_ACM_CLASS_ENABLED);
	LOG_INF("Configuration enabled");

	if (atomic_test_bit(&data->state, CDC_ACM_IRQ_RX_ENABLED)) {
		cdc_acm_irq_rx_enable(dev);
	}

	if (atomic_test_bit(&data->state, CDC_ACM_IRQ_TX_ENABLED)) {
		if (ring_buf_space_get(data->tx_fifo.rb)) {
			/* Raise TX ready interrupt */
			cdc_acm_work_submit(&data->irq_cb_work);
		} else {
			/* Queue pending TX data on IN endpoint */
			cdc_acm_work_schedule(&data->tx_fifo_work, K_NO_WAIT);
		}
	}
}

static void usbd_cdc_acm_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;

	atomic_clear_bit(&data->state, CDC_ACM_CLASS_ENABLED);
	atomic_clear_bit(&data->state, CDC_ACM_CLASS_SUSPENDED);
	LOG_INF("Configuration disabled");
}

static void usbd_cdc_acm_suspended(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;

	/* FIXME: filter stray suspended events earlier */
	atomic_set_bit(&data->state, CDC_ACM_CLASS_SUSPENDED);
}

static void usbd_cdc_acm_resumed(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;

	atomic_clear_bit(&data->state, CDC_ACM_CLASS_SUSPENDED);
}

static void *usbd_cdc_acm_get_desc(struct usbd_class_data *const c_data,
				   const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
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
	};

	switch (data->line_coding.bParityType) {
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
	};

	cfg->flow_ctrl = data->flow_ctrl ? UART_CFG_FLOW_CTRL_RTS_CTS :
					   UART_CFG_FLOW_CTRL_NONE;
}

static void cdc_acm_update_linestate(struct cdc_acm_uart_data *const data)
{
	if (data->line_state & SET_CONTROL_LINE_STATE_RTS) {
		data->line_state_rts = true;
	} else {
		data->line_state_rts = false;
	}

	if (data->line_state & SET_CONTROL_LINE_STATE_DTR) {
		data->line_state_dtr = true;
	} else {
		data->line_state_dtr = false;
	}
}

static int usbd_cdc_acm_cth(struct usbd_class_data *const c_data,
			    const struct usb_setup_packet *const setup,
			    struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;
	size_t min_len;

	if (setup->bRequest == GET_LINE_CODING) {
		if (buf == NULL) {
			errno = -ENOMEM;
			return 0;
		}

		min_len = MIN(sizeof(data->line_coding), setup->wLength);
		net_buf_add_mem(buf, &data->line_coding, min_len);

		return 0;
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x unsupported",
		setup->bmRequestType, setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int usbd_cdc_acm_ctd(struct usbd_class_data *const c_data,
			    const struct usb_setup_packet *const setup,
			    const struct net_buf *const buf)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;
	size_t len;

	switch (setup->bRequest) {
	case SET_LINE_CODING:
		len = sizeof(data->line_coding);
		if (setup->wLength != len) {
			errno = -ENOTSUP;
			return 0;
		}

		memcpy(&data->line_coding, buf->data, len);
		cdc_acm_update_uart_cfg(data);
		usbd_msg_pub_device(uds_ctx, USBD_MSG_CDC_ACM_LINE_CODING, dev);
		return 0;

	case SET_CONTROL_LINE_STATE:
		data->line_state = setup->wValue;
		cdc_acm_update_linestate(data);
		usbd_msg_pub_device(uds_ctx, USBD_MSG_CDC_ACM_CONTROL_LINE_STATE, dev);
		return 0;

	default:
		break;
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x unsupported",
		setup->bmRequestType, setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int usbd_cdc_acm_init(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_acm_uart_data *data = dev->data;
	struct usbd_cdc_acm_desc *desc = data->desc;

	desc->iad.bFirstInterface = desc->if0.bInterfaceNumber;
	desc->if0_union.bControlInterface = desc->if0.bInterfaceNumber;
	desc->if0_union.bSubordinateInterface0 = desc->if1.bInterfaceNumber;

	return 0;
}

static int cdc_acm_send_notification(const struct device *dev,
				     const uint16_t serial_state)
{
	struct cdc_acm_notification notification = {
		.bmRequestType = 0xA1,
		.bNotificationType = USB_CDC_SERIAL_STATE,
		.wValue = 0,
		.wIndex = 0,
		.wLength = sys_cpu_to_le16(sizeof(uint16_t)),
		.data = sys_cpu_to_le16(serial_state),
	};
	struct cdc_acm_uart_data *data = dev->data;
	struct usbd_class_data *c_data = data->c_data;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	if (!atomic_test_bit(&data->state, CDC_ACM_CLASS_ENABLED)) {
		LOG_INF("USB configuration is not enabled");
		return -EACCES;
	}

	if (atomic_test_bit(&data->state, CDC_ACM_CLASS_SUSPENDED)) {
		LOG_INF("USB support is suspended (FIXME)");
		return -EACCES;
	}

	ep = cdc_acm_get_int_in(c_data);
	buf = usbd_ep_buf_alloc(c_data, ep, sizeof(struct cdc_acm_notification));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &notification, sizeof(struct cdc_acm_notification));
	ret = usbd_ep_enqueue(c_data, buf);
	/* FIXME: support for sync transfers */
	k_sem_take(&data->notif_sem, K_FOREVER);

	return ret;
}

/*
 * TX handler is triggered when the state of TX fifo has been altered.
 */
static void cdc_acm_tx_fifo_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct cdc_acm_uart_data *data;
	struct usbd_class_data *c_data;
	struct net_buf *buf;
	size_t len;
	int ret;

	data = CONTAINER_OF(dwork, struct cdc_acm_uart_data, tx_fifo_work);
	c_data = data->c_data;

	if (!atomic_test_bit(&data->state, CDC_ACM_CLASS_ENABLED)) {
		LOG_DBG("USB configuration is not enabled");
		return;
	}

	if (atomic_test_bit(&data->state, CDC_ACM_CLASS_SUSPENDED)) {
		LOG_INF("USB support is suspended (FIXME: submit rwup)");
		return;
	}

	if (atomic_test_and_set_bit(&data->state, CDC_ACM_TX_FIFO_BUSY)) {
		LOG_DBG("TX transfer already in progress");
		return;
	}

	buf = cdc_acm_buf_alloc(cdc_acm_get_bulk_in(c_data));
	if (buf == NULL) {
		atomic_clear_bit(&data->state, CDC_ACM_TX_FIFO_BUSY);
		cdc_acm_work_schedule(&data->tx_fifo_work, K_MSEC(1));
		return;
	}

	len = ring_buf_get(data->tx_fifo.rb, buf->data, buf->size);
	net_buf_add(buf, len);

	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue");
		net_buf_unref(buf);
		atomic_clear_bit(&data->state, CDC_ACM_TX_FIFO_BUSY);
	}
}

/*
 * RX handler should be conditionally triggered at:
 *  - (x) cdc_acm_irq_rx_enable()
 *  - (x) RX transfer completion
 *  - (x) the end of cdc_acm_irq_cb_handler
 *  - (x) USBD class API enable call
 *  - ( ) USBD class API resumed call (TODO)
 */
static void cdc_acm_rx_fifo_handler(struct k_work *work)
{
	struct cdc_acm_uart_data *data;
	struct usbd_class_data *c_data;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	data = CONTAINER_OF(work, struct cdc_acm_uart_data, rx_fifo_work);
	c_data = data->c_data;

	if (!atomic_test_bit(&data->state, CDC_ACM_CLASS_ENABLED) ||
	    atomic_test_bit(&data->state, CDC_ACM_CLASS_SUSPENDED)) {
		LOG_INF("USB configuration is not enabled or suspended");
		return;
	}

	if (ring_buf_space_get(data->rx_fifo.rb) < cdc_acm_get_bulk_mps(c_data)) {
		LOG_INF("RX buffer to small, throttle");
		return;
	}

	if (atomic_test_and_set_bit(&data->state, CDC_ACM_RX_FIFO_BUSY)) {
		LOG_WRN("RX transfer already in progress");
		return;
	}

	ep = cdc_acm_get_bulk_out(c_data);
	buf = cdc_acm_buf_alloc(ep);
	if (buf == NULL) {
		return;
	}

	/* Shrink the buffer size if operating on a full speed bus */
	buf->size = MIN(cdc_acm_get_bulk_mps(c_data), buf->size);

	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
	}
}

static void cdc_acm_irq_tx_enable(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	atomic_set_bit(&data->state, CDC_ACM_IRQ_TX_ENABLED);

	if (ring_buf_space_get(data->tx_fifo.rb)) {
		LOG_INF("tx_en: trigger irq_cb_work");
		cdc_acm_work_submit(&data->irq_cb_work);
	}
}

static void cdc_acm_irq_tx_disable(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	atomic_clear_bit(&data->state, CDC_ACM_IRQ_TX_ENABLED);
}

static void cdc_acm_irq_rx_enable(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	atomic_set_bit(&data->state, CDC_ACM_IRQ_RX_ENABLED);

	/* Permit buffer to be drained regardless of USB state */
	if (!ring_buf_is_empty(data->rx_fifo.rb)) {
		LOG_INF("rx_en: trigger irq_cb_work");
		cdc_acm_work_submit(&data->irq_cb_work);
	}

	if (!atomic_test_bit(&data->state, CDC_ACM_RX_FIFO_BUSY)) {
		LOG_INF("rx_en: trigger rx_fifo_work");
		cdc_acm_work_submit(&data->rx_fifo_work);
	}
}

static void cdc_acm_irq_rx_disable(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	atomic_clear_bit(&data->state, CDC_ACM_IRQ_RX_ENABLED);
}

static int cdc_acm_fifo_fill(const struct device *dev,
			     const uint8_t *const tx_data,
			     const int len)
{
	struct cdc_acm_uart_data *const data = dev->data;
	unsigned int lock;
	uint32_t done;

	if (!check_wq_ctx(dev)) {
		LOG_WRN("Invoked by inappropriate context");
		__ASSERT_NO_MSG(false);
		return 0;
	}

	lock = irq_lock();
	done = ring_buf_put(data->tx_fifo.rb, tx_data, len);
	irq_unlock(lock);
	if (done) {
		data->tx_fifo.altered = true;
	}

	LOG_INF("UART dev %p, len %d, remaining space %u",
		dev, len, ring_buf_space_get(data->tx_fifo.rb));

	return done;
}

static int cdc_acm_fifo_read(const struct device *dev,
			     uint8_t *const rx_data,
			     const int size)
{
	struct cdc_acm_uart_data *const data = dev->data;
	uint32_t len;

	LOG_INF("UART dev %p size %d length %u",
		dev, size, ring_buf_size_get(data->rx_fifo.rb));

	if (!check_wq_ctx(dev)) {
		LOG_WRN("Invoked by inappropriate context");
		__ASSERT_NO_MSG(false);
		return 0;
	}

	len = ring_buf_get(data->rx_fifo.rb, rx_data, size);
	if (len) {
		data->rx_fifo.altered = true;
	}

	return len;
}

static int cdc_acm_irq_tx_ready(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	if (check_wq_ctx(dev)) {
		if (data->tx_fifo.irq) {
			return ring_buf_space_get(data->tx_fifo.rb);
		}
	} else {
		LOG_WRN("Invoked by inappropriate context");
		__ASSERT_NO_MSG(false);
	}

	return 0;
}

static int cdc_acm_irq_rx_ready(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	if (check_wq_ctx(dev)) {
		if (data->rx_fifo.irq) {
			return 1;
		}
	} else {
		LOG_WRN("Invoked by inappropriate context");
		__ASSERT_NO_MSG(false);
	}


	return 0;
}

static int cdc_acm_irq_is_pending(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	if (check_wq_ctx(dev)) {
		if (data->tx_fifo.irq || data->rx_fifo.irq) {
			return 1;
		}
	} else {
		LOG_WRN("Invoked by inappropriate context");
		__ASSERT_NO_MSG(false);
	}

	return 0;
}

static int cdc_acm_irq_update(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	if (!check_wq_ctx(dev)) {
		LOG_WRN("Invoked by inappropriate context");
		__ASSERT_NO_MSG(false);
		return 0;
	}

	if (atomic_test_bit(&data->state, CDC_ACM_IRQ_RX_ENABLED) &&
	    !ring_buf_is_empty(data->rx_fifo.rb)) {
		data->rx_fifo.irq = true;
	} else {
		data->rx_fifo.irq = false;
	}

	if (atomic_test_bit(&data->state, CDC_ACM_IRQ_TX_ENABLED) &&
	    ring_buf_space_get(data->tx_fifo.rb)) {
		data->tx_fifo.irq = true;
	} else {
		data->tx_fifo.irq = false;
	}

	return 1;
}

/*
 * IRQ handler should be conditionally triggered for the TX path at:
 *  - cdc_acm_irq_tx_enable()
 *  - TX transfer completion
 *  - TX buffer is empty
 *  - USBD class API enable and resumed calls
 *
 * for RX path, if enabled, at:
 *  - cdc_acm_irq_rx_enable()
 *  - RX transfer completion
 *  - RX buffer is not empty
 */
static void cdc_acm_irq_cb_handler(struct k_work *work)
{
	struct cdc_acm_uart_data *data;
	struct usbd_class_data *c_data;

	data = CONTAINER_OF(work, struct cdc_acm_uart_data, irq_cb_work);
	c_data = data->c_data;

	if (data->cb == NULL) {
		LOG_ERR("IRQ callback is not set");
		return;
	}

	data->tx_fifo.altered = false;
	data->rx_fifo.altered = false;
	data->rx_fifo.irq = false;
	data->tx_fifo.irq = false;

	if (atomic_test_bit(&data->state, CDC_ACM_IRQ_RX_ENABLED) ||
	    atomic_test_bit(&data->state, CDC_ACM_IRQ_TX_ENABLED)) {
		data->cb(usbd_class_get_private(c_data), data->cb_data);
	}

	if (data->rx_fifo.altered) {
		LOG_DBG("rx fifo altered, submit work");
		cdc_acm_work_submit(&data->rx_fifo_work);
	}

	if (data->tx_fifo.altered) {
		LOG_DBG("tx fifo altered, submit work");
		if (!atomic_test_bit(&data->state, CDC_ACM_TX_FIFO_BUSY)) {
			cdc_acm_work_schedule(&data->tx_fifo_work, K_NO_WAIT);
		}
	}

	if (atomic_test_bit(&data->state, CDC_ACM_IRQ_RX_ENABLED) &&
	    !ring_buf_is_empty(data->rx_fifo.rb)) {
		LOG_DBG("rx irq pending, submit irq_cb_work");
		cdc_acm_work_submit(&data->irq_cb_work);
	}

	if (atomic_test_bit(&data->state, CDC_ACM_IRQ_TX_ENABLED) &&
	    ring_buf_space_get(data->tx_fifo.rb)) {
		LOG_DBG("tx irq pending, submit irq_cb_work");
		cdc_acm_work_submit(&data->irq_cb_work);
	}
}

static void cdc_acm_irq_callback_set(const struct device *dev,
				     const uart_irq_callback_user_data_t cb,
				     void *const cb_data)
{
	struct cdc_acm_uart_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = cb_data;
}

static int cdc_acm_poll_in(const struct device *dev, unsigned char *const c)
{
	struct cdc_acm_uart_data *const data = dev->data;
	uint32_t len;
	int ret = -1;

	if (ring_buf_is_empty(data->rx_fifo.rb)) {
		return ret;
	}

	len = ring_buf_get(data->rx_fifo.rb, c, 1);
	if (len) {
		cdc_acm_work_submit(&data->rx_fifo_work);
		ret = 0;
	}

	return ret;
}

static void cdc_acm_poll_out(const struct device *dev, const unsigned char c)
{
	struct cdc_acm_uart_data *const data = dev->data;
	unsigned int lock;
	uint32_t wrote;

	while (true) {
		lock = irq_lock();
		wrote = ring_buf_put(data->tx_fifo.rb, &c, 1);
		irq_unlock(lock);

		if (wrote == 1) {
			break;
		}

		if (k_is_in_isr() || !data->flow_ctrl) {
			LOG_WRN_ONCE("Ring buffer full, discard data");
			break;
		}

		k_msleep(1);
	}

	/* Schedule with minimal timeout to make it possible to send more than
	 * one byte per USB transfer. The latency increase is negligible while
	 * the increased throughput and reduced CPU usage is easily observable.
	 */
	cdc_acm_work_schedule(&data->tx_fifo_work, K_MSEC(1));
}

#ifdef CONFIG_UART_LINE_CTRL
static int cdc_acm_line_ctrl_set(const struct device *dev,
				 const uint32_t ctrl, const uint32_t val)
{
	struct cdc_acm_uart_data *const data = dev->data;
	uint32_t flag = 0;

	switch (ctrl) {
	case USB_CDC_LINE_CTRL_BAUD_RATE:
		/* Ignore since it can not be used for notification anyway */
		return 0;
	case USB_CDC_LINE_CTRL_DCD:
		flag = USB_CDC_SERIAL_STATE_RXCARRIER;
		break;
	case USB_CDC_LINE_CTRL_DSR:
		flag = USB_CDC_SERIAL_STATE_TXCARRIER;
		break;
	case USB_CDC_LINE_CTRL_BREAK:
		flag = USB_CDC_SERIAL_STATE_BREAK;
		break;
	case USB_CDC_LINE_CTRL_RING_SIGNAL:
		flag = USB_CDC_SERIAL_STATE_RINGSIGNAL;
		break;
	case USB_CDC_LINE_CTRL_FRAMING:
		flag = USB_CDC_SERIAL_STATE_FRAMING;
		break;
	case USB_CDC_LINE_CTRL_PARITY:
		flag = USB_CDC_SERIAL_STATE_PARITY;
		break;
	case USB_CDC_LINE_CTRL_OVER_RUN:
		flag = USB_CDC_SERIAL_STATE_OVERRUN;
		break;
	default:
		return -EINVAL;
	}

	if (val) {
		data->serial_state |= flag;
	} else {
		data->serial_state &= ~flag;
	}

	return cdc_acm_send_notification(dev, data->serial_state);
}

static int cdc_acm_line_ctrl_get(const struct device *dev,
				 const uint32_t ctrl, uint32_t *const val)
{
	struct cdc_acm_uart_data *const data = dev->data;

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		*val = data->uart_cfg.baudrate;
		return 0;
	case UART_LINE_CTRL_RTS:
		*val = data->line_state_rts;
		return 0;
	case UART_LINE_CTRL_DTR:
		*val = data->line_state_dtr;
		return 0;
	}

	return -ENOTSUP;
}
#endif

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int cdc_acm_configure(const struct device *dev,
			     const struct uart_config *const cfg)
{
	struct cdc_acm_uart_data *const data = dev->data;

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		data->flow_ctrl = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		data->flow_ctrl = true;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int cdc_acm_config_get(const struct device *dev,
			      struct uart_config *const cfg)
{
	struct cdc_acm_uart_data *const data = dev->data;

	memcpy(cfg, &data->uart_cfg, sizeof(struct uart_config));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int usbd_cdc_acm_init_wq(void)
{
	k_work_queue_init(&cdc_acm_work_q);
	k_work_queue_start(&cdc_acm_work_q, cdc_acm_stack,
			   K_KERNEL_STACK_SIZEOF(cdc_acm_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, NULL);

	return 0;
}

static int usbd_cdc_acm_preinit(const struct device *dev)
{
	struct cdc_acm_uart_data *const data = dev->data;

	ring_buf_reset(data->tx_fifo.rb);
	ring_buf_reset(data->rx_fifo.rb);

	k_thread_name_set(&cdc_acm_work_q.thread, "cdc_acm_work_q");

	k_work_init_delayable(&data->tx_fifo_work, cdc_acm_tx_fifo_handler);
	k_work_init(&data->rx_fifo_work, cdc_acm_rx_fifo_handler);
	k_work_init(&data->irq_cb_work, cdc_acm_irq_cb_handler);

	return 0;
}

static DEVICE_API(uart, cdc_acm_uart_api) = {
	.irq_tx_enable = cdc_acm_irq_tx_enable,
	.irq_tx_disable = cdc_acm_irq_tx_disable,
	.irq_tx_ready = cdc_acm_irq_tx_ready,
	.irq_rx_enable = cdc_acm_irq_rx_enable,
	.irq_rx_disable = cdc_acm_irq_rx_disable,
	.irq_rx_ready = cdc_acm_irq_rx_ready,
	.irq_is_pending = cdc_acm_irq_is_pending,
	.irq_update = cdc_acm_irq_update,
	.irq_callback_set = cdc_acm_irq_callback_set,
	.poll_in = cdc_acm_poll_in,
	.poll_out = cdc_acm_poll_out,
	.fifo_fill = cdc_acm_fifo_fill,
	.fifo_read = cdc_acm_fifo_read,
#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = cdc_acm_line_ctrl_set,
	.line_ctrl_get = cdc_acm_line_ctrl_get,
#endif
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = cdc_acm_configure,
	.config_get = cdc_acm_config_get,
#endif
};

struct usbd_class_api usbd_cdc_acm_api = {
	.request = usbd_cdc_acm_request,
	.update = usbd_cdc_acm_update,
	.enable = usbd_cdc_acm_enable,
	.disable = usbd_cdc_acm_disable,
	.suspended = usbd_cdc_acm_suspended,
	.resumed = usbd_cdc_acm_resumed,
	.control_to_host = usbd_cdc_acm_cth,
	.control_to_dev = usbd_cdc_acm_ctd,
	.init = usbd_cdc_acm_init,
	.get_desc = usbd_cdc_acm_get_desc,
};

#define CDC_ACM_DEFINE_DESCRIPTOR(n)						\
static struct usbd_cdc_acm_desc cdc_acm_desc_##n = {				\
	.iad = {								\
		.bLength = sizeof(struct usb_association_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,			\
		.bFirstInterface = 0,						\
		.bInterfaceCount = 0x02,					\
		.bFunctionClass = USB_BCC_CDC_CONTROL,				\
		.bFunctionSubClass = ACM_SUBCLASS,				\
		.bFunctionProtocol = 0,						\
		.iFunction = 0,							\
	},									\
										\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 1,						\
		.bInterfaceClass = USB_BCC_CDC_CONTROL,				\
		.bInterfaceSubClass = ACM_SUBCLASS,				\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if0_header = {								\
		.bFunctionLength = sizeof(struct cdc_header_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = HEADER_FUNC_DESC,				\
		.bcdCDC = sys_cpu_to_le16(USB_SRN_1_1),				\
	},									\
										\
	.if0_cm = {								\
		.bFunctionLength = sizeof(struct cdc_cm_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = CALL_MANAGEMENT_FUNC_DESC,		\
		.bmCapabilities = 0,						\
		.bDataInterface = 1,						\
	},									\
										\
	.if0_acm = {								\
		.bFunctionLength = sizeof(struct cdc_acm_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = ACM_FUNC_DESC,				\
		/* See CDC PSTN Subclass Chapter 5.3.2 */			\
		.bmCapabilities = BIT(1),					\
	},									\
										\
	.if0_union = {								\
		.bFunctionLength = sizeof(struct cdc_union_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UNION_FUNC_DESC,				\
		.bControlInterface = 0,						\
		.bSubordinateInterface0 = 1,					\
	},									\
										\
	.if0_int_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(CDC_ACM_DEFAULT_INT_EP_MPS),	\
		.bInterval = CDC_ACM_FS_INT_EP_INTERVAL,			\
	},									\
										\
	.if0_hs_int_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(CDC_ACM_DEFAULT_INT_EP_MPS),	\
		.bInterval = CDC_ACM_HS_INT_EP_INTERVAL,			\
	},									\
										\
	.if1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_CDC_DATA,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if1_in_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0,							\
	},									\
										\
	.if1_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0,							\
	},									\
										\
	.if1_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512U),			\
		.bInterval = 0,							\
	},									\
										\
	.if1_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512U),			\
		.bInterval = 0,							\
	},									\
										\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
const static struct usb_desc_header *cdc_acm_fs_desc_##n[] = {			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.iad,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_header,		\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_cm,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_acm,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_union,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_int_ep,		\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if1,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if1_in_ep,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if1_out_ep,		\
	(struct usb_desc_header *) &cdc_acm_desc_##n.nil_desc,			\
};										\
										\
const static struct usb_desc_header *cdc_acm_hs_desc_##n[] = {			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.iad,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_header,		\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_cm,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_acm,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_union,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if0_hs_int_ep,		\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if1,			\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if1_hs_in_ep,		\
	(struct usb_desc_header *) &cdc_acm_desc_##n.if1_hs_out_ep,		\
	(struct usb_desc_header *) &cdc_acm_desc_##n.nil_desc,			\
}

#define USBD_CDC_ACM_DT_DEVICE_DEFINE(n)					\
	BUILD_ASSERT(DT_INST_ON_BUS(n, usb),					\
		     "node " DT_NODE_PATH(DT_DRV_INST(n))			\
		     " is not assigned to a USB device controller");		\
										\
	CDC_ACM_DEFINE_DESCRIPTOR(n);						\
										\
	USBD_DEFINE_CLASS(cdc_acm_##n,						\
			  &usbd_cdc_acm_api,					\
			  (void *)DEVICE_DT_GET(DT_DRV_INST(n)), NULL);		\
										\
	RING_BUF_DECLARE(cdc_acm_rb_rx_##n, DT_INST_PROP(n, rx_fifo_size));	\
	RING_BUF_DECLARE(cdc_acm_rb_tx_##n, DT_INST_PROP(n, tx_fifo_size));	\
										\
	static struct cdc_acm_uart_data uart_data_##n = {			\
		.line_coding = CDC_ACM_DEFAULT_LINECODING,			\
		.c_data = &cdc_acm_##n,						\
		.rx_fifo.rb = &cdc_acm_rb_rx_##n,				\
		.tx_fifo.rb = &cdc_acm_rb_tx_##n,				\
		.flow_ctrl = DT_INST_PROP(n, hw_flow_control),			\
		.notif_sem = Z_SEM_INITIALIZER(uart_data_##n.notif_sem, 0, 1),	\
		.desc = &cdc_acm_desc_##n,					\
		.fs_desc = cdc_acm_fs_desc_##n,					\
		.hs_desc = cdc_acm_hs_desc_##n,					\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, usbd_cdc_acm_preinit, NULL,			\
		&uart_data_##n, NULL,						\
		PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,			\
		&cdc_acm_uart_api);

DT_INST_FOREACH_STATUS_OKAY(USBD_CDC_ACM_DT_DEVICE_DEFINE);

SYS_INIT(usbd_cdc_acm_init_wq, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
