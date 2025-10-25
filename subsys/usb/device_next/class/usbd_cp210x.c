/*
 * Copyright (c) 2025 Sergey Matsievskiy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_cp210x_uart

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cp210x.h>

#include <zephyr/drivers/usb/udc.h>

#include "usbd_msg.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usbd_cp210x, CONFIG_USBD_CP210X_LOG_LEVEL);

#define CP210X_DEFAULT_LINECODING			\
	.baudrate = 115200,				\
	.parity = UART_CFG_PARITY_NONE,			\
	.stop_bits = UART_CFG_STOP_BITS_1,		\
	.data_bits = UART_CFG_DATA_BITS_8,		\
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE

enum {
	CP210X_CLASS_ENABLED,
	CP210X_CLASS_SUSPENDED,
	CP210X_IRQ_RX_ENABLED,
	CP210X_IRQ_TX_ENABLED,
	CP210X_RX_FIFO_BUSY,
	CP210X_TX_FIFO_BUSY,
};

struct cp210x_uart_fifo {
	struct ring_buf *rb;
	bool irq;
	bool altered;
};

struct usbd_cp210x_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_desc_header nil_desc;
};

struct cp210x_uart_config {
	/* Pointer to the associated USBD class node */
	struct usbd_class_data *c_data;
	/* Pointer to the interface description node or NULL */
	struct usbd_desc_node *const if_desc_data;
	/* Pointer to the class interface descriptors */
	struct usbd_cp210x_desc *const desc;
	const struct usb_desc_header **const fs_desc;
};

struct cp210x_uart_data {
	const struct device *dev;
	/* Serial state bitmap */
	uint16_t serial_state;
	/* UART actual configuration */
	struct uart_config uart_cfg;
	/* UART actual RTS state */
	bool line_state_rts;
	/* UART actual DTR state */
	bool line_state_dtr;
	/* When flow_ctrl is set, poll out is blocked when the buffer is full,
	 * roughly emulating flow control.
	 */
	bool flow_ctrl;
	bool zlp_needed;
	/* UART API IRQ callback */
	uart_irq_callback_user_data_t cb;
	/* UART API user callback data */
	void *cb_data;
	/* UART API IRQ callback work */
	struct k_work irq_cb_work;
	struct cp210x_uart_fifo rx_fifo;
	struct cp210x_uart_fifo tx_fifo;
	/* USBD CDC ACM TX fifo work */
	struct k_work_delayable tx_fifo_work;
	/* USBD CDC ACM RX fifo work */
	struct k_work rx_fifo_work;
	atomic_t state;
	struct k_spinlock lock;
};

static void cp210x_irq_rx_enable(const struct device *dev);

#if CONFIG_USBD_CP210X_BUF_POOL
UDC_BUF_POOL_DEFINE(cp210x_ep_pool,
		    DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 2,
		    USBD_MAX_BULK_MPS, sizeof(struct udc_buf_info), NULL);

static struct net_buf *cp210x_buf_alloc(struct usbd_class_data *const c_data, const uint8_t ep)
{
	ARG_UNUSED(c_data);
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&cp210x_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	return buf;
}
#else
/*
 * The required buffer is 128 bytes per instance on a full-speed device. Use
 * common (UDC) buffer, as this results in a smaller footprint.
 */
static struct net_buf *cp210x_buf_alloc(struct usbd_class_data *const c_data, const uint8_t ep)
{
	return usbd_ep_buf_alloc(c_data, ep, USBD_MAX_BULK_MPS);
}
#endif /* CONFIG_USBD_CP210X_BUF_POOL */

#if CONFIG_USBD_CP210X_WORKQUEUE
static struct k_work_q cp210x_work_q;
static K_KERNEL_STACK_DEFINE(cp210x_stack,
			     CONFIG_USBD_CP210X_STACK_SIZE);

