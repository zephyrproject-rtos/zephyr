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

/** @file espi_io.h
 *MEC1501 eSPI IO Component definitions
 */
/** @defgroup MEC1501 Peripherals eSPI IO Component
 */

#include <stdint.h>
#include <stddef.h>

#ifndef _ESPI_IO_H
#define _ESPI_IO_H


/*------------------------------------------------------------------*/

/* eSPI Global Capabilities 0 */
#define ESPI_GBL_CAP0_MASK          0x0Fu
#define ESPI_GBL_CAP0_PC_SUPP       (1u << 0)
#define ESPI_GBL_CAP0_VW_SUPP       (1u << 1)
#define ESPI_GBL_CAP0_OOB_SUPP      (1u << 2)
#define ESPI_GBL_CAP0_FC_SUPP       (1u << 3)

/* eSPI Global Capabilities 1 */
#define ESPI_GBL_CAP1_MASK              0xFFu
#define ESPI_GBL_CAP1_MAX_FREQ_POS      0u
#define ESPI_GBL_CAP1_MAX_FREQ_MASK     0x07u
#define ESPI_GBL_CAP1_MAX_FREQ_20M      0x00u
#define ESPI_GBL_CAP1_MAX_FREQ_25M      0x01u
#define ESPI_GBL_CAP1_MAX_FREQ_33M      0x02u
#define ESPI_GBL_CAP1_MAX_FREQ_50M      0x03u
#define ESPI_GBL_CAP1_MAX_FREQ_66M      0x04u
#define ESPI_GBL_CAP1_ALERT_POS         3 /* Read-Only */
#define ESPI_GBL_CAP1_ALERT_DED_PIN     (1u << (ESPI_GBL_CAP1_ALERT_POS))
#define ESPI_GBL_CAP1_ALERT_ON_IO1      (0u << (ESPI_GBL_CAP1_ALERT_POS))
#define ESPI_GBL_CAP1_IO_MODE_POS       4
#define ESPI_GBL_CAP1_IO_MODE_MASK0     0x03u
#define ESPI_GBL_CAP1_IO_MODE_MASK      ((ESPI_GBL_CAP1_IO_MODE_MASK0) << (ESPI_GBL_CAP1_IO_MODE_POS))
#define ESPI_GBL_CAP1_IO_MODE0_1        0u
#define ESPI_GBL_CAP1_IO_MODE0_12       1u
#define ESPI_GBL_CAP1_IO_MODE0_14       2u
#define ESPI_GBL_CAP1_IO_MODE0_124      3u
#define ESPI_GBL_CAP1_IO_MODE_1         ((ESPI_GBL_CAP1_IO_MODE0_1) << (ESPI_GBL_CAP1_IO_MODE_POS))
#define ESPI_GBL_CAP1_IO_MODE_12        ((ESPI_GBL_CAP1_IO_MODE0_12) << (ESPI_GBL_CAP1_IO_MODE_POS))
#define ESPI_GBL_CAP1_IO_MODE_14        ((ESPI_GBL_CAP1_IO_MODE0_14) << (ESPI_GBL_CAP1_IO_MODE_POS))
#define ESPI_GBL_CAP1_IO_MODE_124       ((ESPI_GBL_CAP1_IO_MODE0_124) << (ESPI_GBL_CAP1_IO_MODE_POS))
/*
 * Support Open Drain ALERT pin configuration
 * EC sets this bit if it can support open-drain ESPI_ALERT#
 */
#define ESPI_GBL_CAP1_ALERT_ODS_POS     6u
#define ESPI_GBL_CAP1_ALERT_ODS         (1u << (ESPI_GBL_CAP1_ALERT_ODS_POS))
/*
 * Read-Only ALERT Open Drain select.
 * If EC has indicated it can support open-drain ESPI_ALERT# then
 * the Host can enable open-drain ESPI_ALERT# by sending a configuraiton
 * message. This read-only bit relects the configuration selection.
 */
#define ESPI_GBL_CAP1_ALERT_ODS_SEL_POS 7u
#define ESPI_GBL_CAP1_ALERT_SEL_ODS     (1u << (ESPI_GBL_CAP1_ALERT_ODS_SEL_POS))

