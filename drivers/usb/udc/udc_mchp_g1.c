/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include "udc_common.h"

LOG_MODULE_REGISTER(udc_mchp_g1, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_usb_g1

#define TIMEOUT_SYNCBUSY_RDY_US		1000U
#define USB_EP_IN_OFFSET		16U
#define USB_MIN_PACKET_SIZE		8U
#define USB_MAX_STANDARD_PACKET_SIZE	512U
#define USB_MAX_ISO_PACKET_SIZE		1023U
#define USB_ENCODED_ISO_SIZE		7U
#define USB_PACKET_SIZE_LOG2_OFFSET	3U

/* Maximum transfer size for byte_count/multi_packet_size fields (14-bit) */
#define USB_MAX_BYTE_COUNT		16383U

/* Hardware endpoint type values for USB_DEVICE_EPCFG register */
#define USB_HW_EPTYPE_DISABLED		0U
#define USB_HW_EPTYPE_CONTROL		1U
#define USB_HW_EPTYPE_ISOCHRONOUS	2U
#define USB_HW_EPTYPE_BULK		3U
#define USB_HW_EPTYPE_INTERRUPT		4U

#define CALIB_TUPLE_WIDTH        4U
#define OTP_WORD_BITS            32U
#define OTP_WORD_BYTES           4U
#define OTP_WORD_SHIFT           5U
#define OTP_BYTE_SHIFT           2U
#define OTP_BIT_INDEX_MASK       (OTP_WORD_BITS - 1U)

/*
 * Buffer descriptor for OUT endpoints (bank 0) in the Microchip G1 USB
 * device controller.
 *
 * Mirrors the hardware register layout:
 *   - 0x00: Buffer address
 *   - 0x04: PCKSIZE (packet size and transfer control)
 *   - 0x08: EXTREG (protocol-specific signaling)
 *   - 0x0A: STATUS_BK (status and error flags)
 */
struct mchp_ebd_bank0 {
	uint32_t addr;                   /* Buffer address */

	/* PCKSIZE (0x04) */
	uint32_t byte_count : 14;    /* Number of bytes received */
	uint32_t multi_packet_size : 14;
	uint32_t size : 3;           /* Encoded buffer size */
	uint32_t auto_zlp : 1;       /* Auto zero-length packet */

	/* EXTREG (0x08) */
	uint32_t subpid : 4;         /* Protocol-specific PID */
	uint32_t variable : 11;      /* Controller-specific field */
	uint32_t reserved0 : 1;

	/* STATUS_BK (0x0A) */
	uint32_t erroflow : 1;       /* Overflow error */
	uint32_t crcerr : 1;         /* CRC error */
	uint32_t reserved1 : 6;

	uint8_t reserved2[5];
} __packed;

/*
 * Buffer descriptor for IN endpoints (bank 1) in the Microchip G1 USB
 * device controller.
 *
 * Mirrors the hardware register layout:
 *   - 0x10: Buffer address
 *   - 0x14: PCKSIZE (packet size and transfer control)
 *   - 0x1A: STATUS_BK (status and error flags)
 *
 * Holds metadata for transmitting data to the USB host.
 */
struct mchp_ebd_bank1 {
	uint32_t addr;                          /* Buffer address */

	/* PCKSIZE (0x14) */
	uint32_t byte_count : 14;           /* Number of bytes in the buffer */
	uint32_t multi_packet_size : 14;    /* Multi-packet size for HB transfers */
	uint32_t size : 3;                  /* Encoded buffer size */
	uint32_t auto_zlp : 1;              /* Auto zero-length packet */

	uint8_t reserved0[2];

	/* STATUS_BK (0x1A) */
	uint32_t erroflow : 1;              /* Overflow error */
	uint32_t crcerr : 1;                /* CRC error */
	uint32_t reserved1 : 6;

	uint8_t reserved2[5];
} __packed;

/*
 * Endpoint buffer descriptor for both OUT (bank 0) and IN (bank 1) endpoints
 * in the Microchip G1 USB device controller.
 *
 * Each endpoint uses two banks: one for OUT transfers and one for IN transfers.
 */
struct mchp_ep_buffer_desc {
	struct mchp_ebd_bank0 bank0;  /* OUT endpoint descriptor */
	struct mchp_ebd_bank1 bank1;  /* IN endpoint descriptor */
} __packed;

/*
 * Compile-time check to ensure the endpoint buffer descriptor size
 * matches the hardware requirement (32 bytes).
 */
BUILD_ASSERT(sizeof(struct mchp_ep_buffer_desc) == 32,
	     "Broken endpoint buffer descriptor: size must be 32 bytes");

/*
 * Static configuration for the Microchip USB Device Controller (UDC) driver.
 * Contains all hardware-specific and driver-level configuration needed
 * to initialize and operate a UDC instance, including register base
 * address, endpoint tables, pin control configuration, and IRQ/thread
 * setup callbacks.
 */
struct udc_mchp_config {
	usb_device_registers_t *base;				/* USB controller base address */
	struct mchp_ep_buffer_desc *bdt;			/* Endpoint BDT ptr */
	size_t num_of_eps;					/* Count of bidirectional EPs */
	struct udc_ep_config *ep_cfg_in;			/* IN endpoint config array */
	struct udc_ep_config *ep_cfg_out;			/* OUT endpoint config array */
	struct pinctrl_dev_config *const pcfg;			/* Pin control config for device */
	struct {
		const struct device *clock_dev;
		clock_control_subsys_t mclk_subsys;
		clock_control_subsys_t gclk_subsys;
		enum clock_mchp_gclkgen gclk_src;
	} clock;
	const uint8_t *calib; /* Calibration mapping entries from devicetree */
	uint8_t calib_len; /* Number of calibration mapping cells */
	uintptr_t nvm_reg;  /* Address of NVM calibration word */
	void (*irq_enable_func)(const struct device *dev);	/* Function to enable IRQs */
	void (*irq_disable_func)(const struct device *dev);	/* Function to disable IRQs */
	void (*make_thread)(const struct device *dev);		/* Drv thread creation callback */
};

enum mchp_event_type {
	MCHP_EVT_SETUP,			/* Setup packet received on the control endpoint. */
	MCHP_EVT_XFER_NEW,		/* New transfer triggered (except control OUT endpoint). */
	MCHP_EVT_XFER_FINISHED,		/* Transfer for a specific endpoint has finished. */
};

/*
 * Runtime data for the Microchip USB Device Controller (UDC) driver.
 *
 * Contains dynamic state for a UDC instance, including thread context,
 * event signaling, endpoint transfer bitmaps, and buffers used for
 * control transfers.
 */
struct udc_mchp_data {
	const struct device *dev;		/* Pointer to device instance */
	struct k_thread thread_data;		/* Driver thread context */
	struct k_event events;			/* Event flags for thread synchronization. */
	atomic_t xfer_new;			/* Bitmap of new transfer events per endpoint. */
	atomic_t xfer_finished;			/* Bitmap of completed transfer events per EP. */
	uint8_t ctrl_out_buf[USB_CONTROL_EP_MPS];	/* Buffer for control OUT endpoint data. */
	uint8_t setup[sizeof(struct usb_setup_packet)];	/* Buffer for setup packet. */
};

/*
 * Converts a USB endpoint address into a linear bitmap index (0–31)
 * used for endpoint event tracking. OUT endpoints map to indices
 * 0–15, and IN endpoints map to indices 16–31.
 */
static uint8_t udc_ep_to_bnum(const uint8_t ep)
{
	uint8_t idx = USB_EP_GET_IDX(ep);

	return (USB_EP_GET_DIR(ep) == USB_EP_DIR_IN) ? (USB_EP_IN_OFFSET + idx) : idx;
}

/*
 * Returns one pending USB endpoint address from the event bitmap.
 *
 * Each bit in the bitmap represents an endpoint and direction.
 * The function picks the lowest set bit, converts it to the
 * corresponding endpoint address, clears the bit, and returns it.
 */
static uint8_t udc_pull_ep_from_bmsk(uint32_t *const bitmap)
{
	unsigned int bit_idx;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit_idx = find_lsb_set(*bitmap) - 1U;
	*bitmap &= ~BIT(bit_idx);

	if (bit_idx >= USB_EP_IN_OFFSET) {
		return USB_EP_DIR_IN | (uint8_t)(bit_idx - USB_EP_IN_OFFSET);
	}

	return USB_EP_DIR_OUT | (uint8_t)bit_idx;
}

/*
 * Waits until the USB controller finishes synchronization by polling
 * the SYNCBUSY flag until it clears.
 */
static int udc_wait_syncbusy_clear(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	if (WAIT_FOR(base->USB_SYNCBUSY == 0U, TIMEOUT_SYNCBUSY_RDY_US, NULL) == false) {
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * Loads USB pad calibration values from non-volatile memory and applies
 * them to the controller's PADCAL register. Uses fuse/OTP calibration data
 * and substitutes default values if the fuse entries indicate "unused."
 * All parameters are configured via device tree properties.
 */
static inline void udc_load_padcal(const struct device *dev)
{
	const struct udc_mchp_config *cfg = dev->config;
	usb_device_registers_t *const base = cfg->base;

	uint32_t padcal = 0U;
	uint32_t dst, bit, mask, def;
	uint32_t reg = 0U, val, bit_pos;
	uintptr_t addr = 0U, prev_addr = UINTPTR_MAX;

	for (size_t i = 0U; i < cfg->calib_len; i += CALIB_TUPLE_WIDTH) {

		dst  = cfg->calib[i];
		bit  = cfg->calib[i + 1U];
		mask = cfg->calib[i + 2U];
		def  = cfg->calib[i + 3U];

		/* Compute 32-bit OTP word address from absolute bit offset */
		addr = cfg->nvm_reg +
		       ((bit >> OTP_WORD_SHIFT) << OTP_BYTE_SHIFT);

		/* Cache OTP read */
		if (addr != prev_addr) {
			reg = sys_read32(addr);
			prev_addr = addr;
		}

		/* Bit position inside selected OTP word */
		bit_pos = bit & OTP_BIT_INDEX_MASK;

		val = (reg >> bit_pos) & mask;

		/* Invalid OTP pattern → fallback to default */
		if (val == mask) {
			val = def;
		}

		padcal |= (val << dst);
	}

	base->USB_PADCAL = padcal;
}

/*
 * Configure and enable clocks required by the USB controller.
 */
static int udc_mchp_enable_clocks(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	struct clock_mchp_subsys_gclkperiph_config gclk_cfg = {
		.src = config->clock.gclk_src,
	};
	int ret;

	ret = clock_control_configure(config->clock.clock_dev,
				      config->clock.gclk_subsys,
				      &gclk_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure USB GCLK: %d", ret);
		return ret;
	}

	ret = clock_control_on(config->clock.clock_dev, config->clock.gclk_subsys);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable USB GCLK: %d", ret);
		return ret;
	}

	ret = clock_control_on(config->clock.clock_dev, config->clock.mclk_subsys);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable USB MCLK: %d", ret);
		return ret;
	}

	return 0;
}

/*
 * Converts an Endpoint maximum packet size into the hardware-specific
 * buffer descriptor size encoding. Asserts if an unsupported size
 * is provided.
 */
static uint8_t udc_encode_max_packet_size(uint16_t max_packet_size)
{
	__ASSERT_NO_MSG(
		((max_packet_size & (max_packet_size - 1U)) == 0U &&
		 max_packet_size >= USB_MIN_PACKET_SIZE &&
		 max_packet_size <= USB_MAX_STANDARD_PACKET_SIZE) ||
		(max_packet_size == USB_MAX_ISO_PACKET_SIZE)
	);

	if (max_packet_size == USB_MAX_ISO_PACKET_SIZE) {
		return USB_ENCODED_ISO_SIZE;
	}

	return (uint8_t)(ilog2(max_packet_size) - USB_PACKET_SIZE_LOG2_OFFSET);
}

/*
 * Returns the buffer descriptor for the specified endpoint.
 *
 * The descriptor is selected from the buffer descriptor table (BDT)
 * using the endpoint index extracted from the endpoint address.
 */
static struct mchp_ep_buffer_desc *udc_get_ebd(const struct device *dev, const uint8_t ep)
{
	const struct udc_mchp_config *config = dev->config;

	return &config->bdt[USB_EP_GET_IDX(ep)];
}

/*
 * Returns the hardware register pointer for the specified endpoint.
 *
 * The direction bit is ignored; only the endpoint index is used
 * to select the correct endpoint register block.
 */
static usb_device_endpoint_registers_t *udc_get_ep_reg(const struct device *dev, const uint8_t ep)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	return &base->DEVICE_ENDPOINT[USB_EP_GET_IDX(ep)];
}