static int usbd_cp210x_init_wq(void)
{
	k_work_queue_init(&cp210x_work_q);
	k_work_queue_start(&cp210x_work_q, cp210x_stack,
			   K_KERNEL_STACK_SIZEOF(cp210x_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, NULL);
	k_thread_name_set(&cp210x_work_q.thread, "cp210x_work_q");

	return 0;
}

SYS_INIT(usbd_cp210x_init_wq, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static ALWAYS_INLINE int cp210x_work_submit(struct k_work *work)
{
	return k_work_submit_to_queue(&cp210x_work_q, work);
}

static ALWAYS_INLINE int cp210x_work_schedule(struct k_work_delayable *work, k_timeout_t delay)
{
	return k_work_schedule_for_queue(&cp210x_work_q, work, delay);
}

static ALWAYS_INLINE bool check_wq_ctx(const struct device *dev)
{
	return k_current_get() == k_work_queue_thread_get(&cp210x_work_q);
}

#else /* Use system workqueue */

static ALWAYS_INLINE int cp210x_work_submit(struct k_work *work)
{
	return k_work_submit(work);
}

static ALWAYS_INLINE int cp210x_work_schedule(struct k_work_delayable *work, k_timeout_t delay)
{
	return k_work_schedule(work, delay);
}

#define check_wq_ctx(dev) true

#endif /* CONFIG_USBD_CP210X_WORKQUEUE */

static uint8_t cp210x_get_bulk_in(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct cp210x_uart_config *cfg = dev->config;
	struct usbd_cp210x_desc *desc = cfg->desc;

	return desc->if0_in_ep.bEndpointAddress;
}

static uint8_t cp210x_get_bulk_out(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct cp210x_uart_config *cfg = dev->config;
	struct usbd_cp210x_desc *desc = cfg->desc;

	return desc->if0_out_ep.bEndpointAddress;
}

static size_t cp210x_get_bulk_mps(struct usbd_class_data *const c_data)
{
	return 64U;
}

static int usbd_cp210x_request(struct usbd_class_data *const c_data,
			       struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cp210x_uart_data *data = dev->data;
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

		if (bi->ep == cp210x_get_bulk_out(c_data)) {
			atomic_clear_bit(&data->state, CP210X_RX_FIFO_BUSY);
		}

		if (bi->ep == cp210x_get_bulk_in(c_data)) {
			atomic_clear_bit(&data->state, CP210X_TX_FIFO_BUSY);
		}

		goto ep_request_error;
	}

	if (bi->ep == cp210x_get_bulk_out(c_data)) {
		/* RX transfer completion */
		size_t done;

		LOG_HEXDUMP_INF(buf->data, buf->len, "");
		done = ring_buf_put(data->rx_fifo.rb, buf->data, buf->len);
		if (done && data->cb) {
			cp210x_work_submit(&data->irq_cb_work);
		}

		atomic_clear_bit(&data->state, CP210X_RX_FIFO_BUSY);
		cp210x_work_submit(&data->rx_fifo_work);
	}

	if (bi->ep == cp210x_get_bulk_in(c_data)) {
		/* TX transfer completion */
		if (data->cb) {
			cp210x_work_submit(&data->irq_cb_work);
		}

		atomic_clear_bit(&data->state, CP210X_TX_FIFO_BUSY);

		if (!ring_buf_is_empty(data->tx_fifo.rb)) {
			/* Queue pending TX data on IN endpoint */
			cp210x_work_schedule(&data->tx_fifo_work, K_NO_WAIT);
		}
	}

ep_request_error:
	return usbd_ep_buf_free(uds_ctx, buf);
}

static void usbd_cp210x_update(struct usbd_class_data *const c_data,
			       uint8_t iface, uint8_t alternate)
{
	LOG_DBG("New configuration, interface %u alternate %u",
		iface, alternate);
}

static void usbd_cp210x_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cp210x_uart_data *data = dev->data;

	atomic_set_bit(&data->state, CP210X_CLASS_ENABLED);
	LOG_INF("Configuration enabled");

	if (atomic_test_bit(&data->state, CP210X_IRQ_RX_ENABLED)) {
		cp210x_irq_rx_enable(dev);
	}

	if (atomic_test_bit(&data->state, CP210X_IRQ_TX_ENABLED)) {
		if (ring_buf_space_get(data->tx_fifo.rb)) {
			/* Raise TX ready interrupt */
			cp210x_work_submit(&data->irq_cb_work);
		} else {
			/* Queue pending TX data on IN endpoint */
			cp210x_work_schedule(&data->tx_fifo_work, K_NO_WAIT);
		}
	}
}

static void usbd_cp210x_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cp210x_uart_data *data = dev->data;

	atomic_clear_bit(&data->state, CP210X_CLASS_ENABLED);
	atomic_clear_bit(&data->state, CP210X_CLASS_SUSPENDED);
	LOG_INF("Configuration disabled");
}

static void usbd_cp210x_suspended(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cp210x_uart_data *data = dev->data;

	/* FIXME: filter stray suspended events earlier */
	atomic_set_bit(&data->state, CP210X_CLASS_SUSPENDED);
}

static void usbd_cp210x_resumed(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cp210x_uart_data *data = dev->data;

	atomic_clear_bit(&data->state, CP210X_CLASS_SUSPENDED);
}

static void *usbd_cp210x_get_desc(struct usbd_class_data *const c_data,
				  const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct cp210x_uart_config *cfg = dev->config;

	return cfg->fs_desc;
}

