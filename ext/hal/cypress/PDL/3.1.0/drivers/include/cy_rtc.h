/***************************************************************************//**
* \file cy_rtc.h
* \version 2.20
*
* This file provides constants and parameter values for the APIs for the 
* Real-Time Clock (RTC).
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*
*******************************************************************************/

/**
* \defgroup group_rtc Real-Time Clock (RTC)
* \{
*
* The Real-time Clock (RTC) driver provides an application interface 
* for keeping track of time and date.
*
* Use the RTC driver when the system requires the current time or date. You 
* can also use the RTC when you do not need the current time and date but you 
* do need accurate timing of events with one-second resolution.
* 
* The RTC driver provides these features: <br>
* * Different hour format support <br>
* * Multiple alarm function (two-alarms) <br>
* * Daylight Savings Time (DST) support <br>
* * Automatic leap year compensation <br>
* * Option to drive the RTC by an external 50 Hz or 60 Hz clock source
*
* The RTC driver provides access to the HW real-time clock. The HW RTC is 
* located in the Backup domain. You need to choose the clock source for the 
* Backup domain using the Cy_SysClk_ClkBakSetSource() function. If the clock 
* for the Backup domain is set and enabled, the RTC automatically 
* starts counting.
*
* The RTC driver keeps track of second, minute, hour, day of the week, day of 
* the month, month, and year.
*
* DST may be enabled and supports any start and end date. The start and end 
* dates can be a fixed date (like 24 March) or a relative date (like the 
* second Sunday in March).
*
* The RTC has two alarms that you can configure to generate an interrupt. 
* You specify the match value for the time when you want the alarm to occur.
* Your interrupt handler then handles the response. The alarm flexibility 
* supports periodic alarms (such as every minute), or a single alarm 
* (13:45 on 28 September, 2043).
*
* <b> Clock Source </b> <br>
* The Backup domain can be driven by: <br>
* * Watch-crystal oscillator (WCO). This is a high-accuracy oscillator that is
* suitable for RTC applications and requires a 32.768 kHz external crystal 
* populated on the application board. The WCO can be supplied by vddbak and 
* therefore can run without vddd/vccd present. This can be used to wake the chip
* from Hibernate mode.
*
* * The Internal Low-speed Oscillator (ILO) routed from Clk_LF or directly 
* (as alternate backup domain clock source). Depending on the device power 
* mode the alternate backup domain clock source is set. For example, for 
* DeepSleep mode the ILO is routed through Clk_LF. But for Hibernate
* power mode the ILO is set directly. Note that, the ILO should be configured to
* work in the Hibernate mode. For more info refer to the \ref group_sysclk
* driver. The ILO is a low-accuracy RC-oscillator that does not require
* any external elements on the board. Its poor accuracy (+/- 30%) means it is 
* less useful for the RTC. However, current can be supplied by an internal
* power supply (Vback) and therefore it can run without Vddd/Vccd present.
* This also can be used to wake the chip from Hibernate mode using RTC alarm 
* interrupt. For more details refer to Power Modes (syspm) driver description.
*
* * The Precision Internal Low-speed Oscillator (PILO), routed from Clk_LF 
* (alternate backup domain clock source). This is an RC-oscillator (ILO) that
* can achieve accuracy of +/- 2% with periodic calibration. It is not expected 
* to be accurate enough for good RTC capability. The PILO requires 
* Vddd/Vccd present. It can be used in modes down to DeepSleep, but ceases to 
* function in Hibernate mode.
*
* * External 50 Hz or 60 Hz sine-wave clock source or 32.768kHz square clock 
* source.
* For example, the wall AC frequency can be the clock source. Such a clock 
* source can be used if the external 32.768 kHz WCO is absent from the board.
* For more details, refer to the Cy_RTC_SelectFrequencyPrescaler() function 
* description.
*
* The WCO is the recommended clock source for the RTC, if it is present 
* in design. For setting the Backup domain clock source, refer to the 
* \ref group_sysclk driver.
*
* \note If the WCO is enabled, it should source the Backup domain directly. 
* Do not route the WCO through the Clk_LF. This is because Clk_LF is not 
* available in all low-power modes.
*
* \section group_rtc_section_configuration Configuration Considerations
*
* Before RTC set up, ensure that the Backup domain is clocked with the desired 
* clock source.
*
* To set up an RTC, provide the configuration parameters in the 
* cy_stc_rtc_config_t structure. Then call Cy_RTC_Init(). You can also set the 
* date and time at runtime. Call Cy_RTC_SetDateAndTime() using the filled 
* cy_stc_rtc_config_t structure, or call Cy_RTC_SetDateAndTimeDirect() with 
* valid time and date values. 
*
* <b> RTC Interrupt Handling </b> <br>
* The RTC driver provides three interrupt handler functions: 
* Cy_RTC_Alarm1Interrupt(), Cy_RTC_Alarm2Interrupt(), and 
* Cy_RTC_CenturyInterrupt(). All three functions are blank functions with 
* the WEAK attribute. For any interrupt you use, redefine the interrupt handler
* in your source code.
*
* When an interrupt occurs, call the Cy_RTC_Interrupt() function. The RTC 
* hardware provides a single interrupt line to the NVIC for the three RTC 
* interrupts. This function checks the interrupt register to determine which 
* interrupt (out of the three) was generated. It then calls the 
* appropriate handler.
*
* \warning The Cy_RTC_Alarm2Interrupt() is not called if the DST feature is 
* enabled. If DST is enabled, the Cy_RTC_Interrupt() function redirects that 
* interrupt to manage daylight savings time using Cy_RTC_DstInterrupt().
* In general, the RTC interrupt handler function the Cy_RTC_DstInterrupt() 
* function is called instead of Cy_RTC_Alarm2Interrupt().
*
* For RTC interrupt handling, the user should: <br>
* 1) Implement strong interrupt handling function(s) for the required events 
* (see above). If DST is enabled, then Alarm2 is not available. The DST handler 
* is built into the PDL.<br>
* 2) Implement an RTC interrupt handler and call Cy_RTC_Interrupt() 
*    from there<br>
* 3) Configure the RTC interrupt: <br>
* a) Set the mask for RTC required interrupt using 
* Cy_RTC_SetInterruptMask()<br>
* b) Initialize the RTC interrupt by setting priority and the RTC interrupt 
* vector using the Cy_SysInt_Init() function <br>
* c) Enable the RTC interrupt using the CMSIS core function NVIC_EnableIRQ().
*
* <b> Alarm functionality </b> <br>
* To set up an alarm, enable the required RTC interrupt. Then provide the 
* configuration parameters in the cy_stc_rtc_alarm_t structure. You enable 
* any item you want matched, and provide a match value. You disable any other. 
* You do not need to set match values for disabled elements, as they are 
* ignored. 
* \note The alarm itself must be enabled in this structure. When a match 
* occurs, the alarm is triggered and your interrupt handler is called.
*
* An example is the best way to explain how this works. If you want an alarm 
* on every hour, then in the cy_stc_rtc_alarm_t structure, you provide 
* these values:
*
* Alarm_1.sec         = 0u <br>
* Alarm_1.secEn       = CY_RTC_ALARM_ENABLE <br>
* Alarm_1.min         = 0u <br>
* Alarm_1.minEn       = CY_RTC_ALARM_ENABLE <br>
* Alarm_1.hourEn      = CY_RTC_ALARM_DISABLE <br>
* Alarm_1.dayOfWeekEn = CY_RTC_ALARM_DISABLE <br>
* Alarm_1.dateEn      = CY_RTC_ALARM_DISABLE <br>
* Alarm_1.monthEn     = CY_RTC_ALARM_DISABLE <br>
* Alarm_1.almEn       = CY_RTC_ALARM_ENABLE  <br>
* 
* With this setup, every time both the second and minute are zero, Alarm1 is 
* asserted. That happens once per hour. Note that, counterintuitively, to have 
* an alarm every hour, Alarm_1.hourEn is disabled. This is disabled because 
* for an hourly alarm you do not match the value of the hour.
*
* After cy_stc_rtc_alarm_t structure is filled, call the 
* Cy_RTC_SetAlarmDateAndTime(). The alarm can also be set without using the 
* cy_stc_rtc_alarm_t structure. Call Cy_RTC_SetAlarmDateAndTimeDirect() with 
* valid values.
*
* <b> The DST Feature </b> <br>
* The DST feature is managed by the PDL using the RTC Alarm2 interrupt. 
* Therefore, you cannot have both DST enabled and use the Alarm2 interrupt. 
*
* To set up the DST, route the RTC interrupt to NVIC:<br>
* 1) Initialize the RTC interrupt by setting priority and the RTC interrupt 
* vector using Cy_SysInt_Init()  <br>
* 2) Enable the RTC interrupt using the CMSIS core function NVIC_EnableIRQ().
* 
* After this, provide the configuration parameters in the 
* cy_stc_rtc_dst_t structure. This structure consists of two 
* cy_stc_rtc_dst_format_t structures, one for DST Start time and one for 
* DST Stop time. You also specify whether these times are absolute or relative.
* 
* After the cy_stc_rtc_dst_t structure is filled, call Cy_RTC_EnableDstTime() 
*
* \section group_rtc_lp Low Power Support
* The RTC provides the callback functions to facilitate 
* the low-power mode transition. The callback 
* \ref Cy_RTC_DeepSleepCallback must be called during execution 
* of \ref Cy_SysPm_DeepSleep; \ref Cy_RTC_HibernateCallback must be 
* called during execution of \ref Cy_SysPm_Hibernate. 
* To trigger the callback execution, the callback must be registered 
* before calling the mode transition function. 
* Refer to \ref group_syspm driver for more 
* information about low-power mode transitions.
*
* \section group_rtc_section_more_information More Information
*
* For more information on the RTC peripheral, refer to the technical reference
* manual (TRM).
*
* \section group_rtc_MISRA MISRA-C Compliance
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>16.7</td>
*     <td>A</td>
*     <td>The object addressed by the pointer parameter '%s' is not modified and
*         so the pointer could be of type 'pointer to const'.</td>
*     <td>
*           The pointer parameter is not used or modified, as there is no need 
*           to do any actions with it. However, such parameter is 
*           required to be presented in the function, because the 
*           \ref Cy_RTC_DeepSleepCallback and \ref Cy_RTC_HibernateCallback are 
*           callbacks of \ref cy_en_syspm_status_t type.
*           The SysPm driver callback function type requires implementing the 
*           function with the next parameter and return value: <br>
*           cy_en_syspm_status_t (*Cy_SysPmCallback) 
*           (cy_stc_syspm_callback_params_t *callbackParams);
*     </td>
*   </tr>
* </table>
*
* \section group_rtc_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>2.20</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>2.10</td>
*     <td> Corrected Cy_RTC_SetDateAndTimeDirect(), Cy_RTC_SetNextDstTime()
*          function <br>
*          Corrected internal macro <br>
*          Documentation updates</td>
*     <td> Incorrect behavior of \ref Cy_RTC_SetDateAndTimeDirect() and 
*          \ref Cy_RTC_SetNextDstTime() work in debug mode <br>
*          Debug assert correction in \ref Cy_RTC_ConvertDayOfWeek, 
*          \ref Cy_RTC_IsLeapYear, \ref Cy_RTC_DaysInMonth </td>
*   </tr>
*   <tr>
*     <td>2.0</td>
*     <td>Enhancement and defect fixes: <br>
*          * Added input parameter(s) validation to all public functions. <br>
*          * Removed "Cy_RTC_" prefixes from the internal functions names. <br>
*          * Renamed the elements in the cy_stc_rtc_alarm structure. <br>
*          * Changed the type of elements with limited set of values, from 
*            uint32_t to enumeration.
*     </td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_rtc_macros Macros
* \defgroup group_rtc_functions Functions
* \{
* \defgroup group_rtc_general_functions General
* \defgroup group_rtc_alarm_functions Alarm
* \defgroup group_rtc_dst_functions DST functions
* \defgroup group_rtc_low_level_functions Low-Level
* \defgroup group_rtc_interrupt_functions Interrupt
* \defgroup group_rtc_low_power_functions Low Power Callbacks
* \}
* \defgroup group_rtc_data_structures Data Structures
* \defgroup group_rtc_enums Enumerated Types
*/

