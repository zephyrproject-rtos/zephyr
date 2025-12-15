/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include "udc_common.h"

LOG_MODULE_REGISTER(udc_mchp_g1, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_usb_g1
#define TIMEOUT_SYNCBUSY_RDY       1000

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
	unsigned int byte_count : 14;    /* Number of bytes received */
	unsigned int multi_packet_size : 14;
	unsigned int size : 3;           /* Encoded buffer size */
	unsigned int auto_zlp : 1;       /* Auto zero-length packet */

	/* EXTREG (0x08) */
	unsigned int subpid : 4;         /* Protocol-specific PID */
	unsigned int variable : 11;      /* Controller-specific field */
	unsigned int reserved0 : 1;

	/* STATUS_BK (0x0A) */
	unsigned int erroflow : 1;       /* Overflow error */
	unsigned int crcerr : 1;         /* CRC error */
	unsigned int reserved1 : 6;

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
	unsigned int byte_count : 14;           /* Number of bytes in the buffer */
	unsigned int multi_packet_size : 14;    /* Multi-packet size for HB transfers */
	unsigned int size : 3;                  /* Encoded buffer size */
	unsigned int auto_zlp : 1;              /* Auto zero-length packet */

	uint8_t reserved0[2];

	/* STATUS_BK (0x1A) */
	unsigned int erroflow : 1;              /* Overflow error */
	unsigned int crcerr : 1;                /* CRC error */
	unsigned int reserved1 : 6;

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
	struct k_thread thread_data;	/* Driver thread context */
	struct k_event events;		/* Event flags for thread synchronization. */
	atomic_t xfer_new;		/* Bitmap of new transfer events per endpoint. */
	atomic_t xfer_finished;		/* Bitmap of completed transfer events per endpoint. */
	uint8_t ctrl_out_buf[64];	/* Buffer for control OUT endpoint data. */
	uint8_t setup[8];		/* Buffer for the most recent setup packet. */
};

/*
 * Converts a USB endpoint address into the internal buffer number used
 * by the driver. OUT endpoints map to buffer numbers 0–15, and IN endpoints
 * map to 16 and above.
 */
static inline int udc_ep_to_bnum(const uint8_t ep)
{
	uint8_t bnum = 0;

	if (USB_EP_DIR_IS_IN(ep)) {
		bnum = 16UL + USB_EP_GET_IDX(ep);
	} else {
		bnum = USB_EP_GET_IDX(ep);
	}

	return bnum;
}

/*
 * Extracts the next endpoint address from a bitmap.
 *
 * Finds the least significant set bit, clears it in the bitmap, and returns
 * the corresponding USB endpoint address. Bits 0–15 map to OUT endpoints,
 * and bits 16–31 map to IN endpoints.
 *
 * Asserts that the bitmap is valid and contains at least one set bit.
 */
static inline uint8_t udc_pull_ep_from_bmsk(uint32_t *const bitmap)
{
	unsigned int bit = 0;
	uint8_t ep = 0;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit = find_lsb_set(*bitmap) - 1;
	*bitmap &= ~BIT(bit);

	if (bit >= 16U) {
		ep = USB_EP_DIR_IN | (bit - 16U);
	} else {
		ep = USB_EP_DIR_OUT | bit;
	}

	return ep;
}

/*
 * Waits until the USB controller finishes synchronization by polling
 * the SYNCBUSY flag until it clears.
 */
static void udc_wait_syncbusy(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	if (WAIT_FOR((base->USB_SYNCBUSY != 0), TIMEOUT_SYNCBUSY_RDY, NULL) == false) {
		LOG_ERR("SYNC BUSY timed out");
		return;
	}
}

/*
 * Loads USB pad calibration values from non-volatile memory and applies
 * them to the controller’s PADCAL register. Uses fuse/OTP calibration data
 * and substitutes default values if the fuse entries indicate “unused.”
 */
