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

#ifndef _FSL_SAI_H_
#define _FSL_SAI_H_

#include "fsl_common.h"

/*!
 * @addtogroup sai
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_SAI_DRIVER_VERSION (MAKE_VERSION(2, 1, 1)) /*!< Version 2.1.1 */
/*@}*/

/*! @brief SAI return status*/
enum _sai_status_t
{
    kStatus_SAI_TxBusy = MAKE_STATUS(kStatusGroup_SAI, 0),    /*!< SAI Tx is busy. */
    kStatus_SAI_RxBusy = MAKE_STATUS(kStatusGroup_SAI, 1),    /*!< SAI Rx is busy. */
    kStatus_SAI_TxError = MAKE_STATUS(kStatusGroup_SAI, 2),   /*!< SAI Tx FIFO error. */
    kStatus_SAI_RxError = MAKE_STATUS(kStatusGroup_SAI, 3),   /*!< SAI Rx FIFO error. */
    kStatus_SAI_QueueFull = MAKE_STATUS(kStatusGroup_SAI, 4), /*!< SAI transfer queue is full. */
    kStatus_SAI_TxIdle = MAKE_STATUS(kStatusGroup_SAI, 5),    /*!< SAI Tx is idle */
    kStatus_SAI_RxIdle = MAKE_STATUS(kStatusGroup_SAI, 6)     /*!< SAI Rx is idle */
};

/*! @brief Define the SAI bus type */
typedef enum _sai_protocol
{
    kSAI_BusLeftJustified = 0x0U, /*!< Uses left justified format.*/
    kSAI_BusRightJustified,       /*!< Uses right justified format. */
    kSAI_BusI2S,                  /*!< Uses I2S format. */
    kSAI_BusPCMA,                 /*!< Uses I2S PCM A format.*/
    kSAI_BusPCMB                  /*!< Uses I2S PCM B format. */
} sai_protocol_t;

/*! @brief Master or slave mode */
typedef enum _sai_master_slave
{
    kSAI_Master = 0x0U, /*!< Master mode */
    kSAI_Slave = 0x1U   /*!< Slave mode */
} sai_master_slave_t;

/*! @brief Mono or stereo audio format */
typedef enum _sai_mono_stereo
{
    kSAI_Stereo = 0x0U, /*!< Stereo sound. */
    kSAI_MonoLeft,      /*!< Only left channel have sound. */
    kSAI_MonoRight      /*!< Only Right channel have sound. */
} sai_mono_stereo_t;

/*! @brief Synchronous or asynchronous mode */
typedef enum _sai_sync_mode
{
    kSAI_ModeAsync = 0x0U,    /*!< Asynchronous mode */
    kSAI_ModeSync,            /*!< Synchronous mode (with receiver or transmit) */
    kSAI_ModeSyncWithOtherTx, /*!< Synchronous with another SAI transmit */
    kSAI_ModeSyncWithOtherRx  /*!< Synchronous with another SAI receiver */
} sai_sync_mode_t;

/*! @brief Mater clock source */
typedef enum _sai_mclk_source
{
    kSAI_MclkSourceSysclk = 0x0U, /*!< Master clock from the system clock */
    kSAI_MclkSourceSelect1,       /*!< Master clock from source 1 */
    kSAI_MclkSourceSelect2,       /*!< Master clock from source 2 */
    kSAI_MclkSourceSelect3        /*!< Master clock from source 3 */
} sai_mclk_source_t;

/*! @brief Bit clock source */
typedef enum _sai_bclk_source
{
    kSAI_BclkSourceBusclk = 0x0U, /*!< Bit clock using bus clock */
    kSAI_BclkSourceMclkDiv,       /*!< Bit clock using master clock divider */
    kSAI_BclkSourceOtherSai0,     /*!< Bit clock from other SAI device  */
    kSAI_BclkSourceOtherSai1      /*!< Bit clock from other SAI device */
} sai_bclk_source_t;

