/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

/*
 * By default disable both asserts and log for this module.
 * This must be done before DebugP.h is included.
 */
#ifndef DebugP_ASSERT_ENABLED
#define DebugP_ASSERT_ENABLED 0
#endif
#ifndef DebugP_LOG_ENABLED
#define DebugP_LOG_ENABLED 0
#endif
#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

#include <ti/drivers/camera/CameraCC32XXDMA.h>

/* driverlib header files */
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/camera.h>
#include <ti/devices/cc32xx/driverlib/interrupt.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/udma.h>
#include <ti/devices/cc32xx/driverlib/utils.h>

#define CAM_BT_CORRECT_EN   0x00001000

/* CameraCC32XXDMA functions */
void           CameraCC32XXDMA_close(Camera_Handle handle);
int_fast16_t   CameraCC32XXDMA_control(Camera_Handle handle,
                                       uint_fast16_t cmd, void *arg);
void           CameraCC32XXDMA_init(Camera_Handle handle);
Camera_Handle  CameraCC32XXDMA_open(Camera_Handle handle,
                                    Camera_Params *params);
int_fast16_t   CameraCC32XXDMA_capture(Camera_Handle handle, void *buffer,
                                       size_t bufferlen, size_t *frameLen);

/* Camera function table for CameraCC32XXDMA implementation */
const Camera_FxnTable CameraCC32XXDMA_fxnTable = {
    CameraCC32XXDMA_close,
    CameraCC32XXDMA_control,
    CameraCC32XXDMA_init,
    CameraCC32XXDMA_open,
    CameraCC32XXDMA_capture
};

/*
 *  ======== CameraCC32XXDMA_configDMA ========
 */
static void CameraCC32XXDMA_configDMA(Camera_Handle handle)
{
    CameraCC32XXDMA_Object           *object = handle->object;
    CameraCC32XXDMA_HWAttrs const    *hwAttrs = handle->hwAttrs;
    unsigned long **bufferPtr = (unsigned long**)&object->captureBuf;


    /* Clear ALT SELECT Attribute */
    MAP_uDMAChannelAttributeDisable(hwAttrs->channelIndex,UDMA_ATTR_ALTSELECT);

    /* Setup ping-pong transfer */
    MAP_uDMAChannelControlSet(hwAttrs->channelIndex,
              UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_32 | UDMA_ARB_8);
    MAP_uDMAChannelAttributeEnable(hwAttrs->channelIndex,UDMA_ATTR_USEBURST);
    MAP_uDMAChannelTransferSet(hwAttrs->channelIndex, UDMA_MODE_PINGPONG,
                               (void *)CAM_BUFFER_ADDR, (void *)*bufferPtr,
                               CameraCC32XXDMA_DMA_TRANSFER_SIZE);

    /* Pong Buffer */
    *bufferPtr += CameraCC32XXDMA_DMA_TRANSFER_SIZE;
    MAP_uDMAChannelControlSet(hwAttrs->channelIndex | UDMA_ALT_SELECT,
              UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_32 | UDMA_ARB_8);
    MAP_uDMAChannelAttributeEnable(hwAttrs->channelIndex | UDMA_ALT_SELECT,
                                                       UDMA_ATTR_USEBURST);
    MAP_uDMAChannelTransferSet(hwAttrs->channelIndex | UDMA_ALT_SELECT,
                                UDMA_MODE_PINGPONG,
                               (void *)CAM_BUFFER_ADDR,
                               (void *)*bufferPtr,
                               CameraCC32XXDMA_DMA_TRANSFER_SIZE);
    MAP_uDMAChannelEnable(hwAttrs->channelIndex | UDMA_ALT_SELECT);

    /*  Ping Buffer */
    *bufferPtr += CameraCC32XXDMA_DMA_TRANSFER_SIZE;

    DebugP_log1("Camera:(%p) DMA transfer enabled", hwAttrs->baseAddr);

    /* Set mode to Ping buffer initially */
    object->cameraDMA_PingPongMode = 0;

    /* Clear any pending interrupt */
    MAP_CameraIntClear(hwAttrs->baseAddr, CAM_INT_DMA);

    /* DMA Interrupt unmask from apps config */
    MAP_CameraIntEnable(hwAttrs->baseAddr, CAM_INT_DMA);

    DebugP_log3("Camera:(%p) DMA receive, "
                   "CaptureBuf: %p; Count: %d",
                    hwAttrs->baseAddr,
                    (uintptr_t)*bufferPtr,
                    (uintptr_t)object->bufferlength);
}

/*
 *  ======== captureSemCallback ========
 *  Simple callback to post a semaphore for the blocking mode.
 */