static int usbd_cp210x_cth(struct usbd_class_data *const c_data,
			   const struct usb_setup_packet *const setup,
			   struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cp210x_uart_data *data = dev->data;
	struct uart_config *const cfg = &data->uart_cfg;

	switch (setup->bRequest) {
	case USB_CP210X_GET_BAUDDIV:
		net_buf_add_le16(buf, USB_CP210X_BAUDDIV_FREQ / cfg->baudrate);
		return 0;
	case USB_CP210X_GET_BAUDRATE:
		net_buf_add_le32(buf, cfg->baudrate);
		return 0;
	case USB_CP210X_GET_LINE_CTL: {
		union usb_cp210x_line_ctl lctrl = {0};

		switch (cfg->stop_bits) {
		case UART_CFG_STOP_BITS_1:
			lctrl.fld.stop_bits = USB_CP210X_BITS_STOP_1;
			break;
		case UART_CFG_STOP_BITS_1_5:
			lctrl.fld.stop_bits = USB_CP210X_BITS_STOP_1_5;
			break;
		case UART_CFG_STOP_BITS_2:
			lctrl.fld.stop_bits = USB_CP210X_BITS_STOP_2;
			break;
		default:
			break;
		}

		switch (cfg->stop_bits) {
		case UART_CFG_PARITY_NONE:
			lctrl.fld.parity = USB_CP210X_BITS_PARITY_NONE;
			break;
		case UART_CFG_PARITY_ODD:
			lctrl.fld.parity = USB_CP210X_BITS_PARITY_ODD;
			break;
		case UART_CFG_PARITY_EVEN:
			lctrl.fld.parity = USB_CP210X_BITS_PARITY_EVEN;
			break;
		case UART_CFG_PARITY_MARK:
			lctrl.fld.parity = USB_CP210X_BITS_PARITY_MARK;
			break;
		case UART_CFG_PARITY_SPACE:
			lctrl.fld.parity = USB_CP210X_BITS_PARITY_SPACE;
			break;
		default:
			break;
		}

		switch (cfg->stop_bits) {
		case UART_CFG_DATA_BITS_5:
			lctrl.fld.stop_bits = USB_CP210X_BITS_DATA_5;
			break;
		case UART_CFG_DATA_BITS_6:
			lctrl.fld.stop_bits = USB_CP210X_BITS_DATA_6;
			break;
		case UART_CFG_DATA_BITS_7:
			lctrl.fld.stop_bits = USB_CP210X_BITS_DATA_7;
			break;
		case UART_CFG_DATA_BITS_8:
			lctrl.fld.stop_bits = USB_CP210X_BITS_DATA_8;
			break;
		default:
			break;
		}

		net_buf_add_le16(buf, lctrl.val);
		return 0;
	}
	case USB_CP210X_GET_MDMSTS: {
		union usb_cp210x_mdmsts mdmsts = {
			.fld = {
				.dtr = data->line_state_dtr,
				.rts = data->line_state_rts,
				.cts = true,
				.dsr = true,
			},
		};

		net_buf_add_u8(buf, mdmsts.val);
		return 0;
	}
	case USB_CP210X_GET_FLOW: {
		struct usb_cp210x_flow_control flow_ctl = {
			.ulControlHandshake = {
				.fld = {
					.dtr_mask = USB_CP210X_FCS_DTR_MASK_INACTIVE,
				},
			},
		};

		sys_put_le(&flow_ctl, &flow_ctl, sizeof(flow_ctl));
		net_buf_add_mem(buf, &flow_ctl, sizeof(flow_ctl));
		return 0;
	}
	case USB_CP210X_GET_EVENTMASK: {
		union usb_cp210x_event event = {0};

		sys_cpu_to_le(&event, sizeof(event));
		net_buf_add_mem(buf, &event, sizeof(event));
		return 0;
	}
	case USB_CP210X_GET_EVENTSTATE: {
		union usb_cp210x_event event = {0};

		sys_cpu_to_le(&event, sizeof(event));
		net_buf_add_mem(buf, &event, sizeof(event));
		return 0;
	}
	case USB_CP210X_GET_COMM_STATUS: {
		struct usb_cp210x_serial_status status = {
			.ulErrors = {
					.fld = {
							.break_event = false,
							.framing_error = false,
							.hardware_overrun = false,
							.queue_overrun = false,
							.parity_error = false,
						},
				},
			.ulHoldReasons = {
					.fld = {
							.wait_cts = false,
							.wait_dsr = false,
							.wait_dsd = false,
							.wait_xon = false,
							.wait_xoff = false,
							.wait_break = false,
							.wait_dsr_rcv = false,
						},
				},
			.ulAmountInInQueue = ring_buf_size_get(data->rx_fifo.rb),
			.ulAmountInOutQueue = ring_buf_size_get(data->tx_fifo.rb),
			.bEofReceived = 0,
			.bWaitForImmediate = 0,
		};

		sys_cpu_to_le(&status, sizeof(status));
		net_buf_add_mem(buf, &status, sizeof(status));
		return 0;
	}
	case USB_CP210X_GET_CHARS: {
		union usb_cp210x_char_vals chars = {0};

		net_buf_add_mem(buf, &chars, sizeof(chars));
		return 0;
	}
	case USB_CP210X_GET_PROPS: {
		struct usb_cp210x_props props = {
			.wLength = sizeof(struct usb_cp210x_props),
			.bcdVersion = USB_CP210X_PROPS_BSD_VERSION,
			.ulServiceMask = USB_CP210X_PROPS_SERVICE_MASK,
			.ulMaxTxQueue = ring_buf_size_get(data->tx_fifo.rb),
			.ulMaxRxQueue = ring_buf_size_get(data->rx_fifo.rb),
			.ulMaBaud = USB_CP210X_PROPS_MAX_BAUD,
			.ulProvSubType = USB_CP210X_PROPS_PROVSUBTYPE_RS232,
			.ulProvCapabilities = {
					.fld = {
							.dtr_dsr_support = false,
							.rts_cts_support = false,
							.dcd_support = false,
							.can_check_parity = false,
							.xon_xoff_support = false,
							.can_set_xon_xoff_characters = false,
							.can_set_special_characters = false,
							.bit16_mode_supports = false,
						},
				},
			.ulSettableParams = {
					.fld = {
							.can_set_parity_type = true,
							.can_set_baud = true,
							.can_set_number_of_data_bits = true,
							.can_set_stop_bits = true,
							.can_set_handshaking = false,
							.can_set_parity_checking = true,
							.can_set_carrier_detect_checking = false,
						},
				},
			.ulSettableBaud = {
					.fld = {
							.baud_75 = true,
							.baud_110 = true,
							.baud_134_5 = true,
							.baud_150 = true,
							.baud_300 = true,
							.baud_600 = true,
							.baud_1200 = true,
							.baud_1800 = true,
							.baud_2400 = true,
							.baud_4800 = true,
							.baud_7200 = false,
							.baud_9600 = true,
							.baud_14400 = false,
							.baud_19200 = true,
							.baud_38400 = true,
							.baud_56000 = false,
							.baud_128000 = false,
							.baud_115200 = true,
							.baud_57600 = true,
						},
				},
			.wSettableData = {
					.fld = {
							.data_bits_5 = true,
							.data_bits_6 = true,
							.data_bits_7 = true,
							.data_bits_8 = true,
							.data_bits_16 = true,
						},
				},
			.ulCurrentTxQueue = ring_buf_space_get(data->tx_fifo.rb),
			.ulCurrentRxQueue = ring_buf_space_get(data->rx_fifo.rb),
			.uniProvName = "SILABS USB V1.0",
		};

		sys_cpu_to_le(&props, sizeof(props));
		net_buf_add_mem(buf, &props, sizeof(props));
		return 0;
	}
	default:
		break;
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x unsupported",
		setup->bmRequestType, setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int usbd_cp210x_ctd(struct usbd_class_data *const c_data,
			   const struct usb_setup_packet *const setup,
			   const struct net_buf *const buf)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cp210x_uart_data *data = dev->data;
	struct uart_config *const cfg = &data->uart_cfg;

	switch (setup->bRequest) {
	case USB_CP210X_IFC_ENABLE:
		switch (setup->wValue) {
		case USB_CP210X_ENABLE:
			usbd_cp210x_enable(c_data);
			return 0;
		case USB_CP210X_DISABLE:
			usbd_cp210x_disable(c_data);
			return 0;
		default:
			break;
		}
		break;
	case USB_CP210X_RESET:
		/* compatibility placeholder */
		return 0;
	case USB_CP210X_SET_BAUDDIV:
		cfg->baudrate = USB_CP210X_BAUDDIV_FREQ / setup->wValue;
		usbd_msg_pub_device(uds_ctx, USBD_MSG_CDC_ACM_LINE_CODING, dev);
		return 0;
	case USB_CP210X_SET_BAUDRATE:
		cfg->baudrate = sys_get_le32(buf->data);
		usbd_msg_pub_device(uds_ctx, USBD_MSG_CDC_ACM_LINE_CODING, dev);
		return 0;
	case USB_CP210X_SET_LINE_CTL:
		switch (FIELD_GET(USB_CP210X_BITS_STOP, setup->wValue)) {
		case USB_CP210X_BITS_STOP_1:
			cfg->stop_bits = UART_CFG_STOP_BITS_1;
			break;
		case USB_CP210X_BITS_STOP_1_5:
			cfg->stop_bits = UART_CFG_STOP_BITS_1_5;
			break;
		case USB_CP210X_BITS_STOP_2:
			cfg->stop_bits = UART_CFG_STOP_BITS_2;
			break;
		default:
			break;
		}

		switch (FIELD_GET(USB_CP210X_BITS_PARITY, setup->wValue)) {
		case USB_CP210X_BITS_PARITY_NONE:
			cfg->stop_bits = UART_CFG_PARITY_NONE;
			break;
		case USB_CP210X_BITS_PARITY_ODD:
			cfg->stop_bits = UART_CFG_PARITY_ODD;
			break;
		case USB_CP210X_BITS_PARITY_EVEN:
			cfg->stop_bits = UART_CFG_PARITY_EVEN;
			break;
		case USB_CP210X_BITS_PARITY_MARK:
			cfg->stop_bits = UART_CFG_PARITY_MARK;
			break;
		case USB_CP210X_BITS_PARITY_SPACE:
			cfg->stop_bits = UART_CFG_PARITY_SPACE;
			break;
		default:
			break;
		}

		switch (FIELD_GET(USB_CP210X_BITS_DATA, setup->wValue)) {
		case USB_CP210X_BITS_DATA_5:
			cfg->stop_bits = UART_CFG_DATA_BITS_5;
			break;
		case USB_CP210X_BITS_DATA_6:
			cfg->stop_bits = UART_CFG_DATA_BITS_6;
			break;
		case USB_CP210X_BITS_DATA_7:
			cfg->stop_bits = UART_CFG_DATA_BITS_7;
			break;
		case USB_CP210X_BITS_DATA_8:
			cfg->stop_bits = UART_CFG_DATA_BITS_8;
			break;
		default:
			break;
		}

		usbd_msg_pub_device(uds_ctx, USBD_MSG_CDC_ACM_LINE_CODING, dev);
		return 0;
	case USB_CP210X_SET_MHS: {
		union usb_cp210x_mhs mhs;

		mhs.val = sys_cpu_to_le16(setup->wValue);
		if (mhs.fld.dtr_mask) {
			data->line_state_dtr = mhs.fld.dtr_state;
		}
		if (mhs.fld.rts_mask) {
			data->line_state_rts = mhs.fld.rts_state;
		}
		return 0;
	}
	case USB_CP210X_SET_FLOW: {
		struct usb_cp210x_flow_control flow_ctrl;

		if (setup->wLength != sizeof(flow_ctrl)) {
			errno = -ENOTSUP;
			return 0;
		}
		sys_put_le(&flow_ctrl, buf->data, setup->wLength);
		return 0;
	}
	case USB_CP210X_SET_XON:
		/* FIXME: add xon/xoff support */
		return 0;
	case USB_CP210X_SET_XOFF:
		/* FIXME: add xon/xoff support */
		return 0;
	case USB_CP210X_SET_EVENTMASK: {
		union usb_cp210x_event event;

		if (setup->wLength != sizeof(event)) {
			errno = -ENOTSUP;
			return 0;
		}
		sys_get_le(&event, buf->data, setup->wLength);
		return 0;
	}
	case USB_CP210X_SET_RECEIVE:
		/* not supported for cp2101 */
		return 0;
	case USB_CP210X_SET_BREAK:
		return 0;
	case USB_CP210X_IMM_CHAR:
		return 0;
	case USB_CP210X_SET_CHAR: {
		struct usb_cp210x_char chr;

		sys_get_le(&chr, &setup->wValue, sizeof(chr));
		if (chr.char_idx < 6) {
			errno = -ENOTSUP;
			return 0;
		}
		return 0;
	}
	case USB_CP210X_SET_CHARS: {
		union usb_cp210x_char_vals chars = {0};

		if (setup->wLength != sizeof(chars)) {
			errno = -ENOTSUP;
			return 0;
		}
		memcpy(&chars, buf->data, setup->wLength);
		return 0;
	}
	case USB_CP210X_PURGE: {
		union usb_cp210x_purge purge;

		purge.val = sys_cpu_to_le16(setup->wValue);
		if (purge.fld.rx1 && purge.fld.rx2) {
			ring_buf_reset(data->rx_fifo.rb);
		}
		if (purge.fld.tx1 && purge.fld.tx2) {
			ring_buf_reset(data->tx_fifo.rb);
		}
		return 0;
	}
	case USB_CP210X_EMBED_EVENTS: {
		return 0;
	}
	default:
		break;
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x unsupported",
		setup->bmRequestType, setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int usbd_cp210x_vendor_cth(const struct usbd_context *const ctx,
				  const struct usb_setup_packet *const setup,
				  struct net_buf *const buf)
{
	if (setup->bRequest != USB_CP210X_VENDOR_SPECIFIC) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->wValue == USB_CP210X_GET_PARTNUM) {
		net_buf_add_u8(buf, USB_CP210X_PARTNUM_CP2101);
		return 0;
	}

	LOG_DBG("vendor request bRequest 0x%02x unsupported",
		setup->wValue);
	errno = -ENOTSUP;

	return 0;
}

static int usbd_cp210x_vendor_ctd(const struct usbd_context *const ctx,
				  const struct usb_setup_packet *const setup,
				  const struct net_buf *const buf)
{
	LOG_DBG("vendor request bRequest 0x%02x unsupported",
		setup->wValue);
	errno = -ENOTSUP;

	return 0;
}

USBD_VREQUEST_DEFINE(sample_vrequest, 0xff, usbd_cp210x_vendor_cth, usbd_cp210x_vendor_ctd);

static int usbd_cp210x_init(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	const struct cp210x_uart_config *cfg = dev->config;
	struct usbd_cp210x_desc *desc = cfg->desc;

	if (cfg->if_desc_data && desc->if0.iInterface == 0) {
		if (usbd_add_descriptor(uds_ctx, cfg->if_desc_data)) {
			LOG_ERR("Failed to add interface string descriptor");
		} else {
			desc->if0.iInterface = usbd_str_desc_get_idx(cfg->if_desc_data);
		}
	}

	usbd_device_register_vreq(uds_ctx, &sample_vrequest);

	return 0;
}

/*
 * TX handler is triggered when the state of TX fifo has been altered.
 */
static void cp210x_tx_fifo_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct cp210x_uart_data *data;
	const struct cp210x_uart_config *cfg;
	struct usbd_class_data *c_data;
	struct net_buf *buf;
	size_t len;
	int ret;

	data = CONTAINER_OF(dwork, struct cp210x_uart_data, tx_fifo_work);
	cfg = data->dev->config;
	c_data = cfg->c_data;

	if (!atomic_test_bit(&data->state, CP210X_CLASS_ENABLED)) {
		LOG_DBG("USB configuration is not enabled");
		return;
	}

	if (atomic_test_bit(&data->state, CP210X_CLASS_SUSPENDED)) {
		LOG_INF("USB support is suspended (FIXME: submit rwup)");
		return;
	}

	if (atomic_test_and_set_bit(&data->state, CP210X_TX_FIFO_BUSY)) {
		LOG_DBG("TX transfer already in progress");
		return;
	}

	buf = cp210x_buf_alloc(c_data, cp210x_get_bulk_in(c_data));
	if (!buf) {
		atomic_clear_bit(&data->state, CP210X_TX_FIFO_BUSY);
		cp210x_work_schedule(&data->tx_fifo_work, K_MSEC(1));
		return;
	}

	len = ring_buf_get(data->tx_fifo.rb, buf->data, buf->size);
	net_buf_add(buf, len);

	data->zlp_needed = len != 0 && len % cp210x_get_bulk_mps(c_data) == 0;

	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue");
		net_buf_unref(buf);
		atomic_clear_bit(&data->state, CP210X_TX_FIFO_BUSY);
	}
}