/* Peripheral Channel(PC) Capabilites */
#define ESPI_PC_CAP_MASK                0x07u
#define ESPI_PC_CAP_MAX_PLD_SZ_MASK     0x07u
#define ESPI_PC_CAP_MAX_PLD_SZ_64       0x01u
#define ESPI_PC_CAP_MAX_PLD_SZ_128      0x02u
#define ESPI_PC_CAP_MAX_PLD_SZ_256      0x03u

/* Virtual Wire(VW) Capabilities */
#define ESPI_VW_CAP_MASK                0x3Fu
#define ESPI_VW_CAP_MAX_VW_CNT_MASK     0x3Fu

/* Out-of-Band(OOB) Capabilities */
#define ESPI_OOB_CAP_MASK               0x07u
#define ESPI_OOB_CAP_MAX_PLD_SZ_MASK    0x07u
#define ESPI_OOB_CAP_MAX_PLD_SZ_73      0x01u
#define ESPI_OOB_CAP_MAX_PLD_SZ_137     0x02u
#define ESPI_OOB_CAP_MAX_PLD_SZ_265     0x03u

/* Flash Channel(FC) Capabilities */
#define ESPI_FC_CAP_MASK                0xFFu
#define ESPI_FC_CAP_MAX_PLD_SZ_MASK     0x07u
#define ESPI_FC_CAP_MAX_PLD_SZ_64       0x01u
#define ESPI_FC_CAP_SHARE_POS           3u
#define ESPI_FC_CAP_SHARE_MASK0         0x03u
#define ESPI_FC_CAP_SHARE_MASK          ((ESPI_FC_CAP_SHARE_MASK0) << (ESPI_FC_CAP_SHARE_POS))
#define ESPI_FC_CAP_SHARE_MAF_ONLY      (0u << (ESPI_FC_CAP_SHARE_POS))
#define ESPI_FC_CAP_SHARE_MAF2_ONLY     (1u << (ESPI_FC_CAP_SHARE_POS))
#define ESPI_FC_CAP_SHARE_SAF_ONLY      (2u << (ESPI_FC_CAP_SHARE_POS))
#define ESPI_FC_CAP_SHARE_MAF_SAF       (3u << (ESPI_FC_CAP_SHARE_POS))
#define ESPI_FC_CAP_MAX_RD_SZ_POS       5u
#define ESPI_FC_CAP_MAX_RD_SZ_MASK0     0x07u
#define ESPI_FC_CAP_MAX_RD_SZ_MASK      ((ESPI_FC_CAP_MAX_RD_SZ_MASK0) << (ESPI_FC_CAP_MAX_RD_SZ_POS))
#define ESPI_FC_CAP_MAX_RD_SZ_64        ((0x01u) << (ESPI_FC_CAP_MAX_RD_SZ_POS))

/* PC Ready */
#define ESPI_PC_READY_MASK              0x01u;
#define ESPI_PC_READY                   0x01u;

/* OOB Ready */
#define ESPI_OOB_READY_MASK             0x01u;
#define ESPI_OOB_READY                  0x01u;

/* FC Ready */
#define ESPI_FC_READY_MASK              0x01u;
#define ESPI_FC_READY                   0x01u;

/* ESPI_RESET# Interrupt Status */
#define ESPI_RST_ISTS_MASK              0x03u;
#define ESPI_RST_ISTS_POS               0u
#define ESPI_RST_ISTS                   (1u << (ESPI_RST_ISTS_POS))
#define ESPI_RST_ISTS_PIN_RO_POS        1ul
#define ESPI_RST_ISTS_PIN_RO_HI         (1u << (ESPI_RST_ISTS_PIN_RO_POS))

/* ESPI_RESET# Interrupt Enable */
#define ESPI_RST_IEN_MASK               0x01
#define ESPI_RST_IEN                    0x01

/* eSPI Platform Reset Source */
#define ESPI_PLTRST_SRC_MASK            0x01
#define ESPI_PLTRST_SRC_POS             0
#define ESPI_PLTRST_SRC_IS_PIN          0x01
#define ESPI_PLTRST_SRC_IS_VW           0x00

