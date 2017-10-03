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
/** ============================================================================
 *  @file       PowerCC32XX.h
 *
 *  @brief      Power manager interface for the CC32XX
 *
 *  The Power header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/Power.h>
 *  #include <ti/drivers/power/PowerCC32XX.h>
 *  @endcode
 *
 *  Refer to @ref Power.h for a complete description of APIs.
 *
 *  ## Implementation #
 *  This module defines the power resources, constraints, events, sleep
 *  states and transition latencies for CC32XX.
 *
 *  A reference power policy is provided which can transition the MCU from the
 *  active state to one of two sleep states: LPDS or Sleep.
 *  The policy looks at the estimated idle time remaining, and the active
 *  constraints, and determine which sleep state to transition to.  The
 *  policy will give first preference to choosing LPDS, but if that is not
 *  appropriate (e.g., not enough idle time), it will choose Sleep.
 *
 *  ============================================================================
 */

#ifndef ti_drivers_power_PowerCC32XX__include
#define ti_drivers_power_PowerCC32XX__include

#include <stdint.h>
#include <ti/drivers/utils/List.h>
#include <ti/drivers/Power.h>

/* driverlib header files */
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/pin.h>

#ifdef __cplusplus
extern "C" {
#endif

/* latency values were measured with a logic analyzer, and rounded up */

/*! The latency to reserve for resuming from LPDS (usec) */
#define PowerCC32XX_RESUMETIMELPDS          2500

/*! The total latency to reserve for entry to and exit from LPDS (usec) */
#define PowerCC32XX_TOTALTIMELPDS           20000

/*! The total latency to reserve for entry to and exit from Shutdown (usec) */
#define PowerCC32XX_TOTALTIMESHUTDOWN       500000

/* Power resources */
#define PowerCC32XX_PERIPH_CAMERA       0
/*!< Resource ID: Camera */

#define PowerCC32XX_PERIPH_I2S          1
/*!< Resource ID: I2S */

#define PowerCC32XX_PERIPH_SDHOST       2
/*!< Resource ID: SDHost */

#define PowerCC32XX_PERIPH_GSPI         3
/*!< Resource ID: General Purpose SPI (GSPI) */

#define PowerCC32XX_PERIPH_LSPI         4
/*!< Resource ID: LSPI */

#define PowerCC32XX_PERIPH_UDMA         5
/*!< Resource ID: uDMA Controller */

#define PowerCC32XX_PERIPH_GPIOA0       6
/*!< Resource ID: General Purpose I/O Port A0 */

#define PowerCC32XX_PERIPH_GPIOA1       7
/*!< Resource ID: General Purpose I/O Port A1 */

#define PowerCC32XX_PERIPH_GPIOA2       8
/*!< Resource ID: General Purpose I/O Port A2 */

#define PowerCC32XX_PERIPH_GPIOA3       9
/*!< Resource ID: General Purpose I/O Port A3 */

#define PowerCC32XX_PERIPH_GPIOA4       10
/*!< Resource ID: General Purpose I/O Port A4 */

#define PowerCC32XX_PERIPH_WDT          11
/*!< Resource ID: Watchdog module */

#define PowerCC32XX_PERIPH_UARTA0       12
/*!< Resource ID: UART 0 */

#define PowerCC32XX_PERIPH_UARTA1       13
/*!< Resource ID: UART 1 */

#define PowerCC32XX_PERIPH_TIMERA0      14
/*!< Resource ID: General Purpose Timer A0 */

#define PowerCC32XX_PERIPH_TIMERA1      15
/*!< Resource ID: General Purpose Timer A1 */

#define PowerCC32XX_PERIPH_TIMERA2      16
/*!< Resource ID: General Purpose Timer A2 */

#define PowerCC32XX_PERIPH_TIMERA3      17
/*!< Resource ID: General Purpose Timer A3 */

#define PowerCC32XX_PERIPH_DTHE         18
/*!< Resource ID: Cryptography Accelerator (DTHE) */

#define PowerCC32XX_PERIPH_SSPI         19
/*!< Resource ID: Serial Flash SPI (SSPI) */

#define PowerCC32XX_PERIPH_I2CA0        20
/*!< Resource ID: I2C */

/* \cond */
#define PowerCC32XX_NUMRESOURCES        21 /* Number of resources in database */
/* \endcond */

/*
 *  Power constraints on the CC32XX device
 */
#define PowerCC32XX_DISALLOW_LPDS       0
/*!< Constraint: Disallow entry to Low Power Deep Sleep (LPDS) */

#define PowerCC32XX_DISALLOW_SHUTDOWN   1
/*!< Constraint: Disallow entry to Shutdown */

/* \cond */
#define PowerCC32XX_NUMCONSTRAINTS      2   /*!< number of constraints */
/* \endcond */

/*
 *  Power events on the CC32XX device
 *
 *  Each event must be a power of two, and the event IDs must be sequential
 *  without any gaps.
 */
#define PowerCC32XX_ENTERING_LPDS       0x1
/*!< Power event: The device is entering the LPDS sleep state */

#define PowerCC32XX_ENTERING_SHUTDOWN   0x2
/*!< Power event: The device is entering the Shutdown state */

#define PowerCC32XX_AWAKE_LPDS          0x4
/*!< Power event: The device is waking from the LPDS sleep state */

/* \cond */
#define PowerCC32XX_NUMEVENTS           3    /*!< number of events */
/* \endcond */

/* Power sleep states */
#define PowerCC32XX_LPDS                0x1 /*!< The LPDS sleep state */

/* \cond */
/* Use by NVIC Register structure */
#define PowerCC32XX_numNVICSetEnableRegs    6
#define PowerCC32XX_numNVICIntPriority      49
/* \endcond */

/* \cond */
/* Number of pins that can be parked in LPDS */
#define PowerCC32XX_NUMPINS             34
/* \endcond */

/*! @brief  Used to specify parking of a pin during LPDS */
typedef struct PowerCC32XX_ParkInfo {
    uint32_t pin;
    /*!< The pin to be parked */
    uint32_t parkState;
    /*!< The state to park the pin (an enumerated PowerCC32XX_ParkState) */
} PowerCC32XX_ParkInfo;

/*! @brief Power global configuration */
typedef struct PowerCC32XX_ConfigV1 {
    /*! Initialization function for the power policy */
    Power_PolicyInitFxn policyInitFxn;
    /*! The power policy function */
    Power_PolicyFxn policyFxn;
    /*!
     *  @brief  Hook function called before entering LPDS
     *
     *  This function is called after any notifications are complete,
     *  and before any pins are parked, just before entry to LPDS.
     */
    void (*enterLPDSHookFxn)(void);
    /*!
     *  @brief  Hook function called when resuming from LPDS
     *
     *  This function is called early in the wake sequence, before any
     *  notification functions are run.
     */
    void (*resumeLPDSHookFxn)(void);
    /*! Determines whether to run the power policy function */
    bool enablePolicy;
    /*! Enable GPIO as a wakeup source for LPDS */
    bool enableGPIOWakeupLPDS;
    /*! Enable GPIO as a wakeup source for shutdown */
    bool enableGPIOWakeupShutdown;
    /*! Enable Network activity as a wakeup source for LPDS */
    bool enableNetworkWakeupLPDS;
    /*!
     *  @brief  The GPIO source for wakeup from LPDS
     *
     *  Only one GPIO {2,4,11,13,17,24,26} can be specified as a wake source
     *  for LPDS.  The GPIO must be specified as one of the following (as
     *  defined in driverlib/prcm.h): PRCM_LPDS_GPIO2, PRCM_LPDS_GPIO4,
     *  PRCM_LPDS_GPIO11, PRCM_LPDS_GPIO13, PRCM_LPDS_GPIO17, PRCM_LPDS_GPIO24,
     *  PRCM_LPDS_GPIO26
     */
    uint32_t wakeupGPIOSourceLPDS;
    /*!
     *  @brief  The GPIO trigger type for wakeup from LPDS
     *
     *  Value can be one of the following (defined in driverlib/prcm.h):
     *  PRCM_LPDS_LOW_LEVEL, PRCM_LPDS_HIGH_LEVEL,
     *  PRCM_LPDS_FALL_EDGE, PRCM_LPDS_RISE_EDGE
     */
    uint32_t wakeupGPIOTypeLPDS;
    /*!
     *  @brief  Function to be called when the configured GPIO triggers wakeup
     *  from LPDS
     *
     *  During LPDS the internal GPIO module is powered off, and special
     *  periphery logic is used instead to detect the trigger and wake the
     *  device.  No GPIO interrupt service routine will be triggered in this
     *  case (even if an ISR is configured, and used normally to detect GPIO
     *  interrupts when not in LPDS).  This function can be used in lieu of a
     *  GPIO ISR, to take specific action upon LPDS wakeup.
     *
     *  A value of NULL indicates no GPIO wakeup function will be called.
     *
     *  An argument for this wakeup function can be specified via
     *  wakeupGPIOFxnLPDSArg.
     *
     *  Note that this wakeup function will be called as one of the last steps
     *  in Power_sleep(), after all notifications have been sent out, and after
     *  pins have been restored to their previous (non-parked) states.
     */
    void (*wakeupGPIOFxnLPDS)(uint_least8_t argument);
    /*!
     *  @brief  The argument to be passed to wakeupGPIOFxnLPDS()
     */
    uint_least8_t wakeupGPIOFxnLPDSArg;
    /*!
     *  @brief  The GPIO sources for wakeup from shutdown
     *
     *  Only one GPIO {2,4,11,13,17,24,26} can be specified as a wake source
     *  for Shutdown.  The GPIO must be specified as one of the following (as
     *  defined in driverlib/prcm.h): PRCM_HIB_GPIO2, PRCM_HIB_GPIO4,
     *  PRCM_HIB_GPIO11, PRCM_HIB_GPIO13, PRCM_HIB_GPIO17, PRCM_HIB_GPIO24,
     *  PRCM_HIB_GPIO26
     */
    uint32_t wakeupGPIOSourceShutdown;
    /*!
     *  @brief  The GPIO trigger type for wakeup from shutdown
     *
     *  Value can be one of the following (defined in driverlib/prcm.h):
     *  PRCM_HIB_LOW_LEVEL, PRCM_HIB_HIGH_LEVEL,
     *  PRCM_HIB_FALL_EDGE, PRCM_HIB_RISE_EDGE
     */
    uint32_t wakeupGPIOTypeShutdown;
    /*!
     *  @brief  SRAM retention mask for LPDS
     *
     *  Value can be a mask of the following (defined in driverlib/prcm.h):
     *  PRCM_SRAM_COL_1, PRCM_SRAM_COL_2, PRCM_SRAM_COL_3,
     *  PRCM_SRAM_COL_4
     */
    uint32_t ramRetentionMaskLPDS;
    /*!
     *  @brief  Keep debug interface active during LPDS
     *
     *  This Boolean controls whether the debug interface will be left active
     *  when LPDS is entered.  For best power savings this flag should be set
     *  to false.  Setting the flag to true will enable better debug
     *  capability, but will prevent full LPDS, and will result in increased
     *  power consumption.
     */
    bool keepDebugActiveDuringLPDS;
    /*!
     *  @brief  IO retention mask for Shutdown
     *
     *  Value can be a mask of the following (defined in driverlib/prcm.h):
     *  PRCM_IO_RET_GRP_0, PRCM_IO_RET_GRP_1, PRCM_IO_RET_GRP_2
     *  PRCM_IO_RET_GRP_3
     */
    uint32_t ioRetentionShutdown;
    /*!
     *  @brief  Pointer to an array of pins to be parked during LPDS
     *
     *  A value of NULL will disable parking of any pins during LPDS
     */
    PowerCC32XX_ParkInfo * pinParkDefs;
    /*!
     *  @brief  Number of pins to be parked during LPDS
     */
    uint32_t numPins;
} PowerCC32XX_ConfigV1;

/*!
 *  @cond NODOC
 *  NVIC registers that need to be saved before entering LPDS.
 */
typedef struct PowerCC32XX_NVICRegisters {
    uint32_t vectorTable;
    uint32_t auxCtrl;
    uint32_t intCtrlState;
    uint32_t appInt;
    uint32_t sysCtrl;
    uint32_t configCtrl;
    uint32_t sysPri1;
    uint32_t sysPri2;
    uint32_t sysPri3;
    uint32_t sysHcrs;
    uint32_t systickCtrl;
    uint32_t systickReload;
    uint32_t systickCalib;
    uint32_t intSetEn[PowerCC32XX_numNVICSetEnableRegs];
    uint32_t intPriority[PowerCC32XX_numNVICIntPriority];
} PowerCC32XX_NVICRegisters;
/*! @endcond */

/*!
 *  @cond NODOC
 *  MCU core registers that need to be save before entering LPDS.
 */
typedef struct PowerCC32XX_MCURegisters {
    uint32_t msp;
    uint32_t psp;
    uint32_t psr;
    uint32_t primask;
    uint32_t faultmask;
    uint32_t basepri;
    uint32_t control;
} PowerCC32XX_MCURegisters;
/*! @endcond */

/*!
 *  @cond NODOC
 *  Structure of context registers to save before entering LPDS.
 */
typedef struct PowerCC32XX_SaveRegisters {
    PowerCC32XX_MCURegisters m4Regs;
    PowerCC32XX_NVICRegisters nvicRegs;
} PowerCC32XX_SaveRegisters;
/*! @endcond */

/*! @brief Enumeration of states a pin can be parked in */
typedef enum {
    /*! No pull resistor, leave pin in a HIZ state */
    PowerCC32XX_NO_PULL_HIZ = PIN_TYPE_STD,
    /*! Pull-up resistor for standard pin type */
    PowerCC32XX_WEAK_PULL_UP_STD = PIN_TYPE_STD_PU,
    /*! Pull-down resistor for standard pin type */
    PowerCC32XX_WEAK_PULL_DOWN_STD = PIN_TYPE_STD_PD,
    /*! Pull-up resistor for open drain pin type */
    PowerCC32XX_WEAK_PULL_UP_OPENDRAIN = PIN_TYPE_OD_PU,
    /*! Pull-down resistor for open drain pin type */
    PowerCC32XX_WEAK_PULL_DOWN_OPENDRAIN = PIN_TYPE_OD_PD,
    /*! Drive pin to a low logic state */
    PowerCC32XX_DRIVE_LOW,
    /*! Drive pin to a high logic state */
    PowerCC32XX_DRIVE_HIGH,
    /*! Take no action; do not park the pin */
    PowerCC32XX_DONT_PARK
} PowerCC32XX_ParkState;

/*! @brief Enumeration of pins that can be parked */
typedef enum {
    /*! PIN_01 */
    PowerCC32XX_PIN01 = PIN_01,
    /*! PIN_02 */
    PowerCC32XX_PIN02 = PIN_02,
    /*! PIN_03 */
    PowerCC32XX_PIN03 = PIN_03,
    /*! PIN_04 */
    PowerCC32XX_PIN04 = PIN_04,
    /*! PIN_05 */
    PowerCC32XX_PIN05 = PIN_05,
    /*! PIN_06 */
    PowerCC32XX_PIN06 = PIN_06,
    /*! PIN_07 */
    PowerCC32XX_PIN07 = PIN_07,
    /*! PIN_08 */
    PowerCC32XX_PIN08 = PIN_08,
    /*! PIN_11 */
    PowerCC32XX_PIN11 = PIN_11,
    /*! PIN_12 */
    PowerCC32XX_PIN12 = PIN_12,
    /*! PIN_13 */
    PowerCC32XX_PIN13 = PIN_13,
    /*! PIN_14 */
    PowerCC32XX_PIN14 = PIN_14,
    /*! PIN_15 */
    PowerCC32XX_PIN15 = PIN_15,
    /*! PIN_16 */
    PowerCC32XX_PIN16 = PIN_16,
    /*! PIN_17 */
    PowerCC32XX_PIN17 = PIN_17,
    /*! PIN_18 */
    PowerCC32XX_PIN18 = PIN_18,
    /*! PIN_19 */
    PowerCC32XX_PIN19 = PIN_19,
    /*! PIN_20 */
    PowerCC32XX_PIN20 = PIN_20,
    /*! PIN_21 */
    PowerCC32XX_PIN21 = PIN_21,
    /*! PIN_29 */
    PowerCC32XX_PIN29 = 0x1C,
    /*! PIN_30 */
    PowerCC32XX_PIN30 = 0x1D,
    /*! PIN_45 */
    PowerCC32XX_PIN45 = PIN_45,
    /*! PIN_50 */
    PowerCC32XX_PIN50 = PIN_50,
    /*! PIN_52 */
    PowerCC32XX_PIN52 = PIN_52,
    /*! PIN_53 */
    PowerCC32XX_PIN53 = PIN_53,
    /*! PIN_55 */
    PowerCC32XX_PIN55 = PIN_55,
    /*! PIN_57 */
    PowerCC32XX_PIN57 = PIN_57,
    /*! PIN_58 */
    PowerCC32XX_PIN58 = PIN_58,
    /*! PIN_59 */
    PowerCC32XX_PIN59 = PIN_59,
    /*! PIN_60 */
    PowerCC32XX_PIN60 = PIN_60,
    /*! PIN_61 */
    PowerCC32XX_PIN61 = PIN_61,
    /*! PIN_62 */
    PowerCC32XX_PIN62 = PIN_62,
    /*! PIN_63 */
    PowerCC32XX_PIN63 = PIN_63,
    /*! PIN_64 */
    PowerCC32XX_PIN64 = PIN_64
} PowerCC32XX_Pin;

/*!
 *  @brief  Specify the wakeup sources for LPDS and Shutdown
 *
 *  The wakeup sources for LPDS and Shutdown can be dynamically changed
 *  at runtime, via PowerCC32XX_configureWakeup().  The application
 *  should fill a structure of this type, and pass it as the parameter
 *  to PowerCC32XX_configureWakeup() to specify the new wakeup settings.
 */
typedef struct PowerCC32XX_Wakeup {
    /*! Enable GPIO as a wakeup source for LPDS */
    bool enableGPIOWakeupLPDS;
    /*! Enable GPIO as a wakeup source for shutdown */
    bool enableGPIOWakeupShutdown;
    /*! Enable Network activity as a wakeup source for LPDS */
    bool enableNetworkWakeupLPDS;
    /*!
     *  @brief  The GPIO source for wakeup from LPDS
     *
     *  Only one GPIO {2,4,11,13,17,24,26} can be specified as a wake source
     *  for LPDS.  The GPIO must be specified as one of the following (as
     *  defined in driverlib/prcm.h): PRCM_LPDS_GPIO2, PRCM_LPDS_GPIO4,
     *  PRCM_LPDS_GPIO11, PRCM_LPDS_GPIO13, PRCM_LPDS_GPIO17, PRCM_LPDS_GPIO24,
     *  PRCM_LPDS_GPIO26
     */
    uint32_t wakeupGPIOSourceLPDS;
    /*!
     *  @brief  The GPIO trigger type for wakeup from LPDS
     *
     *  Value can be one of the following (defined in driverlib/prcm.h):
     *  PRCM_LPDS_LOW_LEVEL, PRCM_LPDS_HIGH_LEVEL,
     *  PRCM_LPDS_FALL_EDGE, PRCM_LPDS_RISE_EDGE
     */
    uint32_t wakeupGPIOTypeLPDS;
    /*!
     *  @brief  Function to be called when the configured GPIO triggers wakeup
     *  from LPDS
     *
     *  During LPDS the internal GPIO module is powered off, and special
     *  periphery logic is used instead to detect the trigger and wake the
     *  device.  No GPIO interrupt service routine will be triggered in this
     *  case (even if an ISR is configured, and used normally to detect GPIO
     *  interrupts when not in LPDS).  This function can be used in lieu of a
     *  GPIO ISR, to take specific action upon LPDS wakeup.
     *
     *  A value of NULL indicates no GPIO wakeup function will be called.
     *
     *  An argument for this wakeup function can be specified via
     *  wakeupGPIOFxnLPDSArg.
     *
     *  Note that this wakeup function will be called as one of the last steps
     *  in Power_sleep(), after all notifications have been sent out, and after
     *  pins have been restored to their previous (non-parked) states.
     */
    void (*wakeupGPIOFxnLPDS)(uint_least8_t argument);
    /*!
     *  @brief  The argument to be passed to wakeupGPIOFxnLPDS()
     */
    uint_least8_t wakeupGPIOFxnLPDSArg;
    /*!
     *  @brief  The GPIO sources for wakeup from shutdown
     *
     *  Only one GPIO {2,4,11,13,17,24,26} can be specified as a wake source
     *  for Shutdown.  The GPIO must be specified as one of the following (as
     *  defined in driverlib/prcm.h): PRCM_HIB_GPIO2, PRCM_HIB_GPIO4,
     *  PRCM_HIB_GPIO11, PRCM_HIB_GPIO13, PRCM_HIB_GPIO17, PRCM_HIB_GPIO24,
     *  PRCM_HIB_GPIO26
     */
    uint32_t wakeupGPIOSourceShutdown;
    /*!
     *  @brief  The GPIO trigger type for wakeup from shutdown
     *
     *  Value can be one of the following (defined in driverlib/prcm.h):
     *  PRCM_HIB_LOW_LEVEL, PRCM_HIB_HIGH_LEVEL,
     *  PRCM_HIB_FALL_EDGE, PRCM_HIB_RISE_EDGE
     */
    uint32_t wakeupGPIOTypeShutdown;
} PowerCC32XX_Wakeup;

/*!
 *  @cond NODOC
 *  Internal structure defining Power module state.
 */
typedef struct PowerCC32XX_ModuleState {
    List_List notifyList;
    uint32_t constraintMask;
    uint32_t state;
    uint16_t dbRecords[PowerCC32XX_NUMRESOURCES];
    bool enablePolicy;
    bool initialized;
    uint8_t refCount[PowerCC32XX_NUMRESOURCES];
    uint8_t constraintCounts[PowerCC32XX_NUMCONSTRAINTS];
    Power_PolicyFxn policyFxn;
    uint32_t pinType[PowerCC32XX_NUMPINS];
    uint16_t pinDir[PowerCC32XX_NUMPINS];
    uint8_t pinMode[PowerCC32XX_NUMPINS];
    uint16_t stateAntPin29;
    uint16_t stateAntPin30;
    uint32_t pinLockMask;
    PowerCC32XX_Wakeup wakeupConfig;
} PowerCC32XX_ModuleState;
/*! @endcond */

/*!
 *  @brief  Function configures wakeup for LPDS and shutdown
 *
 *  This function allows the app to configure the GPIO source and
 *  type for waking up from LPDS and shutdown and the network host
 *  as a wakeup source for LPDS.  This overwrites any previous
 *  wakeup settings.
 *
 *  @param  wakeup      Settings applied to wakeup configuration
 */
void PowerCC32XX_configureWakeup(PowerCC32XX_Wakeup *wakeup);

/*! OS-specific power policy initialization function */
void PowerCC32XX_initPolicy(void);

/*!
 *  @brief  Function to get wakeup configuration settings
 *
 *  This function allows an app to query the current LPDS and shutdown
 *  wakeup configuration settings.
 *
 *  @param  wakeup      A PowerCC32XX_Wakeup structure to be written into
 */
void PowerCC32XX_getWakeup(PowerCC32XX_Wakeup *wakeup);

/*! CC32XX-specific function to query the LPDS park state for a pin */
PowerCC32XX_ParkState PowerCC32XX_getParkState(PowerCC32XX_Pin pin);

/*! CC32XX-specific function to restore the LPDS park state for a pin */
void PowerCC32XX_restoreParkState(PowerCC32XX_Pin pin,
    PowerCC32XX_ParkState state);

/*! CC32XX-specific function to dynamically set the LPDS park state for a pin */
void PowerCC32XX_setParkState(PowerCC32XX_Pin pin, uint32_t level);

/*!
 *  @brief  Function to disable IO retention and unlock pin groups following
 *  exit from Shutdown.
 *
 *  PowerCC32XX_ConfigV1.ioRetentionShutdown can be used to specify locking and
 *  retention of pin groups during Shutdown.  Upon exit from Shutdown, and
 *  when appropriate, an application can call this function, to
 *  correspondingly disable IO retention, and unlock the specified pin groups.
 *
 *  @param  groupFlags     A logical OR of one or more of the following
 *  flags (defined in driverlib/prcm.h):
 *      PRCM_IO_RET_GRP_0 - all pins except sFlash and JTAG interface
 *      PRCM_IO_RET_GRP_1 - sFlash interface pins 11,12,13,14
 *      PRCM_IO_RET_GRP_2 - JTAG TDI and TDO interface pins 16,17
 *      PRCM_IO_RET_GRP_3 - JTAG TCK and TMS interface pins 19,20
 */
void PowerCC32XX_disableIORetention(unsigned long groupFlags);

/*! OS-specific power policy function */
void PowerCC32XX_sleepPolicy(void);

/* \cond */
#define Power_getPerformanceLevel(void)   0
#define Power_setPerformanceLevel(level)  Power_EFAIL
/* \endcond */

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_power_PowerCC32XX__include */