/*
 * RX handler should be conditionally triggered at:
 *  - (x) cp210x_irq_rx_enable()
 *  - (x) RX transfer completion
 *  - (x) the end of cp210x_irq_cb_handler
 *  - (x) USBD class API enable call
 *  - ( ) USBD class API resumed call (TODO)
 */
static void cp210x_rx_fifo_handler(struct k_work *work)
{
	struct cp210x_uart_data *data;
	const struct cp210x_uart_config *cfg;
	struct usbd_class_data *c_data;
	struct net_buf *buf;
	int ret;

	data = CONTAINER_OF(work, struct cp210x_uart_data, rx_fifo_work);
	cfg = data->dev->config;
	c_data = cfg->c_data;

	if (!atomic_test_bit(&data->state, CP210X_CLASS_ENABLED) ||
	    atomic_test_bit(&data->state, CP210X_CLASS_SUSPENDED)) {
		LOG_INF("USB configuration is not enabled or suspended");
		return;
	}

	if (ring_buf_space_get(data->rx_fifo.rb) < cp210x_get_bulk_mps(c_data)) {
		LOG_INF("RX buffer to small, throttle");
		return;
	}

	if (atomic_test_and_set_bit(&data->state, CP210X_RX_FIFO_BUSY)) {
		LOG_WRN("RX transfer already in progress");
		return;
	}

	buf = cp210x_buf_alloc(c_data, cp210x_get_bulk_out(c_data));
	if (!buf) {
		return;
	}

	/* Shrink the buffer size if operating on a full speed bus */
	buf->size = MIN(cp210x_get_bulk_mps(c_data), buf->size);

	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x",
			cp210x_get_bulk_out(c_data));
		net_buf_unref(buf);
	}
}