/*! @brief The SAI interrupt enable flag */
enum _sai_interrupt_enable_t
{
    kSAI_WordStartInterruptEnable =
        I2S_TCSR_WSIE_MASK, /*!< Word start flag, means the first word in a frame detected */
    kSAI_SyncErrorInterruptEnable = I2S_TCSR_SEIE_MASK,   /*!< Sync error flag, means the sync error is detected */
    kSAI_FIFOWarningInterruptEnable = I2S_TCSR_FWIE_MASK, /*!< FIFO warning flag, means the FIFO is empty */
    kSAI_FIFOErrorInterruptEnable = I2S_TCSR_FEIE_MASK,   /*!< FIFO error flag */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    kSAI_FIFORequestInterruptEnable = I2S_TCSR_FRIE_MASK, /*!< FIFO request, means reached watermark */
#endif                                                    /* FSL_FEATURE_SAI_FIFO_COUNT */
};

/*! @brief The DMA request sources */
enum _sai_dma_enable_t
{
    kSAI_FIFOWarningDMAEnable = I2S_TCSR_FWDE_MASK, /*!< FIFO warning caused by the DMA request */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    kSAI_FIFORequestDMAEnable = I2S_TCSR_FRDE_MASK, /*!< FIFO request caused by the DMA request */
#endif                                              /* FSL_FEATURE_SAI_FIFO_COUNT */
};

/*! @brief The SAI status flag */
enum _sai_flags
{
    kSAI_WordStartFlag = I2S_TCSR_WSF_MASK, /*!< Word start flag, means the first word in a frame detected */
    kSAI_SyncErrorFlag = I2S_TCSR_SEF_MASK, /*!< Sync error flag, means the sync error is detected */
    kSAI_FIFOErrorFlag = I2S_TCSR_FEF_MASK, /*!< FIFO error flag */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    kSAI_FIFORequestFlag = I2S_TCSR_FRF_MASK, /*!< FIFO request flag. */
#endif                                        /* FSL_FEATURE_SAI_FIFO_COUNT */
    kSAI_FIFOWarningFlag = I2S_TCSR_FWF_MASK, /*!< FIFO warning flag */
};

/*! @brief The reset type */
typedef enum _sai_reset_type
{
    kSAI_ResetTypeSoftware = I2S_TCSR_SR_MASK,          /*!< Software reset, reset the logic state */
    kSAI_ResetTypeFIFO = I2S_TCSR_FR_MASK,              /*!< FIFO reset, reset the FIFO read and write pointer */
    kSAI_ResetAll = I2S_TCSR_SR_MASK | I2S_TCSR_FR_MASK /*!< All reset. */
} sai_reset_type_t;

#if defined(FSL_FEATURE_SAI_HAS_FIFO_PACKING) && FSL_FEATURE_SAI_HAS_FIFO_PACKING
/*!
 * @brief The SAI packing mode
 * The mode includes 8 bit and 16 bit packing.
 */
typedef enum _sai_fifo_packing
{
    kSAI_FifoPackingDisabled = 0x0U, /*!< Packing disabled */
    kSAI_FifoPacking8bit = 0x2U,     /*!< 8 bit packing enabled */
    kSAI_FifoPacking16bit = 0x3U     /*!< 16bit packing enabled */
} sai_fifo_packing_t;
#endif /* FSL_FEATURE_SAI_HAS_FIFO_PACKING */

/*! @brief SAI user configuration structure */
typedef struct _sai_config
{
    sai_protocol_t protocol;  /*!< Audio bus protocol in SAI */
    sai_sync_mode_t syncMode; /*!< SAI sync mode, control Tx/Rx clock sync */
#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
    bool mclkOutputEnable;          /*!< Master clock output enable, true means master clock divider enabled */
#endif                              /* FSL_FEATURE_SAI_HAS_MCR */
    sai_mclk_source_t mclkSource;   /*!< Master Clock source */
    sai_bclk_source_t bclkSource;   /*!< Bit Clock source */
    sai_master_slave_t masterSlave; /*!< Master or slave */
} sai_config_t;

