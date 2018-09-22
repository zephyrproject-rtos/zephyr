/***************************************************************************//**
* \file cyip_backup.h
*
* \brief
* BACKUP IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_BACKUP_H_
#define _CYIP_BACKUP_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                    BACKUP
*******************************************************************************/

#define BACKUP_SECTION_SIZE                     0x00010000UL

/**
  * \brief SRSS Backup Domain (BACKUP)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t RESERVED;
  __IOM uint32_t RTC_RW;                        /*!< 0x00000008 RTC Read Write register */
  __IOM uint32_t CAL_CTL;                       /*!< 0x0000000C Oscillator calibration for absolute frequency */
   __IM uint32_t STATUS;                        /*!< 0x00000010 Status */
  __IOM uint32_t RTC_TIME;                      /*!< 0x00000014 Calendar Seconds, Minutes, Hours, Day of Week */
  __IOM uint32_t RTC_DATE;                      /*!< 0x00000018 Calendar Day of Month, Month,  Year */
  __IOM uint32_t ALM1_TIME;                     /*!< 0x0000001C Alarm 1 Seconds, Minute, Hours, Day of Week */
  __IOM uint32_t ALM1_DATE;                     /*!< 0x00000020 Alarm 1 Day of Month, Month */
  __IOM uint32_t ALM2_TIME;                     /*!< 0x00000024 Alarm 2 Seconds, Minute, Hours, Day of Week */
  __IOM uint32_t ALM2_DATE;                     /*!< 0x00000028 Alarm 2 Day of Month, Month */
  __IOM uint32_t INTR;                          /*!< 0x0000002C Interrupt request register */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000030 Interrupt set request register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000034 Interrupt mask register */
   __IM uint32_t INTR_MASKED;                   /*!< 0x00000038 Interrupt masked request register */
   __IM uint32_t OSCCNT;                        /*!< 0x0000003C 32kHz oscillator counter */
   __IM uint32_t TICKS;                         /*!< 0x00000040 128Hz tick counter */
  __IOM uint32_t PMIC_CTL;                      /*!< 0x00000044 PMIC control register */
  __IOM uint32_t RESET;                         /*!< 0x00000048 Backup reset register */
   __IM uint32_t RESERVED1[1005];
  __IOM uint32_t BREG[64];                      /*!< 0x00001000 Backup register region */
   __IM uint32_t RESERVED2[15232];
  __IOM uint32_t TRIM;                          /*!< 0x0000FF00 Trim Register */
} BACKUP_V1_Type;                               /*!< Size = 65284 (0xFF04) */