#if !defined (CY_RTC_H)
#define CY_RTC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cy_device_headers.h"
#include "cy_device.h"
#include "cy_syslib.h"
#include "cy_syspm.h"

#ifndef CY_IP_MXS40SRSS_RTC
    #error "The RTC driver is not supported on this device"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \addtogroup group_rtc_macros
* \{
*/

/** RTC driver identifier */
#define CY_RTC_ID                                   (CY_PDL_DRV_ID(0x28U))

/** Driver major version */
#define CY_RTC_DRV_VERSION_MAJOR                    2

/** Driver minor version */
#define CY_RTC_DRV_VERSION_MINOR                    20
/** \} group_rtc_macros */

/*******************************************************************************
*       Enumerated Types
*******************************************************************************/

/**
* \addtogroup group_rtc_enums
* \{
*/

/** RTC status enumeration */
typedef enum
 {
    CY_RTC_SUCCESS       = 0x00U,    /**< Successful */
    CY_RTC_BAD_PARAM     = CY_RTC_ID | CY_PDL_STATUS_ERROR | 0x01U,    /**< One or more invalid parameters */
    CY_RTC_TIMEOUT       = CY_RTC_ID | CY_PDL_STATUS_ERROR | 0x02U,    /**< Time-out occurs */
    CY_RTC_INVALID_STATE = CY_RTC_ID | CY_PDL_STATUS_ERROR | 0x03U,    /**< Operation not setup or is in an improper state */
    CY_RTC_UNKNOWN       = CY_RTC_ID | CY_PDL_STATUS_ERROR | 0xFFU     /**< Unknown failure */
} cy_en_rtc_status_t;

/** This enumeration is used to set frequency by changing the it pre-scaler */
typedef enum
{
    CY_RTC_FREQ_WCO_32768_HZ,      /**< prescaler value for 32.768 kHz oscillator */
    CY_RTC_FREQ_60_HZ,             /**< prescaler value for 60 Hz source */
    CY_RTC_FREQ_50_HZ,             /**< prescaler value for 50 Hz source */
} cy_en_rtc_clock_freq_t;

/** This enumeration is used to set/get information for alarm 1 or alarm 2 */
typedef enum cy_en_rtc_alarm
{
    CY_RTC_ALARM_1,         /**< Alarm 1 enum */
    CY_RTC_ALARM_2          /**< Alarm 2 enum */
} cy_en_rtc_alarm_t;

/** This enumeration is used to set/get hours format */
typedef enum 
{
    CY_RTC_24_HOURS,         /**< The 24 hour format */
    CY_RTC_12_HOURS          /**< The 12 hour (AM/PM) format */
} cy_en_rtc_hours_format_t;

/** Enumeration to configure the RTC Write register */
typedef enum 
{
    CY_RTC_WRITE_DISABLED,          /**< Writing the RTC is disabled */
    CY_RTC_WRITE_ENABLED            /**< Writing the RTC is enabled */
} cy_en_rtc_write_status_t;

/** Enumeration used to set/get DST format */
typedef enum 
{
    CY_RTC_DST_RELATIVE,        /**< Relative DST format */
    CY_RTC_DST_FIXED            /**< Fixed DST format */
} cy_en_rtc_dst_format_t;

/** Enumeration to indicate the AM/PM period of day */
typedef enum 
{
    CY_RTC_AM,      /**< AM period of day */
    CY_RTC_PM       /**< PM period of day */
} cy_en_rtc_am_pm_t;

/** Enumeration to enable/disable the RTC alarm on match with required value */
typedef enum
{
    CY_RTC_ALARM_DISABLE,     /**< Disable alarm on match with required value */
    CY_RTC_ALARM_ENABLE       /**< Enable alarm on match with required value */
} cy_en_rtc_alarm_enable_t;
/** \} group_rtc_enums */


/*******************************************************************************
*      Types definition
*******************************************************************************/

/**
* \addtogroup group_rtc_data_structures
* \{
*/

/**
* This is the data structure that is used to configure the rtc time 
* and date values.
*/
typedef struct cy_stc_rtc_config
{
    /* Time information */
    uint32_t sec;                    /**< Seconds value, range [0-59] */
    uint32_t min;                    /**< Minutes value, range [0-59] */
    uint32_t hour;                   /**< Hour, range depends on hrFormat, if hrFormat = CY_RTC_24_HOURS, range [0-23];
                                              If hrFormat = CY_RTC_12_HOURS, range [1-12] and appropriate AM/PM day 
                                              period should be set (amPm) */
    cy_en_rtc_am_pm_t amPm;              /**< AM/PM hour period, see \ref cy_en_rtc_am_pm_t.
                                              This element is actual when hrFormat = CY_RTC_12_HOURS. The firmware 
                                              ignores this element if hrFormat = CY_RTC_24_HOURS */
    cy_en_rtc_hours_format_t hrFormat;   /**< Hours format, see \ref cy_en_rtc_hours_format_t */
    uint32_t dayOfWeek;                  /**< Day of the week, range [1-7], see \ref group_rtc_day_of_the_week */
    
    /* Date information  */
    uint32_t date;                        /**< Date of month, range [1-31] */
    uint32_t month;                       /**< Month, range [1-12]. See \ref group_rtc_month */
    uint32_t year;                        /**< Year, range [0-99] */
} cy_stc_rtc_config_t;

/** Decimal data structure that is used to save the Alarms */
typedef struct cy_stc_rtc_alarm
{
    /* Alarm time information  */
    uint32_t sec;                       /**< Alarm seconds, range [0-59]. 
                                             The appropriate ALARMX interrupt is be asserted on matching with this 
                                             value if secEn is previous enabled (secEn = 1) */
    cy_en_rtc_alarm_enable_t secEn;     /**< Enable alarm on seconds matching, see \ref cy_en_rtc_alarm_enable_t. */

    uint32_t min;                       /**< Alarm minutes, range [0-59]. 
                                             The appropriate ALARMX interrupt is be asserted on matching with this
                                             value if minEn is previous enabled (minEn = 1) */
    cy_en_rtc_alarm_enable_t minEn;     /**< Enable alarm on minutes matching, see \ref cy_en_rtc_alarm_enable_t. */

    uint32_t hour;                      /**< Alarm hours, range [0-23]
                                             The appropriate ALARMX interrupt is be asserted on matching with this 
                                             value if hourEn is previous enabled (hourEn = 1) */
    cy_en_rtc_alarm_enable_t hourEn;    /**< Enable alarm on hours matching, see \ref cy_en_rtc_alarm_enable_t. */

    uint32_t dayOfWeek;                     /**< Alarm day of the week, range [1-7]
                                                 The appropriate ALARMX interrupt is be asserted on matching with this
                                                 value if dayOfWeek is previous enabled (dayOfWeekEn = 1) */
    cy_en_rtc_alarm_enable_t dayOfWeekEn;   /**< Enable alarm on day of the week matching, 
                                                  see \ref cy_en_rtc_alarm_enable_t */

    /* Alarm date information */
    uint32_t date;                      /**< Alarm date, range [1-31]. 
                                             The appropriate ALARMX interrupt is be asserted on matching with this 
                                             value if dateEn is previous enabled (dateEn = 1) */
    cy_en_rtc_alarm_enable_t dateEn;    /**< Enable alarm on date matching, see \ref cy_en_rtc_alarm_enable_t. */

    uint32_t month;                     /**< Alarm Month, range [1-12]. 
                                             The appropriate ALARMX interrupt is be asserted on matching with this
                                             value if dateEn is previous enabled (dateEn = 1) */
    cy_en_rtc_alarm_enable_t monthEn;   /**< Enable alarm on month matching, see \ref cy_en_rtc_alarm_enable_t. */

    cy_en_rtc_alarm_enable_t almEn;     /**< Enable Alarm for appropriate ALARMX, see \ref cy_en_rtc_alarm_enable_t.
                                             If all alarm structure elements are enabled (almEn = CY_RTC_ALARM_ENABLE)
                                             the alarm interrupt is be asserted every second. */
} cy_stc_rtc_alarm_t;

/**
* This is DST structure for DST feature setting. Structure is combined with the
* fixed format and the relative format. It is used to save the DST time and date
* fixed or relative time format.
*/
typedef struct
{
    cy_en_rtc_dst_format_t format;   /**< DST format. See /ref cy_en_rtc_dst_format_t. 
                                          Based on this value other structure elements
                                          should be filled or could be ignored */
    uint32_t hour;                   /**< Should be filled for both format types.
                                          Hour is always presented in 24hour format, range[0-23] */
    uint32_t dayOfMonth;             /**< Day of Month, range[1-31]. This element should be filled if 
                                          format = CY_RTC_DST_FIXED. Firmware calculates this value in condition that
                                          format = CY_RTC_DST_RELATIVE is selected */
    uint32_t weekOfMonth;            /**< Week of month, range[1-6]. This element should be filled if 
                                          format = CY_RTC_DST_RELATIVE.
                                          Firmware calculates dayOfMonth value based on weekOfMonth 
                                          and dayOfWeek values */
    uint32_t dayOfWeek;              /**< Day of the week, this element should be filled in condition that 
                                          format = CY_RTC_DST_RELATIVE. Range[1- 7], 
                                          see \ref group_rtc_day_of_the_week. Firmware calculates dayOfMonth value 
                                          based on dayOfWeek and weekOfMonth values */
    uint32_t month;                  /**< Month value, range[1-12], see \ref group_rtc_month.
                                          This value should be filled for both format types */
} cy_stc_rtc_dst_format_t;

/** This is the DST structure to handle start DST and stop DST */
typedef struct
{
    cy_stc_rtc_dst_format_t startDst;    /**< DST start time structure */
    cy_stc_rtc_dst_format_t stopDst;     /**< DST stop time structure */
} cy_stc_rtc_dst_t;

/** \} group_rtc_data_structures */


/*******************************************************************************
*    Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_rtc_functions
* \{
*/

/**
* \addtogroup group_rtc_general_functions
* \{
*/
cy_en_rtc_status_t Cy_RTC_Init(cy_stc_rtc_config_t const *config);
cy_en_rtc_status_t Cy_RTC_SetDateAndTime(cy_stc_rtc_config_t const *dateTime);
void   Cy_RTC_GetDateAndTime(cy_stc_rtc_config_t *dateTime);
cy_en_rtc_status_t Cy_RTC_SetDateAndTimeDirect(uint32_t sec, uint32_t min, uint32_t hour, 
                                               uint32_t date, uint32_t month, uint32_t year);
cy_en_rtc_status_t Cy_RTC_SetHoursFormat(cy_en_rtc_hours_format_t hoursFormat);
void Cy_RTC_SelectFrequencyPrescaler(cy_en_rtc_clock_freq_t clkSel);
/** \} group_rtc_general_functions */

/**
* \addtogroup group_rtc_alarm_functions
* \{
*/
cy_en_rtc_status_t Cy_RTC_SetAlarmDateAndTime(cy_stc_rtc_alarm_t const *alarmDateTime, cy_en_rtc_alarm_t alarmIndex);
void   Cy_RTC_GetAlarmDateAndTime(cy_stc_rtc_alarm_t *alarmDateTime, cy_en_rtc_alarm_t alarmIndex);
cy_en_rtc_status_t Cy_RTC_SetAlarmDateAndTimeDirect(uint32_t sec, uint32_t min, uint32_t hour, 
                                                    uint32_t date, uint32_t month, cy_en_rtc_alarm_t alarmIndex);
/** \} group_rtc_alarm_functions */

/**
* \addtogroup group_rtc_dst_functions
* \{
*/
cy_en_rtc_status_t Cy_RTC_EnableDstTime(cy_stc_rtc_dst_t const *dstTime, cy_stc_rtc_config_t const *timeDate);
cy_en_rtc_status_t Cy_RTC_SetNextDstTime(cy_stc_rtc_dst_format_t const *nextDst);
bool Cy_RTC_GetDstStatus(cy_stc_rtc_dst_t const *dstTime, cy_stc_rtc_config_t const *timeDate);
/** \} group_rtc_dst_functions */

/**
* \addtogroup group_rtc_interrupt_functions
* \{
*/
void Cy_RTC_Interrupt(cy_stc_rtc_dst_t const *dstTime, bool mode);
void Cy_RTC_Alarm1Interrupt(void);
void Cy_RTC_Alarm2Interrupt(void);
void Cy_RTC_DstInterrupt(cy_stc_rtc_dst_t const *dstTime);
void Cy_RTC_CenturyInterrupt(void);
uint32_t Cy_RTC_GetInterruptStatus(void);
uint32_t Cy_RTC_GetInterruptStatusMasked(void);
uint32_t Cy_RTC_GetInterruptMask(void);
void Cy_RTC_ClearInterrupt(uint32_t interruptMask);
void Cy_RTC_SetInterrupt(uint32_t interruptMask);
void Cy_RTC_SetInterruptMask(uint32_t interruptMask);
/** \} group_rtc_interrupt_functions */

/**
* \addtogroup group_rtc_low_power_functions
* \{
*/
cy_en_syspm_status_t Cy_RTC_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams);
cy_en_syspm_status_t Cy_RTC_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams);
/** \} group_rtc_low_power_functions */

