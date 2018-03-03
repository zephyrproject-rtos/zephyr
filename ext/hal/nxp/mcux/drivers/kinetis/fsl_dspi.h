/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FSL_DSPI_H_
#define _FSL_DSPI_H_

#include "fsl_common.h"

/*!
 * @addtogroup dspi_driver
 * @{
 */

/**********************************************************************************************************************
 * Definitions
 *********************************************************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief DSPI driver version 2.2.0. */
#define FSL_DSPI_DRIVER_VERSION (MAKE_VERSION(2, 2, 0))
/*@}*/

#ifndef DSPI_DUMMY_DATA
/*! @brief DSPI dummy data if there is no Tx data.*/
#define DSPI_DUMMY_DATA (0x00U) /*!< Dummy data used for Tx if there is no txData. */
#endif

/*! @brief Status for the DSPI driver.*/
enum _dspi_status
{
    kStatus_DSPI_Busy = MAKE_STATUS(kStatusGroup_DSPI, 0),      /*!< DSPI transfer is busy.*/
    kStatus_DSPI_Error = MAKE_STATUS(kStatusGroup_DSPI, 1),     /*!< DSPI driver error. */
    kStatus_DSPI_Idle = MAKE_STATUS(kStatusGroup_DSPI, 2),      /*!< DSPI is idle.*/
    kStatus_DSPI_OutOfRange = MAKE_STATUS(kStatusGroup_DSPI, 3) /*!< DSPI transfer out of range. */
};

/*! @brief DSPI status flags in SPIx_SR register.*/
enum _dspi_flags
{
    kDSPI_TxCompleteFlag = SPI_SR_TCF_MASK,          /*!< Transfer Complete Flag. */
    kDSPI_EndOfQueueFlag = SPI_SR_EOQF_MASK,         /*!< End of Queue Flag.*/
    kDSPI_TxFifoUnderflowFlag = SPI_SR_TFUF_MASK,    /*!< Transmit FIFO Underflow Flag.*/
    kDSPI_TxFifoFillRequestFlag = SPI_SR_TFFF_MASK,  /*!< Transmit FIFO Fill Flag.*/
    kDSPI_RxFifoOverflowFlag = SPI_SR_RFOF_MASK,     /*!< Receive FIFO Overflow Flag.*/
    kDSPI_RxFifoDrainRequestFlag = SPI_SR_RFDF_MASK, /*!< Receive FIFO Drain Flag.*/
    kDSPI_TxAndRxStatusFlag = SPI_SR_TXRXS_MASK,     /*!< The module is in Stopped/Running state.*/
    kDSPI_AllStatusFlag = SPI_SR_TCF_MASK | SPI_SR_EOQF_MASK | SPI_SR_TFUF_MASK | SPI_SR_TFFF_MASK | SPI_SR_RFOF_MASK |
                          SPI_SR_RFDF_MASK | SPI_SR_TXRXS_MASK /*!< All statuses above.*/
};

/*! @brief DSPI interrupt source.*/
enum _dspi_interrupt_enable
{
    kDSPI_TxCompleteInterruptEnable = SPI_RSER_TCF_RE_MASK,          /*!< TCF  interrupt enable.*/
    kDSPI_EndOfQueueInterruptEnable = SPI_RSER_EOQF_RE_MASK,         /*!< EOQF interrupt enable.*/
    kDSPI_TxFifoUnderflowInterruptEnable = SPI_RSER_TFUF_RE_MASK,    /*!< TFUF interrupt enable.*/
    kDSPI_TxFifoFillRequestInterruptEnable = SPI_RSER_TFFF_RE_MASK,  /*!< TFFF interrupt enable, DMA disable.*/
    kDSPI_RxFifoOverflowInterruptEnable = SPI_RSER_RFOF_RE_MASK,     /*!< RFOF interrupt enable.*/
    kDSPI_RxFifoDrainRequestInterruptEnable = SPI_RSER_RFDF_RE_MASK, /*!< RFDF interrupt enable, DMA disable.*/
    kDSPI_AllInterruptEnable = SPI_RSER_TCF_RE_MASK | SPI_RSER_EOQF_RE_MASK | SPI_RSER_TFUF_RE_MASK |
                               SPI_RSER_TFFF_RE_MASK | SPI_RSER_RFOF_RE_MASK | SPI_RSER_RFDF_RE_MASK
    /*!< All above interrupts enable.*/
};

/*! @brief DSPI DMA source.*/
enum _dspi_dma_enable
{
    kDSPI_TxDmaEnable = (SPI_RSER_TFFF_RE_MASK | SPI_RSER_TFFF_DIRS_MASK), /*!< TFFF flag generates DMA requests.
                                                                                No Tx interrupt request. */
    kDSPI_RxDmaEnable = (SPI_RSER_RFDF_RE_MASK | SPI_RSER_RFDF_DIRS_MASK)  /*!< RFDF flag generates DMA requests.
                                                                                No Rx interrupt request. */
};

/*! @brief DSPI master or slave mode configuration.*/
typedef enum _dspi_master_slave_mode
{
    kDSPI_Master = 1U, /*!< DSPI peripheral operates in master mode.*/
    kDSPI_Slave = 0U   /*!< DSPI peripheral operates in slave mode.*/
} dspi_master_slave_mode_t;

/*!
 * @brief DSPI Sample Point: Controls when the DSPI master samples SIN in the Modified Transfer Format. This field is
 * valid
 * only when the CPHA bit in the CTAR register is 0.
 */
typedef enum _dspi_master_sample_point
{
    kDSPI_SckToSin0Clock = 0U, /*!< 0 system clocks between SCK edge and SIN sample.*/
    kDSPI_SckToSin1Clock = 1U, /*!< 1 system clock  between SCK edge and SIN sample.*/
    kDSPI_SckToSin2Clock = 2U  /*!< 2 system clocks between SCK edge and SIN sample.*/
} dspi_master_sample_point_t;

/*! @brief DSPI Peripheral Chip Select (Pcs) configuration (which Pcs to configure).*/
typedef enum _dspi_which_pcs_config
{
    kDSPI_Pcs0 = 1U << 0, /*!< Pcs[0] */
    kDSPI_Pcs1 = 1U << 1, /*!< Pcs[1] */
    kDSPI_Pcs2 = 1U << 2, /*!< Pcs[2] */
    kDSPI_Pcs3 = 1U << 3, /*!< Pcs[3] */
    kDSPI_Pcs4 = 1U << 4, /*!< Pcs[4] */
    kDSPI_Pcs5 = 1U << 5  /*!< Pcs[5] */
} dspi_which_pcs_t;

/*! @brief DSPI Peripheral Chip Select (Pcs) Polarity configuration.*/
typedef enum _dspi_pcs_polarity_config
{
    kDSPI_PcsActiveHigh = 0U, /*!< Pcs Active High (idles low). */
    kDSPI_PcsActiveLow = 1U   /*!< Pcs Active Low (idles high). */
} dspi_pcs_polarity_config_t;

