/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file timer.h
 *MEC1501 Timer definitions
 */
/** @defgroup MEC1501 Peripherals Timers
 */

#ifndef _TIMER_H
#define _TIMER_H


/*
 * Basic timers base address
 * Offset between each timer block
 */
#define B16TMR_BASE               0x40000C00
#define B16TMR_MAX_INSTANCE       2
#define B32TMR_BASE               0x40000C80
#define B32TMR_MAX_INSTANCE       2

/*
 * Offset between instances of the TMR blocks
 */
#define BTMR_INSTANCE_POS               5ul
#define BTMR_INSTANCE_OFS               (1ul << (BTMR_INSTANCE_POS))

/*
 * Basic Timer Count Register (Offset +00h)
 * 32-bit R/W
 * 16-bit Basic timers: bits[15:0]=R/W, bits[31:15]=RO=0
 */
#define BTMR_CNT_OFS                0x00

/*
 * Basic Timer Preload Register (Offset +04h)
 * 32-bit R/W
 * 16-bit Basic timers: bits[15:0]=R/W, bits[31:15]=RO=0
 */
#define BTMR_PRELOAD_OFS            0x04


/*
 * Basic Timer Status Register (Offset +08h)
 * R/W1C
 */
#define BTMR_STS_OFS                0x08
#define BTMR_STS_MASK               0x01
#define BTMR_STS_ACTIVE_POS         0
#define BTMR_STS_ACTIVE             0x01ul

/*
 * Basic Timer Interrupt Enable Register (Offset +0Ch)
 */
#define BTMR_INTEN_OFS              0x0C
#define BTMR_INTEN_MASK             0x01ul
#define BTMR_INTEN_POS              0
#define BTMR_INTEN                  0x01ul
#define BTMR_INTDIS                 0ul

/*
 * Basic Timer Control Register (Offset +10h)
 */
#define BTMR_CTRL_OFS                   0x10ul
#define BTMR_CTRL_MASK                  0xFFFF00FDul

#define BTMR_CTRL_PRESCALE_POS          16u
#define BTMR_CTRL_PRESCALE_MASK0        0xFFFFul
#define BTMR_CTRL_PRESCALE_MASK         0xFFFF0000ul

#define BTMR_CTRL_HALT                   0x80ul
#define BTMR_CTRL_RELOAD                 0x40ul
#define BTMR_CTRL_START                  0x20ul
#define BTMR_CTRL_SOFT_RESET             0x10ul
#define BTMR_CTRL_AUTO_RESTART           0x08ul
#define BTMR_CTRL_COUNT_UP               0x04ul
#define BTMR_CTRL_ENABLE                 0x01ul
/* */
#define BTMR_CTRL_HALT_POS              7
#define BTMR_CTRL_RELOAD_POS            6
#define BTMR_CTRL_START_POS             5
#define BTMR_CTRL_SRESET_POS            4
#define BTMR_CTRL_AUTO_RESTART_POS      3
#define BTMR_CTRL_COUNT_DIR_POS         2
#define BTMR_CTRL_ENABLE_POS            0

/* =========================================================================*/
/* ================   32/16-bit Basic Timer                ================ */
/* =========================================================================*/

/*
 * Each Basic Timer is spaced 0x20 bytes apart.
 */
#define BTMR_SPACING            0x20
#define BTMR_SPACING_PWROF2     5
#define BTMR_MAX_INSTANCE       4

/**
  * @brief 32-bit and 16-bit Basic Timer (BTMR)
  * @note Basic timers 0 & 1 are 16-bit, 2 & 3 are 32-bit.
  */
typedef struct
{                           /*!< (@ 0x40000C00) BTMR Structure        */
  __IOM uint32_t CNT;       /*!< (@ 0x00000000) BTMR Count            */
  __IOM uint32_t PRLD;      /*!< (@ 0x00000004) BTMR Preload          */
  __IOM uint8_t  STS;       /*!< (@ 0x00000008) BTMR Status           */
        uint8_t  RSVDC[3];
  __IOM uint8_t  IEN;       /*!< (@ 0x0000000C) BTMR Interrupt Enable */
        uint8_t  RSVDD[3];
        uint32_t CTRL;      /*!< (@ 0x00000010) BTMR Control          */
} BTMR_Type;

