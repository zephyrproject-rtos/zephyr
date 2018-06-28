/***************************************************************************//**
* \file cy_rtc.c
* \version 2.20
*
* This file provides constants and parameter values for the APIs for the 
* Real-Time Clock (RTC).
*
********************************************************************************
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_rtc.h"

#ifdef CY_IP_MXS40SRSS_RTC

#if defined(__cplusplus)
extern "C" {
#endif

/** RTC days in months table */
uint8_t const cy_RTC_daysInMonthTbl[CY_RTC_MONTHS_PER_YEAR] = {CY_RTC_DAYS_IN_JANUARY,
                                                               CY_RTC_DAYS_IN_FEBRUARY,
                                                               CY_RTC_DAYS_IN_MARCH,
                                                               CY_RTC_DAYS_IN_APRIL,
                                                               CY_RTC_DAYS_IN_MAY,
                                                               CY_RTC_DAYS_IN_JUNE,
                                                               CY_RTC_DAYS_IN_JULY,
                                                               CY_RTC_DAYS_IN_AUGUST,
                                                               CY_RTC_DAYS_IN_SEPTEMBER,
                                                               CY_RTC_DAYS_IN_OCTOBER,
                                                               CY_RTC_DAYS_IN_NOVEMBER,
                                                               CY_RTC_DAYS_IN_DECEMBER};

/* Static functions */
static void ConstructTimeDate(cy_stc_rtc_config_t const *timeDate, uint32_t *timeBcd, uint32_t *dateBcd);
static void ConstructAlarmTimeDate(cy_stc_rtc_alarm_t const *alarmDateTime, uint32_t *alarmTimeBcd,
                                                                                   uint32_t *alarmDateBcd);
static uint32_t RelativeToFixed(cy_stc_rtc_dst_format_t const *convertDst);


/*******************************************************************************
* Function Name: Cy_RTC_Init
****************************************************************************//**
*
* Initializes the RTC driver.
*
* \param *config
* The pointer to the RTC configuration structure, see \ref cy_stc_rtc_config_t.
*
* \return
* cy_en_rtc_status_t *config checking result. If the pointer is NULL, 
* returns an error.
*
*******************************************************************************/
cy_en_rtc_status_t Cy_RTC_Init(cy_stc_rtc_config_t const *config)
{
    return(Cy_RTC_SetDateAndTime(config));
}


/*******************************************************************************
* Function Name: Cy_RTC_SetDateAndTime
****************************************************************************//**
*
* Sets the time and date values into the RTC_TIME and RTC_DATE registers.
*
* \param dateTime
* The pointer to the RTC configuration structure, see \ref cy_stc_rtc_config_t.
*
* \return
* cy_en_rtc_status_t A validation check result of date and month. Returns an 
* error, if the date range is invalid.
*
*******************************************************************************/
cy_en_rtc_status_t Cy_RTC_SetDateAndTime(cy_stc_rtc_config_t const *dateTime)
{
    cy_en_rtc_status_t retVal = CY_RTC_BAD_PARAM;

    if (NULL != dateTime)
    {
        uint32_t tmpDaysInMonth;

        CY_ASSERT_L3(CY_RTC_IS_SEC_VALID(dateTime->sec));
        CY_ASSERT_L3(CY_RTC_IS_MIN_VALID(dateTime->min));
        CY_ASSERT_L3(CY_RTC_IS_HOUR_VALID(dateTime->hour));
        CY_ASSERT_L3(CY_RTC_IS_DOW_VALID(dateTime->dayOfWeek));
        CY_ASSERT_L3(CY_RTC_IS_MONTH_VALID(dateTime->month));
        CY_ASSERT_L3(CY_RTC_IS_YEAR_SHORT_VALID(dateTime->year));

        /* Check dateTime->date input */
        tmpDaysInMonth = Cy_RTC_DaysInMonth(dateTime->month, (dateTime->year + CY_RTC_TWO_THOUSAND_YEARS));

        if ((dateTime->date > 0U) && (dateTime->date <= tmpDaysInMonth))
        {
            uint32_t interruptState;
            uint32_t tmpTime;
            uint32_t tmpDate;

            ConstructTimeDate(dateTime, &tmpTime, &tmpDate);

            /* The RTC AHB registers can be updated only under condition that the
            *  Write bit is set and the RTC busy bit is cleared (CY_RTC_BUSY = 0).
            */
            interruptState = Cy_SysLib_EnterCriticalSection();
            retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_ENABLED);
            if (retVal == CY_RTC_SUCCESS)
            {
                BACKUP_RTC_TIME = tmpTime;
                BACKUP_RTC_DATE = tmpDate;
                
                /* Clear the RTC Write bit to finish RTC register update */
                retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_DISABLED);
            }
            Cy_SysLib_ExitCriticalSection(interruptState);
        }
    }
    return(retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_GetDateAndTime
****************************************************************************//**
*
* Gets the current RTC time and date. The AHB RTC Time and Date register values 
* are stored into the *dateTime structure.
*
* \param dateTime
* The RTC time and date structure. See \ref group_rtc_data_structures.
*
*******************************************************************************/
void   Cy_RTC_GetDateAndTime(cy_stc_rtc_config_t* dateTime)
{
    uint32_t tmpTime;
    uint32_t tmpDate;

    CY_ASSERT_L1(NULL != dateTime);

    /* Read the current RTC time and date to validate the input parameters */
    Cy_RTC_SyncFromRtc();

    /* Write the AHB RTC registers date and time into the local variables and 
    * updating the dateTime structure elements
    */
    tmpTime = BACKUP_RTC_TIME;
    tmpDate = BACKUP_RTC_DATE;

    dateTime->sec    = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_TIME_RTC_SEC, tmpTime));
    dateTime->min    = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_TIME_RTC_MIN, tmpTime));
    dateTime->hrFormat = ((_FLD2BOOL(BACKUP_RTC_TIME_CTRL_12HR, tmpTime)) ? CY_RTC_12_HOURS : CY_RTC_24_HOURS);

    /* Read the current hour mode to know how many hour bits should be converted
    * In the 24-hour mode, the hour value is presented in [21:16] bits in the 
    * BCD format.
    * In the 12-hour mode the hour value is presented in [20:16] bits in the BCD
    * format and bit [21] is present: 0 - AM; 1 - PM. 
    */
    if (dateTime->hrFormat != CY_RTC_24_HOURS)
    {
        dateTime->hour = 
        Cy_RTC_ConvertBcdToDec((tmpTime & CY_RTC_BACKUP_RTC_TIME_RTC_12HOUR) >> BACKUP_RTC_TIME_RTC_HOUR_Pos);

        dateTime->amPm = ((0U != (tmpTime & CY_RTC_BACKUP_RTC_TIME_RTC_PM)) ? CY_RTC_PM : CY_RTC_AM);
    }
    else
    {
        dateTime->hour = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_TIME_RTC_HOUR, tmpTime));

        dateTime->amPm = CY_RTC_AM;
    }
    dateTime->dayOfWeek = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_TIME_RTC_DAY, tmpTime));
    
    dateTime->date  = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_DATE_RTC_DATE, tmpDate));
    dateTime->month = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_DATE_RTC_MON, tmpDate));
    dateTime->year  = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_DATE_RTC_YEAR, tmpDate));
}


