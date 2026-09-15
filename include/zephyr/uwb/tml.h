/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Transport Management Layer
 *
 * Transport management layer API definitions
 * to handle read and write of UCI packets
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UWB_TML_H_
#define ZEPHYR_INCLUDE_DRIVERS_UWB_TML_H_

#include <zephyr/uwb/status.h>

/**
 * @addtogroup uwb_status Ultra-Wideband subsystem status codes
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
	UWB_TML_STATUS_CODE_TML_INIT_FAILED = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_TML, 0x00),
	/** Failure from underlying transport layer */
	UWB_TML_STATUS_CODE_TRANSPORT_FAILED = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_TML, 0x01),
	/** TML is busy performing other operation - try again */
	UWB_TML_STATUS_CODE_TML_BUSY = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_TML, 0x02),
};

/**
 * @}
 */

/**
 * @brief UWB subsystem Transport Management Layer APIs
 * @defgroup uwb_tml Ultra-Wideband subsystem TML APIs
 * @ingroup uwb
 * @{
 */

/**
 * \brief Initialize Transport Management Layer
 *
 * Initializes the TML and underlying transport layer.
 * This function will create a reader thread which waits for any available
 * packet from UWB device.
 * \ref uwb_uci_open will be called from this API. uwb_uci_open
 * must ensure that underlying transport layer is correctly initialized to enable reading
 * from a thread.
 *
 * \retval UWB_STATUS_CODE_SUCCESS Successfully initialized TML and transport layer
 * \retval UWB_TML_STATUS_CODE_TML_INIT_FAILED Generic failure in TML layer
 * \retval UWB_TML_STATUS_CODE_TRANSPORT_FAILED Transport layer returned a failure
 */
uwb_status_code_t uwb_tml_init(void);

/**
 * \brief De-initialize Transport Management Layer
 *
 * De-initializes the TML and underlying transport layer.
 * All allocated resources are cleared and the reader thread is deleted
 * \ref uwb_uci_close will be called from this API. uwb_uci_close
 * must ensure that underlying transport layer is correctly de-initialized
 */
void uwb_tml_deinit(void);

/**
 * \brief Write UCI packet to UWB device
 *
 * This API will call \ref uwb_uci_send.
 * uwb_uci_send must ensure that the packet is written to UWB device
 *
 * \retval UWB_STATUS_CODE_SUCCESS Successfully initialized TML and transport layer
 * \retval UWB_TML_STATUS_CODE_TRANSPORT_FAILED Transport layer returned a failure
 */
uwb_status_code_t uwb_tml_write(uint8_t *p_data, uint16_t data_len);

/**
 * \brief Read UCI packet from UWB device
 *
 * This API will call start the reader thread which will poll uwb_uci_recv
 * for a successful read.
 *
 * \retval UWB_STATUS_CODE_SUCCESS Successfully initialized TML and transport layer
 */
uwb_status_code_t uwb_tml_read(void);

/**
 * \brief Abort ongoing read of UCI packet
 *
 * This API will discard any data that has been read from UWB device
 * and disable the read operation for the reader thread.
 *
 * \note Reader thread is not suspended in this function. It will wait
 * for \ref uwb_tml_read to be called again to issue the next \ref uwb_uci_recv call
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