/* VW Ready */
#define ESPI_VW_READY_MASK              0x01u;
#define ESPI_VW_READY                   0x01u;

/* VW Error Status */
#define ESPI_VW_ERR_STS_MASK                0x33u
#define ESPI_VW_ERR_STS_FATAL_POS           0u
#define ESPI_VW_ERR_STS_FATAL_RO            (1u << (ESPI_VW_ERR_STS_FATAL_POS))
#define ESPI_VW_ERR_STS_FATAL_CLR_POS       1u
#define ESPI_VW_ERR_STS_FATAL_CLR_WO        (1u << (ESPI_VW_ERR_STS_FATAL_CLR_POS))
#define ESPI_VW_ERR_STS_NON_FATAL_POS       4u
#define ESPI_VW_ERR_STS_NON_FATAL_RO        (1u << (ESPI_VW_ERR_STS_NON_FATAL_POS))
#define ESPI_VW_ERR_STS_NON_FATAL_CLR_POS   5u
#define ESPI_VW_ERR_STS_NON_FATAL_CLR_WO    (1u << (ESPI_VW_ERR_STS_NON_FATAL_CLR_POS))

/* VW Channel Enable Status */
#define ESPI_VW_EN_STS_MASK                 0x01u
#define ESPI_VW_EN_STS_RO                   0x01u

/* =========================================================================*/
/* ================           eSPI IO Component            ================ */
/* =========================================================================*/

/**
  * @brief ESPI Host interface IO Component (ESPI_IO)
  */

/* IO BAR indices */
#define ESPI_IO_BAR_IOC     0
#define ESPI_IO_BAR_MEMC    1
#define ESPI_IO_BAR_MBOX    2
#define ESPI_IO_BAR_KBC     3
#define ESPI_IO_BAR_AEC0    4
#define ESPI_IO_BAR_AEC1    5
#define ESPI_IO_BAR_AEC2    6
#define ESPI_IO_BAR_AEC3    7
#define ESPI_IO_BAR_RSVD8   8
#define ESPI_IO_BAR_PM1     9
#define ESPI_IO_BAR_PORT92  10
#define ESPI_IO_BAR_UART0   11
#define ESPI_IO_BAR_UART1   12
#define ESPI_IO_BAR_EMI0    13
#define ESPI_IO_BAR_EMI1    14
#define ESPI_IO_BAR_RSVD15  15
#define ESPI_IO_BAR_P80CAP0 16
#define ESPI_IO_BAR_P80CAP1 17
#define ESPI_IO_BAR_RTC     18
#define ESPI_IO_BAR_ASIF    19
#define ESPI_IO_BAR_32B     20
#define ESPI_IO_BAR_UART2   21
#define ESPI_IO_BAR_GLUE    22
#define ESPI_IO_BAR_MAX     23

/* SERIRQ indices */
#define ESPI_SIRQ_MBOX_SIRQ     0
#define ESPI_SIRQ_MBOX_SMI      1
#define ESPI_SIRQ_KBC_KIRQ      2
#define ESPI_SIRQ_KBC_MIRQ      3
#define ESPI_SIRQ_ACPI_EC0      4
#define ESPI_SIRQ_ACPI_EC1      5
#define ESPI_SIRQ_ACPI_EC2      6
#define ESPI_SIRQ_ACPI_EC3      7
#define ESPI_SIRQ_RSVD8         8
#define ESPI_SIRQ_UART0         9
#define ESPI_SIRQ_UART1         10
#define ESPI_SIRQ_EMI0_HOST     11
#define ESPI_SIRQ_EMI0_E2H      12
#define ESPI_SIRQ_EMI1_HOST     13
#define ESPI_SIRQ_EMI1_E2H      14
#define ESPI_SIRQ_RSVD15        15
#define ESPI_SIRQ_RSVD16        16
#define ESPI_SIRQ_RTC           17
#define ESPI_SIRQ_EC            18
#define ESPI_SIRQ_UART2         19
#define ESPI_SIRQ_MAX           20

/*
 * ESPI_IO_CAP - eSPI IO capabilities, channel ready, activate,
 * EC 
 * registers @ 0x400F36B0
 */
