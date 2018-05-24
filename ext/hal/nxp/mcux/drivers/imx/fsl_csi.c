/*
 * The Clear BSD License
 * Copyright (c) 2017, NXP Semiconductors, Inc.
 * All rights reserved.
 *
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
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

#include "fsl_csi.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Two frame buffer loaded to CSI register at most. */
#define CSI_MAX_ACTIVE_FRAME_NUM 2

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the instance from the base address
 *
 * @param base CSI peripheral base address
 *
 * @return The CSI module instance
 */
static uint32_t CSI_GetInstance(CSI_Type *base);

/*!
 * @brief Get the delta value of two index in queue.
 *
 * @param startIdx Start index.
 * @param endIdx End index.
 *
 * @return The delta between startIdx and endIdx in queue.
 */
static uint32_t CSI_TransferGetQueueDelta(uint32_t startIdx, uint32_t endIdx);

/*!
 * @brief Increase a index value in queue.
 *
 * This function increases the index value in the queue, if the index is out of
 * the queue range, it is reset to 0.
 *
 * @param idx The index value to increase.
 *
 * @return The index value after increase.
 */
static uint32_t CSI_TransferIncreaseQueueIdx(uint32_t idx);

/*!
 * @brief Get the empty frame buffer count in queue.
 *
 * @param base CSI peripheral base address
 * @param handle Pointer to CSI driver handle.
 *
 * @return Number of the empty frame buffer count in queue.
 */
static uint32_t CSI_TransferGetEmptyBufferCount(CSI_Type *base, csi_handle_t *handle);

/*!
 * @brief Load one empty frame buffer in queue to CSI module.
 *
 * Load one empty frame in queue to CSI module, this function could only be called
 * when there is empty frame buffer in queue.
 *
 * @param base CSI peripheral base address
 * @param handle Pointer to CSI driver handle.
 */
static void CSI_TransferLoadBufferToDevice(CSI_Type *base, csi_handle_t *handle);

/* Typedef for interrupt handler. */
typedef void (*csi_isr_t)(CSI_Type *base, csi_handle_t *handle);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to CSI bases for each instance. */
static CSI_Type *const s_csiBases[] = CSI_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to CSI clocks for each CSI submodule. */
static const clock_ip_name_t s_csiClocks[] = CSI_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/* Array for the CSI driver handle. */
static csi_handle_t *s_csiHandle[ARRAY_SIZE(s_csiBases)];

/* Array of CSI IRQ number. */
static const IRQn_Type s_csiIRQ[] = CSI_IRQS;

/* CSI ISR for transactional APIs. */
static csi_isr_t s_csiIsr;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t CSI_GetInstance(CSI_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_csiBases); instance++)
    {
        if (s_csiBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_csiBases));

    return instance;
}

static uint32_t CSI_TransferGetQueueDelta(uint32_t startIdx, uint32_t endIdx)
{
    if (endIdx >= startIdx)
    {
        return endIdx - startIdx;
    }
    else
    {
        return startIdx + CSI_DRIVER_ACTUAL_QUEUE_SIZE - endIdx;
    }
}

static uint32_t CSI_TransferIncreaseQueueIdx(uint32_t idx)
{
    uint32_t ret;

    /*
     * Here not use the method:
     * ret = (idx+1) % CSI_DRIVER_ACTUAL_QUEUE_SIZE;
     *
     * Because the mod function might be slow.
     */

    ret = idx + 1;

    if (ret >= CSI_DRIVER_ACTUAL_QUEUE_SIZE)
    {
        ret = 0;
    }

    return ret;
}

static uint32_t CSI_TransferGetEmptyBufferCount(CSI_Type *base, csi_handle_t *handle)
{
    return CSI_TransferGetQueueDelta(handle->queueDrvReadIdx, handle->queueUserWriteIdx);
}

static void CSI_TransferLoadBufferToDevice(CSI_Type *base, csi_handle_t *handle)
{
    /* Load the frame buffer address to CSI register. */
    CSI_SetRxBufferAddr(base, handle->nextBufferIdx, handle->frameBufferQueue[handle->queueDrvReadIdx]);

    handle->queueDrvReadIdx = CSI_TransferIncreaseQueueIdx(handle->queueDrvReadIdx);
    handle->activeBufferNum++;

    /* There are two CSI buffers, so could use XOR to get the next index. */
    handle->nextBufferIdx ^= 1U;
}