/* =========================================================================*/
/* ================            HTMR                        ================ */
/* =========================================================================*/

#define HTMR_MAX_INSTANCES      2
#define HTMR_SPACING            0x20
#define HTMR_SPACING_PWROF2     5

/*
 * Set count resolution in bit[0]
 * 0 = 30.5 us (32786 Hz)
 * 1 = 125 ms (8 Hz)
 */
#define HTMR_CTRL_REG_MASK      0x01ul
#define HTMR_CTRL_RESOL_POS     0u
#define HTMR_CTRL_RESOL_MASK    (1u << (HTMR_CTRL_EN_POS))
#define HTMR_CTRL_RESOL_30US    (0u << (HTMR_CTRL_EN_POS))
#define HTMR_CTRL_RESOL_125MS   (1u << (HTMR_CTRL_EN_POS))

/*
 * Hibernation timer is started and stopped by writing a value
 * to the CNT (count) register.
 * Writing a non-zero value resets and start the counter counting down.
 * Writing 0 stops the timer.
 */
#define HTMR_CNT_STOP_VALUE     0u

/**
  * @brief Hibernation Timer (HTMR)
  */
typedef struct
{                           /*!< (@ 0x40009800) HTMR Structure   */
    __IOM uint16_t PRLD;    /*!< (@ 0x00000000) HTMR Preload */
          uint8_t  RSVD1[2];
    __IOM uint16_t CTRL;    /*!< (@ 0x00000004) HTMR Control */
          uint8_t  RSVD2[2];
    __IM  uint16_t CNT;     /*!< (@ 0x00000008) HTMR Count (RO) */
          uint8_t  RSVD3[2];
} HTMR_Type;

/* =========================================================================*/
/* ================   Capture/Compare Timer                ================ */
/* =========================================================================*/

#define CCT_BASE_ADDR          0x40001000
#define CCT_MAX_INSTANCE       1

/* Control register at offset 0x00 */
#define CCT_CTRL_ACTIVATE               (1ul << 0)
#define CCT_CTRL_FREE_RUN_EN            (1ul << 1)
#define CCT_CTRL_FREE_RUN_RESET         (1ul << 2)  /* self clearing bit */
#define CCT_CTRL_TCLK_MASK0             (0x07ul)
#define CCT_CTRL_TCLK_MASK              ((CCT_CTRL_TCLK_MASK0) << 4)
#define CCT_CTRL_TCLK_DIV_1             (0ul)
#define CCT_CTRL_TCLK_DIV_2             (1ul << 4)
#define CCT_CTRL_TCLK_DIV_4             (2ul << 4)
#define CCT_CTRL_TCLK_DIV_8             (3ul << 4)
#define CCT_CTRL_TCLK_DIV_16            (4ul << 4)
#define CCT_CTRL_TCLK_DIV_32            (5ul << 4)
#define CCT_CTRL_TCLK_DIV_64            (6ul << 4)
#define CCT_CTRL_TCLK_DIV_128           (7ul << 4)
#define CCT_CTRL_COMP0_EN               (1ul << 8)
#define CCT_CTRL_COMP1_EN               (1ul << 9)
#define CCT_CTRL_COMP1_SET              (1ul << 16) /* R/WS */
#define CCT_CTRL_COMP0_SET              (1ul << 17) /* R/WS */

/**
  * @brief Capture/Compare Timer (CCT)
  */
typedef struct
{                                   /*!< (@ 0x40001000) CCT Structure         */
    __IOM uint32_t CTRL;            /*!< (@ 0x00000000) CCT Control           */
    __IOM uint32_t CAP0_CTRL;       /*!< (@ 0x00000004) CCT Capture 0 Control */
    __IOM uint32_t CAP1_CTRL;       /*!< (@ 0x00000008) CCT Capture 1 Control */
    __IOM uint32_t FREE_RUN;        /*!< (@ 0x0000000C) CCT Free run counter  */
    __IOM uint32_t CAP0;            /*!< (@ 0x00000010) CCT Capture 0         */
    __IOM uint32_t CAP1;            /*!< (@ 0x00000014) CCT Capture 1         */
    __IOM uint32_t CAP2;            /*!< (@ 0x00000018) CCT Capture 2         */
    __IOM uint32_t CAP3;            /*!< (@ 0x0000001C) CCT Capture 3         */
    __IOM uint32_t CAP4;            /*!< (@ 0x00000020) CCT Capture 4         */
    __IOM uint32_t CAP5;            /*!< (@ 0x00000024) CCT Capture 5         */
    __IOM uint32_t COMP0;           /*!< (@ 0x00000028) CCT Compare 0         */
    __IOM uint32_t COMP1;           /*!< (@ 0x0000002C) CCT Compare 1         */
} CCT_Type;