/* BACKUP.CTL */
#define BACKUP_CTL_WCO_EN_Pos                   3UL
#define BACKUP_CTL_WCO_EN_Msk                   0x8UL
#define BACKUP_CTL_CLK_SEL_Pos                  8UL
#define BACKUP_CTL_CLK_SEL_Msk                  0x300UL
#define BACKUP_CTL_PRESCALER_Pos                12UL
#define BACKUP_CTL_PRESCALER_Msk                0x3000UL
#define BACKUP_CTL_WCO_BYPASS_Pos               16UL
#define BACKUP_CTL_WCO_BYPASS_Msk               0x10000UL
#define BACKUP_CTL_VDDBAK_CTL_Pos               17UL
#define BACKUP_CTL_VDDBAK_CTL_Msk               0x60000UL
#define BACKUP_CTL_VBACKUP_MEAS_Pos             19UL
#define BACKUP_CTL_VBACKUP_MEAS_Msk             0x80000UL
#define BACKUP_CTL_EN_CHARGE_KEY_Pos            24UL
#define BACKUP_CTL_EN_CHARGE_KEY_Msk            0xFF000000UL
/* BACKUP.RTC_RW */
#define BACKUP_RTC_RW_READ_Pos                  0UL
#define BACKUP_RTC_RW_READ_Msk                  0x1UL
#define BACKUP_RTC_RW_WRITE_Pos                 1UL
#define BACKUP_RTC_RW_WRITE_Msk                 0x2UL
/* BACKUP.CAL_CTL */
#define BACKUP_CAL_CTL_CALIB_VAL_Pos            0UL
#define BACKUP_CAL_CTL_CALIB_VAL_Msk            0x3FUL
#define BACKUP_CAL_CTL_CALIB_SIGN_Pos           6UL
#define BACKUP_CAL_CTL_CALIB_SIGN_Msk           0x40UL
#define BACKUP_CAL_CTL_CAL_OUT_Pos              31UL
#define BACKUP_CAL_CTL_CAL_OUT_Msk              0x80000000UL
/* BACKUP.STATUS */
#define BACKUP_STATUS_RTC_BUSY_Pos              0UL
#define BACKUP_STATUS_RTC_BUSY_Msk              0x1UL
#define BACKUP_STATUS_WCO_OK_Pos                2UL
#define BACKUP_STATUS_WCO_OK_Msk                0x4UL
/* BACKUP.RTC_TIME */
#define BACKUP_RTC_TIME_RTC_SEC_Pos             0UL
#define BACKUP_RTC_TIME_RTC_SEC_Msk             0x7FUL
#define BACKUP_RTC_TIME_RTC_MIN_Pos             8UL
#define BACKUP_RTC_TIME_RTC_MIN_Msk             0x7F00UL
#define BACKUP_RTC_TIME_RTC_HOUR_Pos            16UL
#define BACKUP_RTC_TIME_RTC_HOUR_Msk            0x3F0000UL
#define BACKUP_RTC_TIME_CTRL_12HR_Pos           22UL
#define BACKUP_RTC_TIME_CTRL_12HR_Msk           0x400000UL
#define BACKUP_RTC_TIME_RTC_DAY_Pos             24UL
#define BACKUP_RTC_TIME_RTC_DAY_Msk             0x7000000UL
/* BACKUP.RTC_DATE */
#define BACKUP_RTC_DATE_RTC_DATE_Pos            0UL
#define BACKUP_RTC_DATE_RTC_DATE_Msk            0x3FUL
#define BACKUP_RTC_DATE_RTC_MON_Pos             8UL
#define BACKUP_RTC_DATE_RTC_MON_Msk             0x1F00UL
#define BACKUP_RTC_DATE_RTC_YEAR_Pos            16UL
#define BACKUP_RTC_DATE_RTC_YEAR_Msk            0xFF0000UL
/* BACKUP.ALM1_TIME */
#define BACKUP_ALM1_TIME_ALM_SEC_Pos            0UL
#define BACKUP_ALM1_TIME_ALM_SEC_Msk            0x7FUL
#define BACKUP_ALM1_TIME_ALM_SEC_EN_Pos         7UL
#define BACKUP_ALM1_TIME_ALM_SEC_EN_Msk         0x80UL
#define BACKUP_ALM1_TIME_ALM_MIN_Pos            8UL
#define BACKUP_ALM1_TIME_ALM_MIN_Msk            0x7F00UL
#define BACKUP_ALM1_TIME_ALM_MIN_EN_Pos         15UL
#define BACKUP_ALM1_TIME_ALM_MIN_EN_Msk         0x8000UL
#define BACKUP_ALM1_TIME_ALM_HOUR_Pos           16UL
#define BACKUP_ALM1_TIME_ALM_HOUR_Msk           0x3F0000UL
#define BACKUP_ALM1_TIME_ALM_HOUR_EN_Pos        23UL
#define BACKUP_ALM1_TIME_ALM_HOUR_EN_Msk        0x800000UL
#define BACKUP_ALM1_TIME_ALM_DAY_Pos            24UL
#define BACKUP_ALM1_TIME_ALM_DAY_Msk            0x7000000UL
#define BACKUP_ALM1_TIME_ALM_DAY_EN_Pos         31UL
#define BACKUP_ALM1_TIME_ALM_DAY_EN_Msk         0x80000000UL
/* BACKUP.ALM1_DATE */
#define BACKUP_ALM1_DATE_ALM_DATE_Pos           0UL
#define BACKUP_ALM1_DATE_ALM_DATE_Msk           0x3FUL
#define BACKUP_ALM1_DATE_ALM_DATE_EN_Pos        7UL
#define BACKUP_ALM1_DATE_ALM_DATE_EN_Msk        0x80UL
#define BACKUP_ALM1_DATE_ALM_MON_Pos            8UL
#define BACKUP_ALM1_DATE_ALM_MON_Msk            0x1F00UL
#define BACKUP_ALM1_DATE_ALM_MON_EN_Pos         15UL
#define BACKUP_ALM1_DATE_ALM_MON_EN_Msk         0x8000UL
#define BACKUP_ALM1_DATE_ALM_EN_Pos             31UL
#define BACKUP_ALM1_DATE_ALM_EN_Msk             0x80000000UL
/* BACKUP.ALM2_TIME */
#define BACKUP_ALM2_TIME_ALM_SEC_Pos            0UL
#define BACKUP_ALM2_TIME_ALM_SEC_Msk            0x7FUL
#define BACKUP_ALM2_TIME_ALM_SEC_EN_Pos         7UL
#define BACKUP_ALM2_TIME_ALM_SEC_EN_Msk         0x80UL
#define BACKUP_ALM2_TIME_ALM_MIN_Pos            8UL
#define BACKUP_ALM2_TIME_ALM_MIN_Msk            0x7F00UL
#define BACKUP_ALM2_TIME_ALM_MIN_EN_Pos         15UL
#define BACKUP_ALM2_TIME_ALM_MIN_EN_Msk         0x8000UL
#define BACKUP_ALM2_TIME_ALM_HOUR_Pos           16UL
#define BACKUP_ALM2_TIME_ALM_HOUR_Msk           0x3F0000UL
#define BACKUP_ALM2_TIME_ALM_HOUR_EN_Pos        23UL
#define BACKUP_ALM2_TIME_ALM_HOUR_EN_Msk        0x800000UL
#define BACKUP_ALM2_TIME_ALM_DAY_Pos            24UL
#define BACKUP_ALM2_TIME_ALM_DAY_Msk            0x7000000UL
#define BACKUP_ALM2_TIME_ALM_DAY_EN_Pos         31UL
#define BACKUP_ALM2_TIME_ALM_DAY_EN_Msk         0x80000000UL
/* BACKUP.ALM2_DATE */
#define BACKUP_ALM2_DATE_ALM_DATE_Pos           0UL
#define BACKUP_ALM2_DATE_ALM_DATE_Msk           0x3FUL
#define BACKUP_ALM2_DATE_ALM_DATE_EN_Pos        7UL
#define BACKUP_ALM2_DATE_ALM_DATE_EN_Msk        0x80UL
#define BACKUP_ALM2_DATE_ALM_MON_Pos            8UL
#define BACKUP_ALM2_DATE_ALM_MON_Msk            0x1F00UL
#define BACKUP_ALM2_DATE_ALM_MON_EN_Pos         15UL
#define BACKUP_ALM2_DATE_ALM_MON_EN_Msk         0x8000UL
#define BACKUP_ALM2_DATE_ALM_EN_Pos             31UL
#define BACKUP_ALM2_DATE_ALM_EN_Msk             0x80000000UL
/* BACKUP.INTR */
#define BACKUP_INTR_ALARM1_Pos                  0UL
#define BACKUP_INTR_ALARM1_Msk                  0x1UL
#define BACKUP_INTR_ALARM2_Pos                  1UL
#define BACKUP_INTR_ALARM2_Msk                  0x2UL
#define BACKUP_INTR_CENTURY_Pos                 2UL
#define BACKUP_INTR_CENTURY_Msk                 0x4UL
/* BACKUP.INTR_SET */
#define BACKUP_INTR_SET_ALARM1_Pos              0UL
#define BACKUP_INTR_SET_ALARM1_Msk              0x1UL
#define BACKUP_INTR_SET_ALARM2_Pos              1UL
#define BACKUP_INTR_SET_ALARM2_Msk              0x2UL
#define BACKUP_INTR_SET_CENTURY_Pos             2UL
#define BACKUP_INTR_SET_CENTURY_Msk             0x4UL
/* BACKUP.INTR_MASK */
#define BACKUP_INTR_MASK_ALARM1_Pos             0UL
#define BACKUP_INTR_MASK_ALARM1_Msk             0x1UL
#define BACKUP_INTR_MASK_ALARM2_Pos             1UL
#define BACKUP_INTR_MASK_ALARM2_Msk             0x2UL
#define BACKUP_INTR_MASK_CENTURY_Pos            2UL
#define BACKUP_INTR_MASK_CENTURY_Msk            0x4UL
/* BACKUP.INTR_MASKED */
#define BACKUP_INTR_MASKED_ALARM1_Pos           0UL
#define BACKUP_INTR_MASKED_ALARM1_Msk           0x1UL
#define BACKUP_INTR_MASKED_ALARM2_Pos           1UL
#define BACKUP_INTR_MASKED_ALARM2_Msk           0x2UL
#define BACKUP_INTR_MASKED_CENTURY_Pos          2UL
#define BACKUP_INTR_MASKED_CENTURY_Msk          0x4UL
/* BACKUP.OSCCNT */
#define BACKUP_OSCCNT_CNT32KHZ_Pos              0UL
#define BACKUP_OSCCNT_CNT32KHZ_Msk              0xFFUL
/* BACKUP.TICKS */
#define BACKUP_TICKS_CNT128HZ_Pos               0UL
#define BACKUP_TICKS_CNT128HZ_Msk               0x3FUL
/* BACKUP.PMIC_CTL */
#define BACKUP_PMIC_CTL_UNLOCK_Pos              8UL
#define BACKUP_PMIC_CTL_UNLOCK_Msk              0xFF00UL
#define BACKUP_PMIC_CTL_POLARITY_Pos            16UL
#define BACKUP_PMIC_CTL_POLARITY_Msk            0x10000UL
#define BACKUP_PMIC_CTL_PMIC_EN_OUTEN_Pos       29UL
#define BACKUP_PMIC_CTL_PMIC_EN_OUTEN_Msk       0x20000000UL
#define BACKUP_PMIC_CTL_PMIC_ALWAYSEN_Pos       30UL
#define BACKUP_PMIC_CTL_PMIC_ALWAYSEN_Msk       0x40000000UL
#define BACKUP_PMIC_CTL_PMIC_EN_Pos             31UL
#define BACKUP_PMIC_CTL_PMIC_EN_Msk             0x80000000UL
/* BACKUP.RESET */
#define BACKUP_RESET_RESET_Pos                  31UL
#define BACKUP_RESET_RESET_Msk                  0x80000000UL
/* BACKUP.BREG */
#define BACKUP_BREG_BREG_Pos                    0UL
#define BACKUP_BREG_BREG_Msk                    0xFFFFFFFFUL
/* BACKUP.TRIM */
#define BACKUP_TRIM_TRIM_Pos                    0UL
#define BACKUP_TRIM_TRIM_Msk                    0x3FUL


#endif /* _CYIP_BACKUP_H_ */


/* [] END OF FILE */
