/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UWB_TML_H_
#define ZEPHYR_INCLUDE_DRIVERS_UWB_TML_H_

#include "zephyr/uwb/status.h"

/**
 * @brief UWB subsystem Status codes
 * @defgroup uwb_status Ultra-Wideband subsystem status codes
 * @{
 */

/**
 * UWB Transport Management Layer software component
 */
#define UWB_COMPONENT_TML (0x02)

/**
 * UWB TML status codes
 */
enum uwb_tml_status_code {
	/** Transport Management Layer initialization failed */
	kUwb_StatusCode_TmlInitFailed = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_TML, 0x00),
	/** Failure from underlying transport layer */
	kUwb_StatusCode_TransportFailed = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_TML, 0x01),
	/** TML is busy performing other operation - try again */
	kUwb_StatusCode_TmlBusy = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_TML, 0x02),
};

/**
 * @}
 */

/**
 * @brief UWB subsystem transport layer
 * @defgroup uwb_transport Ultra-Wideband subsystem transport APIs
 * @{
 */

/**
 *  \brief Transport layer initialization API
 *
 *  This is an implementation specific API
 *  This API should initialize lower layer UWB driver to enable communication with the UWB device
 *
 *  \retval 0, if successful
 *  \retval non-zero, otherwise
 */
extern int uwb_transport_open(void);
/**
 *  \brief Transport layer de-initialization API
 *
 *  This is an implementation specific API
 *  This API should de-initialize lower layer UWB driver and disable communication with the UWB
 * device
 */
extern void uwb_transport_close(void);
/**
 *  \brief Transport layer initialization API
 *
 *  This is an implementation specific API
 *  This API should read \p bytes_to_read number of bytes from UWB device and populate \p pBuffer
 *
 *  \param[out] pBuffer Output buffer to be populated with read data
 *  \param[in] bytes_to_read Maximum bytes to read from UWB device
 *
 *  \retval >0, number of bytes read
 *  \retval 0 or negative, otherwise
 */
extern int uwb_transport_uci_read(uint8_t *pBuffer, int bytes_to_read);
/**
 *  \brief Transport layer initialization API
 *
 *  This is an implementation specific API
 *  This API should write \p bytes_to_write number of bytes from \p pBuffer to UWB device
 *
 *  \param[out] pBuffer Input buffer to be written to UWB device
 *  \param[in] bytes_to_write Number of bytes to write to UWB device
 *
 *  \retval >0, number of bytes written
 *  \retval 0 or negative, otherwise
 */
extern int uwb_transport_uci_write(uint8_t *pBuffer, uint16_t bytes_to_write);

/**
 * @}
 */

/**
 * @brief UWB subsystem Transport Management Layer APIs
 * @defgroup uwb_tml Ultra-Wideband subsystem TML APIs
 * @{
 */

/**
 * \brief Initialize Transport Management Layer
 *
 * Initializes the TML and underlying transport layer.
 * This function will create a reader thread which waits for any available
 * packet from UWB device.
 * \ref uwb_transport_open will be called from this API. uwb_transport_open
 * must ensure that underlying transport layer is correctly initialized to enable reading
 * from a thread.
 *
 * \retval kUwb_StatusCode_Success Successfully initialized TML and transport layer
 * \retval kUwb_StatusCode_TmlInitFailed Generic failure in TML layer
 * \retval kUwb_StatusCode_TransportFailed Transport layer returned a failure
 */
uwb_status_code_t uwb_tml_init(void);

/**
 * \brief De-initialize Transport Management Layer
 *
 * De-initializes the TML and underlying transport layer.
 * All allocated resources are cleared and the reader thread is deleted
 * \ref uwb_transport_close will be called from this API. uwb_transport_close
 * must ensure that underlying transport layer is correctly de-initialized
 */
void uwb_tml_deinit(void);

/**
 * \brief Write UCI packet to UWB device
 *
 * This API will call \ref uwb_transport_uci_write.
 * uwb_transport_uci_write must ensure that the packet is written to UWB device
 *
 * \retval kUwb_StatusCode_Success Successfully initialized TML and transport layer
 * \retval kUwb_StatusCode_TransportFailed Transport layer returned a failure
 */
uwb_status_code_t uwb_tml_write(uint8_t *p_data, uint16_t data_len);

/**
 * \brief Read UCI packet from UWB device
 *
 * This API will call start the reader thread which will poll uwb_transport_uci_read
 * for a successful read.
 *
 * \retval kUwb_StatusCode_Success Successfully initialized TML and transport layer
 */
uwb_status_code_t uwb_tml_read(void);

/**
 * \brief Abort ongoing read of UCI packet
 *
 * This API will discard any data that has been read from UWB device
 * and disable the read operation for the reader thread.
 *
 * \note Reader thread is not suspended in this function. It will wait
 * for \ref uwb_tml_read to be called again to issue the next \ref uwb_transport_uci_read call
 */
void uwb_tml_read_abort(void);

/**
 * \brief Suspend reader thread
 *
 * This API will discard any data that has been read from UWB device
 * and suspend the reader thread.
 *
 * \note \ref uwb_tml_resume_reader must be called to resume the thread operation
 */
void uwb_tml_suspend_reader(void);

/**
 * \brief Resume reader thread
 *
 * This API will resume the reader thread. Reader thread must have
 * been previously suspended by a call to \ref uwb_tml_suspend_reader
 */
void uwb_tml_resume_reader(void);

/**
 * @}
 */

#endif /*  ZEPHYR_INCLUDE_DRIVERS_UWB_TML_H_  */