/* =========================================================================*/
/* ================            RTMR                        ================ */
/* =========================================================================*/

#define RTMR_CTRL_MASK              0x1Ful
#define RTMR_CTRL_BLK_EN_POS        0u
#define RTMR_CTRL_BLK_EN_MASK       (1ul << (RTMR_CTRL_BLK_EN_POS))
#define RTMR_CTRL_BLK_EN            (1ul << (RTMR_CTRL_BLK_EN_POS))

#define RTMR_CTRL_AUTO_RELOAD_POS   1u
#define RTMR_CTRL_AUTO_RELOAD_MASK  (1ul << (RTMR_CTRL_AUTO_RELOAD_POS))
#define RTMR_CTRL_AUTO_RELOAD       (1ul << (RTMR_CTRL_AUTO_RELOAD_POS))

#define RTMR_CTRL_START_POS         2u
#define RTMR_CTRL_START_MASK        (1ul << (RTMR_CTRL_START_POS))
#define RTMR_CTRL_START             (1ul << (RTMR_CTRL_START_POS))

#define RTMR_CTRL_HW_HALT_EN_POS    3u
#define RTMR_CTRL_HW_HALT_EN_MASK   (1ul << (RTMR_CTRL_HW_HALT_EN_POS))
#define RTMR_CTRL_HW_HALT_EN        (1ul << (RTMR_CTRL_HW_HALT_EN_POS))

#define RTMR_CTRL_FW_HALT_EN_POS    4u
#define RTMR_CTRL_FW_HALT_EN_MASK   (1ul << (RTMR_CTRL_FW_HALT_EN_POS))
#define RTMR_CTRL_FW_HALT_EN        (1ul << (RTMR_CTRL_FW_HALT_EN_POS))


/**
  * @brief RTOS Timer (RTMR)
  */
typedef struct
{                           /*!< (@ 0x40007400) RTMR Structure   */
    __IOM uint32_t CNT;     /*!< (@ 0x00000000) RTMR Counter */
    __IOM uint32_t PRLD;    /*!< (@ 0x00000004) RTMR Preload */
    __IOM uint8_t  CTRL;    /*!< (@ 0x00000008) RTMR Control */
          uint8_t  RSVD1[3];
    __IOM uint32_t SOFTIRQ; /*!< (@ 0x0000000C) RTMR Soft IRQ */
} RTMR_Type;

/* =========================================================================*/
/* ================            WKTMR                       ================ */
/* =========================================================================*/

#define WKTMR_CTRL_MASK                 0x41ul
#define WKTMR_CTRL_WT_EN_POS            0u
#define WKTMR_CTRL_WT_EN_MASK           (1ul << (WKTMR_CTRL_WT_EN_POS))
#define WKTMR_CTRL_WT_EN                (1ul << (WKTMR_CTRL_WT_EN_POS))
#define WKTMR_CTRL_PWRUP_EV_EN_POS      6u
#define WKTMR_CTRL_PWRUP_EV_EN_MASK     (1ul << (WKTMR_CTRL_PWRUP_EV_EN_POS))
#define WKTMR_CTRL_PWRUP_EV_EN          (1ul << (WKTMR_CTRL_PWRUP_EV_EN_POS))

#define WKTMR_ALARM_CNT_MASK            0x0FFFFFFFul
#define WKTMR_TMR_CMP_MASK              0x0FFFFFFFul
#define WKTMR_CLK_DIV_MASK              0x7FFFul