/*!@brief SAI transfer queue size, user can refine it according to use case. */
#define SAI_XFER_QUEUE_SIZE (4)

/*! @brief Audio sample rate */
typedef enum _sai_sample_rate
{
    kSAI_SampleRate8KHz = 8000U,     /*!< Sample rate 8000 Hz */
    kSAI_SampleRate11025Hz = 11025U, /*!< Sample rate 11025 Hz */
    kSAI_SampleRate12KHz = 12000U,   /*!< Sample rate 12000 Hz */
    kSAI_SampleRate16KHz = 16000U,   /*!< Sample rate 16000 Hz */
    kSAI_SampleRate22050Hz = 22050U, /*!< Sample rate 22050 Hz */
    kSAI_SampleRate24KHz = 24000U,   /*!< Sample rate 24000 Hz */
    kSAI_SampleRate32KHz = 32000U,   /*!< Sample rate 32000 Hz */
    kSAI_SampleRate44100Hz = 44100U, /*!< Sample rate 44100 Hz */
    kSAI_SampleRate48KHz = 48000U,   /*!< Sample rate 48000 Hz */
    kSAI_SampleRate96KHz = 96000U    /*!< Sample rate 96000 Hz */
} sai_sample_rate_t;

/*! @brief Audio word width */
typedef enum _sai_word_width
{
    kSAI_WordWidth8bits = 8U,   /*!< Audio data width 8 bits */
    kSAI_WordWidth16bits = 16U, /*!< Audio data width 16 bits */
    kSAI_WordWidth24bits = 24U, /*!< Audio data width 24 bits */
    kSAI_WordWidth32bits = 32U  /*!< Audio data width 32 bits */
} sai_word_width_t;

/*! @brief sai transfer format */
typedef struct _sai_transfer_format
{
    uint32_t sampleRate_Hz;   /*!< Sample rate of audio data */
    uint32_t bitWidth;        /*!< Data length of audio data, usually 8/16/24/32 bits */
    sai_mono_stereo_t stereo; /*!< Mono or stereo */
    uint32_t masterClockHz;   /*!< Master clock frequency in Hz */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    uint8_t watermark;       /*!< Watermark value */
#endif                       /* FSL_FEATURE_SAI_FIFO_COUNT */
    uint8_t channel;         /*!< Data channel used in transfer.*/
    sai_protocol_t protocol; /*!< Which audio protocol used */
} sai_transfer_format_t;

/*! @brief SAI transfer structure */
typedef struct _sai_transfer
{
    uint8_t *data;   /*!< Data start address to transfer. */
    size_t dataSize; /*!< Transfer size. */
} sai_transfer_t;

typedef struct _sai_handle sai_handle_t;

/*! @brief SAI transfer callback prototype */
typedef void (*sai_transfer_callback_t)(I2S_Type *base, sai_handle_t *handle, status_t status, void *userData);

