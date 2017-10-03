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
/*
 *  ======== PowerCC32XX.c ========
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

#include <ti/drivers/utils/List.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

#if defined(__IAR_SYSTEMS_ICC__)
#include <intrinsics.h>
#endif

/* driverlib header files */
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_gprcm.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/inc/hw_nvic.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/driverlib/pin.h>
#include <ti/devices/cc32xx/driverlib/cpu.h>
#include <ti/devices/cc32xx/driverlib/hwspinlock.h>
#include <ti/devices/cc32xx/driverlib/spi.h>

#define TRUE    1
#define FALSE   0
#define STATUS_BUSY   0x01

#define PowerCC32XX_SSPIReadStatusInstruction   (0x05)
#define PowerCC32XX_SSPIPowerDownInstruction    (0xB9)
#define PowerCC32XX_SSPISemaphoreTakeTries      (4000000)
#define PowerCC32XX_SSPICSDelay                 33
#define uSEC_DELAY(x)                           (ROM_UtilsDelayDirect(x*80/3))

#define SYNCBARRIER() {          \
    __asm(" dsb \n"               \
          " isb \n");             \
}

/* Externs */
extern const PowerCC32XX_ConfigV1 PowerCC32XX_config;

/* Module_State */
PowerCC32XX_ModuleState PowerCC32XX_module = {
    { NULL, NULL},  /* list */
    0,              /* constraintsMask */
    Power_ACTIVE,   /* state */
    /* dbRecords */
    {
        PRCM_CAMERA,  /* PERIPH_CAMERA */
        PRCM_I2S,     /* PERIPH_MCASP */
        PRCM_SDHOST,  /* PERIPH_MMCHS */
        PRCM_GSPI,    /* PERIPH_MCSPI_A1 */
        PRCM_LSPI,    /* PERIPH_MCSPI_A2 */
        PRCM_UDMA,    /* PERIPH_UDMA_A */
        PRCM_GPIOA0,  /* PERIPH_GPIO_A */
        PRCM_GPIOA1,  /* PERIPH_GPIO_B */
        PRCM_GPIOA2,  /* PERIPH_GPIO_C */
        PRCM_GPIOA3,  /* PERIPH_GPIO_D */
        PRCM_GPIOA4,  /* PERIPH_GPIO_E */
        PRCM_WDT,     /* PERIPH_WDOG_A */
        PRCM_UARTA0,  /* PERIPH_UART_A0 */
        PRCM_UARTA1,  /* PERIPH_UART_A1 */
        PRCM_TIMERA0, /* PERIPH_GPT_A0 */
        PRCM_TIMERA1, /* PERIPH_GPT_A1 */
        PRCM_TIMERA2, /* PERIPH_GPT_A2 */
        PRCM_TIMERA3, /* PERIPH_GPT_A3 */
        PRCM_DTHE,    /* PERIPH_CRYPTO */
        PRCM_SSPI,    /* PERIPH_MCSPI_S0 */
        PRCM_I2CA0    /* PERIPH_I2C */
    },
    /* enablePolicy */
    FALSE,
    /* initialized */
    FALSE,
    /* refCount */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* constraintCounts */
    { 0, 0 },
    /* policyFxn */
    NULL
};

/* context save variable */
PowerCC32XX_SaveRegisters PowerCC32XX_contextSave;

typedef void (*LPDSFunc)(void);

/* enter LPDS is an assembly function */
extern void PowerCC32XX_enterLPDS(LPDSFunc driverlibFunc);

/* pin parking functions */
void PowerCC32XX_parkPin(PowerCC32XX_Pin pin, PowerCC32XX_ParkState parkState,
    uint32_t * previousState, uint16_t * previousDirection);
void PowerCC32XX_restoreParkedPin(PowerCC32XX_Pin pin, uint32_t type,
    uint16_t direction);
void PowerCC32XX_shutdownSSPI(void);

/* internal functions */
static int_fast16_t notify(uint_fast16_t eventType);
static void restoreNVICRegs(void);
static void restorePeriphClocks(void);
static void saveNVICRegs(void);
static void parkPins(void);
static void restoreParkedPins(void);

/*
 *  ======== Power_disablePolicy ========
 *  Do not run the configured policy
 */
bool Power_disablePolicy(void)
{
    bool enablePolicy = PowerCC32XX_module.enablePolicy;
    PowerCC32XX_module.enablePolicy = FALSE;

    DebugP_log0("Power: disable policy");

    return (enablePolicy);
}

/*
 *  ======== Power_enablePolicy ========
 *  Run the configured policy
 */
void Power_enablePolicy(void)
{
    PowerCC32XX_module.enablePolicy = TRUE;

    DebugP_log0("Power: enable policy");
}

/*
 *  ======== Power_getConstraintMask ========
 *  Get a bitmask indicating the constraints that have been registered with
 *  Power.
 */
uint_fast32_t Power_getConstraintMask(void)
{
    return (PowerCC32XX_module.constraintMask);
}

/*
 *  ======== Power_getDependencyCount ========
 *  Get the count of dependencies that are currently declared upon a resource.
 */
int_fast16_t Power_getDependencyCount(uint_fast16_t resourceId)
{
    int_fast16_t status;

    if (resourceId >= PowerCC32XX_NUMRESOURCES) {
        status = Power_EINVALIDINPUT;
    }
    else {
        status = PowerCC32XX_module.refCount[resourceId];
    }

    return (status);
}

/*
 *  ======== Power_getTransitionLatency ========
 *  Get the transition latency for a sleep state.  The latency is reported
 *  in units of microseconds.
 */
uint_fast32_t Power_getTransitionLatency(uint_fast16_t sleepState,
    uint_fast16_t type)
{
    uint32_t latency = 0;

    if (type == Power_RESUME) {
        latency = PowerCC32XX_RESUMETIMELPDS;
    }
    else {
        latency = PowerCC32XX_TOTALTIMELPDS;
    }

    return (latency);
}