/*! @brief DSPI Peripheral Chip Select (Pcs) Polarity.*/
enum _dspi_pcs_polarity
{
    kDSPI_Pcs0ActiveLow = 1U << 0, /*!< Pcs0 Active Low (idles high). */
    kDSPI_Pcs1ActiveLow = 1U << 1, /*!< Pcs1 Active Low (idles high). */
    kDSPI_Pcs2ActiveLow = 1U << 2, /*!< Pcs2 Active Low (idles high). */
    kDSPI_Pcs3ActiveLow = 1U << 3, /*!< Pcs3 Active Low (idles high). */
    kDSPI_Pcs4ActiveLow = 1U << 4, /*!< Pcs4 Active Low (idles high). */
    kDSPI_Pcs5ActiveLow = 1U << 5, /*!< Pcs5 Active Low (idles high). */
    kDSPI_PcsAllActiveLow = 0xFFU  /*!< Pcs0 to Pcs5 Active Low (idles high). */
};

/*! @brief DSPI clock polarity configuration for a given CTAR.*/
typedef enum _dspi_clock_polarity
{
    kDSPI_ClockPolarityActiveHigh = 0U, /*!< CPOL=0. Active-high DSPI clock (idles low).*/
    kDSPI_ClockPolarityActiveLow = 1U   /*!< CPOL=1. Active-low DSPI clock (idles high).*/
} dspi_clock_polarity_t;

/*! @brief DSPI clock phase configuration for a given CTAR.*/
typedef enum _dspi_clock_phase
{
    kDSPI_ClockPhaseFirstEdge = 0U, /*!< CPHA=0. Data is captured on the leading edge of the SCK and changed on the
                                         following edge.*/
    kDSPI_ClockPhaseSecondEdge = 1U /*!< CPHA=1. Data is changed on the leading edge of the SCK and captured on the
                                        following edge.*/
} dspi_clock_phase_t;

/*! @brief DSPI data shifter direction options for a given CTAR.*/
typedef enum _dspi_shift_direction
{
    kDSPI_MsbFirst = 0U, /*!< Data transfers start with most significant bit.*/
    kDSPI_LsbFirst = 1U  /*!< Data transfers start with least significant bit.
                              Shifting out of LSB is not supported for slave */
} dspi_shift_direction_t;

/*! @brief DSPI delay type selection.*/
typedef enum _dspi_delay_type
{
    kDSPI_PcsToSck = 1U,  /*!< Pcs-to-SCK delay. */
    kDSPI_LastSckToPcs,   /*!< The last SCK edge to Pcs delay. */
    kDSPI_BetweenTransfer /*!< Delay between transfers. */
} dspi_delay_type_t;

/*! @brief DSPI Clock and Transfer Attributes Register (CTAR) selection.*/
typedef enum _dspi_ctar_selection
{
    kDSPI_Ctar0 = 0U, /*!< CTAR0 selection option for master or slave mode; note that CTAR0 and CTAR0_SLAVE are the
                         same register address. */
    kDSPI_Ctar1 = 1U, /*!< CTAR1 selection option for master mode only. */
    kDSPI_Ctar2 = 2U, /*!< CTAR2 selection option for master mode only; note that some devices do not support CTAR2. */
    kDSPI_Ctar3 = 3U, /*!< CTAR3 selection option for master mode only; note that some devices do not support CTAR3. */
    kDSPI_Ctar4 = 4U, /*!< CTAR4 selection option for master mode only; note that some devices do not support CTAR4. */
    kDSPI_Ctar5 = 5U, /*!< CTAR5 selection option for master mode only; note that some devices do not support CTAR5. */
    kDSPI_Ctar6 = 6U, /*!< CTAR6 selection option for master mode only; note that some devices do not support CTAR6. */
    kDSPI_Ctar7 = 7U  /*!< CTAR7 selection option for master mode only; note that some devices do not support CTAR7. */
} dspi_ctar_selection_t;

#define DSPI_MASTER_CTAR_SHIFT (0U)   /*!< DSPI master CTAR shift macro; used internally. */
#define DSPI_MASTER_CTAR_MASK (0x0FU) /*!< DSPI master CTAR mask macro; used internally. */
#define DSPI_MASTER_PCS_SHIFT (4U)    /*!< DSPI master PCS shift macro; used internally. */
#define DSPI_MASTER_PCS_MASK (0xF0U)  /*!< DSPI master PCS mask macro; used internally. */
/*! @brief Use this enumeration for the DSPI master transfer configFlags. */
enum _dspi_transfer_config_flag_for_master
{
    kDSPI_MasterCtar0 = 0U << DSPI_MASTER_CTAR_SHIFT, /*!< DSPI master transfer use CTAR0 setting. */
    kDSPI_MasterCtar1 = 1U << DSPI_MASTER_CTAR_SHIFT, /*!< DSPI master transfer use CTAR1 setting. */
    kDSPI_MasterCtar2 = 2U << DSPI_MASTER_CTAR_SHIFT, /*!< DSPI master transfer use CTAR2 setting. */
    kDSPI_MasterCtar3 = 3U << DSPI_MASTER_CTAR_SHIFT, /*!< DSPI master transfer use CTAR3 setting. */
    kDSPI_MasterCtar4 = 4U << DSPI_MASTER_CTAR_SHIFT, /*!< DSPI master transfer use CTAR4 setting. */
    kDSPI_MasterCtar5 = 5U << DSPI_MASTER_CTAR_SHIFT, /*!< DSPI master transfer use CTAR5 setting. */
    kDSPI_MasterCtar6 = 6U << DSPI_MASTER_CTAR_SHIFT, /*!< DSPI master transfer use CTAR6 setting. */
    kDSPI_MasterCtar7 = 7U << DSPI_MASTER_CTAR_SHIFT, /*!< DSPI master transfer use CTAR7 setting. */

    kDSPI_MasterPcs0 = 0U << DSPI_MASTER_PCS_SHIFT, /*!< DSPI master transfer use PCS0 signal. */
    kDSPI_MasterPcs1 = 1U << DSPI_MASTER_PCS_SHIFT, /*!< DSPI master transfer use PCS1 signal. */
    kDSPI_MasterPcs2 = 2U << DSPI_MASTER_PCS_SHIFT, /*!< DSPI master transfer use PCS2 signal.*/
    kDSPI_MasterPcs3 = 3U << DSPI_MASTER_PCS_SHIFT, /*!< DSPI master transfer use PCS3 signal. */
    kDSPI_MasterPcs4 = 4U << DSPI_MASTER_PCS_SHIFT, /*!< DSPI master transfer use PCS4 signal. */
    kDSPI_MasterPcs5 = 5U << DSPI_MASTER_PCS_SHIFT, /*!< DSPI master transfer use PCS5 signal. */

    kDSPI_MasterPcsContinuous = 1U << 20, /*!< Indicates whether the PCS signal is continuous. */
    kDSPI_MasterActiveAfterTransfer =
        1U << 21, /*!< Indicates whether the PCS signal is active after the last frame transfer.*/
};

#define DSPI_SLAVE_CTAR_SHIFT (0U)   /*!< DSPI slave CTAR shift macro; used internally. */
#define DSPI_SLAVE_CTAR_MASK (0x07U) /*!< DSPI slave CTAR mask macro; used internally. */
/*! @brief Use this enumeration for the DSPI slave transfer configFlags. */
enum _dspi_transfer_config_flag_for_slave
{
    kDSPI_SlaveCtar0 = 0U << DSPI_SLAVE_CTAR_SHIFT, /*!< DSPI slave transfer use CTAR0 setting. */
                                                    /*!< DSPI slave can only use PCS0. */
};

