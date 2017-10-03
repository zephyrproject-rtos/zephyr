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
#include <stdbool.h>
#if defined(__IAR_SYSTEMS_ICC__)
#include <intrinsics.h>
#endif

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

#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOCC32XX.h>
#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

/* driverlib header files */
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_gpio.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/gpio.h>
#include <ti/devices/cc32xx/driverlib/pin.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>

/*
 * Map GPIO_INT types to corresponding CC32XX interrupt options
 */
static const uint8_t interruptType[] = {
    0,                  /* Undefined interrupt type */
    GPIO_FALLING_EDGE,  /* 1 = Interrupt on falling edge */
    GPIO_RISING_EDGE,   /* 2 = Interrupt on rising edge */
    GPIO_BOTH_EDGES,    /* 3 = Interrupt on both edges */
    GPIO_LOW_LEVEL,     /* 4 = Interrupt on low level */
    GPIO_HIGH_LEVEL     /* 5 = Interrupt on high level */
};

/*
 * Table of port interrupt vector numbers
 * Used by setCallback() to create Hwis.
 * Up to 4 port interrupts must be supported
 */
static const uint8_t portInterruptIds[] = {
    INT_GPIOA0, INT_GPIOA1,
    INT_GPIOA2, INT_GPIOA3
};

/* Table of GPIO input types */
const uint16_t inPinTypes [] = {
    PIN_TYPE_STD,       /* GPIO_CFG_IN_NOPULL */
    PIN_TYPE_STD_PU,    /* GPIO_CFG_IN_PU */
    PIN_TYPE_STD_PD     /* GPIO_CFG_IN_PD */
};

/* Table of GPIO output types */
const uint16_t outPinTypes [] = {
    PIN_TYPE_STD,       /* GPIO_CFG_OUT_STD */
    PIN_TYPE_OD,        /* GPIO_CFG_OUT_OD_NOPULL */
    PIN_TYPE_OD_PU,     /* GPIO_CFG_OUT_OD_PU */
    PIN_TYPE_OD_PD      /* GPIO_CFG_OUT_OD_PD */
};

/* Table of GPIO drive strengths */
const uint16_t outPinStrengths [] = {
    PIN_STRENGTH_2MA,    /* GPIO_CFG_OUT_STR_LOW */
    PIN_STRENGTH_4MA,    /* GPIO_CFG_OUT_STR_MED */
    PIN_STRENGTH_6MA     /* GPIO_CFG_OUT_STR_HIGH */
};

/*
 * Table of pin numbers (physical device pins) for use with PinTypeGPIO()
 * driverlib call.
 *
 * Indexed by GPIO number (0-31).
 */
#define PIN_XX  0xFF
static const uint8_t pinTable[] = {
    /* 00     01      02      03      04      05      06      07  */
    PIN_50, PIN_55, PIN_57, PIN_58, PIN_59, PIN_60, PIN_61, PIN_62,
    /* 08     09      10      11      12      13      14      15  */
    PIN_63, PIN_64, PIN_01, PIN_02, PIN_03, PIN_04, PIN_05, PIN_06,
    /* 16     17      18      19      20      21      22      23  */
    PIN_07, PIN_08, PIN_XX, PIN_XX, PIN_XX, PIN_XX, PIN_15, PIN_16,
    /* 24     25      26      27      28      29      30      31  */
    PIN_17, PIN_21, PIN_XX, PIN_XX, PIN_18, PIN_20, PIN_53, PIN_45,
    /* 32 */
    PIN_52
};

/*
 * Table of port bases address. For use with most driverlib calls.
 * Indexed by GPIO port number (0-3).
 */
static const uint32_t gpioBaseAddresses[] = {
    GPIOA0_BASE, GPIOA1_BASE,
    GPIOA2_BASE, GPIOA3_BASE,
    GPIOA4_BASE
};

static const uint32_t powerResources[] = {
    PowerCC32XX_PERIPH_GPIOA0,
    PowerCC32XX_PERIPH_GPIOA1,
    PowerCC32XX_PERIPH_GPIOA2,
    PowerCC32XX_PERIPH_GPIOA3,
    PowerCC32XX_PERIPH_GPIOA4
};

