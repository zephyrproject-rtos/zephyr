/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file udc_mchp_g1.c
 * @brief USB Device Controller (UDC) driver for Microchip Group 1 USB peripherals.
 */

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include "udc_common.h"

LOG_MODULE_REGISTER(udc_mchp_g1, CONFIG_UDC_DRIVER_LOG_LEVEL);

/* Compatibility string for device tree matching */
#define DT_DRV_COMPAT microchip_usb_g1

/**
 * @brief Endpoint buffer descriptor for bank 0 (OUT endpoints).
 *
 * Describes the buffer descriptor fields used by OUT endpoints in the
 * Microchip G1 USB device controller.
 *
 * Register layout:
 * - 0x00: Buffer address
 * - 0x04: PCKSIZE (packet size and transfer control)
 * - 0x08: EXTREG (protocol-specific signaling)
 * - 0x0A: STATUS_BK (status and error flags)
 */
struct mchp_ebd_bank0 {
	/** Buffer address for endpoint data. */
	uint32_t addr;

	/* PCKSIZE (0x04) */
	/** Number of bytes in the buffer. */
	unsigned int byte_count : 14;
	/** Multi-packet size for high-bandwidth transfers. */
	unsigned int multi_packet_size : 14;
	/** Encoded buffer size (see datasheet for encoding). */
	unsigned int size : 3;
	/** Automatically send a zero-length packet if needed. */
	unsigned int auto_zlp : 1;

	/* EXTREG (0x08) */
	/** Sub PID, used for protocol-specific signaling. */
	unsigned int subpid : 4;
	/** Variable field, controller-specific usage. */
	unsigned int variable : 11;
	/** Reserved. */
	unsigned int reserved0 : 1;

	/* STATUS_BK (0x0A) */
	/** Error overflow flag. */
	unsigned int erroflow : 1;
	/** CRC error flag. */
	unsigned int crcerr : 1;
	/** Reserved. */
	unsigned int reserved1 : 6;

	/** Reserved for alignment and future use. */
	uint8_t reserved2[5];
} __packed;


/**
 * @brief Endpoint buffer descriptor for bank 1 (IN endpoints).
 *
 * Describes the buffer descriptor fields used by IN endpoints in the
 * Microchip G1 USB device controller.
 *
 * Register layout:
 * - 0x10: Buffer address
 * - 0x14: PCKSIZE (packet size and transfer control)
 * - 0x1A: STATUS_BK (status and error flags)
 */
struct mchp_ebd_bank1 {
	/** Buffer address for endpoint data. */
	uint32_t addr;

	/* PCKSIZE (0x14) */
	/** Number of bytes in the buffer. */
	unsigned int byte_count : 14;
	/** Multi-packet size for high-bandwidth transfers. */
	unsigned int multi_packet_size : 14;
	/** Encoded buffer size (see datasheet for encoding). */
	unsigned int size : 3;
	/** Automatically send a zero-length packet if needed. */
	unsigned int auto_zlp : 1;

	/** Reserved for alignment and future use. */
	uint8_t reserved0[2];

	/* STATUS_BK (0x1A) */
	/** Error overflow flag. */
	unsigned int erroflow : 1;
	/** CRC error flag. */
	unsigned int crcerr : 1;
	/** Reserved. */
	unsigned int reserved1 : 6;

	/** Reserved for alignment and future use. */
	uint8_t reserved2[5];
} __packed;


/**
 * @brief Endpoint buffer descriptor for both IN and OUT endpoints.
 *
 * Combines the buffer descriptors for OUT (bank 0) and IN (bank 1) endpoints
 * in the Microchip G1 USB device controller.
 */
struct mchp_ep_buffer_desc {
	/** Buffer descriptor for OUT endpoints (0x00, 0x01, ... 0x08). */
	struct mchp_ebd_bank0 bank0;

	/** Buffer descriptor for IN endpoints (0x80, 0x81, ... 0x88). */
	struct mchp_ebd_bank1 bank1;
} __packed;


/**
 * @brief Compile-time check for endpoint buffer descriptor size.
 *
 * Ensures that the size of `struct mchp_ep_buffer_desc` matches
 * the hardware requirement (32 bytes). This prevents misaligned
 * memory accesses and buffer corruption.
 */
BUILD_ASSERT(sizeof(struct mchp_ep_buffer_desc) == 32,
             "Broken endpoint buffer descriptor: size must be 32 bytes");


/**
 * @brief Configuration structure for the Microchip USB Device Controller (UDC) driver.
 *
 * Holds all static configuration data required to initialize and operate
 * a Microchip UDC driver instance.
 */
struct udc_mchp_config {
	/** Base address of the USB device controller registers. */
	usb_device_registers_t *base;

	/** Pointer to the buffer descriptor table (BDT) for endpoints. */
	struct mchp_ep_buffer_desc *bdt;

	/** Number of bidirectional endpoints supported by the controller. */
	size_t num_of_eps;

	/** Array of endpoint configuration structures for IN endpoints. */
	struct udc_ep_config *ep_cfg_in;

	/** Array of endpoint configuration structures for OUT endpoints. */
	struct udc_ep_config *ep_cfg_out;

	/** Pin control configuration for the device. */
	struct pinctrl_dev_config *const pcfg;

	/** Function pointer to enable IRQs for the device. */
	void (*irq_enable_func)(const struct device *dev);

	/** Function pointer to disable IRQs for the device. */
	void (*irq_disable_func)(const struct device *dev);

	/** Function pointer to create the driver thread. */
	void (*make_thread)(const struct device *dev);
};


/**
 * @brief Event types for the Microchip USB Device Controller (UDC) driver.
 *
 * Defines the types of events handled by the Microchip UDC driver.
 */
enum mchp_event_type {
	/** Setup packet received on the control endpoint. */
	MCHP_EVT_SETUP,

