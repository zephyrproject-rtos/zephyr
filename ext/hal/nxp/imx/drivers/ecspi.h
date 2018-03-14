/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
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

#ifndef __ECSPI_H__
#define __ECSPI_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup ecspi_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Channel select. */
enum _ecspi_channel_select
{
    ecspiSelectChannel0 = 0U, /*!< Select Channel 0. Chip Select 0 (SS0) is asserted.*/
    ecspiSelectChannel1 = 1U, /*!< Select Channel 1. Chip Select 1 (SS1) is asserted.*/
    ecspiSelectChannel2 = 2U, /*!< Select Channel 2. Chip Select 2 (SS2) is asserted.*/
    ecspiSelectChannel3 = 3U, /*!< Select Channel 3. Chip Select 3 (SS3) is asserted.*/
};

/*! @brief Channel mode. */
enum _ecspi_master_slave_mode
{
    ecspiSlaveMode  = 0U, /*!< Set Slave Mode.*/
    ecspiMasterMode = 1U, /*!< Set Master Mode.*/
};

/*! @brief Clock phase. */
enum _ecspi_clock_phase
{
    ecspiClockPhaseFirstEdge  = 0U, /*!< Data is captured on the leading edge of the SCK and
                                         changed on the following edge.*/
    ecspiClockPhaseSecondEdge = 1U, /*!< Data is changed on the leading edge of the SCK and
                                         captured on the following edge.*/
};

/*! @brief Clock polarity. */
enum _ecspi_clock_polarity
{
    ecspiClockPolarityActiveHigh = 0U, /*!< Active-high eCSPI clock (idles low).*/
    ecspiClockPolarityActiveLow  = 1U, /*!< Active-low eCSPI clock (idles high).*/
};

/*! @brief SS signal polarity. */
enum _ecspi_ss_polarity
{
    ecspiSSPolarityActiveLow  = 0U, /*!< Active-low, eCSPI SS signal.*/
    ecspiSSPolarityActiveHigh = 1U, /*!< Active-high, eCSPI SS signal.*/
};

/*! @brief Inactive state of data line. */
enum _ecspi_dataline_inactivestate
{
    ecspiDataLineStayHigh = 0U, /*!< Data line inactive state stay high.*/
    ecspiDataLineStayLow  = 1U, /*!< Data line inactive state stay low.*/
};

/*! @brief Inactive state of SCLK. */
enum _ecspi_sclk_inactivestate
{
    ecspiSclkStayLow  = 0U, /*!< SCLK inactive state stay low.*/
    ecspiSclkStayHigh = 1U, /*!< SCLK line inactive state stay high.*/
};

/*! @brief sample period counter clock source. */
enum _ecspi_sampleperiod_clocksource
{
    ecspiSclk       = 0U, /*!< Sample period counter clock from SCLK.*/
    ecspiLowFreq32K = 1U, /*!< Sample period counter clock from from LFRC (32.768 KHz).*/
};

/*! @brief DMA Source definition. */
enum _ecspi_dma_source
{
    ecspiDmaTxfifoEmpty   = 7U,  /*!< TXFIFO Empty DMA Request.*/
    ecspiDmaRxfifoRequest = 23U, /*!< RXFIFO DMA Request.*/
    ecspiDmaRxfifoTail    = 31U, /*!< RXFIFO TAIL DMA Request.*/
};

/*! @brief RXFIFO and TXFIFO threshold. */
enum _ecspi_fifothreshold
{
    ecspiTxfifoThreshold = 0U,  /*!< Defines the FIFO threshold that triggers a TX DMA/INT request.*/
    ecspiRxfifoThreshold = 16U, /*!< defines the FIFO threshold that triggers a RX DMA/INT request.*/
};

/*! @brief Status flag. */
enum _ecspi_status_flag
{
    ecspiFlagTxfifoEmpty       = 1U << 0, /*!< TXFIFO Empty Flag.*/
    ecspiFlagTxfifoDataRequest = 1U << 1, /*!< TXFIFO Data Request Flag.*/
    ecspiFlagTxfifoFull        = 1U << 2, /*!< TXFIFO Full Flag.*/
    ecspiFlagRxfifoReady       = 1U << 3, /*!< RXFIFO Ready Flag.*/
    ecspiFlagRxfifoDataRequest = 1U << 4, /*!< RXFIFO Data Request Flag.*/
    ecspiFlagRxfifoFull        = 1U << 5, /*!< RXFIFO Full Flag.*/
    ecspiFlagRxfifoOverflow    = 1U << 6, /*!< RXFIFO Overflow Flag.*/
    ecspiFlagTxfifoTc          = 1U << 7, /*!< TXFIFO Transform Completed Flag.*/
};

