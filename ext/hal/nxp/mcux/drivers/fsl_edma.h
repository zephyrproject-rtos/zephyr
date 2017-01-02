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

#ifndef _FSL_EDMA_H_
#define _FSL_EDMA_H_

#include "fsl_common.h"

/*!
 * @addtogroup edma_driver
 * @{
 */

/*! @file */
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief eDMA driver version */
#define FSL_EDMA_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0. */
/*@}*/

/*! @brief Compute the offset unit from DCHPRI3 */
#define DMA_DCHPRI_INDEX(channel) (((channel) & ~0x03U) | (3 - ((channel)&0x03U)))

/*! @brief Get the pointer of DCHPRIn */
#define DMA_DCHPRIn(base, channel) ((volatile uint8_t *)&(base->DCHPRI3))[DMA_DCHPRI_INDEX(channel)]

/*! @brief eDMA transfer configuration */
typedef enum _edma_transfer_size
{
    kEDMA_TransferSize1Bytes = 0x0U,  /*!< Source/Destination data transfer size is 1 byte every time */
    kEDMA_TransferSize2Bytes = 0x1U,  /*!< Source/Destination data transfer size is 2 bytes every time */
    kEDMA_TransferSize4Bytes = 0x2U,  /*!< Source/Destination data transfer size is 4 bytes every time */
    kEDMA_TransferSize16Bytes = 0x4U, /*!< Source/Destination data transfer size is 16 bytes every time */
    kEDMA_TransferSize32Bytes = 0x5U, /*!< Source/Destination data transfer size is 32 bytes every time */
} edma_transfer_size_t;

/*! @brief eDMA modulo configuration */
typedef enum _edma_modulo
{
    kEDMA_ModuloDisable = 0x0U, /*!< Disable modulo */
    kEDMA_Modulo2bytes,         /*!< Circular buffer size is 2 bytes. */
    kEDMA_Modulo4bytes,         /*!< Circular buffer size is 4 bytes. */
    kEDMA_Modulo8bytes,         /*!< Circular buffer size is 8 bytes. */
    kEDMA_Modulo16bytes,        /*!< Circular buffer size is 16 bytes. */
    kEDMA_Modulo32bytes,        /*!< Circular buffer size is 32 bytes. */
    kEDMA_Modulo64bytes,        /*!< Circular buffer size is 64 bytes. */
    kEDMA_Modulo128bytes,       /*!< Circular buffer size is 128 bytes. */
    kEDMA_Modulo256bytes,       /*!< Circular buffer size is 256 bytes. */
    kEDMA_Modulo512bytes,       /*!< Circular buffer size is 512 bytes. */
    kEDMA_Modulo1Kbytes,        /*!< Circular buffer size is 1K bytes. */
    kEDMA_Modulo2Kbytes,        /*!< Circular buffer size is 2K bytes. */
    kEDMA_Modulo4Kbytes,        /*!< Circular buffer size is 4K bytes. */
    kEDMA_Modulo8Kbytes,        /*!< Circular buffer size is 8K bytes. */
    kEDMA_Modulo16Kbytes,       /*!< Circular buffer size is 16K bytes. */
    kEDMA_Modulo32Kbytes,       /*!< Circular buffer size is 32K bytes. */
    kEDMA_Modulo64Kbytes,       /*!< Circular buffer size is 64K bytes. */
    kEDMA_Modulo128Kbytes,      /*!< Circular buffer size is 128K bytes. */
    kEDMA_Modulo256Kbytes,      /*!< Circular buffer size is 256K bytes. */
    kEDMA_Modulo512Kbytes,      /*!< Circular buffer size is 512K bytes. */
    kEDMA_Modulo1Mbytes,        /*!< Circular buffer size is 1M bytes. */
    kEDMA_Modulo2Mbytes,        /*!< Circular buffer size is 2M bytes. */
    kEDMA_Modulo4Mbytes,        /*!< Circular buffer size is 4M bytes. */
    kEDMA_Modulo8Mbytes,        /*!< Circular buffer size is 8M bytes. */
    kEDMA_Modulo16Mbytes,       /*!< Circular buffer size is 16M bytes. */
    kEDMA_Modulo32Mbytes,       /*!< Circular buffer size is 32M bytes. */
    kEDMA_Modulo64Mbytes,       /*!< Circular buffer size is 64M bytes. */
    kEDMA_Modulo128Mbytes,      /*!< Circular buffer size is 128M bytes. */
    kEDMA_Modulo256Mbytes,      /*!< Circular buffer size is 256M bytes. */
    kEDMA_Modulo512Mbytes,      /*!< Circular buffer size is 512M bytes. */
    kEDMA_Modulo1Gbytes,        /*!< Circular buffer size is 1G bytes. */
    kEDMA_Modulo2Gbytes,        /*!< Circular buffer size is 2G bytes. */
} edma_modulo_t;

