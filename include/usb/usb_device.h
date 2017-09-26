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

#ifndef USB_DEVICE_H_
#define USB_DEVICE_H_

#include <drivers/usb/usb_dc.h>
#include <usb/usbstruct.h>

/*************************************************************************
 *  USB configuration
 **************************************************************************/

#define MAX_PACKET_SIZE0    64        /**< maximum packet size for EP 0 */

/*************************************************************************
 *  USB application interface
 **************************************************************************/

/** setup packet definitions */
struct usb_setup_packet {
	u8_t bmRequestType;  /**< characteristics of the specific request */
	u8_t bRequest;       /**< specific request */
	u16_t wValue;        /**< request specific parameter */
	u16_t wIndex;        /**< request specific parameter */
	u16_t wLength;       /**< length of data transferred in data phase */
};

/**
 * Callback function signature for the device
 */
typedef void (*usb_status_callback)(enum usb_dc_status_code status_code,
				    u8_t *param);

/**
 * Callback function signature for the USB Endpoint status
 */
typedef void (*usb_ep_callback)(u8_t ep,
		enum usb_dc_ep_cb_status_code cb_status);

/**
 * Function which handles Class specific requests corresponding to an
 * interface number specified in the device descriptor table
 */
typedef int (*usb_request_handler) (struct usb_setup_packet *detup,
		s32_t *transfer_len, u8_t **payload_data);

/*
 * USB Endpoint Configuration
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
	u8_t ep_addr;
};

/**
 * USB Interface Configuration
 */
struct usb_interface_cfg_data {
	/** Handler for USB Class specific Control (EP 0) communications */
	usb_request_handler class_handler;
	/** Handler for USB Vendor specific commands */
	usb_request_handler vendor_handler;
	/**
	 * The custom request handler gets a first chance at handling
	 * the request before it is handed over to the 'chapter 9' request
	 * handler
	 */
	usb_request_handler custom_handler;
	/**
	 * This data area, allocated by the application, is used to store
	 * Class specific command data and must be large enough to store the
	 * largest payload associated with the largest supported Class'
	 * command set. This data area may be used for USB IN or OUT
	 * communications
	 */
	u8_t *payload_data;
	/**
	 * This data area, allocated by the application, is used to store
	 * Vendor specific payload
	 */
	u8_t *vendor_data;
};

/*
 * @brief USB device configuration
 *
 * The Application instantiates this with given parameters added
 * using the "usb_set_config" function. Once this function is called
 * changes to this structure will result in undefined behaviour. This structure
 * may only be updated after calls to usb_deconfig
 */
struct usb_cfg_data {
	/**
	 * USB device description, see
	 * http://www.beyondlogic.org/usbnutshell/usb5.shtml#DeviceDescriptors
	 */
	const u8_t *usb_device_description;
	/** Callback to be notified on USB connection status change */
	usb_status_callback cb_usb_status;
	/** USB interface (Class) handler and storage space */
	struct usb_interface_cfg_data interface;
	/** Number of individual endpoints in the device configuration */
	u8_t num_endpoints;
	/**
	 * Pointer to an array of endpoint structs of length equal to the
	 * number of EP associated with the device description,
	 * not including control endpoints
	 */
	struct usb_ep_cfg_data *endpoint;
};

/*
 * @brief configure USB controller
 *
 * Function to configure USB controller.
 * Configuration parameters must be valid or an error is returned
 *
 * @param[in] config Pointer to configuration structure
 *
 * @return 0 on success, negative errno code on fail
 */
int usb_set_config(struct usb_cfg_data *config);

/*
 * @brief return the USB device to it's initial state
 *
 * @return 0 on success, negative errno code on fail
 */
int usb_deconfig(void);

/*
 * @brief enable USB for host/device connection
 *
 * Function to enable USB for host/device connection.
 * Upon success, the USB module is no longer clock gated in hardware,
 * it is now capable of transmitting and receiving on the USB bus and
 * of generating interrupts.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_enable(struct usb_cfg_data *config);

/*
 * @brief disable the USB device.
 *
 * Function to disable the USB device.
 * Upon success, the specified USB interface is clock gated in hardware,
 * it is no longer capable of generating interrupts.
 *
 * @return 0 on success, negative errno code on fail
 */
int usb_disable(void);

/*
 * @brief write data to the specified endpoint
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
int usb_write(u8_t ep, const u8_t *data, u32_t data_len,
		u32_t *bytes_ret);

/*
 * @brief read data from the specified endpoint
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
int usb_read(u8_t ep, u8_t *data, u32_t max_data_len,
		u32_t *ret_bytes);

/*
 * @brief set STALL condition on the specified endpoint
 *
 * This function is called by USB device class handler code to set stall
 * conditionin on endpoint.
 *
 * @param[in]  ep           Endpoint address corresponding to the one listed in
 *                          the device configuration table
 *
 * @return  0 on success, negative errno code on fail
 */
int usb_ep_set_stall(u8_t ep);


/*
 * @brief clears STALL condition on the specified endpoint
 *
 * This function is called by USB device class handler code to clear stall
 * conditionin on endpoint.
 *
 * @param[in]  ep           Endpoint address corresponding to the one listed in
 *                          the device configuration table
 *
 * @return  0 on success, negative errno code on fail
 */
int usb_ep_clear_stall(u8_t ep);

/**
 * @brief read data from the specified endpoint
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
int usb_ep_read_wait(u8_t ep, u8_t *data, u32_t max_data_len,
		     u32_t *read_bytes);


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
int usb_ep_read_continue(u8_t ep);

#endif /* USB_DEVICE_H_ */