status_t CSI_Init(CSI_Type *base, const csi_config_t *config)
{
    assert(config);
    uint32_t reg;
    uint32_t imgWidth_Bytes;

    imgWidth_Bytes = config->width * config->bytesPerPixel;

    /* The image width and frame buffer pitch should be multiple of 8-bytes. */
    if ((imgWidth_Bytes & 0x07) | ((uint32_t)config->linePitch_Bytes & 0x07))
    {
        return kStatus_InvalidArgument;
    }

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    uint32_t instance = CSI_GetInstance(base);
    CLOCK_EnableClock(s_csiClocks[instance]);
#endif

    CSI_Reset(base);

    /* Configure CSICR1. CSICR1 has been reset to the default value, so could write it directly. */
    reg = ((uint32_t)config->workMode) | config->polarityFlags | CSI_CSICR1_FCC_MASK;

    if (config->useExtVsync)
    {
        reg |= CSI_CSICR1_EXT_VSYNC_MASK;
    }

    base->CSICR1 = reg;

    /*
     * Generally, CSIIMAG_PARA[IMAGE_WIDTH] indicates how many data bus cycles per line.
     * One special case is when receiving 24-bit pixels through 8-bit data bus, and
     * CSICR3[ZERO_PACK_EN] is enabled, in this case, the CSIIMAG_PARA[IMAGE_WIDTH]
     * should be set to the pixel number per line.
     *
     * Currently the CSI driver only support 8-bit data bus, so generally the
     * CSIIMAG_PARA[IMAGE_WIDTH] is bytes number per line. When the CSICR3[ZERO_PACK_EN]
     * is enabled, CSIIMAG_PARA[IMAGE_WIDTH] is pixel number per line.
     *
     * NOTE: The CSIIMAG_PARA[IMAGE_WIDTH] setting code should be updated if the
     * driver is upgraded to support other data bus width.
     */
    if (4U == config->bytesPerPixel)
    {
        /* Enable zero pack. */
        base->CSICR3 |= CSI_CSICR3_ZERO_PACK_EN_MASK;
        /* Image parameter. */
        base->CSIIMAG_PARA = ((uint32_t)(config->width) << CSI_CSIIMAG_PARA_IMAGE_WIDTH_SHIFT) |
                             ((uint32_t)(config->height) << CSI_CSIIMAG_PARA_IMAGE_HEIGHT_SHIFT);
    }
    else
    {
        /* Image parameter. */
        base->CSIIMAG_PARA = ((uint32_t)(imgWidth_Bytes) << CSI_CSIIMAG_PARA_IMAGE_WIDTH_SHIFT) |
                             ((uint32_t)(config->height) << CSI_CSIIMAG_PARA_IMAGE_HEIGHT_SHIFT);
    }

    /* The CSI frame buffer bus is 8-byte width. */
    base->CSIFBUF_PARA = (uint32_t)((config->linePitch_Bytes - imgWidth_Bytes) / 8U)
                         << CSI_CSIFBUF_PARA_FBUF_STRIDE_SHIFT;

    /* Enable auto ECC. */
    base->CSICR3 |= CSI_CSICR3_ECC_AUTO_EN_MASK;

    /*
     * For better performance.
     * The DMA burst size could be set to 16 * 8 byte, 8 * 8 byte, or 4 * 8 byte,
     * choose the best burst size based on bytes per line.
     */
    if (!(imgWidth_Bytes % (8 * 16)))
    {
        base->CSICR2 = CSI_CSICR2_DMA_BURST_TYPE_RFF(3U);
        base->CSICR3 = (CSI->CSICR3 & ~CSI_CSICR3_RxFF_LEVEL_MASK) | ((2U << CSI_CSICR3_RxFF_LEVEL_SHIFT));
    }
    else if (!(imgWidth_Bytes % (8 * 8)))
    {
        base->CSICR2 = CSI_CSICR2_DMA_BURST_TYPE_RFF(2U);
        base->CSICR3 = (CSI->CSICR3 & ~CSI_CSICR3_RxFF_LEVEL_MASK) | ((1U << CSI_CSICR3_RxFF_LEVEL_SHIFT));
    }
    else
    {
        base->CSICR2 = CSI_CSICR2_DMA_BURST_TYPE_RFF(1U);
        base->CSICR3 = (CSI->CSICR3 & ~CSI_CSICR3_RxFF_LEVEL_MASK) | ((0U << CSI_CSICR3_RxFF_LEVEL_SHIFT));
    }

    CSI_ReflashFifoDma(base, kCSI_RxFifo);

    return kStatus_Success;
}