/*******************************************************************************
* Function Name: Cy_RTC_SetAlarmDateAndTime
****************************************************************************//**
*
* Sets alarm time and date values into the ALMx_TIME and ALMx_DATE registers.
*
* \param alarmDateTime
* The alarm configuration structure, see \ref cy_stc_rtc_alarm_t.
*
* \param alarmIndex
* The alarm index to be configured, see \ref cy_en_rtc_alarm_t.
*
* \return
* cy_en_rtc_status_t A validation check result of date and month. Returns an
* error, if the date range is invalid.
*
*******************************************************************************/
cy_en_rtc_status_t Cy_RTC_SetAlarmDateAndTime(cy_stc_rtc_alarm_t const *alarmDateTime, cy_en_rtc_alarm_t alarmIndex)
{
    cy_en_rtc_status_t retVal = CY_RTC_BAD_PARAM;

    if (NULL != alarmDateTime)
    {
        uint32_t tmpYear;
        uint32_t tmpDaysInMonth;

        CY_ASSERT_L3(CY_RTC_IS_ALARM_IDX_VALID(alarmIndex));

        CY_ASSERT_L3(CY_RTC_IS_SEC_VALID(alarmDateTime->sec));
        CY_ASSERT_L3(CY_RTC_IS_ALARM_EN_VALID(alarmDateTime->secEn));

        CY_ASSERT_L3(CY_RTC_IS_MIN_VALID(alarmDateTime->min));
        CY_ASSERT_L3(CY_RTC_IS_ALARM_EN_VALID(alarmDateTime->minEn));

        CY_ASSERT_L3(CY_RTC_IS_HOUR_VALID(alarmDateTime->hour));
        CY_ASSERT_L3(CY_RTC_IS_ALARM_EN_VALID(alarmDateTime->hourEn));

        CY_ASSERT_L3(CY_RTC_IS_DOW_VALID(alarmDateTime->dayOfWeek));
        CY_ASSERT_L3(CY_RTC_IS_ALARM_EN_VALID(alarmDateTime->dayOfWeekEn));

        CY_ASSERT_L3(CY_RTC_IS_MONTH_VALID(alarmDateTime->month));
        CY_ASSERT_L3(CY_RTC_IS_ALARM_EN_VALID(alarmDateTime->monthEn));

        CY_ASSERT_L3(CY_RTC_IS_ALARM_EN_VALID(alarmDateTime->dateEn));

        CY_ASSERT_L3(CY_RTC_IS_ALARM_EN_VALID(alarmDateTime->almEn));

        /* Read the current RTC year to validate alarmDateTime->date */
        Cy_RTC_SyncFromRtc();

        tmpYear = 
        CY_RTC_TWO_THOUSAND_YEARS + Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_DATE_RTC_YEAR, BACKUP_RTC_DATE));

        tmpDaysInMonth = Cy_RTC_DaysInMonth(alarmDateTime->month, tmpYear);

        if ((alarmDateTime->date > 0U) && (alarmDateTime->date <= tmpDaysInMonth))
        {
            uint32_t interruptState;
            uint32_t tmpAlarmTime;
            uint32_t tmpAlarmDate;
            
            ConstructAlarmTimeDate(alarmDateTime, &tmpAlarmTime, &tmpAlarmDate);
            
            /* The RTC AHB registers can be updated only under condition that the
            *  Write bit is set and the RTC busy bit is cleared (RTC_BUSY = 0).
            */
            interruptState = Cy_SysLib_EnterCriticalSection();
            retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_ENABLED);
            if (CY_RTC_SUCCESS == retVal)
            {
                /* Update the AHB RTC registers with formed values */
                if (alarmIndex != CY_RTC_ALARM_2)
                {
                    BACKUP_ALM1_TIME = tmpAlarmTime;
                    BACKUP_ALM1_DATE = tmpAlarmDate;
                }
                else
                {
                    BACKUP_ALM2_TIME = tmpAlarmTime;
                    BACKUP_ALM2_DATE = tmpAlarmDate;
                }

                /* Clear the RTC Write bit to finish RTC update */
                retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_DISABLED);
            }
            Cy_SysLib_ExitCriticalSection(interruptState);
        }
    }
    return(retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_GetAlarmDateAndTime
****************************************************************************//**
*
* Returns the current alarm time and date values from the ALMx_TIME and 
* ALMx_DATE registers.
*
* \param alarmDateTime
* The alarm configuration structure, see \ref cy_stc_rtc_alarm_t.
*
* \param alarmIndex
* The alarm index to be configured, see \ref cy_en_rtc_alarm_t.
*
*******************************************************************************/
void   Cy_RTC_GetAlarmDateAndTime(cy_stc_rtc_alarm_t *alarmDateTime, cy_en_rtc_alarm_t alarmIndex)
{
    uint32_t tmpAlarmTime;
    uint32_t tmpAlarmDate;
    cy_en_rtc_hours_format_t curHoursFormat;

    CY_ASSERT_L1(NULL != alarmDateTime);
    CY_ASSERT_L3(CY_RTC_IS_ALARM_IDX_VALID(alarmIndex));

    /* Read the current RTC time and date to validate the input parameters */
    Cy_RTC_SyncFromRtc();

    curHoursFormat = Cy_RTC_GetHoursFormat();

    /* Write the AHB RTC registers into the local variables and update the
    * alarmDateTime structure elements
    */
    if (alarmIndex != CY_RTC_ALARM_2)
    {
        tmpAlarmTime = BACKUP_ALM1_TIME;
        tmpAlarmDate = BACKUP_ALM1_DATE;

        alarmDateTime->sec   = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM1_TIME_ALM_SEC, tmpAlarmTime));
        alarmDateTime->secEn = 
        ((_FLD2BOOL(BACKUP_ALM1_TIME_ALM_SEC_EN, tmpAlarmTime)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        alarmDateTime->min   = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM1_TIME_ALM_MIN, tmpAlarmTime));
        alarmDateTime->minEn = 
        ((_FLD2BOOL(BACKUP_ALM1_TIME_ALM_MIN_EN, tmpAlarmTime)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        /* Read the current hour mode to know how many hour bits to convert.
        *  In the 24-hour mode, the hour value is presented in [21:16] bits in 
        *  the BCD format.
        *  In the 12-hour mode, the hour value is presented in [20:16] bits in 
        *  the BCD format and bit [21] is present: 0 - AM; 1 - PM.
        */
        if (curHoursFormat != CY_RTC_24_HOURS)
        {
            alarmDateTime->hour = 
            Cy_RTC_ConvertBcdToDec((tmpAlarmTime & CY_RTC_BACKUP_RTC_TIME_RTC_12HOUR) 
                                                                         >> BACKUP_ALM1_TIME_ALM_HOUR_Pos);

            /* In the structure, the hour value should be presented in the 24-hour mode. In 
            *  that condition the firmware checks the AM/PM status and adds 12 hours to 
            *  the converted hour value if the PM bit is set.
            */
            if ((alarmDateTime->hour < CY_RTC_HOURS_PER_HALF_DAY) && 
            (0U != (BACKUP_ALM1_TIME & CY_RTC_BACKUP_RTC_TIME_RTC_PM)))
            {
                alarmDateTime->hour += CY_RTC_HOURS_PER_HALF_DAY;
            }

            /* Set zero hour, as the 12 A hour is zero hour in 24-hour format */
            if ((alarmDateTime->hour == CY_RTC_HOURS_PER_HALF_DAY) && 
              (0U == (BACKUP_ALM1_TIME & CY_RTC_BACKUP_RTC_TIME_RTC_PM)))
            {
                alarmDateTime->hour = 0U;
            }

        }
        else
        {
            alarmDateTime->hour = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM1_TIME_ALM_HOUR, tmpAlarmTime));
        }
        alarmDateTime->hourEn = 
        ((_FLD2BOOL(BACKUP_ALM1_TIME_ALM_HOUR_EN, tmpAlarmTime)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);
        
        alarmDateTime->dayOfWeek = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM1_TIME_ALM_DAY, tmpAlarmTime));
        alarmDateTime->dayOfWeekEn =
        ((_FLD2BOOL(BACKUP_ALM1_TIME_ALM_DAY_EN, tmpAlarmTime)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        alarmDateTime->date = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM1_DATE_ALM_DATE, tmpAlarmDate));
        alarmDateTime->dateEn  = 
        ((_FLD2BOOL(BACKUP_ALM1_DATE_ALM_DATE_EN, tmpAlarmDate)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        alarmDateTime->month = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM1_DATE_ALM_MON, tmpAlarmDate)); 
        alarmDateTime->monthEn = 
        ((_FLD2BOOL(BACKUP_ALM1_DATE_ALM_MON_EN, tmpAlarmDate)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        alarmDateTime->almEn = 
        ((_FLD2BOOL(BACKUP_ALM1_DATE_ALM_EN, tmpAlarmDate)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);
    }
    else
    {
        tmpAlarmTime = BACKUP_ALM2_TIME;
        tmpAlarmDate = BACKUP_ALM2_DATE;

        alarmDateTime->sec   = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM2_TIME_ALM_SEC, tmpAlarmTime));
        alarmDateTime->secEn = 
        ((_FLD2BOOL(BACKUP_ALM2_TIME_ALM_SEC_EN, tmpAlarmTime)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        alarmDateTime->min   = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM2_TIME_ALM_MIN, tmpAlarmTime));
        alarmDateTime->minEn = 
        ((_FLD2BOOL(BACKUP_ALM2_TIME_ALM_MIN_EN, tmpAlarmTime)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        /* Read the current hour mode to know how many hour bits to convert.
        *  In the 24-hour mode, the hour value is presented in [21:16] bits in 
        *  the BCD format.
        *  In the 12-hour mode the hour value is presented in [20:16] bits in 
        *  the BCD format and bit [21] is present: 0 - AM; 1 - PM.
        */
        if (curHoursFormat != CY_RTC_24_HOURS)
        {
            alarmDateTime->hour = Cy_RTC_ConvertBcdToDec((tmpAlarmTime & CY_RTC_BACKUP_RTC_TIME_RTC_12HOUR) >>
                                                                             BACKUP_ALM2_TIME_ALM_HOUR_Pos);

            /* In the structure, the hour value should be presented in the 24-hour mode. In 
            *  that condition the firmware checks the AM/PM status and adds 12 hours to 
            *  the converted hour value if the PM bit is set.
            */
            if ((alarmDateTime->hour < CY_RTC_HOURS_PER_HALF_DAY) && 
            (0U != (BACKUP_ALM2_TIME & CY_RTC_BACKUP_RTC_TIME_RTC_PM)))
            {
                alarmDateTime->hour += CY_RTC_HOURS_PER_HALF_DAY;
            }
            /* Set zero hour, as the 12 am hour is zero hour in 24-hour format */
            else if ((alarmDateTime->hour == CY_RTC_HOURS_PER_HALF_DAY) && 
                    (0U == (BACKUP_ALM2_TIME & CY_RTC_BACKUP_RTC_TIME_RTC_PM)))
            {
                alarmDateTime->hour = 0U;
            }
            else
            {
                /* No corrections are required */
            }
        }
        else
        {
            alarmDateTime->hour = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM2_TIME_ALM_HOUR, tmpAlarmTime));
        }
        alarmDateTime->hourEn = 
        ((_FLD2BOOL(BACKUP_ALM2_TIME_ALM_HOUR_EN, tmpAlarmTime)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);
        
        alarmDateTime->dayOfWeek = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM2_TIME_ALM_DAY, tmpAlarmTime));
        alarmDateTime->dayOfWeekEn =
        ((_FLD2BOOL(BACKUP_ALM2_TIME_ALM_DAY_EN, tmpAlarmTime)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        alarmDateTime->date = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM2_DATE_ALM_DATE, tmpAlarmDate));
        alarmDateTime->dateEn  = 
        ((_FLD2BOOL(BACKUP_ALM2_DATE_ALM_DATE_EN, tmpAlarmDate)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        alarmDateTime->month = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_ALM2_DATE_ALM_MON, tmpAlarmDate)); 
        alarmDateTime->monthEn = 
        ((_FLD2BOOL(BACKUP_ALM2_DATE_ALM_MON_EN, tmpAlarmDate)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);

        alarmDateTime->almEn = 
        ((_FLD2BOOL(BACKUP_ALM2_DATE_ALM_EN, tmpAlarmDate)) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE);
    }
}