/*
 * Prepares an OUT endpoint for receiving data from the host.
 *
 * Configures the buffer descriptor and endpoint registers for the
 * next OUT transaction. Returns -EBUSY if the controller is still
 * using the previous buffer.
 */
static int udc_prep_out(const struct device *dev, struct net_buf *const buf,
			struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	struct mchp_ep_buffer_desc *const bd = udc_get_ebd(dev, ep_cfg->addr);
	const uint16_t size = MIN(USB_MAX_BYTE_COUNT, net_buf_tailroom(buf));
	unsigned int lock_key;

	lock_key = irq_lock();
	if ((endpoint->USB_EPSTATUS & USB_DEVICE_EPSTATUS_BK0RDY_Msk) == 0U) {
		irq_unlock(lock_key);
		LOG_ERR("ep 0x%02x buffer is used by the controller", ep_cfg->addr);
		return -EBUSY;
	}

	if (ep_cfg->addr != USB_CONTROL_EP_OUT) {
		bd->bank0.addr = (uintptr_t)buf->data;
		bd->bank0.byte_count = 0;
		bd->bank0.multi_packet_size = size;
		bd->bank0.size = udc_encode_max_packet_size(udc_mps_ep_size(ep_cfg));
	}

	/* Hand buffer to hardware */
	endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_BK0RDY_Msk;

	irq_unlock(lock_key);

	LOG_DBG("Prepare OUT ep 0x%02x size %u", ep_cfg->addr, size);

	return 0;
}