	/** New transfer triggered (except control OUT endpoint). */
	MCHP_EVT_XFER_NEW,

	/** Transfer for a specific endpoint has finished. */
	MCHP_EVT_XFER_FINISHED,
};


/**
 * @brief Runtime data for the Microchip USB Device Controller (UDC) driver.
 *
 * Holds dynamic state for a UDC instance, including thread context,
 * event signaling, endpoint transfer tracking, and control buffers.
 */
struct udc_mchp_data {
	/** Driver thread context. */
	struct k_thread thread_data;

	/** Event flags for thread synchronization. */
	struct k_event events;

	/** Bitmap of new transfer events per endpoint. */
	atomic_t xfer_new;

	/** Bitmap of completed transfer events per endpoint. */
	atomic_t xfer_finished;

	/** Buffer for control OUT endpoint data. */
	uint8_t ctrl_out_buf[64];

	/** Buffer for the most recent setup packet. */
	uint8_t setup[8];
};



/**
 * @brief Convert a USB endpoint address to a buffer number.
 *
 * This function maps a USB endpoint address to a unique buffer number
 * used internally by the driver. IN endpoints are mapped to buffer numbers
 * 16 and above, while OUT endpoints are mapped to buffer numbers 0-15.
 *
 * @param ep USB endpoint address (including direction bit).
 * @return Buffer number corresponding to the endpoint address.
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

/**
 * @brief Extract the next endpoint address from a bitmap.
 *
 * This function finds the least significant set bit in the given bitmap,
 * clears it, and returns the corresponding USB endpoint address. Bits 0-15
 * represent OUT endpoints, and bits 16-31 represent IN endpoints.
 *
 * @param[in,out] bitmap Pointer to the endpoint bitmap. The function clears the bit it finds.
 * @return The USB endpoint address (with direction bit set).
 *
 * @note The function asserts that the bitmap pointer is valid and that at least one bit is set.
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

/**
 * @brief Wait for USB controller synchronization to complete.
 *
 * This function blocks until the USB controller's SYNCBUSY flag is cleared.
 *
 * @param dev Pointer to the device structure.
 */
static void udc_wait_syncbusy(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	while (base->USB_SYNCBUSY != 0) {
	}
}

/**
 * @brief Load and apply USB pad calibration values.
 *
 * This function reads the USB pad calibration values from the device's
 * non-volatile memory (fuse/OTP area) and applies them to the USB controller's
 * PADCAL register.
 *
 * @param dev Pointer to the device structure.
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

/**
 * @brief Get the buffer descriptor size encoding for a given maximum packet size.
 *
 * This function converts a USB endpoint's maximum packet size (MPS) to the
 * corresponding size encoding required by the hardware buffer descriptor.
 *
 * @param mps The maximum packet size for the endpoint (in bytes).
 * @return The encoded size value for the buffer descriptor.
 *
 * @note If an unsupported packet size is provided, the function asserts and returns 0.
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
		__ASSERT(true, "Wrong maximum packet size value");
		bd_size = 0;
	}

	return bd_size;
}

/**
 * @brief Get the endpoint buffer descriptor for a given endpoint.
 *
 * This function returns a pointer to the buffer descriptor associated with
 * the specified endpoint. The buffer descriptor table (BDT) is indexed by
 * the endpoint index extracted from the endpoint address.
 *
 * @param dev Pointer to the device structure.
 * @param ep  USB endpoint address (including direction bit).
 * @return Pointer to the corresponding @ref mchp_ep_buffer_desc structure.
 */
static struct mchp_ep_buffer_desc *udc_get_ebd(const struct device *dev, const uint8_t ep)
{
	const struct udc_mchp_config *config = dev->config;

	return &config->bdt[USB_EP_GET_IDX(ep)];
}

/**
 * @brief Get the endpoint register pointer for a given endpoint.
 *
 * This function returns a pointer to the hardware register
 * associated with the specified endpoint.
 *
 * @param dev Pointer to the device structure.
 * @param ep  USB endpoint address (including direction bit).
 * @return Pointer to the @ref usb_device_endpoint_registers_t structure for the endpoint.
 */
static usb_device_endpoint_registers_t *udc_get_ep_reg(const struct device *dev, const uint8_t ep)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	return &base->DEVICE_ENDPOINT[USB_EP_GET_IDX(ep)];
}

/**
 * @brief Prepare an OUT endpoint for data reception.
 *
 * This function sets up the buffer descriptor and hardware registers
 * for an OUT endpoint to receive data from the USB host.
 *
 * @param dev     Pointer to the device structure.
 * @param buf     Pointer to the net_buf structure containing the data buffer.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0       Success.
 * @retval -EBUSY  The buffer is currently in use by the controller.
 */
static int udc_prep_out(const struct device *dev, struct net_buf *const buf,
			struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	struct mchp_ep_buffer_desc *const bd = udc_get_ebd(dev, ep_cfg->addr);
	const uint16_t size = MIN(16383U, net_buf_tailroom(buf));
	unsigned int lock_key = 0;
	int ret = 0;

	if (!(endpoint->USB_EPSTATUS & USB_DEVICE_EPSTATUS_BK0RDY_Msk)) {
		LOG_ERR("ep 0x%02x buffer is used by the controller", ep_cfg->addr);
		ret = -EBUSY;
	} else {
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
	}

	return ret;
}

/**
 * @brief Prepare an IN endpoint for data transmission.
 *
 * This function sets up the buffer descriptor and hardware registers
 * for an IN endpoint to transmit data to the USB host.
 *
 * @param dev     Pointer to the device structure.
 * @param buf     Pointer to the net_buf structure containing the data to send.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0        Success.
 * @retval -EAGAIN  The buffer is currently in use by the controller.
 */