void CSI_Deinit(CSI_Type *base)
{
    /* Disable transfer first. */
    CSI_Stop(base);
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    uint32_t instance = CSI_GetInstance(base);
    CLOCK_DisableClock(s_csiClocks[instance]);
#endif
}

void CSI_Reset(CSI_Type *base)
{
    uint32_t csisr;

    /* Disable transfer first. */
    CSI_Stop(base);

    /* Disable DMA request. */
    base->CSICR3 = 0U;

    /* Reset the fame count. */
    base->CSICR3 |= CSI_CSICR3_FRMCNT_RST_MASK;
    while (base->CSICR3 & CSI_CSICR3_FRMCNT_RST_MASK)
    {
    }

    /* Clear the RX FIFO. */
    CSI_ClearFifo(base, kCSI_AllFifo);

    /* Reflash DMA. */
    CSI_ReflashFifoDma(base, kCSI_AllFifo);

    /* Clear the status. */
    csisr = base->CSISR;
    base->CSISR = csisr;

    /* Set the control registers to default value. */
    base->CSICR1 = CSI_CSICR1_HSYNC_POL_MASK | CSI_CSICR1_EXT_VSYNC_MASK;
    base->CSICR2 = 0U;
    base->CSICR3 = 0U;
#if defined(CSI_CSICR18_CSI_LCDIF_BUFFER_LINES)
    base->CSICR18 = CSI_CSICR18_AHB_HPROT(0x0DU) | CSI_CSICR18_CSI_LCDIF_BUFFER_LINES(0x02U);
#else
    base->CSICR18 = CSI_CSICR18_AHB_HPROT(0x0DU);
#endif
    base->CSIFBUF_PARA = 0U;
    base->CSIIMAG_PARA = 0U;
}

void CSI_GetDefaultConfig(csi_config_t *config)
{
    assert(config);

    config->width = 320U;
    config->height = 240U;
    config->polarityFlags = kCSI_HsyncActiveHigh | kCSI_DataLatchOnRisingEdge;
    config->bytesPerPixel = 2U;
    config->linePitch_Bytes = 320U * 2U;
    config->workMode = kCSI_GatedClockMode;
    config->dataBus = kCSI_DataBus8Bit;
    config->useExtVsync = true;
}

void CSI_SetRxBufferAddr(CSI_Type *base, uint8_t index, uint32_t addr)
{
    if (index)
    {
        base->CSIDMASA_FB2 = addr;
    }
    else
    {
        base->CSIDMASA_FB1 = addr;
    }
}

void CSI_ClearFifo(CSI_Type *base, csi_fifo_t fifo)
{
    uint32_t cr1;
    uint32_t mask = 0U;

    /* The FIFO could only be cleared when CSICR1[FCC] = 0, so first clear the FCC. */
    cr1 = base->CSICR1;
    base->CSICR1 = (cr1 & ~CSI_CSICR1_FCC_MASK);

    if ((uint32_t)fifo & (uint32_t)kCSI_RxFifo)
    {
        mask |= CSI_CSICR1_CLR_RXFIFO_MASK;
    }

    if ((uint32_t)fifo & (uint32_t)kCSI_StatFifo)
    {
        mask |= CSI_CSICR1_CLR_STATFIFO_MASK;
    }

    base->CSICR1 = (cr1 & ~CSI_CSICR1_FCC_MASK) | mask;

    /* Wait clear completed. */
    while (base->CSICR1 & mask)
    {
    }

    /* Recover the FCC. */
    base->CSICR1 = cr1;
}

void CSI_ReflashFifoDma(CSI_Type *base, csi_fifo_t fifo)
{
    uint32_t cr3 = 0U;

    if ((uint32_t)fifo & (uint32_t)kCSI_RxFifo)
    {
        cr3 |= CSI_CSICR3_DMA_REFLASH_RFF_MASK;
    }

    if ((uint32_t)fifo & (uint32_t)kCSI_StatFifo)
    {
        cr3 |= CSI_CSICR3_DMA_REFLASH_SFF_MASK;
    }

    base->CSICR3 |= cr3;

    /* Wait clear completed. */
    while (base->CSICR3 & cr3)
    {
    }
}

void CSI_EnableFifoDmaRequest(CSI_Type *base, csi_fifo_t fifo, bool enable)
{
    uint32_t cr3 = 0U;

    if ((uint32_t)fifo & (uint32_t)kCSI_RxFifo)
    {
        cr3 |= CSI_CSICR3_DMA_REQ_EN_RFF_MASK;
    }

    if ((uint32_t)fifo & (uint32_t)kCSI_StatFifo)
    {
        cr3 |= CSI_CSICR3_DMA_REQ_EN_SFF_MASK;
    }

    if (enable)
    {
        base->CSICR3 |= cr3;
    }
    else
    {
        base->CSICR3 &= ~cr3;
    }
}