/*! @brief Bandwidth control */
typedef enum _edma_bandwidth
{
    kEDMA_BandwidthStallNone = 0x0U,   /*!< No eDMA engine stalls. */
    kEDMA_BandwidthStall4Cycle = 0x2U, /*!< eDMA engine stalls for 4 cycles after each read/write. */
    kEDMA_BandwidthStall8Cycle = 0x3U, /*!< eDMA engine stalls for 8 cycles after each read/write. */
} edma_bandwidth_t;

/*! @brief Channel link type */
typedef enum _edma_channel_link_type
{
    kEDMA_LinkNone = 0x0U, /*!< No channel link  */
    kEDMA_MinorLink,       /*!< Channel link after each minor loop */
    kEDMA_MajorLink,       /*!< Channel link while major loop count exhausted */
} edma_channel_link_type_t;

/*!@brief eDMA channel status flags. */
enum _edma_channel_status_flags
{
    kEDMA_DoneFlag = 0x1U,      /*!< DONE flag, set while transfer finished, CITER value exhausted*/
    kEDMA_ErrorFlag = 0x2U,     /*!< eDMA error flag, an error occurred in a transfer */
    kEDMA_InterruptFlag = 0x4U, /*!< eDMA interrupt flag, set while an interrupt occurred of this channel */
};

/*! @brief eDMA channel error status flags. */
enum _edma_error_status_flags
{
    kEDMA_DestinationBusErrorFlag = DMA_ES_DBE_MASK,    /*!< Bus error on destination address */
    kEDMA_SourceBusErrorFlag = DMA_ES_SBE_MASK,         /*!< Bus error on the source address */
    kEDMA_ScatterGatherErrorFlag = DMA_ES_SGE_MASK,     /*!< Error on the Scatter/Gather address, not 32byte aligned. */
    kEDMA_NbytesErrorFlag = DMA_ES_NCE_MASK,            /*!< NBYTES/CITER configuration error */
    kEDMA_DestinationOffsetErrorFlag = DMA_ES_DOE_MASK, /*!< Destination offset not aligned with destination size */
    kEDMA_DestinationAddressErrorFlag = DMA_ES_DAE_MASK, /*!< Destination address not aligned with destination size */
    kEDMA_SourceOffsetErrorFlag = DMA_ES_SOE_MASK,       /*!< Source offset not aligned with source size */
    kEDMA_SourceAddressErrorFlag = DMA_ES_SAE_MASK,      /*!< Source address not aligned with source size*/
    kEDMA_ErrorChannelFlag = DMA_ES_ERRCHN_MASK,         /*!< Error channel number of the cancelled channel number */
    kEDMA_ChannelPriorityErrorFlag = DMA_ES_CPE_MASK,    /*!< Channel priority is not unique. */
    kEDMA_TransferCanceledFlag = DMA_ES_ECX_MASK,        /*!< Transfer cancelled */
#if defined(FSL_FEATURE_EDMA_CHANNEL_GROUP_COUNT) && FSL_FEATURE_EDMA_CHANNEL_GROUP_COUNT > 1
    kEDMA_GroupPriorityErrorFlag = DMA_ES_GPE_MASK, /*!< Group priority is not unique. */
#endif
    kEDMA_ValidFlag = DMA_ES_VLD_MASK, /*!< No error occurred, this bit will be 0, otherwise be 1 */
};

/*! @brief eDMA interrupt source */
typedef enum _edma_interrupt_enable
{
    kEDMA_ErrorInterruptEnable = 0x1U,                  /*!< Enable interrupt while channel error occurs. */
    kEDMA_MajorInterruptEnable = DMA_CSR_INTMAJOR_MASK, /*!< Enable interrupt while major count exhausted. */
    kEDMA_HalfInterruptEnable = DMA_CSR_INTHALF_MASK,   /*!< Enable interrupt while major count to half value. */
} edma_interrupt_enable_t;