/*! @brief SAI handle structure */
struct _sai_handle
{
    uint32_t state;                               /*!< Transfer status */
    sai_transfer_callback_t callback;             /*!< Callback function called at transfer event*/
    void *userData;                               /*!< Callback parameter passed to callback function*/
    uint8_t bitWidth;                             /*!< Bit width for transfer, 8/16/24/32 bits */
    uint8_t channel;                              /*!< Transfer channel */
    sai_transfer_t saiQueue[SAI_XFER_QUEUE_SIZE]; /*!< Transfer queue storing queued transfer */
    size_t transferSize[SAI_XFER_QUEUE_SIZE];     /*!< Data bytes need to transfer */
    volatile uint8_t queueUser;                   /*!< Index for user to queue transfer */
    volatile uint8_t queueDriver;                 /*!< Index for driver to get the transfer data and size */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    uint8_t watermark; /*!< Watermark value */
#endif
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus*/

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Initializes the SAI Tx peripheral.
 *
 * Ungates the SAI clock, resets the module, and configures SAI Tx with a configuration structure.
 * The configuration structure can be custom filled or set with default values by
 * SAI_TxGetDefaultConfig().
 *
 * @note  This API should be called at the beginning of the application to use
 * the SAI driver. Otherwise, accessing the SAIM module can cause a hard fault
 * because the clock is not enabled.
 *
 * @param base SAI base pointer
 * @param config SAI configuration structure.
*/
void SAI_TxInit(I2S_Type *base, const sai_config_t *config);

/*!
 * @brief Initializes the the SAI Rx peripheral.
 *
 * Ungates the SAI clock, resets the module, and configures the SAI Rx with a configuration structure.
 * The configuration structure can be custom filled or set with default values by
 * SAI_RxGetDefaultConfig().
 *
 * @note  This API should be called at the beginning of the application to use
 * the SAI driver. Otherwise, accessing the SAI module can cause a hard fault
 * because the clock is not enabled.
 *
 * @param base SAI base pointer
 * @param config SAI configuration structure.
 */
void SAI_RxInit(I2S_Type *base, const sai_config_t *config);

/*!
 * @brief  Sets the SAI Tx configuration structure to default values.
 *
 * This API initializes the configuration structure for use in SAI_TxConfig().
 * The initialized structure can remain unchanged in SAI_TxConfig(), or it can be modified
 *  before calling SAI_TxConfig().
 * This is an example.
   @code
   sai_config_t config;
   SAI_TxGetDefaultConfig(&config);
   @endcode
 *
 * @param config pointer to master configuration structure
 */
void SAI_TxGetDefaultConfig(sai_config_t *config);

/*!
 * @brief  Sets the SAI Rx configuration structure to default values.
 *
 * This API initializes the configuration structure for use in SAI_RxConfig().
 * The initialized structure can remain unchanged in SAI_RxConfig() or it can be modified
 *  before calling SAI_RxConfig().
 * This is an example.
   @code
   sai_config_t config;
   SAI_RxGetDefaultConfig(&config);
   @endcode
 *
 * @param config pointer to master configuration structure
 */
void SAI_RxGetDefaultConfig(sai_config_t *config);

/*!
 * @brief De-initializes the SAI peripheral.
 *
 * This API gates the SAI clock. The SAI module can't operate unless SAI_TxInit
 * or SAI_RxInit is called to enable the clock.
 *
 * @param base SAI base pointer
*/
void SAI_Deinit(I2S_Type *base);

/*!
 * @brief Resets the SAI Tx.
 *
 * This function enables the software reset and FIFO reset of SAI Tx. After reset, clear the reset bit.
 *
 * @param base SAI base pointer
 */
void SAI_TxReset(I2S_Type *base);

/*!
 * @brief Resets the SAI Rx.
 *
 * This function enables the software reset and FIFO reset of SAI Rx. After reset, clear the reset bit.
 *
 * @param base SAI base pointer
 */
void SAI_RxReset(I2S_Type *base);

/*!
 * @brief Enables/disables the SAI Tx.
 *
 * @param base SAI base pointer
 * @param enable True means enable SAI Tx, false means disable.
 */
void SAI_TxEnable(I2S_Type *base, bool enable);

/*!
 * @brief Enables/disables the SAI Rx.
 *
 * @param base SAI base pointer
 * @param enable True means enable SAI Rx, false means disable.
 */
void SAI_RxEnable(I2S_Type *base, bool enable);

/*! @} */

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Gets the SAI Tx status flag state.
 *
 * @param base SAI base pointer
 * @return SAI Tx status flag value. Use the Status Mask to get the status value needed.
 */
static inline uint32_t SAI_TxGetStatusFlag(I2S_Type *base)
{
    return base->TCSR;
}

/*!
 * @brief Clears the SAI Tx status flag state.
 *
 * @param base SAI base pointer
 * @param mask State mask. It can be a combination of the following source if defined:
 *        @arg kSAI_WordStartFlag
 *        @arg kSAI_SyncErrorFlag
 *        @arg kSAI_FIFOErrorFlag
 */
static inline void SAI_TxClearStatusFlags(I2S_Type *base, uint32_t mask)
{
    base->TCSR = ((base->TCSR & 0xFFE3FFFFU) | mask);
}

/*!
 * @brief Gets the SAI Tx status flag state.
 *
 * @param base SAI base pointer
 * @return SAI Rx status flag value. Use the Status Mask to get the status value needed.
 */
static inline uint32_t SAI_RxGetStatusFlag(I2S_Type *base)
{
    return base->RCSR;
}

/*!
 * @brief Clears the SAI Rx status flag state.
 *
 * @param base SAI base pointer
 * @param mask State mask. It can be a combination of the following sources if defined.
 *        @arg kSAI_WordStartFlag
 *        @arg kSAI_SyncErrorFlag
 *        @arg kSAI_FIFOErrorFlag
 */
static inline void SAI_RxClearStatusFlags(I2S_Type *base, uint32_t mask)
{
    base->RCSR = ((base->RCSR & 0xFFE3FFFFU) | mask);
}

/*! @} */

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enables the SAI Tx interrupt requests.
 *
 * @param base SAI base pointer
 * @param mask interrupt source
 *     The parameter can be a combination of the following sources if defined.
 *     @arg kSAI_WordStartInterruptEnable
 *     @arg kSAI_SyncErrorInterruptEnable
 *     @arg kSAI_FIFOWarningInterruptEnable
 *     @arg kSAI_FIFORequestInterruptEnable
 *     @arg kSAI_FIFOErrorInterruptEnable
 */
static inline void SAI_TxEnableInterrupts(I2S_Type *base, uint32_t mask)
{
    base->TCSR = ((base->TCSR & 0xFFE3FFFFU) | mask);
}

/*!
 * @brief Enables the SAI Rx interrupt requests.
 *
 * @param base SAI base pointer
 * @param mask interrupt source
 *     The parameter can be a combination of the following sources if defined.
 *     @arg kSAI_WordStartInterruptEnable
 *     @arg kSAI_SyncErrorInterruptEnable
 *     @arg kSAI_FIFOWarningInterruptEnable
 *     @arg kSAI_FIFORequestInterruptEnable
 *     @arg kSAI_FIFOErrorInterruptEnable
 */
static inline void SAI_RxEnableInterrupts(I2S_Type *base, uint32_t mask)
{
    base->RCSR = ((base->RCSR & 0xFFE3FFFFU) | mask);
}

/*!
 * @brief Disables the SAI Tx interrupt requests.
 *
 * @param base SAI base pointer
 * @param mask interrupt source
 *     The parameter can be a combination of the following sources if defined.
 *     @arg kSAI_WordStartInterruptEnable
 *     @arg kSAI_SyncErrorInterruptEnable
 *     @arg kSAI_FIFOWarningInterruptEnable
 *     @arg kSAI_FIFORequestInterruptEnable
 *     @arg kSAI_FIFOErrorInterruptEnable
 */
static inline void SAI_TxDisableInterrupts(I2S_Type *base, uint32_t mask)
{
    base->TCSR = ((base->TCSR & 0xFFE3FFFFU) & (~mask));
}

/*!
 * @brief Disables the SAI Rx interrupt requests.
 *
 * @param base SAI base pointer
 * @param mask interrupt source
 *     The parameter can be a combination of the following sources if defined.
 *     @arg kSAI_WordStartInterruptEnable
 *     @arg kSAI_SyncErrorInterruptEnable
 *     @arg kSAI_FIFOWarningInterruptEnable
 *     @arg kSAI_FIFORequestInterruptEnable
 *     @arg kSAI_FIFOErrorInterruptEnable
 */
static inline void SAI_RxDisableInterrupts(I2S_Type *base, uint32_t mask)
{
    base->RCSR = ((base->RCSR & 0xFFE3FFFFU) & (~mask));
}

/*! @} */

/*!
 * @name DMA Control
 * @{
 */

/*!
 * @brief Enables/disables the SAI Tx DMA requests.
 * @param base SAI base pointer
 * @param mask DMA source
 *     The parameter can be combination of the following sources if defined.
 *     @arg kSAI_FIFOWarningDMAEnable
 *     @arg kSAI_FIFORequestDMAEnable
 * @param enable True means enable DMA, false means disable DMA.
 */
static inline void SAI_TxEnableDMA(I2S_Type *base, uint32_t mask, bool enable)
{
    if (enable)
    {
        base->TCSR = ((base->TCSR & 0xFFE3FFFFU) | mask);
    }
    else
    {
        base->TCSR = ((base->TCSR & 0xFFE3FFFFU) & (~mask));
    }
}

/*!
 * @brief Enables/disables the SAI Rx DMA requests.
 * @param base SAI base pointer
 * @param mask DMA source
 *     The parameter can be a combination of the following sources if defined.
 *     @arg kSAI_FIFOWarningDMAEnable
 *     @arg kSAI_FIFORequestDMAEnable
 * @param enable True means enable DMA, false means disable DMA.
 */
static inline void SAI_RxEnableDMA(I2S_Type *base, uint32_t mask, bool enable)
{
    if (enable)
    {
        base->RCSR = ((base->RCSR & 0xFFE3FFFFU) | mask);
    }
    else
    {
        base->RCSR = ((base->RCSR & 0xFFE3FFFFU) & (~mask));
    }
}

/*!
 * @brief  Gets the SAI Tx data register address.
 *
 * This API is used to provide a transfer address for the SAI DMA transfer configuration.
 *
 * @param base SAI base pointer.
 * @param channel Which data channel used.
 * @return data register address.
 */
static inline uint32_t SAI_TxGetDataRegisterAddress(I2S_Type *base, uint32_t channel)
{
    return (uint32_t)(&(base->TDR)[channel]);
}

/*!
 * @brief  Gets the SAI Rx data register address.
 *
 * This API is used to provide a transfer address for the SAI DMA transfer configuration.
 *
 * @param base SAI base pointer.
 * @param channel Which data channel used.
 * @return data register address.
 */
static inline uint32_t SAI_RxGetDataRegisterAddress(I2S_Type *base, uint32_t channel)
{
    return (uint32_t)(&(base->RDR)[channel]);
}

/*! @} */

/*!
 * @name Bus Operations
 * @{
 */

/*!
 * @brief Configures the SAI Tx audio format.
 *
 * The audio format can be changed at run-time. This function configures the sample rate and audio data
 * format to be transferred.
 *
 * @param base SAI base pointer.
 * @param format Pointer to the SAI audio data format structure.
 * @param mclkSourceClockHz SAI master clock source frequency in Hz.
 * @param bclkSourceClockHz SAI bit clock source frequency in Hz. If the bit clock source is a master
 * clock, this value should equal the masterClockHz.
*/
void SAI_TxSetFormat(I2S_Type *base,
                     sai_transfer_format_t *format,
                     uint32_t mclkSourceClockHz,
                     uint32_t bclkSourceClockHz);

/*!
 * @brief Configures the SAI Rx audio format.
 *
 * The audio format can be changed at run-time. This function configures the sample rate and audio data
 * format to be transferred.
 *
 * @param base SAI base pointer.
 * @param format Pointer to the SAI audio data format structure.
 * @param mclkSourceClockHz SAI master clock source frequency in Hz.
 * @param bclkSourceClockHz SAI bit clock source frequency in Hz. If the bit clock source is a master
 * clock, this value should equal the masterClockHz.
*/
void SAI_RxSetFormat(I2S_Type *base,
                     sai_transfer_format_t *format,
                     uint32_t mclkSourceClockHz,
                     uint32_t bclkSourceClockHz);

/*!
 * @brief Sends data using a blocking method.
 *
 * @note This function blocks by polling until data is ready to be sent.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be written.
 * @param size Bytes to be written.
 */
void SAI_WriteBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size);

