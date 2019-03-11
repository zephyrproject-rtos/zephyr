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

/** @file mec1501_ecia.h
 *MEC1501 EC Interrupt Aggregator Subsystem definitions
 */
/** @defgroup MEC1501 Peripherals ECIA
 */

#ifndef _ECIA_H
#define _ECIA_H

#include <stdint.h>
#include <stddef.h>


#define MEC_ECIA_ADDR   0x4000E000
#define MEC_FIRST_GIRQ  8
#define MEC_LAST_GIRQ   26

#define MEC_ECIA_GIRQ_NO_NVIC   22

#define MEC_ECIA_AGGR_BITMAP ((1ul << 8) + (1ul << 9) + (1ul << 10) +\
                                (1ul << 11) + (1ul << 12) + (1ul << 24) +\
                                (1ul << 25) + (1ul << 26))

#define MEC_ECIA_DIRECT_BITMAP  ((1ul << 13) + (1ul << 14) +\
                                     (1ul << 15) + (1ul << 16) +\
                                     (1ul << 17) + (1ul << 18) +\
                                     (1ul << 19) + (1ul << 21) +\
                                     (1ul << 23))

/*
 * ARM Cortex-M4 NVIC registers
 * External sources are grouped by 32-bit registers.
 * MEC15xx has 173 external sources requiring 6 32-bit registers.
  */
#define MEC_NUM_NVIC_REGS           6
#define MEC_NVIC_SET_EN_BASE        0xE000E100
#define MEC_NVIC_CLR_EN_BASE        0xE000E180
#define MEC_NVIC_SET_PEND_BASE      0xE000E200
#define MEC_NVIC_CLR_PEND_BASE      0xE000E280
#define MEC_NVIC_ACTIVE_BASE        0xE000E800
#define MEC_NVIC_PRI_BASE           0xE000E400

/* =========================================================================*/
/* ================            ECIA                        ================ */
/* =========================================================================*/

/**
  * @brief EC Interrupt Aggregator (ECIA)
  */
#define GIRQ_START_NUM              8
#define GIRQ_LAST_NUM               26
#define GIRQ_IDX(girq)              ((girq) - 8)
#define GIRQ_IDX_FIRST              0
#define GIRQ_IDX_MAX                19
#define MAX_NVIC_IDX                6

/* size = 0x14(20) bytes */
typedef struct
{
    __IOM uint32_t SRC;
    __IOM uint32_t EN_SET;
    __IOM uint32_t RESULT;
    __IOM uint32_t EN_CLR;
          uint8_t  RSVD1[4];
} GIRQ_Type;

typedef struct
{               /*!< (@ 0x4000E000) ECIA Structure   */
    GIRQ_Type       GIRQ[GIRQ_IDX_MAX]; /*! (@ 0x00000000) GIRQ08 - GIRQ26 */
          uint8_t   RSVD2[(0x200 - ((GIRQ_IDX_MAX) * (20)))]; /* offsets 0x0180 - 0x1FF */
    __IOM uint32_t  BLK_EN_SET;         /*! (@ 0x00000200) Aggregated GIRQ output Enable Set */
    __IOM uint32_t  BLK_EN_CLR;         /*! (@ 0x00000204) Aggregated GIRQ output Enable Clear */
    __IM  uint32_t  BLK_ACTIVE;         /*! (@ 0x00000204) GIRQ Active (RO) */
} ECIA_Type;

#endif // #ifndef _ECIA_H
/* end ecia.h */
/**   @}
 */