#define WKTMR_SS_MASK                   0x0Ful
#define WKTMR_SS_RATE_DIS               0x00
#define WKTMR_SS_RATE_2HZ               0x01ul
#define WKTMR_SS_RATE_4HZ               0x02ul
#define WKTMR_SS_RATE_8HZ               0x03ul
#define WKTMR_SS_RATE_16HZ              0x04ul
#define WKTMR_SS_RATE_32HZ              0x05ul
#define WKTMR_SS_RATE_64HZ              0x06ul
#define WKTMR_SS_RATE_128HZ             0x07ul
#define WKTMR_SS_RATE_256HZ             0x08ul
#define WKTMR_SS_RATE_512HZ             0x09ul
#define WKTMR_SS_RATE_1024HZ            0x0Aul
#define WKTMR_SS_RATE_2048HZ            0x0Bul
#define WKTMR_SS_RATE_4096HZ            0x0Cul
#define WKTMR_SS_RATE_8192HZ            0x0Dul
#define WKTMR_SS_RATE_16384HZ           0x0Eul
#define WKTMR_SS_RATE_32768HZ           0x0Ful

#define WKTMR_SWKC_MASK                 0x3C3ul
#define WKTMR_SWKC_PWRUP_EV_STS_POS     0ul
#define WKTMR_SWKC_PWRUP_EV_STS_MASK    (1ul << (WKTMR_SWKC_PWRUP_EV_STS_POS))
#define WKTMR_SWKC_PWRUP_EV_STS         (1ul << (WKTMR_SWKC_PWRUP_EV_STS_POS))
#define WKTMR_SWKC_SYSPWR_PRES_STS_POS  4ul
#define WKTMR_SWKC_SYSPWR_PRES_STS_MASK (1ul << (WKTMR_SWKC_SYSPWR_PRES_STS_POS))
#define WKTMR_SWKC_SYSPWR_PRES_STS      (1ul << (WKTMR_SWKC_SYSPWR_PRES_STS_POS))
#define WKTMR_SWKC_SYSPWR_PRES_EN_POS   5ul
#define WKTMR_SWKC_SYSPWR_PRES_EN_MASK  (1ul << (WKTMR_SWKC_SYSPWR_PRES_EN_POS))
#define WKTMR_SWKC_SYSPWR_PRES_EN       (1ul << (WKTMR_SWKC_SYSPWR_PRES_EN_POS))
#define WKTMR_SWKC_AUTO_RELOAD_POS      6ul
#define WKTMR_SWKC_AUTO_RELOAD_MASK     (1ul << (WKTMR_SWKC_AUTO_RELOAD_POS))
#define WKTMR_SWKC_AUTO_RELOAD          (1ul << (WKTMR_SWKC_AUTO_RELOAD_POS))

/* TODO - More register defines */

/**
  * @brief Week Timer (WKTMR)
  */

typedef struct
{               /*!< (@ 0x4000AC80) WKTMR Structure   */
    __IOM uint32_t CTRL;            /*! (@ 0x00000000) WKTMR control */
    __IOM uint32_t ALARM_CNT;       /*! (@ 0x00000004) WKTMR Week alarm counter */
    __IOM uint32_t TMR_COMP;        /*! (@ 0x00000008) WKTMR Week timer compare */
    __IM  uint32_t CLKDIV;          /*! (@ 0x0000000C) WKTMR Clock Divider (RO) */
    __IOM uint32_t SS_INTR_SEL;     /*! (@ 0x00000010) WKTMR Sub-second interrupt select */
    __IOM uint32_t SWK_CTRL;        /*! (@ 0x00000014) WKTMR Sub-week control */
    __IOM uint32_t SWK_ALARM;       /*! (@ 0x00000018) WKTMR Sub-week alarm */
    __IOM uint32_t BGPO_DATA;       /*! (@ 0x0000001C) WKTMR BGPO data */
    __IOM uint32_t BGPO_PWR;        /*! (@ 0x00000020) WKTMR BGPO power */
    __IOM uint32_t BGPO_RST;        /*! (@ 0x00000024) WKTMR BGPO reset */
} WKTMR_Type;

#endif /* #ifndef _TIMER_H */
/* end timer.h */
/**   @}
 */