#define NUM_PORTS            4
#define NUM_PINS_PER_PORT    8
#define PORT_MASK            0x3

/*
 * Extracts the GPIO interrupt type from the pinConfig.  Value to index into the
 * interruptType table.
 */
#define getIntTypeNumber(pinConfig) \
    ((pinConfig & GPIO_CFG_INT_MASK) >> GPIO_CFG_INT_LSB)

/* Returns the GPIO port base address */
#define getPortBase(port) (gpioBaseAddresses[(port) & PORT_MASK])

/* Returns the GPIO port number */
#define getPort(port) (port & PORT_MASK)

/* Returns the GPIO power resource ID */
#define getPowerResource(port) (powerResources[port & PORT_MASK])

/* Returns GPIO number from the pinConfig */
#define getGpioNumber(pinConfig) \
    (((pinConfig->port & PORT_MASK) * 8) + getPinNumber(pinConfig->pin))

/* Uninitialized callbackInfo pinIndex */
#define CALLBACK_INDEX_NOT_CONFIGURED 0xFF

/*
 * Device specific interpretation of the GPIO_PinConfig content
 */
typedef struct PinConfig {
    uint8_t pin;
    uint8_t port;
    uint16_t config;
} PinConfig;

/*
 * User defined pin indexes assigned to a port's 8 pins.
 * Used by port interrupt function to locate callback assigned
 * to a pin.
 */
typedef struct PortCallbackInfo {
    /*
     * the port's 8 corresponding
     * user defined pinId indices
     */
    uint8_t pinIndex[NUM_PINS_PER_PORT];
} PortCallbackInfo;

/*
 * Table of portCallbackInfos.
 * One for each port.
 */
static PortCallbackInfo gpioCallbackInfo[NUM_PORTS];

/*
 * bit mask used to determine if a Hwi has been created/constructed
 * for a port already.
 * up to NUM_PORTS port interrupts must be supported
 */
static uint8_t portHwiCreatedBitMask = 0;

/*
 *  Bit mask used to keep track of which of the GPIO objects in the config
 *  structure have interrupts enabled.  This will be used to restore the
 *  interrupts after coming out of LPDS.
 */
static uint32_t configIntsEnabledMask = 0;

/*
 * Internal boolean to confirm that GPIO_init() has been called.
 */
static bool initCalled = false;

/* Notification for going into and waking up from LPDS */
static Power_NotifyObj powerNotifyObj;

extern const GPIOCC32XX_Config GPIOCC32XX_config;

static int powerNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg);

/*
 *  ======== getPinNumber ========
 *
 *  Internal function to efficiently find the index of the right most set bit.
 */
static inline uint32_t getPinNumber(uint32_t x) {
#if defined(__TI_COMPILER_VERSION__)
    return (uint32_t)(__clz(__rbit(x)) & 0x7);
#elif defined(codered) || defined(__GNUC__) || defined(sourcerygxx)
    return (uint32_t)(__builtin_ctz(x) & 0x7);
#elif defined(__IAR_SYSTEMS_ICC__)
    return (uint32_t)(__CLZ(__RBIT(x)) & 0x7);
#elif defined(rvmdk) || defined(__ARMCC_VERSION)
    return (uint32_t)(__clz(__rbit(x)) & 0x7);
#else
    #error "Unsupported compiler used"
#endif
}

/*
 *  ======== GPIO_clearInt ========
 */
void GPIO_clearInt(uint_least8_t index)
{
    PinConfig *config = (PinConfig *) &GPIOCC32XX_config.pinConfigs[index];

    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfPinConfigs);

    /* Clear GPIO interrupt flag */
    MAP_GPIOIntClear(getPortBase(config->port), config->pin);

    DebugP_log2("GPIO: port 0x%x, pin 0x%x interrupt flag cleared",
        getPort(config->port), config->pin);
}

/*
 *  ======== GPIO_disableInt ========
 */