/**
* \addtogroup group_rtc_low_level_functions
* \{
*/
__STATIC_INLINE uint32_t Cy_RTC_ConvertDayOfWeek(uint32_t day, uint32_t month, uint32_t year);
__STATIC_INLINE bool Cy_RTC_IsLeapYear(uint32_t year);
__STATIC_INLINE uint32_t Cy_RTC_DaysInMonth(uint32_t month, uint32_t year);
__STATIC_INLINE void Cy_RTC_SyncFromRtc(void);
__STATIC_INLINE cy_en_rtc_status_t Cy_RTC_WriteEnable(cy_en_rtc_write_status_t writeEnable);
__STATIC_INLINE uint32_t Cy_RTC_GetSyncStatus(void);
__STATIC_INLINE uint32_t Cy_RTC_ConvertBcdToDec(uint32_t bcdNum);
__STATIC_INLINE uint32_t Cy_RTC_ConvertDecToBcd(uint32_t decNum);
__STATIC_INLINE cy_en_rtc_hours_format_t Cy_RTC_GetHoursFormat(void);
__STATIC_INLINE bool Cy_RTC_IsExternalResetOccurred(void);

__STATIC_INLINE void Cy_RTC_SyncToRtcAhbDateAndTime(uint32_t timeBcd, uint32_t dateBcd);
__STATIC_INLINE void Cy_RTC_SyncToRtcAhbAlarm(uint32_t alarmTimeBcd, uint32_t alarmDateBcd, cy_en_rtc_alarm_t alarmIndex);
/** \} group_rtc_low_level_functions */