/*!
 * @brief Writes data into SAI FIFO.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @param data Data needs to be written.
 */
static inline void SAI_WriteData(I2S_Type *base, uint32_t channel, uint32_t data)
{
    base->TDR[channel] = data;
}

/*!
 * @brief Receives data using a blocking method.
 *
 * @note This function blocks by polling until data is ready to be sent.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be read.
 * @param size Bytes to be read.
 */
void SAI_ReadBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size);

/*!
 * @brief Reads data from the SAI FIFO.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @return Data in SAI FIFO.
 */
static inline uint32_t SAI_ReadData(I2S_Type *base, uint32_t channel)
{
    return base->RDR[channel];
}

/*! @} */

/*!
 * @name Transactional
 * @{
 */

/*!
 * @brief Initializes the SAI Tx handle.
 *
 * This function initializes the Tx handle for the SAI Tx transactional APIs. Call
 * this function once to get the handle initialized.
 *
 * @param base SAI base pointer
 * @param handle SAI handle pointer.
 * @param callback Pointer to the user callback function.
 * @param userData User parameter passed to the callback function
 */
void SAI_TransferTxCreateHandle(I2S_Type *base, sai_handle_t *handle, sai_transfer_callback_t callback, void *userData);

/*!
 * @brief Initializes the SAI Rx handle.
 *
 * This function initializes the Rx handle for the SAI Rx transactional APIs. Call
 * this function once to get the handle initialized.
 *
 * @param base SAI base pointer.
 * @param handle SAI handle pointer.
 * @param callback Pointer to the user callback function.
 * @param userData User parameter passed to the callback function.
 */
