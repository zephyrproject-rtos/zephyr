/*
 * Copyright (c) 2022-2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_SOC_RISCV_TELINK_B91
#include "driver_b91.h"
#elif CONFIG_SOC_RISCV_TELINK_B92
#include "driver_b92.h"
#endif

#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

#include <soc.h>

#if (defined CONFIG_USB_TELINK_B9X && DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != 48000000u) \
&& (defined CONFIG_USB_TELINK_B9X && DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != 96000000u)
#error USB on B91 and B92 paltform requires CPU clocks frequency equal 48MHz or 96 MHz.
#endif

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_b9x);

#define DT_DRV_COMPAT telink_b9x_usbd

#define USBD_B9X_IRQN_BY_IDX(idx)	  DT_INST_IRQN_BY_IDX(0, idx)
#define USBD_B9X_IRQ_PRIORITY_BY_IDX(idx) DT_INST_IRQ_BY_IDX(0, idx, priority)

#define IS_REQUESTTYPE_DEV_TO_HOST(bmRT) (bmRT & 0x80)
#define IS_REQUESTTYPE_HOST_TO_DEV(bmRT) (!(bmRT & 0x80))

#define CTRL_EP_NORMAL_PACKET_REG_VALUE 0x38
#define CTRL_EP_ZLP_REG_VALUE		0x18

static const uint8_t ep_en_bit[] = {0,
				    FLD_USB_EDP1_EN,
				    FLD_USB_EDP2_EN,
				    FLD_USB_EDP3_EN,
				    FLD_USB_EDP4_EN,
				    FLD_USB_EDP5_EN,
				    FLD_USB_EDP6_EN,
				    FLD_USB_EDP7_EN,
				    FLD_USB_EDP8_EN};

#define USB_IN_EDP_IRQ_BITS                                                                        \
	(FLD_USB_EDP1_IRQ | FLD_USB_EDP2_IRQ | FLD_USB_EDP3_IRQ | FLD_USB_EDP4_IRQ |               \
	 FLD_USB_EDP7_IRQ | FLD_USB_EDP8_IRQ)
#define USB_OUT_EDP_IRQ_BITS (FLD_USB_EDP5_IRQ | FLD_USB_EDP6_IRQ)

enum usbd_endpoint_index_e {
	USBD_EP0_IDX = 0,     /* only for control transfer */
	USBD_IN_EP1_IDX = 1,  /* only IN */
	USBD_IN_EP2_IDX = 2,  /* only IN */
	USBD_IN_EP3_IDX = 3,  /* only IN */
	USBD_IN_EP4_IDX = 4,  /* only IN */
	USBD_OUT_EP5_IDX = 5, /* only OUT */
	USBD_OUT_EP6_IDX = 6, /* only OUT */
	USBD_IN_EP7_IDX = 7,  /* only IN */
	USBD_IN_EP8_IDX = 8,  /* only IN */
};

enum usbd_endpoint_index_e endpoint_in_idx[] = {USBD_IN_EP1_IDX, USBD_IN_EP2_IDX, USBD_IN_EP3_IDX,
					   USBD_IN_EP4_IDX, USBD_IN_EP7_IDX, USBD_IN_EP8_IDX};

enum usbd_endpoint_index_e endpoint_out_idx[] = {USBD_OUT_EP5_IDX, USBD_OUT_EP6_IDX};

#define USBD_EPIN_BUSY_RETRY_TIMEOUT_US        10000

#define USBD_EPIN_CNT	   (sizeof(endpoint_in_idx) / sizeof(enum usbd_endpoint_index_e))
#define USBD_EPOUT_CNT	   (sizeof(endpoint_out_idx) / sizeof(enum usbd_endpoint_index_e))
#define USBD_EP_IN_OUT_CNT (USBD_EPIN_CNT + USBD_EPOUT_CNT)
#define USBD_EP_TOTAL_CNT  (USBD_EP_IN_OUT_CNT + 1)

/** @brief The value of direction bit for the IN endpoint direction. */
#define USBD_EP_DIR_IN (1U << 7)

/** @brief The value of direction bit for the OUT endpoint direction. */
#define USBD_EP_DIR_OUT (0U << 7)

/**
 * @brief Macro for making the IN endpoint identifier from endpoint number.
 *
 * @details Macro that sets direction bit to make IN endpoint.
 *
 * @param[in] epn Endpoint number.
 *
 * @return IN Endpoint identifier.
 */
#define USBD_EPIN(epn) (((uint8_t)(epn)) | USBD_EP_DIR_IN)

/**
 * @brief Macro for making the OUT endpoint identifier from endpoint number.
 *
 * @details Macro that sets direction bit to make OUT endpoint.
 *
 * @param[in] epn Endpoint number.
 *
 * @return OUT Endpoint identifier.
 */
#define USBD_EPOUT(epn) (((uint8_t)(epn)) | USBD_EP_DIR_OUT)

#define EP_DATA_BUF_LEN 512

/* The total hardware buffer size */
#define EPS_BUFFER_TOTAL_SIZE 256

/**
 * @brief Endpoint buffer information.
 *
 * @param init_list			ep_idx that has been configured with BUF address
 * @param seg_addr			Available starting address of the USB endpoint cache.
 * @param init_num			Number of eps whose BUF address has been configured Max
 * packet size supported by endpoint.
 * @param remaining_size	The remaining available size of the USB endpoint cache.
 */
struct ep_buf_t {
	enum usbd_endpoint_index_e init_list[USBD_EP_TOTAL_CNT];
	uint8_t seg_addr;
	uint8_t init_num;
	uint16_t remaining_size;
};

static struct ep_buf_t eps_buf_inf = {.init_list = {0, 0, 0, 0, 0, 0, 0, 0, 0},
			       .seg_addr = 0,
			       .init_num = 0,
			       .remaining_size = EPS_BUFFER_TOTAL_SIZE};

/**
 * @brief Endpoint configuration.
 *
 * @param cb		Endpoint callback.
 * @param max_sz	Max packet size supported by endpoint.
 * @param en		Enable/Disable flag.
 * @param addr		Endpoint address.
 * @param type		Endpoint transfer type.
 * @param stall		Endpoint stall flag.
 */
struct b9x_usbd_ep_cfg {
	usb_dc_ep_callback cb;
	uint32_t max_sz;
	bool en;
	uint8_t addr;
	enum usb_dc_ep_transfer_type type;
	bool stall;
};