static void udc_load_padcal(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;
	volatile uint32_t usbCalibValue = (*((uint32_t *)SW0_ADDR + 1));
	uint16_t usbPadValue = 0;

	usbPadValue = (uint16_t)(usbCalibValue & 0x001FU);
	if (usbPadValue == 0x001FU) {
		usbPadValue = 5;
	}
	base->USB_PADCAL |= USB_PADCAL_TRANSN(usbPadValue);

	usbPadValue = (uint16_t)((usbCalibValue >> 5) & 0x001FU);
	if (usbPadValue == 0x001FU) {
		usbPadValue = 29;
	}
	base->USB_PADCAL |= USB_PADCAL_TRANSP(usbPadValue);

	usbPadValue = (uint16_t)((usbCalibValue >> 10) & 0x0007U);
	if (usbPadValue == 0x0007U) {
		usbPadValue = 3;
	}
	base->USB_PADCAL |= USB_PADCAL_TRIM(usbPadValue);
}

/*
 * Converts a maximum packet size (MPS) into the hardware-specific
 * buffer descriptor size encoding. Asserts if an unsupported size
 * is provided.
 */
static uint8_t udc_get_bd_size(const uint16_t mps)
{
	uint8_t bd_size = 0;

	switch (mps) {
	case 8:
		bd_size = 0;
		break;
	case 16:
		bd_size = 1;
		break;
	case 32:
		bd_size = 2;
		break;
	case 64:
		bd_size = 3;
		break;
	case 128:
		bd_size = 4;
		break;
	case 256:
		bd_size = 5;
		break;
	case 512:
		bd_size = 6;
		break;
	case 1023:
		bd_size = 7;
		break;
	default:
		LOG_ERR("Wrong max packet size: %d", bd_size);
		bd_size = 0;
	}

	return bd_size;
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
	const uint16_t size = MIN(16383U, net_buf_tailroom(buf));
	unsigned int lock_key = 0;

	if (!(endpoint->USB_EPSTATUS & USB_DEVICE_EPSTATUS_BK0RDY_Msk)) {
		LOG_ERR("ep 0x%02x buffer is used by the controller", ep_cfg->addr);
		return -EBUSY;
	}

	lock_key = irq_lock();

	if (ep_cfg->addr != USB_CONTROL_EP_OUT) {
		bd->bank0.addr = (uintptr_t)buf->data;
		bd->bank0.byte_count = 0;

		bd->bank0.multi_packet_size = size;
		bd->bank0.size = udc_get_bd_size(udc_mps_ep_size(ep_cfg));
	}
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
	const uint16_t len = MIN(16383U, buf->len);
	unsigned int lock_key = 0;

	if (endpoint->USB_EPSTATUS & USB_DEVICE_EPSTATUS_BK1RDY_Msk) {
		LOG_ERR("ep 0x%02x buffer is used by the controller", ep_cfg->addr);
		return -EAGAIN;
	}

	lock_key = irq_lock();

	bd->bank1.addr = (uintptr_t)buf->data;
	bd->bank1.size = udc_get_bd_size(udc_mps_ep_size(ep_cfg));

	bd->bank1.multi_packet_size = 0;
	bd->bank1.byte_count = len;
	bd->bank1.auto_zlp = 0;

	/* Set BK1RDY bit */
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
	int ret = 0;

	if (buf == NULL) {
		ret = -ENOMEM;
	} else {
		udc_buf_put(ep_cfg, buf);
		ret = udc_prep_out(dev, buf, ep_cfg);
	}

	return ret;
}

/*
 * Releases all pending control endpoint transfers by freeing any queued
 * buffers for both control OUT and control IN endpoints.
 */