void CSI_EnableInterrupts(CSI_Type *base, uint32_t mask)
{
    base->CSICR1 |= (mask & CSI_CSICR1_INT_EN_MASK);
    base->CSICR3 |= (mask & CSI_CSICR3_INT_EN_MASK);
    base->CSICR18 |= ((mask & CSI_CSICR18_INT_EN_MASK) >> 6U);
}

void CSI_DisableInterrupts(CSI_Type *base, uint32_t mask)
{
    base->CSICR1 &= ~(mask & CSI_CSICR1_INT_EN_MASK);
    base->CSICR3 &= ~(mask & CSI_CSICR3_INT_EN_MASK);
    base->CSICR18 &= ~((mask & CSI_CSICR18_INT_EN_MASK) >> 6U);
}

status_t CSI_TransferCreateHandle(CSI_Type *base,
                                  csi_handle_t *handle,
                                  csi_transfer_callback_t callback,
                                  void *userData)
{
    assert(handle);
    uint32_t instance;

    memset(handle, 0, sizeof(*handle));

    /* Set the callback and user data. */
    handle->callback = callback;
    handle->userData = userData;

    /* Get instance from peripheral base address. */
    instance = CSI_GetInstance(base);

    /* Save the handle in global variables to support the double weak mechanism. */
    s_csiHandle[instance] = handle;

    s_csiIsr = CSI_TransferHandleIRQ;

    /* Enable interrupt. */
    EnableIRQ(s_csiIRQ[instance]);

    return kStatus_Success;
}

status_t CSI_TransferStart(CSI_Type *base, csi_handle_t *handle)
{
    assert(handle);

    uint32_t emptyBufferCount;

    emptyBufferCount = CSI_TransferGetEmptyBufferCount(base, handle);

    if (emptyBufferCount < 2U)
    {
        return kStatus_CSI_NoEmptyBuffer;
    }

    handle->nextBufferIdx = 0U;
    handle->activeBufferNum = 0U;

    /* Write to memory from second completed frame. */
    base->CSICR18 = (base->CSICR18 & ~CSI_CSICR18_MASK_OPTION_MASK) | CSI_CSICR18_MASK_OPTION(2);

    /* Load the frame buffer to CSI register, there are at least two empty buffers. */
    CSI_TransferLoadBufferToDevice(base, handle);
    CSI_TransferLoadBufferToDevice(base, handle);

    /* After reflash DMA, the CSI saves frame to frame buffer 0. */
    CSI_ReflashFifoDma(base, kCSI_RxFifo);

    handle->transferStarted = true;
    handle->transferOnGoing = true;

    CSI_EnableInterrupts(base, kCSI_RxBuffer1DmaDoneInterruptEnable | kCSI_RxBuffer0DmaDoneInterruptEnable);

    CSI_Start(base);

    return kStatus_Success;
}

status_t CSI_TransferStop(CSI_Type *base, csi_handle_t *handle)
{
    assert(handle);

    CSI_Stop(base);
    CSI_DisableInterrupts(base, kCSI_RxBuffer1DmaDoneInterruptEnable | kCSI_RxBuffer0DmaDoneInterruptEnable);

    handle->transferStarted = false;
    handle->transferOnGoing = false;

    /* Stoped, reset the state flags. */
    handle->queueDrvReadIdx = handle->queueDrvWriteIdx;
    handle->activeBufferNum = 0U;

    return kStatus_Success;
}