/** \} group_rtc_functions */

/**
* \addtogroup group_rtc_macros
* \{
*/

/*******************************************************************************
*    API Constants
*******************************************************************************/

/**
* \defgroup group_rtc_day_of_the_week Day of the week definitions
* \{
* Definitions of days in the week
*/
#define CY_RTC_SUNDAY                                   (1UL) /**< Sequential number of Sunday in the week */
#define CY_RTC_MONDAY                                   (2UL) /**< Sequential number of Monday in the week */
#define CY_RTC_TUESDAY                                  (3UL) /**< Sequential number of Tuesday in the week */
#define CY_RTC_WEDNESDAY                                (4UL) /**< Sequential number of Wednesday in the week */
#define CY_RTC_THURSDAY                                 (5UL) /**< Sequential number of Thursday in the week */
#define CY_RTC_FRIDAY                                   (6UL) /**< Sequential number of Friday in the week */
#define CY_RTC_SATURDAY                                 (7UL) /**< Sequential number of Saturday in the week */
/** \} group_rtc_day_of_the_week */

/**
* \defgroup group_rtc_dst_week_of_month Week of month definitions
* \{
* Week of Month setting constants definitions for Daylight Saving Time feature
*/
#define CY_RTC_FIRST_WEEK_OF_MONTH                      (1UL)  /**< First week in the month */
#define CY_RTC_SECOND_WEEK_OF_MONTH                     (2UL)  /**< Second week in the month  */
#define CY_RTC_THIRD_WEEK_OF_MONTH                      (3UL)  /**< Third week in the month  */
#define CY_RTC_FOURTH_WEEK_OF_MONTH                     (4UL)  /**< Fourth week in the month  */
#define CY_RTC_FIFTH_WEEK_OF_MONTH                      (5UL)  /**< Fifth week in the month  */
#define CY_RTC_LAST_WEEK_OF_MONTH                       (6UL)  /**< Last week in the month  */
/** \} group_rtc_dst_week_of_month */

