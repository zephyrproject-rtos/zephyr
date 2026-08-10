/*
 * Copyright 2021-2022, 2026 NXP.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __UWB_UWBS_TML_INTERFACE_H__
#define __UWB_UWBS_TML_INTERFACE_H__

#include <uwb_uwbs_tml_io.h>
#include <uwb_bus_interface.h>
#include "uwb_tml_status.h"
#include <zephyr/kernel.h>
#include <stdbool.h>

/** @defgroup uwb_uwbs UWBS Specific Transport Interface
 *
 * This layer depends on ``uwb_bus_board.h`` and
 * ``uwb_bus_interface.h``
 *
 * The implementation of all these APIs would be UWBD and Transport specific.
 * e.g. If PnP mode of transport is used, RTCSync pins of SR1XX need be bothered
 * about.
 *
 *
 * These APIs are HANDSHAKE and Protocol Aware with the UWBS.
 *
 * - It menas waiting for any PIN before any operation
 * - It means reading "header" first, then then reading the rest of the frame later.
 *
 * @{
 */

/** @defgroup uwb_uwbs_tml_ctx UWB BUS Context Management
 *
 * @{
 */

/** Mode of operation of the TML Layer
 *
 * This functional mode selection simplifies handling of
 * low level transport protocol.
 */

typedef enum {
	/** Defaut Mode */
	kUWB_UWBS_TML_MODE_UCI,
	/** FW Download mode for SR040 */
	kUWB_UWBS_TML_MODE_SWUP,
	/** FW Donwnload mode for SR1XXT */
	kUWB_UWBS_TML_MODE_HBCI,
	/** FW Download mode/protocol for SR2XXT */
	kUWB_UWBS_TML_MODE_HDLL,
} uwb_uwbs_tml_mode_t;

/**
 * @brief Context for the Transport Layer
 *
 * This allows management of data / layer information.
 *
 */
typedef struct {
	/** Initialization flag for TML layer */
	bool isInitialized;
	/** Lower lever bus specific context SPI/PNP/SOCKET
	 *
	 * This structure is defined by Implementation layer,
	 * and not by the common UWB Library
	 */
	uwb_bus_board_ctx_t busCtx;
	/** tml interface read write mode */
	uwb_uwbs_tml_mode_t mode;
	/** This mutex is used to make tml interface read and write operations mutually exclusive */
	struct k_mutex mSyncMutex;
#if (UWBIOT_UWBD_SR04X)
	struct k_sem mReadyIrqWaitSem;
#endif /* (UWBIOT_UWBD_SR04X) */
#if UWBIOT_TML_PNP || UWBIOT_TML_SOCKET
	uint8_t boardVersion;
#endif
	int noOfBytesWritten;
} uwb_uwbs_tml_ctx_t;

/** @} */

/** @defgroup uwb_uwbs_tml_data UWB BUS DATA Interface
 *
 * @{
 */

/** Initailize it with some sane values
 *
 * @param      pCtx     The context
 */
UWBStatus_t uwb_uwbs_tml_init(uwb_uwbs_tml_ctx_t *pCtx);

/** De-Iniailize the context and free up references
 *
 * @param      pCtx  The context
 */
UWBStatus_t uwb_uwbs_tml_deinit(uwb_uwbs_tml_ctx_t *pCtx);

/** Initailize it with some sane values
 *
 * @param      pCtx  The context
 * @param      mode  See @ref uwb_uwbs_tml_mode_t
 */
UWBStatus_t uwb_uwbs_tml_setmode(uwb_uwbs_tml_ctx_t *pCtx, uwb_uwbs_tml_mode_t mode);

/** Transmit a data frame
 *
 * @param      pCtx    The context
 * @param[in]  pBuf    The data that we want to transmit
 * @param[in]  bufLen  The data length
 *
 * @return     Status of Transmit
 */
UWBStatus_t uwb_uwbs_tml_data_tx(uwb_uwbs_tml_ctx_t *pCtx, uint8_t *pBuf, size_t bufLen);

/** Receive a data frame
 *
 * @param      pCtx     The context
 * @param[out]      pBuf     The pointer where we copy received data
 * @param[in,out]  pBufLen  Input: The max length that we can copy.  Output: actual length read.
 *
 * @return     Status of Receive
 */
UWBStatus_t uwb_uwbs_tml_data_rx(uwb_uwbs_tml_ctx_t *pCtx, uint8_t *pBuf, size_t *pBufLen);

#if UWBIOT_UWBD_SR2XXT

#if UWBIOT_TML_PNP || UWBIOT_TML_SOCKET
/** Transmit a data frame which resets the device.
 *
 * @param          pCtx               The context
 * @param[in]      isFWDownloadDone   Set to True if FW download is completed & switch to uci mode
 *
 * @return     Status of Tx
 */
UWBStatus_t uwb_uwbs_tml_helios_hdll_reset(uwb_uwbs_tml_ctx_t *pCtx, bool isFWDownloadDone);

#endif // UWBIOT_TML_PNP || UWBIOT_TML_SOCKET

/** Transmit a data frame which read hdll and edl notification.
 *
 * @param          pCtx         The context
 * @param[out]     pRxBuf       The pointer where we copy received data
 * @param[in]      pRxBufLen    No of bytes to read
 *
 * @return     Status of Tx/Rx
 */
UWBStatus_t uwb_uwbs_tml_helios_get_hdll_edl_ntf(uwb_uwbs_tml_ctx_t *pCtx, uint8_t *pRxBuf,
						 size_t *pRxBufLen);
#endif // UWBIOT_UWBD_SR2XX

/** Reset uwbs tml interface
 *
 * @param      pCtx  The context
 *
 * @return     Status of reset
 */
UWBStatus_t uwb_uwbs_tml_reset(uwb_uwbs_tml_ctx_t *pCtx);

#if (UWBIOT_UWBD_SR04X)
/** Flush tml bus read buffer
 *
 * @param      pCtx  The context
 *
 */
void uwb_uwbs_tml_flush_read_buffer(uwb_uwbs_tml_ctx_t *pCtx);

#endif /* (UWBIOT_UWBD_SR04X) */

#if UWBIOT_TML_S32UART
/** Change uwbs protocol UCI/SWUP
 *
 * @param      pCtx  The context
 * @param      mode  The uwbs protocol to change to
 *
 */
void uwb_uwbs_tml_switch_protocol(uwb_uwbs_tml_ctx_t *pCtx, uwb_uwbs_tml_mode_t mode);

#define SWITCH_PROTOCOL_SWUP phTmlUwb_switch_protocol((uint8_t)kUWB_UWBS_TML_MODE_SWUP);
#define SWITCH_PROTOCOL_UCI  phTmlUwb_switch_protocol((uint8_t)kUWB_UWBS_TML_MODE_UCI);
#else
#define SWITCH_PROTOCOL_SWUP
#define SWITCH_PROTOCOL_UCI
#endif // UWBIOT_TML_S32UART

/** @} */ // uwb_uwbs_tml_data

#endif