/*! @brief DSPI transfer state, which is used for DSPI transactional API state machine. */
enum _dspi_transfer_state
{
    kDSPI_Idle = 0x0U, /*!< Nothing in the transmitter/receiver. */
    kDSPI_Busy,        /*!< Transfer queue is not finished. */
    kDSPI_Error        /*!< Transfer error. */
};

/*! @brief DSPI master command date configuration used for the SPIx_PUSHR.*/
typedef struct _dspi_command_data_config
{
    bool isPcsContinuous; /*!< Option to enable the continuous assertion of the chip select between transfers.*/
    dspi_ctar_selection_t whichCtar; /*!< The desired Clock and Transfer Attributes
                                          Register (CTAR) to use for CTAS.*/
    dspi_which_pcs_t whichPcs;       /*!< The desired PCS signal to use for the data transfer.*/
    bool isEndOfQueue;               /*!< Signals that the current transfer is the last in the queue.*/
    bool clearTransferCount;         /*!< Clears the SPI Transfer Counter (SPI_TCNT) before transmission starts.*/
} dspi_command_data_config_t;

/*! @brief DSPI master ctar configuration structure.*/
typedef struct _dspi_master_ctar_config
{
    uint32_t baudRate;                /*!< Baud Rate for DSPI. */
    uint32_t bitsPerFrame;            /*!< Bits per frame, minimum 4, maximum 16.*/
    dspi_clock_polarity_t cpol;       /*!< Clock polarity. */
    dspi_clock_phase_t cpha;          /*!< Clock phase. */
    dspi_shift_direction_t direction; /*!< MSB or LSB data shift direction. */

    uint32_t pcsToSckDelayInNanoSec;     /*!< PCS to SCK delay time in nanoseconds; setting to 0 sets the minimum
                                            delay. It also sets the boundary value if out of range.*/
    uint32_t lastSckToPcsDelayInNanoSec; /*!< The last SCK to PCS delay time in nanoseconds; setting to 0 sets the
                                            minimum delay. It also sets the boundary value if out of range.*/

    uint32_t betweenTransferDelayInNanoSec; /*!< After the SCK delay time in nanoseconds; setting to 0 sets the minimum
                                             delay. It also sets the boundary value if out of range.*/
} dspi_master_ctar_config_t;

/*! @brief DSPI master configuration structure.*/
typedef struct _dspi_master_config
{
    dspi_ctar_selection_t whichCtar;      /*!< The desired CTAR to use. */
    dspi_master_ctar_config_t ctarConfig; /*!< Set the ctarConfig to the desired CTAR. */

    dspi_which_pcs_t whichPcs;                     /*!< The desired Peripheral Chip Select (pcs). */
    dspi_pcs_polarity_config_t pcsActiveHighOrLow; /*!< The desired PCS active high or low. */

    bool enableContinuousSCK;   /*!< CONT_SCKE, continuous SCK enable. Note that the continuous SCK is only
                                     supported for CPHA = 1.*/
    bool enableRxFifoOverWrite; /*!< ROOE, receive FIFO overflow overwrite enable. If ROOE = 0, the incoming
                                     data is ignored and the data from the transfer that generated the overflow
                                     is also ignored. If ROOE = 1, the incoming data is shifted to the
                                     shift register. */

    bool enableModifiedTimingFormat;        /*!< Enables a modified transfer format to be used if true.*/
    dspi_master_sample_point_t samplePoint; /*!< Controls when the module master samples SIN in the Modified Transfer
                                                 Format. It's valid only when CPHA=0. */
} dspi_master_config_t;

/*! @brief DSPI slave ctar configuration structure.*/
typedef struct _dspi_slave_ctar_config
{
    uint32_t bitsPerFrame;      /*!< Bits per frame, minimum 4, maximum 16.*/
    dspi_clock_polarity_t cpol; /*!< Clock polarity. */
    dspi_clock_phase_t cpha;    /*!< Clock phase. */
                                /*!< Slave only supports MSB and does not support LSB.*/
} dspi_slave_ctar_config_t;

/*! @brief DSPI slave configuration structure.*/
typedef struct _dspi_slave_config
{
    dspi_ctar_selection_t whichCtar;     /*!< The desired CTAR to use. */
    dspi_slave_ctar_config_t ctarConfig; /*!< Set the ctarConfig to the desired CTAR. */

    bool enableContinuousSCK;               /*!< CONT_SCKE, continuous SCK enable. Note that the continuous SCK is only
                                                 supported for CPHA = 1.*/
    bool enableRxFifoOverWrite;             /*!< ROOE, receive FIFO overflow overwrite enable. If ROOE = 0, the incoming
                                                 data is ignored and the data from the transfer that generated the overflow
                                                 is also ignored. If ROOE = 1, the incoming data is shifted to the
                                                 shift register. */
    bool enableModifiedTimingFormat;        /*!< Enables a modified transfer format to be used if true.*/
    dspi_master_sample_point_t samplePoint; /*!< Controls when the module master samples SIN in the Modified Transfer
                                               Format. It's valid only when CPHA=0. */
} dspi_slave_config_t;

/*!
* @brief Forward declaration of the _dspi_master_handle typedefs.
*/
typedef struct _dspi_master_handle dspi_master_handle_t;

/*!
* @brief Forward declaration of the _dspi_slave_handle typedefs.
*/
typedef struct _dspi_slave_handle dspi_slave_handle_t;

/*!
 * @brief Completion callback function pointer type.
 *
 * @param base DSPI peripheral address.
 * @param handle Pointer to the handle for the DSPI master.
 * @param status Success or error code describing whether the transfer completed.
 * @param userData Arbitrary pointer-dataSized value passed from the application.
 */
typedef void (*dspi_master_transfer_callback_t)(SPI_Type *base,
                                                dspi_master_handle_t *handle,
                                                status_t status,
                                                void *userData);
/*!
 * @brief Completion callback function pointer type.
 *
 * @param base DSPI peripheral address.
 * @param handle Pointer to the handle for the DSPI slave.
 * @param status Success or error code describing whether the transfer completed.
 * @param userData Arbitrary pointer-dataSized value passed from the application.
 */
typedef void (*dspi_slave_transfer_callback_t)(SPI_Type *base,
                                               dspi_slave_handle_t *handle,
                                               status_t status,
                                               void *userData);

/*! @brief DSPI master/slave transfer structure.*/
typedef struct _dspi_transfer
{
    uint8_t *txData;          /*!< Send buffer. */
    uint8_t *rxData;          /*!< Receive buffer. */
    volatile size_t dataSize; /*!< Transfer bytes. */

    uint32_t
        configFlags; /*!< Transfer transfer configuration flags; set from _dspi_transfer_config_flag_for_master if the
                        transfer is used for master or _dspi_transfer_config_flag_for_slave enumeration if the transfer
                        is used for slave.*/
} dspi_transfer_t;