static void udc_drop_control_transfers(const struct device *dev)
{
	struct net_buf *buf = NULL;

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
	struct net_buf *buf = NULL;
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
	struct net_buf *buf = NULL;
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

	if (ep_cfg->addr == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
			err = udc_ctrl_submit_status(dev, buf);
			if (err != 0) {
				LOG_ERR("Failed to submit control status stage (err=%d)", err);
			}
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {

			/* IN transfer finished, submit buffer for status stage */
			net_buf_unref(buf);

			err = udc_ctrl_feed_dout(dev, 0);
			if (err == -ENOMEM) {
				LOG_WRN("Failed to feed DOUT (ENOMEM), retrying once...");
				err = udc_submit_ep_event(dev, NULL, err);
			}
		}
	} else {
		err = udc_submit_ep_event(dev, buf, 0);
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
	struct net_buf *buf = NULL;
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
	struct net_buf *buf = NULL;
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
	struct udc_ep_config *ep_cfg = NULL;
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
	struct udc_ep_config *ep_cfg = NULL;
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

	if (bd->bank0.byte_count != 8) {
		LOG_ERR("Wrong byte count %u for setup packet", bd->bank0.byte_count);
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
	int submit_err = 0;

	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep);
		submit_err = udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
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

	if (intflag & USB_DEVICE_INTFLAG_SUSPEND_Msk) {
		if (!udc_is_suspended(dev)) {
			udc_set_suspended(dev, true);
			udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		}
	}

	if (intflag & USB_DEVICE_INTFLAG_EORSM_Msk) {
		if (udc_is_suspended(dev)) {
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
		}
	}

	/*
	 * This controller does not support VBUS status detection. To work
	 * smoothly, we should consider whether it would be possible to use the
	 * GPIO pin for VBUS state detection (e.g. PA7 on SAM R21 Xplained Pro).
	 */

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
	unsigned int lock_key = 0;
	struct net_buf *buf = NULL;

	lock_key = irq_lock();

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		/* Set BK1RDY bit*/
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_BK1RDY_Msk;
	} else {
		/* Clear BK0RDY bit */
		endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_BK0RDY_Msk;
	}

	buf = udc_buf_get_all(ep_cfg);
	if (buf) {
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
	bd->bank0.size = udc_get_bd_size(64);
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
		type = 1;
		break;
	case USB_EP_TYPE_ISO:
		type = 2;
		break;
	case USB_EP_TYPE_BULK:
		type = 3;
		break;
	case USB_EP_TYPE_INTERRUPT:
		type = 4;
		break;
	default:
		return -EINVAL;
	}

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		udc_setup_control_out_ep(dev);
		endpoint->USB_EPINTENSET = USB_DEVICE_EPINTENSET_RXSTP_Msk;
	}

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		endpoint->USB_EPCFG |= (USB_DEVICE_EPCFG_EPTYPE1_Msk &
					(_UINT8_(type) << USB_DEVICE_EPCFG_EPTYPE1_Pos));
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_BK1RDY_Msk;
		endpoint->USB_EPINTENSET = USB_DEVICE_EPINTENSET_TRCPT1_Msk;
	} else {
		endpoint->USB_EPCFG |= (USB_DEVICE_EPCFG_EPTYPE0_Msk &
					(_UINT8_(type) << USB_DEVICE_EPCFG_EPTYPE0_Pos));
		endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_BK0RDY_Msk;
		endpoint->USB_EPINTENSET = USB_DEVICE_EPINTENSET_TRCPT0_Msk;
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
		endpoint->USB_EPINTENCLR = USB_DEVICE_EPINTENCLR_RXSTP_Msk;
	}

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		endpoint->USB_EPINTENCLR = USB_DEVICE_EPINTENCLR_TRCPT1_Msk;
		endpoint->USB_EPCFG &= ~USB_DEVICE_EPCFG_EPTYPE1_Msk;
	} else {
		endpoint->USB_EPINTENCLR = USB_DEVICE_EPINTENCLR_TRCPT0_Msk;
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

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
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

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_STALLRQ1_Msk;
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_DTGLIN_Msk;
	} else {
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_STALLRQ0_Msk;
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_DTGLOUT_Msk;
	}

	if (USB_EP_GET_IDX(ep_cfg->addr) != 0 && !udc_ep_is_busy(ep_cfg)) {
		if (udc_buf_peek(ep_cfg)) {
			atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(ep_cfg->addr));
			k_event_post(&priv->events, BIT(MCHP_EVT_XFER_NEW));
		}
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
	struct udc_data *data = dev->data;

	return data->caps.hs ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
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
	int ret = 0;

	/* Reset controller */
	base->USB_CTRLA |= USB_CTRLA_SWRST_Msk;
	udc_wait_syncbusy(dev);

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

	base->USB_CTRLA = USB_CTRLA_RUNSTDBY_Msk;
	base->USB_CTRLA &= ~USB_CTRLA_MODE_Msk;
	base->USB_CTRLB = USB_DEVICE_CTRLB_SPDCONF_FS;

	base->USB_DESCADD = (uintptr_t)config->bdt;

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	base->USB_INTENSET = USB_DEVICE_INTENSET_EORSM_Msk | USB_DEVICE_INTENSET_EORST_Msk |
	USB_DEVICE_INTENSET_SUSPEND_Msk;

	base->USB_CTRLA |= USB_CTRLA_ENABLE_Msk;
	udc_wait_syncbusy(dev);
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
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	config->irq_disable_func(dev);
	base->USB_CTRLB |= USB_DEVICE_CTRLB_DETACH_Msk;
	base->USB_CTRLA &= ~USB_CTRLA_ENABLE_Msk;
	udc_wait_syncbusy(dev);

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
 *
 * This is a placeholder implementation. For this hardware, no initialization
 * is required at this stage because VBUS detection is not supported and no
 * additional setup is needed.
 *
 * Always returns success.
 */