void GPIO_disableInt(uint_least8_t index)
{
    uintptr_t  key;
    PinConfig *config = (PinConfig *) &GPIOCC32XX_config.pinConfigs[index];

    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfPinConfigs);

    /* Make atomic update */
    key = HwiP_disable();

    /* Disable GPIO interrupt */
    MAP_GPIOIntDisable(getPortBase(config->port), config->pin);

    configIntsEnabledMask &= ~(1 << index);

    HwiP_restore(key);

    DebugP_log2("GPIO: port 0x%x, pin 0x%x interrupts disabled",
        getPort(config->port), config->pin);
}

/*
 *  ======== GPIO_enableInt ========
 */
void GPIO_enableInt(uint_least8_t index)
{
    uintptr_t  key;
    PinConfig *config = (PinConfig *) &GPIOCC32XX_config.pinConfigs[index];

    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfPinConfigs);

    /* Make atomic update */
    key = HwiP_disable();

    /* Enable GPIO interrupt */
    MAP_GPIOIntEnable(getPortBase(config->port), config->pin);

    configIntsEnabledMask |= (1 << index);

    HwiP_restore(key);

    DebugP_log2("GPIO: port 0x%x, pin 0x%x interrupts enabled",
        getPort(config->port), config->pin);
}

/*
 *  ======== GPIO_getConfig ========
 */
void GPIO_getConfig(uint_least8_t index, GPIO_PinConfig *pinConfig)
{
    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfPinConfigs);

    *pinConfig = GPIOCC32XX_config.pinConfigs[index];
}

/*
 *  ======== GPIO_hwiIntFxn ========
 *  Hwi function that processes GPIO interrupts.
 */
void GPIO_hwiIntFxn(uintptr_t portIndex)
{
    unsigned int      bitNum;
    unsigned int      pinIndex;
    uint32_t          pins;
    uint32_t          portBase;
    PortCallbackInfo *portCallbackInfo;

    portCallbackInfo = &gpioCallbackInfo[portIndex];
    portBase = getPortBase(portIndex);

    /* Find out which pins have their interrupt flags set */
    pins = MAP_GPIOIntStatus(portBase, 0xFF) & 0xFF;

    /* clear all the set bits at once */
    MAP_GPIOIntClear(portBase, pins);

    /* Match the interrupt to its corresponding callback function */
    while (pins) {
        /* Gets the lowest order set bit number */
        bitNum = getPinNumber(pins);
        pinIndex = portCallbackInfo->pinIndex[bitNum & 0x7];
        /* only call plugged callbacks */
        if (pinIndex != CALLBACK_INDEX_NOT_CONFIGURED) {
            GPIOCC32XX_config.callbacks[pinIndex](pinIndex);
        }
        pins &= ~(1 << bitNum);
    }
}

/*
 *  ======== GPIO_init ========
 */
void GPIO_init()
{
    unsigned int i, j, hwiKey;
    SemaphoreP_Handle sem;
    static SemaphoreP_Handle initSem;

    /* speculatively create a binary semaphore */
    sem = SemaphoreP_createBinary(1);

    /* There is no way to inform user of this fatal error. */
    if (sem == NULL) return;

    hwiKey = HwiP_disable();

    if (initSem == NULL) {
        initSem = sem;
        HwiP_restore(hwiKey);
    }
    else {
        /* init already called */
        HwiP_restore(hwiKey);
        /* delete unused Semaphore */
        if (sem) SemaphoreP_delete(sem);
    }

    /* now use the semaphore to protect init code */
    SemaphoreP_pend(initSem, SemaphoreP_WAIT_FOREVER);

    /* Only perform init once */
    if (initCalled) {
        SemaphoreP_post(initSem);
        return;
    }

    for (i = 0; i < NUM_PORTS; i++) {
        for (j = 0; j < NUM_PINS_PER_PORT; j++) {
            gpioCallbackInfo[i].pinIndex[j] = CALLBACK_INDEX_NOT_CONFIGURED;
        }
    }

    /*
     * Configure pins and create Hwis per static array content
     */
    for (i = 0; i < GPIOCC32XX_config.numberOfPinConfigs; i++) {
        if (!(GPIOCC32XX_config.pinConfigs[i] & GPIO_DO_NOT_CONFIG)) {

            GPIO_setConfig(i, GPIOCC32XX_config.pinConfigs[i]);
        }
        if (i < GPIOCC32XX_config.numberOfCallbacks) {
            if (GPIOCC32XX_config.callbacks[i] != NULL) {
                /* create Hwi as necessary */
                GPIO_setCallback(i, GPIOCC32XX_config.callbacks[i]);
            }
        }
    }

    Power_registerNotify(&powerNotifyObj,
            PowerCC32XX_ENTERING_LPDS | PowerCC32XX_AWAKE_LPDS,
            powerNotifyFxn, (uintptr_t) NULL);

    initCalled = true;

    SemaphoreP_post(initSem);
}