/**
 * @brief Endpoint buffer
 *
 * @param total_len		Total length to be read/written.
 * @param left_len		Remaining length to be read/written.
 * @param current_len	Length of this time to be read/written.
 * @param data			Pointer to data buffer for the endpoint.
 * @param current_pos	Pointer to the current offset in the endpoint buffer.
 */
struct b9x_usbd_ep_buf {
	uint32_t total_len;
	uint32_t left_len;
	uint32_t current_len;
	uint8_t *data;
	uint8_t *current_pos;
};

uint8_t ep_data_buf[USBD_EPOUT_CNT + 1][EP_DATA_BUF_LEN]; /* Only for EP0、EP5、EP6 */

/**
 * @brief Endpoint context
 *
 * @param cfg	Endpoint configuration
 * @param buf	Endpoint buffer
 */
struct b9x_usbd_ep_ctx {
	struct b9x_usbd_ep_cfg cfg;
	struct b9x_usbd_ep_buf buf;
	bool reading;
	uint8_t writing_len;
};

/**
 * @brief USBD control structure
 *
 * @param status_cb			Status callback for USB DC notifications
 * @param setup				Setup packet for Control requests
 * @param setup_Rsp			Response flag for Control requests
 * @param ctrl_zlp			Control endpoint zlp flag
 * @param attached			USBD Attached flag
 * @param ready				USBD Ready flag set after pullup
 * @param suspend			Suspend flag
 * @param suspend_ignore	Ignore suspend interrupt
 * @param drv_lock			Mutex for thread-safe b9x driver use
 * @param ep_ctx			Endpoint contexts
 */
struct b9x_usbd_ctx {
	usb_dc_status_callback status_cb;
	struct usb_setup_packet setup;
	bool setup_Rsp;
	bool ctrl_zlp;
	bool attached;
	bool ready;
	bool suspend;
	bool suspend_ignore;
	struct k_mutex drv_lock;
	struct b9x_usbd_ep_ctx ep_ctx[USBD_EP_TOTAL_CNT];
};

static struct b9x_usbd_ctx usbd_ctx = {
	.setup_Rsp = false,
	.ctrl_zlp = false,
	.attached = false,
	.ready = false,
	.suspend = true,
	.suspend_ignore = false,
};

static inline struct b9x_usbd_ctx *get_usbd_ctx(void)
{
	return &usbd_ctx;
}

static inline bool dev_attached(void)
{
	return get_usbd_ctx()->attached;
}

static inline bool dev_ready(void)
{
	return get_usbd_ctx()->ready;
}

static inline bool ep_is_valid(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx > USBD_EP_IN_OUT_CNT) {
		LOG_ERR("Endpoit index %d is out of range.", ep_idx);
		return false;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		if ((ep_idx == USBD_OUT_EP5_IDX) || (ep_idx == USBD_OUT_EP6_IDX)) {
			LOG_ERR("EP%d is only for OUT.", ep_idx);
			return false;
		}
	} else {
		if ((ep_idx != USBD_EP0_IDX) && (ep_idx != USBD_OUT_EP5_IDX) &&
		    (ep_idx != USBD_OUT_EP6_IDX)) {
			LOG_ERR("EP%d is only for IN.", ep_idx);
			return false;
		}
	}

	return true;
}

/** @brief Gets the structure pointer to the corresponding endpoint */
static struct b9x_usbd_ep_ctx *endpoint_ctx(const uint8_t ep)
{
	struct b9x_usbd_ctx *ctx;

	if (!ep_is_valid(ep)) {
		return NULL;
	}

	ctx = get_usbd_ctx();

	return &ctx->ep_ctx[USB_EP_GET_IDX(ep)];
}

static struct b9x_usbd_ep_ctx *in_endpoint_ctx(const uint8_t ep)
{
	return endpoint_ctx(USBD_EPIN(ep));
}

static struct b9x_usbd_ep_ctx *out_endpoint_ctx(const uint8_t ep)
{
	return endpoint_ctx(USBD_EPOUT(ep));
}


enum usbd_event_type {
	USBD_EVT_IRQ_EP,
	USBD_EVT_EP_COMPLETE,
	USBD_EVT_EP_BUSY,
	USBD_EVT_REINIT,
	USBD_EVT_SETUP,
	USBD_EVT_DATA,
	USBD_EVT_STATUS,
	USBD_EVT_RESET,
	USBD_EVT_SUSPEND,
	USBD_EVT_SLEEP,
};

struct usbd_event {
	enum usbd_event_type evt_type;
	uint8_t ep_bits;
	uint8_t ep_idx;
};

K_MSGQ_DEFINE(usbd_event_msgq, sizeof(struct usbd_event),
	CONFIG_USB_B9X_EVT_QUEUE_SIZE, sizeof(uint32_t));
static void usbd_work_handler(struct k_msgq *event_msgq);
static K_THREAD_DEFINE(usbd_b9x, CONFIG_USB_B9X_THREAD_STACK_SIZE,
	usbd_work_handler, &usbd_event_msgq, NULL, NULL, CONFIG_USB_B9X_THREAD_PRIORITY, 0, 0);

static void submit_usbd_event(enum usbd_event_type evt_type, uint8_t value)
{
	struct usbd_event ev = {
		.evt_type = evt_type
	};

	if (evt_type == USBD_EVT_IRQ_EP) {
		ev.ep_bits = value;
	} else if (evt_type == USBD_EVT_EP_COMPLETE) {
		ev.ep_idx = value;
	} else if (evt_type == USBD_EVT_EP_BUSY) {
		ev.ep_idx = value;
	}
	if (k_msgq_put(&usbd_event_msgq, &ev, K_NO_WAIT)) {
		LOG_ERR("Can't rise event %u", evt_type);
	}
}

/**
 * @brief Reset endpoint state.
 *
 * @details Reset the internal logic state for a given endpoint.
 *
 * @param[in]  ep_idx   Endpoint number
 */
static void ep_ctx_reset(enum usbd_endpoint_index_e ep_idx)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (ep_idx == USBD_EP0_IDX) {
		usbhw_reset_ctrl_ep_ptr();
	} else {
		reg_usb_ep_ptr(ep_idx) = 0;
	}
	if ((ep_idx == USBD_OUT_EP5_IDX) || (ep_idx == USBD_OUT_EP6_IDX)) {
		ep_ctx = out_endpoint_ctx(ep_idx);
	} else {
		ep_ctx = in_endpoint_ctx(ep_idx);
	}
	ep_ctx->buf.current_pos = ep_ctx->buf.data;
	ep_ctx->buf.total_len = 0;
	ep_ctx->buf.left_len = 0;
	ep_ctx->reading = false;
	ep_ctx->writing_len = 0;
}

