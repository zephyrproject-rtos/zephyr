/* SPDX-License-Identifier: BSD-3-Clause */

/*
 *  LPCUSB, an USB device driver for LPC microcontrollers
 *  Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)
 *  Copyright (c) 2016 Intel Corporation
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief USB device core layer APIs and structures
 *
 * This file contains the USB device core layer APIs and structures.
 */

#ifndef ZEPHYR_INCLUDE_USB_USB_DEVICE_H_
#define ZEPHYR_INCLUDE_USB_USB_DEVICE_H_

#include <drivers/usb/usb_dc.h>
#include <usb/usbstruct.h>
#include <logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * These macros should be used to place the USB descriptors
 * in predetermined order in the RAM.
 */
#define USBD_DEVICE_DESCR_DEFINE(p) \
	static __in_section(usb, descriptor_##p, 0) __used __aligned(1)
#define USBD_CLASS_DESCR_DEFINE(p, instance) \
	static __in_section(usb, descriptor_##p.1, instance) __used __aligned(1)
#define USBD_MISC_DESCR_DEFINE(p) \
	static __in_section(usb, descriptor_##p, 2) __used __aligned(1)
#define USBD_USER_DESCR_DEFINE(p) \
	static __in_section(usb, descriptor_##p, 3) __used __aligned(1)
#define USBD_STRING_DESCR_DEFINE(p) \
	static __in_section(usb, descriptor_##p, 4) __used __aligned(1)
#define USBD_TERM_DESCR_DEFINE(p) \
	static __in_section(usb, descriptor_##p, 5) __used __aligned(1)

/*
 * This macro should be used to place the struct usb_cfg_data
 * inside usb data section in the RAM.
 */
#define USBD_CFG_DATA_DEFINE(p, name) \
	static __in_section(usb, data_##p, name) __used

/*************************************************************************
 *  USB configuration
 **************************************************************************/

#define USB_MAX_CTRL_MPS	64   /**< maximum packet size (MPS) for EP 0 */
#define USB_MAX_FS_BULK_MPS	64   /**< full speed MPS for bulk EP */
#define USB_MAX_FS_INT_MPS	64   /**< full speed MPS for interrupt EP */
#define USB_MAX_FS_ISO_MPS	1023 /**< full speed MPS for isochronous EP */

/*************************************************************************
 *  USB application interface
 **************************************************************************/

/** setup packet definitions */
struct usb_setup_packet {
	uint8_t bmRequestType;  /**< characteristics of the specific request */
	uint8_t bRequest;       /**< specific request */
	uint16_t wValue;        /**< request specific parameter */
	uint16_t wIndex;        /**< request specific parameter */
	uint16_t wLength;       /**< length of data transferred in data phase */
};

/**
 * @brief USB Device Core Layer API
 * @defgroup _usb_device_core_api USB Device Core API
 * @{
 */

/**
 * @brief Callback function signature for the USB Endpoint status
 */
typedef void (*usb_ep_callback)(uint8_t ep,
				enum usb_dc_ep_cb_status_code cb_status);

/**
 * @brief Callback function signature for class specific requests
 *
 * Function which handles Class specific requests corresponding to an
 * interface number specified in the device descriptor table. For host
 * to device direction the 'len' and 'payload_data' contain the length
 * of the received data and the pointer to the received data respectively.
 * For device to host class requests, 'len' and 'payload_data' should be
 * set by the callback function with the length and the address of the
 * data to be transmitted buffer respectively.
 */
typedef int (*usb_request_handler)(struct usb_setup_packet *setup,
				   int32_t *transfer_len, uint8_t **payload_data);

/**
 * @brief Function for interface runtime configuration
 */
typedef void (*usb_interface_config)(struct usb_desc_header *head,
				     uint8_t bInterfaceNumber);

/**
 * @brief USB Endpoint Configuration
 *
 * This structure contains configuration for the endpoint.
 */
struct usb_ep_cfg_data {
	/**
	 * Callback function for notification of data received and
	 * available to application or transmit done, NULL if callback
	 * not required by application code
	 */
	usb_ep_callback ep_cb;
	/**
	 * The number associated with the EP in the device configuration
	 * structure
	 *   IN  EP = 0x80 | \<endpoint number\>
	 *   OUT EP = 0x00 | \<endpoint number\>
	 */
	uint8_t ep_addr;
};

/**
 * @brief USB Interface Configuration
 *
 * This structure contains USB interface configuration.
 */
struct usb_interface_cfg_data {
	/** Handler for USB Class specific Control (EP 0) communications */
	usb_request_handler class_handler;
	/** Handler for USB Vendor specific commands */
	usb_request_handler vendor_handler;
	/**
	 * The custom request handler gets a first chance at handling
	 * the request before it is handed over to the 'chapter 9' request
	 * handler.
	 * return 0 on success, -EINVAL if the request has not been handled by
	 *	  the custom handler and instead needs to be handled by the
	 *	  core USB stack. Any other error code to denote failure within
	 *	  the custom handler.
	 */
	usb_request_handler custom_handler;
};

/**
 * @brief USB device configuration
 *
 * The Application instantiates this with given parameters added
 * using the "usb_set_config" function. Once this function is called
 * changes to this structure will result in undefined behavior. This structure
 * may only be updated after calls to usb_deconfig
 */
struct usb_cfg_data {
	/**
	 * USB device description, see
	 * http://www.beyondlogic.org/usbnutshell/usb5.shtml#DeviceDescriptors
	 */
	const uint8_t *usb_device_description;
	/** Pointer to interface descriptor */
	const void *interface_descriptor;
	/** Function for interface runtime configuration */
	usb_interface_config interface_config;
	/** Callback to be notified on USB connection status change */
	void (*cb_usb_status)(struct usb_cfg_data *cfg,
			      enum usb_dc_status_code cb_status,
			      const uint8_t *param);
	/** USB interface (Class) handler and storage space */
	struct usb_interface_cfg_data interface;
	/** Number of individual endpoints in the device configuration */
	uint8_t num_endpoints;
	/**
	 * Pointer to an array of endpoint structs of length equal to the
	 * number of EP associated with the device description,
	 * not including control endpoints
	 */
	struct usb_ep_cfg_data *endpoint;
};

/**
 * @brief Configure USB controller
 *
 * Function to configure USB controller.
 * Configuration parameters must be valid or an error is returned
 *
 * @param[in] usb_descriptor USB descriptor table
 *
 * @return 0 on success, negative errno code on fail
 */
int usb_set_config(const uint8_t *usb_descriptor);

/**
 * @brief Deconfigure USB controller
 *
 * This function returns the USB device to it's initial state
 *
 * @return 0 on success, negative errno code on fail
 */
int usb_deconfig(void);

/**
 * @brief Enable the USB subsystem and associated hardware
 *
 * This function initializes the USB core subsystem and enables the
 * corresponding hardware so that it can begin transmitting and receiving
 * on the USB bus, as well as generating interrupts.
 *
 * Class-specific initialization and registration must be performed by the user
 * before invoking this, so that any data or events on the bus are processed
 * correctly by the associated class handling code.
 *
 * @param[in] status_cb Callback registered by user to notify
 *                      about USB device controller state.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_enable(usb_dc_status_callback status_cb);

/**
 * @brief Disable the USB device
 *
 * Function to disable the USB device.
 * Upon success, the specified USB interface is clock gated in hardware,
 * it is no longer capable of generating interrupts.
 *
 * @return 0 on success, negative errno code on fail
 */
int usb_disable(void);

/**
 * @brief Write data to the specified endpoint
 *
 * Function to write data to the specified endpoint. The supplied
 * usb_ep_callback will be called when transmission is done.
 *
 * @param[in]  ep        Endpoint address corresponding to the one listed in the
 *                       device configuration table
 * @param[in]  data      Pointer to data to write
 * @param[in]  data_len  Length of data requested to write. This may be zero for
 *                       a zero length status packet.
 * @param[out] bytes_ret Bytes written to the EP FIFO. This value may be NULL if
 *                       the application expects all bytes to be written
 *
 * @return 0 on success, negative errno code on fail
 */
int usb_write(uint8_t ep, const uint8_t *data, uint32_t data_len, uint32_t *bytes_ret);

/**
 * @brief Read data from the specified endpoint
 *
 * This function is called by the Endpoint handler function, after an
 * OUT interrupt has been received for that EP. The application must
 * only call this function through the supplied usb_ep_callback function.
 *
 * @param[in]  ep           Endpoint address corresponding to the one listed in
 *                          the device configuration table
 * @param[in]  data         Pointer to data buffer to write to
 * @param[in]  max_data_len Max length of data to read
 * @param[out] ret_bytes    Number of bytes read. If data is NULL and
 *                          max_data_len is 0 the number of bytes available
 *                          for read is returned.
 *
 * @return  0 on success, negative errno code on fail
 */
int usb_read(uint8_t ep, uint8_t *data, uint32_t max_data_len, uint32_t *ret_bytes);

/**
 * @brief Set STALL condition on the specified endpoint
 *
 * This function is called by USB device class handler code to set stall
 * condition on endpoint.
 *
 * @param[in]  ep           Endpoint address corresponding to the one listed in
 *                          the device configuration table
 *
 * @return  0 on success, negative errno code on fail
 */
int usb_ep_set_stall(uint8_t ep);

/**
 * @brief Clears STALL condition on the specified endpoint
 *
 * This function is called by USB device class handler code to clear stall
 * condition on endpoint.
 *
 * @param[in]  ep           Endpoint address corresponding to the one listed in
 *                          the device configuration table
 *
 * @return  0 on success, negative errno code on fail
 */
int usb_ep_clear_stall(uint8_t ep);

/**
 * @brief Read data from the specified endpoint
 *
 * This is similar to usb_ep_read, the difference being that, it doesn't
 * clear the endpoint NAKs so that the consumer is not bogged down by further
 * upcalls till he is done with the processing of the data. The caller should
 * reactivate ep by invoking usb_ep_read_continue() do so.
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 * @param[in]  data         pointer to data buffer to write to
 * @param[in]  max_data_len max length of data to read
 * @param[out] read_bytes   Number of bytes read. If data is NULL and
 *                          max_data_len is 0 the number of bytes
 *                          available for read should be returned.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
		     uint32_t *read_bytes);


/**
 * @brief Continue reading data from the endpoint
 *
 * Clear the endpoint NAK and enable the endpoint to accept more data
 * from the host. Usually called after usb_ep_read_wait() when the consumer
 * is fine to accept more data. Thus these calls together acts as flow control
 * mechanism.
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_ep_read_continue(uint8_t ep);

/**
 * Callback function signature for transfer completion.
 */
typedef void (*usb_transfer_callback)(uint8_t ep, int tsize, void *priv);

/* USB transfer flags */
#define USB_TRANS_READ       BIT(0)   /** Read transfer flag */
#define USB_TRANS_WRITE      BIT(1)   /** Write transfer flag */
#define USB_TRANS_NO_ZLP     BIT(2)   /** No zero-length packet flag */

/**
 * @brief Transfer management endpoint callback
 *
 * If a USB class driver wants to use high-level transfer functions, driver
 * needs to register this callback as usb endpoint callback.
 */
void usb_transfer_ep_callback(uint8_t ep, enum usb_dc_ep_cb_status_code);

/**
 * @brief Start a transfer
 *
 * Start a usb transfer to/from the data buffer. This function is asynchronous
 * and can be executed in IRQ context. The provided callback will be called
 * on transfer completion (or error) in thread context.
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 * @param[in]  data         Pointer to data buffer to write-to/read-from
 * @param[in]  dlen         Size of data buffer
 * @param[in]  flags        Transfer flags (USB_TRANS_READ, USB_TRANS_WRITE...)
 * @param[in]  cb           Function called on transfer completion/failure
 * @param[in]  priv         Data passed back to the transfer completion callback
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_transfer(uint8_t ep, uint8_t *data, size_t dlen, unsigned int flags,
		 usb_transfer_callback cb, void *priv);

/**
 * @brief Start a transfer and block-wait for completion
 *
 * Synchronous version of usb_transfer, wait for transfer completion before
 * returning.
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 * @param[in]  data         Pointer to data buffer to write-to/read-from
 * @param[in]  dlen         Size of data buffer
 * @param[in]  flags        Transfer flags
 *
 * @return number of bytes transferred on success, negative errno code on fail.
 */
int usb_transfer_sync(uint8_t ep, uint8_t *data, size_t dlen, unsigned int flags);

/**
 * @brief Cancel any ongoing transfer on the specified endpoint
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
void usb_cancel_transfer(uint8_t ep);

/**
 * @brief Cancel all ongoing transfers
 */
void usb_cancel_transfers(void);

/**
 * @brief Check that transfer is ongoing for the endpoint
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 *
 * @return true if transfer is ongoing, false otherwise.
 */
bool usb_transfer_is_busy(uint8_t ep);

/**
 * @brief Start the USB remote wakeup procedure
 *
 * Function to request a remote wakeup.
 * This feature must be enabled in configuration, otherwise
 * it will always return -ENOTSUP error.
 *
 * @return 0 on success, negative errno code on fail,
 *         i.e. when the bus is already active.
 */
int usb_wakeup_request(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_USB_DEVICE_H_ */