/*
 *  ======== GPIO_read ========
 */
uint_fast8_t GPIO_read(uint_least8_t index)
{
    unsigned int value;

    PinConfig *config = (PinConfig *) &GPIOCC32XX_config.pinConfigs[index];

    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfPinConfigs);

    value = MAP_GPIOPinRead(getPortBase(config->port), config->pin);

    DebugP_log3("GPIO: port 0x%x, pin 0x%x read 0x%x",
        getPort(config->port), config->pin, value);

    value = (value & config->pin) ? 1 : 0;

    return (value);
}

/*
 *  ======== GPIO_setCallback ========
 */
void GPIO_setCallback(uint_least8_t index, GPIO_CallbackFxn callback)
{
    uint32_t   pinNum;
    uint32_t   portIndex;
    PinConfig *config = (PinConfig *) &GPIOCC32XX_config.pinConfigs[index];

    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfCallbacks);

    /*
     * plug the pin index into the corresponding
     * port's callbackInfo pinIndex entry
     */
    pinNum = getPinNumber(config->pin);
    portIndex = config->port & PORT_MASK;

    if (callback == NULL) {
        gpioCallbackInfo[portIndex].pinIndex[pinNum] =
            CALLBACK_INDEX_NOT_CONFIGURED;
    }
    else {
        gpioCallbackInfo[portIndex].pinIndex[pinNum] = index;
    }

    /*
     * Only update callBackFunctions entry if different.
     * This allows the callBackFunctions array to be in flash for static systems.
     */
    if (GPIOCC32XX_config.callbacks[index] != callback) {
        GPIOCC32XX_config.callbacks[index] = callback;
    }
}

/*
 *  ======== GPIO_setConfig ========
 */