/*
 * Prepares an IN endpoint for transmitting data to the host.
 *
 * Configures the buffer descriptor and endpoint registers with the
 * data to be sent. Returns -EAGAIN if the controller is still using
 * the previous buffer.
 */
static int udc_prep_in(const struct device *dev, struct net_buf *const buf,
	 struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	struct mchp_ep_buffer_desc *const bd = udc_get_ebd(dev, ep_cfg->addr);
	const uint16_t len = MIN(USB_MAX_BYTE_COUNT, buf->len);
	unsigned int lock_key;

	lock_key = irq_lock();

	if ((endpoint->USB_EPSTATUS & USB_DEVICE_EPSTATUS_BK1RDY_Msk) != 0U) {
		irq_unlock(lock_key);
		LOG_ERR("ep 0x%02x buffer is used by the controller", ep_cfg->addr);
		return -EAGAIN;
	}

	bd->bank1.addr = (uintptr_t)buf->data;
	bd->bank1.size = udc_encode_max_packet_size(udc_mps_ep_size(ep_cfg));
	bd->bank1.multi_packet_size = 0;
	bd->bank1.byte_count = len;
	bd->bank1.auto_zlp = 0;

	/* Hand buffer to hardware */
	endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_BK1RDY_Msk;

	irq_unlock(lock_key);

	LOG_DBG("Prepare IN ep 0x%02x length %u", ep_cfg->addr, len);

	return 0;
}

/*
 * Allocates and prepares a buffer for the control OUT (EP0 OUT) data stage.
 *
 * Creates a buffer of the requested length and queues it for receiving data
 * from the host. Returns an error if allocation fails.
 */
static int udc_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_ep_config *const ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);

	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(ep_cfg, buf);
	return udc_prep_out(dev, buf, ep_cfg);
}

/*
 * Releases all pending control endpoint transfers by freeing any queued
 * buffers for both control OUT and control IN endpoints.
 */
static void udc_drop_control_transfers(const struct device *dev)
{
	struct net_buf *buf;

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
	if (buf != NULL) {
		net_buf_unref(buf);
	}
}

/*
 * Handles a SETUP event on the control endpoint.
 *
 * Allocates a buffer for the received SETUP packet, updates the control
 * transfer state machine, and prepares the next stage of the transfer.
 * Depending on the request type, this may queue a data OUT stage, trigger
 * a data IN stage, or submit a status stage. Reports errors if allocation
 * or submission fails.
 */
static int udc_handle_evt_setup(const struct device *dev)
{
	struct udc_mchp_data *const priv = udc_get_private(dev);
	struct net_buf *buf;
	int err = 0;

	udc_drop_control_transfers(dev);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, priv->setup, sizeof(priv->setup));
	udc_ep_buf_set_setup(buf);

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/* Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", (void *)buf);

		err = udc_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		LOG_DBG("s:%p|feed for -in-status", (void *)buf);
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		LOG_DBG("s:%p|no data", (void *)buf);
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

/*
 * Handles completion of an IN (device-to-host) transfer for a USB endpoint.
 *
 * Retrieves the completed buffer, clears the busy flag, and processes the
 * transfer. For the control IN endpoint, advances the control transfer state
 * machine and handles status/no-data stages. For other endpoints, forwards
 * the completed buffer to the upper layer.
 *
 * Returns 0 on success, -ENOBUFS if no buffer was available, or a negative
 * error code from lower-level handlers.
 */
static int udc_handle_evt_din(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	int err = 0;

	if (ep_cfg == NULL) {
		LOG_ERR("Invalid parameter: ep_cfg is NULL");
		return -EINVAL;
	}

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep_cfg->addr);
		return -ENOBUFS;
	}

	udc_ep_set_busy(ep_cfg, false);

	/* Non-control endpoints: forward buffer directly */
	if (ep_cfg->addr != USB_CONTROL_EP_IN) {
		return udc_submit_ep_event(dev, buf, 0);
	}

	/* Handle control endpoint IN transfer completion */
	if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
		err = udc_ctrl_submit_status(dev, buf);
		if (err != 0) {
			LOG_WRN("Failed to submit control status stage (err=%d)", err);
		}
	}

	/* Advance control transfer state machine */
	udc_ctrl_update_stage(dev, buf);

	/* If next stage is STATUS OUT, prepare it */
	if (!udc_ctrl_stage_is_status_out(dev)) {
		return err;
	}

	net_buf_unref(buf);

	err = udc_ctrl_feed_dout(dev, 0);
	if (err == -ENOMEM) {
		LOG_WRN("Failed to feed DOUT (ENOMEM), retrying once...");
		err = udc_submit_ep_event(dev, NULL, err);
	}

	return err;
}

/*
 * Handles completion of an OUT (host-to-device) transfer for a USB endpoint.
 *
 * Retrieves the completed buffer, clears the busy flag, and processes the
 * transfer. For the control OUT endpoint, it advances the control transfer
 * state machine and handles status/data stages. For all other endpoints,
 * it notifies the upper layer of the completed transfer.
 *
 * Returns 0 on success, -ENODATA if no buffer was available, or a negative
 * error code from lower-level handlers.
 */
