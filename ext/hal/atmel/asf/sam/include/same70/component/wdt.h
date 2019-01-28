/**
 * \file
 *
 * \brief Component description for WDT
 *
 * Copyright (c) 2018 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

/* file generated from device description version 2017-08-25T14:00:00Z */
#ifndef _SAME70_WDT_COMPONENT_H_
#define _SAME70_WDT_COMPONENT_H_
#define _SAME70_WDT_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME_SAME70 Watchdog Timer
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR WDT */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define WDT_6080                       /**< (WDT) Module ID */
#define REV_WDT J                      /**< (WDT) Module revision */

/* -------- WDT_CR : (WDT Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WDRSTT:1;                  /**< bit:      0  Watchdog Restart                         */
    uint32_t :23;                       /**< bit:  1..23  Reserved */
    uint32_t KEY:8;                     /**< bit: 24..31  Password                                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} WDT_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define WDT_CR_OFFSET                       (0x00)                                        /**<  (WDT_CR) Control Register  Offset */

#define WDT_CR_WDRSTT_Pos                   0                                              /**< (WDT_CR) Watchdog Restart Position */
#define WDT_CR_WDRSTT_Msk                   (_U_(0x1) << WDT_CR_WDRSTT_Pos)                /**< (WDT_CR) Watchdog Restart Mask */
#define WDT_CR_WDRSTT                       WDT_CR_WDRSTT_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use WDT_CR_WDRSTT_Msk instead */
#define WDT_CR_KEY_Pos                      24                                             /**< (WDT_CR) Password Position */
#define WDT_CR_KEY_Msk                      (_U_(0xFF) << WDT_CR_KEY_Pos)                  /**< (WDT_CR) Password Mask */
#define WDT_CR_KEY(value)                   (WDT_CR_KEY_Msk & ((value) << WDT_CR_KEY_Pos))
#define   WDT_CR_KEY_PASSWD_Val             _U_(0xA5)                                      /**< (WDT_CR) Writing any other value in this field aborts the write operation.  */
#define WDT_CR_KEY_PASSWD                   (WDT_CR_KEY_PASSWD_Val << WDT_CR_KEY_Pos)      /**< (WDT_CR) Writing any other value in this field aborts the write operation. Position  */
#define WDT_CR_MASK                         _U_(0xFF000001)                                /**< \deprecated (WDT_CR) Register MASK  (Use WDT_CR_Msk instead)  */
#define WDT_CR_Msk                          _U_(0xFF000001)                                /**< (WDT_CR) Register Mask  */


/* -------- WDT_MR : (WDT Offset: 0x04) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WDV:12;                    /**< bit:  0..11  Watchdog Counter Value                   */
    uint32_t WDFIEN:1;                  /**< bit:     12  Watchdog Fault Interrupt Enable          */
    uint32_t WDRSTEN:1;                 /**< bit:     13  Watchdog Reset Enable                    */
    uint32_t :1;                        /**< bit:     14  Reserved */
    uint32_t WDDIS:1;                   /**< bit:     15  Watchdog Disable                         */
    uint32_t WDD:12;                    /**< bit: 16..27  Watchdog Delta Value                     */
    uint32_t WDDBGHLT:1;                /**< bit:     28  Watchdog Debug Halt                      */
    uint32_t WDIDLEHLT:1;               /**< bit:     29  Watchdog Idle Halt                       */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} WDT_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define WDT_MR_OFFSET                       (0x04)                                        /**<  (WDT_MR) Mode Register  Offset */