static void cp210x_irq_tx_enable(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

	atomic_set_bit(&data->state, CP210X_IRQ_TX_ENABLED);

	if (ring_buf_space_get(data->tx_fifo.rb)) {
		LOG_INF("tx_en: trigger irq_cb_work");
		cp210x_work_submit(&data->irq_cb_work);
	}
}

static void cp210x_irq_tx_disable(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

	atomic_clear_bit(&data->state, CP210X_IRQ_TX_ENABLED);
}

static void cp210x_irq_rx_enable(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

	atomic_set_bit(&data->state, CP210X_IRQ_RX_ENABLED);

	/* Permit buffer to be drained regardless of USB state */
	if (!ring_buf_is_empty(data->rx_fifo.rb)) {
		LOG_INF("rx_en: trigger irq_cb_work");
		cp210x_work_submit(&data->irq_cb_work);
	}

	if (!atomic_test_bit(&data->state, CP210X_RX_FIFO_BUSY)) {
		LOG_INF("rx_en: trigger rx_fifo_work");
		cp210x_work_submit(&data->rx_fifo_work);
	}
}

static void cp210x_irq_rx_disable(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

	atomic_clear_bit(&data->state, CP210X_IRQ_RX_ENABLED);
}

static int cp210x_fifo_fill(const struct device *dev,
			    const uint8_t *const tx_data,
			    const int len)
{
	struct cp210x_uart_data *const data = dev->data;
	k_spinlock_key_t key;
	uint32_t done;

	if (!check_wq_ctx(dev)) {
		LOG_WRN("Invoked by inappropriate context");
		__ASSERT_NO_MSG(false);
		return 0;
	}

	key = k_spin_lock(&data->lock);
	done = ring_buf_put(data->tx_fifo.rb, tx_data, len);
	k_spin_unlock(&data->lock, key);
	if (done) {
		data->tx_fifo.altered = true;
	}

	LOG_INF("UART dev %p, len %d, remaining space %u",
		dev, len, ring_buf_space_get(data->tx_fifo.rb));

	return done;
}