void SAI_TransferRxCreateHandle(I2S_Type *base, sai_handle_t *handle, sai_transfer_callback_t callback, void *userData);

/*!
 * @brief Configures the SAI Tx audio format.
 *
 * The audio format can be changed at run-time. This function configures the sample rate and audio data
 * format to be transferred.
 *
 * @param base SAI base pointer.
 * @param handle SAI handle pointer.
 * @param format Pointer to the SAI audio data format structure.
 * @param mclkSourceClockHz SAI master clock source frequency in Hz.
 * @param bclkSourceClockHz SAI bit clock source frequency in Hz. If a bit clock source is a master
 * clock, this value should equal the masterClockHz in format.
 * @return Status of this function. Return value is the status_t.
*/
status_t SAI_TransferTxSetFormat(I2S_Type *base,
                                 sai_handle_t *handle,
                                 sai_transfer_format_t *format,
                                 uint32_t mclkSourceClockHz,
                                 uint32_t bclkSourceClockHz);

/*!
 * @brief Configures the SAI Rx audio format.
 *
 * The audio format can be changed at run-time. This function configures the sample rate and audio data
 * format to be transferred.
 *
 * @param base SAI base pointer.
 * @param handle SAI handle pointer.
 * @param format Pointer to the SAI audio data format structure.
 * @param mclkSourceClockHz SAI master clock source frequency in Hz.
 * @param bclkSourceClockHz SAI bit clock source frequency in Hz. If a bit clock source is a master
 * clock, this value should equal the masterClockHz in format.
 * @return Status of this function. Return value is one of status_t.
*/
status_t SAI_TransferRxSetFormat(I2S_Type *base,
                                 sai_handle_t *handle,
                                 sai_transfer_format_t *format,
                                 uint32_t mclkSourceClockHz,
                                 uint32_t bclkSourceClockHz);

