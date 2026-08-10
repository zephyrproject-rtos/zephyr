/*
 * Copyright 2012-2023,2025,2026 NXP.
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

/* Basic type definitions */
#ifndef __UWB_TML_TRANSPORT_H__
#define __UWB_TML_TRANSPORT_H__

#include "uwb_uwbs_tml_io.h"
#include "uwb_uwbs_tml_interface.h"
#include "uwb_tml_status.h"

#define SR200_MAGIC          0xEA
#define SR200_SET_PWR        _IOW(SR200_MAGIC, 0x01, long)
#define SR200_SET_DBG        _IOW(SR200_MAGIC, 0x02, long)
#define SR200_GET_THROUGHPUT _IOW(SR200_MAGIC, 0x05, long)

#define PWR_DISABLE        0
#define PWR_ENABLE         1
#define ABORT_READ_PENDING 2

#define NORMAL_MODE_HEADER_LEN 4
#define NORMAL_MODE_LEN_OFFSET 3

#define EXTENDED_SIZE_LEN_OFFSET 1
#define UCI_EXTENDED_PKT_MASK    0xC0
#define UCI_EXTENDED_SIZE_SHIFT  6
#define UCI_NORMAL_PKT_SIZE      0
#define UCI_EXT_PKT_SIZE_512B    1
#define UCI_EXT_PKT_SIZE_1K      2
#define UCI_EXT_PKT_SIZE_2K      3

#define UCI_PKT_SIZE_512B 512
#define UCI_PKT_SIZE_1K   1024
#define UCI_PKT_SIZE_2K   2048

/* TML UWB Context */
extern uwb_uwbs_tml_ctx_t g_uwbs_tml_ctx;

/*Global Function declarations */

/**
**
** Function         uwb_transport_open
**
** Description      Open and configure helios device
**
**
** Returns          UWB status:
**                  UWBSTATUS_SUCCESS - open_and_configure operation success
**                  UWBSTATUS_INVALID_DEVICE - device open operation failure
**
*/
int uwb_transport_open();

/**
** Function         uwb_transport_io_set
**
** Description      set uwbs io
**
**
** Returns          UWB status:
**                  UWBSTATUS_SUCCESS - uwb_transport_io_init operation success
**                  UWBSTATUS_INVALID_DEVICE - device open operation failure
**
*/
UWBSTATUS uwb_transport_io_set(uwbs_io_t ioPin, uint8_t value);

// TODO: Temporary for sn110 and sr1xx simultaneous irq enblement
#if UWBIOT_SESN_SNXXX
/*******************************************************************************
**
** Function         uwb_tml_io_enable_uwb_irq
**
** Description      Enables the UWB interrupt for the UWBS transport layer
**
** Parameters       None
**
** Returns          None
**
*******************************************************************************/
void uwb_tml_io_enable_uwb_irq();
#endif

/**
** Function         uwb_transport_close
**
** Description      SPI Cleanup
**
**
** Returns          None
**
*/
void uwb_transport_close();

/**
** Function         uwb_transport_uci_read
**
** Description      Reads requested number of bytes from SR100 device into given
**                  buffer
**
** Parameters       pBuffer          - buffer for read data
**                  nNbBytesToRead   - number of bytes requested to be read
**
** Returns          numRead   - number of successfully read bytes
**                  -1        - read operation failure
**
*/
int uwb_transport_uci_read(uint8_t *pBuffer, int nNbBytesToRead);

/**
** Function         uwb_transport_uci_write
**
** Description      Writes requested number of bytes from given buffer into
**                  SR100 device
**
** Parameters       pBuffer          - buffer for read data
**                  nNbBytesToWrite  - number of bytes requested to be written
**
** Returns          numWrote   - number of successfully written bytes
**                  -1         - write operation failure
**
*/
int uwb_transport_uci_write(uint8_t *pBuffer, uint16_t nNbBytesToWrite);