static void ep_buf_clear(uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);

	ep_ctx->buf.current_pos = ep_ctx->buf.data;
	ep_ctx->buf.total_len = 0;
	ep_ctx->buf.left_len = 0;
}

static void ep_buf_init(uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);

	if (USB_EP_GET_IDX(ep) == USBD_EP0_IDX) {
		ep_ctx->buf.data = ep_data_buf[0];
	} else if (USB_EP_GET_IDX(ep) == USBD_OUT_EP5_IDX) {
		ep_ctx->buf.data = ep_data_buf[1];
	} else if (USB_EP_GET_IDX(ep) == USBD_OUT_EP6_IDX) {
		ep_ctx->buf.data = ep_data_buf[2];
	} else {
		ep_ctx->buf.data = NULL;
	}
	ep_buf_clear(ep);
}

static uint32_t ep_write(uint8_t ep, const uint8_t *data, uint32_t data_len)
{
	uint16_t i;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	struct b9x_usbd_ctx *ctx = get_usbd_ctx();
	struct b9x_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);
	uint32_t valid_len = 0;

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);
	if (usbhw_is_ep_busy(ep_idx)) {
		submit_usbd_event(USBD_EVT_EP_BUSY, ep_idx);
	} else {
		if (data_len > ep_ctx->cfg.max_sz) {
			valid_len = ep_ctx->cfg.max_sz;
		} else {
			valid_len = data_len;
		}

		if (ep_idx == USBD_EP0_IDX) {
			ep_ctx->buf.current_len = valid_len;
			reg_usb_sups_cyc_cali = CTRL_EP_NORMAL_PACKET_REG_VALUE;
			usbhw_reset_ctrl_ep_ptr();

			for (i = 0; i < valid_len; i++) {
				usbhw_write_ctrl_ep_data(data[i]);
			}
		} else {
			usbhw_reset_ep_ptr(ep_idx);
			for (i = 0; i < valid_len; i++) {
				reg_usb_ep_dat(ep_idx) = data[i];
			}
			ep_ctx->writing_len = valid_len;
			usbhw_data_ep_ack(ep_idx);
			submit_usbd_event(USBD_EVT_EP_COMPLETE, ep_idx);
		}
	}
	k_mutex_unlock(&ctx->drv_lock);
	return valid_len;
}

static void usb_irq_setup_handler(void)
{
	struct b9x_usbd_ep_ctx *ep_ctx;
	struct b9x_usbd_ctx *ctx = get_usbd_ctx();

	memset(&ctx->setup, 0, sizeof(struct usb_setup_packet));
	reg_usb_sups_cyc_cali = CTRL_EP_NORMAL_PACKET_REG_VALUE;
	usbhw_reset_ctrl_ep_ptr();
	ctx->setup.bmRequestType = usbhw_read_ctrl_ep_data();
	ctx->setup.bRequest = usbhw_read_ctrl_ep_data();
	ctx->setup.wValue = usbhw_read_ctrl_ep_u16();
	ctx->setup.wIndex = usbhw_read_ctrl_ep_u16();
	ctx->setup.wLength = usbhw_read_ctrl_ep_u16();

	LOG_DBG("SETUP:bmRT:0x%02x  bR:0x%02x wV:0x%04x wI:0x%04x wL:%d",
		(uint32_t)ctx->setup.bmRequestType, (uint32_t)ctx->setup.bRequest,
		(uint32_t)ctx->setup.wValue, (uint32_t)ctx->setup.wIndex,
		(uint32_t)ctx->setup.wLength);

	if (get_usbd_ctx()->suspend) {
		get_usbd_ctx()->suspend = false;
		get_usbd_ctx()->suspend_ignore = true;
		riscv_plic_irq_enable(USBD_B9X_IRQN_BY_IDX(5));
		if (get_usbd_ctx()->status_cb) {
			LOG_DBG("USB resume");
			get_usbd_ctx()->status_cb(USB_DC_RESUME, NULL);
		}
	}
	ep_ctx = endpoint_ctx(USB_EP_GET_ADDR(USBD_EP0_IDX, USB_EP_DIR_OUT));

	if (IS_REQUESTTYPE_DEV_TO_HOST(ctx->setup.bmRequestType) && ctx->setup.wLength) {
		ctx->setup_Rsp = true;
		ctx->ctrl_zlp = false;
	} else {
		ctx->setup_Rsp = false;
	}
	ep_ctx->cfg.cb(USB_EP_GET_ADDR(USBD_EP0_IDX, USB_EP_DIR_OUT), USB_DC_EP_SETUP);

	if (ep_ctx->cfg.stall) {
		usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_STALL);
	} else {
		usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK);
	}

	if (!IS_REQUESTTYPE_DEV_TO_HOST(ctx->setup.bmRequestType) && ctx->setup.wLength) {
		ep_ctx->reading = true;
		ep_ctx->buf.left_len = ctx->setup.wLength;
		ep_ctx->buf.total_len = ctx->setup.wLength;
		ep_ctx->buf.current_pos = ep_ctx->buf.data;
	}
}

static void usb_ctrl_data_read_handler(void)
{
	struct b9x_usbd_ep_ctx *ep_ctx =
		endpoint_ctx(USB_EP_GET_ADDR(USBD_EP0_IDX, USB_EP_DIR_OUT));
	uint32_t i = 0;
	uint32_t len = 0;

	if (!ep_ctx->reading) {
		return;
	}
	if (ep_ctx->buf.left_len > 8) {
		len = 8;
		ep_ctx->buf.left_len -= 8;
	} else {
		len = ep_ctx->buf.left_len;
		ep_ctx->buf.left_len = 0;
	}

	usbhw_reset_ctrl_ep_ptr();
	for (i = 0; i < len; i++) {
		ep_ctx->buf.current_pos[i] = usbhw_read_ctrl_ep_data();
	}
	ep_ctx->buf.current_pos += len;

	usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK);
	if (!ep_ctx->buf.left_len) {
		LOG_HEXDUMP_DBG(ep_ctx->buf.data, ep_ctx->buf.total_len, "");
		if (ep_ctx->cfg.cb) {
			ep_ctx->cfg.cb(USB_EP_GET_ADDR(USBD_EP0_IDX, USB_EP_DIR_OUT),
				       USB_DC_EP_DATA_OUT);
		}
	}
}