/**
* \defgroup group_rtc_month Month definitions
* \{
* Constants definition for Months
*/
#define CY_RTC_JANUARY                                  (1UL)   /**< Sequential number of January in the year */
#define CY_RTC_FEBRUARY                                 (2UL)   /**< Sequential number of February in the year */
#define CY_RTC_MARCH                                    (3UL)   /**< Sequential number of March in the year */
#define CY_RTC_APRIL                                    (4UL)   /**< Sequential number of April in the year */
#define CY_RTC_MAY                                      (5UL)   /**< Sequential number of May in the year */
#define CY_RTC_JUNE                                     (6UL)   /**< Sequential number of June in the year */
#define CY_RTC_JULY                                     (7UL)   /**< Sequential number of July in the year */
#define CY_RTC_AUGUST                                   (8UL)   /**< Sequential number of August in the year */
#define CY_RTC_SEPTEMBER                                (9UL)   /**< Sequential number of September in the year */
#define CY_RTC_OCTOBER                                  (10UL)  /**< Sequential number of October in the year */
#define CY_RTC_NOVEMBER                                 (11UL)  /**< Sequential number of November in the year */
#define CY_RTC_DECEMBER                                 (12UL)  /**< Sequential number of December in the year */
/** \} group_rtc_month */

/**
* \defgroup group_rtc_days_in_month Number of days in month definitions
* \{
* Definition of days in current month
*/
#define CY_RTC_DAYS_IN_JANUARY                          (31U)  /**< Number of days in January */
#define CY_RTC_DAYS_IN_FEBRUARY                         (28U)  /**< Number of days in February */
#define CY_RTC_DAYS_IN_MARCH                            (31U)  /**< Number of days in March */
#define CY_RTC_DAYS_IN_APRIL                            (30U)  /**< Number of days in April */
#define CY_RTC_DAYS_IN_MAY                              (31U)  /**< Number of days in May */
#define CY_RTC_DAYS_IN_JUNE                             (30U)  /**< Number of days in June */
#define CY_RTC_DAYS_IN_JULY                             (31U)  /**< Number of days in July */
#define CY_RTC_DAYS_IN_AUGUST                           (31U)  /**< Number of days in August */
#define CY_RTC_DAYS_IN_SEPTEMBER                        (30U)  /**< Number of days in September */
#define CY_RTC_DAYS_IN_OCTOBER                          (31U)  /**< Number of days in October */
#define CY_RTC_DAYS_IN_NOVEMBER                         (30U)  /**< Number of days in November */
#define CY_RTC_DAYS_IN_DECEMBER                         (31U)  /**< Number of days in December */
/** \} group_rtc_days_in_month */

/**
* \defgroup group_rtc_macros_interrupts RTC Interrupt sources
* \{
* Definitions for RTC interrupt sources
*/
/** Alarm 1 status */
#define CY_RTC_INTR_ALARM1                               BACKUP_INTR_ALARM1_Msk

/** Alarm 2 status */
#define CY_RTC_INTR_ALARM2                               BACKUP_INTR_ALARM2_Msk

/**
* This interrupt occurs when the year is reached to 2100 which is rolling
* over the year field value from 99 to 0
*/
#define CY_RTC_INTR_CENTURY                              BACKUP_INTR_CENTURY_Msk
/** \} group_rtc_macros_interrupts */

/**
* \defgroup group_rtc_busy_status RTC Status definitions
* \{
* Definitions for indicating the RTC BUSY bit 
*/
#define CY_RTC_BUSY                                     (1UL) /**< RTC Busy bit is set, RTC is pending */
#define CY_RTC_AVAILABLE                                (0UL) /**< RTC Busy bit is cleared, RTC is available */
/** \} group_rtc_busy_status */

/*******************************************************************************
*         Internal Constants
*******************************************************************************/

/** \cond INTERNAL */

/** Days per week definition */
#define CY_RTC_DAYS_PER_WEEK                         (7UL)

/** Month per year definition */
#define CY_RTC_MONTHS_PER_YEAR                       (12U)

/** Maximum value of seconds and minutes */
#define CY_RTC_MAX_SEC_OR_MIN                        (59UL)

/** Biggest value of hours definition */
#define CY_RTC_MAX_HOURS_24H                         (23UL)

/** Maximum value of year definition */
#define CY_RTC_MAX_DAYS_IN_MONTH                     (31UL)

/** Maximum value of year definition */
#define CY_RTC_MAX_YEAR                              (99UL)

/** Number of RTC interrupts  */
#define CY_RTC_NUM_OF_INTR                           (3U)

/** Number of RTC interrupts  */
#define CY_RTC_TRYES_TO_SETUP_DST                    (24U)

/** RTC AM/PM bit for 12H hour mode */
#define CY_RTC_12HRS_PM_BIT                          (0x20UL)

/** Mask for reading RTC AM/PM bit for 12H mode  */
#define CY_RTC_BACKUP_RTC_TIME_RTC_PM                ((uint32_t) (CY_RTC_12HRS_PM_BIT << BACKUP_RTC_TIME_RTC_HOUR_Pos))

/** Internal define for BCD values converting */
#define CY_RTC_BCD_NUMBER_SIZE                       (4UL)

/** Internal mask for BCD values converting */
#define CY_RTC_BCD_ONE_DIGIT_MASK                    (0x0000000FUL)

/** Internal define of dozen degree for BCD values converting */
#define CY_RTC_BCD_DOZED_DEGREE                      (10UL)

/** Internal define of hundred degree for BCD values converting */
#define CY_RTC_BCD_HUNDRED_DEGRE                     (100UL)

/** Definition of six WCO clocks in microseconds */
#define CY_RTC_DELAY_WHILE_READING_US                (183U)

/** Definition of two WCO clocks in microseconds */
#define CY_RTC_DELAY_WRITE_US                        (62U)

/** Definition of two WCO clocks in microseconds */
#define CY_RTC_DELAY_FOR_NEXT_DST                    (2000U)

/** Two thousand years definition  */
#define CY_RTC_TWO_THOUSAND_YEARS                    (2000UL)

/** Two thousand years definition  */
#define CY_RTC_TWENTY_ONE_HUNDRED_YEARS              (2100UL)

/** Mask for reading RTC hour for 12H mode  */
#define CY_RTC_BACKUP_RTC_TIME_RTC_12HOUR            (0x1f0000UL)

/** Half day hours definition  */
#define CY_RTC_HOURS_PER_HALF_DAY                    (12UL)

/** First day of the month definition  */
#define CY_RTC_FIRST_DAY_OF_MONTH                    (1UL)

/** Internal definition for DST GetDstStatus() function */
#define CY_RTC_DST_MONTH_POSITION                    (10UL)

/** Internal definition for DST GetDstStatus() function */
#define CY_RTC_DST_DAY_OF_MONTH_POSITION             (5UL)

/** Definition of delay in microseconds after try to set DST */
#define CY_RTC_DELAY_AFTER_DST_US                    (62U)

/** RTC days in months table */
extern uint8_t const cy_RTC_daysInMonthTbl[CY_RTC_MONTHS_PER_YEAR];

/* Internal macro to validate parameters in Cy_RTC_SelectFrequencyPrescaler() function */
#define CY_RTC_IS_CLK_VALID(clkSel)               (((clkSel) == CY_RTC_FREQ_WCO_32768_HZ) || \
                                                   ((clkSel) == CY_RTC_FREQ_60_HZ) || \
                                                   ((clkSel) == CY_RTC_FREQ_50_HZ))