/*! @brief Data Ready Control. */
enum _ecspi_data_ready
{
    ecspiRdyNoCare       = 0U, /*!< The SPI_RDY signal is ignored.*/
    ecspiRdyFallEdgeTrig = 1U, /*!< Burst is triggered by the falling edge of the SPI_RDY signal (edge-triggered).*/
    ecspiRdyLowLevelTrig = 2U, /*!< Burst is triggered by a low level of the SPI_RDY signal (level-triggered).*/
    ecspiRdyReserved     = 3U, /*!< Reserved.*/
};

/*! @brief Init structure. */
typedef struct _ecspi_init_config
{
    uint32_t clockRate;     /*!< Specifies ECSPII module clock freq.*/
    uint32_t baudRate;      /*!< Specifies desired eCSPI baud rate.*/
    uint32_t channelSelect; /*!< Specifies the channel select.*/
    uint32_t mode;          /*!< Specifies the mode.*/
    uint32_t burstLength;   /*!< Specifies the length of a burst to be transferred.*/
    uint32_t clockPhase;    /*!< Specifies the clock phase.*/
    uint32_t clockPolarity; /*!< Specifies the clock polarity.*/
    bool ecspiAutoStart;    /*!< Specifies the start mode.*/
} ecspi_init_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name eCSPI Initialization and Configuration functions
 * @{
 */

/*!
 * @brief Initializes the eCSPI module.
 *
 * @param base eCSPI base pointer.
 * @param initConfig eCSPI initialization structure.
 */
void ECSPI_Init(ECSPI_Type* base, const ecspi_init_config_t* initConfig);

/*!
 * @brief Enables the specified eCSPI module.
 *
 * @param base eCSPI base pointer.
 */
static inline void ECSPI_Enable(ECSPI_Type* base)
{
    /* Enable the eCSPI. */
    ECSPI_CONREG_REG(base) |= ECSPI_CONREG_EN_MASK;
}

/*!
 * @brief Disable the specified eCSPI module.
 *
 * @param base eCSPI base pointer.
 */
static inline void ECSPI_Disable(ECSPI_Type* base)
{
    /* Enable the eCSPI. */
    ECSPI_CONREG_REG(base) &= ~ECSPI_CONREG_EN_MASK;
}

/*!
 * @brief Insert the number of wait states to be inserted in data transfers.
 *
 * @param base eCSPI base pointer.
 * @param number the number of wait states.
 */
static inline void ECSPI_InsertWaitState(ECSPI_Type* base, uint32_t number)
{
    /* Configure the number of wait states inserted. */
    ECSPI_PERIODREG_REG(base) = (ECSPI_PERIODREG_REG(base) & (~ECSPI_PERIODREG_SAMPLE_PERIOD_MASK)) |
                                ECSPI_PERIODREG_SAMPLE_PERIOD(number);
}

/*!
 * @brief Set the clock source for the sample period counter.
 *
 * @param base eCSPI base pointer.
 * @param source The clock source (see @ref _ecspi_sampleperiod_clocksource enumeration).
 */
void ECSPI_SetSampClockSource(ECSPI_Type* base, uint32_t source);

/*!
 * @brief Set the eCSPI clocks insert between the chip select active edge
 *        and the first eCSPI clock edge.
 *
 * @param base eCSPI base pointer.
 * @param delay The number of wait states.
 */
static inline void ECSPI_SetDelay(ECSPI_Type* base, uint32_t delay)
{
    /* Set the number of clocks insert. */
    ECSPI_PERIODREG_REG(base) = (ECSPI_PERIODREG_REG(base) & (~ECSPI_PERIODREG_CSD_CTL_MASK)) |
                                ECSPI_PERIODREG_CSD_CTL(delay);
}

/*!
 * @brief Set the inactive state of SCLK.
 *
 * @param base eCSPI base pointer.
 * @param channel eCSPI channel select (see @ref _ecspi_channel_select enumeration).
 * @param state SCLK inactive state (see @ref _ecspi_sclk_inactivestate enumeration).
 */
static inline void ECSPI_SetSCLKInactiveState(ECSPI_Type* base, uint32_t channel, uint32_t state)
{
    /* Configure the inactive state of SCLK. */
    ECSPI_CONFIGREG_REG(base) = (ECSPI_CONFIGREG_REG(base) & (~ECSPI_CONFIGREG_SCLK_CTL(1 << channel))) |
                                ECSPI_CONFIGREG_SCLK_CTL((state & 1) << channel);
}

/*!
 * @brief Set the inactive state of data line.
 *
 * @param base eCSPI base pointer.
 * @param channel eCSPI channel select (see @ref _ecspi_channel_select enumeration).
 * @param state Data line inactive state (see @ref _ecspi_dataline_inactivestate enumeration).
 */
static inline void ECSPI_SetDataInactiveState(ECSPI_Type* base, uint32_t channel, uint32_t state)
{
    /* Set the inactive state of Data Line. */
    ECSPI_CONFIGREG_REG(base) = (ECSPI_CONFIGREG_REG(base) & (~ECSPI_CONFIGREG_DATA_CTL(1 << channel))) |
                                ECSPI_CONFIGREG_DATA_CTL((state & 1) << channel);
}

/*!
 * @brief Trigger a burst.
 *
 * @param base eCSPI base pointer.
 */
static inline void ECSPI_StartBurst(ECSPI_Type* base)
{
    /* Start a burst. */
    ECSPI_CONREG_REG(base) |= ECSPI_CONREG_XCH_MASK;
}

/*!
 * @brief Set the burst length.
 *
 * @param base eCSPI base pointer.
 * @param length The value of burst length.
 */
static inline void ECSPI_SetBurstLength(ECSPI_Type* base, uint32_t length)
{
    /* Set the burst length according to length. */
    ECSPI_CONREG_REG(base) = (ECSPI_CONREG_REG(base) & (~ECSPI_CONREG_BURST_LENGTH_MASK)) |
                            ECSPI_CONREG_BURST_LENGTH(length);
}

/*!
 * @brief Set eCSPI SS Wave Form.
 *
 * @param base eCSPI base pointer.
 * @param channel eCSPI channel selected (see @ref _ecspi_channel_select enumeration).
 * @param ssMultiBurst For master mode, set true for multiple burst and false for one burst.
 *        For slave mode, set true to complete burst by SS signal edges and false to complete
 *        burst by number of bits received.
 */
static inline void ECSPI_SetSSMultipleBurst(ECSPI_Type* base, uint32_t channel, bool ssMultiBurst)
{
    /* Set the SS wave form. */
    ECSPI_CONFIGREG_REG(base) = (ECSPI_CONFIGREG_REG(base) & (~ECSPI_CONFIGREG_SS_CTL(1 << channel))) |
                                ECSPI_CONFIGREG_SS_CTL(ssMultiBurst << channel);
}

/*!
 * @brief Set eCSPI SS Polarity.
 *
 * @param base eCSPI base pointer.
 * @param channel eCSPI channel selected (see @ref _ecspi_channel_select enumeration).
 * @param polarity Set SS signal active logic (see @ref _ecspi_ss_polarity enumeration).
 */
static inline void ECSPI_SetSSPolarity(ECSPI_Type* base, uint32_t channel, uint32_t polarity)
{
    /* Set the SS polarity. */
    ECSPI_CONFIGREG_REG(base) = (ECSPI_CONFIGREG_REG(base) & (~ECSPI_CONFIGREG_SS_POL(1 << channel))) |
                                ECSPI_CONFIGREG_SS_POL(polarity << channel);
}

/*!
 * @brief Set the Data Ready Control.
 *
 * @param base eCSPI base pointer.
 * @param spidataready eCSPI data ready control (see @ref _ecspi_data_ready enumeration).
 */
static inline void ECSPI_SetSPIDataReady(ECSPI_Type* base, uint32_t spidataready)
{
    /* Set the Data Ready Control. */
    ECSPI_CONREG_REG(base) = (ECSPI_CONREG_REG(base) & (~ECSPI_CONREG_DRCTL_MASK)) |
                             ECSPI_CONREG_DRCTL(spidataready);
}

/*!
 * @brief Calculated the eCSPI baud rate in bits per second.
 *        The calculated baud rate must not exceed the desired baud rate.
 *
 * @param base eCSPI base pointer.
 * @param sourceClockInHz eCSPI Clock(SCLK) (in Hz).
 * @param bitsPerSec the value of Baud Rate.
 * @return The calculated baud rate in bits-per-second, the nearest possible
 *         baud rate without exceeding the desired baud rate.
 */
uint32_t ECSPI_SetBaudRate(ECSPI_Type* base, uint32_t sourceClockInHz, uint32_t bitsPerSec);

/*@}*/

/*!
 * @name Data transfers functions
 * @{
 */

/*!
  * @brief Transmits a data to TXFIFO.
  *
  * @param base eCSPI base pointer.
  * @param data Data to be transmitted.
  */
static inline void ECSPI_SendData(ECSPI_Type* base, uint32_t data)
{
    /* Write data to Transmit Data Register. */
    ECSPI_TXDATA_REG(base) = data;
}

/*!
  * @brief Receives a data from RXFIFO.
  *
  * @param base eCSPI base pointer.
  * @return The value of received data.
  */
static inline uint32_t ECSPI_ReceiveData(ECSPI_Type* base)
{
    /* Read data from Receive Data Register. */
    return ECSPI_RXDATA_REG(base);
}

/*!
  * @brief Read the number of words in the RXFIFO.
  *
  * @param base eCSPI base pointer.
  * @return The number of words in the RXFIFO.
  */
static inline uint32_t ECSPI_GetRxfifoCounter(ECSPI_Type* base)
{
    /* Get the number of words in the RXFIFO. */
    return ((ECSPI_TESTREG_REG(base) & ECSPI_TESTREG_RXCNT_MASK) >> ECSPI_TESTREG_RXCNT_SHIFT);
}

/*!
  * @brief Read the number of words in the TXFIFO.
  *
  * @param base eCSPI base pointer.
  * @return The number of words in the TXFIFO.
  */
static inline uint32_t ECSPI_GetTxfifoCounter(ECSPI_Type* base)
{
    /* Get the number of words in the RXFIFO. */
    return ((ECSPI_TESTREG_REG(base) & ECSPI_TESTREG_TXCNT_MASK) >> ECSPI_TESTREG_TXCNT_SHIFT);
}

/*@}*/

/*!
 * @name DMA management functions
 * @{
 */

/*!
 * @brief Enable or disable the specified DMA Source.
 *
 * @param base eCSPI base pointer.
 * @param source specifies DMA source (see @ref _ecspi_dma_source enumeration).
 * @param enable Enable/Disable specified DMA Source.
 *               - true: Enable specified DMA Source.
 *               - false: Disable specified DMA Source.
 */
void ECSPI_SetDMACmd(ECSPI_Type* base, uint32_t source, bool enable);

/*!
  * @brief Set the burst length of a DMA operation.
  *
  * @param base eCSPI base pointer.
  * @param length Specifies the burst length of a DMA operation.
  */
static inline void ECSPI_SetDMABurstLength(ECSPI_Type* base, uint32_t length)
{
    /* Configure the burst length of a DMA operation. */
    ECSPI_DMAREG_REG(base) = (ECSPI_DMAREG_REG(base) & (~ECSPI_DMAREG_RX_DMA_LENGTH_MASK)) |
                             ECSPI_DMAREG_RX_DMA_LENGTH(length);
}

/*!
  * @brief Set the RXFIFO or TXFIFO threshold.
  *
  * @param base eCSPI base pointer.
  * @param fifo Data transfer FIFO (see @ref _ecspi_fifothreshold enumeration).
  * @param threshold Threshold value.
  */
void ECSPI_SetFIFOThreshold(ECSPI_Type* base, uint32_t fifo, uint32_t threshold);

/*@}*/

/*!
 * @name Interrupts and flags management functions
 * @{
 */

/*!
 * @brief Enable or disable the specified eCSPI interrupts.
 *
 * @param base eCSPI base pointer.
 * @param flags eCSPI status flag mask (see @ref _ecspi_status_flag for bit definition).
 * @param enable Interrupt enable.
 *               - true: Enable specified eCSPI interrupts.
 *               - false: Disable specified eCSPI interrupts.
 */
void ECSPI_SetIntCmd(ECSPI_Type* base, uint32_t flags, bool enable);

/*!
 * @brief Checks whether the specified eCSPI flag is set or not.
 *
 * @param base eCSPI base pointer.
 * @param flags eCSPI status flag mask (see @ref _ecspi_status_flag for bit definition).
 * @return eCSPI status, each bit represents one status flag.
 */
static inline uint32_t ECSPI_GetStatusFlag(ECSPI_Type* base, uint32_t flags)
{
    /* return the vale of eCSPI status. */
    return ECSPI_STATREG_REG(base) & flags;
}

/*!
 * @brief Clear one or more eCSPI status flag.
 *
 * @param base eCSPI base pointer.
 * @param flags eCSPI status flag mask (see @ref _ecspi_status_flag for bit definition).
 */
static inline void ECSPI_ClearStatusFlag(ECSPI_Type* base, uint32_t flags)
{
    /* Write 1 to the status bit. */
    ECSPI_STATREG_REG(base) = flags;
}

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /*__ECSPI_H__*/

/*******************************************************************************
 * EOF
 ******************************************************************************/