typedef struct
{
   __IOM uint32_t   VW_EN_STS;                   /*! (@ 0x36B0) Virtual Wire Enable Status */
          uint8_t       RSVD1[0x36E0 - 0x36B4];
    __IOM uint8_t   CAP_ID;                      /*! (@ 0x36E0) Capabilities ID */
    __IOM uint8_t   GLB_CAP0;                    /*! (@ 0x36E1) Global Capabilities 0 */
    __IOM uint8_t   GLB_CAP1;                    /*! (@ 0x36E2) Global Capabilities 1 */
    __IOM uint8_t   PC_CAP;                      /*! (@ 0x3633) Periph Chan Capabilities */
    __IOM uint8_t   VW_CAP;                      /*! (@ 0x3634) Virtual Wire Chan Capabilities */
    __IOM uint8_t   OOB_CAP;                     /*! (@ 0x3635) OOB Chan Capabilities */
    __IOM uint8_t   FC_CAP;                      /*! (@ 0x3636) Flash Chan Capabilities */
    __IOM uint8_t   PC_RDY;                      /*! (@ 0x3637) PC ready */
    __IOM uint8_t   OOB_RDY;                     /*! (@ 0x3638) OOB ready */
    __IOM uint8_t   FC_RDY;                      /*! (@ 0x3639) OOB ready */
    __IOM uint8_t   ERST_STS;                    /*! (@ 0x363A) eSPI Reset interrupt status */
    __IOM uint8_t   ERST_IEN;                    /*! (@ 0x363B) eSPI Reset interrupt enable */
    __IOM uint8_t   PLTRST_SRC;                  /*! (@ 0x363C) Platform Reset Source */
    __IOM uint8_t   VW_RDY;                      /*! (@ 0x363D) VW ready */
          uint8_t       RSVD2[0x3730 - 0x36EE];
    __IOM uint32_t  ACTV;                        /*! (@ 0x3730) eSPI IO activate */
          uint8_t       RSVD3[0x37F0 - 0x3734];
    __IOM uint32_t  VW_ERR_STS;                  /*! (@ 0x37F0) IO Virtual Wire Error */
} ESPI_IO_CAP;

/*
 * ESPI_IO_PC - eSPI IO Peripheral Channel registers @ 0x400F3500
 */
typedef struct
{
    __IOM uint32_t      PC_LC_ADDR_LSW;              /*! (@ 0x00000100) Periph Chan Last Cycle address LSW */
    __IOM uint32_t      PC_LC_ADDR_MSW;              /*! (@ 0x00000104) Periph Chan Last Cycle address MSW */
    __IOM uint32_t      PC_LC_LEN_TYPE_TAG;          /*! (@ 0x00000108) Periph Chan Last Cycle length/type/tag */
    __IOM uint32_t      PC_ERR_ADDR_LSW;             /*! (@ 0x0000010C) Periph Chan Error Address LSW */
    __IOM uint32_t      PC_ERR_ADDR_MSW;             /*! (@ 0x00000110) Periph Chan Error Address MSW */
    __IOM uint32_t      PC_STATUS;                   /*! (@ 0x00000114) Periph Chan Status */
    __IOM uint32_t      PC_IEN;                      /*! (@ 0x00000118) Periph Chan IEN */
} ESPI_IO_PC;

/*
 * ESPIO_IO_LTR - eSPI IO LTR registers @ 0x400F3620
 */
typedef struct
{
    __IOM uint32_t      LTR_STS;                     /*! (@ 0x0220) LTR Periph Status */
    __IOM uint32_t      LTR_EN;                      /*! (@ 0x0224) LTR Periph Enable */
    __IOM uint32_t      LTR_CTRL;                    /*! (@ 0x0228) LTR Periph Control */
    __IOM uint32_t      LTR_MESG;                    /*! (@ 0x022C) LTR Periph Message */
} ESPI_IO_LTR;

/*
 * ESPIO_IO_OOB - eSPI IO OOB registers @ 0x400F3640
 */
