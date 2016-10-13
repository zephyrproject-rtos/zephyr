/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_USB_H__
#define __QM_USB_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * USB peripheral driver for Quark Microcontrollers.
 *
 * @defgroup groupUSB USB
 * @{
 */

/**
 * USB Driver Status Codes.
 */
typedef enum {
	QM_USB_RESET,	/**< USB reset. */
	QM_USB_CONNECTED,    /**< USB connection ready and enumeration done. */
	QM_USB_CONFIGURED,   /**< USB configuration completed. */
	QM_USB_DISCONNECTED, /**< USB connection lost. */
	QM_USB_SUSPEND,      /**< USB connection suspended by the HOST. */
	QM_USB_RESUME,       /**< USB connection resumed by the HOST. */
} qm_usb_status_t;

/**
 * USB Endpoint Callback Status Codes.
 */
typedef enum {
	QM_USB_EP_SETUP,    /**< SETUP received. */
	QM_USB_EP_DATA_OUT, /**< Out transaction on this EP. */
	QM_USB_EP_DATA_IN   /**< In transaction on this EP. */
} qm_usb_ep_status_t;

/**
 * USB Endpoint type.
 */
typedef enum {
	QM_USB_EP_CONTROL = 0,  /**< Control endpoint. */
	QM_USB_EP_BULK = 2,     /**< Bulk type endpoint. */
	QM_USB_EP_INTERRUPT = 3 /**< Interrupt type endpoint. */
} qm_usb_ep_type_t;

/**
 * USB Endpoint Configuration.
 */
typedef struct {
	qm_usb_ep_type_t type;    /**< Endpoint type. */
	uint16_t max_packet_size; /**< Endpoint max packet size. */

	qm_usb_ep_idx_t ep;

	/**
	 * Callback for the USB Endpoint status.
	 *
	 * Called for notifying of data received and available to application
	 * on this endpoint.
	 *
	 * @param[in] data The callback user data.
	 * @param[in] error 0 on success.
	 *            Negative @ref errno for possible error codes.
	 * @param[in] ep Endpoint index.
	 * @param[in] status USB Endpoint status.
	 */
	void (*callback)(void *data, int error, qm_usb_ep_idx_t ep,
			 qm_usb_ep_status_t status);
	void *callback_data; /**< Callback user data. */
} qm_usb_ep_config_t;

/**
 * Callback function signature for the device status.
 *
 * @param[in] data The callback user data.
 * @param[in] error 0 on success.
 *            Negative @ref errno for possible error codes.
 * @param[in] status USB Controller Driver status.
 */
typedef void (*qm_usb_status_callback_t)(void *data, int error,
					 qm_usb_status_t status);

/**
 * Attach the USB device.
 *
 * Upon success, the USB PLL is enabled, and the USB device is now capable
 * of transmitting and receiving on the USB bus and of generating interrupts.
 *
 * @param[in] usb Which USB module to perform action.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_attach(const qm_usb_t usb);

/**
 * Detach the USB device.
 *
 * Upon success, the USB hardware PLL is powered down and USB communication
 * is disabled.
 *
 * @param[in] usb Which USB module to perform action.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_detach(const qm_usb_t usb);

/**
 * Reset the USB device controller back to it's initial state.
 *
 * Performs the Core Soft Reset from the USB controller.
 * This means that all internal state machines are reset to the IDLE state,
 * all FIFOs are flushed and all ongoing transactions are terminated.
 *
 * @note This function does NOT disable the USB clock PLL.
 *
 * @param[in] usb Which USB module to perform action.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_reset(const qm_usb_t usb);

/**
 * Set USB device address.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] addr USB Device Address.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_set_address(const qm_usb_t usb, const uint8_t addr);

/**
 * Set USB device controller status callback.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] cb USB Device Status callback.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_set_status_callback(const qm_usb_t usb,
			       const qm_usb_status_callback_t cb);

/**
 * Configure endpoint.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] cfg Endpoint configuration.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_set_config(const qm_usb_t usb,
			 const qm_usb_ep_config_t *const cfg);

/**
 * Set / Clear stall condition for the selected endpoint.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep Endpoint index corresponding to the one listed in the
 *               device configuration table.
 * @param[in] is_stalled Endpoint stall state to be set.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_set_stall_state(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
			      const bool is_stalled);

/**
 * Check stall condition for the selected endpoint.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep Endpoint index corresponding to the one listed in the
 *               device configuration table.
 * @param[out] stalled Endpoint stall state. Must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_is_stalled(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
			 bool *const stalled);

/**
 * Halt the selected endpoint.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep Endpoint index corresponding to the one listed in the
 *               device configuration table.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_halt(const qm_usb_t usb, const qm_usb_ep_idx_t ep);

/**
 * Enable the selected endpoint.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep Endpoint index corresponding to the one listed in the
 *               device configuration table.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_enable(const qm_usb_t usb, const qm_usb_ep_idx_t ep);

/**
 * Disable the selected endpoint.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep Endpoint index corresponding to the one listed in the
 *               device configuration table.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_disable(const qm_usb_t usb, const qm_usb_ep_idx_t ep);

/**
 * Flush the selected IN endpoint TX FIFO.
 *
 * RX FIFO is global and cannot be flushed per endpoint. Thus, this function
 * only applies to endpoints of direction IN.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep Endpoint index corresponding to the one listed in the
 *               device configuration table. Must be of IN direction.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_flush(const qm_usb_t usb, const qm_usb_ep_idx_t ep);

/**
 * Write data to the specified IN endpoint.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep    Endpoint index corresponding to the one listed in the
 *                  device configuration table. Must be of IN direction.
 * @param[in] data  Pointer to data to write.
 * @param[in] len   Length of data requested to write. This may be zero for
 *                  a zero length status packet.
 * @param[out] ret_bytes Bytes scheduled for transmission. This value may be
 *                       NULL if the application expects all bytes to be
 *                       written.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_write(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
		    const uint8_t *const data, const uint32_t len,
		    uint32_t *const ret_bytes);

/**
 * Read data from OUT endpoint.
 *
 * This function is called by the Endpoint handler function, after an OUT
 * interrupt has been received for that EP.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep    Endpoint index corresponding to the one listed in the
 *                  device configuration table. Must be of OUT direction.
 * @param[in] data  Pointer to data to read to. Must not be null.
 * @param[in] max_len   Length of data requested to be read.
 * @param[out] read_bytes Number of bytes read.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_read(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
		   uint8_t *const data, const uint32_t max_len,
		   uint32_t *const read_bytes);

/**
 * Check how many bytes are available on OUT endpoint.
 *
 * @param[in] usb Which USB module to perform action.
 * @param[in] ep    Endpoint index corresponding to the one listed in the
 *                  device configuration table. Must be of OUT direction.
 * @param[out] read_bytes Number of bytes read. Must not be null.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_usb_ep_get_bytes_read(const qm_usb_t usb, const qm_usb_ep_idx_t ep,
			     uint32_t *const read_bytes);

/**
 * @}
 */
#endif /* __QM_USB_H__ */