/*
 *  ======== Power_getTransitionState ========
 *  Get the current sleep transition state.
 */
uint_fast16_t Power_getTransitionState(void)
{
    return (PowerCC32XX_module.state);
}

/*
 *  ======== Power_idleFunc ========
 *  Function needs to be plugged into the idle loop.
 *  It calls the configured policy function if the
 *  'enablePolicy' flag is set.
 */
void Power_idleFunc()
{
    if (PowerCC32XX_module.enablePolicy) {
        if (PowerCC32XX_module.policyFxn != NULL) {
            DebugP_log1("Power: calling policy function (%p)",
                (uintptr_t) PowerCC32XX_module.policyFxn);
            (*(PowerCC32XX_module.policyFxn))();
        }
    }
}

/*
 *  ======== Power_init ========
 */
int_fast16_t Power_init()
{
    /* if this function has already been called, just return */
    if (PowerCC32XX_module.initialized) {
        return (Power_SOK);
    }

    /* set module state field 'initialized' to true */
    PowerCC32XX_module.initialized = TRUE;

    /* set the module state enablePolicy field */
    PowerCC32XX_module.enablePolicy = PowerCC32XX_config.enablePolicy;

    /* call the config policy init function if its not null */
    if (PowerCC32XX_config.policyInitFxn != NULL) {
        (*(PowerCC32XX_config.policyInitFxn))();
    }

    /* copy wakeup settings to module state */
    PowerCC32XX_module.wakeupConfig.enableGPIOWakeupLPDS =
        PowerCC32XX_config.enableGPIOWakeupLPDS;
    PowerCC32XX_module.wakeupConfig.enableGPIOWakeupShutdown =
        PowerCC32XX_config.enableGPIOWakeupShutdown;
    PowerCC32XX_module.wakeupConfig.enableNetworkWakeupLPDS =
        PowerCC32XX_config.enableNetworkWakeupLPDS;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOSourceLPDS =
        PowerCC32XX_config.wakeupGPIOSourceLPDS;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOTypeLPDS =
        PowerCC32XX_config.wakeupGPIOTypeLPDS;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOFxnLPDS =
        PowerCC32XX_config.wakeupGPIOFxnLPDS;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOFxnLPDSArg =
        PowerCC32XX_config.wakeupGPIOFxnLPDSArg;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOSourceShutdown =
        PowerCC32XX_config.wakeupGPIOSourceShutdown;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOTypeShutdown =
        PowerCC32XX_config.wakeupGPIOTypeShutdown;

    /* now configure these wakeup settings in the device... */
    PowerCC32XX_configureWakeup(&PowerCC32XX_module.wakeupConfig);

    /* copy the Power policy function to module state */
    PowerCC32XX_module.policyFxn = PowerCC32XX_config.policyFxn;

    /* spin if too many pins were specified in the pin park array */
    if (PowerCC32XX_config.numPins > PowerCC32XX_NUMPINS) {
        while(1){}
    }

    return (Power_SOK);
}

/*
 *  ======== Power_registerNotify ========
 *  Register a function to be called on a specific power event.
 */
int_fast16_t Power_registerNotify(Power_NotifyObj * pNotifyObj,
    uint_fast16_t eventTypes, Power_NotifyFxn notifyFxn, uintptr_t clientArg)
{
    int_fast16_t status = Power_SOK;

    /* check for NULL pointers  */
    if ((pNotifyObj == NULL) || (notifyFxn == NULL)) {
        status = Power_EINVALIDPOINTER;
    }

    else {
        /* fill in notify object elements */
        pNotifyObj->eventTypes = eventTypes;
        pNotifyObj->notifyFxn = notifyFxn;
        pNotifyObj->clientArg = clientArg;

        /* place notify object on event notification queue */
        List_put(&PowerCC32XX_module.notifyList, (List_Elem*)pNotifyObj);
    }

    DebugP_log3(
        "Power: register notify (%p), eventTypes (0x%x), notifyFxn (%p)",
        (uintptr_t) pNotifyObj, eventTypes, (uintptr_t) notifyFxn);

    return (status);
}

/*
 *  ======== Power_releaseConstraint ========
 *  Release a previously declared constraint.
 */
int_fast16_t Power_releaseConstraint(uint_fast16_t constraintId)
{
    int_fast16_t status = Power_SOK;
    uintptr_t key;
    uint8_t count;

    /* first ensure constraintId is valid */
    if (constraintId >= PowerCC32XX_NUMCONSTRAINTS) {
        status = Power_EINVALIDINPUT;
    }

    /* if constraintId is OK ... */
    else {

        /* disable interrupts */
        key = HwiP_disable();

        /* get the count of the constraint */
        count = PowerCC32XX_module.constraintCounts[constraintId];

        /* ensure constraint count is not already zero */
        if (count == 0) {
            status = Power_EFAIL;
        }

        /* if not already zero ... */
        else {
            /* decrement the count */
            count--;

            /* save the updated count */
            PowerCC32XX_module.constraintCounts[constraintId] = count;

            /* if constraint count reaches zero, remove constraint from mask */
            if (count == 0) {
                PowerCC32XX_module.constraintMask &= ~(1 << constraintId);
            }
        }

        /* restore interrupts */
        HwiP_restore(key);

        DebugP_log1("Power: release constraint (%d)", constraintId);
    }

    return (status);
}

/*
 *  ======== Power_releaseDependency ========
 *  Release a previously declared dependency.
 */