static void captureSemCallback(Camera_Handle handle, void *buffer, size_t count)
{
    CameraCC32XXDMA_Object *object = handle->object;

    SemaphoreP_post(object->captureSem);
}

/*
 *  ======== CameraCC32XXDMA_hwiIntFxn ========
 *  Hwi function that processes Camera interrupts.
 *
 *  The Frame end,DMA interrupts is enabled for the camera.
 *  The DMA interrupt is triggered for every 64 elements.
 *  The ISR will check for Frame end interrupt to trigger the callback
 *  in the non-blocking mode/post a semaphore in the blocking mode to
 *  indicate capture complete.
 *
 *  @param(arg)         The Camera_Handle for this Hwi.
 */
static void CameraCC32XXDMA_hwiIntFxn(uintptr_t arg)
{
    uint32_t                     status;
    CameraCC32XXDMA_Object        *object = ((Camera_Handle)arg)->object;
    CameraCC32XXDMA_HWAttrs const *hwAttrs = ((Camera_Handle)arg)->hwAttrs;
    unsigned long **bufferPtr = (unsigned long**)&object->captureBuf;

    status = MAP_CameraIntStatus(hwAttrs->baseAddr);
    if ((object->cameraDMAxIntrRcvd > 1) && (status & (CAM_INT_FE))) {
        DebugP_log2("Camera:(%p) Interrupt with mask 0x%x",
               hwAttrs->baseAddr,status);

        MAP_CameraIntClear(hwAttrs->baseAddr, CAM_INT_FE);
        object->captureCallback((Camera_Handle)arg, *bufferPtr,
                                 object->frameLength);
        DebugP_log2("Camera:(%p) capture finished, %d bytes written",
                hwAttrs->baseAddr, object->frameLength);
        object->inUse = 0;

        MAP_CameraCaptureStop(hwAttrs->baseAddr, true);

        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    }

    if (status & CAM_INT_DMA) {
        // Camera DMA Done clear
        MAP_CameraIntClear(hwAttrs->baseAddr, CAM_INT_DMA);

        object->cameraDMAxIntrRcvd++;

        object->frameLength +=
         (CameraCC32XXDMA_DMA_TRANSFER_SIZE*sizeof(unsigned long));
        if (object->frameLength < object->bufferlength) {
             if (object->cameraDMA_PingPongMode == 0) {
                MAP_uDMAChannelControlSet(hwAttrs->channelIndex,
                UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_32 | UDMA_ARB_8);

                MAP_uDMAChannelAttributeEnable(hwAttrs->channelIndex,
                                                 UDMA_ATTR_USEBURST);
                MAP_uDMAChannelTransferSet(hwAttrs->channelIndex,
                                           UDMA_MODE_PINGPONG,
                                           (void *)CAM_BUFFER_ADDR,
                                           (void *)*bufferPtr,
                                           CameraCC32XXDMA_DMA_TRANSFER_SIZE);
                MAP_uDMAChannelEnable(hwAttrs->channelIndex);
                *bufferPtr += CameraCC32XXDMA_DMA_TRANSFER_SIZE;
                object->cameraDMA_PingPongMode = 1;
            }
            else {
                MAP_uDMAChannelControlSet(hwAttrs->channelIndex | UDMA_ALT_SELECT,
                  UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_32 | UDMA_ARB_8);

                MAP_uDMAChannelAttributeEnable(
                    hwAttrs->channelIndex | UDMA_ALT_SELECT,
                    UDMA_ATTR_USEBURST);
                MAP_uDMAChannelTransferSet(
                    hwAttrs->channelIndex | UDMA_ALT_SELECT,
                    UDMA_MODE_PINGPONG,
                    (void *)CAM_BUFFER_ADDR, (void *)*bufferPtr,
                    CameraCC32XXDMA_DMA_TRANSFER_SIZE);
                MAP_uDMAChannelEnable(hwAttrs->channelIndex | UDMA_ALT_SELECT);
                *bufferPtr += CameraCC32XXDMA_DMA_TRANSFER_SIZE;
                object->cameraDMA_PingPongMode = 0;
            }
        }
        else {
            // Disable DMA
            ROM_UtilsDelayDirect(40000);
            MAP_uDMAChannelDisable(hwAttrs->channelIndex);
            MAP_CameraIntDisable(hwAttrs->baseAddr, CAM_INT_DMA);
            object->cameraDMA_PingPongMode = 0;
            object->captureCallback((Camera_Handle)arg, *bufferPtr,
                                     object->frameLength);
            DebugP_log2("Camera:(%p) capture finished, %d bytes written",
                    hwAttrs->baseAddr, object->frameLength);

            MAP_CameraCaptureStop(hwAttrs->baseAddr, true);

            Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
        }
    }
}