/*!
 * @brief Performs an interrupt non-blocking send transfer on SAI.
 *
 * @note This API returns immediately after the transfer initiates.
 * Call the SAI_TxGetTransferStatusIRQ to poll the transfer status and check whether
 * the transfer is finished. If the return status is not kStatus_SAI_Busy, the transfer
 * is finished.
 *
 * @param base SAI base pointer.
 * @param handle Pointer to the sai_handle_t structure which stores the transfer state.
 * @param xfer Pointer to the sai_transfer_t structure.
 * @retval kStatus_Success Successfully started the data receive.
 * @retval kStatus_SAI_TxBusy Previous receive still not finished.
 * @retval kStatus_InvalidArgument The input parameter is invalid.
 */
status_t SAI_TransferSendNonBlocking(I2S_Type *base, sai_handle_t *handle, sai_transfer_t *xfer);

/*!
 * @brief Performs an interrupt non-blocking receive transfer on SAI.
 *
 * @note This API returns immediately after the transfer initiates.
 * Call the SAI_RxGetTransferStatusIRQ to poll the transfer status and check whether
 * the transfer is finished. If the return status is not kStatus_SAI_Busy, the transfer
 * is finished.
 *
 * @param base SAI base pointer
 * @param handle Pointer to the sai_handle_t structure which stores the transfer state.
 * @param xfer Pointer to the sai_transfer_t structure.
 * @retval kStatus_Success Successfully started the data receive.
 * @retval kStatus_SAI_RxBusy Previous receive still not finished.
 * @retval kStatus_InvalidArgument The input parameter is invalid.
 */