int_fast16_t Power_releaseDependency(uint_fast16_t resourceId)
{
    int_fast16_t status = Power_SOK;
    uint8_t count;
    uint32_t id;
    uintptr_t key;

    /* first check that resourceId is valid */
    if (resourceId >= PowerCC32XX_NUMRESOURCES) {
        status = Power_EINVALIDINPUT;
    }

    /* if resourceId is OK ... */
    else {

        /* disable interrupts */
        key = HwiP_disable();

        /* read the reference count */
        count = PowerCC32XX_module.refCount[resourceId];

        /* ensure dependency count is not already zero */
        if (count == 0) {
            status = Power_EFAIL;
        }

        /* if not already zero ... */
        else {

            /* decrement the reference count */
            count--;

            /* if this was the last dependency being released.., */
            if (count == 0) {
                /* deactivate this resource ... */
                id = PowerCC32XX_module.dbRecords[resourceId];

                /* disable clk to peripheral */
                MAP_PRCMPeripheralClkDisable(id,
                    PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);
            }

            /* save the updated count */
            PowerCC32XX_module.refCount[resourceId] = count;
        }

        /* restore interrupts */
        HwiP_restore(key);

        DebugP_log1("Power: release dependency (%d)", resourceId);
    }

    return (status);
}

/*
 *  ======== Power_setConstraint ========
 *  Declare an operational constraint.
 */
int_fast16_t Power_setConstraint(uint_fast16_t constraintId)
{
    int_fast16_t status = Power_SOK;
    uintptr_t key;

    /* ensure that constraintId is valid */
    if (constraintId >= PowerCC32XX_NUMCONSTRAINTS) {
        status = Power_EINVALIDINPUT;
    }

    else {

        /* disable interrupts */
        key = HwiP_disable();

        /* set the specified constraint in the constraintMask */
        PowerCC32XX_module.constraintMask |= 1 << constraintId;

        /* increment the specified constraint count */
        PowerCC32XX_module.constraintCounts[constraintId]++;

        /* restore interrupts */
        HwiP_restore(key);

        DebugP_log1("Power: set constraint (%d)", constraintId);
    }

    return (status);
}

/*
 *  ======== Power_setDependency ========
 *  Declare a dependency upon a resource.
 */
int_fast16_t Power_setDependency(uint_fast16_t resourceId)
{
    int_fast16_t status = Power_SOK;
    uint8_t count;
    uint32_t id;
    uintptr_t key;

    /* ensure resourceId is valid */
    if (resourceId >= PowerCC32XX_NUMRESOURCES) {
        status = Power_EINVALIDINPUT;
    }

    /* resourceId is OK ... */
    else {

        /* disable interrupts */
        key = HwiP_disable();

        /* read and increment reference count */
        count = PowerCC32XX_module.refCount[resourceId]++;

        /* if resource was NOT activated previously ... */
        if (count == 0) {
            /* now activate this resource ... */
            id = PowerCC32XX_module.dbRecords[resourceId];

            /* enable the peripheral clock to the resource */
            MAP_PRCMPeripheralClkEnable(id,
                PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);

            /* spin here until status returns TRUE */
            while(!MAP_PRCMPeripheralStatusGet(id)) {
            }
        }

        /* restore interrupts */
        HwiP_restore(key);
        DebugP_log1("Power: set dependency (%d)", resourceId);
    }

    return (status);
}

/*
 *  ======== Power_setPolicy ========
 *  Set the Power policy function
 */
void Power_setPolicy(Power_PolicyFxn policy)
{
    PowerCC32XX_module.policyFxn = policy;
}

/*
 *  ======== Power_shutdown ========
 */
int_fast16_t Power_shutdown(uint_fast16_t shutdownState,
                            uint_fast32_t shutdownTime)
{
    int_fast16_t status = Power_EFAIL;
    uint32_t constraints;
    uintptr_t hwiKey;
    uint64_t counts;

    /* disable interrupts */
    hwiKey = HwiP_disable();

    /* make sure shutdown request doesn't violate a constraint */
    constraints = Power_getConstraintMask();
    if (constraints & (1 << PowerCC32XX_DISALLOW_SHUTDOWN)) {
        status = Power_ECHANGE_NOT_ALLOWED;
    }
    else {
        if (PowerCC32XX_module.state == Power_ACTIVE) {
            /* set new transition state to entering shutdown */
            PowerCC32XX_module.state = Power_ENTERING_SHUTDOWN;

            /* signal all clients registered for pre-shutdown notification */
            status = notify(PowerCC32XX_ENTERING_SHUTDOWN);
            /* check for timeout or any other error */
            if (status != Power_SOK) {
                PowerCC32XX_module.state = Power_ACTIVE;
                HwiP_restore(hwiKey);
                return (status);
            }
            /* shutdown the flash */
            PowerCC32XX_shutdownSSPI();
            /* if shutdown wakeup time was configured to be large enough */
            if (shutdownTime > (PowerCC32XX_TOTALTIMESHUTDOWN / 1000)) {
                /* calculate the wakeup time for hibernate in RTC counts */
                counts =
                    (((uint64_t)(shutdownTime -
                                (PowerCC32XX_TOTALTIMESHUTDOWN / 1000))
                                * 32768) / 1000);

                /* set the hibernate wakeup time */
                MAP_PRCMHibernateIntervalSet(counts);

                /* enable the wake source to be RTC */
                MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
            }

            /* enable IO retention */
            if (PowerCC32XX_config.ioRetentionShutdown) {
                MAP_PRCMIORetentionEnable(
                    PowerCC32XX_config.ioRetentionShutdown);
            }

            DebugP_log2(
                "Power: entering shutdown state (%d), shutdownTime (%d)",
                shutdownState, shutdownTime);

            /* enter hibernate - we should never return from here */
            MAP_PRCMHibernateEnter();
        }
        else {
            status = Power_EBUSY;
        }
    }

    /* set state to Power_ACTIVE */
    PowerCC32XX_module.state = Power_ACTIVE;

    /* re-enable interrupts */
    HwiP_restore(hwiKey);

    /* if get here, failed to shutdown, return error code */
    return (status);
}

/*
 *  ======== Power_sleep ========
 */