/*! @brief DSPI half-duplex(master) transfer structure */
typedef struct _dspi_half_duplex_transfer
{
    uint8_t *txData;            /*!< Send buffer */
    uint8_t *rxData;            /*!< Receive buffer */
    size_t txDataSize;          /*!< Transfer bytes for transmit */
    size_t rxDataSize;          /*!< Transfer bytes */
    uint32_t configFlags;       /*!< Transfer configuration flags; set from _dspi_transfer_config_flag_for_master. */
    bool isPcsAssertInTransfer; /*!< If Pcs pin keep assert between transmit and receive. true for assert and false for
                                   deassert. */
    bool isTransmitFirst;       /*!< True for transmit first and false for receive first. */
} dspi_half_duplex_transfer_t;

/*! @brief DSPI master transfer handle structure used for transactional API. */
struct _dspi_master_handle
{
    uint32_t bitsPerFrame;         /*!< The desired number of bits per frame. */
    volatile uint32_t command;     /*!< The desired data command. */
    volatile uint32_t lastCommand; /*!< The desired last data command. */

    uint8_t fifoSize; /*!< FIFO dataSize. */

    volatile bool
        isPcsActiveAfterTransfer;   /*!< Indicates whether the PCS signal is active after the last frame transfer.*/
    volatile bool isThereExtraByte; /*!< Indicates whether there are extra bytes.*/

    uint8_t *volatile txData;                  /*!< Send buffer. */
    uint8_t *volatile rxData;                  /*!< Receive buffer. */
    volatile size_t remainingSendByteCount;    /*!< A number of bytes remaining to send.*/
    volatile size_t remainingReceiveByteCount; /*!< A number of bytes remaining to receive.*/
    size_t totalByteCount;                     /*!< A number of transfer bytes*/

    volatile uint8_t state; /*!< DSPI transfer state, see _dspi_transfer_state.*/

    dspi_master_transfer_callback_t callback; /*!< Completion callback. */
    void *userData;                           /*!< Callback user data. */
};

/*! @brief DSPI slave transfer handle structure used for the transactional API. */
struct _dspi_slave_handle
{
    uint32_t bitsPerFrame;          /*!< The desired number of bits per frame. */
    volatile bool isThereExtraByte; /*!< Indicates whether there are extra bytes.*/

    uint8_t *volatile txData;                  /*!< Send buffer. */
    uint8_t *volatile rxData;                  /*!< Receive buffer. */
    volatile size_t remainingSendByteCount;    /*!< A number of bytes remaining to send.*/
    volatile size_t remainingReceiveByteCount; /*!< A number of bytes remaining to receive.*/
    size_t totalByteCount;                     /*!< A number of transfer bytes*/

    volatile uint8_t state; /*!< DSPI transfer state.*/

    volatile uint32_t errorCount; /*!< Error count for slave transfer.*/

    dspi_slave_transfer_callback_t callback; /*!< Completion callback. */
    void *userData;                          /*!< Callback user data. */
};

/**********************************************************************************************************************
 * API
 *********************************************************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus*/

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Initializes the DSPI master.
 *
 * This function initializes the DSPI master configuration. This is an example use case.
 *  @code
 *   dspi_master_config_t  masterConfig;
 *   masterConfig.whichCtar                                = kDSPI_Ctar0;
 *   masterConfig.ctarConfig.baudRate                      = 500000000U;
 *   masterConfig.ctarConfig.bitsPerFrame                  = 8;
 *   masterConfig.ctarConfig.cpol                          = kDSPI_ClockPolarityActiveHigh;
 *   masterConfig.ctarConfig.cpha                          = kDSPI_ClockPhaseFirstEdge;
 *   masterConfig.ctarConfig.direction                     = kDSPI_MsbFirst;
 *   masterConfig.ctarConfig.pcsToSckDelayInNanoSec        = 1000000000U / masterConfig.ctarConfig.baudRate ;
 *   masterConfig.ctarConfig.lastSckToPcsDelayInNanoSec    = 1000000000U / masterConfig.ctarConfig.baudRate ;
 *   masterConfig.ctarConfig.betweenTransferDelayInNanoSec = 1000000000U / masterConfig.ctarConfig.baudRate ;
 *   masterConfig.whichPcs                                 = kDSPI_Pcs0;
 *   masterConfig.pcsActiveHighOrLow                       = kDSPI_PcsActiveLow;
 *   masterConfig.enableContinuousSCK                      = false;
 *   masterConfig.enableRxFifoOverWrite                    = false;
 *   masterConfig.enableModifiedTimingFormat               = false;
 *   masterConfig.samplePoint                              = kDSPI_SckToSin0Clock;
 *   DSPI_MasterInit(base, &masterConfig, srcClock_Hz);
 *  @endcode
 *
 * @param base DSPI peripheral address.
 * @param masterConfig Pointer to the structure dspi_master_config_t.
 * @param srcClock_Hz Module source input clock in Hertz.
 */
void DSPI_MasterInit(SPI_Type *base, const dspi_master_config_t *masterConfig, uint32_t srcClock_Hz);

/*!
 * @brief Sets the dspi_master_config_t structure to default values.
 *
 * The purpose of this API is to get the configuration structure initialized for the DSPI_MasterInit().
 * Users may use the initialized structure unchanged in the DSPI_MasterInit() or modify the structure
 * before calling the DSPI_MasterInit().
 * Example:
 * @code
 *  dspi_master_config_t  masterConfig;
 *  DSPI_MasterGetDefaultConfig(&masterConfig);
 * @endcode
 * @param masterConfig pointer to dspi_master_config_t structure
 */
void DSPI_MasterGetDefaultConfig(dspi_master_config_t *masterConfig);

/*!
 * @brief DSPI slave configuration.
 *
 * This function initializes the DSPI slave configuration. This is an example use case.
 *  @code
 *   dspi_slave_config_t  slaveConfig;
 *  slaveConfig->whichCtar                  = kDSPI_Ctar0;
 *  slaveConfig->ctarConfig.bitsPerFrame    = 8;
 *  slaveConfig->ctarConfig.cpol            = kDSPI_ClockPolarityActiveHigh;
 *  slaveConfig->ctarConfig.cpha            = kDSPI_ClockPhaseFirstEdge;
 *  slaveConfig->enableContinuousSCK        = false;
 *  slaveConfig->enableRxFifoOverWrite      = false;
 *  slaveConfig->enableModifiedTimingFormat = false;
 *  slaveConfig->samplePoint                = kDSPI_SckToSin0Clock;
 *   DSPI_SlaveInit(base, &slaveConfig);
 *  @endcode
 *
 * @param base DSPI peripheral address.
 * @param slaveConfig Pointer to the structure dspi_master_config_t.
 */
void DSPI_SlaveInit(SPI_Type *base, const dspi_slave_config_t *slaveConfig);

/*!
 * @brief Sets the dspi_slave_config_t structure to a default value.
 *
 * The purpose of this API is to get the configuration structure initialized for the DSPI_SlaveInit().
 * Users may use the initialized structure unchanged in the DSPI_SlaveInit() or modify the structure
 * before calling the DSPI_SlaveInit().
 * This is an example.
 * @code
 *  dspi_slave_config_t  slaveConfig;
 *  DSPI_SlaveGetDefaultConfig(&slaveConfig);
 * @endcode
 * @param slaveConfig Pointer to the dspi_slave_config_t structure.
 */