static void usb_ctrl_data_write_handler(void)
{
	struct b9x_usbd_ctx *ctx = get_usbd_ctx();
	struct b9x_usbd_ep_ctx *ep_ctx =
		endpoint_ctx(USB_EP_GET_ADDR(USBD_EP0_IDX, USB_EP_DIR_IN));

	ep_ctx->cfg.cb(USB_EP_GET_ADDR(USBD_EP0_IDX, USB_EP_DIR_IN), USB_DC_EP_DATA_IN);

	if (ep_ctx->cfg.stall) {
		usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_STALL);
	} else if ((ep_ctx->buf.total_len % 8 == 0) && (ep_ctx->buf.current_len == 0) &&
		    (ep_ctx->buf.total_len != ctx->setup.wLength) && !ctx->ctrl_zlp) {
		reg_usb_sups_cyc_cali = CTRL_EP_ZLP_REG_VALUE;
		ctx->ctrl_zlp = true;
		usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK);
	} else {
		usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK);
	}
}

static void usb_irq_data_handler(void)
{
	struct b9x_usbd_ctx *ctx = get_usbd_ctx();

	if (IS_REQUESTTYPE_HOST_TO_DEV(ctx->setup.bmRequestType)) {
		usb_ctrl_data_read_handler();
		return;
	}

	usb_ctrl_data_write_handler();
}

static void usb_irq_status_handler(void)
{
	reg_usb_sups_cyc_cali = CTRL_EP_NORMAL_PACKET_REG_VALUE;
	if (endpoint_ctx(USB_EP_GET_ADDR(USBD_EP0_IDX, USB_EP_DIR_OUT))->cfg.stall) {
		endpoint_ctx(USB_EP_GET_ADDR(USBD_EP0_IDX, USB_EP_DIR_OUT))->cfg.stall = false;
	} else {
		usbhw_write_ctrl_ep_ctrl(FLD_EP_STA_ACK);
	}
}

static void usb_irq_reset_handler(void)
{
	uint32_t i;

	for (i = 1; i <= 8; i++) {
		reg_usb_ep_ctrl(i) = 0;
	}

	if (get_usbd_ctx()->suspend) {
		if (get_usbd_ctx()->status_cb) {
			get_usbd_ctx()->status_cb(USB_DC_CONNECTED, NULL);
		}
	}
	if (get_usbd_ctx()->status_cb) {
		LOG_DBG("USB reset");
		get_usbd_ctx()->status_cb(USB_DC_RESET, NULL);
	}
	if (get_usbd_ctx()->suspend) {
		get_usbd_ctx()->suspend = false;
		get_usbd_ctx()->suspend_ignore = true;
		riscv_plic_irq_enable(USBD_B9X_IRQN_BY_IDX(5));
		if (get_usbd_ctx()->status_cb) {
			LOG_DBG("USB resume");
			get_usbd_ctx()->status_cb(USB_DC_RESUME, NULL);
		}
	}
}

static void usb_irq_suspend_handler(void)
{
	if (dev_ready()) {
		if (get_usbd_ctx()->status_cb) {
			get_usbd_ctx()->status_cb(USB_DC_SUSPEND, NULL);
		}
		if (!(reg_usb_mdev & FLD_USB_MDEV_WAKE_FEA)) {
			if (get_usbd_ctx()->status_cb) {
				get_usbd_ctx()->status_cb(USB_DC_DISCONNECTED, NULL);
			}
		}
	}
}

static void usb_irq_setup(void)
{
	usbhw_clr_ctrl_ep_irq(FLD_CTRL_EP_IRQ_SETUP);
	submit_usbd_event(USBD_EVT_SETUP, 0);
}

static void usb_irq_data(void)
{
	usbhw_clr_ctrl_ep_irq(FLD_CTRL_EP_IRQ_DATA);
	submit_usbd_event(USBD_EVT_DATA, 0);
}

static void usb_irq_status(void)
{
	usbhw_clr_ctrl_ep_irq(FLD_CTRL_EP_IRQ_STA);
	submit_usbd_event(USBD_EVT_STATUS, 0);
}

static inline void usb_ep_send_zlp_if_needed(const uint8_t ep_idx)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	ep_ctx = in_endpoint_ctx(ep_idx);
	if ((ep_ctx != NULL) && (ep_ctx->cfg.max_sz == ep_ctx->writing_len)) {
		ep_ctx->writing_len = 0;
		usbhw_reset_ep_ptr(ep_idx);
		usbhw_data_ep_ack(ep_idx);
	}
}

static inline void irq_in_ep_handler(usb_ep_irq_e ep_irq_bit, enum usbd_endpoint_index_e ep_idx)
{
	usbhw_clr_eps_irq(ep_irq_bit);
	usbhw_reset_ep_ptr(ep_idx);
	usb_ep_send_zlp_if_needed(ep_idx);
}

static inline void irq_in_eps_handler(uint8_t in_eps)
{
	if (!in_eps) {
		return;
	}

	LOG_DBG("in_eps: 0x%02X", in_eps);
	if (in_eps & FLD_USB_EDP1_IRQ) {
		irq_in_ep_handler(FLD_USB_EDP1_IRQ, USBD_IN_EP1_IDX);
	}
	if (in_eps & FLD_USB_EDP2_IRQ) {
		irq_in_ep_handler(FLD_USB_EDP2_IRQ, USBD_IN_EP2_IDX);
	}
	if (in_eps & FLD_USB_EDP3_IRQ) {
		irq_in_ep_handler(FLD_USB_EDP3_IRQ, USBD_IN_EP3_IDX);
	}
	if (in_eps & FLD_USB_EDP4_IRQ) {
		irq_in_ep_handler(FLD_USB_EDP4_IRQ, USBD_IN_EP4_IDX);
	}
	if (in_eps & FLD_USB_EDP7_IRQ) {
		irq_in_ep_handler(FLD_USB_EDP7_IRQ, USBD_IN_EP7_IDX);
	}
	if (in_eps & FLD_USB_EDP8_IRQ) {
		irq_in_ep_handler(FLD_USB_EDP8_IRQ, USBD_IN_EP8_IDX);
	}
}

static inline void irq_out_eps_handler(uint8_t out_eps)
{
	if (!out_eps) {
		return;
	}

	LOG_DBG("out_eps: 0x%02X", out_eps);
	usbhw_clr_eps_irq(out_eps);
	submit_usbd_event(USBD_EVT_IRQ_EP, out_eps);
}