/**
** Function         uwb_tml_rci_read
**
** Description      Reads requested number of bytes from SR040 device into given
**                  buffer using SWUP protocol
**
** Parameters       pBuffer          - buffer for read data
**
** Returns          nNbBytesToRead   - number of successfully read bytes
**                  -1        - read operation failure
**
*/
int uwb_tml_rci_read(uint8_t *pBuffer, int nNbBytesToRead);

/**
** Function         uwb_tml_rci_write
**
** Description      Writes requested number of bytes from given buffer into
**                  SR040 device in SWUP mode
**
** Parameters       pBuffer          - buffer to write
**                  nNbBytesToWrite  - number of to write
**
** Returns          numWrote   - number of successfully written bytes
**                  -1         - write operation failure
**
*/
int uwb_tml_rci_write(uint8_t *pBuffer, uint16_t nNbBytesToWrite);

#if UWBIOT_UWBD_SR2XXT
/**
** Function         uwb_tml_hdll_read
**
** Description      Reads requested number of bytes from SR200 device into given
**                  buffer using HDLL protocol
**
** Parameters       pBuffer          - buffer for read data
**
** Returns          numRead   - number of successfully read bytes
**                  -1        - read operation failure
**
*/
int uwb_tml_hdll_read(uint8_t *pBuffer, uint16_t *pRspBufLen);

/**
** Function         uwb_tml_hdll_transceive
**
** Description      HDLL Write read operation for SR2XXT devices for FW download
**
** Parameters       pWriteBuf        - buffer to write
**                  writeBufLen  - number of bytes to written
**                  pRespBuf     - response buffer
**                  pRspBufLen   - response bufferLen
**
** Returns          UWB status:
**                  UWBSTATUS_SUCCESS - uwb_tml_hdll_transceive operation success
**                  UWBSTATUS_FAILED - uwb_tml_hdll_transceive failure
**
*/
UWBSTATUS uwb_tml_hdll_transceive(uint8_t *pWriteBuf, size_t writeBufLen, uint8_t *pRespBuf,
				  size_t *pRspBufLen);

#if (UWBIOT_TML_PNP || UWBIOT_TML_SOCKET)
/**
** Function         uwb_tml_hdll_reset
**
** Description      Send HDLL Reset Command for SR2XXT devices. Resets device back to UCI mode
**
** Parameters       isFWDownloadDone  - true if FW download is in Done
**
** Returns          UWB status:
**                  UWBSTATUS_SUCCESS - uwb_tml_hdll_transceive operation success
**                  UWBSTATUS_FAILED - uwb_tml_hdll_transceive failure
**
*/
UWBSTATUS uwb_tml_hdll_reset(bool isFWDownloadDone);
#endif //(UWBIOT_TML_PNP || UWBIOT_TML_SOCKET)
#endif // UWBIOT_UWBD_SR2XXT

/**
** Function         uwb_tml_reset_uwbs
**
** Description      Reset UWBS device
**
** Returns          UWB status:
**                  UWBSTATUS_SUCCESS - uwb_tml_reset_uwbs operation success
**                  UWBSTATUS_FAILED - uwb_tml_reset_uwbs failure
**
*/
UWBSTATUS uwb_tml_reset_uwbs(void);

/**
** Function         uwb_tml_reset
**
** Description      Reset SR100 device, using VEN pin
**
** Parameters       level          - reset level
**
** Returns           0   - reset operation success
**                  -1   - reset operation failure
**
*/
int uwb_tml_reset(long level);
#if (UWBIOT_UWBD_SR04X)
#if UWBIOT_TML_SPI
/**
** Function         uwb_tml_flush_read_buffer
**
** Description      flush tml read buffer
**
**
**
*/
void uwb_tml_flush_read_buffer(void);

/**
** Function         uwb_tml_helios_irq_enable
**
** Description      enable uwbs irq
**
**
**
*/
void uwb_tml_helios_irq_enable(void);

/**
** Function         uwb_tml_rdy_read
**
** Description      get uwbs ready pin status
**
**
**
*/
bool uwb_tml_rdy_read(void);
#endif // UWBIOT_TML_SPI
#endif /* (UWBIOT_UWBD_SR04X) */