static int udc_prep_in(const struct device *dev, struct net_buf *const buf,
		       struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	struct mchp_ep_buffer_desc *const bd = udc_get_ebd(dev, ep_cfg->addr);
	const uint16_t len = MIN(16383U, buf->len);
	unsigned int lock_key = 0;
	int ret = 0;

	if (endpoint->USB_EPSTATUS & USB_DEVICE_EPSTATUS_BK1RDY_Msk) {
		LOG_ERR("ep 0x%02x buffer is used by the controller", ep_cfg->addr);
		ret = -EAGAIN;
	} else {
		lock_key = irq_lock();

		bd->bank1.addr = (uintptr_t)buf->data;
		bd->bank1.size = udc_get_bd_size(udc_mps_ep_size(ep_cfg));

		bd->bank1.multi_packet_size = 0;
		bd->bank1.byte_count = len;
		bd->bank1.auto_zlp = 0;

		endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_BK1RDY_Msk;
		irq_unlock(lock_key);

		LOG_DBG("Prepare IN ep 0x%02x length %u", ep_cfg->addr, len);
	}

	return ret;
}

/**
 * @brief Allocate and prepare a buffer for a control OUT endpoint data stage.
 *
 * This function allocates a buffer for the control OUT endpoint (EP0 OUT)
 * and prepares it for receiving data from the host.
 *
 * @param dev    Pointer to the device structure.
 * @param length Number of bytes to allocate for the buffer.
 *
 * @retval 0         Success.
 * @retval -ENOMEM   Buffer allocation failed (out of memory).
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
/**
 * @brief Drop (release) all pending control endpoint transfers.
 *
 * This function releases (unreferences) all buffers associated with the
 * control OUT and control IN endpoints.
 *
 * @param dev Pointer to the device structure.
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

/**
 * @brief Handle a SETUP event on the control endpoint.
 *
 * This function processes a received SETUP packet on the control endpoint.
 *
 * @param dev Pointer to the device structure.
 *
 * @retval 0         Success.
 * @retval -ENOMEM   Buffer allocation failed (out of memory).
 * @retval <0        Error code from lower-level transfer preparation or submission.
 */
static int udc_handle_evt_setup(const struct device *dev)
{
	struct udc_mchp_data *const priv = udc_get_private(dev);
	struct net_buf *buf = NULL;
	int err = 0;
	int submit_err = 0;
	
	if (!priv) {
		return -EINVAL;
	}
	
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
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", (void *)buf);

		err = udc_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			do {
				submit_err = udc_submit_ep_event(dev, buf, err);
			} while (submit_err != 0);
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

/**
 * @brief Handle an IN transfer complete event for a USB endpoint.
 *
 * This function processes the completion of an IN transfer (device-to-host)
 * for the specified endpoint. For the control IN endpoint, it manages the
 * control transfer state machine, including status and data stages. For other
 * endpoints, it notifies the upper layer of the completed transfer.
 *
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0          Success.
 * @retval -ENOBUFS   No buffer was available for the endpoint.
 * @retval <0         Error code from lower-level transfer preparation or submission.
 */
static int udc_handle_evt_din(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf = NULL;
	int err = 0;
	int submit_err = 0;

	do {
		buf = udc_buf_get(ep_cfg);
		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep_cfg->addr);
			err = -ENOBUFS;
			break;
		}

		udc_ep_set_busy(ep_cfg, false);

		if (ep_cfg->addr == USB_CONTROL_EP_IN) {
			if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
				/* Status stage finished, notify upper layer */
				udc_ctrl_submit_status(dev, buf);
			}

			/* Update to next stage of control transfer */
			udc_ctrl_update_stage(dev, buf);

			if (udc_ctrl_stage_is_status_out(dev)) {

				/* IN transfer finished, submit buffer for status stage */
				net_buf_unref(buf);

				err = udc_ctrl_feed_dout(dev, 0);
				if (err == -ENOMEM) {
					do {
						submit_err = udc_submit_ep_event(dev, buf, err);
					} while (submit_err != 0);
				} else {
					break;
				}
			}

			err = 0;
			break;
		}

		err = udc_submit_ep_event(dev, buf, 0);
		break;
	} while (0);

	return err;
}

/**
 * @brief Handle an OUT transfer finished event for a USB endpoint.
 *
 * This function processes the completion of an OUT transfer (host-to-device)
 * for the specified endpoint. For the control OUT endpoint, it manages the
 * control transfer state machine, including status and data stages. For other
 * endpoints, it notifies the upper layer of the completed transfer.
 *
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0         Success.
 * @retval -ENODATA  No buffer was available for the endpoint.
 * @retval <0        Error code from lower-level transfer preparation or submission.
 */
static inline int udc_handle_evt_dout(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf = NULL;
	int err = 0;

	do {
		buf = udc_buf_get(ep_cfg);
		if (buf == NULL) {
			LOG_ERR("No buffer for OUT ep 0x%02x", ep_cfg->addr);
			err = -ENODATA;
			break;
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
	} while (0);

	return err;
}

/**
 * @brief Prepare and start the next transfer for a USB endpoint, if available.
 *
 * This function checks if there is a pending buffer for the specified endpoint.
 * If a buffer is available, it prepares the endpoint for the next transfer
 * (using either OUT or IN preparation as appropriate).
 *
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 */
static void udc_handle_xfer_next(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf = NULL;
	int err = 0;
	int submit_err = 0;

	buf = udc_buf_peek(ep_cfg);
	do {
		if (buf == NULL) {
			break;
		}

		if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
			err = udc_prep_out(dev, buf, ep_cfg);
		} else {
			err = udc_prep_in(dev, buf, ep_cfg);
		}

		if (err != 0) {
			buf = udc_buf_get(ep_cfg);
			do {
				submit_err = udc_submit_ep_event(dev, buf, -ECONNREFUSED);
			} while (submit_err != 0);
		} else {
			udc_ep_set_busy(ep_cfg, true);
		}
	} while (0);
}

