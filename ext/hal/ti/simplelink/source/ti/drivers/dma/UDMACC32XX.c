/*
 * Copyright (c) 2016, Texas Instruments Incorporated
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

#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/dma/UDMACC32XX.h>

#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/udma.h>

extern const UDMACC32XX_Config UDMACC32XX_config[];

static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
        uintptr_t clientArg);

static bool dmaInitialized = false;
static Power_NotifyObj postNotifyObj;    /* LPDS wake-up notify object */

/* Reference count for open calls */
static uint32_t          refCount = 0;

/*
 *  ======== UDMACC32XX_close ========
 */
void UDMACC32XX_close(UDMACC32XX_Handle handle)
{
    UDMACC32XX_Object    *object = handle->object;
    uintptr_t             key;

    Power_releaseDependency(PowerCC32XX_PERIPH_UDMA);

    key = HwiP_disable();

    refCount--;

    if (refCount == 0) {
        Power_unregisterNotify(&postNotifyObj);
        object->isOpen = false;
    }

    HwiP_restore(key);
}

/*
 *  ======== UDMACC32XX_init ========
 */
void UDMACC32XX_init()
{
    HwiP_Params           hwiParams;
    UDMACC32XX_Handle     handle = (UDMACC32XX_Handle)&(UDMACC32XX_config[0]);
    UDMACC32XX_HWAttrs    const *hwAttrs = handle->hwAttrs;
    UDMACC32XX_Object    *object = handle->object;

    if (!dmaInitialized) {
        object->isOpen = false;

        HwiP_Params_init(&hwiParams);
        hwiParams.priority = hwAttrs->intPriority;

        /* Will check in UDMACC32XX_open() if this failed */
        object->hwiHandle = HwiP_create(hwAttrs->intNum, hwAttrs->dmaErrorFxn,
                &hwiParams);
        if (object->hwiHandle == NULL) {
            DebugP_log0("Failed to create uDMA error Hwi!!\n");
        }
        else {
            dmaInitialized = true;
        }
    }
}

/*
 *  ======== UDMACC32XX_open ========
 */
UDMACC32XX_Handle UDMACC32XX_open()
{
    UDMACC32XX_Handle     handle = (UDMACC32XX_Handle)&(UDMACC32XX_config);
    UDMACC32XX_Object    *object = handle->object;
    UDMACC32XX_HWAttrs    const *hwAttrs = handle->hwAttrs;
    uintptr_t             key;

    if (!dmaInitialized) {
        return (NULL);
    }

    Power_setDependency(PowerCC32XX_PERIPH_UDMA);

    key = HwiP_disable();

    /*
     *  If the UDMA has not been opened yet, create the error Hwi
     *  and initialize the control table base address.
     */
    if (object->isOpen == false) {
        MAP_PRCMPeripheralReset(PRCM_UDMA);

        MAP_uDMAEnable();
        MAP_uDMAControlBaseSet(hwAttrs->controlBaseAddr);

        Power_registerNotify(&postNotifyObj, PowerCC32XX_AWAKE_LPDS,
                postNotifyFxn, (uintptr_t)handle);

        object->isOpen = true;
    }

    refCount++;

    HwiP_restore(key);

    return (handle);
}

/*
 *  ======== postNotifyFxn ========
 *  Called by Power module when waking up from LPDS.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
        uintptr_t clientArg)
{
    UDMACC32XX_Handle handle = (UDMACC32XX_Handle)clientArg;
    UDMACC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;

    MAP_uDMAEnable();
    MAP_uDMAControlBaseSet(hwAttrs->controlBaseAddr);

    return (Power_NOTIFYDONE);
}