static void usb_irq_eps(void)
{
	uint8_t irq_eps = usbhw_get_eps_irq();

	irq_in_eps_handler(irq_eps & USB_IN_EDP_IRQ_BITS);
	irq_out_eps_handler(irq_eps & USB_OUT_EDP_IRQ_BITS);
}

static void usb_irq_reset(void)
{
	usbhw_clr_irq_status(USB_IRQ_RESET_STATUS);
	submit_usbd_event(USBD_EVT_RESET, 0);
}

static void usb_irq_suspend(void)
{
	if (get_usbd_ctx()->suspend_ignore) {
		get_usbd_ctx()->suspend_ignore = false;
		return;
	}
	riscv_plic_irq_disable(USBD_B9X_IRQN_BY_IDX(5));
	if (!get_usbd_ctx()->suspend) {
		get_usbd_ctx()->suspend = true;
		submit_usbd_event(USBD_EVT_SUSPEND, 0);
	}
}

static int usb_irq_init(void)
{
	IRQ_CONNECT(USBD_B9X_IRQN_BY_IDX(0), USBD_B9X_IRQ_PRIORITY_BY_IDX(0), usb_irq_setup, 0, 0);
	riscv_plic_irq_enable(USBD_B9X_IRQN_BY_IDX(0));
	riscv_plic_set_priority(USBD_B9X_IRQN_BY_IDX(0), USBD_B9X_IRQ_PRIORITY_BY_IDX(0));

	IRQ_CONNECT(USBD_B9X_IRQN_BY_IDX(1), USBD_B9X_IRQ_PRIORITY_BY_IDX(1), usb_irq_data, 0, 0);
	riscv_plic_irq_enable(USBD_B9X_IRQN_BY_IDX(1));
	riscv_plic_set_priority(USBD_B9X_IRQN_BY_IDX(1), USBD_B9X_IRQ_PRIORITY_BY_IDX(1));

	IRQ_CONNECT(USBD_B9X_IRQN_BY_IDX(2), USBD_B9X_IRQ_PRIORITY_BY_IDX(2), usb_irq_status, 0, 0);
	riscv_plic_irq_enable(USBD_B9X_IRQN_BY_IDX(2));
	riscv_plic_set_priority(USBD_B9X_IRQN_BY_IDX(2), USBD_B9X_IRQ_PRIORITY_BY_IDX(2));

	IRQ_CONNECT(USBD_B9X_IRQN_BY_IDX(4), USBD_B9X_IRQ_PRIORITY_BY_IDX(4), usb_irq_eps, 0, 0);
	riscv_plic_irq_enable(USBD_B9X_IRQN_BY_IDX(4));
	riscv_plic_set_priority(USBD_B9X_IRQN_BY_IDX(4), USBD_B9X_IRQ_PRIORITY_BY_IDX(4));

	IRQ_CONNECT(USBD_B9X_IRQN_BY_IDX(5), USBD_B9X_IRQ_PRIORITY_BY_IDX(5), usb_irq_suspend, 0,
		    0);
	riscv_plic_irq_enable(USBD_B9X_IRQN_BY_IDX(5));
	riscv_plic_set_priority(USBD_B9X_IRQN_BY_IDX(5), USBD_B9X_IRQ_PRIORITY_BY_IDX(5));

	get_usbd_ctx()->suspend_ignore = true;

	IRQ_CONNECT(USBD_B9X_IRQN_BY_IDX(6), USBD_B9X_IRQ_PRIORITY_BY_IDX(6), usb_irq_reset, 0, 0);
	riscv_plic_irq_enable(USBD_B9X_IRQN_BY_IDX(6));
	riscv_plic_set_priority(USBD_B9X_IRQN_BY_IDX(6), USBD_B9X_IRQ_PRIORITY_BY_IDX(6));

	usbhw_enable_manual_interrupt(FLD_CTRL_EP_AUTO_CFG | FLD_CTRL_EP_AUTO_DESC |
				FLD_CTRL_EP_AUTO_FEAT | FLD_CTRL_EP_AUTO_STD);
	usbhw_set_eps_irq_mask(FLD_USB_EDP5_IRQ | FLD_USB_EDP6_IRQ);

#if CONFIG_SOC_RISCV_TELINK_B91
	usbhw_set_irq_mask(USB_IRQ_RESET_MASK | USB_IRQ_SUSPEND_MASK);
#endif
	usbhw_clr_irq_status(USB_IRQ_RESET_STATUS);

	return 0;
}

/**
 * @brief Attach USB for device connection
 *
 * @details Function to attach USB for device connection. Upon success, the USB PLL
 * is enabled, and the USB device is now capable of transmitting and receiving on
 * the USB bus and of generating interrupts.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_attach(void)
{
	struct b9x_usbd_ctx *ctx = get_usbd_ctx();
	uint32_t i;

	if (ctx->attached) {
		return 0;
	}

	k_mutex_init(&ctx->drv_lock);

	for (i = USBD_IN_EP1_IDX; i <= USBD_EP_IN_OUT_CNT; i++) {
		usbhw_set_eps_dis(ep_en_bit[i]);
		ep_ctx_reset(i);
	}

	ctx->attached = true;
	ctx->ready = true;

	return 0;
}

/**
 * @brief Detach the USB device
 *
 * @details Function to detach the USB device. Upon success, the USB hardware PLL
 * is powered down and USB communication is disabled.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_detach(void)
{
	struct b9x_usbd_ctx *ctx = get_usbd_ctx();
	struct b9x_usbd_ep_ctx *ep_ctx;
	uint8_t i;

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);

	for (i = USBD_IN_EP1_IDX; i <= USBD_EP_IN_OUT_CNT; i++) {
		if ((i == USBD_OUT_EP5_IDX) || (i == USBD_OUT_EP6_IDX)) {
			ep_ctx = out_endpoint_ctx(i);
		} else {
			ep_ctx = in_endpoint_ctx(i);
		}
		memset(ep_ctx, 0, sizeof(*ep_ctx));
	}
	ctx->attached = false;
	k_mutex_unlock(&ctx->drv_lock);

	return 0;
}

/**
 * @brief Reset the USB device
 *
 * @details This function returns the USB device and firmware back to it's initial state.
 * N.B. the USB PLL is handled by the usb_detach function
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_reset(void)
{
	int ret;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	LOG_DBG("USBD Reset");

	ret = usb_dc_detach();
	if (ret) {
		return ret;
	}

	ret = usb_dc_attach();
	if (ret) {
		return ret;
	}

	return 0;
}

/**
 * @brief Set USB device address
 *
 * @param[in] addr Device address
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_set_address(const uint8_t addr)
{
	return 0;
}

/**
 * @brief Set USB device controller status callback
 *
 * @details Function to set USB device controller status callback. The registered
 * callback is used to report changes in the status of the device controller. The
 * status code are described by the usb_dc_status_code enumeration.
 *
 * @param[in] cb Callback function
 */