/**
 * @brief Submit a USB error event for the device, retrying until successful.
 *
 * This function attempts to submit an error event to the USB device.
 * If submission fails, it retries in a loop until the event is successfully submitted.
 *
 * @param dev  Pointer to the device structure.
 * @param err  Error code to submit with the event.
 */
static inline void udc_submit_error_event(const struct device *const dev, int err)
{
    int submit_err = 0;
    do {
        submit_err = udc_submit_event(dev, UDC_EVT_ERROR, err);
    } while (submit_err != 0);
}

/**
 * @brief Handle the XFER_FINISHED USB event for the device.
 *
 * This function processes the XFER_FINISHED event, which indicates that
 * one or more USB endpoint transfers have completed. For each endpoint
 * flagged by the event, it calls the appropriate IN or OUT event handler,
 * submits an error event if handling fails, and triggers the next transfer
 * if the endpoint is not busy.
 *
 * @param dev  Pointer to the USB device structure.
 */
static inline void udc_handle_xfer_finished(const struct device *const dev)
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
            udc_submit_error_event(dev, err);  
        }

        if (udc_ep_is_busy(ep_cfg)) {
            LOG_ERR("Endpoint 0x%02x busy", ep);
            continue;                       
        }

        udc_handle_xfer_next(dev, ep_cfg);
    }
}

/**
 * @brief Handle the XFER_NEW USB event for the device.
 *
 * This function processes the XFER_NEW event, which indicates that
 * one or more new USB transfers are queued for the device. For each
 * endpoint flagged by the event, it checks if the endpoint is busy.
 * If not busy, it triggers the next transfer in the queue. Busy
 * endpoints are logged as errors.
 *
 * @param dev  Pointer to the USB device structure.
 */
static ALWAYS_INLINE void udc_handle_xfer_new(const struct device *const dev)
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

/**
 * @brief Handle the SETUP USB event for the device.
 *
 * This function processes the SETUP event, which indicates that
 * a USB setup packet has been received. It calls the setup event
 * handler and submits an error event if the handling fails.
 *
 * @param dev  Pointer to the USB device structure.
 */
static inline void udc_handle_setup(const struct device *const dev)
{
    int err = 0;

    err = udc_handle_evt_setup(dev);

    if (err) {
        udc_submit_error_event(dev, err);  
    }
}

/**
 * @brief Main event handler thread for the Microchip UDC driver.
 *
 * This function is intended to be run as the main thread for the USB device controller
 * driver. It waits for events (such as transfer completion, new transfer requests, or setup
 * packets) and dispatches the appropriate handlers for each event type.
 *
 * @param dev Pointer to the device structure.
 */
static inline void udc_thread_handler(const struct device *const dev)
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

/**
 * @brief UDC thread main loop.
 *
 * This static function runs an infinite loop, repeatedly calling the
 * udc_thread_handler(). It is intended to be used as a thread
 * entry point for handling USB Device Controller (UDC) events.
 *
 * @param[in] dev   Pointer to the device-specific data structure.
 * @param[in] arg1  Unused argument, reserved for future use.
 * @param[in] arg2  Unused argument, reserved for future use.
 *
 * @note This function does not return; it runs indefinitely.
 */
static void udc_thread(void *dev, void *arg1, void *arg2)
{
	while (true) {
		udc_thread_handler(dev);
	}
}

/**
 * @brief Handle the SETUP packet interrupt service routine (ISR).
 *
 * This function is called from the USB interrupt context when a SETUP packet
 * is received on the control OUT endpoint.
 *
 * @param dev Pointer to the device structure.
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

/**
 * @brief Handle the OUT endpoint interrupt service routine (ISR).
 *
 * This function is called from the USB interrupt context when an OUT transfer
 * (host-to-device) completes on the specified endpoint. It processes the received
 * data, updates the buffer, and either prepares the endpoint for the next transfer
 * or signals the main driver thread that the transfer is finished.
 *
 * @param dev Pointer to the device structure.
 * @param ep  USB endpoint address (without direction bit).
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

	do {
		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep);
			do {
				submit_err = udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			} while (submit_err != 0);

			break;
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
			__maybe_unused int err;

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
	} while (0);
}

/**
 * @brief Handle the IN endpoint interrupt service routine (ISR).
 *
 * This function is called from the USB interrupt context when an IN transfer
 * (device-to-host) completes on the specified endpoint. It processes the transmitted
 * data, updates the buffer, and either prepares the endpoint for the next transfer
 * or signals the main driver thread that the transfer is finished.
 *
 * If there is more data to send, it prepares the next IN transaction. If a zero-length
 * packet (ZLP) is required, it handles that as well. Otherwise, it sets the transfer
 * finished flag and posts the event to the driver thread.
 *
 * @param dev Pointer to the device structure.
 * @param ep  USB endpoint address (with direction bit).
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

	do {
		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep);
			do {
				submit_err = udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			} while (submit_err != 0);
			break;
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
				break;
			}

			atomic_set_bit(&priv->xfer_finished, udc_ep_to_bnum(ep));
			k_event_post(&priv->events, BIT(MCHP_EVT_XFER_FINISHED));
		}
	} while (0);
}

/**
 * @brief Handle endpoint-specific interrupt service routine (ISR).
 *
 * This function is called from the main USB interrupt handler to process
 * endpoint-specific interrupt flags for a given endpoint index. It checks
 * which interrupt(s) occurred (IN transfer complete, OUT transfer complete,
 * or SETUP packet received) and dispatches the appropriate handler for each.
 *
 * @param dev Pointer to the device structure.
 * @param idx Endpoint index (without direction bit).
 */
static inline void udc_handle_ep_isr(const struct device *dev, const uint8_t idx)
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

/**
 * @brief Main USB interrupt service routine (ISR) handler for the Microchip UDC driver.
 *
 * This function is called when a USB interrupt occurs. It handles all USB Interrupts.
 *
 * @param dev Pointer to the device structure.
 */