int_fast16_t Power_sleep(uint_fast16_t sleepState)
{
    int_fast16_t status = Power_SOK;
    uint32_t romMajorVer;
    uint32_t romMinorVer;
    uint32_t preEvent;
    uint32_t postEvent;
    uint32_t semBits;
    bool earlyPG = true;

    /* first validate the sleep state */
    if (sleepState != PowerCC32XX_LPDS) {
        status = Power_EINVALIDINPUT;
    }

    else if (PowerCC32XX_module.state == Power_ACTIVE) {

        /* set transition state to entering sleep */
        PowerCC32XX_module.state = Power_ENTERING_SLEEP;

        /* setup sleep vars */
        preEvent = PowerCC32XX_ENTERING_LPDS;
        postEvent = PowerCC32XX_AWAKE_LPDS;

        /* signal all clients registered for pre-sleep notification */
        status = notify(preEvent);

        /* check for timeout or any other error */
        if (status != Power_SOK) {
            PowerCC32XX_module.state = Power_ACTIVE;
            return (status);
        }

        DebugP_log1("Power: sleep, sleepState (%d)", sleepState);

        /* invoke specific sequence to activate LPDS ...*/

        /* enable RAM retention */
        MAP_PRCMSRAMRetentionEnable(
            PowerCC32XX_config.ramRetentionMaskLPDS,
            PRCM_SRAM_LPDS_RET);

        /* call the enter LPDS hook function if configured */
        if (PowerCC32XX_config.enterLPDSHookFxn != NULL) {
            (*(PowerCC32XX_config.enterLPDSHookFxn))();
        }

        /* park pins, based upon board file definitions */
        if (PowerCC32XX_config.pinParkDefs != NULL) {
            parkPins();
        }

        /* save the NVIC registers */
        saveNVICRegs();

        /* check if PG >= 2.01 */
        romMajorVer = HWREG(0x00000400) & 0xFFFF;
        romMinorVer = HWREG(0x00000400) >> 16;
        if ((romMajorVer >= 3) || ((romMajorVer == 2) && (romMinorVer >= 1))) {
            earlyPG = false;
        }

        /* call sync barrier */
        SYNCBARRIER();

        /* now enter LPDS - function does not return... */
        if (PowerCC32XX_config.keepDebugActiveDuringLPDS == TRUE) {
            if (earlyPG) {
                PowerCC32XX_enterLPDS(PRCMLPDSEnterKeepDebugIf);
            }
            else {
                PowerCC32XX_enterLPDS(ROM_PRCMLPDSEnterKeepDebugIfDirect);
            }
        }
        else {
            if (earlyPG) {
                PowerCC32XX_enterLPDS(PRCMLPDSEnter);
            }
            else {
                PowerCC32XX_enterLPDS(ROM_PRCMLPDSEnterDirect);
            }
        }

        /* return here after reset, from Power_resumeLPDS() */

        /* restore NVIC registers */
        restoreNVICRegs();

        /* restore clock to those peripherals with dependecy set */
        restorePeriphClocks();

        /* call PRCMCC3200MCUInit() for any necessary post-LPDS restore */
        MAP_PRCMCC3200MCUInit();

        /* take the GPIO semaphore bits for the MCU */
        semBits = HWREG(0x400F703C);
        semBits = (semBits & ~0x3FF) | 0x155;
        HWREG(0x400F703C) = semBits;

        /* call the resume LPDS hook function if configured */
        if (PowerCC32XX_config.resumeLPDSHookFxn != NULL) {
            (*(PowerCC32XX_config.resumeLPDSHookFxn))();
        }

        /* re-enable Slow Clock Counter Interrupt */
        MAP_PRCMIntEnable(PRCM_INT_SLOW_CLK_CTR);

        /* set transition state to EXITING_SLEEP */
        PowerCC32XX_module.state = Power_EXITING_SLEEP;

        /*
         * signal clients registered for post-sleep notification; for example,
         * a driver that needs to reinitialize its peripheral state, that was
         * lost during LPDS
         */
        status = notify(postEvent);

        /* restore pins parked before LPDS to their previous states */
        if (PowerCC32XX_config.pinParkDefs != NULL) {
            restoreParkedPins();
        }

        /* if wake source was GPIO, optionally call wakeup function */
        if (MAP_PRCMLPDSWakeupCauseGet() == PRCM_LPDS_GPIO) {
            if (PowerCC32XX_module.wakeupConfig.wakeupGPIOFxnLPDS != NULL) {
                (*(PowerCC32XX_module.wakeupConfig.wakeupGPIOFxnLPDS))
                  (PowerCC32XX_module.wakeupConfig.wakeupGPIOFxnLPDSArg);
            }
        }

        /* now clear the transition state before re-enabling scheduler */
        PowerCC32XX_module.state = Power_ACTIVE;
    }
    else {
        status = Power_EBUSY;
    }

    return (status);
}

/*
 *  ======== Power_unregisterNotify ========
 *  Unregister for a power notification.
 *
 */
void Power_unregisterNotify(Power_NotifyObj * pNotifyObj)
{
    uintptr_t key;

    /* disable interrupts */
    key = HwiP_disable();

    /* remove notify object from its event queue */
    List_remove(&PowerCC32XX_module.notifyList, (List_Elem *)pNotifyObj);

    /* re-enable interrupts */
    HwiP_restore(key);

    DebugP_log1("Power: unregister notify (%p)", (uintptr_t) pNotifyObj);
}

/*********************** CC32XX-specific functions **************************/

/*
 *  ======== PowerCC32XX_configureWakeup ========
 *  Configure LPDS and shutdown wakeups; copy settings into driver state
 */