void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	get_usbd_ctx()->status_cb = cb;
}

/**
 * @brief check endpoint capabilities
 *
 * @details Function to check capabilities of an endpoint. usb_dc_ep_cfg_data structure
 * provides the endpoint configuration parameters: endpoint address, endpoint maximum
 * packet size and endpoint type. The driver should check endpoint capabilities and
 * return 0 if the endpoint configuration is possible.
 *
 * @param[in] cfg Endpoint config
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->ep_addr);

	LOG_DBG("ep 0x%02x, mps %d, type %d", ep_cfg->ep_addr, ep_cfg->ep_mps, ep_cfg->ep_type);

	if (ep_idx > USBD_IN_EP8_IDX) {
		LOG_ERR("Endpoint index %d is out of range.", ep_idx);
		return -EINVAL;
	}

	if (ep_idx == USBD_EP0_IDX) {
		if (ep_cfg->ep_type != USB_DC_EP_CONTROL) {
			LOG_ERR("EP%d can only be a control endpoint.", USBD_EP0_IDX);
			return -EINVAL;
		}
		if (ep_cfg->ep_mps > 8) {
			LOG_ERR("EP%d's max packet size is fixed to 8.", USBD_EP0_IDX);
			return -EINVAL;
		}
	} else if (USB_EP_DIR_IS_IN(ep_cfg->ep_addr)) {
		if (ep_cfg->ep_type == USB_DC_EP_CONTROL) {
			LOG_ERR("EP%d cannot be a control endpoint.", ep_idx);
			return -EINVAL;
		}
		if ((ep_idx == USBD_OUT_EP5_IDX) || (ep_idx == USBD_OUT_EP6_IDX)) {
			LOG_ERR("EP%d can only be an OUT endpoint.", ep_idx);
			return -EINVAL;
		}
	} else {
		if (ep_cfg->ep_type == USB_DC_EP_CONTROL) {
			LOG_ERR("EP%d cannot be a control endpoint.", ep_idx);
			return -EINVAL;
		}
		if ((ep_idx != USBD_OUT_EP5_IDX) && (ep_idx != USBD_OUT_EP6_IDX)) {
			LOG_ERR("EP%d can only be an IN endpoint.", ep_idx);
			return -EINVAL;
		}
	}

	if (ep_cfg->ep_mps > EPS_BUFFER_TOTAL_SIZE) {
		LOG_ERR("invalid endpoint max packet size: %d", ep_cfg->ep_mps);
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Configure endpoint
 *
 * Function to configure an endpoint. usb_dc_ep_cfg_data structure provides
 * the endpoint configuration parameters: endpoint address, endpoint maximum
 * packet size and endpoint type.
 *
 * @param[in] cfg Endpoint config
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	struct b9x_usbd_ep_ctx *ep_ctx;
	uint8_t i;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->ep_addr);

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep_cfg->ep_addr);
	if (!ep_ctx) {
		return -EINVAL;
	}
	LOG_DBG("ep_addr: 0x%02x, ep_type:%d, ep_mps:%d", ep_cfg->ep_addr, ep_cfg->ep_type,
		ep_cfg->ep_mps);
	if (ep_idx == USBD_EP0_IDX) {
		if (ep_cfg->ep_type != USB_DC_EP_CONTROL) {
			LOG_ERR("EP%d only supports the control transmission mode.", USBD_EP0_IDX);
			return -EINVAL;
		}
	} else {
		if (ep_cfg->ep_type == USB_DC_EP_CONTROL) {
			LOG_ERR("Only EP%d supports the control transmission mode!", USBD_EP0_IDX);
			return -EINVAL;
		}
		for (i = 0; i < eps_buf_inf.init_num; i++) {
			if (eps_buf_inf.init_list[i] == ep_idx) {
				LOG_DBG("ep%d buf address already configured", ep_idx);
				return 0;
			}
		}
		if (eps_buf_inf.remaining_size < ep_cfg->ep_mps) {
			LOG_ERR("There is only %d bytes left for endpoint buffer.",
				eps_buf_inf.remaining_size);
			return -EINVAL;
		}
		if (ep_cfg->ep_type == USB_DC_EP_ISOCHRONOUS) {
			BM_SET(reg_usb_iso_mode, BIT(ep_idx & 0x07));
		} else {
			if ((ep_idx == USBD_OUT_EP6_IDX) || (ep_idx == USBD_IN_EP7_IDX)) {
				/* EP 6 and 7 are default for synchronous data transmission and */
				/* need to be cleared */
				BM_CLR(reg_usb_iso_mode, BIT(ep_idx & 0x07));
			}
		}
		ep_ctx->cfg.type = ep_cfg->ep_type;
		reg_usb_ep_buf_addr(ep_idx) = eps_buf_inf.seg_addr;
		eps_buf_inf.seg_addr += ep_ctx->cfg.max_sz;
		eps_buf_inf.remaining_size -= ep_ctx->cfg.max_sz;
		eps_buf_inf.init_list[eps_buf_inf.init_num] = ep_idx;
		eps_buf_inf.init_num++;
	}
	ep_ctx->cfg.max_sz = ep_cfg->ep_mps;
	ep_buf_init(ep_cfg->ep_addr);
	ep_ctx->cfg.addr = ep_cfg->ep_addr;
	ep_ctx->cfg.type = ep_cfg->ep_type;
	if ((ep_ctx->cfg.type == USB_DC_EP_BULK) && USB_EP_DIR_IS_OUT(ep_ctx->cfg.addr)) {
		usbhw_data_ep_ack(ep_idx);
	}

	return 0;
}