/*
 *  ======== CameraCC32XXDMA_init ========
 */
void CameraCC32XXDMA_init(Camera_Handle handle)
{
    CameraCC32XXDMA_Object *object = handle->object;

    object->opened = false;
}

/*
 *  ======== CameraCC32XXDMA_open ========
 */
Camera_Handle CameraCC32XXDMA_open(Camera_Handle handle, Camera_Params *params)
{
    uintptr_t                      key;
    CameraCC32XXDMA_Object        *object = handle->object;
    CameraCC32XXDMA_HWAttrs const *hwAttrs = handle->hwAttrs;
    unsigned long                  hSyncPolarityConfig;
    unsigned long                  vSyncPolarityConfig;
    unsigned long                  byteOrderConfig;
    unsigned long                  interfaceSync;
    unsigned long                  pixelClkConfig;
    HwiP_Params                    hwiParams;
    SemaphoreP_Params              semParams;

    /* Timeouts cannot be 0 */
    DebugP_assert((params->captureTimeout != 0));

    /* Check that a callback is set */
    DebugP_assert((params->captureMode != Camera_MODE_CALLBACK) ||
                  (params->captureCallback != NULL));

    /* Disable preemption while checking if the Camera is open. */
    key = HwiP_disable();

    /* Check if the Camera is open already with the base addr. */
    if (object->opened == true) {
        HwiP_restore(key);

        DebugP_log1("Camera:(%p) already in use.", hwAttrs->baseAddr);
        return (NULL);
    }

    object->opened = true;
    HwiP_restore(key);

    object->operationMode    = params->captureMode;
    object->captureCallback  = params->captureCallback;
    object->captureTimeout   = params->captureTimeout;

    /* Set Camera variables to defaults. */
    object->captureBuf = NULL;
    object->bufferlength = 0;
    object->frameLength  = 0;
    object->inUse        = 0;

    /*
     *  Register power dependency. Keeps the clock running in SLP
     *  and DSLP modes.
     */
    Power_setDependency(PowerCC32XX_PERIPH_CAMERA);
    Power_setDependency(PowerCC32XX_PERIPH_UDMA);

    /* Disable the Camera interrupt. */
    MAP_CameraIntDisable(hwAttrs->baseAddr, (CAM_INT_FE | CAM_INT_DMA));

    HwiP_clearInterrupt(hwAttrs->intNum);

    /* Create Hwi object for the Camera peripheral. */
    /* Register the interrupt for this Camera peripheral. */
    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t)handle;
    hwiParams.priority = hwAttrs->intPriority;
    object->hwiHandle = HwiP_create(hwAttrs->intNum, CameraCC32XXDMA_hwiIntFxn,
                                    &hwiParams);
    if (object->hwiHandle == NULL) {
        CameraCC32XXDMA_close(handle);
        return (NULL);
    }

    MAP_IntEnable(INT_CAMERA);

    SemaphoreP_Params_init(&semParams);
    semParams.mode = SemaphoreP_Mode_BINARY;

    /* If capture is blocking create a semaphore and set callback. */
    if (object->operationMode == Camera_MODE_BLOCKING) {
        object->captureSem = SemaphoreP_create(0, &semParams);
        if (object->captureSem == NULL) {
            CameraCC32XXDMA_close(handle);
            return (NULL);
        }
        object->captureCallback = &captureSemCallback;
    }

    MAP_CameraReset(hwAttrs->baseAddr);

    if (params->hsyncPolarity == Camera_HSYNC_POLARITY_HIGH) {
        hSyncPolarityConfig = CAM_HS_POL_HI;
    }
    else {
        hSyncPolarityConfig = CAM_HS_POL_LO;
    }

    if (params->vsyncPolarity == Camera_VSYNC_POLARITY_HIGH) {
        vSyncPolarityConfig = CAM_VS_POL_HI;
    }
    else {
        vSyncPolarityConfig = CAM_VS_POL_LO;
    }

    if (params->byteOrder == Camera_BYTE_ORDER_SWAP) {
        byteOrderConfig = CAM_ORDERCAM_SWAP;
    }
    else {
        byteOrderConfig = 0;
    }

    if (params->interfaceSync == Camera_INTERFACE_SYNC_OFF) {
        interfaceSync = CAM_NOBT_SYNCHRO;
    }
    else {
        interfaceSync = CAM_NOBT_SYNCHRO | CAM_IF_SYNCHRO | CAM_BT_CORRECT_EN;
    }

    if (params->pixelClkConfig == Camera_PCLK_CONFIG_RISING_EDGE) {
        pixelClkConfig = CAM_PCLK_RISE_EDGE;
    }
    else {
        pixelClkConfig = CAM_PCLK_FALL_EDGE;
    }

    MAP_CameraParamsConfig(hwAttrs->baseAddr, hSyncPolarityConfig,
                           vSyncPolarityConfig,
                           byteOrderConfig | interfaceSync | pixelClkConfig);

    /*Set the clock divider based on the output clock */
    MAP_CameraXClkConfig(hwAttrs->baseAddr,
                         MAP_PRCMPeripheralClockGet(PRCM_CAMERA),
                         params->outputClock);

    /*Setting the FIFO threshold for a DMA request */
    MAP_CameraThresholdSet(hwAttrs->baseAddr, 8);

    MAP_CameraIntEnable(hwAttrs->baseAddr, (CAM_INT_FE | CAM_INT_DMA));
    MAP_CameraDMAEnable(hwAttrs->baseAddr);

    DebugP_log1("Camera:(%p) opened", hwAttrs->baseAddr);

    /* Return the handle */
    return (handle);
}