static int udc_handle_evt_dout(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	int err = 0;

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for OUT ep 0x%02x", ep_cfg->addr);
		return -ENODATA;
	}

	udc_ep_set_busy(ep_cfg, false);

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			LOG_DBG("dout:%p|status, feed >s", (void *)buf);

			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);
		if (udc_ctrl_stage_is_status_in(dev)) {
			err = udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		err = udc_submit_ep_event(dev, buf, 0);
	}

	return err;
}

/*
 * Starts the next pending transfer for an endpoint, if a buffer is available.
 *
 * Prepares the endpoint for an OUT or IN transfer depending on direction.
 * On failure, dequeues the buffer and reports an error; otherwise marks
 * the endpoint as busy.
 */
static void udc_handle_xfer_next(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	int err = 0;

	buf = udc_buf_peek(ep_cfg);

	if (buf == NULL) {
		return;
	}

	if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		err = udc_prep_out(dev, buf, ep_cfg);
	} else {
		err = udc_prep_in(dev, buf, ep_cfg);
	}

	if (err != 0) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, -ECONNREFUSED);
	} else {
		udc_ep_set_busy(ep_cfg, true);
	}
}

/*
 * Handles the XFER_FINISHED event, indicating that endpoint transfers
 * have completed. Processes completion for each endpoint, reports errors
 * if handlers fail, and starts the next transfer if the endpoint is idle.
 */
static void udc_handle_xfer_finished(const struct device *const dev)
{
	struct udc_mchp_data *const priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	uint32_t eps = 0;
	uint8_t ep = 0;
	int err = 0;

	eps = atomic_clear(&priv->xfer_finished);

	while (eps) {
		ep = udc_pull_ep_from_bmsk(&eps);
		ep_cfg = udc_get_ep_cfg(dev, ep);

		LOG_DBG("Finished event ep 0x%02x", ep);

		if (USB_EP_DIR_IS_IN(ep)) {
			err = udc_handle_evt_din(dev, ep_cfg);
		} else {
			err = udc_handle_evt_dout(dev, ep_cfg);
		}

		if (err) {
			udc_submit_event(dev, UDC_EVT_ERROR, err);
		}

		if (udc_ep_is_busy(ep_cfg)) {
			LOG_ERR("Endpoint 0x%02x busy", ep);
			continue;
		}

		udc_handle_xfer_next(dev, ep_cfg);
	}
}

/*
 * Handles the XFER_NEW event, indicating that new USB transfers
 * have been queued. Iterates over all endpoints flagged for new
 * transfers, skipping those that are currently busy, and starts
 * the next transfer for each available endpoint.
 */
static void udc_handle_xfer_new(const struct device *const dev)
{
	struct udc_mchp_data *const priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	uint32_t eps = 0;
	uint8_t ep = 0;

	eps = atomic_clear(&priv->xfer_new);

	while (eps) {
		ep = udc_pull_ep_from_bmsk(&eps);
		ep_cfg = udc_get_ep_cfg(dev, ep);

		LOG_INF("New transfer ep 0x%02x in the queue", ep);

		if (udc_ep_is_busy(ep_cfg)) {
			LOG_ERR("Endpoint 0x%02x busy", ep);
			continue;
		}

		udc_handle_xfer_next(dev, ep_cfg);
	}
}

/*
 * Handles a SETUP event for the USB device.
 *
 * Invokes the setup handler and, if an error occurs, submits an
 * error event to the upper layer.
 */
static void udc_handle_setup(const struct device *const dev)
{
	int err = 0;

	err = udc_handle_evt_setup(dev);

	if (err) {
		udc_submit_event(dev, UDC_EVT_ERROR, err);
	}
}

/*
 * Main event handler for the Microchip UDC driver.
 *
 * Waits for USB-related events (new transfers, completed transfers,
 * and SETUP packets) and dispatches the appropriate handlers for each.
 * Runs within the dedicated UDC worker thread.
 */
static void udc_thread_handler(const struct device *const dev)
{
	struct udc_mchp_data *const priv = udc_get_private(dev);

	uint32_t evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);

	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(MCHP_EVT_XFER_FINISHED)) {
		k_event_clear(&priv->events, BIT(MCHP_EVT_XFER_FINISHED));

		udc_handle_xfer_finished(dev);
	}

	if (evt & BIT(MCHP_EVT_XFER_NEW)) {
		k_event_clear(&priv->events, BIT(MCHP_EVT_XFER_NEW));

		udc_handle_xfer_new(dev);
	}

	if (evt & BIT(MCHP_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(MCHP_EVT_SETUP));

		udc_handle_setup(dev);
	}

	udc_unlock_internal(dev);
}

/*
 * Main loop for the UDC worker thread.
 *
 * Runs indefinitely and repeatedly calls udc_thread_handler() to
 * process USB device controller events.
 */
static void udc_thread(void *dev, void *arg1, void *arg2)
{
	while (true) {
		udc_thread_handler(dev);
	}
}

/*
 * Handles SETUP packet reception on the control OUT endpoint.
 *
 * Copies the SETUP packet into the driver’s buffer and posts a setup
 * event to the driver thread. Logs an error if the packet size is not 8 bytes.
 */
static void udc_handle_setup_isr(const struct device *dev)
{
	struct mchp_ep_buffer_desc *const bd = udc_get_ebd(dev, 0);
	struct udc_mchp_data *const priv = udc_get_private(dev);

	if (bd->bank0.byte_count != sizeof(struct usb_setup_packet)) {
		LOG_ERR("Wrong byte count %u for setup packet", bd->bank0.byte_count);
		return;
	}

	memcpy(priv->setup, priv->ctrl_out_buf, sizeof(priv->setup));
	k_event_post(&priv->events, BIT(MCHP_EVT_SETUP));
}

/*
 * Handles OUT endpoint (host-to-device) interrupt processing.
 *
 * Called when an OUT transfer completes. Copies received data into the
 * active buffer, prepares the next OUT transaction if space remains,
 * or marks the transfer as finished and notifies the driver thread.
 * Reports an error if no buffer is available.
 */