static void udc_mchp_isr_handler(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;
	uint32_t epintsmry = base->USB_EPINTSMRY;
	uint32_t intflag = 0;
	int submit_err = 0;

	/* Check endpoint interrupts bit-by-bit */
	for (uint8_t idx = 0U; epintsmry != 0U; epintsmry >>= 1) {
		if ((epintsmry & 1) != 0U) {
			udc_handle_ep_isr(dev, idx);
		}

		idx++;
	}

	intflag = base->USB_INTFLAG;
	/* Clear interrupt flags */
	base->USB_INTFLAG = intflag;

	if (intflag & USB_DEVICE_INTFLAG_SOF_Msk) {
		do {
			submit_err = udc_submit_event(dev, UDC_EVT_SOF, 0);
		} while (submit_err != 0);
	}

	if (intflag & USB_DEVICE_INTFLAG_EORST_Msk) {
		usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, 0);

		/* Re-enable control endpoint interrupts */
		endpoint->USB_EPINTENSET = USB_DEVICE_EPINTENSET_TRCPT0_Msk |
					   USB_DEVICE_EPINTENSET_TRCPT1_Msk |
					   USB_DEVICE_EPINTENSET_RXSTP_Msk;
		do {
			submit_err = udc_submit_event(dev, UDC_EVT_RESET, 0);
		} while (submit_err != 0);
	}

	if (intflag & USB_DEVICE_INTFLAG_SUSPEND_Msk) {
		if (!udc_is_suspended(dev)) {
			udc_set_suspended(dev, true);
			do {
				submit_err = udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
			} while (submit_err != 0);
		}
	}

	if (intflag & USB_DEVICE_INTFLAG_EORSM_Msk) {
		if (udc_is_suspended(dev)) {
			udc_set_suspended(dev, false);
			do {
				submit_err = udc_submit_event(dev, UDC_EVT_RESUME, 0);
			} while (submit_err != 0);
		}
	}

	/*
	 * This controller does not support VBUS status detection. To work
	 * smoothly, we should consider whether it would be possible to use the
	 * GPIO pin for VBUS state detection (e.g. PA7 on SAM R21 Xplained Pro).
	 */

	if (intflag & USB_DEVICE_INTFLAG_RAMACER_Msk) {
		do {
			submit_err = udc_submit_event(dev, UDC_EVT_ERROR, -EINVAL);
		} while (submit_err != 0);
	}
}

/**
 * @brief Enqueue a buffer for transfer on a USB endpoint.
 *
 * This function adds a buffer to the transfer queue for the specified endpoint.
 * If the endpoint is not halted, it sets the corresponding bit in the transfer
 * bitmap and posts a new transfer event to the driver thread.
 *
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 * @param buf     Pointer to the net_buf structure to enqueue.
 *
 * @retval 0 Success.
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

/**
 * @brief Dequeue (abort) all buffers for a USB endpoint.
 *
 * This function aborts all pending transfers for the specified endpoint,
 * clears the endpoint's ready status, and notifies the upper layer of the
 * aborted transfer.
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0 Success.
 */
static int udc_mchp_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	unsigned int lock_key = 0;
	struct net_buf *buf = NULL;
	int submit_err = 0;

	lock_key = irq_lock();

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		endpoint->USB_EPSTATUSCLR |= USB_DEVICE_EPSTATUSCLR_BK1RDY_Msk;
	} else {
		endpoint->USB_EPSTATUSSET |= USB_DEVICE_EPSTATUSSET_BK0RDY_Msk;
	}

	buf = udc_buf_get_all(ep_cfg);
	if (buf) {
		do {
			submit_err = udc_submit_ep_event(dev, buf, -ECONNABORTED);
		} while (submit_err != 0);
		udc_ep_set_busy(ep_cfg, false);
	}

	irq_unlock(lock_key);

	return 0;
}

/**
 * @brief Initialize the buffer descriptor for the control OUT endpoint (EP0 OUT).
 *
 * This function sets up the buffer descriptor for the control OUT endpoint
 * to use a persistent buffer.
 *
 * @param dev Pointer to the device structure.
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

/**
 * @brief Enable a USB endpoint and configure its type and interrupts.
 *
 * This function configures and enables the specified endpoint.
 *
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0         Success.
 * @retval -EINVAL   Invalid or unsupported endpoint type.
 */
static int udc_mchp_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	uint8_t type = 0;
	int err = 0;

	do {
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
			err = -EINVAL;
			break;
		}
		if (err != 0) {
			break;
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

		err = 0;
	} while (0);

	return err;
}

/**
 * @brief Disable a USB endpoint and clear its configuration and interrupts.
 *
 * This function disables the specified endpoint by clearing its type configuration
 * and disabling the associated interrupts. For the control OUT endpoint, it also
 * disables the SETUP packet interrupt.
 *
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0 Success.
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

/**
 * @brief Set the halt (stall) condition on a USB endpoint.
 *
 * This function sets the STALL condition on the specified endpoint, causing
 * the endpoint to respond with a STALL handshake to the host until the halt
 * is cleared.
 *
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0 Success.
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

/**
 * @brief Clear the halt (stall) condition on a USB endpoint.
 *
 * This function clears the STALL condition on the specified endpoint, allowing
 * it to resume normal data transfers.
 *
 * @param dev     Pointer to the device structure.
 * @param ep_cfg  Pointer to the endpoint configuration structure.
 *
 * @retval 0 Success.
 */
static int udc_mchp_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	usb_device_endpoint_registers_t *const endpoint = udc_get_ep_reg(dev, ep_cfg->addr);
	struct udc_mchp_data *const priv = udc_get_private(dev);

	do {
		if (USB_EP_GET_IDX(ep_cfg->addr) == 0) {
			break;
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
	} while (0);

	return 0;
}