void DSPI_SlaveGetDefaultConfig(dspi_slave_config_t *slaveConfig);

/*!
 * @brief De-initializes the DSPI peripheral. Call this API to disable the DSPI clock.
 * @param base DSPI peripheral address.
 */
void DSPI_Deinit(SPI_Type *base);

/*!
 * @brief Enables the DSPI peripheral and sets the MCR MDIS to 0.
 *
 * @param base DSPI peripheral address.
 * @param enable Pass true to enable module, false to disable module.
 */
static inline void DSPI_Enable(SPI_Type *base, bool enable)
{
    if (enable)
    {
        base->MCR &= ~SPI_MCR_MDIS_MASK;
    }
    else
    {
        base->MCR |= SPI_MCR_MDIS_MASK;
    }
}

/*!
 *@}
*/

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Gets the DSPI status flag state.
 * @param base DSPI peripheral address.
 * @return DSPI status (in SR register).
 */
static inline uint32_t DSPI_GetStatusFlags(SPI_Type *base)
{
    return (base->SR);
}

/*!
 * @brief Clears the DSPI status flag.
 *
 * This function  clears the desired status bit by using a write-1-to-clear. The user passes in the base and the
 * desired status bit to clear.  The list of status bits is defined in the dspi_status_and_interrupt_request_t. The
 * function uses these bit positions in its algorithm to clear the desired flag state.
 * This is an example.
 * @code
 *  DSPI_ClearStatusFlags(base, kDSPI_TxCompleteFlag|kDSPI_EndOfQueueFlag);
 * @endcode
 *
 * @param base DSPI peripheral address.
 * @param statusFlags The status flag used from the type dspi_flags.
 */
static inline void DSPI_ClearStatusFlags(SPI_Type *base, uint32_t statusFlags)
{
    base->SR = statusFlags; /*!< The status flags are cleared by writing 1 (w1c).*/
}

/*!
 *@}
*/

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enables the DSPI interrupts.
 *
 * This function configures the various interrupt masks of the DSPI.  The parameters are a base and an interrupt mask.
 * Note, for Tx Fill and Rx FIFO drain requests, enable the interrupt request and disable the DMA request.
 *       Do not use this API(write to RSER register) while DSPI is in running state.
 *
 * @code
 *  DSPI_EnableInterrupts(base, kDSPI_TxCompleteInterruptEnable | kDSPI_EndOfQueueInterruptEnable );
 * @endcode
 *
 * @param base DSPI peripheral address.
 * @param mask The interrupt mask; use the enum _dspi_interrupt_enable.
 */
void DSPI_EnableInterrupts(SPI_Type *base, uint32_t mask);

/*!
 * @brief Disables the DSPI interrupts.
 *
 * @code
 *  DSPI_DisableInterrupts(base, kDSPI_TxCompleteInterruptEnable | kDSPI_EndOfQueueInterruptEnable );
 * @endcode
 *
 * @param base DSPI peripheral address.
 * @param mask The interrupt mask; use the enum _dspi_interrupt_enable.
 */
static inline void DSPI_DisableInterrupts(SPI_Type *base, uint32_t mask)
{
    base->RSER &= ~mask;
}

/*!
 *@}
*/

/*!
 * @name DMA Control
 * @{
 */

/*!
 * @brief Enables the DSPI DMA request.
 *
 * This function configures the Rx and Tx DMA mask of the DSPI.  The parameters are a base and a DMA mask.
 * @code
 *  DSPI_EnableDMA(base, kDSPI_TxDmaEnable | kDSPI_RxDmaEnable);
 * @endcode
 *
 * @param base DSPI peripheral address.
 * @param mask The interrupt mask; use the enum dspi_dma_enable.
 */
static inline void DSPI_EnableDMA(SPI_Type *base, uint32_t mask)
{
    base->RSER |= mask;
}

/*!
 * @brief Disables the DSPI DMA request.
 *
 * This function configures the Rx and Tx DMA mask of the DSPI.  The parameters are a base and a DMA mask.
 * @code
 *  SPI_DisableDMA(base, kDSPI_TxDmaEnable | kDSPI_RxDmaEnable);
 * @endcode
 *
 * @param base DSPI peripheral address.
 * @param mask The interrupt mask; use the enum dspi_dma_enable.
 */
static inline void DSPI_DisableDMA(SPI_Type *base, uint32_t mask)
{
    base->RSER &= ~mask;
}

/*!
 * @brief Gets the DSPI master PUSHR data register address for the DMA operation.
 *
 * This function gets the DSPI master PUSHR data register address because this value is needed for the DMA operation.
 *
 * @param base DSPI peripheral address.
 * @return The DSPI master PUSHR data register address.
 */
static inline uint32_t DSPI_MasterGetTxRegisterAddress(SPI_Type *base)
{
    return (uint32_t) & (base->PUSHR);
}

/*!
 * @brief Gets the DSPI slave PUSHR data register address for the DMA operation.
 *
 * This function gets the DSPI slave PUSHR data register address as this value is needed for the DMA operation.
 *
 * @param base DSPI peripheral address.
 * @return The DSPI slave PUSHR data register address.
 */
static inline uint32_t DSPI_SlaveGetTxRegisterAddress(SPI_Type *base)
{
    return (uint32_t) & (base->PUSHR_SLAVE);
}

/*!
 * @brief Gets the DSPI POPR data register address for the DMA operation.
 *
 * This function gets the DSPI POPR data register address as this value is needed for the DMA operation.
 *
 * @param base DSPI peripheral address.
 * @return The DSPI POPR data register address.
 */
static inline uint32_t DSPI_GetRxRegisterAddress(SPI_Type *base)
{
    return (uint32_t) & (base->POPR);
}

/*!
 *@}
*/

/*!
 * @name Bus Operations
 * @{
 */

/*!
 * @brief Configures the DSPI for master or slave.
 *
 * @param base DSPI peripheral address.
 * @param mode Mode setting (master or slave) of type dspi_master_slave_mode_t.
 */
static inline void DSPI_SetMasterSlaveMode(SPI_Type *base, dspi_master_slave_mode_t mode)
{
    base->MCR = (base->MCR & (~SPI_MCR_MSTR_MASK)) | SPI_MCR_MSTR(mode);
}

/*!
 * @brief Returns whether the DSPI module is in master mode.
 *
 * @param base DSPI peripheral address.
 * @return Returns true if the module is in master mode or false if the module is in slave mode.
 */
static inline bool DSPI_IsMaster(SPI_Type *base)
{
    return (bool)((base->MCR) & SPI_MCR_MSTR_MASK);
}
/*!
 * @brief Starts the DSPI transfers and clears HALT bit in MCR.
 *
 * This function sets the module to start data transfer in either master or slave mode.
 *
 * @param base DSPI peripheral address.
 */
static inline void DSPI_StartTransfer(SPI_Type *base)
{
    base->MCR &= ~SPI_MCR_HALT_MASK;
}
/*!
 * @brief Stops DSPI transfers and sets the HALT bit in MCR.
 *
 * This function stops data transfers in either master or slave modes.
 *
 * @param base DSPI peripheral address.
 */
static inline void DSPI_StopTransfer(SPI_Type *base)
{
    base->MCR |= SPI_MCR_HALT_MASK;
}