static void udc_handle_out_isr(const struct device *dev, const uint8_t ep)
{
	struct mchp_ep_buffer_desc *const bd = udc_get_ebd(dev, ep);
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep);
	struct udc_mchp_data *const priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct net_buf *buf = udc_buf_peek(ep_cfg);
	uint32_t size = 0;
	int submit_err = 0;

	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep);
		submit_err = udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	LOG_DBG("ISR ep 0x%02x byte_count %u room %u mps %u", ep, bd->bank0.byte_count,
		net_buf_tailroom(buf), udc_mps_ep_size(ep_cfg));

	size = MIN(bd->bank0.byte_count, net_buf_tailroom(buf));
	if (ep == USB_CONTROL_EP_OUT) {
		net_buf_add_mem(buf, priv->ctrl_out_buf, size);
	} else {
		net_buf_add(buf, size);
	}

	/*
	 * The remaining buffer size should actually be at least equal to MPS,
	 * if (net_buf_tailroom(buf) >= udc_mps_ep_size(ep_cfg) && ...,
	 * otherwise the controller may write outside the buffer, this must be
	 * fixed in the UDC buffer allocation.
	 */
	if (net_buf_tailroom(buf) && size == udc_mps_ep_size(ep_cfg)) {
		__maybe_unused int err = 0;

		if (ep == USB_CONTROL_EP_OUT) {
			/* This is the same as udc_prep_out() would do for the
			 * control OUT endpoint, but shorter.
			 */
			endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_BK0RDY_Msk;
		} else {
			err = udc_prep_out(dev, buf, ep_cfg);
			__ASSERT(err == 0, "Failed to start new OUT transaction");
		}
	} else {
		atomic_set_bit(&priv->xfer_finished, udc_ep_to_bnum(ep));
		k_event_post(&priv->events, BIT(MCHP_EVT_XFER_FINISHED));
	}
}

/*
 * Handles IN endpoint (device-to-host) interrupt processing.
 *
 * Called when an IN transfer completes. Updates the active buffer, prepares
 * the next IN transaction if more data remains, handles zero-length packets
 * when required, or marks the transfer as finished and notifies the driver
 * thread.
 */
static void udc_handle_in_isr(const struct device *dev, const uint8_t ep)
{
	struct mchp_ep_buffer_desc *const bd = udc_get_ebd(dev, ep);
	struct udc_mchp_data *const priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	__maybe_unused int err = 0;
	struct net_buf *buf = udc_buf_peek(ep_cfg);
	uint32_t len = 0;

	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep);
		(void)udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	len = bd->bank1.byte_count;
	LOG_DBG("ISR ep 0x%02x byte_count %u", ep, len);
	net_buf_pull(buf, len);

	if (buf->len) {
		err = udc_prep_in(dev, buf, ep_cfg);
		__ASSERT(err == 0, "Failed to start new IN transaction");
	} else {
		if (udc_ep_buf_has_zlp(buf)) {
			err = udc_prep_in(dev, buf, ep_cfg);
			__ASSERT(err == 0, "Failed to start new IN transaction");
			udc_ep_buf_clear_zlp(buf);
			return;
		}

		atomic_set_bit(&priv->xfer_finished, udc_ep_to_bnum(ep));
		k_event_post(&priv->events, BIT(MCHP_EVT_XFER_FINISHED));
	}
}

/*
 * Handles endpoint-specific USB interrupt processing for the given endpoint
 * index. Dispatches handlers for IN/OUT transfer completion and SETUP packet
 * reception.
 */
static void udc_handle_ep_isr(const struct device *dev, const uint8_t idx)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, idx);
	uint32_t intflag = endpoint->USB_EPINTFLAG;

	/* Clear endpoint interrupt flags */
	endpoint->USB_EPINTFLAG = intflag;

	if (intflag & USB_DEVICE_EPINTFLAG_TRCPT1_Msk) {
		udc_handle_in_isr(dev, idx | USB_EP_DIR_IN);
	}

	if (intflag & USB_DEVICE_EPINTFLAG_TRCPT0_Msk) {
		udc_handle_out_isr(dev, idx);
	}

	if (intflag & USB_DEVICE_EPINTFLAG_RXSTP_Msk) {
		udc_handle_setup_isr(dev);
	}
}

/*
 * Main USB interrupt service routine for the Microchip UDC driver.
 *
 * Handles endpoint interrupts, core USB events (SOF, reset, suspend,
 * resume), and error conditions, forwarding events to the upper USB
 * stack as needed.
 */
static void udc_mchp_isr_handler(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;
	uint32_t epintsmry = base->USB_EPINTSMRY;
	uint32_t intflag = 0;

	/* Check endpoint interrupts bit-by-bit */
	for (uint8_t idx = 0U; epintsmry != 0U; epintsmry >>= 1) {
		if ((epintsmry & 1) != 0U) {
			udc_handle_ep_isr(dev, idx);
		}
		idx++;
	}

	/* Clear interrupt flags */
	intflag = base->USB_INTFLAG;
	base->USB_INTFLAG = intflag;

	if (intflag & USB_DEVICE_INTFLAG_SOF_Msk) {
		udc_submit_event(dev, UDC_EVT_SOF, 0);
	}

	if (intflag & USB_DEVICE_INTFLAG_EORST_Msk) {
		usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, 0);

		/* Re-enable control endpoint interrupts */
		endpoint->USB_EPINTENSET = USB_DEVICE_EPINTENSET_TRCPT0_Msk
					|  USB_DEVICE_EPINTENSET_TRCPT1_Msk
					|  USB_DEVICE_EPINTENSET_RXSTP_Msk;
		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}


	if ((intflag & USB_DEVICE_INTFLAG_SUSPEND_Msk) &&
		!udc_is_suspended(dev)) {
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	if ((intflag & USB_DEVICE_INTFLAG_EORSM_Msk) &&
		udc_is_suspended(dev)) {
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}


	if (intflag & USB_DEVICE_INTFLAG_RAMACER_Msk) {
		udc_submit_event(dev, UDC_EVT_ERROR, -EINVAL);
	}
}

/*
 * Enqueues a buffer for transfer on a USB endpoint.
 *
 * Adds the buffer to the endpoint’s queue and, if the endpoint is not halted,
 * marks the transfer as pending and posts a new transfer event.
 */
static int udc_mchp_ep_enqueue(const struct device *dev, struct udc_ep_config *const ep_cfg,
	  struct net_buf *buf)
{
	struct udc_mchp_data *const priv = udc_get_private(dev);

	LOG_DBG("%s enqueue 0x%02x %p", dev->name, ep_cfg->addr, (void *)buf);
	udc_buf_put(ep_cfg, buf);

	if (!ep_cfg->stat.halted) {
		atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(MCHP_EVT_XFER_NEW));
	}

	return 0;
}

/*
 * Aborts all pending transfers for a USB endpoint.
 *
 * Clears the endpoint’s ready status, removes all queued buffers, and
 * notifies the upper layer that the transfers were aborted.
 */