/*******************************************************************************
* Function Name: Cy_RTC_SetDateAndTimeDirect
****************************************************************************//**
*
* Sets the time and date values into the RTC_TIME and RTC_DATE registers using 
* direct time parameters.
*
* \param sec The second valid range is [0-59].
*
* \param min The minute valid range is [0-59].
*
* \param hour
* The hour valid range is [0-23]. This parameter should be presented in the 
* 24-hour format.
*
* The function reads the current 12/24-hour mode, then converts the hour value
* properly as the mode.
*
* \param date
* The date valid range is [1-31], if the month of February is 
* selected as the Month parameter, then the valid range is [0-29].
*
* \param month The month valid range is [1-12].
*
* \param year The year valid range is [0-99].
*
* \return
* cy_en_rtc_status_t A validation check result of date and month. Returns an 
* error, if the date range is invalid or the RTC time and date set was 
* cancelled: the RTC Write bit was not set, the RTC was synchronizing.
*
*******************************************************************************/
cy_en_rtc_status_t Cy_RTC_SetDateAndTimeDirect(uint32_t sec, uint32_t min, uint32_t hour, 
                                               uint32_t date, uint32_t month, uint32_t year)
{
    uint32_t tmpDaysInMonth;
    cy_en_rtc_status_t retVal = CY_RTC_BAD_PARAM;

    CY_ASSERT_L3(CY_RTC_IS_SEC_VALID(sec));
    CY_ASSERT_L3(CY_RTC_IS_MIN_VALID(min));
    CY_ASSERT_L3(CY_RTC_IS_HOUR_VALID(hour));
    CY_ASSERT_L3(CY_RTC_IS_MONTH_VALID(month));
    CY_ASSERT_L3(CY_RTC_IS_YEAR_SHORT_VALID(year));

    /* Check date input */
    tmpDaysInMonth = Cy_RTC_DaysInMonth(month, (year + CY_RTC_TWO_THOUSAND_YEARS));

    if ((date > 0U) && (date <= tmpDaysInMonth))
    {
        cy_stc_rtc_config_t curTimeAndDate;
        uint32_t tmpTime;
        uint32_t tmpDate;
        uint32_t interruptState;
        
        /* Fill the date and time structure */
        curTimeAndDate.sec = sec;
        curTimeAndDate.min = min;
        
        /* Read the current hour mode */
        Cy_RTC_SyncFromRtc();
        
        if (CY_RTC_12_HOURS != Cy_RTC_GetHoursFormat())
        {
            curTimeAndDate.hrFormat = CY_RTC_24_HOURS;
            curTimeAndDate.hour = hour;
        }
        else
        {
            curTimeAndDate.hrFormat = CY_RTC_12_HOURS;

            /* Convert the 24-hour format input value into the 12-hour format */
            if (hour >= CY_RTC_HOURS_PER_HALF_DAY)
            {
                /* The current hour is more than 12 or equal 12, in the 24-hour 
                *  format. Set the PM bit and convert the hour: hour = hour - 12, 
                *  except that the hour is 12.
                */
                curTimeAndDate.hour = 
                (hour > CY_RTC_HOURS_PER_HALF_DAY) ? ((uint32_t) hour - CY_RTC_HOURS_PER_HALF_DAY) : hour;

                curTimeAndDate.amPm = CY_RTC_PM;
            }
            else
            {
                /* The current hour is less than 12 AM. The zero hour is equal 
                *  to 12:00 AM
                */
                curTimeAndDate.hour = ((hour == 0U) ? CY_RTC_HOURS_PER_HALF_DAY : hour);
                curTimeAndDate.amPm = CY_RTC_AM;
            }
        }
        curTimeAndDate.dayOfWeek = Cy_RTC_ConvertDayOfWeek(date, month, (year + CY_RTC_TWO_THOUSAND_YEARS));
        curTimeAndDate.date = date;
        curTimeAndDate.month = month;
        curTimeAndDate.year = year;

        ConstructTimeDate(&curTimeAndDate, &tmpTime, &tmpDate);

        /* The RTC AHB register can be updated only under condition that the
        *  Write bit is set and the RTC busy bit is cleared (CY_RTC_BUSY = 0).
        */
        interruptState = Cy_SysLib_EnterCriticalSection();
        retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_ENABLED);
        if (retVal == CY_RTC_SUCCESS)
        {
            BACKUP_RTC_TIME = tmpTime;
            BACKUP_RTC_DATE = tmpDate;

            /* Clear the RTC Write bit to finish RTC register update */
            retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_DISABLED);
        }
        Cy_SysLib_ExitCriticalSection(interruptState);
    }

    return(retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_SetAlarmDateAndTimeDirect
****************************************************************************//**
*
* Sets alarm time and date values into the ALMx_TIME and ALMx_DATE 
* registers using direct time parameters. ALM_DAY_EN is default 0 (=ignore) for 
* this function.
*
* \param sec The alarm second valid range is [0-59].
*
* \param min The alarm minute valid range is [0-59].
*
* \param hour
* The valid range is [0-23].
* This parameter type is always in the 24-hour type. This function reads the 
* current 12/24-hour mode, then converts the hour value properly as the mode.
*
* \param date
* The valid range is [1-31], if the month of February is selected as
* the Month parameter, then the valid range is [0-29].
*
* \param month The alarm month valid range is [1-12].
*
* \param alarmIndex
* The alarm index to be configured, see \ref cy_en_rtc_alarm_t.
*
* \return
* cy_en_rtc_status_t A validation check result of date and month. Returns an 
* error, if the date range is invalid.
*
*******************************************************************************/
cy_en_rtc_status_t Cy_RTC_SetAlarmDateAndTimeDirect(uint32_t sec, uint32_t min, uint32_t hour, 
                                                    uint32_t date, uint32_t month, cy_en_rtc_alarm_t alarmIndex)
{
    uint32_t tmpDaysInMonth;
    uint32_t tmpCurrentYear;
    cy_en_rtc_status_t retVal = CY_RTC_BAD_PARAM;

    CY_ASSERT_L3(CY_RTC_IS_SEC_VALID(sec));
    CY_ASSERT_L3(CY_RTC_IS_MIN_VALID(min));
    CY_ASSERT_L3(CY_RTC_IS_HOUR_VALID(hour));
    CY_ASSERT_L3(CY_RTC_IS_MONTH_VALID(month));
    CY_ASSERT_L3(CY_RTC_IS_ALARM_IDX_VALID(alarmIndex));
    
    /* Read the current time to validate the input parameters */
    Cy_RTC_SyncFromRtc();

    /* Get the current year value to calculate */
    tmpCurrentYear =
    Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_DATE_RTC_YEAR, BACKUP_RTC_DATE));

    tmpDaysInMonth = Cy_RTC_DaysInMonth(month, (tmpCurrentYear + CY_RTC_TWO_THOUSAND_YEARS));

    if ((date > 0U) && (date <= tmpDaysInMonth))
    {
        uint32_t tmpAlarmTime;
        uint32_t tmpAlarmDate;
        uint32_t interruptState;
        cy_stc_rtc_alarm_t alarmDateTime;
        
        /* Fill the alarm structure */
        alarmDateTime.sec         = sec;
        alarmDateTime.secEn       = CY_RTC_ALARM_ENABLE;
        alarmDateTime.min         = min;
        alarmDateTime.minEn       = CY_RTC_ALARM_ENABLE;
        alarmDateTime.hour        = hour;
        alarmDateTime.hourEn      = CY_RTC_ALARM_ENABLE;
        alarmDateTime.dayOfWeek   = CY_RTC_SUNDAY;
        alarmDateTime.dayOfWeekEn = CY_RTC_ALARM_DISABLE;

        alarmDateTime.date    = date;
        alarmDateTime.dateEn  = CY_RTC_ALARM_ENABLE;
        alarmDateTime.month   = month;
        alarmDateTime.monthEn = CY_RTC_ALARM_ENABLE;
        alarmDateTime.almEn   = CY_RTC_ALARM_ENABLE;

        ConstructAlarmTimeDate(&alarmDateTime, &tmpAlarmTime, &tmpAlarmDate);

        /* The RTC AHB register can be updated only under condition that the
        *  Write bit is set and the RTC busy bit is cleared (CY_RTC_BUSY = 0).
        */
        interruptState = Cy_SysLib_EnterCriticalSection();
        retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_ENABLED);
        if (retVal == CY_RTC_SUCCESS)
        {
            if (alarmIndex != CY_RTC_ALARM_2)
            {
                BACKUP_ALM1_TIME = tmpAlarmTime;
                BACKUP_ALM1_DATE = tmpAlarmDate;
            }
            else
            {
                BACKUP_ALM2_TIME = tmpAlarmTime;
                BACKUP_ALM2_DATE = tmpAlarmDate;
            }

            /* Clear the RTC Write bit to finish RTC register update */
            retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_DISABLED);
        }
        Cy_SysLib_ExitCriticalSection(interruptState);
    }

    return(retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_SetHoursFormat
****************************************************************************//**
*
* Sets the 12/24-hour mode.
*
* \param hoursFormat 
* The current hour format, see \ref cy_en_rtc_hours_format_t.
*
* \return cy_en_rtc_status_t A validation check result of RTC register update.
*
*******************************************************************************/
cy_en_rtc_status_t Cy_RTC_SetHoursFormat(cy_en_rtc_hours_format_t hoursFormat)
{
    uint32_t curTime;
    cy_en_rtc_status_t retVal = CY_RTC_BAD_PARAM;

    CY_ASSERT_L3(CY_RTC_IS_HRS_FORMAT_VALID(hoursFormat));
    
    /* Read the current time to validate the input parameters */
    Cy_RTC_SyncFromRtc();
    curTime = BACKUP_RTC_TIME;

    /* Hour format can be changed in condition that current hour format is not 
    * the same as requested in function argument
    */
    if (hoursFormat != Cy_RTC_GetHoursFormat())
    {
        uint32_t hourValue;

        /* Convert the current hour value from 24H into the 12H format */
        if (hoursFormat == CY_RTC_12_HOURS)
        {
            hourValue = Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_TIME_RTC_HOUR, curTime));
            if (hourValue >= CY_RTC_HOURS_PER_HALF_DAY)
            {
                /* The current hour is more than 12 or equal 12 in the 24-hour 
                *  mode. Set the PM bit and convert the hour: hour = hour - 12.
                */
                hourValue = (uint32_t) (hourValue - CY_RTC_HOURS_PER_HALF_DAY);
                hourValue = ((0U != hourValue) ? hourValue : CY_RTC_HOURS_PER_HALF_DAY);

                curTime = (_CLR_SET_FLD32U(curTime, BACKUP_RTC_TIME_RTC_HOUR, Cy_RTC_ConvertDecToBcd(hourValue)));
                curTime |= CY_RTC_BACKUP_RTC_TIME_RTC_PM;
            }
            else if (hourValue < 1U)
            {
                /* The current hour in the 24-hour mode is 0 which is equal 
                *  to 12:00 AM
                */
                curTime =
                (_CLR_SET_FLD32U(curTime, BACKUP_RTC_TIME_RTC_HOUR, 
                  Cy_RTC_ConvertDecToBcd(CY_RTC_HOURS_PER_HALF_DAY)));

                /* Set the AM bit */
                curTime &= ((uint32_t) ~CY_RTC_BACKUP_RTC_TIME_RTC_PM);
            }
            else
            {
                /* The current hour is less than 12 */
                curTime = (_CLR_SET_FLD32U(curTime, BACKUP_RTC_TIME_RTC_HOUR, Cy_RTC_ConvertDecToBcd(hourValue)));
                curTime &= ((uint32_t) ~CY_RTC_BACKUP_RTC_TIME_RTC_PM);
            }

            curTime |= BACKUP_RTC_TIME_CTRL_12HR_Msk;
        }
        /* Convert the hour value into 24H format value */
        else
        {
            /* Mask the AM/PM bit as the hour value is in [20:16] bits */
            hourValue = 
            Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_TIME_RTC_HOUR, 
            (curTime & (uint32_t) ~CY_RTC_BACKUP_RTC_TIME_RTC_PM)));

            /* Add 12 hours in condition that current time is in PM period */
            if ((hourValue < CY_RTC_HOURS_PER_HALF_DAY) && (0U != (curTime & CY_RTC_BACKUP_RTC_TIME_RTC_PM)))
            {
                hourValue += CY_RTC_HOURS_PER_HALF_DAY;
            }

            /* Current hour is 12 AM which is equal to zero hour in 24-hour format */
            if ((hourValue == CY_RTC_HOURS_PER_HALF_DAY) && (0U == (curTime & CY_RTC_BACKUP_RTC_TIME_RTC_PM)))
            {
                hourValue = 0U;
            }

            curTime = (_CLR_SET_FLD32U(curTime, BACKUP_RTC_TIME_RTC_HOUR, Cy_RTC_ConvertDecToBcd(hourValue)));
            curTime &= (uint32_t) ~BACKUP_RTC_TIME_CTRL_12HR_Msk;
        }

        /* Writing corrected hour value and hour format bit into the RTC AHB 
        *  register. The RTC AHB register can be updated only under condition 
        *  that the Write bit is set and the RTC busy bit is cleared
        *  (CY_RTC_BUSY = 0).
        */
        uint32_t interruptState;

        interruptState = Cy_SysLib_EnterCriticalSection();
        retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_ENABLED);
        if (retVal == CY_RTC_SUCCESS)
        {
            BACKUP_RTC_TIME = curTime;
            /* Clear the RTC Write bit to finish RTC register update */
            retVal = Cy_RTC_WriteEnable(CY_RTC_WRITE_DISABLED);
        }
        Cy_SysLib_ExitCriticalSection(interruptState);
    }
    return(retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_SelectFrequencyPrescaler()
****************************************************************************//**
*
* Selects the RTC pre-scaler value and changes its clock frequency. 
* If the external 32.768 kHz WCO is absent on the board, the RTC can
* be driven by a 32.768kHz square clock source or an external 50-Hz or 60-Hz 
* sine-wave clock source, for example the wall AC frequency.
*
* \param clkSel clock frequency, see \ref cy_en_rtc_clock_freq_t.
*
* In addition to generating the 32.768 kHz clock from external crystals, the WCO
* can be sourced by an external clock source (50 Hz or 60Hz), even the wall AC 
* frequency as a timebase. The API helps select between the RTC sources:
* * A 32.768 kHz digital clock source <br>
* * An external 50-Hz or 60-Hz sine-wave clock source
*
* If you want to use an external 50-Hz or 60-Hz sine-wave clock source to 
* drive the RTC, the next procedure is required: <br>
* 1) Disable the WCO <br>
* 2) Bypass the WCO using the Cy_SysClk_WcoBypass() function <br>
* 3) Configure both wco_out and wco_in pins. Note that only one of the wco pins 
* should be driven and the other wco pin should be floating, which depends on 
* the source that drives the RTC (*1) <br>
* 4) Call Cy_RTC_SelectFrequencyPrescaler(CY_RTC_FREQ_60_HZ), if you want to 
* drive the WCO, for example, with a 60 Hz source <br>
* 5) Enable the WCO <br>
*
* If you want to use the WCO after using an external 50-Hz or 60-Hz sine-wave 
* clock source: <br>
* 1) Disable the WCO <br>
* 2) Switch-off the WCO bypass using the Cy_SysClk_WcoBypass() function <br>
* 3) Drive off the wco pin with an external signal source <br>
* 4) Call Cy_RTC_SelectFrequencyPrescaler(CY_RTC_FREQ_WCO_32768_HZ) <br>
* 5) Enable the WCO <br>
*
* (1) - Refer to the device TRM to know how to configure the wco pins properly 
* and which wco pin should be driven/floating.
*
* \warning 
* There is a limitation to the external clock source frequencies. Only two 
* frequencies are allowed - 50 Hz or 60 Hz. Note that this limitation is related
* to the RTC pre-scaling feature presented in this function. This 
* limitation is not related to WCO external clock sources which can drive the 
* WCO in Bypass mode.
*
*******************************************************************************/
void Cy_RTC_SelectFrequencyPrescaler(cy_en_rtc_clock_freq_t clkSel)
{
    CY_ASSERT_L3(CY_RTC_IS_CLK_VALID(clkSel));

    BACKUP_CTL = (_CLR_SET_FLD32U(BACKUP_CTL, BACKUP_CTL_PRESCALER, (uint32_t) clkSel));
}