/**
 * @brief Set the USB device address.
 *
 * This function sets the USB device address in the controller's register.
 *
 * @param dev  Pointer to the device structure.
 * @param addr The new USB device address to set.
 *
 * @retval 0 Success.
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

/**
 * @brief Issue a remote wakeup signal to the USB host.
 *
 * This function triggers a remote wakeup event, requesting the host to resume
 * communication with the device after it has been suspended.
 *
 * @param dev Pointer to the device structure.
 *
 * @retval 0 Success.
 */
static int udc_mchp_host_wakeup(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	LOG_DBG("Remote wakeup from %s", dev->name);
	base->USB_CTRLB |= USB_DEVICE_CTRLB_UPRSM_Msk;

	return 0;
}

/**
 * @brief Get the current USB device bus speed.
 *
 * This function returns the current bus speed (high-speed or full-speed)
 * as reported by the device's capabilities.
 *
 * @param dev Pointer to the device structure.
 * @return The current USB bus speed (UDC_BUS_SPEED_HS or UDC_BUS_SPEED_FS).
 */
static enum udc_bus_speed udc_mchp_device_speed(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->caps.hs ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
}

/**
 * @brief Enable and initialize the USB device controller.
 *
 * This function performs all necessary steps to enable the USB device controller,
 * including hardware reset, pin configuration, pad calibration, endpoint setup,
 * and interrupt enabling.
 *
 * @param dev Pointer to the device structure.
 *
 * @retval 0        Success.
 * @retval -EIO     Failed to enable a control endpoint.
 * @retval <0       Error code from pin control configuration.
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
	do {
		if (ret != 0) {
			LOG_ERR("Failed to apply default pinctrl state (%d)", ret);
			break;
		}

		udc_load_padcal(dev);

		base->USB_CTRLA = USB_CTRLA_RUNSTDBY_Msk;
		base->USB_CTRLA &= ~USB_CTRLA_MODE_Msk;
		base->USB_CTRLB = USB_DEVICE_CTRLB_SPDCONF_FS;

		base->USB_DESCADD = (uintptr_t)config->bdt;

		if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0)) {
			LOG_ERR("Failed to enable control endpoint");
			ret = -EIO;
			break;
		}

		if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 64, 0)) {
			LOG_ERR("Failed to enable control endpoint");
			ret = -EIO;
			break;
		}

		base->USB_INTENSET = USB_DEVICE_INTENSET_EORSM_Msk | USB_DEVICE_INTENSET_EORST_Msk |
				     USB_DEVICE_INTENSET_SUSPEND_Msk;

		base->USB_CTRLA |= USB_CTRLA_ENABLE_Msk;
		udc_wait_syncbusy(dev);
		base->USB_CTRLB &= ~USB_DEVICE_CTRLB_DETACH_Msk;

		config->irq_enable_func(dev);
		LOG_DBG("Enable device %s", dev->name);

		ret = 0;
		break;
	} while (0);

	return ret;
}

/**
 * @brief Disable the USB device controller and its endpoints.
 *
 * This function disables the USB device controller by detaching it from the bus,
 * disabling the controller. It also disables
 * the control endpoints and disables all related interrupts.
 *
 * @param dev Pointer to the device structure.
 *
 * @retval 0     Success.
 * @retval -EIO  Failed to disable a control endpoint.
 */
static int udc_mchp_disable(const struct device *dev)
{
	const struct udc_mchp_config *config = dev->config;
	usb_device_registers_t *const base = config->base;

	config->irq_disable_func(dev);
	base->USB_CTRLB |= USB_DEVICE_CTRLB_DETACH_Msk;
	base->USB_CTRLA &= ~USB_CTRLA_ENABLE_Msk;
	udc_wait_syncbusy(dev);
	int err = 0;

	do {
		if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
			LOG_ERR("Failed to disable control endpoint");
			err = -EIO;
			break;
		}

		if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
			LOG_ERR("Failed to disable control endpoint");
			err = -EIO;
			break;
		}

		LOG_DBG("Disable device %s", dev->name);
	} while (0);

	return err;
}
/**
 * @brief Initialize the USB device controller.
 *
 * This function is a placeholder for controller initialization. For this
 * hardware, there is nothing to initialize at this stage because VBUS state
 * change detection is not supported and no other initialization is required.
 *
 * @param dev Pointer to the device structure.
 * @retval 0 Always returns success.
 */
static int udc_mchp_init(const struct device *dev)
{
	LOG_DBG("Init device %s", dev->name);

	return 0;
}

/**
 * @brief Shutdown the USB device controller.
 *
 * This function is a placeholder for controller shutdown.
 *
 * @param dev Pointer to the device structure.
 * @retval 0 Always returns success.
 */
static int udc_mchp_shutdown(const struct device *dev)
{
	LOG_DBG("Shutdown device %s", dev->name);

	return 0;
}

/**
 * @brief Pre-initialize the Microchip USB device controller driver.
 *
 * This function performs early initialization of the driver, including:
 * - Initializing synchronization primitives (mutex, events, atomics)
 * - Setting device capability flags
 * - Configuring and registering all IN and OUT endpoints
 * - Creating the driver thread
 *
 * It sets up endpoint capabilities for control, bulk, interrupt, and isochronous
 * transfers, and registers each endpoint with the USB device stack.
 *
 * @param dev Pointer to the device structure.
 *
 * @retval 0        Success.
 * @retval <0       Error code if endpoint registration fails.
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
	do {
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
				break;
			}
		}
		if (err != 0) {
			break;
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
				break;
			}
		}
		if (err != 0) {
			break;
		}

		config->make_thread(dev);
		err = 0;
	} while (0);

	return err;
}
/**
 * @brief Lock the USB device controller driver for exclusive access.
 *
 * This function locks the scheduler and acquires the internal driver lock.
 *
 * @param dev Pointer to the device structure.
 */
static void udc_mchp_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

