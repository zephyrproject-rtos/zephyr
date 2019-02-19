/**
 * \file
 *
 * \brief Component description for RFCTRL
 *
 * Copyright (c) 2016 Atmel Corporation,
 *                    a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAMR21_RFCTRL_COMPONENT_
#define _SAMR21_RFCTRL_COMPONENT_

/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR RFCTRL */
/* ========================================================================== */
/** \addtogroup SAMR21_RFCTRL RF233 control module */
/*@{*/

#define RFCTRL_U2233
#define REV_RFCTRL                  0x100

/* -------- RFCTRL_FECFG : (RFCTRL Offset: 0x0) (R/W 16) Front-end control bus configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t F0CFG:2;          /*!< bit:  0.. 1  Front-end control signal 0 configuration */
    uint16_t F1CFG:2;          /*!< bit:  2.. 3  Front-end control signal 1 configuration */
    uint16_t F2CFG:2;          /*!< bit:  4.. 5  Front-end control signal 2 configuration */
    uint16_t F3CFG:2;          /*!< bit:  6.. 7  Front-end control signal 3 configuration */
    uint16_t F4CFG:2;          /*!< bit:  8.. 9  Front-end control signal 4 configuration */
    uint16_t F5CFG:2;          /*!< bit: 10..11  Front-end control signal 5 configuration */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RFCTRL_FECFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RFCTRL_FECFG_OFFSET         0x0          /**< \brief (RFCTRL_FECFG offset) Front-end control bus configuration */
#define RFCTRL_FECFG_RESETVALUE     _U(0x0000)   /**< \brief (RFCTRL_FECFG reset_value) Front-end control bus configuration */

#define RFCTRL_FECFG_F0CFG_Pos      0            /**< \brief (RFCTRL_FECFG) Front-end control signal 0 configuration */
#define RFCTRL_FECFG_F0CFG_Msk      (_U(0x3) << RFCTRL_FECFG_F0CFG_Pos)
#define RFCTRL_FECFG_F0CFG(value)   (RFCTRL_FECFG_F0CFG_Msk & ((value) << RFCTRL_FECFG_F0CFG_Pos))
#define RFCTRL_FECFG_F1CFG_Pos      2            /**< \brief (RFCTRL_FECFG) Front-end control signal 1 configuration */
#define RFCTRL_FECFG_F1CFG_Msk      (_U(0x3) << RFCTRL_FECFG_F1CFG_Pos)
#define RFCTRL_FECFG_F1CFG(value)   (RFCTRL_FECFG_F1CFG_Msk & ((value) << RFCTRL_FECFG_F1CFG_Pos))
#define RFCTRL_FECFG_F2CFG_Pos      4            /**< \brief (RFCTRL_FECFG) Front-end control signal 2 configuration */
#define RFCTRL_FECFG_F2CFG_Msk      (_U(0x3) << RFCTRL_FECFG_F2CFG_Pos)
#define RFCTRL_FECFG_F2CFG(value)   (RFCTRL_FECFG_F2CFG_Msk & ((value) << RFCTRL_FECFG_F2CFG_Pos))
#define RFCTRL_FECFG_F3CFG_Pos      6            /**< \brief (RFCTRL_FECFG) Front-end control signal 3 configuration */
#define RFCTRL_FECFG_F3CFG_Msk      (_U(0x3) << RFCTRL_FECFG_F3CFG_Pos)
#define RFCTRL_FECFG_F3CFG(value)   (RFCTRL_FECFG_F3CFG_Msk & ((value) << RFCTRL_FECFG_F3CFG_Pos))
#define RFCTRL_FECFG_F4CFG_Pos      8            /**< \brief (RFCTRL_FECFG) Front-end control signal 4 configuration */
#define RFCTRL_FECFG_F4CFG_Msk      (_U(0x3) << RFCTRL_FECFG_F4CFG_Pos)
#define RFCTRL_FECFG_F4CFG(value)   (RFCTRL_FECFG_F4CFG_Msk & ((value) << RFCTRL_FECFG_F4CFG_Pos))
#define RFCTRL_FECFG_F5CFG_Pos      10           /**< \brief (RFCTRL_FECFG) Front-end control signal 5 configuration */
#define RFCTRL_FECFG_F5CFG_Msk      (_U(0x3) << RFCTRL_FECFG_F5CFG_Pos)
#define RFCTRL_FECFG_F5CFG(value)   (RFCTRL_FECFG_F5CFG_Msk & ((value) << RFCTRL_FECFG_F5CFG_Pos))
#define RFCTRL_FECFG_MASK           _U(0x0FFF)   /**< \brief (RFCTRL_FECFG) MASK Register */

/** \brief RFCTRL hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO RFCTRL_FECFG_Type         FECFG;       /**< \brief Offset: 0x0 (R/W 16) Front-end control bus configuration */
} Rfctrl;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/*@}*/

#endif /* _SAMR21_RFCTRL_COMPONENT_ */