/*! @brief eDMA transfer type */
typedef enum _edma_transfer_type
{
    kEDMA_MemoryToMemory = 0x0U, /*!< Transfer from memory to memory */
    kEDMA_PeripheralToMemory,    /*!< Transfer from peripheral to memory */
    kEDMA_MemoryToPeripheral,    /*!< Transfer from memory to peripheral */
} edma_transfer_type_t;

/*! @brief eDMA transfer status */
enum _edma_transfer_status
{
    kStatus_EDMA_QueueFull = MAKE_STATUS(kStatusGroup_EDMA, 0), /*!< TCD queue is full. */
    kStatus_EDMA_Busy = MAKE_STATUS(kStatusGroup_EDMA, 1),      /*!< Channel is busy and can't handle the
                                                                     transfer request. */
};

/*! @brief eDMA global configuration structure.*/
typedef struct _edma_config
{
    bool enableContinuousLinkMode;    /*!< Enable (true) continuous link mode. Upon minor loop completion, the channel
                                           activates again if that channel has a minor loop channel link enabled and
                                           the link channel is itself. */
    bool enableHaltOnError;           /*!< Enable (true) transfer halt on error. Any error causes the HALT bit to set.
                                           Subsequently, all service requests are ignored until the HALT bit is cleared.*/
    bool enableRoundRobinArbitration; /*!< Enable (true) round robin channel arbitration method, or fixed priority
                                           arbitration is used for channel selection */
    bool enableDebugMode; /*!< Enable(true) eDMA debug mode. When in debug mode, the eDMA stalls the start of
                               a new channel. Executing channels are allowed to complete. */
} edma_config_t;

/*!
 * @brief eDMA transfer configuration
 *
 * This structure configures the source/destination transfer attribute.
 * This figure shows the eDMA's transfer model:
 *  _________________________________________________
 *              | Transfer Size |                    |
 *   Minor Loop |_______________| Major loop Count 1 |
 *     Bytes    | Transfer Size |                    |
 *  ____________|_______________|____________________|--> Minor loop complete
 *               ____________________________________
 *              |               |                    |
 *              |_______________| Major Loop Count 2 |
 *              |               |                    |
 *              |_______________|____________________|--> Minor loop  Complete
 *
 *               ---------------------------------------------------------> Transfer complete
 */
typedef struct _edma_transfer_config
{
    uint32_t srcAddr;                      /*!< Source data address. */
    uint32_t destAddr;                     /*!< Destination data address. */
    edma_transfer_size_t srcTransferSize;  /*!< Source data transfer size. */
    edma_transfer_size_t destTransferSize; /*!< Destination data transfer size. */
    int16_t srcOffset;                     /*!< Sign-extended offset applied to the current source address to
                                                form the next-state value as each source read is completed. */
    int16_t destOffset;                    /*!< Sign-extended offset applied to the current destination address to
                                                form the next-state value as each destination write is completed. */
    uint16_t minorLoopBytes;               /*!< Bytes to transfer in a minor loop*/
    uint32_t majorLoopCounts;              /*!< Major loop iteration count. */
} edma_transfer_config_t;

/*! @brief eDMA channel priority configuration */
typedef struct _edma_channel_Preemption_config
{
    bool enableChannelPreemption; /*!< If true: channel can be suspended by other channel with higher priority */
    bool enablePreemptAbility;    /*!< If true: channel can suspend other channel with low priority */
    uint8_t channelPriority;      /*!< Channel priority */
} edma_channel_Preemption_config_t;

/*! @brief eDMA minor offset configuration */
typedef struct _edma_minor_offset_config
{
    bool enableSrcMinorOffset;  /*!< Enable(true) or Disable(false) source minor loop offset. */
    bool enableDestMinorOffset; /*!< Enable(true) or Disable(false) destination minor loop offset. */
    uint32_t minorOffset;       /*!< Offset for minor loop mapping. */
} edma_minor_offset_config_t;

/*!
 * @brief eDMA TCD.
 *
 * This structure is same as TCD register which is described in reference manual,
 * and is used to configure scatter/gather feature as a next hardware TCD.
 */