/*!
 * @brief Enables or disables the DSPI FIFOs.
 *
 * This function  allows the caller to disable/enable the Tx and Rx FIFOs independently.
 * Note that to disable, pass in a logic 0 (false) for the particular FIFO configuration.  To enable,
 * pass in a logic 1 (true).
 *
 * @param base DSPI peripheral address.
 * @param enableTxFifo Disables (false) the TX FIFO; Otherwise, enables (true) the TX FIFO
 * @param enableRxFifo Disables (false) the RX FIFO; Otherwise, enables (true) the RX FIFO
 */
static inline void DSPI_SetFifoEnable(SPI_Type *base, bool enableTxFifo, bool enableRxFifo)
{
    base->MCR = (base->MCR & (~(SPI_MCR_DIS_RXF_MASK | SPI_MCR_DIS_TXF_MASK))) | SPI_MCR_DIS_TXF(!enableTxFifo) |
                SPI_MCR_DIS_RXF(!enableRxFifo);
}

/*!
 * @brief Flushes the DSPI FIFOs.
 *
 * @param base DSPI peripheral address.
 * @param flushTxFifo Flushes (true) the Tx FIFO; Otherwise, does not flush (false) the Tx FIFO
 * @param flushRxFifo Flushes (true) the Rx FIFO; Otherwise, does not flush (false) the Rx FIFO
 */
static inline void DSPI_FlushFifo(SPI_Type *base, bool flushTxFifo, bool flushRxFifo)
{
    base->MCR = (base->MCR & (~(SPI_MCR_CLR_TXF_MASK | SPI_MCR_CLR_RXF_MASK))) | SPI_MCR_CLR_TXF(flushTxFifo) |
                SPI_MCR_CLR_RXF(flushRxFifo);
}

/*!
 * @brief Configures the DSPI peripheral chip select polarity simultaneously.
 * For example, PCS0 and PCS1 are set to active low and other PCS is set to active high. Note that the number of
 * PCSs is specific to the device.
 * @code
 *  DSPI_SetAllPcsPolarity(base, kDSPI_Pcs0ActiveLow | kDSPI_Pcs1ActiveLow);
   @endcode
 * @param base DSPI peripheral address.
 * @param mask The PCS polarity mask; use the enum _dspi_pcs_polarity.
 */
static inline void DSPI_SetAllPcsPolarity(SPI_Type *base, uint32_t mask)
{
    base->MCR = (base->MCR & ~SPI_MCR_PCSIS_MASK) | SPI_MCR_PCSIS(mask);
}

/*!
 * @brief Sets the DSPI baud rate in bits per second.
 *
 * This function  takes in the desired baudRate_Bps (baud rate) and calculates the nearest possible baud rate without
 * exceeding the desired baud rate, and returns the calculated baud rate in bits-per-second. It requires that the
 * caller also provide the frequency of the module source clock (in Hertz).
 *
 * @param base DSPI peripheral address.
 * @param whichCtar The desired Clock and Transfer Attributes Register (CTAR) of the type dspi_ctar_selection_t
 * @param baudRate_Bps The desired baud rate in bits per second
 * @param srcClock_Hz Module source input clock in Hertz
 * @return The actual calculated baud rate
 */
uint32_t DSPI_MasterSetBaudRate(SPI_Type *base,
                                dspi_ctar_selection_t whichCtar,
                                uint32_t baudRate_Bps,
                                uint32_t srcClock_Hz);

/*!
 * @brief Manually configures the delay prescaler and scaler for a particular CTAR.
 *
 * This function configures the PCS to SCK delay pre-scalar (PcsSCK) and scalar (CSSCK), after SCK delay pre-scalar
 * (PASC) and scalar (ASC), and the delay after transfer pre-scalar (PDT) and scalar (DT).
 *
 * These delay names are available in the type dspi_delay_type_t.
 *
 * The user passes the delay to the configuration along with the prescaler and scaler value.
 * This allows the user to directly set the prescaler/scaler values if pre-calculated or
 * to manually increment either value.
 *
 * @param base DSPI peripheral address.
 * @param whichCtar The desired Clock and Transfer Attributes Register (CTAR) of type dspi_ctar_selection_t.
 * @param prescaler The prescaler delay value (can be an integer 0, 1, 2, or 3).
 * @param scaler The scaler delay value (can be any integer between 0 to 15).
 * @param whichDelay The desired delay to configure; must be of type dspi_delay_type_t
 */
void DSPI_MasterSetDelayScaler(
    SPI_Type *base, dspi_ctar_selection_t whichCtar, uint32_t prescaler, uint32_t scaler, dspi_delay_type_t whichDelay);

/*!
 * @brief Calculates the delay prescaler and scaler based on the desired delay input in nanoseconds.
 *
 * This function calculates the values for the following.
 * PCS to SCK delay pre-scalar (PCSSCK) and scalar (CSSCK), or
 * After SCK delay pre-scalar (PASC) and scalar (ASC), or
 * Delay after transfer pre-scalar (PDT) and scalar (DT).
 *
 * These delay names are available in the type dspi_delay_type_t.
 *
 * The user passes which delay to configure along with the desired delay value in nanoseconds.  The function
 * calculates the values needed for the prescaler and scaler. Note that returning the calculated delay as an exact
 * delay match may not be possible. In this case, the closest match is calculated without going below the desired
 * delay value input.
 * It is possible to input a very large delay value that exceeds the capability of the part, in which case the maximum
 * supported delay is returned. The higher-level peripheral driver alerts the user of an out of range delay
 * input.
 *
 * @param base DSPI peripheral address.
 * @param whichCtar The desired Clock and Transfer Attributes Register (CTAR) of type dspi_ctar_selection_t.
 * @param whichDelay The desired delay to configure, must be of type dspi_delay_type_t
 * @param srcClock_Hz Module source input clock in Hertz
 * @param delayTimeInNanoSec The desired delay value in nanoseconds.
 * @return The actual calculated delay value.
 */
uint32_t DSPI_MasterSetDelayTimes(SPI_Type *base,
                                  dspi_ctar_selection_t whichCtar,
                                  dspi_delay_type_t whichDelay,
                                  uint32_t srcClock_Hz,
                                  uint32_t delayTimeInNanoSec);

/*!
 * @brief Writes data into the data buffer for master mode.
 *
 * In master mode, the 16-bit data is appended to the 16-bit command info. The command portion
 * provides characteristics of the data, such as the optional continuous chip select
 * operation between transfers, the desired Clock and Transfer Attributes register to use for the
 * associated SPI frame, the desired PCS signal to use for the data transfer, whether the current
 * transfer is the last in the queue, and whether to clear the transfer count (normally needed when
 * sending the first frame of a data packet). This is an example.
 * @code
 *  dspi_command_data_config_t commandConfig;
 *  commandConfig.isPcsContinuous = true;
 *  commandConfig.whichCtar = kDSPICtar0;
 *  commandConfig.whichPcs = kDSPIPcs0;
 *  commandConfig.clearTransferCount = false;
 *  commandConfig.isEndOfQueue = false;
 *  DSPI_MasterWriteData(base, &commandConfig, dataWord);
   @endcode
 *
 * @param base DSPI peripheral address.
 * @param command Pointer to the command structure.
 * @param data The data word to be sent.
 */