void PowerCC32XX_configureWakeup(PowerCC32XX_Wakeup *wakeup)
{
    /* configure network (Host IRQ) as wakeup source for LPDS */
    if (wakeup->enableNetworkWakeupLPDS) {
        MAP_PRCMLPDSWakeupSourceEnable(PRCM_LPDS_HOST_IRQ);
    }
    else {
        MAP_PRCMLPDSWakeupSourceDisable(PRCM_LPDS_HOST_IRQ);
    }
    PowerCC32XX_module.wakeupConfig.enableNetworkWakeupLPDS =
        wakeup->enableNetworkWakeupLPDS;

    /* configure GPIO as wakeup source for LPDS */
    if (wakeup->enableGPIOWakeupLPDS) {
        MAP_PRCMLPDSWakeUpGPIOSelect(
            wakeup->wakeupGPIOSourceLPDS,
            wakeup->wakeupGPIOTypeLPDS);
        MAP_PRCMLPDSWakeupSourceEnable(PRCM_LPDS_GPIO);
    }
    else {
        MAP_PRCMLPDSWakeupSourceDisable(PRCM_LPDS_GPIO);
    }
    PowerCC32XX_module.wakeupConfig.enableGPIOWakeupLPDS =
        wakeup->enableGPIOWakeupLPDS;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOSourceLPDS =
        wakeup->wakeupGPIOSourceLPDS;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOTypeLPDS =
        wakeup->wakeupGPIOTypeLPDS;

    /* configure GPIO as wakeup source for Shutdown */
    if (wakeup->enableGPIOWakeupShutdown) {
        MAP_PRCMHibernateWakeUpGPIOSelect(
            wakeup->wakeupGPIOSourceShutdown,
            wakeup->wakeupGPIOTypeShutdown);
        MAP_PRCMHibernateWakeupSourceEnable(
            wakeup->wakeupGPIOSourceShutdown);
    }
    else {
        MAP_PRCMHibernateWakeupSourceDisable(
            wakeup->wakeupGPIOSourceShutdown);
    }
    PowerCC32XX_module.wakeupConfig.enableGPIOWakeupShutdown =
        wakeup->enableGPIOWakeupShutdown;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOSourceShutdown =
        wakeup->wakeupGPIOSourceShutdown;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOTypeShutdown =
        wakeup->wakeupGPIOTypeShutdown;

    /* copy the LPDS GPIO wakeup function and arg to module state */
    PowerCC32XX_module.wakeupConfig.wakeupGPIOFxnLPDS =
        wakeup->wakeupGPIOFxnLPDS;
    PowerCC32XX_module.wakeupConfig.wakeupGPIOFxnLPDSArg =
        wakeup->wakeupGPIOFxnLPDSArg;
}

/*
 *  ======== PowerCC32XX_disableIORetention ========
 *  Disable IO retention and unlock pins after exit from Shutdown
 */
void PowerCC32XX_disableIORetention(unsigned long groupFlags)
{
    MAP_PRCMIORetentionDisable(groupFlags);
}

/*
 *  ======== PowerCC32XX_getParkState ========
 *  Get the current LPDS park state for a pin
 */
PowerCC32XX_ParkState PowerCC32XX_getParkState(PowerCC32XX_Pin pin)
{
    PowerCC32XX_ParkInfo parkInfo;
    PowerCC32XX_ParkState state = PowerCC32XX_DONT_PARK;
    uint32_t i;

    DebugP_assert(PowerCC32XX_config.numPins < PowerCC32XX_NUMPINS + 1);

    /* step thru the pin park array until find the pin */
    for (i = 0; i < PowerCC32XX_config.numPins; i++) {

        parkInfo = PowerCC32XX_config.pinParkDefs[i];

        /* if this is the pin to be checked... */
        if (parkInfo.pin == pin) {
            state = (PowerCC32XX_ParkState) parkInfo.parkState;
            break;
        }
    }

    return (state);
}

/*
 *  ======== PowerCC32XX_getWakeup ========
 *  Get the current LPDS and shutdown wakeup configuration
 */
void PowerCC32XX_getWakeup(PowerCC32XX_Wakeup *wakeup)
{
    *wakeup = PowerCC32XX_module.wakeupConfig;
}

/*
 *  ======== PowerCC32XX_parkPin ========
 *  Park a device pin in preparation for LPDS
 */
void PowerCC32XX_parkPin(PowerCC32XX_Pin pin, PowerCC32XX_ParkState parkState,
    uint32_t * previousType, uint16_t * previousDirection)
{
    unsigned long strength;
    unsigned long type;

    /* get the current pin configuration */
    MAP_PinConfigGet(pin, &strength, &type);

    /* stash the current pin type */
    *previousType = type;

    /* get and stash the current pin direction */
    *previousDirection = (uint16_t)MAP_PinDirModeGet(pin);

    /* set pin type to the parking state */
    MAP_PinConfigSet(pin, strength, (unsigned long) parkState);

    /* set pin direction to input to HiZ the pin */
    MAP_PinDirModeSet(pin, PIN_DIR_MODE_IN);
}

/*
 *  ======== PowerCC32XX_restoreParkedPin ========
 *  Restore a pin that was previously parked with PowerCC32XX_parkPin
 */
void PowerCC32XX_restoreParkedPin(PowerCC32XX_Pin pin, uint32_t type,
    uint16_t direction)
{
    unsigned long strength;
    unsigned long currentType;

    /* get the current pin configuration */
    MAP_PinConfigGet(pin, &strength, &currentType);

    /* restore the pin type */
    MAP_PinConfigSet(pin, strength, type);

    /* restore the pin direction */
    MAP_PinDirModeSet(pin, (unsigned long)direction);
}

/*
 *  ======== PowerCC32XX_restoreParkState ========
 *  Restore the LPDS park state for a pin
 */
void PowerCC32XX_restoreParkState(PowerCC32XX_Pin pin,
    PowerCC32XX_ParkState state)
{
    PowerCC32XX_ParkInfo parkInfo;
    uint32_t i;

    DebugP_assert(PowerCC32XX_config.numPins < PowerCC32XX_NUMPINS + 1);

    /* step thru the park array until find the pin to be updated */
    for (i = 0; i < PowerCC32XX_config.numPins; i++) {

        parkInfo = PowerCC32XX_config.pinParkDefs[i];

        /* if this is the pin to be restored... */
        if (parkInfo.pin == pin) {
            parkInfo.parkState = state;
            PowerCC32XX_config.pinParkDefs[i] = parkInfo;
            break;
        }
    }
}