/* Internal macro to validate parameters in Cy_RTC_SetHoursFormat() function */
#define CY_RTC_IS_HRS_FORMAT_VALID(hoursFormat)    (((hoursFormat) == CY_RTC_24_HOURS) || \
                                                    ((hoursFormat) == CY_RTC_12_HOURS))

/* Internal macro to validate parameters in Cy_RTC_WriteEnable() function */
#define CY_RTC_IS_WRITE_VALID(writeEnable)         (((writeEnable) == CY_RTC_WRITE_DISABLED) || \
                                                    ((writeEnable) == CY_RTC_WRITE_ENABLED))

/* Internal macro of all possible RTC interrupts */
#define CY_RTC_INTR_MASK                    (CY_RTC_INTR_ALARM1 | CY_RTC_INTR_ALARM2 | CY_RTC_INTR_CENTURY)

/* Macro to validate parameters in interrupt related functions */
#define CY_RTC_INTR_VALID(interruptMask)    (0UL == ((interruptMask) & ((uint32_t) ~(CY_RTC_INTR_MASK))))

/* Internal macro to validate RTC seconds and minutes parameters */
#define CY_RTC_IS_SEC_VALID(sec)            ((sec) <= CY_RTC_MAX_SEC_OR_MIN)

/* Internal macro to validate RTC seconds and minutes parameters */
#define CY_RTC_IS_MIN_VALID(min)            ((min) <= CY_RTC_MAX_SEC_OR_MIN)

/* Internal macro to validate RTC hour parameter */
#define CY_RTC_IS_HOUR_VALID(hour)          ((hour) <= CY_RTC_MAX_HOURS_24H)

/* Internal macro to validate RTC day of the week parameter */
#define CY_RTC_IS_DOW_VALID(dayOfWeek)      (((dayOfWeek) > 0U) && ((dayOfWeek) <= CY_RTC_DAYS_PER_WEEK))
        
/* Internal macro to validate RTC day parameter */
#define CY_RTC_IS_DAY_VALID(day)            (((day) > 0U) && ((day) <= CY_RTC_MAX_DAYS_IN_MONTH))

/* Internal macro to validate RTC month parameter */
#define CY_RTC_IS_MONTH_VALID(month)        (((month) > 0U) && ((month) <= CY_RTC_MONTHS_PER_YEAR))

/* Internal macro to validate RTC year parameter */
#define CY_RTC_IS_YEAR_SHORT_VALID(year)         ((year) <= CY_RTC_MAX_YEAR)

/* Internal macro to validate the year value in the Cy_RTC_ConvertDayOfWeek() */
#define CY_RTC_IS_YEAR_LONG_VALID(year)          (((year) >= CY_RTC_TWO_THOUSAND_YEARS) && \
                                                  ((year) <= CY_RTC_TWENTY_ONE_HUNDRED_YEARS))

/* Internal macro to validate RTC alarm parameter */
#define CY_RTC_IS_ALARM_EN_VALID(alarmEn)        (((alarmEn) == CY_RTC_ALARM_DISABLE) || \
                                                  ((alarmEn) == CY_RTC_ALARM_ENABLE))

/* Internal macro to validate RTC alarm index parameter */
#define CY_RTC_IS_ALARM_IDX_VALID(alarmIndex)    (((alarmIndex) == CY_RTC_ALARM_1) || ((alarmIndex) == CY_RTC_ALARM_2))

/* Internal macro to validate RTC alarm index parameter */
#define CY_RTC_IS_DST_FORMAT_VALID(format)       (((format) == CY_RTC_DST_RELATIVE) || ((format) == CY_RTC_DST_FIXED))

/** \endcond */
/** \} group_rtc_macros */