#if UWBIOT_TML_S32UART
/**
** Function         uwb_tml_switch_protocol
**
** Description      switch uwbs protocl
**
** Parameters       protocol       - protocol SWUP or UCI
**
*/
void uwb_tml_switch_protocol(uint8_t protocol);
#endif // UWBIOT_TML_S32UART

/**
** Function         uwb_tml_hbci_transceive
**
** Description      HBCI Write read operation for SR1xxT devices for FW download
**
** Parameters       pWriteBuf        - buffer to write
**                  writeBufLen  - number of bytes to written
**                  pRespBuf     - response buffer
**                  pRspBufLen   - response bufferLen
**
** Returns          UWB status:
**                  UWBSTATUS_SUCCESS - uwb_tml_hbci_transceive operation success
**                  UWBSTATUS_FAILED - uwb_tml_hbci_transceive failure
**
*/
UWBSTATUS uwb_tml_hbci_transceive(uint8_t *pWriteBuf, size_t writeBufLen, uint8_t *pRespBuf,
				  size_t *pRspBufLen);

/**
** Function         uwb_tml_set_mode_uci
**
** Description      Set TML transfer mode to UCI
**
*/
void uwb_tml_set_mode_uci(void);

/**
** Function         uwb_tml_set_mode_fwdld
**
** Description      Set appropriate FW download TML transfer mode.
**
** Returns          UWB status:
**                  UWBSTATUS_SUCCESS - uwb_tml_set_mode_fwdld operation success
**                  UWBSTATUS_FAILED - uwb_tml_set_mode_fwdld failure
*/
UWBSTATUS uwb_tml_set_mode_fwdld(void);

/* ============================================================================
 * Backward Compatibility Defines
 * ============================================================================
 */

#define phTmlUwb_open_and_configure uwb_transport_open
#define phTmlUwb_io_set             uwb_transport_io_set
#define phTmlUwb_io_enable_uwb_irq  uwb_tml_io_enable_uwb_irq
#define phTmlUwb_close              uwb_transport_close
#define phTmlUwb_uci_read           uwb_transport_uci_read
#define phTmlUwb_uci_write          uwb_transport_uci_write
#define phTmlUwb_rci_read           uwb_tml_rci_read
#define phTmlUwb_rci_write          uwb_tml_rci_write
#define phTmlUwb_hdll_read          uwb_tml_hdll_read
#define phTmlUwb_hdll_transceive    uwb_tml_hdll_transceive
#define phTmlUwb_hdll_reset         uwb_tml_hdll_reset
#define phTmlUwb_reset_uwbs         uwb_tml_reset_uwbs
#define phTmlUwb_reset              uwb_tml_reset
#define phTmlUwb_flush_read_buffer  uwb_tml_flush_read_buffer
#define phTmlUwb_helios_irq_enable  uwb_tml_helios_irq_enable
#define phTmlUwb_rdy_read           uwb_tml_rdy_read
#define phTmlUwb_switch_protocol    uwb_tml_switch_protocol
#define phTmlUwb_hbci_transceive    uwb_tml_hbci_transceive
#define phTmlUwb_set_mode_uci       uwb_tml_set_mode_uci
#define phTmlUwb_set_mode_fwdld     uwb_tml_set_mode_fwdld
#define gUwbsTmlCtx                 g_uwbs_tml_ctx
#define phTmlUwb_Init(...)          uwb_tml_init()
#define phTmlUwb_Shutdown           uwb_tml_deinit
#define phTmlUwb_Write              uwb_tml_write
#define phTmlUwb_ReadAbort          uwb_tml_read_abort
#define phTmlUwb_StartThread        uwb_tml_start_thread
#define phTmlUwb_TmlReaderThread    uwb_tml_reader_thread
#define phTmlUwb_suspendReader      uwb_tml_suspend_reader
#define phTmlUwb_resumeReader       uwb_tml_resume_reader
#define phTmlUwb_Chip_Reset         uwb_tml_chip_reset

#endif //__UWB_TML_TRANSPORT_H__