status_t SAI_TransferReceiveNonBlocking(I2S_Type *base, sai_handle_t *handle, sai_transfer_t *xfer);

/*!
 * @brief Gets a set byte count.
 *
 * @param base SAI base pointer.
 * @param handle Pointer to the sai_handle_t structure which stores the transfer state.
 * @param count Bytes count sent.
 * @retval kStatus_Success Succeed get the transfer count.
 * @retval kStatus_NoTransferInProgress There is not a non-blocking transaction currently in progress.
 */
status_t SAI_TransferGetSendCount(I2S_Type *base, sai_handle_t *handle, size_t *count);

/*!
 * @brief Gets a received byte count.
 *
 * @param base SAI base pointer.
 * @param handle Pointer to the sai_handle_t structure which stores the transfer state.
 * @param count Bytes count received.
 * @retval kStatus_Success Succeed get the transfer count.
 * @retval kStatus_NoTransferInProgress There is not a non-blocking transaction currently in progress.
 */
status_t SAI_TransferGetReceiveCount(I2S_Type *base, sai_handle_t *handle, size_t *count);

/*!
 * @brief Aborts the current send.
 *
 * @note This API can be called any time when an interrupt non-blocking transfer initiates
 * to abort the transfer early.
 *
 * @param base SAI base pointer.
 * @param handle Pointer to the sai_handle_t structure which stores the transfer state.
 */
void SAI_TransferAbortSend(I2S_Type *base, sai_handle_t *handle);

/*!
 * @brief Aborts the the current IRQ receive.
 *
 * @note This API can be called when an interrupt non-blocking transfer initiates
 * to abort the transfer early.
 *
 * @param base SAI base pointer
 * @param handle Pointer to the sai_handle_t structure which stores the transfer state.
 */
void SAI_TransferAbortReceive(I2S_Type *base, sai_handle_t *handle);

/*!
 * @brief Tx interrupt handler.
 *
 * @param base SAI base pointer.
 * @param handle Pointer to the sai_handle_t structure.
 */
void SAI_TransferTxHandleIRQ(I2S_Type *base, sai_handle_t *handle);

/*!
 * @brief Tx interrupt handler.
 *
 * @param base SAI base pointer.
 * @param handle Pointer to the sai_handle_t structure.
 */
void SAI_TransferRxHandleIRQ(I2S_Type *base, sai_handle_t *handle);

/*! @} */

#if defined(__cplusplus)
}
#endif /*_cplusplus*/

/*! @} */

#endif /* _FSL_SAI_H_ */