/**
 * @brief Unlock the USB device controller driver.
 *
 * This function releases the internal driver lock and unlocks the scheduler.
 *
 * @param dev Pointer to the device structure.
 */
static void udc_mchp_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

/**
 * @brief API interface structure for the Microchip USB Device Controller (UDC) driver.
 *
 * This structure provides function pointers to all driver operations, allowing
 * the upper layer (USB device stack or application) to access the UDC driver's
 * functionality.
 */
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

/**
 * @def UDC_MCHP_IRQ_ENABLE
 * @brief Connect and enable an IRQ for a given device instance.
 *
 * This macro connects the specified IRQ for a device instance to the
 * `udc_mchp_isr_handler` interrupt service routine and enables the IRQ.
 *
 * @param i IRQ index for the device instance.
 * @param n Device instance number.
 */
#define UDC_MCHP_IRQ_ENABLE(i, n)                                                          \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq), DT_INST_IRQ_BY_IDX(n, i, priority),         \
		    udc_mchp_isr_handler, DEVICE_DT_INST_GET(n), 0);                               \
	irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));

/**
 * @def UDC_MCHP_IRQ_DISABLE
 * @brief Disable an IRQ for a given device instance.
 *
 * This macro disables the specified IRQ for a device instance.
 *
 * @param i IRQ index for the device instance.
 * @param n Device instance number.
 */
#define UDC_MCHP_IRQ_DISABLE(i, n) irq_disable(DT_INST_IRQ_BY_IDX(n, i, irq));

/**
 * @def UDC_MCHP_IRQ_ENABLE_DEFINE
 * @brief Define the IRQ enable function for a device instance.
 *
 * This macro generates a function that enables all IRQs associated with
 * a given device instance by calling the @ref UDC_MCHP_IRQ_ENABLE macro
 * for each IRQ defined in the device tree.
 *
 * @param i IRQ index (used by LISTIFY, typically ignored here).
 * @param n Device instance number.
 */
/* clang-format off */
#define UDC_MCHP_IRQ_ENABLE_DEFINE(i, n)                                \
	static void udc_mchp_irq_enable_func_##n(const struct device *dev)  \
	{                                                                   \
		LISTIFY(                                                        \
			DT_INST_NUM_IRQS(n),                                        \
			UDC_MCHP_IRQ_ENABLE,                                        \
			(),                                                         \
			n                                                           \
		)                                                               \
	}
/* clang-format on */

/**
 * @def UDC_MCHP_IRQ_DISABLE_DEFINE
 * @brief Define the IRQ disable function for a device instance.
 *
 * This macro generates a function that disables all IRQs associated with
 * a given device instance by calling the @ref UDC_MCHP_IRQ_DISABLE macro
 * for each IRQ defined in the device tree.
 *
 * @param i IRQ index (used by LISTIFY, typically ignored here).
 * @param n Device instance number.
 */
/* clang-format off */
#define UDC_MCHP_IRQ_DISABLE_DEFINE(i, n)                               \
	static void udc_mchp_irq_disable_func_##n(const struct device *dev) \
	{                                                                   \
		LISTIFY(                                                        \
			DT_INST_NUM_IRQS(n),                                        \
			UDC_MCHP_IRQ_DISABLE,                                       \
			(),                                                         \
			n                                                           \
		)                                                               \
	}
/* clang-format on */

/**
 * @def UDC_MCHP_PINCTRL_DT_INST_DEFINE
 * @brief Define pin control configuration for a device instance if available.
 *
 * This macro conditionally defines the pin control configuration for a device instance
 * if the device tree node has a "default" pinctrl state. If not, it expands to nothing.
 *
 * @param n Device instance number.
 */
/* clang-format off */
#define UDC_MCHP_PINCTRL_DT_INST_DEFINE(n)                              \
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),                   \
			(PINCTRL_DT_INST_DEFINE(n)), ())
/* clang-format on */

/**
 * @def UDC_MCHP_PINCTRL_DT_INST_DEV_CONFIG_GET
 * @brief Get the pin control device configuration for a device instance if available.
 *
 * This macro conditionally retrieves the pin control device configuration pointer
 * for a device instance if the device tree node has a "default" pinctrl state.
 * If not, it returns NULL.
 *
 * @param n Device instance number.
 * @return Pointer to the pin control device configuration, or NULL if not available.
 */
/* clang-format off */
#define UDC_MCHP_PINCTRL_DT_INST_DEV_CONFIG_GET(n)                      \
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),                   \
				((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))
/* clang-format on */

/**
 * @def UDC_MCHP_THREAD_DEFINE(n)
 * @brief Defines and creates a UDC MCHP thread and its stack.
 *
 * This macro defines a thread stack and a thread creation function for a
 * USB Device Controller (UDC) Microchip (MCHP) instance. It creates a stack
 * of size CONFIG_UDC_MCHP_G1_STACK_SIZE and a function to initialize
 * and start the thread for the specified instance number @p n.
 *
 * The generated function, @c udc_mchp_make_thread_##n, creates a thread
 * using the Zephyr kernel API, assigns the thread to the device's private
 * data structure, and sets the thread's name to the device's name.
 *
 * @param n  Instance number to uniquely identify the thread and stack.
 *
 * @note This macro should be used for each UDC MCHP instance that requires
 *       a dedicated thread.
 *
 * Example usage:
 * @code
 * UDC_MCHP_THREAD_DEFINE(0);
 * @endcode
 */