typedef struct _edma_tcd
{
    __IO uint32_t SADDR;     /*!< SADDR register, used to save source address */
    __IO uint16_t SOFF;      /*!< SOFF register, save offset bytes every transfer */
    __IO uint16_t ATTR;      /*!< ATTR register, source/destination transfer size and modulo */
    __IO uint32_t NBYTES;    /*!< Nbytes register, minor loop length in bytes */
    __IO uint32_t SLAST;     /*!< SLAST register */
    __IO uint32_t DADDR;     /*!< DADDR register, used for destination address */
    __IO uint16_t DOFF;      /*!< DOFF register, used for destination offset */
    __IO uint16_t CITER;     /*!< CITER register, current minor loop numbers, for unfinished minor loop.*/
    __IO uint32_t DLAST_SGA; /*!< DLASTSGA register, next stcd address used in scatter-gather mode */
    __IO uint16_t CSR;       /*!< CSR register, for TCD control status */
    __IO uint16_t BITER;     /*!< BITER register, begin minor loop count. */
} edma_tcd_t;

/*! @brief Callback for eDMA */
struct _edma_handle;

/*! @brief Define Callback function for eDMA. */
typedef void (*edma_callback)(struct _edma_handle *handle, void *userData, bool transferDone, uint32_t tcds);

/*! @brief eDMA transfer handle structure */
typedef struct _edma_handle
{
    edma_callback callback;  /*!< Callback function for major count exhausted. */
    void *userData;          /*!< Callback function parameter. */
    DMA_Type *base;          /*!< eDMA peripheral base address. */
    edma_tcd_t *tcdPool;     /*!< Pointer to memory stored TCDs. */
    uint8_t channel;         /*!< eDMA channel number. */
    volatile int8_t header;  /*!< The first TCD index. */
    volatile int8_t tail;    /*!< The last TCD index. */
    volatile int8_t tcdUsed; /*!< The number of used TCD slots. */
    volatile int8_t tcdSize; /*!< The total number of TCD slots in the queue. */
    uint8_t flags;           /*!< The status of the current channel. */
} edma_handle_t;

/*******************************************************************************
 * APIs
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name eDMA initialization and De-initialization
 * @{
 */

/*!
 * @brief Initializes eDMA peripheral.
 *
 * This function ungates the eDMA clock and configure eDMA peripheral according
 * to the configuration structure.
 *
 * @param base eDMA peripheral base address.
 * @param config Pointer to configuration structure, see "edma_config_t".
 * @note This function enable the minor loop map feature.
 */
void EDMA_Init(DMA_Type *base, const edma_config_t *config);

/*!
 * @brief Deinitializes eDMA peripheral.
 *
 * This function gates the eDMA clock.
 *
 * @param base eDMA peripheral base address.
 */
void EDMA_Deinit(DMA_Type *base);

/*!
 * @brief Gets the eDMA default configuration structure.
 *
 * This function sets the configuration structure to a default value.
 * The default configuration is set to the following value:
 * @code
 *   config.enableContinuousLinkMode = false;
 *   config.enableHaltOnError = true;
 *   config.enableRoundRobinArbitration = false;
 *   config.enableDebugMode = false;
 * @endcode
 *
 * @param config Pointer to eDMA configuration structure.
 */
void EDMA_GetDefaultConfig(edma_config_t *config);

/* @} */
/*!
 * @name eDMA Channel Operation
 * @{
 */

/*!
 * @brief Sets all TCD registers to a default value.
 *
 * This function sets TCD registers for this channel to default value.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @note This function must not be called while the channel transfer is on-going,
 *       or it will case unpredicated results.
 * @note This function will enable auto stop request feature.
 */
void EDMA_ResetChannel(DMA_Type *base, uint32_t channel);

/*!
 * @brief Configures the eDMA transfer attribute.
 *
 * This function configure the transfer attribute, including source address, destination address,
 * transfer size, address offset, and so on. It also configures the scatter gather feature if the
 * user supplies the TCD address.
 * Example:
 * @code
 *  edma_transfer_t config;
 *  edma_tcd_t tcd;
 *  config.srcAddr = ..;
 *  config.destAddr = ..;
 *  ...
 *  EDMA_SetTransferConfig(DMA0, channel, &config, &stcd);
 * @endcode
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param config Pointer to eDMA transfer configuration structure.
 * @param nextTcd Point to TCD structure. It can be NULL if user
 *                do not want to enable scatter/gather feature.
 * @note If nextTcd is not NULL, it means scatter gather feature will be enabled.
 *       And DREQ bit will be cleared in the previous transfer configuration which
 *       will be set in eDMA_ResetChannel.
 */