/*
 *  ======== PowerCC32XX_setParkState ========
 *  Set a new LPDS park state for a pin
 */
void PowerCC32XX_setParkState(PowerCC32XX_Pin pin, uint32_t level)
{
    PowerCC32XX_ParkInfo parkInfo;
    PowerCC32XX_ParkState state;
    uint32_t i;

    DebugP_assert(PowerCC32XX_config.numPins < PowerCC32XX_NUMPINS + 1);

    /* first check if level indicates "don't park" */
    if (level == ~1) {
        state = PowerCC32XX_DONT_PARK;
    }

    /* else, check device revision to choose park state */
    /* if ES2.00 or later, drive the pin */
    else if((HWREG(0x00000400) & 0xFFFF) >= 2) {
        state = (level) ? PowerCC32XX_DRIVE_HIGH : PowerCC32XX_DRIVE_LOW;
    }
    /* else, for earlier devices use the weak pull resistor */
    else {
        state = (level) ? PowerCC32XX_WEAK_PULL_UP_STD :
                PowerCC32XX_WEAK_PULL_DOWN_STD;
    }

    /* step thru the park array until find the pin to be updated */
    for (i = 0; i < PowerCC32XX_config.numPins; i++) {

        parkInfo = PowerCC32XX_config.pinParkDefs[i];

        /* if this is the pin to be updated... */
        if (parkInfo.pin == pin) {
            parkInfo.parkState = state;
            PowerCC32XX_config.pinParkDefs[i] = parkInfo;
            break;
        }
    }
}

/*
 *  ======== PowerCC32XX_shutdownSSPI ========
 *  Put SPI flash into Deep Power Down mode
 */
void PowerCC32XX_shutdownSSPI(void)
{
    unsigned long status = 0;

    /* Acquire SSPI HwSpinlock. */
    if (0 != MAP_HwSpinLockTryAcquire(HWSPINLOCK_SSPI, PowerCC32XX_SSPISemaphoreTakeTries)){
        return;
    }

    /* Enable clock for SSPI module */
    MAP_PRCMPeripheralClkEnable(PRCM_SSPI, PRCM_RUN_MODE_CLK);

    /* Reset SSPI at PRCM level and wait for reset to complete */
    MAP_PRCMPeripheralReset(PRCM_SSPI);
    while(MAP_PRCMPeripheralStatusGet(PRCM_SSPI)== false){
    }

    /* Reset SSPI at module level */
    MAP_SPIReset(SSPI_BASE);

    /* Configure SSPI module */
    MAP_SPIConfigSetExpClk(SSPI_BASE,PRCMPeripheralClockGet(PRCM_SSPI),
                    20000000,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                    (SPI_SW_CTRL_CS |
                    SPI_4PIN_MODE |
                    SPI_TURBO_OFF |
                    SPI_CS_ACTIVELOW |
                    SPI_WL_8));

    /* Enable SSPI module */
    MAP_SPIEnable(SSPI_BASE);

    /* Allow settling before enabling chip select */
    uSEC_DELAY(PowerCC32XX_SSPICSDelay);

    /* Enable chip select for the spi flash. */
    MAP_SPICSEnable(SSPI_BASE);

     /* Wait for spi flash. */
    do{
        /* Send status register read instruction and read back a dummy byte. */
        MAP_SPIDataPut(SSPI_BASE,PowerCC32XX_SSPIReadStatusInstruction);
        MAP_SPIDataGet(SSPI_BASE,&status);

        /* Write a dummy byte then read back the actual status. */
        MAP_SPIDataPut(SSPI_BASE,0xFF);
        MAP_SPIDataGet(SSPI_BASE,&status);
    } while((status & 0xFF )== STATUS_BUSY);

    /* Disable chip select for the spi flash. */
    MAP_SPICSDisable(SSPI_BASE);

    /* Start another CS enable sequence for Power down command. */
    MAP_SPICSEnable(SSPI_BASE);

    /* Send Deep Power Down command to spi flash */
    MAP_SPIDataPut(SSPI_BASE,PowerCC32XX_SSPIPowerDownInstruction);

    /* Disable chip select for the spi flash. */
    MAP_SPICSDisable(SSPI_BASE);

    /* Release SSPI HwSpinlock. */
    MAP_HwSpinLockRelease(HWSPINLOCK_SSPI);

    return;
}

/*************************internal functions ****************************/

/*
 *  ======== notify ========
 *  Note: When this function is called hardware interrupts are disabled
 */
static int_fast16_t notify(uint_fast16_t eventType)
{
    int_fast16_t notifyStatus;
    Power_NotifyFxn notifyFxn;
    uintptr_t clientArg;
    List_Elem *elem;

    /* if queue is empty, return immediately */
    if (!List_empty(&PowerCC32XX_module.notifyList)) {
        /* point to first client notify object */
        elem = List_head(&PowerCC32XX_module.notifyList);

        /* walk the queue and notify each registered client of the event */
        do {
            if (((Power_NotifyObj *)elem)->eventTypes & eventType) {
                /* pull params from notify object */
                notifyFxn = ((Power_NotifyObj *)elem)->notifyFxn;
                clientArg = ((Power_NotifyObj *)elem)->clientArg;

                /* call the client's notification function */
                notifyStatus = (int_fast16_t) (*(Power_NotifyFxn)notifyFxn)(
                    eventType, 0, clientArg);

                /* if client declared error stop all further notifications */
                if (notifyStatus != Power_NOTIFYDONE) {
                    return (Power_EFAIL);
                }
            }

            /* get next element in the notification queue */
            elem = List_next(elem);

        } while (elem != NULL);
    }

    return (Power_SOK);
}

/*
 *  ======== restoreNVICRegs ========
 *  Restore the NVIC registers
 */