/**
* \addtogroup group_rtc_low_level_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_RTC_ConvertDayOfWeek
****************************************************************************//**
*
* Returns a day of the week for a year, month, and day of month that are passed
* through parameters. Zeller's congruence is used to calculate the day of
* the week. 
* RTC HW block does not provide the converting function for day of week. This 
* function should be called before Cy_RTC_SetDateAndTime() to get the day of 
* week.
*
* For the Georgian calendar, Zeller's congruence is:
* h = (q + [13 * (m + 1)] + K + [K/4] + [J/4] - 2J) mod 7
*
* h - The day of the week (0 = Saturday, 1 = Sunday, 2 = Monday, ., 6 = Friday).
* q - The day of the month.
* m - The month (3 = March, 4 = April, 5 = May, ..., 14 = February)
* K - The year of the century (year mod 100).
* J - The zero-based century (actually [year/100]) For example, the zero-based
* centuries for 1995 and 2000 are 19 and 20 respectively (not to be
* confused with the common ordinal century enumeration which indicates
* 20th for both cases).
*
* \note In this algorithm January and February are counted as months 13 and 14
* of the previous year.
*
* \param day The day of the month, Valid range 1..31.
*
* \param month The month of the year, see \ref group_rtc_month.
*
* \param year The year value. Valid range - 2000...2100.
*
* \return
* Returns a day of the week, see \ref group_rtc_day_of_the_week.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_RTC_ConvertDayOfWeek(uint32_t day, uint32_t month, uint32_t year)
{
    uint32_t retVal;

    CY_ASSERT_L3(CY_RTC_IS_DAY_VALID(day));
    CY_ASSERT_L3(CY_RTC_IS_MONTH_VALID(month));
    CY_ASSERT_L3(CY_RTC_IS_YEAR_LONG_VALID(year));
    
    /* Converts month number from regular convention
    * (1=January,..., 12=December) to convention required for this
    * algorithm (January and February are counted as months 13 and 14 of
    * previous year).
    */
    if (month < CY_RTC_MARCH)
    {
        month = CY_RTC_MONTHS_PER_YEAR + month;
        year--;
    }

    /* Calculates Day of Week using Zeller's congruence algorithms */
    retVal = 
    (day + (((month + 1UL) * 26UL) / 10UL) + year + (year / 4UL) + (6UL * (year / 100UL)) + (year / 400UL)) % 7UL;

    /* Makes correction for Saturday. Saturday number should be 7 instead of 0*/
    if (0u == retVal)
    {
        retVal = CY_RTC_SATURDAY;
    }

    return(retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_IsLeapYear
****************************************************************************//**
*
* Checks whether the year passed through the parameter is leap or not.
*
* This API is for checking an invalid value input for leap year.
* RTC HW block does not provide a validation checker against time/date values, 
* the valid range of days in Month should be checked before SetDateAndTime()
* function call. Leap year is identified as a year that is a multiple of 4 
* or 400 but not 100.
*
* \param year The year to be checked. Valid range - 2000...2100.
*
* \return
* false - The year is not leap; true - The year is leap.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_RTC_IsLeapYear(uint32_t year)
{
    CY_ASSERT_L3(CY_RTC_IS_YEAR_LONG_VALID(year));

    return(((0U == (year % 4UL)) && (0U != (year % 100UL))) || (0U == (year % 400UL)));
}


/*******************************************************************************
* Function Name: Cy_RTC_DaysInMonth
****************************************************************************//**
*
*  Returns a number of days in a month passed through the parameters. This API
*  is for checking an invalid value input for days.
*  RTC HW block does not provide a validation checker against time/date values, 
*  the valid range of days in Month should be checked before SetDateAndTime()
*  function call.
*
* \param month The month of the year, see \ref group_rtc_month.
*
* \param year A year value. Valid range - 2000...2100.
*
* \return A number of days in a month in the year passed through the parameters.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_RTC_DaysInMonth(uint32_t month, uint32_t year)
{
    uint32_t retVal;

    CY_ASSERT_L3(CY_RTC_IS_MONTH_VALID(month));
    CY_ASSERT_L3(CY_RTC_IS_YEAR_LONG_VALID(year));

    retVal = cy_RTC_daysInMonthTbl[month - 1UL];

    if (CY_RTC_FEBRUARY == month)
    {
        if (Cy_RTC_IsLeapYear(year))
        {
            retVal++;
        }
    }
    return(retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_SyncFromRtc
****************************************************************************//**
*
* The Synchronizer updates RTC values into AHB RTC user registers from the 
* actual RTC. By calling this function, the actual RTC register values is 
* copied to AHB user registers.
*
* \note Only after calling Cy_RTC_SyncFromRtc(), the RTC time values can be 
* read.
* After Cy_RTC_SyncFromRtc() calling the snapshot of the actual RTC registers
* are copied to the user registers. Meanwhile the RTC continues to clock.
*
*******************************************************************************/
__STATIC_INLINE void Cy_RTC_SyncFromRtc(void)
{
    uint32_t interruptState;
    
    interruptState = Cy_SysLib_EnterCriticalSection(); 
    
    /* RTC Write is possible only in the condition that CY_RTC_BUSY bit = 0 
    *  or RTC Write bit is not set.
    */
    if ((CY_RTC_BUSY != Cy_RTC_GetSyncStatus()) && (!_FLD2BOOL(BACKUP_RTC_RW_WRITE, BACKUP_RTC_RW)))
    {
        /* Setting RTC Read bit */
        BACKUP_RTC_RW = BACKUP_RTC_RW_READ_Msk;

        /* Delay to guarantee RTC data reading */
        Cy_SysLib_DelayUs(CY_RTC_DELAY_WHILE_READING_US);

        /* Clearing RTC Read bit */
        BACKUP_RTC_RW = 0U;
    }
    Cy_SysLib_ExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: Cy_RTC_WriteEnable
****************************************************************************//**
*
* Set/Clear writeable option for RTC user registers. When the Write bit is set, 
* data can be written into the RTC user registers. After all the RTC writes are 
* done, the firmware must clear (call Cy_RTC_WriteEnable(RTC_WRITE_DISABLED))
* the Write bit for the RTC update to take effect. 
*
* Set/Clear cannot be done if the RTC is still busy with a previous update 
* (CY_RTC_BUSY = 1) or RTC Reading is executing.
*
* \param writeEnable write status, see \ref cy_en_rtc_write_status_t.
*
* \return
* cy_en_rtc_status_t CY_RTC_SUCCESS - Set/Clear Write bit was successful <br>
* CY_RTC_INVALID_STATE - RTC is busy with a previous update.
*
*******************************************************************************/
__STATIC_INLINE cy_en_rtc_status_t Cy_RTC_WriteEnable(cy_en_rtc_write_status_t writeEnable)
{
    cy_en_rtc_status_t retVal = CY_RTC_INVALID_STATE;

    CY_ASSERT_L3(CY_RTC_IS_WRITE_VALID(writeEnable));

    if (writeEnable == CY_RTC_WRITE_ENABLED)
    {
        /* RTC Write bit set is possible only in condition that CY_RTC_BUSY bit = 0 
        * or RTC Read bit is not set
        */
        if ((CY_RTC_BUSY != Cy_RTC_GetSyncStatus()) && (!_FLD2BOOL(BACKUP_RTC_RW_READ, BACKUP_RTC_RW)))
        {
            BACKUP_RTC_RW |= BACKUP_RTC_RW_WRITE_Msk;
            retVal = CY_RTC_SUCCESS;
        }
    }
    else
    {
        /* Clearing Write Bit to complete write procedure */
        BACKUP_RTC_RW &= ((uint32_t) ~BACKUP_RTC_RW_WRITE_Msk);

        /* Delay to guarantee data write after clearing write bit */
        Cy_SysLib_DelayUs(CY_RTC_DELAY_WRITE_US);
        retVal = CY_RTC_SUCCESS;
    }

    return(retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_GetSyncStatus
****************************************************************************//**
*
* Return current status of CY_RTC_BUSY. The status indicates
* synchronization between the RTC user register and the actual RTC register. 
* CY_RTC_BUSY bit is set if it is synchronizing. It is not possible to set 
* the Read or Write bit until CY_RTC_BUSY clears.
*
* \return
* The status of RTC user register synchronization. See
* \ref group_rtc_busy_status
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_RTC_GetSyncStatus(void)
{  
    return((_FLD2BOOL(BACKUP_STATUS_RTC_BUSY, BACKUP_STATUS)) ? CY_RTC_BUSY : CY_RTC_AVAILABLE);
}


/*******************************************************************************
* Function Name: Cy_RTC_ConvertBcdToDec
****************************************************************************//**
*
* Converts an 8-bit BCD number into an 8-bit hexadecimal number. Each byte is
* converted individually and returned as an individual byte in the 32-bit
* variable.
*
* \param
* bcdNum An 8-bit BCD number. Each byte represents BCD.
*
* \return
* decNum An 8-bit hexadecimal equivalent number of the BCD number.
*
* For example, for 0x11223344 BCD number, the function returns 
* 0x2C in hexadecimal format.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_RTC_ConvertBcdToDec(uint32_t bcdNum)
{
    uint32_t retVal;

    retVal = 
    ((bcdNum & (CY_RTC_BCD_ONE_DIGIT_MASK << CY_RTC_BCD_NUMBER_SIZE)) 
                          >> CY_RTC_BCD_NUMBER_SIZE ) * CY_RTC_BCD_DOZED_DEGREE;

    retVal += bcdNum & CY_RTC_BCD_ONE_DIGIT_MASK;

    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_ConvertDecToBcd
****************************************************************************//**
*
* Converts an 8-bit hexadecimal number into an 8-bit BCD number. Each byte
* is converted individually and returned as an individual byte in the 32-bit
* variable.
*
* \param 
* decNum An 8-bit hexadecimal number. Each byte is represented in hex.
* 0x11223344 -> 0x20 hex format.
*
* \return
* bcdNum - An 8-bit BCD equivalent of the passed hexadecimal number.
*
* For example, for 0x11223344 hexadecimal number, the function returns 
* 0x20 BCD number.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_RTC_ConvertDecToBcd(uint32_t decNum)
{
    uint32_t retVal;
    uint32_t tmpVal;

    tmpVal = decNum % CY_RTC_BCD_HUNDRED_DEGRE;
    retVal = ((uint32_t)(tmpVal / CY_RTC_BCD_DOZED_DEGREE)) << CY_RTC_BCD_NUMBER_SIZE;
    retVal += tmpVal % CY_RTC_BCD_DOZED_DEGREE;

    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_GetHoursFormat
****************************************************************************//**
*
* Returns current 12/24 hours format.
*
* \note
* Before getting the RTC current hours format, the Cy_RTC_SyncFromRtc() function
* should be called.
*
* \return
* The current RTC hours format. See \ref cy_en_rtc_hours_format_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_rtc_hours_format_t Cy_RTC_GetHoursFormat(void)
{
    return((_FLD2BOOL(BACKUP_RTC_TIME_CTRL_12HR, BACKUP_RTC_TIME)) ? CY_RTC_12_HOURS : CY_RTC_24_HOURS);
}


/*******************************************************************************
* Function Name: Cy_RTC_IsExternalResetOccurred
****************************************************************************//**
*
* The function checks the reset cause and returns the Boolean result.
*
* \return
* True if the reset reason is the power cycle and the XRES (external reset) <br>
* False if the reset reason is other than power cycle and the XRES.
*
* \note Based on a return value the RTC time and date can be updated or skipped 
* after the device reset. For example, you should skip the 
* Cy_RTC_SetAlarmDateAndTime() call function if internal WDT reset occurs.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_RTC_IsExternalResetOccurred(void)
{
    return(0u == Cy_SysLib_GetResetReason());
}


/*******************************************************************************
* Function Name: Cy_RTC_SyncToRtcAhbDateAndTime
****************************************************************************//**
*
* This function updates new time and date into the time and date RTC AHB 
* registers.
*
* \param timeBcd
* The BCD-formatted time variable which has the same bit masks as the 
* RTC_TIME register: <br>
* [0:6]   - Calendar seconds in BCD, the range 0-59. <br>
* [14:8]  - Calendar minutes in BCD, the range 0-59. <br>
* [21:16] - Calendar hours in BCD, value depends on the 12/24-hour mode. <br>
* 12HR: [21]:0 = AM, 1 = PM, [20:16] = 1 - 12;  <br>
* 24HR: [21:16] = 0-23. <br>
* [22]    - Selects the 12/24-hour mode: 1 - 12-hour, 0 - 24-hour. <br>
* [26:24] - A calendar day of the week, the range 1 - 7, where 1 - Sunday. <br>
*
* \param dateBcd
* The BCD-formatted time variable which has the same bit masks as the 
* RTC_DATE register: <br>
* [5:0]   - A calendar day of a month in BCD, the range 1-31. <br>
* [12:8]  - A calendar month in BCD, the range 1-12. <br>
* [23:16] - A calendar year in BCD, the range 0-99. <br>
*
* \note Ensure that the parameters are presented in the BCD format. Use the
* ConstructTimeDate() function to construct BCD time and date values.
* Refer to ConstructTimeDate() function description for more details 
* about the RTC_TIME and RTC_DATE bit fields format.  
*
* The RTC AHB registers can be updated only under condition that the
* Write bit is set and the RTC busy bit is cleared (RTC_BUSY = 0). Call the 
* Cy_RTC_WriteEnable(CY_RTC_WRITE_ENABLED) and ensure that Cy_RTC_WriteEnable()
* returned CY_RTC_SUCCESS. Then you can call Cy_RTC_SyncToRtcAhbDateAndTime().
* Do not forget to clear the RTC Write bit to finish an RTC register update by 
* calling Cy_RTC_WriteEnable(CY_RTC_WRITE_DISABLED) after you executed 
* Cy_RTC_SyncToRtcAhbDateAndTime(). Ensure that Cy_RTC_WriteEnable()
* retuned CY_RTC_SUCCESS.
*
*******************************************************************************/
__STATIC_INLINE void Cy_RTC_SyncToRtcAhbDateAndTime(uint32_t timeBcd, uint32_t dateBcd)
{
    BACKUP_RTC_TIME = timeBcd;
    BACKUP_RTC_DATE = dateBcd;
}


/*******************************************************************************
* Function Name: Cy_RTC_SyncToRtcAhbAlarm
****************************************************************************//**
*
* This function updates new alarm time and date into the alarm tire and date 
* RTC AHB registers.
*
* \param alarmTimeBcd
* The BCD-formatted time variable which has the same bit masks as the 
* ALMx_TIME register time fields: <br>
* [0:6]   - Alarm seconds in BCD, the range 0-59. <br>
* [7]     - Alarm seconds Enable: 0 - ignore, 1 - match. <br>
* [14:8]  - Alarm minutes in BCD, the range 0-59. <br>
* [15]    - Alarm minutes Enable: 0 - ignore, 1 - match. <br>
* [21:16] - Alarm hours in BCD, value depending on the 12/24-hour mode  
* (RTC_CTRL_12HR) <br>
* 12HR: [21]:0 = AM, 1 = PM, [20:16] = 1 - 12;  <br>
* 24HR: [21:16] = the range 0-23. <br>
* [23]    - Alarm hours Enable: 0 - ignore, 1 - match. <br>
* [26:24] - An alarm day of the week, the range 1 - 7, where 1 - Monday. <br>
* [31]    - An alarm day of the week Enable: 0 - ignore, 1 - match. <br>
*
* \param alarmDateBcd
* The BCD-formatted date variable which has the same bit masks as the 
* ALMx_DATE register date fields: <br>
* [5:0]  - An alarm day of a month in BCD, the range 1-31. <br>
* [7]    - An alarm day of a month Enable: 0 - ignore, 1 - match. <br>
* [12:8] - An alarm month in BCD, the range 1-12. <br>
* [15]   - An alarm month Enable: 0 - ignore, 1 - match. <br>
* [31]   - The Enable alarm: 0 - Alarm is disabled, 1 - Alarm is enabled. <br>
*
* \param alarmIndex
* The alarm index to be configured, see \ref cy_en_rtc_alarm_t.
*
* \note Ensure that the parameters are presented in the BCD format. Use the 
* ConstructTimeDate() function to construct BCD time and date values.
* Refer to ConstructTimeDate() function description for more details 
* about the RTC ALMx_TIME and ALMx_DATE bit-fields format.  
*
* The RTC AHB registers can be updated only under condition that the
* Write bit is set and the RTC busy bit is cleared (RTC_BUSY = 0). Call the 
* Cy_RTC_WriteEnable(CY_RTC_WRITE_ENABLED) and ensure that Cy_RTC_WriteEnable()
* returned CY_RTC_SUCCESS. Then you can call Cy_RTC_SyncToRtcAhbDateAndTime().
* Do not forget to clear the RTC Write bit to finish an RTC register update by 
* calling the Cy_RTC_WriteEnable(CY_RTC_WRITE_DISABLED) after you executed 
* Cy_RTC_SyncToRtcAhbDateAndTime(). Ensure that Cy_RTC_WriteEnable()
* retuned CY_RTC_SUCCESS.
*
*******************************************************************************/
__STATIC_INLINE void Cy_RTC_SyncToRtcAhbAlarm(uint32_t alarmTimeBcd, uint32_t alarmDateBcd, cy_en_rtc_alarm_t alarmIndex)
{
    CY_ASSERT_L3(CY_RTC_IS_ALARM_IDX_VALID(alarmIndex));
    
    if (alarmIndex != CY_RTC_ALARM_2)
    {
        BACKUP_ALM1_TIME = alarmTimeBcd;
        BACKUP_ALM1_DATE = alarmDateBcd;
    }
    else
    {
        BACKUP_ALM2_TIME = alarmTimeBcd;
        BACKUP_ALM2_DATE = alarmDateBcd;
    }
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_RTC_H */

/** \} group_rtc_low_level_functions */
/** \} group_rtc */


/* [] END OF FILE */