void EDMA_SetTransferConfig(DMA_Type *base,
                            uint32_t channel,
                            const edma_transfer_config_t *config,
                            edma_tcd_t *nextTcd);

/*!
 * @brief Configures the eDMA minor offset feature.
 *
 * Minor offset means signed-extended value added to source address or destination
 * address after each minor loop.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param config Pointer to Minor offset configuration structure.
 */
void EDMA_SetMinorOffsetConfig(DMA_Type *base, uint32_t channel, const edma_minor_offset_config_t *config);

/*!
 * @brief Configures the eDMA channel preemption feature.
 *
 * This function configures the channel preemption attribute and the priority of the channel.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number
 * @param config Pointer to channel preemption configuration structure.
 */
static inline void EDMA_SetChannelPreemptionConfig(DMA_Type *base,
                                                   uint32_t channel,
                                                   const edma_channel_Preemption_config_t *config)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(config != NULL);

    DMA_DCHPRIn(base, channel) =
        (DMA_DCHPRI0_DPA(!config->enablePreemptAbility) | DMA_DCHPRI0_ECP(config->enableChannelPreemption) |
         DMA_DCHPRI0_CHPRI(config->channelPriority));
}

/*!
 * @brief Sets the channel link for the eDMA transfer.
 *
 * This function configures  minor link or major link mode. The minor link means that the channel link is
 * triggered every time CITER decreases by 1. The major link means that the channel link is triggered when the CITER is exhausted.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param type Channel link type, it can be one of:
 *   @arg kEDMA_LinkNone
 *   @arg kEDMA_MinorLink
 *   @arg kEDMA_MajorLink
 * @param linkedChannel The linked channel number.
 * @note User should ensure that DONE flag is cleared before call this interface, or the configuration will be invalid.
 */
void EDMA_SetChannelLink(DMA_Type *base, uint32_t channel, edma_channel_link_type_t type, uint32_t linkedChannel);

/*!
 * @brief Sets the bandwidth for the eDMA transfer.
 *
 * In general, because the eDMA processes the minor loop, it continuously generates read/write sequences
 * until the minor count is exhausted. The bandwidth forces the eDMA to stall after the completion of
 * each read/write access to control the bus request bandwidth seen by the crossbar switch.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param bandWidth Bandwidth setting, it can be one of:
 *     @arg kEDMABandwidthStallNone
 *     @arg kEDMABandwidthStall4Cycle
 *     @arg kEDMABandwidthStall8Cycle
 */
void EDMA_SetBandWidth(DMA_Type *base, uint32_t channel, edma_bandwidth_t bandWidth);

/*!
 * @brief Sets the source modulo and destination modulo for eDMA transfer.
 *
 * This function defines a specific address range specified to be the value after (SADDR + SOFF)/(DADDR + DOFF)
 * calculation is performed or the original register value. It provides the ability to implement a circular data
 * queue easily.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param srcModulo Source modulo value.
 * @param destModulo Destination modulo value.
 */
void EDMA_SetModulo(DMA_Type *base, uint32_t channel, edma_modulo_t srcModulo, edma_modulo_t destModulo);

#if defined(FSL_FEATURE_EDMA_ASYNCHRO_REQUEST_CHANNEL_COUNT) && FSL_FEATURE_EDMA_ASYNCHRO_REQUEST_CHANNEL_COUNT
/*!
 * @brief Enables an async request for the eDMA transfer.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param enable The command for enable(ture) or disable(false).
 */
static inline void EDMA_EnableAsyncRequest(DMA_Type *base, uint32_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->EARS = (base->EARS & (~(1U << channel))) | ((uint32_t)enable << channel);
}
#endif /* FSL_FEATURE_EDMA_ASYNCHRO_REQUEST_CHANNEL_COUNT */

/*!
 * @brief Enables an auto stop request for the eDMA transfer.
 *
 * If enabling the auto stop request, the eDMA hardware automatically disables the hardware channel request.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param enable The command for enable (true) or disable (false).
 */
static inline void EDMA_EnableAutoStopRequest(DMA_Type *base, uint32_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->TCD[channel].CSR = (base->TCD[channel].CSR & (~DMA_CSR_DREQ_MASK)) | DMA_CSR_DREQ(enable);
}