/*
 *  ======== CameraCC32XXDMA_close ========
 */
void CameraCC32XXDMA_close(Camera_Handle handle)
{
    CameraCC32XXDMA_Object           *object = handle->object;
    CameraCC32XXDMA_HWAttrs const    *hwAttrs = handle->hwAttrs;

    /* Disable Camera and interrupts. */
    MAP_CameraIntDisable(hwAttrs->baseAddr,CAM_INT_FE);
    MAP_CameraDMADisable(hwAttrs->baseAddr);

    if (object->hwiHandle) {
        HwiP_delete(object->hwiHandle);
    }
    if (object->captureSem) {
        SemaphoreP_delete(object->captureSem);
    }

    Power_releaseDependency(PowerCC32XX_PERIPH_CAMERA);
    Power_releaseDependency(PowerCC32XX_PERIPH_UDMA);

    object->opened = false;

    DebugP_log1("Camera:(%p) closed", hwAttrs->baseAddr);
}

/*
 *  ======== CameraCC32XXDMA_control ========
 *  @pre    Function assumes that the handle is not NULL
 */
int_fast16_t CameraCC32XXDMA_control(Camera_Handle handle, uint_fast16_t cmd,
                                     void *arg)
{
    /* No implementation yet */
    return (CAMERA_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== CameraCC32XXDMA_capture ========
 */

int_fast16_t CameraCC32XXDMA_capture(Camera_Handle handle, void *buffer,
    size_t bufferlen, size_t *frameLen)
{
   CameraCC32XXDMA_Object        *object = handle->object;
   CameraCC32XXDMA_HWAttrs const *hwAttrs = handle->hwAttrs;
   uintptr_t                      key;

   key = HwiP_disable();
   if (object->inUse) {
       HwiP_restore(key);
       DebugP_log1("Camera:(%p) Could not capture data, camera in use.",
                      ((CameraCC32XXDMA_HWAttrs const *)
                          (handle->hwAttrs))->baseAddr);

       return (CAMERA_STATUS_ERROR);
   }

   object->captureBuf             = buffer;
   object->bufferlength           = bufferlen;
   object->frameLength            = 0;
   object->cameraDMAxIntrRcvd     = 0;
   object->inUse                  = 1;
   object->cameraDMA_PingPongMode = 0;

   HwiP_restore(key);

   /* Set constraints to guarantee transaction */
   Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

   /* Start the DMA transfer */
   CameraCC32XXDMA_configDMA(handle);
   MAP_CameraCaptureStart(hwAttrs->baseAddr);

   /* If operationMode is blocking, block and get the status. */
   if (object->operationMode == Camera_MODE_BLOCKING) {
       /* Pend on semaphore and wait for Hwi to finish. */
       if (SemaphoreP_OK == SemaphoreP_pend(object->captureSem,
                   object->captureTimeout)) {

           DebugP_log2("Camera:(%p) Capture timed out, %d bytes captured",
                          ((CameraCC32XXDMA_HWAttrs const *)
                          (handle->hwAttrs))->baseAddr,
                          object->frameLength);
       }
       else {
           MAP_CameraCaptureStop(hwAttrs->baseAddr, true);
           *frameLen = object->frameLength;
           return (CAMERA_STATUS_SUCCESS);
       }
   }

   *frameLen = 0;
   return (CAMERA_STATUS_SUCCESS);
}