int_fast16_t GPIO_setConfig(uint_least8_t index, GPIO_PinConfig pinConfig)
{
    uintptr_t      key;
    uint32_t       pinMask;
    uint32_t       pin;
    uint32_t       portBase;
    uint32_t       portIndex;
    uint32_t       portBitMask;
    uint16_t       direction;
    uint16_t       strength;
    uint16_t       pinType;
    HwiP_Handle    hwiHandle;
    HwiP_Params    hwiParams;
    GPIO_PinConfig gpioPinConfig;
    PinConfig     *config = (PinConfig *) &GPIOCC32XX_config.pinConfigs[index];

    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfPinConfigs);

    if (pinConfig & GPIO_DO_NOT_CONFIG) {
        return (GPIO_STATUS_SUCCESS);
    }

    portBase = getPortBase(config->port);
    pinMask = config->pin;
    pin = pinTable[getGpioNumber(config)];

    /* Make atomic update */
    key = HwiP_disable();

    /* set the pin's pinType to GPIO */
    MAP_PinModeSet(pin, PIN_MODE_0);

    /* enable clocks for the GPIO port */
    Power_setDependency(getPowerResource(config->port));

    HwiP_restore(key);

    if ((pinConfig & GPIO_CFG_IN_INT_ONLY) == 0) {
        if (pinConfig & GPIO_CFG_INPUT) {
            /* configure input */
            direction = GPIO_DIR_MODE_IN;
            strength = PIN_STRENGTH_2MA;
            pinType = inPinTypes[(pinConfig & GPIO_CFG_IN_TYPE_MASK) >>
                GPIO_CFG_IN_TYPE_LSB];
        }
        else {
            /* configure output */
            direction = GPIO_DIR_MODE_OUT;
            strength =
                outPinStrengths[(pinConfig & GPIO_CFG_OUT_STRENGTH_MASK) >>
                GPIO_CFG_OUT_STRENGTH_LSB];
            pinType = outPinTypes[(pinConfig & GPIO_CFG_OUT_TYPE_MASK) >>
                GPIO_CFG_OUT_TYPE_LSB];
        }

        key = HwiP_disable();

        /* Configure the GPIO pin */
        MAP_GPIODirModeSet(portBase, pinMask, direction);
        MAP_PinConfigSet(pin, strength, pinType);

        /* Set output value */
        if (direction == GPIO_DIR_MODE_OUT) {
            MAP_GPIOPinWrite(portBase, pinMask,
                ((pinConfig & GPIO_CFG_OUT_HIGH) ? 0xFF : 0));
        }

        /*
         *  Update pinConfig with the latest GPIO configuration and
         *  clear the GPIO_DO_NOT_CONFIG bit if it was set.
         */
        gpioPinConfig = GPIOCC32XX_config.pinConfigs[index];
        gpioPinConfig &= ~(GPIO_CFG_IO_MASK | GPIO_DO_NOT_CONFIG);
        gpioPinConfig |= (pinConfig & GPIO_CFG_IO_MASK);
        GPIOCC32XX_config.pinConfigs[index] = gpioPinConfig;

        HwiP_restore(key);
    }

    /* Set type of interrupt and then clear it */
    if (pinConfig & GPIO_CFG_INT_MASK) {
        portIndex = config->port & PORT_MASK;
        portBitMask = 1 << portIndex;

        /* if Hwi has not already been created, do so */
        if ((portHwiCreatedBitMask & portBitMask) == 0) {
            HwiP_Params_init(&hwiParams);
            hwiParams.arg = (uintptr_t) portIndex;
            hwiParams.priority = GPIOCC32XX_config.intPriority;
            hwiHandle = HwiP_create(portInterruptIds[portIndex], GPIO_hwiIntFxn,
                &hwiParams);
            if (hwiHandle == NULL) {
                /* Error creating Hwi */
                DebugP_log1("GPIO: Error constructing Hwi for GPIO Port %d",
                    getPort(config->port));
                return (GPIO_STATUS_ERROR);
            }
        }

        key = HwiP_disable();

        /* Mark the Hwi as created */
        portHwiCreatedBitMask |= portBitMask;

        MAP_GPIOIntTypeSet(portBase, pinMask,
            interruptType[getIntTypeNumber(pinConfig)]);
        MAP_GPIOIntClear(portBase, pinMask);

        /*
         *  Update pinConfig with the latest interrupt configuration and
         *  clear the GPIO_DO_NOT_CONFIG bit if it was set.
         */
        gpioPinConfig = GPIOCC32XX_config.pinConfigs[index];
        gpioPinConfig &= ~(GPIO_CFG_INT_MASK | GPIO_DO_NOT_CONFIG);
        gpioPinConfig |= (pinConfig & GPIO_CFG_INT_MASK);
        GPIOCC32XX_config.pinConfigs[index] = gpioPinConfig;

        HwiP_restore(key);
    }

    return (GPIO_STATUS_SUCCESS);
}

/*
 *  ======== GPIO_toggle ========
 */