typedef struct
{
    __IOM uint32_t      RX_ADDR_LSW;             /*! (@ 0x0240) OOB Receive Address bits[31:0] */
    __IOM uint32_t      RX_ADDR_MSW;             /*! (@ 0x0244) OOB Receive Address bits[63:32] */
    __IOM uint32_t      TX_ADDR_LSW;             /*! (@ 0x0248) OOB Transmit Address bits[31:0] */
    __IOM uint32_t      TX_ADDR_MSW;             /*! (@ 0x024C) OOB Transmit Address bits[63:32] */
    __IOM uint32_t      RX_LEN;                  /*! (@ 0x0250) OOB Receive length */
    __IOM uint32_t      TX_LEN;                  /*! (@ 0x0254) OOB Transmit length */
    __IOM uint32_t      RX_CTRL;                 /*! (@ 0x0258) OOB Receive control */
    __IOM uint32_t      RX_IEN;                  /*! (@ 0x025C) OOB Receive interrupt enable */
    __IOM uint32_t      RX_STS;                  /*! (@ 0x0260) OOB Receive interrupt status */
    __IOM uint32_t      TX_CTRL;                 /*! (@ 0x0264) OOB Transmit control */
    __IOM uint32_t      TX_IEN;                  /*! (@ 0x0268) OOB Transmit interrupt enable */
    __IOM uint32_t      TX_STS;                  /*! (@ 0x026C) OOB Transmit interrupt status */
} ESPI_IO_OOB;

/*
 * ESPI_IO_FC - eSPI IO Flash channel registers @ 0x40003680
 */
typedef struct
{
    __IOM uint32_t      FL_ADDR_LSW;                /*! (@ 0x0280) FC flash address bits[31:0] */
    __IOM uint32_t      FL_ADDR_MSW;                /*! (@ 0x0284) FC flash address bits[63:32] */
    __IOM uint32_t      MEM_ADDR_LSW;               /*! (@ 0x0288) FC EC Memory address bits[31:0] */
    __IOM uint32_t      MEM_ADDR_MSW;               /*! (@ 0x028C) FC EC Memory address bits[63:32] */
    __IOM uint32_t      XFR_LEN;                    /*! (@ 0x0290) FC transfer length */
    __IOM uint32_t      CTRL;                       /*! (@ 0x0294) FC Control */
    __IOM uint32_t      IEN;                        /*! (@ 0x0298) FC interrupt enable */
    __IOM uint32_t      CFG;                        /*! (@ 0x029C) FC configuration */
    __IOM uint32_t      STS;                        /*! (@ 0x02A0) FC status */
} ESPI_IO_FC;

/*
 * ESPI_IO_BAR_HOST - eSPI IO Host visible BAR registers @ 0x400F3520
 */
typedef struct
{
    __IOM uint32_t      IOBAR_INH_LSW;               /*! (@ 0x00000120) BAR Inhibit LSW */
    __IOM uint32_t      IOBAR_INH_MSW;               /*! (@ 0x00000124) BAR Inhibit MSW */
    __IOM uint32_t      IOBAR_INIT;                  /*! (@ 0x00000128) BAR Init */
    __IOM uint32_t      EC_IRQ;                      /*! (@ 0x0000012C) EC IRQ */
          uint8_t           RSVD3[4];
    union {
        __IOM uint32_t  u32;
        struct {
            __IOM uint16_t VALID;
            __IOM uint16_t HOST_IO_ADDR;
        } HIB;
    } HOST_IO_BAR[ESPI_IO_BAR_MAX]; /*! (@ 0x3534 - 0x358F) Host address IO BAR's */
} ESPI_IO_BAR_HOST;

/*
 * ESPIO_IO_BAR_EC - eSPI IO EC-only component of IO BAR @ 0x400F3734
 */
typedef struct
{
    union {
        __IOM uint32_t  u32;
        struct {
            __IOM uint8_t  MASK;
            __IOM uint8_t  LDN;
            __IOM uint16_t VIRT;
        } EIB;
    } EC_IO_BAR[ESPI_IO_BAR_MAX]; /*! (@ 0x3734 - 0x378F) Host address IO BAR's */
} ESPI_IO_BAR_EC;

#endif /* #ifndef _ESPI_IO_H */
/* end espi_io.h */
/**   @}
 */