/*!
 * @brief Enables the interrupt source for the eDMA transfer.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param mask The mask of interrupt source to be set. User need to use
 *             the defined edma_interrupt_enable_t type.
 */
void EDMA_EnableChannelInterrupts(DMA_Type *base, uint32_t channel, uint32_t mask);

/*!
 * @brief Disables the interrupt source for the eDMA transfer.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param mask The mask of interrupt source to be set. Use
 *             the defined edma_interrupt_enable_t type.
 */
void EDMA_DisableChannelInterrupts(DMA_Type *base, uint32_t channel, uint32_t mask);

/* @} */
/*!
 * @name eDMA TCD Operation
 * @{
 */

/*!
 * @brief Sets all fields to default values for the TCD structure.
 *
 * This function sets all fields for this TCD structure to default value.
 *
 * @param tcd Pointer to the TCD structure.
 * @note This function will enable auto stop request feature.
 */
void EDMA_TcdReset(edma_tcd_t *tcd);

/*!
 * @brief Configures the eDMA TCD transfer attribute.
 *
 * TCD is a transfer control descriptor. The content of the TCD is the same as hardware TCD registers.
 * STCD is used in scatter-gather mode.
 * This function configures the TCD transfer attribute, including source address, destination address,
 * transfer size, address offset, and so on. It also configures the scatter gather feature if the
 * user supplies the next TCD address.
 * Example:
 * @code
 *   edma_transfer_t config = {
 *   ...
 *   }
 *   edma_tcd_t tcd __aligned(32);
 *   edma_tcd_t nextTcd __aligned(32);
 *   EDMA_TcdSetTransferConfig(&tcd, &config, &nextTcd);
 * @endcode
 *
 * @param tcd Pointer to the TCD structure.
 * @param config Pointer to eDMA transfer configuration structure.
 * @param nextTcd Pointer to the next TCD structure. It can be NULL if user
 *                do not want to enable scatter/gather feature.
 * @note TCD address should be 32 bytes aligned, or it will cause eDMA error.
 * @note If nextTcd is not NULL, it means scatter gather feature will be enabled.
 *       And DREQ bit will be cleared in the previous transfer configuration which
 *       will be set in EDMA_TcdReset.
 */
void EDMA_TcdSetTransferConfig(edma_tcd_t *tcd, const edma_transfer_config_t *config, edma_tcd_t *nextTcd);

/*!
 * @brief Configures the eDMA TCD minor offset feature.
 *
 * Minor offset is a signed-extended value added to the source address or destination
 * address after each minor loop.
 *
 * @param tcd Point to the TCD structure.
 * @param config Pointer to Minor offset configuration structure.
 */
void EDMA_TcdSetMinorOffsetConfig(edma_tcd_t *tcd, const edma_minor_offset_config_t *config);

/*!
 * @brief Sets the channel link for eDMA TCD.
 *
 * This function configures either a minor link or a major link. The minor link means the channel link is
 * triggered every time CITER decreases by 1. The major link means that the channel link  is triggered when the CITER is exhausted.
 *
 * @note User should ensure that DONE flag is cleared before call this interface, or the configuration will be invalid.
 * @param tcd Point to the TCD structure.
 * @param type Channel link type, it can be one of:
 *   @arg kEDMA_LinkNone
 *   @arg kEDMA_MinorLink
 *   @arg kEDMA_MajorLink
 * @param linkedChannel The linked channel number.
 */
void EDMA_TcdSetChannelLink(edma_tcd_t *tcd, edma_channel_link_type_t type, uint32_t linkedChannel);

/*!
 * @brief Sets the bandwidth for the eDMA TCD.
 *
 * In general, because the eDMA processes the minor loop, it continuously generates read/write sequences
 * until the minor count is exhausted. Bandwidth forces the eDMA to stall after the completion of
 * each read/write access to control the bus request bandwidth seen by the crossbar switch.
 * @param tcd Point to the TCD structure.
 * @param bandWidth Bandwidth setting, it can be one of:
 *     @arg kEDMABandwidthStallNone
 *     @arg kEDMABandwidthStall4Cycle
 *     @arg kEDMABandwidthStall8Cycle
 */
static inline void EDMA_TcdSetBandWidth(edma_tcd_t *tcd, edma_bandwidth_t bandWidth)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0);

    tcd->CSR = (tcd->CSR & (~DMA_CSR_BWC_MASK)) | DMA_CSR_BWC(bandWidth);
}