void GPIO_toggle(uint_least8_t index)
{
    uintptr_t  key;
    uint32_t   value;
    PinConfig *config = (PinConfig *) &GPIOCC32XX_config.pinConfigs[index];

    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfPinConfigs);
    DebugP_assert((GPIOCC32XX_config.pinConfigs[index] & GPIO_CFG_INPUT) ==
        GPIO_CFG_OUTPUT);

    /* Make atomic update */
    key = HwiP_disable();

    value = MAP_GPIOPinRead(getPortBase(config->port), config->pin);
    value ^= (uint32_t)config->pin;
    MAP_GPIOPinWrite(getPortBase(config->port), config->pin, value);

    /* Update config table entry with value written */
    GPIOCC32XX_config.pinConfigs[index] ^= GPIO_CFG_OUT_HIGH;

    HwiP_restore(key);

    DebugP_log2("GPIO: port 0x%x, pin 0x%x toggled",
        getPort(config->port), config->pin);
}

/*
 *  ======== GPIO_write ========
 */
void GPIO_write(uint_least8_t index, unsigned int value)
{
    uintptr_t  key;
    uint32_t   output;
    PinConfig *config = (PinConfig *) &GPIOCC32XX_config.pinConfigs[index];

    DebugP_assert(initCalled && index < GPIOCC32XX_config.numberOfPinConfigs);
    DebugP_assert((GPIOCC32XX_config.pinConfigs[index] & GPIO_CFG_INPUT) ==
        GPIO_CFG_OUTPUT);

    key = HwiP_disable();

    /* Clear output from pinConfig */
    GPIOCC32XX_config.pinConfigs[index] &= ~GPIO_CFG_OUT_HIGH;

    if (value) {
        output = config->pin;

        /* Set the pinConfig output bit to high */
        GPIOCC32XX_config.pinConfigs[index] |= GPIO_CFG_OUT_HIGH;
    }
    else {
        output = value;
    }

    MAP_GPIOPinWrite(getPortBase(config->port), config->pin, output);

    HwiP_restore(key);

    DebugP_log3("GPIO: port 0x%x, pin 0x%x wrote 0x%x",
       getPort(config->port), config->pin, value);
}

/*
 *  ======== powerNotifyFxn ========
 */
static int powerNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg)
{
    unsigned int   i;
    GPIO_PinConfig config;
    uint32_t       output;
    uint32_t       pin;
    PinConfig     *pinConfig;
    PowerCC32XX_ParkState state;

    if (eventType == PowerCC32XX_AWAKE_LPDS) {

        for (i = 0; i < GPIOCC32XX_config.numberOfPinConfigs; i++) {
            if (!(GPIOCC32XX_config.pinConfigs[i] & GPIO_DO_NOT_CONFIG)) {
                config = GPIOCC32XX_config.pinConfigs[i];

                GPIO_setConfig(i, config);

                if (configIntsEnabledMask & (1 << i)) {
                    GPIO_enableInt(i);
                }
            }
        }
    }
    else {
        /* Entering LPDS */
        /*
         *  For pins configured as GPIO output, if the GPIOCC32XX_USE_STATIC
         *  configuration flag is *not* set, get the current pin state, and
         *  then call to the Power manager to define the state to be held
         *  during LPDS.
         *  If GPIOCC32XX_USE_STATIC *is* defined, do nothing, and the pin
         *  will be parked in the state statically defined in
         *  PowerCC32XX_config.pinParkDefs[] in the board file.
         */
        for (i = 0; i < GPIOCC32XX_config.numberOfPinConfigs; i++) {
            if (GPIOCC32XX_config.pinConfigs[i] & GPIO_DO_NOT_CONFIG) {
                continue;
            }

            config =  GPIOCC32XX_config.pinConfigs[i];

            /* if OUTPUT, and GPIOCC32XX_USE_STATIC flag is not set */
            if ((!(config & GPIO_CFG_INPUT)) &&
                (!(config & GPIOCC32XX_USE_STATIC))) {

                pinConfig = (PinConfig *) &GPIOCC32XX_config.pinConfigs[i];

                /* determine state to be held */
                pin = pinTable[getGpioNumber(pinConfig)];
                output = config & GPIO_CFG_OUT_HIGH;
                state = (PowerCC32XX_ParkState) ((output) ? 1 : 0);

                /* set the new park state */
                PowerCC32XX_setParkState((PowerCC32XX_Pin)pin, state);
            }
        }
    }

    return (Power_NOTIFYDONE);
}