/* clang-format off */
#define UDC_MCHP_THREAD_DEFINE(n)                                                   \
	K_THREAD_STACK_DEFINE(udc_mchp_stack_##n, CONFIG_UDC_MCHP_G1_STACK_SIZE);       \
                                                                                    \
	static void udc_mchp_make_thread_##n(const struct device *dev)                  \
	{                                                                               \
		struct udc_mchp_data *priv = udc_get_private(dev);                          \
                                                                                    \
		k_thread_create(&priv->thread_data, udc_mchp_stack_##n,                     \
				K_THREAD_STACK_SIZEOF(udc_mchp_stack_##n), udc_thread,              \
				(void *)dev, NULL, NULL,                                            \
				K_PRIO_COOP(CONFIG_UDC_MCHP_G1_THREAD_PRIORITY), K_ESSENTIAL,       \
				K_NO_WAIT);                                                         \
		k_thread_name_set(&priv->thread_data, dev->name);                           \
	}
/* clang-format on */

/**
 * @def UDC_MCHP_CONFIG_DEFINE(n)
 * @brief Defines static configuration structures and buffers for a UDC MCHP instance.
 *
 * This macro generates all necessary static data structures and configuration
 * objects for a USB Device Controller (UDC) Microchip (MCHP) instance specified
 * by the instance number @p n. It defines endpoint buffer descriptors, endpoint
 * configuration arrays for IN and OUT directions, and a constant configuration
 * structure used to initialize the UDC MCHP driver.
 *
 * The generated configuration structure, @c udc_mchp_config_##n, includes
 * hardware register base address, buffer descriptor table, endpoint configuration
 * arrays, IRQ enable/disable function pointers, pin control configuration, and
 * a thread creation function pointer.
 *
 * @param n  Instance number to uniquely identify the configuration and buffers.
 *
 * @note This macro should be invoked for each UDC MCHP instance to ensure
 *       proper static allocation and configuration.
 *
 * Example usage:
 * @code
 * UDC_MCHP_CONFIG_DEFINE(0);
 * @endcode
 */
/* clang-format off */
#define UDC_MCHP_CONFIG_DEFINE(n)                                                   \
	static __aligned(sizeof(void *)) struct mchp_ep_buffer_desc                     \
		mchp_bdt_##n[DT_INST_PROP(n, num_bidir_endpoints)];                         \
                                                                                    \
	static struct udc_ep_config ep_cfg_out[DT_INST_PROP(n, num_bidir_endpoints)];   \
	static struct udc_ep_config ep_cfg_in[DT_INST_PROP(n, num_bidir_endpoints)];    \
                                                                                    \
	static const struct udc_mchp_config udc_mchp_config_##n = {                     \
		.base = (usb_device_registers_t *)DT_INST_REG_ADDR(n),                      \
		.bdt = mchp_bdt_##n,                                                        \
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),                         \
		.ep_cfg_in = ep_cfg_in,                                                     \
		.ep_cfg_out = ep_cfg_out,                                                   \
		.irq_enable_func = udc_mchp_irq_enable_func_##n,                            \
		.irq_disable_func = udc_mchp_irq_disable_func_##n,                          \
		.pcfg = UDC_MCHP_PINCTRL_DT_INST_DEV_CONFIG_GET(n),                         \
		.make_thread = udc_mchp_make_thread_##n,                                    \
	}
/* clang-format on */

/**
 * @def UDC_MCHP_DATA_DEFINE(n)
 * @brief Defines static data structures for a UDC MCHP instance.
 *
 * This macro allocates and initializes the static data structures required
 * for a USB Device Controller (UDC) Microchip (MCHP) instance specified by
 * the instance number @p n. It creates a private data structure for the
 * instance and a public data structure that includes a mutex for thread
 * safety and a pointer to the private data.
 *
 * The generated structures are:
 * - @c udc_priv_##n: Instance-specific private data.
 * - @c udc_data_##n: Public data structure containing a mutex and a pointer
 *   to the private data.
 *
 * @param n  Instance number to uniquely identify the data structures.
 *
 * @note This macro should be used for each UDC MCHP instance to ensure
 *       proper static allocation of driver data.
 *
 * Example usage:
 * @code
 * UDC_MCHP_DATA_DEFINE(0);
 * @endcode
 */
/* clang-format off */
#define UDC_MCHP_DATA_DEFINE(n)                                                     \
	static struct udc_mchp_data udc_priv_##n = {};                                  \
	static struct udc_data udc_data_##n = {                                         \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                           \
		.priv = &udc_priv_##n,                                                      \
	};
/* clang-format on */

/**
 * @def UDC_MCHP_DEVICE_DEFINE
 * @brief Instantiate all driver data, configuration, thread, and device structures for a device
 * instance.
 *
 * This macro generates all necessary data structures, configuration, thread stack,
 * and device registration code for a Microchip USB device controller instance.
 * It handles:
 * - Pin control and
 * - IRQ enable/disable function definitions
 * - Thread stack and thread creation for the driver
 * - Endpoint buffer descriptor allocation
 * - Endpoint configuration arrays for IN and OUT endpoints
 * - Driver configuration and runtime data structures
 * - Device registration with the Zephyr device model
 *
 * @param n Device instance number.
 */
 /* clang-format off */
#define UDC_MCHP_DEVICE_DEFINE(n)                                                   \
	UDC_MCHP_PINCTRL_DT_INST_DEFINE(n);                                             \
	UDC_MCHP_IRQ_ENABLE_DEFINE(i, n);                                               \
	UDC_MCHP_IRQ_DISABLE_DEFINE(i, n);                                              \
	UDC_MCHP_THREAD_DEFINE(n);                                                      \
	UDC_MCHP_DATA_DEFINE(n);                                                        \
	UDC_MCHP_CONFIG_DEFINE(n);                                                      \
                                                                                    \
	DEVICE_DT_INST_DEFINE(n, udc_mchp_driver_preinit, NULL, &udc_data_##n,          \
			      &udc_mchp_config_##n, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_mchp_api);
/* clang-format on */

/**
 * @brief Instantiate the Microchip USB device controller driver for each enabled device
 * instance.
 *
 * This macro expands to a call to @ref UDC_MCHP_DEVICE_DEFINE for every device instance
 * in the device tree that has status "okay".
 */
DT_INST_FOREACH_STATUS_OKAY(UDC_MCHP_DEVICE_DEFINE)