/**
 * @brief Set stall condition for the selected endpoint
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_set_stall(const uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}
	ep_ctx->cfg.stall = true;
	ep_buf_clear(ep);
	LOG_DBG("Stall on ep%d", USB_EP_GET_IDX(ep));

	return 0;
}

/**
 * @brief Clear stall condition for the selected endpoint
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_clear_stall(const uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}
	ep_ctx->cfg.stall = false;
	LOG_DBG("Unstall on EP 0x%02x", ep);
	return 0;
}

/**
 * @brief Check if the selected endpoint is stalled
 *
 * @param[in]  ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 * @param[out] stalled	Endpoint stall status
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	*stalled = ep_ctx->cfg.stall;

	return 0;
}

/**
 * @brief Halt the selected endpoint
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_halt(const uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

/**
 * @brief Enable the selected endpoint
 *
 * @details Function to enable the selected endpoint. Upon success interrupts are
 * enabled for the corresponding endpoint and the endpoint is ready for
 * transmitting/receiving data.
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_enable(const uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	LOG_DBG("EP enable: 0x%02x", ep);
	ep_ctx->cfg.en = true;

	if (dev_ready()) {
		ep_ctx->cfg.stall = false;
		usbhw_set_eps_en(ep_en_bit[USB_EP_GET_IDX(ep)]);
	}
	if ((ep_ctx->cfg.type == USB_DC_EP_BULK) && USB_EP_DIR_IS_OUT(ep_ctx->cfg.addr)) {
		usbhw_data_ep_ack(USB_EP_GET_IDX(ep));
	}
	return 0;
}

/**
 * @brief Disable the selected endpoint
 *
 * @details Function to disable the selected endpoint. Upon success interrupts are
 * disabled for the corresponding endpoint and the endpoint is no longer able for
 * transmitting/receiving data.
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_disable(const uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		return -EALREADY;
	}

	LOG_DBG("EP disable: 0x%02x", ep);
	usbhw_set_eps_dis(ep_en_bit[USB_EP_GET_IDX(ep)]);
	ep_ctx_reset(USB_EP_GET_IDX(ep));
	ep_ctx->cfg.stall = true;
	ep_ctx->cfg.en = false;
	return 0;
}

/**
 * @brief Flush the selected endpoint
 *
 * @details This function flushes the FIFOs for the selected endpoint.
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_flush(const uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}
	ep_buf_clear(ep);
	LOG_DBG("ep%d flush", USB_EP_GET_IDX(ep));

	return 0;
}

/**
 * @brief Write data to the specified endpoint
 *
 * @details This function is called to write data to the specified endpoint. The
 * supplied usb_ep_callback function will be called when data is transmitted out.
 *
 * @param[in]  ep			Endpoint address corresponding to the one
 *							listed in the device configuration table
 * @param[in]  data			Pointer to data to write
 * @param[in]  data_len		Length of the data requested to write. This may
 *							be zero for a zero length status packet.
 * @param[out] ret_bytes	Bytes scheduled for transmission. This value
 *							may be NULL if the application expects all
 *							bytes to be written
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data, const uint32_t data_len,
		    uint32_t *const ret_bytes)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	LOG_DBG("ep 0x%02x, len %d", ep, data_len);

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}
	if (USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Endpoint 0x%02x is invalid, it has direaction error.", ep);
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}
	if (!ep_ctx->cfg.en) {
		LOG_ERR("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}
	if (get_usbd_ctx()->setup_Rsp) {
		get_usbd_ctx()->setup_Rsp = false;
		ep_ctx->cfg.stall = false;
		ep_ctx->buf.total_len = data_len;
		LOG_HEXDUMP_DBG(data, data_len, "");
	}
	*ret_bytes = ep_write(ep, data, data_len);

	return 0;
}

/**
 * @brief Read data from the specified endpoint
 *
 * @details This function is called by the endpoint handler function, after an OUT
 * interrupt has been received for that EP. The application must only call this
 * function through the supplied usb_ep_callback function. This function clears
 * the ENDPOINT NAK, if all data in the endpoint FIFO has been read, so as to
 * accept more data from host.
 *
 * @param[in]  ep			Endpoint address corresponding to the one
 *							listed in the device configuration table
 * @param[in]  data			Pointer to data buffer to write to
 * @param[in]  max_data_len	Max length of data to read
 * @param[out] read_bytes	Number of bytes read. If data is NULL and
 *							max_data_len is 0 the number of bytes
 *							available for read should be returned.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_read(const uint8_t ep, uint8_t *const data, const uint32_t max_data_len,
		   uint32_t *const read_bytes)
{
	int ret;

	LOG_DBG("dc_ep_read: ep 0x%02x, maxlen %d", ep, max_data_len);
	ret = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);

	if (ret) {
		return ret;
	}

	if (!data && !max_data_len) {
		return ret;
	}

	if (USB_EP_GET_IDX(ep) != USBD_EP0_IDX) {
		ret = usb_dc_ep_read_continue(ep);
	}
	return ret;
}

/**
 * @brief Set callback function for the specified endpoint
 *
 * @details Function to set callback function for notification of data received and
 * available to application or transmit done on the selected endpoint, NULL if
 * callback not required by application code. The callback status code is
 * described by usb_dc_ep_cb_status_code.
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 * @param[in] cb	Callback function
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	ep_ctx->cfg.cb = cb;

	return 0;
}

/**
 * @brief Read data from the specified endpoint
 *
 * @details This is similar to usb_dc_ep_read, the difference being that, it doesn't
 * clear the endpoint NAKs so that the consumer is not bogged down by further
 * upcalls till he is done with the processing of the data. The caller should
 * reactivate ep by invoking usb_dc_ep_read_continue() do so.
 *
 * @param[in]  ep			Endpoint address corresponding to the one
 *							listed in the device configuration table
 * @param[in]  data			Pointer to data buffer to write to
 * @param[in]  max_data_len Max length of data to read
 * @param[out] read_bytes	Number of bytes read. If data is NULL and
 *							max_data_len is 0 the number of bytes
 *							available for read should be returned.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len, uint32_t *read_bytes)
{
	struct b9x_usbd_ep_ctx *ep_ctx;
	struct b9x_usbd_ctx *ctx = get_usbd_ctx();
	uint32_t bytes_to_copy;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		return -EINVAL;
	}

	if (!data && max_data_len) {
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		LOG_ERR("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);

	if (USB_EP_GET_IDX(ep) == USBD_EP0_IDX) {
		if (ep_ctx->reading) {
			ep_ctx->reading = false;
			bytes_to_copy = MIN(max_data_len, ep_ctx->buf.total_len);
			memcpy(data, ep_ctx->buf.data, bytes_to_copy);
		} else {
			bytes_to_copy = MIN(max_data_len, sizeof(struct usb_setup_packet));
			memcpy(data, &ctx->setup, bytes_to_copy);
		}
	} else {
		bytes_to_copy = MIN(max_data_len, ep_ctx->buf.total_len);
		memcpy(data, ep_ctx->buf.data, bytes_to_copy);
	}
	k_mutex_unlock(&ctx->drv_lock);
	*read_bytes = bytes_to_copy;
	LOG_HEXDUMP_DBG(data, bytes_to_copy, "");
	return 0;
}

/**
 * @brief Continue reading data from the endpoint
 *
 * @details Clear the endpoint NAK and enable the endpoint to accept more data from
 * the host. Usually called after usb_dc_ep_read_wait() when the consumer is fine
 * to accept more data. Thus these calls together act as a flow control mechanism.
 *
 * @param[in]  ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_read_continue(uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		LOG_ERR("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}
	LOG_DBG("Continue reading data from the Endpoint 0x%02x", ep);

	if (USB_EP_GET_IDX(ep) == USBD_EP0_IDX) {
		usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK);
	} else {
		usbhw_data_ep_ack(USB_EP_GET_IDX(ep));
	}
	return 0;
}

/**
 * @brief Get endpoint max packet size
 *
 * @param[in]  ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return Endpoint max packet size (mps)
 */