static int udc_mchp_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		/* Set BK1RDY bit*/
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_BK1RDY_Msk;
	} else {
		/* Clear BK0RDY bit */
		endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_BK0RDY_Msk;
	}

	buf = udc_buf_get_all(ep_cfg);
	if (buf != NULL) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
		udc_ep_set_busy(ep_cfg, false);
	}

	irq_unlock(lock_key);

	return 0;
}

/*
 * Initializes the buffer descriptor for the control OUT endpoint (EP0 OUT)
 * to use a persistent buffer during device operation.
 */
static void udc_setup_control_out_ep(const struct device *dev)
{
	struct mchp_ep_buffer_desc *const bd = udc_get_ebd(dev, 0);
	struct udc_mchp_data *const priv = udc_get_private(dev);

	/* It will never be reassigned to anything else during device runtime. */
	bd->bank0.addr = (uintptr_t)priv->ctrl_out_buf;
	bd->bank0.multi_packet_size = 0;
	bd->bank0.size = udc_encode_max_packet_size(USB_CONTROL_EP_MPS);
	bd->bank0.auto_zlp = 0;
}

/*
 * Enables a USB endpoint by configuring its type and enabling the
 * appropriate interrupts. Returns an error if the endpoint type is
 * invalid or unsupported.
 */
static int udc_mchp_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	uint8_t type = 0;

	switch (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		type = USB_HW_EPTYPE_CONTROL;
		break;
	case USB_EP_TYPE_ISO:
		type = USB_HW_EPTYPE_ISOCHRONOUS;
		break;
	case USB_EP_TYPE_BULK:
		type = USB_HW_EPTYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		type = USB_HW_EPTYPE_INTERRUPT;
		break;
	default:
		return -EINVAL;
	}

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		udc_setup_control_out_ep(dev);
		endpoint->USB_EPINTENSET |= USB_DEVICE_EPINTENSET_RXSTP_Msk;
	}

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		endpoint->USB_EPCFG |= (USB_DEVICE_EPCFG_EPTYPE1_Msk &
					(_UINT8_(type) << USB_DEVICE_EPCFG_EPTYPE1_Pos));
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_BK1RDY_Msk;
		endpoint->USB_EPINTENSET |= USB_DEVICE_EPINTENSET_TRCPT1_Msk;
	} else {
		endpoint->USB_EPCFG |= (USB_DEVICE_EPCFG_EPTYPE0_Msk &
					(_UINT8_(type) << USB_DEVICE_EPCFG_EPTYPE0_Pos));
		endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_BK0RDY_Msk;
		endpoint->USB_EPINTENSET |= USB_DEVICE_EPINTENSET_TRCPT0_Msk;
	}
	LOG_DBG("Enable ep 0x%02x", ep_cfg->addr);

	return 0;
}

/*
 * Disables a USB endpoint and clears its configuration and interrupts.
 *
 * Clears the endpoint type, disables transfer-complete interrupts, and for
 * the control OUT endpoint also disables the SETUP interrupt.
 */
static int udc_mchp_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		endpoint->USB_EPINTENCLR |= USB_DEVICE_EPINTENCLR_RXSTP_Msk;
	}

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		endpoint->USB_EPINTENCLR |= USB_DEVICE_EPINTENCLR_TRCPT1_Msk;
		endpoint->USB_EPCFG &= ~USB_DEVICE_EPCFG_EPTYPE1_Msk;
	} else {
		endpoint->USB_EPINTENCLR |= USB_DEVICE_EPINTENCLR_TRCPT0_Msk;
		endpoint->USB_EPCFG &= ~USB_DEVICE_EPCFG_EPTYPE0_Msk;
	}

	LOG_DBG("Disable ep 0x%02x", ep_cfg->addr);

	return 0;
}

/*
 * Sets the halt (stall) condition on a USB endpoint, causing it to respond
 * with a STALL handshake until the halt is cleared.
 */
static int udc_mchp_ep_set_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);

	if (USB_EP_GET_DIR(ep_cfg->addr) == USB_EP_DIR_IN) {
		endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_STALLRQ1_Msk;
	} else {
		endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_STALLRQ0_Msk;
	}

	LOG_DBG("Set halt ep 0x%02x", ep_cfg->addr);
	if (USB_EP_GET_IDX(ep_cfg->addr) != 0) {
		ep_cfg->stat.halted = true;
	}

	return 0;
}

/*
 * Clears the halt (stall) condition on a USB endpoint so it can resume
 * normal data transfers.
 */
static int udc_mchp_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	struct udc_mchp_data *const priv = udc_get_private(dev);

	if (USB_EP_GET_IDX(ep_cfg->addr) == 0) {
		return 0;
	}

	if (USB_EP_GET_DIR(ep_cfg->addr) == USB_EP_DIR_IN) {
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_STALLRQ1_Msk;
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_DTGLIN_Msk;
	} else {
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_STALLRQ0_Msk;
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_DTGLOUT_Msk;
	}

	if ((udc_ep_is_busy(ep_cfg) == false) &&
		(udc_buf_peek(ep_cfg) != NULL)) {
		atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(MCHP_EVT_XFER_NEW));
	}

	LOG_DBG("Clear halt ep 0x%02x", ep_cfg->addr);
	ep_cfg->stat.halted = false;

	return 0;
}

/*
 * Sets the USB device address in the controller’s register.
 *
 * Enables address recognition when the address is non-zero; clears the
 * register when the address is zero.
 *
 * Always returns success.
 */
static int udc_mchp_set_address(const struct device *dev, const uint8_t addr)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	LOG_DBG("Set new address %u for %s", addr, dev->name);
	if (addr != 0) {
		base->USB_DADD = addr | USB_DEVICE_DADD_ADDEN_Msk;
	} else {
		base->USB_DADD = 0;
	}

	return 0;
}

/*
 * Issues a remote wakeup signal to the USB host, requesting it to resume
 * communication after the device has been suspended.
 *
 * Always returns success.
 */
static int udc_mchp_host_wakeup(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	LOG_DBG("Remote wakeup from %s", dev->name);
	base->USB_CTRLB |= USB_DEVICE_CTRLB_UPRSM_Msk;

	return 0;
}

/*
 * Returns the current USB device bus speed (high-speed or full-speed)
 * based on the driver’s capability flags.
 */
static enum udc_bus_speed udc_mchp_device_speed(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* This USB IP supports Low-speed or Full-speed only; report Full Speed. */
	return UDC_BUS_SPEED_FS;
}