static void restoreNVICRegs(void)
{
    uint32_t i;
    uint32_t *base_reg_addr;

    /* Restore the NVIC control registers */
    HWREG(NVIC_VTABLE) = PowerCC32XX_contextSave.nvicRegs.vectorTable;
    HWREG(NVIC_ACTLR) = PowerCC32XX_contextSave.nvicRegs.auxCtrl;
    HWREG(NVIC_APINT) = PowerCC32XX_contextSave.nvicRegs.appInt;
    HWREG(NVIC_INT_CTRL) = PowerCC32XX_contextSave.nvicRegs.intCtrlState;
    HWREG(NVIC_SYS_CTRL) = PowerCC32XX_contextSave.nvicRegs.sysCtrl;
    HWREG(NVIC_CFG_CTRL) = PowerCC32XX_contextSave.nvicRegs.configCtrl;
    HWREG(NVIC_SYS_PRI1) = PowerCC32XX_contextSave.nvicRegs.sysPri1;
    HWREG(NVIC_SYS_PRI2) = PowerCC32XX_contextSave.nvicRegs.sysPri2;
    HWREG(NVIC_SYS_PRI3) = PowerCC32XX_contextSave.nvicRegs.sysPri3;
    HWREG(NVIC_SYS_HND_CTRL) = PowerCC32XX_contextSave.nvicRegs.sysHcrs;

    /* Systick registers */
    HWREG(NVIC_ST_CTRL) = PowerCC32XX_contextSave.nvicRegs.systickCtrl;
    HWREG(NVIC_ST_RELOAD) = PowerCC32XX_contextSave.nvicRegs.systickReload;
    HWREG(NVIC_ST_CAL) = PowerCC32XX_contextSave.nvicRegs.systickCalib;

    /* Restore the interrupt priority registers */
    base_reg_addr = (uint32_t *)NVIC_PRI0;
    for(i = 0; i < PowerCC32XX_numNVICIntPriority; i++) {
        base_reg_addr[i] = PowerCC32XX_contextSave.nvicRegs.intPriority[i];
    }

    /* Restore the interrupt enable registers */
    base_reg_addr = (uint32_t *)NVIC_EN0;
    for(i = 0; i < PowerCC32XX_numNVICSetEnableRegs; i++) {
        base_reg_addr[i] = PowerCC32XX_contextSave.nvicRegs.intSetEn[i];
    }

    /* Data and instruction sync barriers */
    SYNCBARRIER();
}

/*
 *  ======== restorePeriphClocks ========
 *  Restores the peripheral clocks that had dependency set
 */
static void restorePeriphClocks(void)
{
    uint32_t dependCount;
    uint32_t i;

    /* need to re-enable peripheral clocks to those with set dependency */
    for (i = 0; i < PowerCC32XX_NUMRESOURCES; i++) {
        dependCount = Power_getDependencyCount(i);
        if (dependCount > 0) {
            MAP_PRCMPeripheralClkEnable(PowerCC32XX_module.dbRecords[i],
                PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);

            while(!MAP_PRCMPeripheralStatusGet(PowerCC32XX_module.dbRecords[i])) {
            }
        }
    }
}

/*
 *  ======== saveNVICRegs ========
 *  Save away the NVIC registers for LPDS mode.
 */
static void saveNVICRegs(void)
{
    uint32_t i;
    uint32_t *base_reg_addr;

    /* Save the NVIC control registers */
    PowerCC32XX_contextSave.nvicRegs.vectorTable = HWREG(NVIC_VTABLE);
    PowerCC32XX_contextSave.nvicRegs.auxCtrl = HWREG(NVIC_ACTLR);
    PowerCC32XX_contextSave.nvicRegs.intCtrlState = HWREG(NVIC_INT_CTRL);
    PowerCC32XX_contextSave.nvicRegs.appInt = HWREG(NVIC_APINT);
    PowerCC32XX_contextSave.nvicRegs.sysCtrl = HWREG(NVIC_SYS_CTRL);
    PowerCC32XX_contextSave.nvicRegs.configCtrl = HWREG(NVIC_CFG_CTRL);
    PowerCC32XX_contextSave.nvicRegs.sysPri1 = HWREG(NVIC_SYS_PRI1);
    PowerCC32XX_contextSave.nvicRegs.sysPri2 = HWREG(NVIC_SYS_PRI2);
    PowerCC32XX_contextSave.nvicRegs.sysPri3 = HWREG(NVIC_SYS_PRI3);
    PowerCC32XX_contextSave.nvicRegs.sysHcrs = HWREG(NVIC_SYS_HND_CTRL);

    /* Systick registers */
    PowerCC32XX_contextSave.nvicRegs.systickCtrl = HWREG(NVIC_ST_CTRL);
    PowerCC32XX_contextSave.nvicRegs.systickReload = HWREG(NVIC_ST_RELOAD);
    PowerCC32XX_contextSave.nvicRegs.systickCalib = HWREG(NVIC_ST_CAL);

    /* Save the interrupt enable registers */
    base_reg_addr = (uint32_t *)NVIC_EN0;
    for (i = 0; i < PowerCC32XX_numNVICSetEnableRegs; i++) {
        PowerCC32XX_contextSave.nvicRegs.intSetEn[i] = base_reg_addr[i];
    }

    /* Save the interrupt priority registers */
    base_reg_addr = (uint32_t *)NVIC_PRI0;
    for (i = 0; i < PowerCC32XX_numNVICIntPriority; i++) {
        PowerCC32XX_contextSave.nvicRegs.intPriority[i] = base_reg_addr[i];
    }
}

/*
 *  ======== parkPins ========
 */