static inline void DSPI_MasterWriteData(SPI_Type *base, dspi_command_data_config_t *command, uint16_t data)
{
    base->PUSHR = SPI_PUSHR_CONT(command->isPcsContinuous) | SPI_PUSHR_CTAS(command->whichCtar) |
                  SPI_PUSHR_PCS(command->whichPcs) | SPI_PUSHR_EOQ(command->isEndOfQueue) |
                  SPI_PUSHR_CTCNT(command->clearTransferCount) | SPI_PUSHR_TXDATA(data);
}

/*!
 * @brief Sets the dspi_command_data_config_t structure to default values.
 *
 * The purpose of this API is to get the configuration structure initialized for use in the DSPI_MasterWrite_xx().
 * Users may use the initialized structure unchanged in the DSPI_MasterWrite_xx() or modify the structure
 * before calling the DSPI_MasterWrite_xx().
 * This is an example.
 * @code
 *  dspi_command_data_config_t  command;
 *  DSPI_GetDefaultDataCommandConfig(&command);
 * @endcode
 * @param command Pointer to the dspi_command_data_config_t structure.
 */
void DSPI_GetDefaultDataCommandConfig(dspi_command_data_config_t *command);

/*!
 * @brief Writes data into the data buffer master mode and waits till complete to return.
 *
 * In master mode, the 16-bit data is appended to the 16-bit command info. The command portion
 * provides characteristics of the data, such as the optional continuous chip select
 * operation between transfers, the desired Clock and Transfer Attributes register to use for the
 * associated SPI frame, the desired PCS signal to use for the data transfer, whether the current
 * transfer is the last in the queue, and whether to clear the transfer count (normally needed when
 * sending the first frame of a data packet). This is an example.
 * @code
 *  dspi_command_config_t commandConfig;
 *  commandConfig.isPcsContinuous = true;
 *  commandConfig.whichCtar = kDSPICtar0;
 *  commandConfig.whichPcs = kDSPIPcs1;
 *  commandConfig.clearTransferCount = false;
 *  commandConfig.isEndOfQueue = false;
 *  DSPI_MasterWriteDataBlocking(base, &commandConfig, dataWord);
 * @endcode
 *
 * Note that this function does not return until after the transmit is complete. Also note that the DSPI must be
 * enabled and running to transmit data (MCR[MDIS] & [HALT] = 0). Because the SPI is a synchronous protocol,
 * the received data is available when the transmit completes.
 *
 * @param base DSPI peripheral address.
 * @param command Pointer to the command structure.
 * @param data The data word to be sent.
 */
void DSPI_MasterWriteDataBlocking(SPI_Type *base, dspi_command_data_config_t *command, uint16_t data);

/*!
 * @brief Returns the DSPI command word formatted to the PUSHR data register bit field.
 *
 * This function allows the caller to pass in the data command structure and returns the command word formatted
 * according to the DSPI PUSHR register bit field placement. The user can then "OR" the returned command word with the
 * desired data to send and use the function DSPI_HAL_WriteCommandDataMastermode or
 * DSPI_HAL_WriteCommandDataMastermodeBlocking to write the entire 32-bit command data word to the PUSHR. This helps
 * improve performance in cases where the command structure is constant. For example, the user calls this function
 * before starting a transfer to generate the command word. When they are ready to transmit the data, they OR
 * this formatted command word with the desired data to transmit. This process increases transmit performance when
 * compared to calling send functions, such as DSPI_HAL_WriteDataMastermode,  which format the command word each time a
 * data word is to be sent.
 *
 * @param command Pointer to the command structure.
 * @return The command word formatted to the PUSHR data register bit field.
 */
static inline uint32_t DSPI_MasterGetFormattedCommand(dspi_command_data_config_t *command)
{
    /* Format the 16-bit command word according to the PUSHR data register bit field*/
    return (uint32_t)(SPI_PUSHR_CONT(command->isPcsContinuous) | SPI_PUSHR_CTAS(command->whichCtar) |
                      SPI_PUSHR_PCS(command->whichPcs) | SPI_PUSHR_EOQ(command->isEndOfQueue) |
                      SPI_PUSHR_CTCNT(command->clearTransferCount));
}

/*!
 * @brief Writes a 32-bit data word (16-bit command appended with 16-bit data) into the data
 *        buffer master mode and waits till complete to return.
 *
 * In this function, the user must append the 16-bit data to the 16-bit command information and then provide the total
* 32-bit word
 * as the data to send.
 * The command portion provides characteristics of the data, such as the optional continuous chip select operation
 * between transfers, the desired Clock and Transfer Attributes register to use for the associated SPI frame, the
* desired PCS
 * signal to use for the data transfer, whether the current transfer is the last in the queue, and whether to clear the
 * transfer count (normally needed when sending the first frame of a data packet). The user is responsible for
 * appending this command with the data to send. This is an example:
 * @code
 *  dataWord = <16-bit command> | <16-bit data>;
 *  DSPI_MasterWriteCommandDataBlocking(base, dataWord);
 * @endcode
 *
 * Note that this function does not return until after the transmit is complete. Also note that the DSPI must be
 * enabled and running to transmit data (MCR[MDIS] & [HALT] = 0).
 * Because the SPI is a synchronous protocol, the received data is available when the transmit completes.
 *
 *  For a blocking polling transfer, see methods below.
 *  Option 1:
*   uint32_t command_to_send = DSPI_MasterGetFormattedCommand(&command);
*   uint32_t data0 = command_to_send | data_need_to_send_0;
*   uint32_t data1 = command_to_send | data_need_to_send_1;
*   uint32_t data2 = command_to_send | data_need_to_send_2;
*
*   DSPI_MasterWriteCommandDataBlocking(base,data0);
*   DSPI_MasterWriteCommandDataBlocking(base,data1);
*   DSPI_MasterWriteCommandDataBlocking(base,data2);
*
*  Option 2:
*   DSPI_MasterWriteDataBlocking(base,&command,data_need_to_send_0);
*   DSPI_MasterWriteDataBlocking(base,&command,data_need_to_send_1);
*   DSPI_MasterWriteDataBlocking(base,&command,data_need_to_send_2);
*
 * @param base DSPI peripheral address.
 * @param data The data word (command and data combined) to be sent.
 */
void DSPI_MasterWriteCommandDataBlocking(SPI_Type *base, uint32_t data);

/*!
 * @brief Writes data into the data buffer in slave mode.
 *
 * In slave mode, up to 16-bit words may be written.
 *
 * @param base DSPI peripheral address.
 * @param data The data to send.
 */
static inline void DSPI_SlaveWriteData(SPI_Type *base, uint32_t data)
{
    base->PUSHR_SLAVE = data;
}

/*!
 * @brief Writes data into the data buffer in slave mode, waits till data was transmitted, and returns.
 *
 * In slave mode, up to 16-bit words may be written. The function first clears the transmit complete flag, writes data
 * into data register, and finally waits until the data is transmitted.
 *
 * @param base DSPI peripheral address.
 * @param data The data to send.
 */
void DSPI_SlaveWriteDataBlocking(SPI_Type *base, uint32_t data);