/*
 * Enables and initializes the USB device controller.
 *
 * Performs a hardware reset, applies pin configuration, loads pad
 * calibration values, sets up control endpoints, configures descriptor
 * memory, enables key USB interrupts, and attaches the controller to
 * the USB bus. Returns an error if pinctrl configuration fails or if a
 * control endpoint cannot be enabled.
 */
static int udc_mchp_enable(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	const struct pinctrl_dev_config *const pcfg = config->pcfg;
	usb_device_registers_t *const base = config->base;
	int ret;

	ret = udc_mchp_enable_clocks(dev);
	if (ret != 0) {
		return ret;
	}

	/* Reset controller */
	base->USB_CTRLA |= USB_CTRLA_SWRST_Msk;

	/* Wait for synchronization */
	ret = udc_wait_syncbusy_clear(dev);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Change QOS values to have the best performance and correct USB
	 * behaviour.
	 */
	base->USB_QOSCTRL |= (USB_QOSCTRL_CQOS_Msk & (_UINT8_(2) << USB_QOSCTRL_CQOS_Pos));
	base->USB_QOSCTRL |= (USB_QOSCTRL_DQOS_Msk & (_UINT8_(2) << USB_QOSCTRL_DQOS_Pos));

	ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("Failed to apply default pinctrl state (%d)", ret);
		return ret;
	}

	udc_load_padcal(dev);

	base->USB_CTRLA |= USB_CTRLA_RUNSTDBY_Msk;
	base->USB_CTRLA &= ~USB_CTRLA_MODE_Msk;
	base->USB_CTRLB &= ~USB_DEVICE_CTRLB_SPDCONF_Msk;
	base->USB_CTRLB |= USB_DEVICE_CTRLB_SPDCONF_FS;

	base->USB_DESCADD = (uintptr_t)config->bdt;

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL,
				   USB_CONTROL_EP_MPS, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL,
				   USB_CONTROL_EP_MPS, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	base->USB_INTENSET = USB_DEVICE_INTENSET_EORSM_Msk | USB_DEVICE_INTENSET_EORST_Msk |
	USB_DEVICE_INTENSET_SUSPEND_Msk;

	base->USB_CTRLA |= USB_CTRLA_ENABLE_Msk;

	/* Wait for synchronization */
	ret = udc_wait_syncbusy_clear(dev);
	if (ret != 0) {
		return ret;
	}

	base->USB_CTRLB &= ~USB_DEVICE_CTRLB_DETACH_Msk;

	config->irq_enable_func(dev);
	LOG_DBG("Enable device %s", dev->name);

	return 0;
}

/*
 * Disables the USB device controller and its endpoints.
 *
 * Detaches the controller from the USB bus, disables the hardware block,
 * disables all IRQs, and shuts down both control endpoints. Returns an
 * error if either control endpoint fails to disable.
 */
static int udc_mchp_disable(const struct device *dev)
{
	int ret;
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	config->irq_disable_func(dev);
	base->USB_CTRLB |= USB_DEVICE_CTRLB_DETACH_Msk;
	base->USB_CTRLA &= ~USB_CTRLA_ENABLE_Msk;

	/* Wait for synchronization */
	ret = udc_wait_syncbusy_clear(dev);
	if (ret != 0) {
		return ret;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	LOG_DBG("Disable device %s", dev->name);

	return 0;
}

/*
 * Initializes the USB device controller.
 */
static int udc_mchp_init(const struct device *dev)
{
	LOG_DBG("Init device %s", dev->name);

	return 0;
}

/*
 * Shuts down the USB device controller.
 *
 * Disables interrupts, detaches from the USB bus, resets the hardware,
 * and brings the controller to a clean state.
 */
static int udc_mchp_shutdown(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;
	int ret;

	LOG_DBG("Shutdown device %s", dev->name);

	/* Disable IRQs from the device (if enabled) */
	if (config->irq_disable_func) {
		config->irq_disable_func(dev);
	}

	/* Detach from USB bus and disable the USB peripheral */
	base->USB_CTRLB |= USB_DEVICE_CTRLB_DETACH_Msk;
	base->USB_CTRLA &= ~USB_CTRLA_ENABLE_Msk;

	/* Issue a software reset to bring the hardware to a clean state */
	base->USB_CTRLA |= USB_CTRLA_SWRST_Msk;

	/* Wait for synchronization with the APB/USB bridge */
	ret = udc_wait_syncbusy_clear(dev);
	if (ret) {
		LOG_ERR("Timeout waiting for SWRST sync");
		return ret;
	}

	/* Clear descriptor table pointer */
	base->USB_DESCADD = 0;

	LOG_DBG("Shutdown complete for %s", dev->name);

	return 0;
}

/*
 * Configure and register USB endpoints for a given direction.
 */
static int udc_mchp_register_eps(const struct device *dev,
				 struct udc_ep_config *ep_cfg,
				 size_t num_eps,
				 uint8_t dir,
				 uint16_t mps)
{
	int err;

	for (int i = 0; i < num_eps; i++) {

		if (dir == USB_EP_DIR_IN) {
			ep_cfg[i].caps.in = 1;
		} else {
			ep_cfg[i].caps.out = 1;
		}

		if (i == 0) {
			ep_cfg[i].caps.control = 1;
			ep_cfg[i].caps.mps = USB_CONTROL_EP_MPS;
		} else {
			ep_cfg[i].caps.bulk = 1;
			ep_cfg[i].caps.interrupt = 1;
			ep_cfg[i].caps.iso = 1;
			ep_cfg[i].caps.mps = mps;
		}

		ep_cfg[i].addr = dir | i;

		err = udc_register_ep(dev, &ep_cfg[i]);
		if (err) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	return 0;
}

/*
 * Performs initial setup of the Microchip USB device g1 controller driver.
 *
 * Initializes internal state, configures all IN and OUT endpoints,
 * registers them with the USB stack, and starts the driver thread.
 *
 * Returns 0 on success, or a negative error code on failure.
 */
static int udc_mchp_driver_preinit(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	struct udc_mchp_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	uint16_t mps = USB_MAX_ISO_PACKET_SIZE;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	data->caps.can_detect_vbus = false;

	err = udc_mchp_register_eps(dev, config->ep_cfg_out,
				    config->num_of_eps, USB_EP_DIR_OUT, mps);
	if (err) {
		return err;
	}

	err = udc_mchp_register_eps(dev, config->ep_cfg_in,
				    config->num_of_eps, USB_EP_DIR_IN, mps);
	if (err) {
		return err;
	}

	config->make_thread(dev);

	return 0;
}

/*
 * Locks the USB device controller driver.
 *
 * Prevents concurrent access by locking the scheduler and
 * acquiring the driver’s internal lock.
 */
static void udc_mchp_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

/*
 * Unlocks the USB device controller driver.
 *
 * Releases the driver’s internal lock and then
 * unlocks the scheduler.
 */
static void udc_mchp_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

static const struct udc_api udc_mchp_api = {
	.lock = udc_mchp_lock,
	.unlock = udc_mchp_unlock,
	.device_speed = udc_mchp_device_speed,
	.init = udc_mchp_init,
	.enable = udc_mchp_enable,
	.disable = udc_mchp_disable,
	.shutdown = udc_mchp_shutdown,
	.set_address = udc_mchp_set_address,
	.host_wakeup = udc_mchp_host_wakeup,
	.ep_enable = udc_mchp_ep_enable,
	.ep_disable = udc_mchp_ep_disable,
	.ep_set_halt = udc_mchp_ep_set_halt,
	.ep_clear_halt = udc_mchp_ep_clear_halt,
	.ep_enqueue = udc_mchp_ep_enqueue,
	.ep_dequeue = udc_mchp_ep_dequeue,
};

#define UDC_MCHP_IRQ_ENABLE(i, n)							\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq), DT_INST_IRQ_BY_IDX(n, i, priority),	\
	udc_mchp_isr_handler, DEVICE_DT_INST_GET(n), 0);				\
	irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));