int usb_dc_ep_mps(uint8_t ep)
{
	struct b9x_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	return ep_ctx->cfg.max_sz;
}

/**
 * @brief Start the host wake up procedure.
 *
 * @details Function to wake up the host if it's currently in sleep mode.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_wakeup_request(void)
{
	LOG_DBG("Remote wakeup");
	if (reg_usb_mdev & FLD_USB_MDEV_WAKE_FEA) {
		reg_wakeup_en = FLD_USB_RESUME;
		reg_wakeup_en = FLD_USB_PWDN_I;
	}
	return 0;
}

static void ep_read(enum usbd_endpoint_index_e ep_idx)
{
	uint8_t i;
	uint8_t len;
	struct b9x_usbd_ep_ctx *ep_ctx;
	struct b9x_usbd_ctx *ctx = get_usbd_ctx();

	if ((ep_idx != USBD_OUT_EP5_IDX) && (ep_idx != USBD_OUT_EP6_IDX)) {
		LOG_ERR("EP%d is only for IN.", ep_idx);
		return;
	}

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);
	len = reg_usb_ep_ptr(ep_idx);
	ep_ctx = endpoint_ctx(USB_EP_GET_ADDR(ep_idx, USB_EP_DIR_OUT));
	usbhw_reset_ep_ptr(ep_idx);

	if (len && len <= ep_ctx->cfg.max_sz) {
		for (i = 0; i < len; i++) {
			ep_ctx->buf.data[i] = reg_usb_ep_dat(ep_idx);
		}
		ep_ctx->buf.left_len = ep_ctx->buf.total_len = len;
		if (ep_ctx->cfg.cb) {
			ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_OUT);
		}
	}
	k_mutex_unlock(&ctx->drv_lock);
}

static void usbd_work_handler(struct k_msgq *event_msgq)
{
	for (;;) {
		struct usbd_event ev;

		if (!k_msgq_get(event_msgq, &ev, K_FOREVER)) {

			if (!dev_ready()) {
				LOG_DBG("USBD is not ready, event drops.");
				continue;
			}

			struct b9x_usbd_ctx *ctx = get_usbd_ctx();
			struct b9x_usbd_ep_ctx *ep_ctx;

			switch (ev.evt_type) {
			case USBD_EVT_IRQ_EP:
				LOG_DBG("USBD_EVT_IRQ_EP");
				if (ev.ep_bits & FLD_USB_EDP5_IRQ) {
					ep_read(USBD_OUT_EP5_IDX);
				}
				if (ev.ep_bits & FLD_USB_EDP6_IRQ) {
					ep_read(USBD_OUT_EP6_IDX);
				}
				break;
			case USBD_EVT_EP_COMPLETE:
				LOG_DBG("USBD_EVT_EP_COMPLETE");
				if ((ev.ep_idx == USBD_OUT_EP5_IDX) ||
					(ev.ep_idx == USBD_OUT_EP6_IDX)) {
					ep_ctx = endpoint_ctx(USB_EP_GET_ADDR(ev.ep_idx,
						USB_EP_DIR_OUT));
					if (ep_ctx->cfg.cb) {
						ep_ctx->cfg.cb(ep_ctx->cfg.addr,
							USB_DC_EP_DATA_OUT);
					}
				} else {
					ep_ctx = endpoint_ctx(USB_EP_GET_ADDR(ev.ep_idx,
						USB_EP_DIR_IN));
					if (ep_ctx->cfg.cb) {
						ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_IN);
					}
				}
				break;
			case USBD_EVT_EP_BUSY:
				LOG_DBG("USBD_EVT_EP_BUSY");
				k_usleep(USBD_EPIN_BUSY_RETRY_TIMEOUT_US);
				ep_ctx = endpoint_ctx(USB_EP_GET_ADDR(ev.ep_idx, USB_EP_DIR_IN));
				if (ep_ctx->cfg.cb) {
					ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_IN);
				}
				if (ev.ep_idx == USBD_EP0_IDX) {
					if (ep_ctx->cfg.stall) {
						usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_STALL);
					} else if ((ep_ctx->buf.total_len % 8 == 0) &&
						(ep_ctx->buf.current_len == 0) &&
						(ep_ctx->buf.total_len != ctx->setup.wLength) &&
						!ctx->ctrl_zlp) {
						reg_usb_sups_cyc_cali = CTRL_EP_ZLP_REG_VALUE;
						ctx->ctrl_zlp = true;
						usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK);
					} else {
						usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK);
					}
				}
				break;
			case USBD_EVT_DATA:
				LOG_DBG("USBD_EVT_DATA");
				usb_irq_data_handler();
				break;
			case USBD_EVT_SETUP:
				LOG_DBG("USBD_EVT_SETUP");
				usb_irq_setup_handler();
				break;
			case USBD_EVT_STATUS:
				LOG_DBG("USBD_EVT_STATUS");
				usb_irq_status_handler();
				break;
			case USBD_EVT_SUSPEND:
				LOG_DBG("USBD_EVT_SUSPEND");
				usb_irq_suspend_handler();
				break;
			case USBD_EVT_RESET:
				LOG_DBG("USBD_EVT_RESET");
				usb_irq_reset_handler();
				break;
			case USBD_EVT_REINIT:
				LOG_DBG("USBD_EVT_REINIT");
				break;
			default:
				LOG_ERR("Unknown USBD event: %" PRId16, ev.evt_type);
				break;
			}
		}
	}
}

static int usb_init(void)
{
	reg_wakeup_en = 0;
	usb_set_pin_en();
	return usb_irq_init();
}

SYS_INIT(usb_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