#define WDT_MR_WDV_Pos                      0                                              /**< (WDT_MR) Watchdog Counter Value Position */
#define WDT_MR_WDV_Msk                      (_U_(0xFFF) << WDT_MR_WDV_Pos)                 /**< (WDT_MR) Watchdog Counter Value Mask */
#define WDT_MR_WDV(value)                   (WDT_MR_WDV_Msk & ((value) << WDT_MR_WDV_Pos))
#define WDT_MR_WDFIEN_Pos                   12                                             /**< (WDT_MR) Watchdog Fault Interrupt Enable Position */
#define WDT_MR_WDFIEN_Msk                   (_U_(0x1) << WDT_MR_WDFIEN_Pos)                /**< (WDT_MR) Watchdog Fault Interrupt Enable Mask */
#define WDT_MR_WDFIEN                       WDT_MR_WDFIEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use WDT_MR_WDFIEN_Msk instead */
#define WDT_MR_WDRSTEN_Pos                  13                                             /**< (WDT_MR) Watchdog Reset Enable Position */
#define WDT_MR_WDRSTEN_Msk                  (_U_(0x1) << WDT_MR_WDRSTEN_Pos)               /**< (WDT_MR) Watchdog Reset Enable Mask */
#define WDT_MR_WDRSTEN                      WDT_MR_WDRSTEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use WDT_MR_WDRSTEN_Msk instead */
#define WDT_MR_WDDIS_Pos                    15                                             /**< (WDT_MR) Watchdog Disable Position */
#define WDT_MR_WDDIS_Msk                    (_U_(0x1) << WDT_MR_WDDIS_Pos)                 /**< (WDT_MR) Watchdog Disable Mask */
#define WDT_MR_WDDIS                        WDT_MR_WDDIS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use WDT_MR_WDDIS_Msk instead */
#define WDT_MR_WDD_Pos                      16                                             /**< (WDT_MR) Watchdog Delta Value Position */
#define WDT_MR_WDD_Msk                      (_U_(0xFFF) << WDT_MR_WDD_Pos)                 /**< (WDT_MR) Watchdog Delta Value Mask */
#define WDT_MR_WDD(value)                   (WDT_MR_WDD_Msk & ((value) << WDT_MR_WDD_Pos))
#define WDT_MR_WDDBGHLT_Pos                 28                                             /**< (WDT_MR) Watchdog Debug Halt Position */
#define WDT_MR_WDDBGHLT_Msk                 (_U_(0x1) << WDT_MR_WDDBGHLT_Pos)              /**< (WDT_MR) Watchdog Debug Halt Mask */
#define WDT_MR_WDDBGHLT                     WDT_MR_WDDBGHLT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use WDT_MR_WDDBGHLT_Msk instead */
#define WDT_MR_WDIDLEHLT_Pos                29                                             /**< (WDT_MR) Watchdog Idle Halt Position */
#define WDT_MR_WDIDLEHLT_Msk                (_U_(0x1) << WDT_MR_WDIDLEHLT_Pos)             /**< (WDT_MR) Watchdog Idle Halt Mask */
#define WDT_MR_WDIDLEHLT                    WDT_MR_WDIDLEHLT_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use WDT_MR_WDIDLEHLT_Msk instead */
#define WDT_MR_MASK                         _U_(0x3FFFBFFF)                                /**< \deprecated (WDT_MR) Register MASK  (Use WDT_MR_Msk instead)  */
#define WDT_MR_Msk                          _U_(0x3FFFBFFF)                                /**< (WDT_MR) Register Mask  */


/* -------- WDT_SR : (WDT Offset: 0x08) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WDUNF:1;                   /**< bit:      0  Watchdog Underflow (cleared on read)     */
    uint32_t WDERR:1;                   /**< bit:      1  Watchdog Error (cleared on read)         */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} WDT_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define WDT_SR_OFFSET                       (0x08)                                        /**<  (WDT_SR) Status Register  Offset */

#define WDT_SR_WDUNF_Pos                    0                                              /**< (WDT_SR) Watchdog Underflow (cleared on read) Position */
#define WDT_SR_WDUNF_Msk                    (_U_(0x1) << WDT_SR_WDUNF_Pos)                 /**< (WDT_SR) Watchdog Underflow (cleared on read) Mask */
#define WDT_SR_WDUNF                        WDT_SR_WDUNF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use WDT_SR_WDUNF_Msk instead */
#define WDT_SR_WDERR_Pos                    1                                              /**< (WDT_SR) Watchdog Error (cleared on read) Position */
#define WDT_SR_WDERR_Msk                    (_U_(0x1) << WDT_SR_WDERR_Pos)                 /**< (WDT_SR) Watchdog Error (cleared on read) Mask */
#define WDT_SR_WDERR                        WDT_SR_WDERR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use WDT_SR_WDERR_Msk instead */
#define WDT_SR_MASK                         _U_(0x03)                                      /**< \deprecated (WDT_SR) Register MASK  (Use WDT_SR_Msk instead)  */
#define WDT_SR_Msk                          _U_(0x03)                                      /**< (WDT_SR) Register Mask  */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief WDT hardware registers */
typedef struct {  
  __O  uint32_t WDT_CR;         /**< (WDT Offset: 0x00) Control Register */
  __IO uint32_t WDT_MR;         /**< (WDT Offset: 0x04) Mode Register */
  __I  uint32_t WDT_SR;         /**< (WDT Offset: 0x08) Status Register */
} Wdt;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief WDT hardware registers */
typedef struct {  
  __O  WDT_CR_Type                    WDT_CR;         /**< Offset: 0x00 ( /W  32) Control Register */
  __IO WDT_MR_Type                    WDT_MR;         /**< Offset: 0x04 (R/W  32) Mode Register */
  __I  WDT_SR_Type                    WDT_SR;         /**< Offset: 0x08 (R/   32) Status Register */
} Wdt;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/** @}  end of Watchdog Timer */

#endif /* _SAME70_WDT_COMPONENT_H_ */