static int cp210x_fifo_read(const struct device *dev,
			    uint8_t *const rx_data,
			    const int size)
{
	struct cp210x_uart_data *const data = dev->data;
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

static int cp210x_irq_tx_ready(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

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

static int cp210x_irq_rx_ready(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

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

static int cp210x_irq_is_pending(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

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

static int cp210x_irq_update(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

	if (!check_wq_ctx(dev)) {
		LOG_WRN("Invoked by inappropriate context");
		__ASSERT_NO_MSG(false);
		return 0;
	}

	if (atomic_test_bit(&data->state, CP210X_IRQ_RX_ENABLED) &&
	    !ring_buf_is_empty(data->rx_fifo.rb)) {
		data->rx_fifo.irq = true;
	} else {
		data->rx_fifo.irq = false;
	}

	if (atomic_test_bit(&data->state, CP210X_IRQ_TX_ENABLED) &&
	    ring_buf_space_get(data->tx_fifo.rb)) {
		data->tx_fifo.irq = true;
	} else {
		data->tx_fifo.irq = false;
	}

	return 1;
}

/*
 * IRQ handler should be conditionally triggered for the TX path at:
 *  - cp210x_irq_tx_enable()
 *  - TX transfer completion
 *  - TX buffer is empty
 *  - USBD class API enable and resumed calls
 *
 * for RX path, if enabled, at:
 *  - cp210x_irq_rx_enable()
 *  - RX transfer completion
 *  - RX buffer is not empty
 */
static void cp210x_irq_cb_handler(struct k_work *work)
{
	struct cp210x_uart_data *data;
	const struct cp210x_uart_config *cfg;
	struct usbd_class_data *c_data;

	data = CONTAINER_OF(work, struct cp210x_uart_data, irq_cb_work);
	cfg = data->dev->config;
	c_data = cfg->c_data;

	if (!data->cb) {
		LOG_ERR("IRQ callback is not set");
		return;
	}

	data->tx_fifo.altered = false;
	data->rx_fifo.altered = false;
	data->rx_fifo.irq = false;
	data->tx_fifo.irq = false;

	if (atomic_test_bit(&data->state, CP210X_IRQ_RX_ENABLED) ||
	    atomic_test_bit(&data->state, CP210X_IRQ_TX_ENABLED)) {
		data->cb(usbd_class_get_private(c_data), data->cb_data);
	}

	if (data->rx_fifo.altered) {
		LOG_DBG("rx fifo altered, submit work");
		cp210x_work_submit(&data->rx_fifo_work);
	}

	if (!atomic_test_bit(&data->state, CP210X_TX_FIFO_BUSY)) {
		if (data->tx_fifo.altered) {
			LOG_DBG("tx fifo altered, submit work");
			cp210x_work_schedule(&data->tx_fifo_work, K_NO_WAIT);
		} else if (data->zlp_needed) {
			LOG_DBG("zlp needed, submit work");
			cp210x_work_schedule(&data->tx_fifo_work, K_NO_WAIT);
		}
	}

	if (atomic_test_bit(&data->state, CP210X_IRQ_RX_ENABLED) &&
	    !ring_buf_is_empty(data->rx_fifo.rb)) {
		LOG_DBG("rx irq pending, submit irq_cb_work");
		cp210x_work_submit(&data->irq_cb_work);
	}

	if (atomic_test_bit(&data->state, CP210X_IRQ_TX_ENABLED) &&
	    ring_buf_space_get(data->tx_fifo.rb)) {
		LOG_DBG("tx irq pending, submit irq_cb_work");
		cp210x_work_submit(&data->irq_cb_work);
	}
}

static void cp210x_irq_callback_set(const struct device *dev,
				    const uart_irq_callback_user_data_t cb,
				    void *const cb_data)
{
	struct cp210x_uart_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = cb_data;
}

static int cp210x_poll_in(const struct device *dev, unsigned char *const c)
{
	struct cp210x_uart_data *const data = dev->data;
	uint32_t len;
	int ret = -1;

	if (ring_buf_is_empty(data->rx_fifo.rb)) {
		return ret;
	}

	len = ring_buf_get(data->rx_fifo.rb, c, 1);
	if (len) {
		cp210x_work_submit(&data->rx_fifo_work);
		ret = 0;
	}

	return ret;
}

static void cp210x_poll_out(const struct device *dev, const unsigned char c)
{
	struct cp210x_uart_data *const data = dev->data;
	k_spinlock_key_t key;
	uint32_t wrote;

	while (true) {
		key = k_spin_lock(&data->lock);
		wrote = ring_buf_put(data->tx_fifo.rb, &c, 1);
		k_spin_unlock(&data->lock, key);

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
	cp210x_work_schedule(&data->tx_fifo_work, K_MSEC(1));
}

#ifdef CONFIG_UART_LINE_CTRL
static int cp210x_line_ctrl_set(const struct device *dev,
				const uint32_t ctrl, const uint32_t val)
{
	/* FIXME: add line ctrl set support */
	return -EINVAL;
}

static int cp210x_line_ctrl_get(const struct device *dev,
				const uint32_t ctrl, uint32_t *const val)
{
	struct cp210x_uart_data *const data = dev->data;

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
	default:
		return -ENOTSUP;
	}
}
#endif

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int cp210x_configure(const struct device *dev,
			    const struct uart_config *const cfg)
{
	struct cp210x_uart_data *const data = dev->data;

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		data->flow_ctrl = false;
		break;
	case UART_CFG_FLOW_CTRL_DTR_DSR:
		data->flow_ctrl = true;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int cp210x_config_get(const struct device *dev,
			     struct uart_config *const cfg)
{
	struct cp210x_uart_data *const data = dev->data;

	memcpy(cfg, &data->uart_cfg, sizeof(struct uart_config));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int usbd_cp210x_preinit(const struct device *dev)
{
	struct cp210x_uart_data *const data = dev->data;

	ring_buf_reset(data->tx_fifo.rb);
	ring_buf_reset(data->rx_fifo.rb);

	k_work_init_delayable(&data->tx_fifo_work, cp210x_tx_fifo_handler);
	k_work_init(&data->rx_fifo_work, cp210x_rx_fifo_handler);
	k_work_init(&data->irq_cb_work, cp210x_irq_cb_handler);

	return 0;
}

static DEVICE_API(uart, cp210x_uart_api) = {
	.irq_tx_enable = cp210x_irq_tx_enable,
	.irq_tx_disable = cp210x_irq_tx_disable,
	.irq_tx_ready = cp210x_irq_tx_ready,
	.irq_rx_enable = cp210x_irq_rx_enable,
	.irq_rx_disable = cp210x_irq_rx_disable,
	.irq_rx_ready = cp210x_irq_rx_ready,
	.irq_is_pending = cp210x_irq_is_pending,
	.irq_update = cp210x_irq_update,
	.irq_callback_set = cp210x_irq_callback_set,
	.poll_in = cp210x_poll_in,
	.poll_out = cp210x_poll_out,
	.fifo_fill = cp210x_fifo_fill,
	.fifo_read = cp210x_fifo_read,
#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = cp210x_line_ctrl_set,
	.line_ctrl_get = cp210x_line_ctrl_get,
#endif
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = cp210x_configure,
	.config_get = cp210x_config_get,
#endif
};

struct usbd_class_api usbd_cp210x_api = {
	.request = usbd_cp210x_request,
	.update = usbd_cp210x_update,
	.enable = usbd_cp210x_enable,
	.disable = usbd_cp210x_disable,
	.suspended = usbd_cp210x_suspended,
	.resumed = usbd_cp210x_resumed,
	.control_to_host = usbd_cp210x_cth,
	.control_to_dev = usbd_cp210x_ctd,
	.init = usbd_cp210x_init,
	.get_desc = usbd_cp210x_get_desc,
};

#define CP210X_DEFINE_DESCRIPTOR(n)                                                                \
	static struct usbd_cp210x_desc cp210x_desc_##n = {                                         \
		.if0 =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 0,                                             \
				.bAlternateSetting = 0,                                            \
				.bNumEndpoints = 2,                                                \
				.bInterfaceClass = USB_BCC_VENDOR,                                 \
				.bInterfaceSubClass = 0,                                           \
				.bInterfaceProtocol = 0,                                           \
				.iInterface = 0,                                                   \
			},                                                                         \
                                                                                                   \
		.if0_in_ep =                                                                       \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x81,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
				.bInterval = 0,                                                    \
			},                                                                         \
                                                                                                   \
		.if0_out_ep =                                                                      \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x01,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
				.bInterval = 0,                                                    \
			},                                                                         \
                                                                                                   \
		.nil_desc =                                                                        \
			{                                                                          \
				.bLength = 0,                                                      \
				.bDescriptorType = 0,                                              \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	const static struct usb_desc_header *cp210x_fs_desc_##n[] = {                              \
		(struct usb_desc_header *)&cp210x_desc_##n.if0,                                    \
		(struct usb_desc_header *)&cp210x_desc_##n.if0_in_ep,                              \
		(struct usb_desc_header *)&cp210x_desc_##n.if0_out_ep,                             \
		(struct usb_desc_header *)&cp210x_desc_##n.nil_desc,                               \
	}

#define USBD_CP210X_DT_DEVICE_DEFINE(n)                                                            \
	BUILD_ASSERT(DT_INST_ON_BUS(n, usb),                                                       \
		     "node " DT_NODE_PATH(                                                         \
			     DT_DRV_INST(n)) " is not assigned to a USB device controller");       \
                                                                                                   \
	CP210X_DEFINE_DESCRIPTOR(n);                                                               \
	USBD_DEFINE_CLASS(cp210x_##n, &usbd_cp210x_api, (void *)DEVICE_DT_GET(DT_DRV_INST(n)),     \
			  NULL);                                                                   \
                                                                                                   \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, label), (				\
	USBD_DESC_STRING_DEFINE(cp210x_if_desc_data_##n,			\
				DT_INST_PROP(n, label),				\
				USBD_DUT_STRING_INTERFACE);			\
	))                                         \
                                                                                                   \
	RING_BUF_DECLARE(cp210x_rb_rx_##n, DT_INST_PROP(n, rx_fifo_size));                         \
	RING_BUF_DECLARE(cp210x_rb_tx_##n, DT_INST_PROP(n, tx_fifo_size));                         \
                                                                                                   \
	static const struct cp210x_uart_config uart_config_##n = {                                 \
		.c_data = &cp210x_##n,                                                             \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, label), (		\
		.if_desc_data = &cp210x_if_desc_data_##n,			\
		)) .desc = &cp210x_desc_##n,       \
			.fs_desc = cp210x_fs_desc_##n,                                             \
	};                                                                                         \
                                                                                                   \
	static struct cp210x_uart_data uart_data_##n = {                                           \
		.dev = DEVICE_DT_GET(DT_DRV_INST(n)),                                              \
		.uart_cfg = {CP210X_DEFAULT_LINECODING},                                           \
		.rx_fifo.rb = &cp210x_rb_rx_##n,                                                   \
		.tx_fifo.rb = &cp210x_rb_tx_##n,                                                   \
		.flow_ctrl = DT_INST_PROP(n, hw_flow_control),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, usbd_cp210x_preinit, NULL, &uart_data_##n, &uart_config_##n,      \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &cp210x_uart_api);

DT_INST_FOREACH_STATUS_OKAY(USBD_CP210X_DT_DEVICE_DEFINE);