/*!
 * @brief Sets the source modulo and destination modulo for eDMA TCD.
 *
 * This function defines a specific address range specified to be the value after (SADDR + SOFF)/(DADDR + DOFF)
 * calculation is performed or the original register value. It provides the ability to implement a circular data
 * queue easily.
 *
 * @param tcd Point to the TCD structure.
 * @param srcModulo Source modulo value.
 * @param destModulo Destination modulo value.
 */
void EDMA_TcdSetModulo(edma_tcd_t *tcd, edma_modulo_t srcModulo, edma_modulo_t destModulo);

/*!
 * @brief Sets the auto stop request for the eDMA TCD.
 *
 * If enabling the auto stop request, the eDMA hardware automatically disables the hardware channel request.
 *
 * @param tcd Point to the TCD structure.
 * @param enable The command for enable(ture) or disable(false).
 */
static inline void EDMA_TcdEnableAutoStopRequest(edma_tcd_t *tcd, bool enable)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0);

    tcd->CSR = (tcd->CSR & (~DMA_CSR_DREQ_MASK)) | DMA_CSR_DREQ(enable);
}

/*!
 * @brief Enables the interrupt source for the eDMA TCD.
 *
 * @param tcd Point to the TCD structure.
 * @param mask The mask of interrupt source to be set. User need to use
 *             the defined edma_interrupt_enable_t type.
 */
void EDMA_TcdEnableInterrupts(edma_tcd_t *tcd, uint32_t mask);

/*!
 * @brief Disables the interrupt source for the eDMA TCD.
 *
 * @param tcd Point to the TCD structure.
 * @param mask The mask of interrupt source to be set. User need to use
 *             the defined edma_interrupt_enable_t type.
 */
void EDMA_TcdDisableInterrupts(edma_tcd_t *tcd, uint32_t mask);

/*! @} */
/*!
 * @name eDMA Channel Transfer Operation
 * @{
 */

/*!
 * @brief Enables the eDMA hardware channel request.
 *
 * This function enables the hardware channel request.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 */
static inline void EDMA_EnableChannelRequest(DMA_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->SERQ = DMA_SERQ_SERQ(channel);
}

/*!
 * @brief Disables the eDMA hardware channel request.
 *
 * This function disables the hardware channel request.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 */
static inline void EDMA_DisableChannelRequest(DMA_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->CERQ = DMA_CERQ_CERQ(channel);
}

/*!
 * @brief Starts the eDMA transfer by software trigger.
 *
 * This function starts a minor loop transfer.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 */
static inline void EDMA_TriggerChannelStart(DMA_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->SSRT = DMA_SSRT_SSRT(channel);
}

/*! @} */
/*!
 * @name eDMA Channel Status Operation
 * @{
 */

/*!
 * @brief Gets the Remaining bytes from the eDMA current channel TCD.
 *
 * This function checks the TCD (Task Control Descriptor) status for a specified
 * eDMA channel and returns the the number of bytes that have not finished.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @return Bytes have not been transferred yet for the current TCD.
 * @note This function can only be used to get unfinished bytes of transfer without
 *       the next TCD, or it might be inaccuracy.
 */
uint32_t EDMA_GetRemainingBytes(DMA_Type *base, uint32_t channel);

/*!
 * @brief Gets the eDMA channel error status flags.
 *
 * @param base eDMA peripheral base address.
 * @return The mask of error status flags. User need to use the
 *         _edma_error_status_flags type to decode the return variables.
 */
static inline uint32_t EDMA_GetErrorStatusFlags(DMA_Type *base)
{
    return base->ES;
}

/*!
 * @brief Gets the eDMA channel status flags.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @return The mask of channel status flags. User need to use the
 *         _edma_channel_status_flags type to decode the return variables.
 */
uint32_t EDMA_GetChannelStatusFlags(DMA_Type *base, uint32_t channel);

/*!
 * @brief Clears the eDMA channel status flags.
 *
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 * @param mask The mask of channel status to be cleared. User need to use
 *             the defined _edma_channel_status_flags type.
 */
void EDMA_ClearChannelStatusFlags(DMA_Type *base, uint32_t channel, uint32_t mask);

/*! @} */
/*!
 * @name eDMA Transactional Operation
 */