status_t CSI_TransferSubmitEmptyBuffer(CSI_Type *base, csi_handle_t *handle, uint32_t frameBuffer)
{
    uint32_t csicr1;

    if (CSI_DRIVER_QUEUE_SIZE == CSI_TransferGetQueueDelta(handle->queueUserReadIdx, handle->queueUserWriteIdx))
    {
        return kStatus_CSI_QueueFull;
    }

    /* Disable the interrupt to protect the index information in handle. */
    csicr1 = base->CSICR1;

    base->CSICR1 = (csicr1 & ~(CSI_CSICR1_FB2_DMA_DONE_INTEN_MASK | CSI_CSICR1_FB1_DMA_DONE_INTEN_MASK));

    /* Save the empty frame buffer address to queue. */
    handle->frameBufferQueue[handle->queueUserWriteIdx] = frameBuffer;
    handle->queueUserWriteIdx = CSI_TransferIncreaseQueueIdx(handle->queueUserWriteIdx);

    base->CSICR1 = csicr1;

    if (handle->transferStarted)
    {
        /*
         * If user has started transfer using @ref CSI_TransferStart, and the CSI is
         * stopped due to no empty frame buffer in queue, then start the CSI.
         */
        if ((!handle->transferOnGoing) && (CSI_TransferGetEmptyBufferCount(base, handle) >= 2U))
        {
            handle->transferOnGoing = true;
            handle->nextBufferIdx = 0U;

            /* Load the frame buffers to CSI module. */
            CSI_TransferLoadBufferToDevice(base, handle);
            CSI_TransferLoadBufferToDevice(base, handle);
            CSI_ReflashFifoDma(base, kCSI_RxFifo);
            CSI_Start(base);
        }
    }

    return kStatus_Success;
}

status_t CSI_TransferGetFullBuffer(CSI_Type *base, csi_handle_t *handle, uint32_t *frameBuffer)
{
    uint32_t csicr1;

    /* No full frame buffer. */
    if (handle->queueUserReadIdx == handle->queueDrvWriteIdx)
    {
        return kStatus_CSI_NoFullBuffer;
    }

    /* Disable the interrupt to protect the index information in handle. */
    csicr1 = base->CSICR1;

    base->CSICR1 = (csicr1 & ~(CSI_CSICR1_FB2_DMA_DONE_INTEN_MASK | CSI_CSICR1_FB1_DMA_DONE_INTEN_MASK));

    *frameBuffer = handle->frameBufferQueue[handle->queueUserReadIdx];

    handle->queueUserReadIdx = CSI_TransferIncreaseQueueIdx(handle->queueUserReadIdx);

    base->CSICR1 = csicr1;

    return kStatus_Success;
}

void CSI_TransferHandleIRQ(CSI_Type *base, csi_handle_t *handle)
{
    uint32_t queueDrvWriteIdx;
    uint32_t csisr = base->CSISR;

    /* Clear the error flags. */
    base->CSISR = csisr;

    /*
     * If both frame buffer 0 and frame buffer 1 flags assert, driver does not
     * know which frame buffer ready just now, so reset the CSI transfer to
     * start from frame buffer 0.
     */
    if ((csisr & (CSI_CSISR_DMA_TSF_DONE_FB2_MASK | CSI_CSISR_DMA_TSF_DONE_FB1_MASK)) ==
        (CSI_CSISR_DMA_TSF_DONE_FB2_MASK | CSI_CSISR_DMA_TSF_DONE_FB1_MASK))
    {
        CSI_Stop(base);

        /* Reset the active buffers. */
        if (1 <= handle->activeBufferNum)
        {
            queueDrvWriteIdx = handle->queueDrvWriteIdx;

            base->CSIDMASA_FB1 = handle->frameBufferQueue[queueDrvWriteIdx];

            if (2U == handle->activeBufferNum)
            {
                queueDrvWriteIdx = CSI_TransferIncreaseQueueIdx(queueDrvWriteIdx);
                base->CSIDMASA_FB2 = handle->frameBufferQueue[queueDrvWriteIdx];
                handle->nextBufferIdx = 0U;
            }
            else
            {
                handle->nextBufferIdx = 1U;
            }
        }
        CSI_ReflashFifoDma(base, kCSI_RxFifo);
        CSI_Start(base);
    }
    else if (csisr & (CSI_CSISR_DMA_TSF_DONE_FB2_MASK | CSI_CSISR_DMA_TSF_DONE_FB1_MASK))
    {
        handle->queueDrvWriteIdx = CSI_TransferIncreaseQueueIdx(handle->queueDrvWriteIdx);

        handle->activeBufferNum--;

        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_CSI_FrameDone, handle->userData);
        }

        /* No frame buffer to save incoming data, then stop the CSI module. */
        if (!(handle->activeBufferNum))
        {
            CSI_Stop(base);
            handle->transferOnGoing = false;
        }
        else
        {
            if (CSI_TransferGetEmptyBufferCount(base, handle))
            {
                CSI_TransferLoadBufferToDevice(base, handle);
            }
        }
    }
    else
    {
    }
}

#if defined(CSI)
void CSI_DriverIRQHandler(void)
{
    s_csiIsr(CSI, s_csiHandle[0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CSI0)
void CSI0_DriverIRQHandler(void)
{
    s_csiIsr(CSI, s_csiHandle[0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