#define UDC_MCHP_IRQ_DISABLE(i, n) irq_disable(DT_INST_IRQ_BY_IDX(n, i, irq));

#define UDC_MCHP_IRQ_ENABLE_DEFINE(i, n)					\
	static void udc_mchp_irq_enable_func_##n(const struct device *dev)	\
	{									\
		LISTIFY(							\
		DT_INST_NUM_IRQS(n),						\
			UDC_MCHP_IRQ_ENABLE,					\
			(),							\
			n							\
		)								\
	}

#define UDC_MCHP_IRQ_DISABLE_DEFINE(i, n)					\
	static void udc_mchp_irq_disable_func_##n(const struct device *dev)	\
	{									\
		LISTIFY(							\
		DT_INST_NUM_IRQS(n),						\
		UDC_MCHP_IRQ_DISABLE,						\
			(),							\
			n							\
		)								\
	}

#define UDC_MCHP_PINCTRL_DT_INST_DEFINE(n)				\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),		\
			(PINCTRL_DT_INST_DEFINE(n)), ())

#define UDC_MCHP_PINCTRL_DT_INST_DEV_CONFIG_GET(n)					\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),				\
				((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

#define UDC_MCHP_GCLKPERIPH_NODE(n) DT_CHILD(DT_NODELABEL(gclkperiph), usb0)

#define UDC_MCHP_THREAD_DEFINE(n)								\
	K_THREAD_STACK_DEFINE(udc_mchp_stack_##n, CONFIG_UDC_MCHP_G1_STACK_SIZE);		\
												\
	static void udc_mchp_make_thread_##n(const struct device *dev)				\
	{											\
		struct udc_mchp_data *priv = udc_get_private(dev);				\
												\
		k_thread_create(&priv->thread_data, udc_mchp_stack_##n,				\
				K_THREAD_STACK_SIZEOF(udc_mchp_stack_##n), udc_thread,		\
				(void *)dev, NULL, NULL,					\
				K_PRIO_COOP(CONFIG_UDC_MCHP_G1_THREAD_PRIORITY), K_ESSENTIAL,	\
				K_NO_WAIT);							\
		k_thread_name_set(&priv->thread_data, dev->name);				\
	}

#define UDC_MCHP_CONFIG_DEFINE(n)								\
	static const uint8_t calib_map_##n[] = DT_INST_PROP(n, calib_mapping);			\
												\
	static __aligned(sizeof(void *)) struct mchp_ep_buffer_desc				\
		mchp_bdt_##n[DT_INST_PROP(n, num_bidir_endpoints)];				\
												\
	static struct udc_ep_config ep_cfg_out[DT_INST_PROP(n, num_bidir_endpoints)];		\
	static struct udc_ep_config ep_cfg_in[DT_INST_PROP(n, num_bidir_endpoints)];		\
												\
	static const struct udc_mchp_config udc_mchp_config_##n = {				\
		.base = (usb_device_registers_t *)DT_INST_REG_ADDR(n),				\
		.bdt = mchp_bdt_##n,								\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),				\
		.ep_cfg_in = ep_cfg_in,								\
		.ep_cfg_out = ep_cfg_out,							\
		.pcfg = UDC_MCHP_PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),				\
		.clock.mclk_subsys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),	\
		.clock.gclk_subsys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),	\
		.clock.gclk_src =								\
			(enum clock_mchp_gclkgen)						\
			DT_ENUM_IDX(UDC_MCHP_GCLKPERIPH_NODE(n),				\
					gclkperiph_src),					\
		.nvm_reg = DT_REG_ADDR(DT_INST_PHANDLE(n, nvm_calib)),				\
		.calib = calib_map_##n,								\
		.calib_len = DT_INST_PROP_LEN(n, calib_mapping),				\
		.irq_enable_func = udc_mchp_irq_enable_func_##n,				\
		.irq_disable_func = udc_mchp_irq_disable_func_##n,				\
		.make_thread = udc_mchp_make_thread_##n,					\
	}

#define UDC_MCHP_DATA_DEFINE(n)								\
	static struct udc_mchp_data udc_priv_##n = {};					\
	static struct udc_data udc_data_##n = {						\
	.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),				\
		.priv = &udc_priv_##n,							\
	};

#define UDC_MCHP_DEVICE_DEFINE(n)							\
	UDC_MCHP_PINCTRL_DT_INST_DEFINE(n);						\
	UDC_MCHP_IRQ_ENABLE_DEFINE(i, n);						\
	UDC_MCHP_IRQ_DISABLE_DEFINE(i, n);						\
	UDC_MCHP_THREAD_DEFINE(n);							\
	UDC_MCHP_DATA_DEFINE(n);							\
	UDC_MCHP_CONFIG_DEFINE(n);							\
	DEVICE_DT_INST_DEFINE(n, udc_mchp_driver_preinit, NULL, &udc_data_##n,		\
	 &udc_mchp_config_##n, POST_KERNEL,						\
	 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_mchp_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_MCHP_DEVICE_DEFINE)