/*******************************************************************************
* Function Name: Cy_RTC_EnableDstTime
****************************************************************************//**
* 
* The function sets the DST time and configures the ALARM2 interrupt register 
* with the appropriate DST time. This function sets the DST stop time if the 
* current time is already in the DST period. The DST period is a period of time 
* between the DST start time and DST stop time. The DST start time and DST stop 
* time is presented in the DST configuration structure, 
* see \ref cy_stc_rtc_dst_t.
*
* \param dstTime The DST configuration structure, see \ref cy_stc_rtc_dst_t.
*
* \param timeDate
* The time and date structure. The the appropriate DST time is 
* set based on this time and date, see \ref cy_stc_rtc_config_t.
*
* \return
* cy_en_rtc_status_t A validation check result of RTC register update.
*
*******************************************************************************/
cy_en_rtc_status_t Cy_RTC_EnableDstTime(cy_stc_rtc_dst_t const *dstTime, cy_stc_rtc_config_t const *timeDate)
{
    cy_en_rtc_status_t retVal = CY_RTC_BAD_PARAM;

    if ((NULL != dstTime) && (NULL != timeDate))
    {
        if (Cy_RTC_GetDstStatus(dstTime, timeDate))
        {
            retVal = Cy_RTC_SetNextDstTime(&dstTime->stopDst);
        }
        else
        {
            retVal = Cy_RTC_SetNextDstTime(&dstTime->startDst);
        }

        Cy_RTC_SetInterruptMask(CY_RTC_INTR_ALARM2);
    }

    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_SetNextDstTime
****************************************************************************//**
*
* Set the next time of the DST. This function sets the time to ALARM2 for a next
* DST event. If Cy_RTC_GetDSTStatus() is true(=1), the next DST event should be
* the DST stop, then this function should be called with the DST stop time. 
*
* If the time format(.format) is relative option(=0), the 
* RelativeToFixed() is called to convert to a fixed date. 
*
* \param nextDst 
* The structure with time at which a next DST event should occur 
* (ALARM2 interrupt should occur). See \ref cy_stc_rtc_config_t.
*
* \return
* cy_en_rtc_status_t A validation check result of RTC register update.
*
*******************************************************************************/
cy_en_rtc_status_t Cy_RTC_SetNextDstTime(cy_stc_rtc_dst_format_t const *nextDst)
{
    cy_en_rtc_status_t retVal = CY_RTC_BAD_PARAM;

    CY_ASSERT_L3(CY_RTC_IS_DST_FORMAT_VALID(nextDst->format));

    if (NULL != nextDst)
    {
        uint32_t tryesToSetup = CY_RTC_TRYES_TO_SETUP_DST;
        cy_stc_rtc_alarm_t dstAlarmTimeAndDate;

        /* Configure an alarm structure based on the DST structure */
        dstAlarmTimeAndDate.sec = 0U;
        dstAlarmTimeAndDate.secEn = CY_RTC_ALARM_ENABLE; 
        dstAlarmTimeAndDate.min = 0U;
        dstAlarmTimeAndDate.minEn = CY_RTC_ALARM_ENABLE;
        dstAlarmTimeAndDate.hour = nextDst->hour;
        dstAlarmTimeAndDate.hourEn = CY_RTC_ALARM_ENABLE;
        dstAlarmTimeAndDate.dayOfWeek = nextDst->dayOfWeek;
        dstAlarmTimeAndDate.dayOfWeekEn = CY_RTC_ALARM_DISABLE;

        /* Calculate a day-of-month value for the relative DST start structure */
        if (CY_RTC_DST_FIXED != nextDst->format)
        {
            dstAlarmTimeAndDate.date = RelativeToFixed(nextDst);
        }
        else
        {
            dstAlarmTimeAndDate.date = nextDst->dayOfMonth;
        }
        dstAlarmTimeAndDate.dateEn = CY_RTC_ALARM_ENABLE;
        dstAlarmTimeAndDate.month = nextDst->month;
        dstAlarmTimeAndDate.monthEn = CY_RTC_ALARM_ENABLE;
        dstAlarmTimeAndDate.almEn = CY_RTC_ALARM_ENABLE;

        while((retVal != CY_RTC_SUCCESS) && (0U != tryesToSetup))
        {
            retVal = Cy_RTC_SetAlarmDateAndTime(&dstAlarmTimeAndDate, CY_RTC_ALARM_2);
            --tryesToSetup;
            
            /* Delay after try to set the DST */
            Cy_SysLib_DelayUs(CY_RTC_DELAY_AFTER_DST_US);
        }

        if (tryesToSetup == 0U)
        {
            retVal = CY_RTC_TIMEOUT;
        }
    }

    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_GetDstStatus
****************************************************************************//**
*
* Returns the current DST status using given time information. This function 
* is used in the initial state of a system. If the DST is enabled, the system 
* sets the DST start or stop as a result of this function.
*
* \param dstTime The DST configuration structure, see \ref cy_stc_rtc_dst_t.
*
* \param timeDate
* The time and date structure. The the appropriate DST time is 
* set based on this time and date, see \ref cy_stc_rtc_config_t.
*
* \return
* false - The current date and time is out of the DST period.
* true - The current date and time is in the DST period.
*
*******************************************************************************/
bool Cy_RTC_GetDstStatus(cy_stc_rtc_dst_t const *dstTime, cy_stc_rtc_config_t const *timeDate)
{
    uint32_t dstStartTime;
    uint32_t currentTime;
    uint32_t dstStopTime; 
    uint32_t dstStartDayOfMonth;
    uint32_t dstStopDayOfMonth;

    CY_ASSERT_L1(NULL != dstTime);
    CY_ASSERT_L1(NULL != timeDate);
    
    /* Calculate a day-of-month value for the relative DST start structure */
    if (CY_RTC_DST_RELATIVE != dstTime->startDst.format)
    {
        dstStartDayOfMonth = dstTime->startDst.dayOfMonth;
    }
    else
    {
        dstStartDayOfMonth = RelativeToFixed(&dstTime->startDst);
    }

    /* Calculate the day of a month value for the relative DST stop structure */
    if (CY_RTC_DST_RELATIVE != dstTime->stopDst.format)
    {
        dstStopDayOfMonth = dstTime->stopDst.dayOfMonth;
    }
    else
    {
        dstStopDayOfMonth = RelativeToFixed(&dstTime->stopDst);
    }

    /* The function forms the date and time values for the DST start time, 
    *  the DST Stop Time and for the Current Time. The function that compares 
    *  the three formed values returns "true" under condition that: 
    *  dstStartTime < currentTime < dstStopTime.
    *  The date and time value are formed this way:
    *  [13-10] - Month
    *  [9-5]   - Day of Month
    *  [0-4]   - Hour
    */
    dstStartTime = ((uint32_t) (dstTime->startDst.month << CY_RTC_DST_MONTH_POSITION) |
    (dstStartDayOfMonth << CY_RTC_DST_DAY_OF_MONTH_POSITION) | (dstTime->startDst.hour));

    currentTime = ((uint32_t) (timeDate->month << CY_RTC_DST_MONTH_POSITION) |
    (timeDate->date << CY_RTC_DST_DAY_OF_MONTH_POSITION) | (timeDate->hour));
    
    dstStopTime = ((uint32_t) (dstTime->stopDst.month << CY_RTC_DST_MONTH_POSITION) |
    (dstStopDayOfMonth << CY_RTC_DST_DAY_OF_MONTH_POSITION) | (dstTime->stopDst.hour));

    return((dstStartTime <= currentTime) && (dstStopTime > currentTime));
}


/*******************************************************************************
* Function Name: Cy_RTC_Alarm1Interrupt
****************************************************************************//**
*
* A blank weak interrupt handler function which indicates assert of the RTC 
* alarm 1 interrupt.
* 
* Function implementation should be defined in user source code in condition 
* that such event handler is required. If such event is not required user 
* should not do any actions.
*
* This function is called in the general RTC interrupt handler 
* `$INSTANCE_NAME`_Interrupt() function.
*
*******************************************************************************/
__WEAK void Cy_RTC_Alarm1Interrupt(void)
{
    /* weak blank function */
}


/*******************************************************************************
* Function Name: Cy_RTC_Alarm2Interrupt
****************************************************************************//**
*
* A blank weak interrupt handler function which indicates assert of the RTC 
* alarm 2 interrupt.
* 
* Function implementation should be defined in user source code in condition 
* that such event handler is required. If such event is not required user 
* should not do any actions.
*
* This function is called in the general RTC interrupt handler 
* `$INSTANCE_NAME`_Interrupt() function. Cy_RTC_Alarm2Interrupt() function is 
* ignored in `$INSTANCE_NAME`_Interrupt() function if DST is enabled. Refer to 
* `$INSTANCE_NAME`_Interrupt() description.
*
*******************************************************************************/
__WEAK void Cy_RTC_Alarm2Interrupt(void)
{
    /* weak blank function */
}


/*******************************************************************************
* Function Name: Cy_RTC_DstInterrupt
****************************************************************************//**
* 
* This is a processing handler against the DST event. It adjusts the current 
* time using the DST start/stop parameters and registers the next DST event time
* into the ALARM2 interrupt.
* 
* \param dstTime The DST configuration structure, see \ref cy_stc_rtc_dst_t.
*
*******************************************************************************/
void Cy_RTC_DstInterrupt(cy_stc_rtc_dst_t const *dstTime)
{
    cy_stc_rtc_config_t curDateTime;

    Cy_RTC_GetDateAndTime(&curDateTime);

    if (Cy_RTC_GetDstStatus(dstTime, &curDateTime))
    {
        /* Under condition that the DST start time was selected as 23:00, and 
        *  the time adjusting occurs, the other time and date values should be 
        *  corrected (day of the week, date, month and year).
        */
        if (curDateTime.hour > CY_RTC_MAX_HOURS_24H)
        {
            /* Incrementing day of the week value as hour adjusted next day of 
            *  the week and date. Correcting hour value as its incrementation 
            *  adjusted it out of valid range [0-23].
            */
            curDateTime.dayOfWeek++;
            curDateTime.hour = 0U;

            /* Correct a day of the week if its incrementation adjusted it out 
            *  of valid range [1-7].
            */
            if (curDateTime.dayOfWeek > CY_RTC_SATURDAY)
            {
                curDateTime.dayOfWeek = CY_RTC_SUNDAY;
            }
            
            curDateTime.date++;

            /* Correct a day of a month if its incrementation adjusted it out of
            *  the valid range [1-31]. Increment month value.
            */
            if (curDateTime.date > Cy_RTC_DaysInMonth(curDateTime.month, 
                                                    (curDateTime.year + CY_RTC_TWO_THOUSAND_YEARS)))
            {
               curDateTime.date = CY_RTC_FIRST_DAY_OF_MONTH;
               curDateTime.month++;
            }

            /* Correct a month if its incrementation adjusted it out of the 
            *  valid range [1-12]. Increment year value.
            */
            if (curDateTime.month > CY_RTC_MONTHS_PER_YEAR)
            {
                curDateTime.month = CY_RTC_JANUARY;
                curDateTime.year++;
            }
        }
        else
        {
            curDateTime.hour++;
        }
        
        (void) Cy_RTC_SetDateAndTime(&curDateTime);
        (void) Cy_RTC_SetNextDstTime(&dstTime->stopDst);
    }
    else
    {
        if (curDateTime.hour < 1U)
        {
            /* Decrementing day of the week time and date values as hour 
            *  adjusted next day of the week and date. Correct hour value as 
            *  its incrementation adjusted it out of valid range [0-23]. 
            */
            curDateTime.hour = CY_RTC_MAX_HOURS_24H;
            curDateTime.dayOfWeek--;

            /* Correct a day of the week if its incrementation adjusted it out 
            *  of the valid range [1-7].
            */
            if (curDateTime.dayOfWeek < CY_RTC_SUNDAY)
            {
                curDateTime.dayOfWeek = CY_RTC_SUNDAY;
            }

            curDateTime.date--;

            /* Correct a day of a month value if its incrementation adjusted it
            *  out of the valid range [1-31]. Decrement month value.
            */
            if (curDateTime.date < CY_RTC_FIRST_DAY_OF_MONTH)
            {
               curDateTime.date = 
               Cy_RTC_DaysInMonth(curDateTime.month, (curDateTime.year + CY_RTC_TWO_THOUSAND_YEARS));
               curDateTime.month--;
            }

            /* Correct a month if its increment pushed it out of the valid 
            *  range [1-12]. Decrement year value.
            */
            if (curDateTime.month < CY_RTC_JANUARY)
            {
                curDateTime.month = CY_RTC_DECEMBER;
                curDateTime.year--;
            }
        }
        else
        {
            curDateTime.hour--;
        }
        
        (void) Cy_RTC_SetDateAndTime(&curDateTime);
        (void) Cy_RTC_SetNextDstTime(&dstTime->startDst);
    }
}


/*******************************************************************************
* Function Name: Cy_RTC_CenturyInterrupt
****************************************************************************//**
*
* This is a weak function and it should be redefined in user source code
* in condition that such event handler is required.
* By calling this function, it indicates the year reached 2100. It 
* should add an adjustment to avoid the Y2K problem.
*
* Function implementation should be defined in user source code in condition 
* that such event handler is required. If such event is not required user 
* should not do any actions.
*
*******************************************************************************/
__WEAK void Cy_RTC_CenturyInterrupt(void)
{
    /* weak blank function */
}


/*******************************************************************************
* Function Name: Cy_RTC_GetInterruptStatus
****************************************************************************//**
*
* Returns a status of RTC interrupt requests.
*
* \return
* Bit mapping information, see \ref group_rtc_macros_interrupts.
*
*******************************************************************************/
uint32_t Cy_RTC_GetInterruptStatus(void)
{
    return(BACKUP_INTR);
}


/*******************************************************************************
* Function Name: Cy_RTC_GetInterruptStatusMasked
****************************************************************************//**
*
* Returns an interrupt request register masked by the interrupt mask. Returns a 
* result of the bitwise AND operation between the corresponding interrupt 
* request and mask bits.
*
* \return
* Bit mapping information, see \ref group_rtc_macros_interrupts.
*
*******************************************************************************/
uint32_t Cy_RTC_GetInterruptStatusMasked(void)
{
    return(BACKUP_INTR_MASKED);
}


/*******************************************************************************
* Function Name: Cy_RTC_GetInterruptMask
****************************************************************************//**
*
* Returns an interrupt mask.
*
* \return
* Bit mapping information, see \ref group_rtc_macros_interrupts.
*
*******************************************************************************/
uint32_t Cy_RTC_GetInterruptMask(void)
{
    return (BACKUP_INTR_MASK);
}


/*******************************************************************************
* Function Name: Cy_RTC_SetInterrupt
****************************************************************************//**
*
* Sets a software interrupt request
*
* \param interruptMask Bit mask, see \ref group_rtc_macros_interrupts.
*
*******************************************************************************/
void Cy_RTC_SetInterrupt(uint32_t interruptMask)
{
    CY_ASSERT_L3(CY_RTC_INTR_VALID(interruptMask));

    BACKUP_INTR_SET = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_RTC_ClearInterrupt
****************************************************************************//**
*
* Clears RTC interrupts by setting each bit. 
*
* \param
* interruptMask The bit mask of interrupts to set,
* see \ref group_rtc_macros_interrupts.
*
*******************************************************************************/
void Cy_RTC_ClearInterrupt(uint32_t interruptMask)
{
    CY_ASSERT_L3(CY_RTC_INTR_VALID(interruptMask));

    BACKUP_INTR = interruptMask;

    (void) BACKUP_INTR;
}


/*******************************************************************************
* Function Name: Cy_RTC_SetInterruptMask
****************************************************************************//**
*
* Configures which bits of the interrupt request register that triggers an 
* interrupt event.
*
* \param interruptMask
* The bit mask of interrupts to set, see \ref group_rtc_macros_interrupts.
*
*******************************************************************************/
void Cy_RTC_SetInterruptMask(uint32_t interruptMask)
{
    CY_ASSERT_L3(CY_RTC_INTR_VALID(interruptMask));

    BACKUP_INTR_MASK = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_RTC_Interrupt
****************************************************************************//**
*
* The interrupt handler function which should be called in user provided
* RTC interrupt function.
*
* This is the handler of the RTC interrupt in CPU NVIC. The handler checks 
* which RTC interrupt was asserted and calls the respective RTC interrupt 
* handler functions: Cy_RTC_Alarm1Interrupt(), Cy_RTC_Alarm2Interrupt() or 
* Cy_RTC_DstInterrupt(), and Cy_RTC_CenturyInterrupt().
* 
* The order of the RTC handler functions execution is incremental. 
* Cy_RTC_Alarm1Interrupt() is run as the first one and Cy_RTC_CenturyInterrupt()
* is called as the last one.
*
* This function clears the RTC interrupt every time when it is called.
*
* Cy_RTC_DstInterrupt() function is called instead of Cy_RTC_Alarm2Interrupt() 
* in condition that the mode parameter is true.
*
* \param dstTime
* The daylight saving time configuration structure, see \ref cy_stc_rtc_dst_t.
*
* \param mode false - if the DST is disabled, true - if DST is enabled.
*
* \note This function is required to be called in user interrupt handler.
*
*******************************************************************************/
void Cy_RTC_Interrupt(cy_stc_rtc_dst_t const *dstTime, bool mode)
{
    uint32_t interruptStatus;
    interruptStatus = Cy_RTC_GetInterruptStatusMasked();

    Cy_RTC_ClearInterrupt(interruptStatus);

    if (0U != (CY_RTC_INTR_ALARM1 & interruptStatus))
    {
        Cy_RTC_Alarm1Interrupt();
    }

    if (0U != (CY_RTC_INTR_ALARM2 & interruptStatus))
    {
        if (mode)
        {
            Cy_RTC_DstInterrupt(dstTime);
        }
        else
        {
            Cy_RTC_Alarm2Interrupt();
        }
    }

    if (0U != (CY_RTC_INTR_CENTURY & interruptStatus))
    {
        Cy_RTC_CenturyInterrupt();
    }
}


/*******************************************************************************
* Function Name: Cy_RTC_DeepSleepCallback
****************************************************************************//**
*
* This function checks the RTC_BUSY bit to avoid data corruption before 
* enters the deep sleep mode.
*
* \param callbackParams
* structure with the syspm callback parameters, 
* see \ref cy_stc_syspm_callback_params_t
*
* \return
* syspm return status, see \ref cy_en_syspm_status_t
*
* \note The *base and *context elements are required to be present in 
* the parameter structure because this function uses the SysPm driver 
* callback type.
* The SysPm driver callback function type requires implementing the function 
* with next parameters and return value: <br>
* cy_en_syspm_status_t (*Cy_SysPmCallback) 
* (cy_stc_syspm_callback_params_t *callbackParams);
*
*******************************************************************************/
cy_en_syspm_status_t Cy_RTC_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    cy_en_syspm_status_t retVal = CY_SYSPM_FAIL;

    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            if (CY_RTC_AVAILABLE == Cy_RTC_GetSyncStatus())
            {
                retVal = CY_SYSPM_SUCCESS;
            }
        }
        break;

        case CY_SYSPM_CHECK_FAIL:
        {
            retVal = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_BEFORE_TRANSITION:
        {
            retVal = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_AFTER_TRANSITION:
        {
            retVal = CY_SYSPM_SUCCESS;
        }
        break;

        default:
            break;
    }

    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_RTC_HibernateCallback
****************************************************************************//**
*
* This function checks the RTC_BUSY bit to avoid data corruption before 
* enters the hibernate mode.
*
* \param callbackParams
* structure with the syspm callback parameters, 
* see \ref cy_stc_syspm_callback_params_t.
*
* \return
* syspm return status, see \ref cy_en_syspm_status_t
*
* \note The *base and *context elements are required to be present in 
* the parameter structure because this function uses the SysPm driver 
* callback type.
* The SysPm driver callback function type requires implementing the function 
* with next parameters and return value: <br>
* cy_en_syspm_status_t (*Cy_SysPmCallback) 
* (cy_stc_syspm_callback_params_t *callbackParams);
*
*******************************************************************************/
cy_en_syspm_status_t Cy_RTC_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    return (Cy_RTC_DeepSleepCallback(callbackParams));
}


/*******************************************************************************
* Function Name: ConstructTimeDate
****************************************************************************//**
*
* Returns BCD time and BCD date in the format used in APIs from individual 
* elements passed.
* Converted BCD time(*timeBcd) and BCD date(*dateBcd) are matched with RTC_TIME 
* and RTC_DATE bit fields format.
*
* \param timeDate 
* The structure of time and date, see \ref cy_stc_rtc_config_t.
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
*******************************************************************************/
static void ConstructTimeDate(cy_stc_rtc_config_t const *timeDate, uint32_t *timeBcd, uint32_t *dateBcd)
{
    uint32_t tmpTime;
    uint32_t tmpDate;

    /* Prepare the RTC TIME value based on the structure obtained */
    tmpTime = (_VAL2FLD(BACKUP_RTC_TIME_RTC_SEC, Cy_RTC_ConvertDecToBcd(timeDate->sec)));
    tmpTime |= (_VAL2FLD(BACKUP_RTC_TIME_RTC_MIN, Cy_RTC_ConvertDecToBcd(timeDate->min)));

    /* Read the current hour mode to know how many hour bits to convert.
    *  In the 24-hour mode, the hour value is presented in [21:16] bits in the 
    *  BCD format.
    *  In the 12-hour mode, the hour value is presented in [20:16] bits in the 
    *  BCD format and
    *  bit [21] is present: 0 - AM; 1 - PM.
    */
    if (timeDate->hrFormat != CY_RTC_24_HOURS)
    {
        if (CY_RTC_AM != timeDate->amPm)
        {
            /* Set the PM bit */
            tmpTime |= CY_RTC_BACKUP_RTC_TIME_RTC_PM;
        }
        else
        {
            /* Set the AM bit */
            tmpTime &= ((uint32_t) ~CY_RTC_BACKUP_RTC_TIME_RTC_PM); 
        }
        tmpTime |= BACKUP_RTC_TIME_CTRL_12HR_Msk;
        tmpTime |= 
        (_VAL2FLD(BACKUP_RTC_TIME_RTC_HOUR, 
        (Cy_RTC_ConvertDecToBcd(timeDate->hour) & ((uint32_t) ~CY_RTC_12HRS_PM_BIT))));
    }
    else
    {
        tmpTime &= ((uint32_t) ~BACKUP_RTC_TIME_CTRL_12HR_Msk);
        tmpTime |= (_VAL2FLD(BACKUP_RTC_TIME_RTC_HOUR, Cy_RTC_ConvertDecToBcd(timeDate->hour)));
    }
    tmpTime |= (_VAL2FLD(BACKUP_RTC_TIME_RTC_DAY, Cy_RTC_ConvertDecToBcd(timeDate->dayOfWeek)));

    /* Prepare the RTC Date value based on the structure obtained */
    tmpDate  = (_VAL2FLD(BACKUP_RTC_DATE_RTC_DATE, Cy_RTC_ConvertDecToBcd(timeDate->date)));
    tmpDate |= (_VAL2FLD(BACKUP_RTC_DATE_RTC_MON, Cy_RTC_ConvertDecToBcd(timeDate->month)));
    tmpDate |= (_VAL2FLD(BACKUP_RTC_DATE_RTC_YEAR, Cy_RTC_ConvertDecToBcd(timeDate->year)));

    /* Update the parameter values with prepared values */
    *timeBcd = tmpTime;
    *dateBcd = tmpDate;
}


/*******************************************************************************
* Function Name: ConstructAlarmTimeDate
****************************************************************************//**
*
* Returns the BCD time and BCD date in the format used in APIs from individual 
* elements passed for alarm.
* Converted BCD time(*alarmTimeBcd) and BCD date(*alarmDateBcd) should be 
* matched with the ALMx_TIME and ALMx_DATE bit fields format.
*
* \param timeDate
* The structure of time and date, see \ref cy_stc_rtc_alarm_t.
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
* This function reads current AHB register RTC_TIME value to know hour mode.
* It is recommended to call Cy_RTC_SyncFromRtc() function before calling the 
* ConstructAlarmTimeDate() functions.
* 
* Construction is based on RTC_ALARM1 register bit fields.
*
*******************************************************************************/
static void ConstructAlarmTimeDate(cy_stc_rtc_alarm_t const *alarmDateTime, uint32_t *alarmTimeBcd,
                                                                                   uint32_t *alarmDateBcd)
{
    uint32_t tmpAlarmTime;
    uint32_t tmpAlarmDate;
    uint32_t hourValue;

    /* Prepare the RTC ALARM value based on the structure obtained */
    tmpAlarmTime  = (_VAL2FLD(BACKUP_ALM1_TIME_ALM_SEC, Cy_RTC_ConvertDecToBcd(alarmDateTime->sec)));
    tmpAlarmTime |= (_VAL2FLD(BACKUP_ALM1_TIME_ALM_SEC_EN, alarmDateTime->secEn));
    tmpAlarmTime |= (_VAL2FLD(BACKUP_ALM1_TIME_ALM_MIN, Cy_RTC_ConvertDecToBcd(alarmDateTime->min)));
    tmpAlarmTime |= (_VAL2FLD(BACKUP_ALM1_TIME_ALM_MIN_EN, alarmDateTime->minEn));

    /* Read the current hour mode to know how many hour bits to convert.
    *  In the 24-hour mode, the hour value is presented in [21:16] bits in the 
    *  BCD format.
    *  In the 12-hour mode, the hour value is presented in [20:16] bits in the 
    *  BCD format and bit [21] is present: 0 - AM; 1 - PM
    */
    Cy_RTC_SyncFromRtc();
    if (CY_RTC_24_HOURS != Cy_RTC_GetHoursFormat())
    {
        /* Convert the hour from the 24-hour mode into the 12-hour mode */
        if (alarmDateTime->hour >= CY_RTC_HOURS_PER_HALF_DAY)
        {
            /* The current hour is more than 12 in the 24-hour mode. Set the PM 
            *  bit and converting hour: hour = hour - 12
            */
            hourValue = (uint32_t) alarmDateTime->hour - CY_RTC_HOURS_PER_HALF_DAY;
            hourValue = ((0U != hourValue) ? hourValue : CY_RTC_HOURS_PER_HALF_DAY);
            tmpAlarmTime |= 
            CY_RTC_BACKUP_RTC_TIME_RTC_PM | (_VAL2FLD(BACKUP_ALM1_TIME_ALM_HOUR, Cy_RTC_ConvertDecToBcd(hourValue)));
        }
        else if (alarmDateTime->hour < 1U)
        {
            /* The current hour in the 24-hour mode is 0 which is equal to 12:00 AM */
            tmpAlarmTime = (tmpAlarmTime & ((uint32_t) ~CY_RTC_BACKUP_RTC_TIME_RTC_PM)) | 
            (_VAL2FLD(BACKUP_ALM1_TIME_ALM_HOUR, CY_RTC_HOURS_PER_HALF_DAY));
        }
        else
        {
            /* The current hour is less than 12. Set the AM bit */
            tmpAlarmTime = (tmpAlarmTime & ((uint32_t) ~CY_RTC_BACKUP_RTC_TIME_RTC_PM)) |
            (_VAL2FLD(BACKUP_ALM1_TIME_ALM_HOUR, Cy_RTC_ConvertDecToBcd(alarmDateTime->hour)));
        }
        tmpAlarmTime |= BACKUP_RTC_TIME_CTRL_12HR_Msk;
    }
    else
    {
        tmpAlarmTime |= (_VAL2FLD(BACKUP_ALM1_TIME_ALM_HOUR, Cy_RTC_ConvertDecToBcd(alarmDateTime->hour)));
        tmpAlarmTime &= ((uint32_t) ~BACKUP_RTC_TIME_CTRL_12HR_Msk);
    }
    tmpAlarmTime |= (_VAL2FLD(BACKUP_ALM1_TIME_ALM_HOUR_EN, alarmDateTime->hourEn));
    tmpAlarmTime |= (_VAL2FLD(BACKUP_ALM1_TIME_ALM_DAY, Cy_RTC_ConvertDecToBcd(alarmDateTime->dayOfWeek)));
    tmpAlarmTime |= (_VAL2FLD(BACKUP_ALM1_TIME_ALM_DAY_EN, alarmDateTime->dayOfWeekEn));

    /* Prepare the RTC ALARM DATE value based on the obtained structure */
    tmpAlarmDate  = (_VAL2FLD(BACKUP_ALM1_DATE_ALM_DATE, Cy_RTC_ConvertDecToBcd(alarmDateTime->date)));
    tmpAlarmDate |= (_VAL2FLD(BACKUP_ALM1_DATE_ALM_DATE_EN, alarmDateTime->dateEn));
    tmpAlarmDate |= (_VAL2FLD(BACKUP_ALM1_DATE_ALM_MON, Cy_RTC_ConvertDecToBcd(alarmDateTime->month)));
    tmpAlarmDate |= (_VAL2FLD(BACKUP_ALM1_DATE_ALM_MON_EN, alarmDateTime->monthEn));
    tmpAlarmDate |= (_VAL2FLD(BACKUP_ALM1_DATE_ALM_EN, alarmDateTime->almEn));

    /* Update the parameter values with prepared values */
    *alarmTimeBcd = tmpAlarmTime;
    *alarmDateBcd = tmpAlarmDate;
}


/*******************************************************************************
* Function Name: RelativeToFixed
****************************************************************************//**
*
* Converts time from a relative format to a fixed format to set the ALARM2.
*
* \param convertDst
* The DST structure, its appropriate elements should be converted.
*
* \return
* The current date of a month.
*
*******************************************************************************/
static uint32_t RelativeToFixed(cy_stc_rtc_dst_format_t const *convertDst)
{
    uint32_t currentYear;
    uint32_t currentDay;
    uint32_t currentWeek;
    uint32_t daysInMonth;
    uint32_t tmpDayOfMonth;

    /* Read the current year */
    Cy_RTC_SyncFromRtc();

    currentYear = 
    CY_RTC_TWO_THOUSAND_YEARS + Cy_RTC_ConvertBcdToDec(_FLD2VAL(BACKUP_RTC_DATE_RTC_YEAR, BACKUP_RTC_DATE));

    currentDay  = CY_RTC_FIRST_DAY_OF_MONTH;
    currentWeek = CY_RTC_FIRST_WEEK_OF_MONTH;
    daysInMonth = Cy_RTC_DaysInMonth(convertDst->month, currentYear);
    tmpDayOfMonth  = currentDay;

    while((currentWeek <= convertDst->weekOfMonth) && (currentDay <= daysInMonth))
    {
        if (convertDst->dayOfWeek == Cy_RTC_ConvertDayOfWeek(currentDay, convertDst->month, currentYear))
        {
            tmpDayOfMonth = currentDay;
            currentWeek++;
        }
        currentDay++;
    }
    return(tmpDayOfMonth);
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40SRSS_RTC */

/* [] END OF FILE */