/*!
 * @brief Reads data from the data buffer.
 *
 * @param base DSPI peripheral address.
 * @return The data from the read data buffer.
 */
static inline uint32_t DSPI_ReadData(SPI_Type *base)
{
    return (base->POPR);
}

/*!
 * @brief Set up the dummy data.
 *
 * @param base DSPI peripheral address.
 * @param dummyData Data to be transferred when tx buffer is NULL.
 */
void DSPI_SetDummyData(SPI_Type *base, uint8_t dummyData);

/*!
 *@}
*/

/*!
 * @name Transactional
 * @{
 */
/*Transactional APIs*/

/*!
 * @brief Initializes the DSPI master handle.
 *
 * This function initializes the DSPI handle, which can be used for other DSPI transactional APIs.  Usually, for a
 * specified DSPI instance,  call this API once to get the initialized handle.
 *
 * @param base DSPI peripheral base address.
 * @param handle DSPI handle pointer to dspi_master_handle_t.
 * @param callback DSPI callback.
 * @param userData Callback function parameter.
 */
void DSPI_MasterTransferCreateHandle(SPI_Type *base,
                                     dspi_master_handle_t *handle,
                                     dspi_master_transfer_callback_t callback,
                                     void *userData);

/*!
 * @brief DSPI master transfer data using polling.
 *
 * This function transfers data using polling. This is a blocking function, which does not return until all transfers
 * have been completed.
 *
 * @param base DSPI peripheral base address.
 * @param transfer Pointer to the dspi_transfer_t structure.
 * @return status of status_t.
 */
status_t DSPI_MasterTransferBlocking(SPI_Type *base, dspi_transfer_t *transfer);

/*!
 * @brief DSPI master transfer data using interrupts.
 *
 * This function transfers data using interrupts. This is a non-blocking function, which returns right away. When all
 * data is transferred, the callback function is called.

 * @param base DSPI peripheral base address.
 * @param handle Pointer to the dspi_master_handle_t structure which stores the transfer state.
 * @param transfer Pointer to the dspi_transfer_t structure.
 * @return status of status_t.
 */
status_t DSPI_MasterTransferNonBlocking(SPI_Type *base, dspi_master_handle_t *handle, dspi_transfer_t *transfer);

/*!
 * @brief Transfers a block of data using a polling method.
 *
 * This function will do a half-duplex transfer for DSPI master, This is a blocking function,
 * which does not retuen until all transfer have been completed. And data transfer will be half-duplex,
 * users can set transmit first or receive first.
 *
 * @param base DSPI base pointer
 * @param xfer pointer to dspi_half_duplex_transfer_t structure
 * @return status of status_t.
 */
status_t DSPI_MasterHalfDuplexTransferBlocking(SPI_Type *base, dspi_half_duplex_transfer_t *xfer);

/*!
 * @brief Performs a non-blocking DSPI interrupt transfer.
 *
 * This function transfers data using interrupts, the transfer mechanism is half-duplex. This is a non-blocking
 * function,
 * which returns right away. When all data is transferred, the callback function is called.
 *
 * @param base DSPI peripheral base address.
 * @param handle pointer to dspi_master_handle_t structure which stores the transfer state
 * @param xfer pointer to dspi_half_duplex_transfer_t structure
 * @return status of status_t.
 */
status_t DSPI_MasterHalfDuplexTransferNonBlocking(SPI_Type *base,
                                                  dspi_master_handle_t *handle,
                                                  dspi_half_duplex_transfer_t *xfer);

/*!
 * @brief Gets the master transfer count.
 *
 * This function gets the master transfer count.
 *
 * @param base DSPI peripheral base address.
 * @param handle Pointer to the dspi_master_handle_t structure which stores the transfer state.
 * @param count The number of bytes transferred by using the non-blocking transaction.
 * @return status of status_t.
 */
status_t DSPI_MasterTransferGetCount(SPI_Type *base, dspi_master_handle_t *handle, size_t *count);

/*!
 * @brief DSPI master aborts a transfer using an interrupt.
 *
 * This function aborts a transfer using an interrupt.
 *
 * @param base DSPI peripheral base address.
 * @param handle Pointer to the dspi_master_handle_t structure which stores the transfer state.
 */
void DSPI_MasterTransferAbort(SPI_Type *base, dspi_master_handle_t *handle);

/*!
 * @brief DSPI Master IRQ handler function.
 *
 * This function processes the DSPI transmit and receive IRQ.

 * @param base DSPI peripheral base address.
 * @param handle Pointer to the dspi_master_handle_t structure which stores the transfer state.
 */
void DSPI_MasterTransferHandleIRQ(SPI_Type *base, dspi_master_handle_t *handle);

/*!
 * @brief Initializes the DSPI slave handle.
 *
 * This function initializes the DSPI handle, which can be used for other DSPI transactional APIs.  Usually, for a
 * specified DSPI instance, call this API once to get the initialized handle.
 *
 * @param handle DSPI handle pointer to the dspi_slave_handle_t.
 * @param base DSPI peripheral base address.
 * @param callback DSPI callback.
 * @param userData Callback function parameter.
 */
void DSPI_SlaveTransferCreateHandle(SPI_Type *base,
                                    dspi_slave_handle_t *handle,
                                    dspi_slave_transfer_callback_t callback,
                                    void *userData);

/*!
 * @brief DSPI slave transfers data using an interrupt.
 *
 * This function transfers data using an interrupt. This is a non-blocking function, which returns right away. When all
 * data is transferred, the callback function is called.
 *
 * @param base DSPI peripheral base address.
 * @param handle Pointer to the dspi_slave_handle_t structure which stores the transfer state.
 * @param transfer Pointer to the dspi_transfer_t structure.
 * @return status of status_t.
 */
status_t DSPI_SlaveTransferNonBlocking(SPI_Type *base, dspi_slave_handle_t *handle, dspi_transfer_t *transfer);

/*!
 * @brief Gets the slave transfer count.
 *
 * This function gets the slave transfer count.
 *
 * @param base DSPI peripheral base address.
 * @param handle Pointer to the dspi_master_handle_t structure which stores the transfer state.
 * @param count The number of bytes transferred by using the non-blocking transaction.
 * @return status of status_t.
 */
status_t DSPI_SlaveTransferGetCount(SPI_Type *base, dspi_slave_handle_t *handle, size_t *count);

/*!
 * @brief DSPI slave aborts a transfer using an interrupt.
 *
 * This function aborts a transfer using an interrupt.
 *
 * @param base DSPI peripheral base address.
 * @param handle Pointer to the dspi_slave_handle_t structure which stores the transfer state.
 */
void DSPI_SlaveTransferAbort(SPI_Type *base, dspi_slave_handle_t *handle);

/*!
 * @brief DSPI Master IRQ handler function.
 *
 * This function processes the DSPI transmit and receive IRQ.
 *
 * @param base DSPI peripheral base address.
 * @param handle Pointer to the dspi_slave_handle_t structure which stores the transfer state.
 */
void DSPI_SlaveTransferHandleIRQ(SPI_Type *base, dspi_slave_handle_t *handle);

/*!
 *@}
*/

#if defined(__cplusplus)
}
#endif /*_cplusplus*/
       /*!
        *@}
       */

#endif /*_FSL_DSPI_H_*/