static int udc_mchp_init(const struct device *dev)
{
	LOG_DBG("Init device %s", dev->name);

	return 0;
}

/*
 * Shuts down the USB device controller.
 *
 * Currently implemented as a placeholder. Always returns success.
 */
static int udc_mchp_shutdown(const struct device *dev)
{
	LOG_DBG("Shutdown device %s", dev->name);

	return 0;
}

/*
 * Performs pre initialization of the Microchip USB device controller driver.
 *
 * Initializes synchronization primitives, sets driver capability flags, and
 * configures all IN and OUT endpoints with their respective capabilities.
 * Registers each endpoint with the USB device stack and creates the driver
 * thread for handling deferred processing.
 *
 * Returns 0 on success, or a negative error code if endpoint registration fails.
 */
static int udc_mchp_driver_preinit(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	struct udc_mchp_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	uint16_t mps = 1023;
	int err = 0;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = mps;
		}

		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = 64;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = mps;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	config->make_thread(dev);

	return 0;
}

/*
 * Locks the USB device controller driver for exclusive access.
 *
 * Locks the scheduler and acquires the internal driver lock.
 */
static void udc_mchp_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

/*
 * Unlocks the USB device controller driver.
 *
 * Releases the internal driver lock and then unlocks the scheduler.
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


#define UDC_MCHP_CONFIG_DEFINE(n)							\
	static __aligned(sizeof(void *)) struct mchp_ep_buffer_desc			\
		mchp_bdt_##n[DT_INST_PROP(n, num_bidir_endpoints)];			\
											\
	static struct udc_ep_config ep_cfg_out[DT_INST_PROP(n, num_bidir_endpoints)];	\
	static struct udc_ep_config ep_cfg_in[DT_INST_PROP(n, num_bidir_endpoints)];	\
											\
	static const struct udc_mchp_config udc_mchp_config_##n = {			\
		.base = (usb_device_registers_t *)DT_INST_REG_ADDR(n),			\
		.bdt = mchp_bdt_##n,							\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),			\
		.ep_cfg_in = ep_cfg_in,							\
		.ep_cfg_out = ep_cfg_out,						\
		.irq_enable_func = udc_mchp_irq_enable_func_##n,			\
		.irq_disable_func = udc_mchp_irq_disable_func_##n,			\
		.pcfg = UDC_MCHP_PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.make_thread = udc_mchp_make_thread_##n,				\
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
											\
	DEVICE_DT_INST_DEFINE(n, udc_mchp_driver_preinit, NULL, &udc_data_##n,		\
	 &udc_mchp_config_##n, POST_KERNEL,						\
	 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_mchp_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_MCHP_DEVICE_DEFINE)