static void parkPins(void)
{
    PowerCC32XX_ParkInfo parkInfo;
    uint32_t antpadreg;
    uint32_t i;

    DebugP_assert(PowerCC32XX_config.numPins < PowerCC32XX_NUMPINS + 1);

    /* for each pin in the park array ... */
    for (i = 0; i < PowerCC32XX_config.numPins; i++) {

        parkInfo = PowerCC32XX_config.pinParkDefs[i];

        /* skip this pin if "don't park" is specified */
        if (parkInfo.parkState == PowerCC32XX_DONT_PARK) {
            continue;
        }

        /* if this is a special antenna select pin, stash current pad state */
        if (parkInfo.pin == PowerCC32XX_PIN29) {
            antpadreg = 0x4402E108;
            PowerCC32XX_module.stateAntPin29 = (uint16_t) HWREG(antpadreg);
        }
        else if (parkInfo.pin == PowerCC32XX_PIN30) {
            antpadreg = 0x4402E10C;
            PowerCC32XX_module.stateAntPin30 = (uint16_t) HWREG(antpadreg);
        }
        else {
            antpadreg = 0;
        }

        /* if this is antenna select pin, park via direct writes to pad reg */
        if (antpadreg != 0) {
            HWREG(antpadreg) &= 0xFFFFF0EF;     /* first clear bits 4, 8-11 */
            if (parkInfo.parkState == PowerCC32XX_NO_PULL_HIZ) {
                HWREG(antpadreg) |= 0x00000C00;
            }
            else if (parkInfo.parkState == PowerCC32XX_WEAK_PULL_UP_STD) {
                HWREG(antpadreg) |= 0x00000D00;
            }
            else if (parkInfo.parkState == PowerCC32XX_WEAK_PULL_DOWN_STD) {
                HWREG(antpadreg) |= 0x00000E00;
            }
            else if (parkInfo.parkState == PowerCC32XX_WEAK_PULL_UP_OPENDRAIN) {
                HWREG(antpadreg) |= 0x00000D10;
            }
            else if (parkInfo.parkState ==
                PowerCC32XX_WEAK_PULL_DOWN_OPENDRAIN) {
                HWREG(antpadreg) |= 0x00000E10;
            }
        }

        /* else, for all other pins */
        else {

            /* if pin is NOT to be driven, park it to the specified state... */
            if ((parkInfo.parkState != PowerCC32XX_DRIVE_LOW) &&
                (parkInfo.parkState != PowerCC32XX_DRIVE_HIGH)) {

                PowerCC32XX_parkPin(
                    (PowerCC32XX_Pin)parkInfo.pin,
                    (PowerCC32XX_ParkState)parkInfo.parkState,
                    &PowerCC32XX_module.pinType[i],
                    &PowerCC32XX_module.pinDir[i]);
            }

            /*
             * else, now check if the pin CAN be driven (pins 45, 53, and 55
             * can't be driven)
             */
            else if ((parkInfo.pin != PowerCC32XX_PIN45) &&
                     (parkInfo.pin != PowerCC32XX_PIN53) &&
                     (parkInfo.pin != PowerCC32XX_PIN55)){

                /*
                 * must ensure pin mode is zero; first get/stash current mode,
                 * then set mode to zero
                 */
                PowerCC32XX_module.pinMode[i] =
                    (uint8_t)MAP_PinModeGet(parkInfo.pin);
                MAP_PinModeSet(parkInfo.pin, 0);

                /* if pin is to be driven low, set the lock level to 0 */
                if (parkInfo.parkState == PowerCC32XX_DRIVE_LOW) {
                    MAP_PinLockLevelSet((PowerCC32XX_Pin)parkInfo.pin, 0);
                    PowerCC32XX_module.pinLockMask |= 1 <<
                        PinToPadGet(parkInfo.pin);
                }

                /* else, pin to be driven high, set lock level to 1 */
                else {
                    MAP_PinLockLevelSet((PowerCC32XX_Pin)parkInfo.pin, 1);
                    PowerCC32XX_module.pinLockMask |= 1 <<
                        PinToPadGet(parkInfo.pin);
                }
            }
        }
    }

    /* if any pins are to be driven, lock them now */
    if (PowerCC32XX_module.pinLockMask) {
        MAP_PinLock(PowerCC32XX_module.pinLockMask);
    }
}

/*
 *  ======== restoreParkedPins ========
 */
static void restoreParkedPins(void)
{
    PowerCC32XX_ParkInfo parkInfo;
    uint32_t i;

    DebugP_assert(PowerCC32XX_config.numPins < PowerCC32XX_NUMPINS + 1);

    /* first, unlock any locked pins (that were driven high or low) */
    if (PowerCC32XX_module.pinLockMask) {
        MAP_PinUnlock();
    }

    /* now, for each pin in the park array ... */
    for (i = 0; i < PowerCC32XX_config.numPins; i++) {

        parkInfo = PowerCC32XX_config.pinParkDefs[i];

        /* skip this pin if "don't park" is specified */
        if (parkInfo.parkState == PowerCC32XX_DONT_PARK) {
            continue;
        }

        /* if this is special antenna select pin: restore the saved pad state */
        if (parkInfo.pin == PowerCC32XX_PIN29) {
            HWREG(0x4402E108) = ((HWREG(0x4402E108) & 0xFFFFF000) |
                  (PowerCC32XX_module.stateAntPin29 & 0x00000FFF));
        }

        else if (parkInfo.pin == PowerCC32XX_PIN30) {
            HWREG(0x4402E10C) = ((HWREG(0x4402E10C) & 0xFFFFF000) |
                  (PowerCC32XX_module.stateAntPin30 & 0x00000FFF));
        }

        /* else if pin was driven during LPDS, restore the pin mode */
        else if ((parkInfo.parkState == PowerCC32XX_DRIVE_LOW) ||
            (parkInfo.parkState == PowerCC32XX_DRIVE_HIGH)) {
            MAP_PinModeSet(parkInfo.pin,
                (unsigned long)PowerCC32XX_module.pinMode[i]);
        }

        /* else, restore all others */
        else {
            /* if pin parked in a non-driven state, restore type & direction */
            if ((parkInfo.parkState != PowerCC32XX_DRIVE_LOW) &&
                (parkInfo.parkState != PowerCC32XX_DRIVE_HIGH)) {

                PowerCC32XX_restoreParkedPin(
                    (PowerCC32XX_Pin)parkInfo.pin,
                    PowerCC32XX_module.pinType[i],
                    PowerCC32XX_module.pinDir[i]);
            }
        }
    }
}