/*!
 * @brief Creates the eDMA handle.
 *
 * This function is called if using transaction API for eDMA. This function
 * initializes the internal state of eDMA handle.
 *
 * @param handle eDMA handle pointer. The eDMA handle stores callback function and
 *               parameters.
 * @param base eDMA peripheral base address.
 * @param channel eDMA channel number.
 */
void EDMA_CreateHandle(edma_handle_t *handle, DMA_Type *base, uint32_t channel);

/*!
 * @brief Installs the TCDs memory pool into eDMA handle.
 *
 * This function is called after the EDMA_CreateHandle to use scatter/gather feature.
 *
 * @param handle eDMA handle pointer.
 * @param tcdPool Memory pool to store TCDs. It must be 32 bytes aligned.
 * @param tcdSize The number of TCD slots.
 */
void EDMA_InstallTCDMemory(edma_handle_t *handle, edma_tcd_t *tcdPool, uint32_t tcdSize);

/*!
 * @brief Installs a callback function for the eDMA transfer.
 *
 * This callback is called in eDMA IRQ handler. Use the callback to do something after
 * the current major loop transfer completes.
 *
 * @param handle eDMA handle pointer.
 * @param callback eDMA callback function pointer.
 * @param userData Parameter for callback function.
 */
void EDMA_SetCallback(edma_handle_t *handle, edma_callback callback, void *userData);

/*!
 * @brief Prepares the eDMA transfer structure.
 *
 * This function prepares the transfer configuration structure according to the user input.
 *
 * @param config The user configuration structure of type edma_transfer_t.
 * @param srcAddr eDMA transfer source address.
 * @param srcWidth eDMA transfer source address width(bytes).
 * @param destAddr eDMA transfer destination address.
 * @param destWidth eDMA transfer destination address width(bytes).
 * @param bytesEachRequest eDMA transfer bytes per channel request.
 * @param transferBytes eDMA transfer bytes to be transferred.
 * @param type eDMA transfer type.
 * @note The data address and the data width must be consistent. For example, if the SRC
 *       is 4 bytes, so the source address must be 4 bytes aligned, or it shall result in
 *       source address error(SAE).
 */
void EDMA_PrepareTransfer(edma_transfer_config_t *config,
                          void *srcAddr,
                          uint32_t srcWidth,
                          void *destAddr,
                          uint32_t destWidth,
                          uint32_t bytesEachRequest,
                          uint32_t transferBytes,
                          edma_transfer_type_t type);

/*!
 * @brief Submits the eDMA transfer request.
 *
 * This function submits the eDMA transfer request according to the transfer configuration structure.
 * If the user submits the transfer request repeatedly, this function packs an unprocessed request as
 * a TCD and enables scatter/gather feature to process it in the next time.
 *
 * @param handle eDMA handle pointer.
 * @param config Pointer to eDMA transfer configuration structure.
 * @retval kStatus_EDMA_Success It means submit transfer request succeed.
 * @retval kStatus_EDMA_QueueFull It means TCD queue is full. Submit transfer request is not allowed.
 * @retval kStatus_EDMA_Busy It means the given channel is busy, need to submit request later.
 */
status_t EDMA_SubmitTransfer(edma_handle_t *handle, const edma_transfer_config_t *config);

/*!
 * @brief eDMA start transfer.
 *
 * This function enables the channel request. User can call this function after submitting the transfer request
 * or before submitting the transfer request.
 *
 * @param handle eDMA handle pointer.
 */
void EDMA_StartTransfer(edma_handle_t *handle);

/*!
 * @brief eDMA stop transfer.
 *
 * This function disables the channel request to pause the transfer. User can call EDMA_StartTransfer()
 * again to resume the transfer.
 *
 * @param handle eDMA handle pointer.
 */
void EDMA_StopTransfer(edma_handle_t *handle);

/*!
 * @brief eDMA abort transfer.
 *
 * This function disables the channel request and clear transfer status bits.
 * User can submit another transfer after calling this API.
 *
 * @param handle DMA handle pointer.
 */
void EDMA_AbortTransfer(edma_handle_t *handle);

/*!
 * @brief eDMA IRQ handler for current major loop transfer complete.
 *
 * This function clears the channel major interrupt flag and call
 * the callback function if it is not NULL.
 *
 * @param handle eDMA handle pointer.
 */
void EDMA_HandleIRQ(edma_handle_t *handle);

/* @} */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/* @} */

#endif /*_FSL_EDMA_H_*/
