/**
 * \file
 *
 * \brief Component description for ISI
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
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

#ifndef _SAME70_ISI_COMPONENT_H_
#define _SAME70_ISI_COMPONENT_H_
#define _SAME70_ISI_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_ISI Image Sensor Interface
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR ISI */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define ISI_6350                       /**< (ISI) Module ID */
#define REV_ISI I                      /**< (ISI) Module revision */

/* -------- ISI_CFG1 : (ISI Offset: 0x00) (R/W 32) ISI Configuration 1 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t HSYNC_POL:1;               /**< bit:      2  Horizontal Synchronization Polarity      */
    uint32_t VSYNC_POL:1;               /**< bit:      3  Vertical Synchronization Polarity        */
    uint32_t PIXCLK_POL:1;              /**< bit:      4  Pixel Clock Polarity                     */
    uint32_t :1;                        /**< bit:      5  Reserved */
    uint32_t EMB_SYNC:1;                /**< bit:      6  Embedded Synchronization                 */
    uint32_t CRC_SYNC:1;                /**< bit:      7  Embedded Synchronization Correction      */
    uint32_t FRATE:3;                   /**< bit:  8..10  Frame Rate [0..7]                        */
    uint32_t DISCR:1;                   /**< bit:     11  Disable Codec Request                    */
    uint32_t FULL:1;                    /**< bit:     12  Full Mode is Allowed                     */
    uint32_t THMASK:2;                  /**< bit: 13..14  Threshold Mask                           */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t SLD:8;                     /**< bit: 16..23  Start of Line Delay                      */
    uint32_t SFD:8;                     /**< bit: 24..31  Start of Frame Delay                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_CFG1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_CFG1_OFFSET                     (0x00)                                        /**<  (ISI_CFG1) ISI Configuration 1 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_CFG1_HSYNC_POL_Pos              2                                              /**< (ISI_CFG1) Horizontal Synchronization Polarity Position */
#define ISI_CFG1_HSYNC_POL_Msk              (0x1U << ISI_CFG1_HSYNC_POL_Pos)               /**< (ISI_CFG1) Horizontal Synchronization Polarity Mask */
#define ISI_CFG1_HSYNC_POL                  ISI_CFG1_HSYNC_POL_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG1_HSYNC_POL_Msk instead */
#define ISI_CFG1_VSYNC_POL_Pos              3                                              /**< (ISI_CFG1) Vertical Synchronization Polarity Position */
#define ISI_CFG1_VSYNC_POL_Msk              (0x1U << ISI_CFG1_VSYNC_POL_Pos)               /**< (ISI_CFG1) Vertical Synchronization Polarity Mask */
#define ISI_CFG1_VSYNC_POL                  ISI_CFG1_VSYNC_POL_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG1_VSYNC_POL_Msk instead */
#define ISI_CFG1_PIXCLK_POL_Pos             4                                              /**< (ISI_CFG1) Pixel Clock Polarity Position */
#define ISI_CFG1_PIXCLK_POL_Msk             (0x1U << ISI_CFG1_PIXCLK_POL_Pos)              /**< (ISI_CFG1) Pixel Clock Polarity Mask */
#define ISI_CFG1_PIXCLK_POL                 ISI_CFG1_PIXCLK_POL_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG1_PIXCLK_POL_Msk instead */
#define ISI_CFG1_EMB_SYNC_Pos               6                                              /**< (ISI_CFG1) Embedded Synchronization Position */
#define ISI_CFG1_EMB_SYNC_Msk               (0x1U << ISI_CFG1_EMB_SYNC_Pos)                /**< (ISI_CFG1) Embedded Synchronization Mask */
#define ISI_CFG1_EMB_SYNC                   ISI_CFG1_EMB_SYNC_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG1_EMB_SYNC_Msk instead */
#define ISI_CFG1_CRC_SYNC_Pos               7                                              /**< (ISI_CFG1) Embedded Synchronization Correction Position */
#define ISI_CFG1_CRC_SYNC_Msk               (0x1U << ISI_CFG1_CRC_SYNC_Pos)                /**< (ISI_CFG1) Embedded Synchronization Correction Mask */
#define ISI_CFG1_CRC_SYNC                   ISI_CFG1_CRC_SYNC_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG1_CRC_SYNC_Msk instead */
#define ISI_CFG1_FRATE_Pos                  8                                              /**< (ISI_CFG1) Frame Rate [0..7] Position */
#define ISI_CFG1_FRATE_Msk                  (0x7U << ISI_CFG1_FRATE_Pos)                   /**< (ISI_CFG1) Frame Rate [0..7] Mask */
#define ISI_CFG1_FRATE(value)               (ISI_CFG1_FRATE_Msk & ((value) << ISI_CFG1_FRATE_Pos))
#define ISI_CFG1_DISCR_Pos                  11                                             /**< (ISI_CFG1) Disable Codec Request Position */
#define ISI_CFG1_DISCR_Msk                  (0x1U << ISI_CFG1_DISCR_Pos)                   /**< (ISI_CFG1) Disable Codec Request Mask */
#define ISI_CFG1_DISCR                      ISI_CFG1_DISCR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG1_DISCR_Msk instead */
#define ISI_CFG1_FULL_Pos                   12                                             /**< (ISI_CFG1) Full Mode is Allowed Position */
#define ISI_CFG1_FULL_Msk                   (0x1U << ISI_CFG1_FULL_Pos)                    /**< (ISI_CFG1) Full Mode is Allowed Mask */
#define ISI_CFG1_FULL                       ISI_CFG1_FULL_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG1_FULL_Msk instead */
#define ISI_CFG1_THMASK_Pos                 13                                             /**< (ISI_CFG1) Threshold Mask Position */
#define ISI_CFG1_THMASK_Msk                 (0x3U << ISI_CFG1_THMASK_Pos)                  /**< (ISI_CFG1) Threshold Mask Mask */
#define ISI_CFG1_THMASK(value)              (ISI_CFG1_THMASK_Msk & ((value) << ISI_CFG1_THMASK_Pos))
#define   ISI_CFG1_THMASK_BEATS_4_Val       (0x0U)                                         /**< (ISI_CFG1) Only 4 beats AHB burst allowed  */
#define   ISI_CFG1_THMASK_BEATS_8_Val       (0x1U)                                         /**< (ISI_CFG1) Only 4 and 8 beats AHB burst allowed  */
#define   ISI_CFG1_THMASK_BEATS_16_Val      (0x2U)                                         /**< (ISI_CFG1) 4, 8 and 16 beats AHB burst allowed  */
#define ISI_CFG1_THMASK_BEATS_4             (ISI_CFG1_THMASK_BEATS_4_Val << ISI_CFG1_THMASK_Pos)  /**< (ISI_CFG1) Only 4 beats AHB burst allowed Position  */
#define ISI_CFG1_THMASK_BEATS_8             (ISI_CFG1_THMASK_BEATS_8_Val << ISI_CFG1_THMASK_Pos)  /**< (ISI_CFG1) Only 4 and 8 beats AHB burst allowed Position  */
#define ISI_CFG1_THMASK_BEATS_16            (ISI_CFG1_THMASK_BEATS_16_Val << ISI_CFG1_THMASK_Pos)  /**< (ISI_CFG1) 4, 8 and 16 beats AHB burst allowed Position  */
#define ISI_CFG1_SLD_Pos                    16                                             /**< (ISI_CFG1) Start of Line Delay Position */
#define ISI_CFG1_SLD_Msk                    (0xFFU << ISI_CFG1_SLD_Pos)                    /**< (ISI_CFG1) Start of Line Delay Mask */
#define ISI_CFG1_SLD(value)                 (ISI_CFG1_SLD_Msk & ((value) << ISI_CFG1_SLD_Pos))
#define ISI_CFG1_SFD_Pos                    24                                             /**< (ISI_CFG1) Start of Frame Delay Position */
#define ISI_CFG1_SFD_Msk                    (0xFFU << ISI_CFG1_SFD_Pos)                    /**< (ISI_CFG1) Start of Frame Delay Mask */
#define ISI_CFG1_SFD(value)                 (ISI_CFG1_SFD_Msk & ((value) << ISI_CFG1_SFD_Pos))
#define ISI_CFG1_Msk                        (0xFFFF7FDCU)                                  /**< (ISI_CFG1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_CFG2 : (ISI Offset: 0x04) (R/W 32) ISI Configuration 2 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IM_VSIZE:11;               /**< bit:  0..10  Vertical Size of the Image Sensor [0..2047] */
    uint32_t GS_MODE:1;                 /**< bit:     11  Grayscale Pixel Format Mode              */
    uint32_t RGB_MODE:1;                /**< bit:     12  RGB Input Mode                           */
    uint32_t GRAYSCALE:1;               /**< bit:     13  Grayscale Mode Format Enable             */
    uint32_t RGB_SWAP:1;                /**< bit:     14  RGB Format Swap Mode                     */
    uint32_t COL_SPACE:1;               /**< bit:     15  Color Space for the Image Data           */
    uint32_t IM_HSIZE:11;               /**< bit: 16..26  Horizontal Size of the Image Sensor [0..2047] */
    uint32_t :1;                        /**< bit:     27  Reserved */
    uint32_t YCC_SWAP:2;                /**< bit: 28..29  YCrCb Format Swap Mode                   */
    uint32_t RGB_CFG:2;                 /**< bit: 30..31  RGB Pixel Mapping Configuration          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_CFG2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_CFG2_OFFSET                     (0x04)                                        /**<  (ISI_CFG2) ISI Configuration 2 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_CFG2_IM_VSIZE_Pos               0                                              /**< (ISI_CFG2) Vertical Size of the Image Sensor [0..2047] Position */
#define ISI_CFG2_IM_VSIZE_Msk               (0x7FFU << ISI_CFG2_IM_VSIZE_Pos)              /**< (ISI_CFG2) Vertical Size of the Image Sensor [0..2047] Mask */
#define ISI_CFG2_IM_VSIZE(value)            (ISI_CFG2_IM_VSIZE_Msk & ((value) << ISI_CFG2_IM_VSIZE_Pos))
#define ISI_CFG2_GS_MODE_Pos                11                                             /**< (ISI_CFG2) Grayscale Pixel Format Mode Position */
#define ISI_CFG2_GS_MODE_Msk                (0x1U << ISI_CFG2_GS_MODE_Pos)                 /**< (ISI_CFG2) Grayscale Pixel Format Mode Mask */
#define ISI_CFG2_GS_MODE                    ISI_CFG2_GS_MODE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG2_GS_MODE_Msk instead */
#define ISI_CFG2_RGB_MODE_Pos               12                                             /**< (ISI_CFG2) RGB Input Mode Position */
#define ISI_CFG2_RGB_MODE_Msk               (0x1U << ISI_CFG2_RGB_MODE_Pos)                /**< (ISI_CFG2) RGB Input Mode Mask */
#define ISI_CFG2_RGB_MODE                   ISI_CFG2_RGB_MODE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG2_RGB_MODE_Msk instead */
#define ISI_CFG2_GRAYSCALE_Pos              13                                             /**< (ISI_CFG2) Grayscale Mode Format Enable Position */
#define ISI_CFG2_GRAYSCALE_Msk              (0x1U << ISI_CFG2_GRAYSCALE_Pos)               /**< (ISI_CFG2) Grayscale Mode Format Enable Mask */
#define ISI_CFG2_GRAYSCALE                  ISI_CFG2_GRAYSCALE_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG2_GRAYSCALE_Msk instead */
#define ISI_CFG2_RGB_SWAP_Pos               14                                             /**< (ISI_CFG2) RGB Format Swap Mode Position */
#define ISI_CFG2_RGB_SWAP_Msk               (0x1U << ISI_CFG2_RGB_SWAP_Pos)                /**< (ISI_CFG2) RGB Format Swap Mode Mask */
#define ISI_CFG2_RGB_SWAP                   ISI_CFG2_RGB_SWAP_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG2_RGB_SWAP_Msk instead */
#define ISI_CFG2_COL_SPACE_Pos              15                                             /**< (ISI_CFG2) Color Space for the Image Data Position */
#define ISI_CFG2_COL_SPACE_Msk              (0x1U << ISI_CFG2_COL_SPACE_Pos)               /**< (ISI_CFG2) Color Space for the Image Data Mask */
#define ISI_CFG2_COL_SPACE                  ISI_CFG2_COL_SPACE_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CFG2_COL_SPACE_Msk instead */
#define ISI_CFG2_IM_HSIZE_Pos               16                                             /**< (ISI_CFG2) Horizontal Size of the Image Sensor [0..2047] Position */
#define ISI_CFG2_IM_HSIZE_Msk               (0x7FFU << ISI_CFG2_IM_HSIZE_Pos)              /**< (ISI_CFG2) Horizontal Size of the Image Sensor [0..2047] Mask */
#define ISI_CFG2_IM_HSIZE(value)            (ISI_CFG2_IM_HSIZE_Msk & ((value) << ISI_CFG2_IM_HSIZE_Pos))
#define ISI_CFG2_YCC_SWAP_Pos               28                                             /**< (ISI_CFG2) YCrCb Format Swap Mode Position */
#define ISI_CFG2_YCC_SWAP_Msk               (0x3U << ISI_CFG2_YCC_SWAP_Pos)                /**< (ISI_CFG2) YCrCb Format Swap Mode Mask */
#define ISI_CFG2_YCC_SWAP(value)            (ISI_CFG2_YCC_SWAP_Msk & ((value) << ISI_CFG2_YCC_SWAP_Pos))
#define   ISI_CFG2_YCC_SWAP_DEFAULT_Val     (0x0U)                                         /**< (ISI_CFG2) Byte 0 Cb(i)Byte 1 Y(i)Byte 2 Cr(i)Byte 3 Y(i+1)  */
#define   ISI_CFG2_YCC_SWAP_MODE1_Val       (0x1U)                                         /**< (ISI_CFG2) Byte 0 Cr(i)Byte 1 Y(i)Byte 2 Cb(i)Byte 3 Y(i+1)  */
#define   ISI_CFG2_YCC_SWAP_MODE2_Val       (0x2U)                                         /**< (ISI_CFG2) Byte 0 Y(i)Byte 1 Cb(i)Byte 2 Y(i+1)Byte 3 Cr(i)  */
#define   ISI_CFG2_YCC_SWAP_MODE3_Val       (0x3U)                                         /**< (ISI_CFG2) Byte 0 Y(i)Byte 1 Cr(i)Byte 2 Y(i+1)Byte 3 Cb(i)  */
#define ISI_CFG2_YCC_SWAP_DEFAULT           (ISI_CFG2_YCC_SWAP_DEFAULT_Val << ISI_CFG2_YCC_SWAP_Pos)  /**< (ISI_CFG2) Byte 0 Cb(i)Byte 1 Y(i)Byte 2 Cr(i)Byte 3 Y(i+1) Position  */
#define ISI_CFG2_YCC_SWAP_MODE1             (ISI_CFG2_YCC_SWAP_MODE1_Val << ISI_CFG2_YCC_SWAP_Pos)  /**< (ISI_CFG2) Byte 0 Cr(i)Byte 1 Y(i)Byte 2 Cb(i)Byte 3 Y(i+1) Position  */
#define ISI_CFG2_YCC_SWAP_MODE2             (ISI_CFG2_YCC_SWAP_MODE2_Val << ISI_CFG2_YCC_SWAP_Pos)  /**< (ISI_CFG2) Byte 0 Y(i)Byte 1 Cb(i)Byte 2 Y(i+1)Byte 3 Cr(i) Position  */
#define ISI_CFG2_YCC_SWAP_MODE3             (ISI_CFG2_YCC_SWAP_MODE3_Val << ISI_CFG2_YCC_SWAP_Pos)  /**< (ISI_CFG2) Byte 0 Y(i)Byte 1 Cr(i)Byte 2 Y(i+1)Byte 3 Cb(i) Position  */
#define ISI_CFG2_RGB_CFG_Pos                30                                             /**< (ISI_CFG2) RGB Pixel Mapping Configuration Position */
#define ISI_CFG2_RGB_CFG_Msk                (0x3U << ISI_CFG2_RGB_CFG_Pos)                 /**< (ISI_CFG2) RGB Pixel Mapping Configuration Mask */
#define ISI_CFG2_RGB_CFG(value)             (ISI_CFG2_RGB_CFG_Msk & ((value) << ISI_CFG2_RGB_CFG_Pos))
#define   ISI_CFG2_RGB_CFG_DEFAULT_Val      (0x0U)                                         /**< (ISI_CFG2) Byte 0 R/G(MSB)Byte 1 G(LSB)/BByte 2 R/G(MSB)Byte 3 G(LSB)/B  */
#define   ISI_CFG2_RGB_CFG_MODE1_Val        (0x1U)                                         /**< (ISI_CFG2) Byte 0 B/G(MSB)Byte 1 G(LSB)/RByte 2 B/G(MSB)Byte 3 G(LSB)/R  */
#define   ISI_CFG2_RGB_CFG_MODE2_Val        (0x2U)                                         /**< (ISI_CFG2) Byte 0 G(LSB)/RByte 1 B/G(MSB)Byte 2 G(LSB)/RByte 3 B/G(MSB)  */
#define   ISI_CFG2_RGB_CFG_MODE3_Val        (0x3U)                                         /**< (ISI_CFG2) Byte 0 G(LSB)/BByte 1 R/G(MSB)Byte 2 G(LSB)/BByte 3 R/G(MSB)  */
#define ISI_CFG2_RGB_CFG_DEFAULT            (ISI_CFG2_RGB_CFG_DEFAULT_Val << ISI_CFG2_RGB_CFG_Pos)  /**< (ISI_CFG2) Byte 0 R/G(MSB)Byte 1 G(LSB)/BByte 2 R/G(MSB)Byte 3 G(LSB)/B Position  */
#define ISI_CFG2_RGB_CFG_MODE1              (ISI_CFG2_RGB_CFG_MODE1_Val << ISI_CFG2_RGB_CFG_Pos)  /**< (ISI_CFG2) Byte 0 B/G(MSB)Byte 1 G(LSB)/RByte 2 B/G(MSB)Byte 3 G(LSB)/R Position  */
#define ISI_CFG2_RGB_CFG_MODE2              (ISI_CFG2_RGB_CFG_MODE2_Val << ISI_CFG2_RGB_CFG_Pos)  /**< (ISI_CFG2) Byte 0 G(LSB)/RByte 1 B/G(MSB)Byte 2 G(LSB)/RByte 3 B/G(MSB) Position  */
#define ISI_CFG2_RGB_CFG_MODE3              (ISI_CFG2_RGB_CFG_MODE3_Val << ISI_CFG2_RGB_CFG_Pos)  /**< (ISI_CFG2) Byte 0 G(LSB)/BByte 1 R/G(MSB)Byte 2 G(LSB)/BByte 3 R/G(MSB) Position  */
#define ISI_CFG2_MASK                       (0xF7FFFFFFU)                                  /**< \deprecated (ISI_CFG2) Register MASK  (Use ISI_CFG2_Msk instead)  */
#define ISI_CFG2_Msk                        (0xF7FFFFFFU)                                  /**< (ISI_CFG2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_PSIZE : (ISI Offset: 0x08) (R/W 32) ISI Preview Size Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PREV_VSIZE:10;             /**< bit:   0..9  Vertical Size for the Preview Path       */
    uint32_t :6;                        /**< bit: 10..15  Reserved */
    uint32_t PREV_HSIZE:10;             /**< bit: 16..25  Horizontal Size for the Preview Path     */
    uint32_t :6;                        /**< bit: 26..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_PSIZE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_PSIZE_OFFSET                    (0x08)                                        /**<  (ISI_PSIZE) ISI Preview Size Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_PSIZE_PREV_VSIZE_Pos            0                                              /**< (ISI_PSIZE) Vertical Size for the Preview Path Position */
#define ISI_PSIZE_PREV_VSIZE_Msk            (0x3FFU << ISI_PSIZE_PREV_VSIZE_Pos)           /**< (ISI_PSIZE) Vertical Size for the Preview Path Mask */
#define ISI_PSIZE_PREV_VSIZE(value)         (ISI_PSIZE_PREV_VSIZE_Msk & ((value) << ISI_PSIZE_PREV_VSIZE_Pos))
#define ISI_PSIZE_PREV_HSIZE_Pos            16                                             /**< (ISI_PSIZE) Horizontal Size for the Preview Path Position */
#define ISI_PSIZE_PREV_HSIZE_Msk            (0x3FFU << ISI_PSIZE_PREV_HSIZE_Pos)           /**< (ISI_PSIZE) Horizontal Size for the Preview Path Mask */
#define ISI_PSIZE_PREV_HSIZE(value)         (ISI_PSIZE_PREV_HSIZE_Msk & ((value) << ISI_PSIZE_PREV_HSIZE_Pos))
#define ISI_PSIZE_MASK                      (0x3FF03FFU)                                   /**< \deprecated (ISI_PSIZE) Register MASK  (Use ISI_PSIZE_Msk instead)  */
#define ISI_PSIZE_Msk                       (0x3FF03FFU)                                   /**< (ISI_PSIZE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_PDECF : (ISI Offset: 0x0c) (R/W 32) ISI Preview Decimation Factor Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DEC_FACTOR:8;              /**< bit:   0..7  Decimation Factor                        */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_PDECF_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_PDECF_OFFSET                    (0x0C)                                        /**<  (ISI_PDECF) ISI Preview Decimation Factor Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_PDECF_DEC_FACTOR_Pos            0                                              /**< (ISI_PDECF) Decimation Factor Position */
#define ISI_PDECF_DEC_FACTOR_Msk            (0xFFU << ISI_PDECF_DEC_FACTOR_Pos)            /**< (ISI_PDECF) Decimation Factor Mask */
#define ISI_PDECF_DEC_FACTOR(value)         (ISI_PDECF_DEC_FACTOR_Msk & ((value) << ISI_PDECF_DEC_FACTOR_Pos))
#define ISI_PDECF_MASK                      (0xFFU)                                        /**< \deprecated (ISI_PDECF) Register MASK  (Use ISI_PDECF_Msk instead)  */
#define ISI_PDECF_Msk                       (0xFFU)                                        /**< (ISI_PDECF) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_Y2R_SET0 : (ISI Offset: 0x10) (R/W 32) ISI Color Space Conversion YCrCb To RGB Set 0 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t C0:8;                      /**< bit:   0..7  Color Space Conversion Matrix Coefficient C0 */
    uint32_t C1:8;                      /**< bit:  8..15  Color Space Conversion Matrix Coefficient C1 */
    uint32_t C2:8;                      /**< bit: 16..23  Color Space Conversion Matrix Coefficient C2 */
    uint32_t C3:8;                      /**< bit: 24..31  Color Space Conversion Matrix Coefficient C3 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_Y2R_SET0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_Y2R_SET0_OFFSET                 (0x10)                                        /**<  (ISI_Y2R_SET0) ISI Color Space Conversion YCrCb To RGB Set 0 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_Y2R_SET0_C0_Pos                 0                                              /**< (ISI_Y2R_SET0) Color Space Conversion Matrix Coefficient C0 Position */
#define ISI_Y2R_SET0_C0_Msk                 (0xFFU << ISI_Y2R_SET0_C0_Pos)                 /**< (ISI_Y2R_SET0) Color Space Conversion Matrix Coefficient C0 Mask */
#define ISI_Y2R_SET0_C0(value)              (ISI_Y2R_SET0_C0_Msk & ((value) << ISI_Y2R_SET0_C0_Pos))
#define ISI_Y2R_SET0_C1_Pos                 8                                              /**< (ISI_Y2R_SET0) Color Space Conversion Matrix Coefficient C1 Position */
#define ISI_Y2R_SET0_C1_Msk                 (0xFFU << ISI_Y2R_SET0_C1_Pos)                 /**< (ISI_Y2R_SET0) Color Space Conversion Matrix Coefficient C1 Mask */
#define ISI_Y2R_SET0_C1(value)              (ISI_Y2R_SET0_C1_Msk & ((value) << ISI_Y2R_SET0_C1_Pos))
#define ISI_Y2R_SET0_C2_Pos                 16                                             /**< (ISI_Y2R_SET0) Color Space Conversion Matrix Coefficient C2 Position */
#define ISI_Y2R_SET0_C2_Msk                 (0xFFU << ISI_Y2R_SET0_C2_Pos)                 /**< (ISI_Y2R_SET0) Color Space Conversion Matrix Coefficient C2 Mask */
#define ISI_Y2R_SET0_C2(value)              (ISI_Y2R_SET0_C2_Msk & ((value) << ISI_Y2R_SET0_C2_Pos))
#define ISI_Y2R_SET0_C3_Pos                 24                                             /**< (ISI_Y2R_SET0) Color Space Conversion Matrix Coefficient C3 Position */
#define ISI_Y2R_SET0_C3_Msk                 (0xFFU << ISI_Y2R_SET0_C3_Pos)                 /**< (ISI_Y2R_SET0) Color Space Conversion Matrix Coefficient C3 Mask */
#define ISI_Y2R_SET0_C3(value)              (ISI_Y2R_SET0_C3_Msk & ((value) << ISI_Y2R_SET0_C3_Pos))
#define ISI_Y2R_SET0_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (ISI_Y2R_SET0) Register MASK  (Use ISI_Y2R_SET0_Msk instead)  */
#define ISI_Y2R_SET0_Msk                    (0xFFFFFFFFU)                                  /**< (ISI_Y2R_SET0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_Y2R_SET1 : (ISI Offset: 0x14) (R/W 32) ISI Color Space Conversion YCrCb To RGB Set 1 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t C4:9;                      /**< bit:   0..8  Color Space Conversion Matrix Coefficient C4 */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t Yoff:1;                    /**< bit:     12  Color Space Conversion Luminance Default Offset */
    uint32_t Croff:1;                   /**< bit:     13  Color Space Conversion Red Chrominance Default Offset */
    uint32_t Cboff:1;                   /**< bit:     14  Color Space Conversion Blue Chrominance Default Offset */
    uint32_t :17;                       /**< bit: 15..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_Y2R_SET1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_Y2R_SET1_OFFSET                 (0x14)                                        /**<  (ISI_Y2R_SET1) ISI Color Space Conversion YCrCb To RGB Set 1 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_Y2R_SET1_C4_Pos                 0                                              /**< (ISI_Y2R_SET1) Color Space Conversion Matrix Coefficient C4 Position */
#define ISI_Y2R_SET1_C4_Msk                 (0x1FFU << ISI_Y2R_SET1_C4_Pos)                /**< (ISI_Y2R_SET1) Color Space Conversion Matrix Coefficient C4 Mask */
#define ISI_Y2R_SET1_C4(value)              (ISI_Y2R_SET1_C4_Msk & ((value) << ISI_Y2R_SET1_C4_Pos))
#define ISI_Y2R_SET1_Yoff_Pos               12                                             /**< (ISI_Y2R_SET1) Color Space Conversion Luminance Default Offset Position */
#define ISI_Y2R_SET1_Yoff_Msk               (0x1U << ISI_Y2R_SET1_Yoff_Pos)                /**< (ISI_Y2R_SET1) Color Space Conversion Luminance Default Offset Mask */
#define ISI_Y2R_SET1_Yoff                   ISI_Y2R_SET1_Yoff_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_Y2R_SET1_Yoff_Msk instead */
#define ISI_Y2R_SET1_Croff_Pos              13                                             /**< (ISI_Y2R_SET1) Color Space Conversion Red Chrominance Default Offset Position */
#define ISI_Y2R_SET1_Croff_Msk              (0x1U << ISI_Y2R_SET1_Croff_Pos)               /**< (ISI_Y2R_SET1) Color Space Conversion Red Chrominance Default Offset Mask */
#define ISI_Y2R_SET1_Croff                  ISI_Y2R_SET1_Croff_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_Y2R_SET1_Croff_Msk instead */
#define ISI_Y2R_SET1_Cboff_Pos              14                                             /**< (ISI_Y2R_SET1) Color Space Conversion Blue Chrominance Default Offset Position */
#define ISI_Y2R_SET1_Cboff_Msk              (0x1U << ISI_Y2R_SET1_Cboff_Pos)               /**< (ISI_Y2R_SET1) Color Space Conversion Blue Chrominance Default Offset Mask */
#define ISI_Y2R_SET1_Cboff                  ISI_Y2R_SET1_Cboff_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_Y2R_SET1_Cboff_Msk instead */
#define ISI_Y2R_SET1_MASK                   (0x71FFU)                                      /**< \deprecated (ISI_Y2R_SET1) Register MASK  (Use ISI_Y2R_SET1_Msk instead)  */
#define ISI_Y2R_SET1_Msk                    (0x71FFU)                                      /**< (ISI_Y2R_SET1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_R2Y_SET0 : (ISI Offset: 0x18) (R/W 32) ISI Color Space Conversion RGB To YCrCb Set 0 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t C0:7;                      /**< bit:   0..6  Color Space Conversion Matrix Coefficient C0 */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t C1:7;                      /**< bit:  8..14  Color Space Conversion Matrix Coefficient C1 */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t C2:7;                      /**< bit: 16..22  Color Space Conversion Matrix Coefficient C2 */
    uint32_t :1;                        /**< bit:     23  Reserved */
    uint32_t Roff:1;                    /**< bit:     24  Color Space Conversion Red Component Offset */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_R2Y_SET0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_R2Y_SET0_OFFSET                 (0x18)                                        /**<  (ISI_R2Y_SET0) ISI Color Space Conversion RGB To YCrCb Set 0 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_R2Y_SET0_C0_Pos                 0                                              /**< (ISI_R2Y_SET0) Color Space Conversion Matrix Coefficient C0 Position */
#define ISI_R2Y_SET0_C0_Msk                 (0x7FU << ISI_R2Y_SET0_C0_Pos)                 /**< (ISI_R2Y_SET0) Color Space Conversion Matrix Coefficient C0 Mask */
#define ISI_R2Y_SET0_C0(value)              (ISI_R2Y_SET0_C0_Msk & ((value) << ISI_R2Y_SET0_C0_Pos))
#define ISI_R2Y_SET0_C1_Pos                 8                                              /**< (ISI_R2Y_SET0) Color Space Conversion Matrix Coefficient C1 Position */
#define ISI_R2Y_SET0_C1_Msk                 (0x7FU << ISI_R2Y_SET0_C1_Pos)                 /**< (ISI_R2Y_SET0) Color Space Conversion Matrix Coefficient C1 Mask */
#define ISI_R2Y_SET0_C1(value)              (ISI_R2Y_SET0_C1_Msk & ((value) << ISI_R2Y_SET0_C1_Pos))
#define ISI_R2Y_SET0_C2_Pos                 16                                             /**< (ISI_R2Y_SET0) Color Space Conversion Matrix Coefficient C2 Position */
#define ISI_R2Y_SET0_C2_Msk                 (0x7FU << ISI_R2Y_SET0_C2_Pos)                 /**< (ISI_R2Y_SET0) Color Space Conversion Matrix Coefficient C2 Mask */
#define ISI_R2Y_SET0_C2(value)              (ISI_R2Y_SET0_C2_Msk & ((value) << ISI_R2Y_SET0_C2_Pos))
#define ISI_R2Y_SET0_Roff_Pos               24                                             /**< (ISI_R2Y_SET0) Color Space Conversion Red Component Offset Position */
#define ISI_R2Y_SET0_Roff_Msk               (0x1U << ISI_R2Y_SET0_Roff_Pos)                /**< (ISI_R2Y_SET0) Color Space Conversion Red Component Offset Mask */
#define ISI_R2Y_SET0_Roff                   ISI_R2Y_SET0_Roff_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_R2Y_SET0_Roff_Msk instead */
#define ISI_R2Y_SET0_MASK                   (0x17F7F7FU)                                   /**< \deprecated (ISI_R2Y_SET0) Register MASK  (Use ISI_R2Y_SET0_Msk instead)  */
#define ISI_R2Y_SET0_Msk                    (0x17F7F7FU)                                   /**< (ISI_R2Y_SET0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_R2Y_SET1 : (ISI Offset: 0x1c) (R/W 32) ISI Color Space Conversion RGB To YCrCb Set 1 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t C3:7;                      /**< bit:   0..6  Color Space Conversion Matrix Coefficient C3 */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t C4:7;                      /**< bit:  8..14  Color Space Conversion Matrix Coefficient C4 */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t C5:7;                      /**< bit: 16..22  Color Space Conversion Matrix Coefficient C5 */
    uint32_t :1;                        /**< bit:     23  Reserved */
    uint32_t Goff:1;                    /**< bit:     24  Color Space Conversion Green Component Offset */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_R2Y_SET1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_R2Y_SET1_OFFSET                 (0x1C)                                        /**<  (ISI_R2Y_SET1) ISI Color Space Conversion RGB To YCrCb Set 1 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_R2Y_SET1_C3_Pos                 0                                              /**< (ISI_R2Y_SET1) Color Space Conversion Matrix Coefficient C3 Position */
#define ISI_R2Y_SET1_C3_Msk                 (0x7FU << ISI_R2Y_SET1_C3_Pos)                 /**< (ISI_R2Y_SET1) Color Space Conversion Matrix Coefficient C3 Mask */
#define ISI_R2Y_SET1_C3(value)              (ISI_R2Y_SET1_C3_Msk & ((value) << ISI_R2Y_SET1_C3_Pos))
#define ISI_R2Y_SET1_C4_Pos                 8                                              /**< (ISI_R2Y_SET1) Color Space Conversion Matrix Coefficient C4 Position */
#define ISI_R2Y_SET1_C4_Msk                 (0x7FU << ISI_R2Y_SET1_C4_Pos)                 /**< (ISI_R2Y_SET1) Color Space Conversion Matrix Coefficient C4 Mask */
#define ISI_R2Y_SET1_C4(value)              (ISI_R2Y_SET1_C4_Msk & ((value) << ISI_R2Y_SET1_C4_Pos))
#define ISI_R2Y_SET1_C5_Pos                 16                                             /**< (ISI_R2Y_SET1) Color Space Conversion Matrix Coefficient C5 Position */
#define ISI_R2Y_SET1_C5_Msk                 (0x7FU << ISI_R2Y_SET1_C5_Pos)                 /**< (ISI_R2Y_SET1) Color Space Conversion Matrix Coefficient C5 Mask */
#define ISI_R2Y_SET1_C5(value)              (ISI_R2Y_SET1_C5_Msk & ((value) << ISI_R2Y_SET1_C5_Pos))
#define ISI_R2Y_SET1_Goff_Pos               24                                             /**< (ISI_R2Y_SET1) Color Space Conversion Green Component Offset Position */
#define ISI_R2Y_SET1_Goff_Msk               (0x1U << ISI_R2Y_SET1_Goff_Pos)                /**< (ISI_R2Y_SET1) Color Space Conversion Green Component Offset Mask */
#define ISI_R2Y_SET1_Goff                   ISI_R2Y_SET1_Goff_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_R2Y_SET1_Goff_Msk instead */
#define ISI_R2Y_SET1_MASK                   (0x17F7F7FU)                                   /**< \deprecated (ISI_R2Y_SET1) Register MASK  (Use ISI_R2Y_SET1_Msk instead)  */
#define ISI_R2Y_SET1_Msk                    (0x17F7F7FU)                                   /**< (ISI_R2Y_SET1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_R2Y_SET2 : (ISI Offset: 0x20) (R/W 32) ISI Color Space Conversion RGB To YCrCb Set 2 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t C6:7;                      /**< bit:   0..6  Color Space Conversion Matrix Coefficient C6 */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t C7:7;                      /**< bit:  8..14  Color Space Conversion Matrix Coefficient C7 */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t C8:7;                      /**< bit: 16..22  Color Space Conversion Matrix Coefficient C8 */
    uint32_t :1;                        /**< bit:     23  Reserved */
    uint32_t Boff:1;                    /**< bit:     24  Color Space Conversion Blue Component Offset */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_R2Y_SET2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_R2Y_SET2_OFFSET                 (0x20)                                        /**<  (ISI_R2Y_SET2) ISI Color Space Conversion RGB To YCrCb Set 2 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_R2Y_SET2_C6_Pos                 0                                              /**< (ISI_R2Y_SET2) Color Space Conversion Matrix Coefficient C6 Position */
#define ISI_R2Y_SET2_C6_Msk                 (0x7FU << ISI_R2Y_SET2_C6_Pos)                 /**< (ISI_R2Y_SET2) Color Space Conversion Matrix Coefficient C6 Mask */
#define ISI_R2Y_SET2_C6(value)              (ISI_R2Y_SET2_C6_Msk & ((value) << ISI_R2Y_SET2_C6_Pos))
#define ISI_R2Y_SET2_C7_Pos                 8                                              /**< (ISI_R2Y_SET2) Color Space Conversion Matrix Coefficient C7 Position */
#define ISI_R2Y_SET2_C7_Msk                 (0x7FU << ISI_R2Y_SET2_C7_Pos)                 /**< (ISI_R2Y_SET2) Color Space Conversion Matrix Coefficient C7 Mask */
#define ISI_R2Y_SET2_C7(value)              (ISI_R2Y_SET2_C7_Msk & ((value) << ISI_R2Y_SET2_C7_Pos))
#define ISI_R2Y_SET2_C8_Pos                 16                                             /**< (ISI_R2Y_SET2) Color Space Conversion Matrix Coefficient C8 Position */
#define ISI_R2Y_SET2_C8_Msk                 (0x7FU << ISI_R2Y_SET2_C8_Pos)                 /**< (ISI_R2Y_SET2) Color Space Conversion Matrix Coefficient C8 Mask */
#define ISI_R2Y_SET2_C8(value)              (ISI_R2Y_SET2_C8_Msk & ((value) << ISI_R2Y_SET2_C8_Pos))
#define ISI_R2Y_SET2_Boff_Pos               24                                             /**< (ISI_R2Y_SET2) Color Space Conversion Blue Component Offset Position */
#define ISI_R2Y_SET2_Boff_Msk               (0x1U << ISI_R2Y_SET2_Boff_Pos)                /**< (ISI_R2Y_SET2) Color Space Conversion Blue Component Offset Mask */
#define ISI_R2Y_SET2_Boff                   ISI_R2Y_SET2_Boff_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_R2Y_SET2_Boff_Msk instead */
#define ISI_R2Y_SET2_MASK                   (0x17F7F7FU)                                   /**< \deprecated (ISI_R2Y_SET2) Register MASK  (Use ISI_R2Y_SET2_Msk instead)  */
#define ISI_R2Y_SET2_Msk                    (0x17F7F7FU)                                   /**< (ISI_R2Y_SET2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_CR : (ISI Offset: 0x24) (/W 32) ISI Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ISI_EN:1;                  /**< bit:      0  ISI Module Enable Request                */
    uint32_t ISI_DIS:1;                 /**< bit:      1  ISI Module Disable Request               */
    uint32_t ISI_SRST:1;                /**< bit:      2  ISI Software Reset Request               */
    uint32_t :5;                        /**< bit:   3..7  Reserved */
    uint32_t ISI_CDC:1;                 /**< bit:      8  ISI Codec Request                        */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_CR_OFFSET                       (0x24)                                        /**<  (ISI_CR) ISI Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_CR_ISI_EN_Pos                   0                                              /**< (ISI_CR) ISI Module Enable Request Position */
#define ISI_CR_ISI_EN_Msk                   (0x1U << ISI_CR_ISI_EN_Pos)                    /**< (ISI_CR) ISI Module Enable Request Mask */
#define ISI_CR_ISI_EN                       ISI_CR_ISI_EN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CR_ISI_EN_Msk instead */
#define ISI_CR_ISI_DIS_Pos                  1                                              /**< (ISI_CR) ISI Module Disable Request Position */
#define ISI_CR_ISI_DIS_Msk                  (0x1U << ISI_CR_ISI_DIS_Pos)                   /**< (ISI_CR) ISI Module Disable Request Mask */
#define ISI_CR_ISI_DIS                      ISI_CR_ISI_DIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CR_ISI_DIS_Msk instead */
#define ISI_CR_ISI_SRST_Pos                 2                                              /**< (ISI_CR) ISI Software Reset Request Position */
#define ISI_CR_ISI_SRST_Msk                 (0x1U << ISI_CR_ISI_SRST_Pos)                  /**< (ISI_CR) ISI Software Reset Request Mask */
#define ISI_CR_ISI_SRST                     ISI_CR_ISI_SRST_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CR_ISI_SRST_Msk instead */
#define ISI_CR_ISI_CDC_Pos                  8                                              /**< (ISI_CR) ISI Codec Request Position */
#define ISI_CR_ISI_CDC_Msk                  (0x1U << ISI_CR_ISI_CDC_Pos)                   /**< (ISI_CR) ISI Codec Request Mask */
#define ISI_CR_ISI_CDC                      ISI_CR_ISI_CDC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_CR_ISI_CDC_Msk instead */
#define ISI_CR_MASK                         (0x107U)                                       /**< \deprecated (ISI_CR) Register MASK  (Use ISI_CR_Msk instead)  */
#define ISI_CR_Msk                          (0x107U)                                       /**< (ISI_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_SR : (ISI Offset: 0x28) (R/ 32) ISI Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ENABLE:1;                  /**< bit:      0  Module Enable                            */
    uint32_t DIS_DONE:1;                /**< bit:      1  Module Disable Request has Terminated (cleared on read) */
    uint32_t SRST:1;                    /**< bit:      2  Module Software Reset Request has Terminated (cleared on read) */
    uint32_t :5;                        /**< bit:   3..7  Reserved */
    uint32_t CDC_PND:1;                 /**< bit:      8  Pending Codec Request                    */
    uint32_t :1;                        /**< bit:      9  Reserved */
    uint32_t VSYNC:1;                   /**< bit:     10  Vertical Synchronization (cleared on read) */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t PXFR_DONE:1;               /**< bit:     16  Preview DMA Transfer has Terminated (cleared on read) */
    uint32_t CXFR_DONE:1;               /**< bit:     17  Codec DMA Transfer has Terminated (cleared on read) */
    uint32_t :1;                        /**< bit:     18  Reserved */
    uint32_t SIP:1;                     /**< bit:     19  Synchronization in Progress              */
    uint32_t :4;                        /**< bit: 20..23  Reserved */
    uint32_t P_OVR:1;                   /**< bit:     24  Preview Datapath Overflow (cleared on read) */
    uint32_t C_OVR:1;                   /**< bit:     25  Codec Datapath Overflow (cleared on read) */
    uint32_t CRC_ERR:1;                 /**< bit:     26  CRC Synchronization Error (cleared on read) */
    uint32_t FR_OVR:1;                  /**< bit:     27  Frame Rate Overrun (cleared on read)     */
    uint32_t :4;                        /**< bit: 28..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_SR_OFFSET                       (0x28)                                        /**<  (ISI_SR) ISI Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_SR_ENABLE_Pos                   0                                              /**< (ISI_SR) Module Enable Position */
#define ISI_SR_ENABLE_Msk                   (0x1U << ISI_SR_ENABLE_Pos)                    /**< (ISI_SR) Module Enable Mask */
#define ISI_SR_ENABLE                       ISI_SR_ENABLE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_ENABLE_Msk instead */
#define ISI_SR_DIS_DONE_Pos                 1                                              /**< (ISI_SR) Module Disable Request has Terminated (cleared on read) Position */
#define ISI_SR_DIS_DONE_Msk                 (0x1U << ISI_SR_DIS_DONE_Pos)                  /**< (ISI_SR) Module Disable Request has Terminated (cleared on read) Mask */
#define ISI_SR_DIS_DONE                     ISI_SR_DIS_DONE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_DIS_DONE_Msk instead */
#define ISI_SR_SRST_Pos                     2                                              /**< (ISI_SR) Module Software Reset Request has Terminated (cleared on read) Position */
#define ISI_SR_SRST_Msk                     (0x1U << ISI_SR_SRST_Pos)                      /**< (ISI_SR) Module Software Reset Request has Terminated (cleared on read) Mask */
#define ISI_SR_SRST                         ISI_SR_SRST_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_SRST_Msk instead */
#define ISI_SR_CDC_PND_Pos                  8                                              /**< (ISI_SR) Pending Codec Request Position */
#define ISI_SR_CDC_PND_Msk                  (0x1U << ISI_SR_CDC_PND_Pos)                   /**< (ISI_SR) Pending Codec Request Mask */
#define ISI_SR_CDC_PND                      ISI_SR_CDC_PND_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_CDC_PND_Msk instead */
#define ISI_SR_VSYNC_Pos                    10                                             /**< (ISI_SR) Vertical Synchronization (cleared on read) Position */
#define ISI_SR_VSYNC_Msk                    (0x1U << ISI_SR_VSYNC_Pos)                     /**< (ISI_SR) Vertical Synchronization (cleared on read) Mask */
#define ISI_SR_VSYNC                        ISI_SR_VSYNC_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_VSYNC_Msk instead */
#define ISI_SR_PXFR_DONE_Pos                16                                             /**< (ISI_SR) Preview DMA Transfer has Terminated (cleared on read) Position */
#define ISI_SR_PXFR_DONE_Msk                (0x1U << ISI_SR_PXFR_DONE_Pos)                 /**< (ISI_SR) Preview DMA Transfer has Terminated (cleared on read) Mask */
#define ISI_SR_PXFR_DONE                    ISI_SR_PXFR_DONE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_PXFR_DONE_Msk instead */
#define ISI_SR_CXFR_DONE_Pos                17                                             /**< (ISI_SR) Codec DMA Transfer has Terminated (cleared on read) Position */
#define ISI_SR_CXFR_DONE_Msk                (0x1U << ISI_SR_CXFR_DONE_Pos)                 /**< (ISI_SR) Codec DMA Transfer has Terminated (cleared on read) Mask */
#define ISI_SR_CXFR_DONE                    ISI_SR_CXFR_DONE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_CXFR_DONE_Msk instead */
#define ISI_SR_SIP_Pos                      19                                             /**< (ISI_SR) Synchronization in Progress Position */
#define ISI_SR_SIP_Msk                      (0x1U << ISI_SR_SIP_Pos)                       /**< (ISI_SR) Synchronization in Progress Mask */
#define ISI_SR_SIP                          ISI_SR_SIP_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_SIP_Msk instead */
#define ISI_SR_P_OVR_Pos                    24                                             /**< (ISI_SR) Preview Datapath Overflow (cleared on read) Position */
#define ISI_SR_P_OVR_Msk                    (0x1U << ISI_SR_P_OVR_Pos)                     /**< (ISI_SR) Preview Datapath Overflow (cleared on read) Mask */
#define ISI_SR_P_OVR                        ISI_SR_P_OVR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_P_OVR_Msk instead */
#define ISI_SR_C_OVR_Pos                    25                                             /**< (ISI_SR) Codec Datapath Overflow (cleared on read) Position */
#define ISI_SR_C_OVR_Msk                    (0x1U << ISI_SR_C_OVR_Pos)                     /**< (ISI_SR) Codec Datapath Overflow (cleared on read) Mask */
#define ISI_SR_C_OVR                        ISI_SR_C_OVR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_C_OVR_Msk instead */
#define ISI_SR_CRC_ERR_Pos                  26                                             /**< (ISI_SR) CRC Synchronization Error (cleared on read) Position */
#define ISI_SR_CRC_ERR_Msk                  (0x1U << ISI_SR_CRC_ERR_Pos)                   /**< (ISI_SR) CRC Synchronization Error (cleared on read) Mask */
#define ISI_SR_CRC_ERR                      ISI_SR_CRC_ERR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_CRC_ERR_Msk instead */
#define ISI_SR_FR_OVR_Pos                   27                                             /**< (ISI_SR) Frame Rate Overrun (cleared on read) Position */
#define ISI_SR_FR_OVR_Msk                   (0x1U << ISI_SR_FR_OVR_Pos)                    /**< (ISI_SR) Frame Rate Overrun (cleared on read) Mask */
#define ISI_SR_FR_OVR                       ISI_SR_FR_OVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_SR_FR_OVR_Msk instead */
#define ISI_SR_MASK                         (0xF0B0507U)                                   /**< \deprecated (ISI_SR) Register MASK  (Use ISI_SR_Msk instead)  */
#define ISI_SR_Msk                          (0xF0B0507U)                                   /**< (ISI_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_IER : (ISI Offset: 0x2c) (/W 32) ISI Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t DIS_DONE:1;                /**< bit:      1  Disable Done Interrupt Enable            */
    uint32_t SRST:1;                    /**< bit:      2  Software Reset Interrupt Enable          */
    uint32_t :7;                        /**< bit:   3..9  Reserved */
    uint32_t VSYNC:1;                   /**< bit:     10  Vertical Synchronization Interrupt Enable */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t PXFR_DONE:1;               /**< bit:     16  Preview DMA Transfer Done Interrupt Enable */
    uint32_t CXFR_DONE:1;               /**< bit:     17  Codec DMA Transfer Done Interrupt Enable */
    uint32_t :6;                        /**< bit: 18..23  Reserved */
    uint32_t P_OVR:1;                   /**< bit:     24  Preview Datapath Overflow Interrupt Enable */
    uint32_t C_OVR:1;                   /**< bit:     25  Codec Datapath Overflow Interrupt Enable */
    uint32_t CRC_ERR:1;                 /**< bit:     26  Embedded Synchronization CRC Error Interrupt Enable */
    uint32_t FR_OVR:1;                  /**< bit:     27  Frame Rate Overflow Interrupt Enable     */
    uint32_t :4;                        /**< bit: 28..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_IER_OFFSET                      (0x2C)                                        /**<  (ISI_IER) ISI Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_IER_DIS_DONE_Pos                1                                              /**< (ISI_IER) Disable Done Interrupt Enable Position */
#define ISI_IER_DIS_DONE_Msk                (0x1U << ISI_IER_DIS_DONE_Pos)                 /**< (ISI_IER) Disable Done Interrupt Enable Mask */
#define ISI_IER_DIS_DONE                    ISI_IER_DIS_DONE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_DIS_DONE_Msk instead */
#define ISI_IER_SRST_Pos                    2                                              /**< (ISI_IER) Software Reset Interrupt Enable Position */
#define ISI_IER_SRST_Msk                    (0x1U << ISI_IER_SRST_Pos)                     /**< (ISI_IER) Software Reset Interrupt Enable Mask */
#define ISI_IER_SRST                        ISI_IER_SRST_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_SRST_Msk instead */
#define ISI_IER_VSYNC_Pos                   10                                             /**< (ISI_IER) Vertical Synchronization Interrupt Enable Position */
#define ISI_IER_VSYNC_Msk                   (0x1U << ISI_IER_VSYNC_Pos)                    /**< (ISI_IER) Vertical Synchronization Interrupt Enable Mask */
#define ISI_IER_VSYNC                       ISI_IER_VSYNC_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_VSYNC_Msk instead */
#define ISI_IER_PXFR_DONE_Pos               16                                             /**< (ISI_IER) Preview DMA Transfer Done Interrupt Enable Position */
#define ISI_IER_PXFR_DONE_Msk               (0x1U << ISI_IER_PXFR_DONE_Pos)                /**< (ISI_IER) Preview DMA Transfer Done Interrupt Enable Mask */
#define ISI_IER_PXFR_DONE                   ISI_IER_PXFR_DONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_PXFR_DONE_Msk instead */
#define ISI_IER_CXFR_DONE_Pos               17                                             /**< (ISI_IER) Codec DMA Transfer Done Interrupt Enable Position */
#define ISI_IER_CXFR_DONE_Msk               (0x1U << ISI_IER_CXFR_DONE_Pos)                /**< (ISI_IER) Codec DMA Transfer Done Interrupt Enable Mask */
#define ISI_IER_CXFR_DONE                   ISI_IER_CXFR_DONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_CXFR_DONE_Msk instead */
#define ISI_IER_P_OVR_Pos                   24                                             /**< (ISI_IER) Preview Datapath Overflow Interrupt Enable Position */
#define ISI_IER_P_OVR_Msk                   (0x1U << ISI_IER_P_OVR_Pos)                    /**< (ISI_IER) Preview Datapath Overflow Interrupt Enable Mask */
#define ISI_IER_P_OVR                       ISI_IER_P_OVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_P_OVR_Msk instead */
#define ISI_IER_C_OVR_Pos                   25                                             /**< (ISI_IER) Codec Datapath Overflow Interrupt Enable Position */
#define ISI_IER_C_OVR_Msk                   (0x1U << ISI_IER_C_OVR_Pos)                    /**< (ISI_IER) Codec Datapath Overflow Interrupt Enable Mask */
#define ISI_IER_C_OVR                       ISI_IER_C_OVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_C_OVR_Msk instead */
#define ISI_IER_CRC_ERR_Pos                 26                                             /**< (ISI_IER) Embedded Synchronization CRC Error Interrupt Enable Position */
#define ISI_IER_CRC_ERR_Msk                 (0x1U << ISI_IER_CRC_ERR_Pos)                  /**< (ISI_IER) Embedded Synchronization CRC Error Interrupt Enable Mask */
#define ISI_IER_CRC_ERR                     ISI_IER_CRC_ERR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_CRC_ERR_Msk instead */
#define ISI_IER_FR_OVR_Pos                  27                                             /**< (ISI_IER) Frame Rate Overflow Interrupt Enable Position */
#define ISI_IER_FR_OVR_Msk                  (0x1U << ISI_IER_FR_OVR_Pos)                   /**< (ISI_IER) Frame Rate Overflow Interrupt Enable Mask */
#define ISI_IER_FR_OVR                      ISI_IER_FR_OVR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IER_FR_OVR_Msk instead */
#define ISI_IER_MASK                        (0xF030406U)                                   /**< \deprecated (ISI_IER) Register MASK  (Use ISI_IER_Msk instead)  */
#define ISI_IER_Msk                         (0xF030406U)                                   /**< (ISI_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_IDR : (ISI Offset: 0x30) (/W 32) ISI Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t DIS_DONE:1;                /**< bit:      1  Disable Done Interrupt Disable           */
    uint32_t SRST:1;                    /**< bit:      2  Software Reset Interrupt Disable         */
    uint32_t :7;                        /**< bit:   3..9  Reserved */
    uint32_t VSYNC:1;                   /**< bit:     10  Vertical Synchronization Interrupt Disable */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t PXFR_DONE:1;               /**< bit:     16  Preview DMA Transfer Done Interrupt Disable */
    uint32_t CXFR_DONE:1;               /**< bit:     17  Codec DMA Transfer Done Interrupt Disable */
    uint32_t :6;                        /**< bit: 18..23  Reserved */
    uint32_t P_OVR:1;                   /**< bit:     24  Preview Datapath Overflow Interrupt Disable */
    uint32_t C_OVR:1;                   /**< bit:     25  Codec Datapath Overflow Interrupt Disable */
    uint32_t CRC_ERR:1;                 /**< bit:     26  Embedded Synchronization CRC Error Interrupt Disable */
    uint32_t FR_OVR:1;                  /**< bit:     27  Frame Rate Overflow Interrupt Disable    */
    uint32_t :4;                        /**< bit: 28..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_IDR_OFFSET                      (0x30)                                        /**<  (ISI_IDR) ISI Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_IDR_DIS_DONE_Pos                1                                              /**< (ISI_IDR) Disable Done Interrupt Disable Position */
#define ISI_IDR_DIS_DONE_Msk                (0x1U << ISI_IDR_DIS_DONE_Pos)                 /**< (ISI_IDR) Disable Done Interrupt Disable Mask */
#define ISI_IDR_DIS_DONE                    ISI_IDR_DIS_DONE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_DIS_DONE_Msk instead */
#define ISI_IDR_SRST_Pos                    2                                              /**< (ISI_IDR) Software Reset Interrupt Disable Position */
#define ISI_IDR_SRST_Msk                    (0x1U << ISI_IDR_SRST_Pos)                     /**< (ISI_IDR) Software Reset Interrupt Disable Mask */
#define ISI_IDR_SRST                        ISI_IDR_SRST_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_SRST_Msk instead */
#define ISI_IDR_VSYNC_Pos                   10                                             /**< (ISI_IDR) Vertical Synchronization Interrupt Disable Position */
#define ISI_IDR_VSYNC_Msk                   (0x1U << ISI_IDR_VSYNC_Pos)                    /**< (ISI_IDR) Vertical Synchronization Interrupt Disable Mask */
#define ISI_IDR_VSYNC                       ISI_IDR_VSYNC_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_VSYNC_Msk instead */
#define ISI_IDR_PXFR_DONE_Pos               16                                             /**< (ISI_IDR) Preview DMA Transfer Done Interrupt Disable Position */
#define ISI_IDR_PXFR_DONE_Msk               (0x1U << ISI_IDR_PXFR_DONE_Pos)                /**< (ISI_IDR) Preview DMA Transfer Done Interrupt Disable Mask */
#define ISI_IDR_PXFR_DONE                   ISI_IDR_PXFR_DONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_PXFR_DONE_Msk instead */
#define ISI_IDR_CXFR_DONE_Pos               17                                             /**< (ISI_IDR) Codec DMA Transfer Done Interrupt Disable Position */
#define ISI_IDR_CXFR_DONE_Msk               (0x1U << ISI_IDR_CXFR_DONE_Pos)                /**< (ISI_IDR) Codec DMA Transfer Done Interrupt Disable Mask */
#define ISI_IDR_CXFR_DONE                   ISI_IDR_CXFR_DONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_CXFR_DONE_Msk instead */
#define ISI_IDR_P_OVR_Pos                   24                                             /**< (ISI_IDR) Preview Datapath Overflow Interrupt Disable Position */
#define ISI_IDR_P_OVR_Msk                   (0x1U << ISI_IDR_P_OVR_Pos)                    /**< (ISI_IDR) Preview Datapath Overflow Interrupt Disable Mask */
#define ISI_IDR_P_OVR                       ISI_IDR_P_OVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_P_OVR_Msk instead */
#define ISI_IDR_C_OVR_Pos                   25                                             /**< (ISI_IDR) Codec Datapath Overflow Interrupt Disable Position */
#define ISI_IDR_C_OVR_Msk                   (0x1U << ISI_IDR_C_OVR_Pos)                    /**< (ISI_IDR) Codec Datapath Overflow Interrupt Disable Mask */
#define ISI_IDR_C_OVR                       ISI_IDR_C_OVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_C_OVR_Msk instead */
#define ISI_IDR_CRC_ERR_Pos                 26                                             /**< (ISI_IDR) Embedded Synchronization CRC Error Interrupt Disable Position */
#define ISI_IDR_CRC_ERR_Msk                 (0x1U << ISI_IDR_CRC_ERR_Pos)                  /**< (ISI_IDR) Embedded Synchronization CRC Error Interrupt Disable Mask */
#define ISI_IDR_CRC_ERR                     ISI_IDR_CRC_ERR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_CRC_ERR_Msk instead */
#define ISI_IDR_FR_OVR_Pos                  27                                             /**< (ISI_IDR) Frame Rate Overflow Interrupt Disable Position */
#define ISI_IDR_FR_OVR_Msk                  (0x1U << ISI_IDR_FR_OVR_Pos)                   /**< (ISI_IDR) Frame Rate Overflow Interrupt Disable Mask */
#define ISI_IDR_FR_OVR                      ISI_IDR_FR_OVR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IDR_FR_OVR_Msk instead */
#define ISI_IDR_MASK                        (0xF030406U)                                   /**< \deprecated (ISI_IDR) Register MASK  (Use ISI_IDR_Msk instead)  */
#define ISI_IDR_Msk                         (0xF030406U)                                   /**< (ISI_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_IMR : (ISI Offset: 0x34) (R/ 32) ISI Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t DIS_DONE:1;                /**< bit:      1  Module Disable Operation Completed       */
    uint32_t SRST:1;                    /**< bit:      2  Software Reset Completed                 */
    uint32_t :7;                        /**< bit:   3..9  Reserved */
    uint32_t VSYNC:1;                   /**< bit:     10  Vertical Synchronization                 */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t PXFR_DONE:1;               /**< bit:     16  Preview DMA Transfer Completed           */
    uint32_t CXFR_DONE:1;               /**< bit:     17  Codec DMA Transfer Completed             */
    uint32_t :6;                        /**< bit: 18..23  Reserved */
    uint32_t P_OVR:1;                   /**< bit:     24  Preview FIFO Overflow                    */
    uint32_t C_OVR:1;                   /**< bit:     25  Codec FIFO Overflow                      */
    uint32_t CRC_ERR:1;                 /**< bit:     26  CRC Synchronization Error                */
    uint32_t FR_OVR:1;                  /**< bit:     27  Frame Rate Overrun                       */
    uint32_t :4;                        /**< bit: 28..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_IMR_OFFSET                      (0x34)                                        /**<  (ISI_IMR) ISI Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_IMR_DIS_DONE_Pos                1                                              /**< (ISI_IMR) Module Disable Operation Completed Position */
#define ISI_IMR_DIS_DONE_Msk                (0x1U << ISI_IMR_DIS_DONE_Pos)                 /**< (ISI_IMR) Module Disable Operation Completed Mask */
#define ISI_IMR_DIS_DONE                    ISI_IMR_DIS_DONE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_DIS_DONE_Msk instead */
#define ISI_IMR_SRST_Pos                    2                                              /**< (ISI_IMR) Software Reset Completed Position */
#define ISI_IMR_SRST_Msk                    (0x1U << ISI_IMR_SRST_Pos)                     /**< (ISI_IMR) Software Reset Completed Mask */
#define ISI_IMR_SRST                        ISI_IMR_SRST_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_SRST_Msk instead */
#define ISI_IMR_VSYNC_Pos                   10                                             /**< (ISI_IMR) Vertical Synchronization Position */
#define ISI_IMR_VSYNC_Msk                   (0x1U << ISI_IMR_VSYNC_Pos)                    /**< (ISI_IMR) Vertical Synchronization Mask */
#define ISI_IMR_VSYNC                       ISI_IMR_VSYNC_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_VSYNC_Msk instead */
#define ISI_IMR_PXFR_DONE_Pos               16                                             /**< (ISI_IMR) Preview DMA Transfer Completed Position */
#define ISI_IMR_PXFR_DONE_Msk               (0x1U << ISI_IMR_PXFR_DONE_Pos)                /**< (ISI_IMR) Preview DMA Transfer Completed Mask */
#define ISI_IMR_PXFR_DONE                   ISI_IMR_PXFR_DONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_PXFR_DONE_Msk instead */
#define ISI_IMR_CXFR_DONE_Pos               17                                             /**< (ISI_IMR) Codec DMA Transfer Completed Position */
#define ISI_IMR_CXFR_DONE_Msk               (0x1U << ISI_IMR_CXFR_DONE_Pos)                /**< (ISI_IMR) Codec DMA Transfer Completed Mask */
#define ISI_IMR_CXFR_DONE                   ISI_IMR_CXFR_DONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_CXFR_DONE_Msk instead */
#define ISI_IMR_P_OVR_Pos                   24                                             /**< (ISI_IMR) Preview FIFO Overflow Position */
#define ISI_IMR_P_OVR_Msk                   (0x1U << ISI_IMR_P_OVR_Pos)                    /**< (ISI_IMR) Preview FIFO Overflow Mask */
#define ISI_IMR_P_OVR                       ISI_IMR_P_OVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_P_OVR_Msk instead */
#define ISI_IMR_C_OVR_Pos                   25                                             /**< (ISI_IMR) Codec FIFO Overflow Position */
#define ISI_IMR_C_OVR_Msk                   (0x1U << ISI_IMR_C_OVR_Pos)                    /**< (ISI_IMR) Codec FIFO Overflow Mask */
#define ISI_IMR_C_OVR                       ISI_IMR_C_OVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_C_OVR_Msk instead */
#define ISI_IMR_CRC_ERR_Pos                 26                                             /**< (ISI_IMR) CRC Synchronization Error Position */
#define ISI_IMR_CRC_ERR_Msk                 (0x1U << ISI_IMR_CRC_ERR_Pos)                  /**< (ISI_IMR) CRC Synchronization Error Mask */
#define ISI_IMR_CRC_ERR                     ISI_IMR_CRC_ERR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_CRC_ERR_Msk instead */
#define ISI_IMR_FR_OVR_Pos                  27                                             /**< (ISI_IMR) Frame Rate Overrun Position */
#define ISI_IMR_FR_OVR_Msk                  (0x1U << ISI_IMR_FR_OVR_Pos)                   /**< (ISI_IMR) Frame Rate Overrun Mask */
#define ISI_IMR_FR_OVR                      ISI_IMR_FR_OVR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_IMR_FR_OVR_Msk instead */
#define ISI_IMR_MASK                        (0xF030406U)                                   /**< \deprecated (ISI_IMR) Register MASK  (Use ISI_IMR_Msk instead)  */
#define ISI_IMR_Msk                         (0xF030406U)                                   /**< (ISI_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_CHER : (ISI Offset: 0x38) (/W 32) DMA Channel Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P_CH_EN:1;                 /**< bit:      0  Preview Channel Enable                   */
    uint32_t C_CH_EN:1;                 /**< bit:      1  Codec Channel Enable                     */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_CHER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_CHER_OFFSET                 (0x38)                                        /**<  (ISI_DMA_CHER) DMA Channel Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_CHER_P_CH_EN_Pos            0                                              /**< (ISI_DMA_CHER) Preview Channel Enable Position */
#define ISI_DMA_CHER_P_CH_EN_Msk            (0x1U << ISI_DMA_CHER_P_CH_EN_Pos)             /**< (ISI_DMA_CHER) Preview Channel Enable Mask */
#define ISI_DMA_CHER_P_CH_EN                ISI_DMA_CHER_P_CH_EN_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_CHER_P_CH_EN_Msk instead */
#define ISI_DMA_CHER_C_CH_EN_Pos            1                                              /**< (ISI_DMA_CHER) Codec Channel Enable Position */
#define ISI_DMA_CHER_C_CH_EN_Msk            (0x1U << ISI_DMA_CHER_C_CH_EN_Pos)             /**< (ISI_DMA_CHER) Codec Channel Enable Mask */
#define ISI_DMA_CHER_C_CH_EN                ISI_DMA_CHER_C_CH_EN_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_CHER_C_CH_EN_Msk instead */
#define ISI_DMA_CHER_MASK                   (0x03U)                                        /**< \deprecated (ISI_DMA_CHER) Register MASK  (Use ISI_DMA_CHER_Msk instead)  */
#define ISI_DMA_CHER_Msk                    (0x03U)                                        /**< (ISI_DMA_CHER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_CHDR : (ISI Offset: 0x3c) (/W 32) DMA Channel Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P_CH_DIS:1;                /**< bit:      0  Preview Channel Disable Request          */
    uint32_t C_CH_DIS:1;                /**< bit:      1  Codec Channel Disable Request            */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_CHDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_CHDR_OFFSET                 (0x3C)                                        /**<  (ISI_DMA_CHDR) DMA Channel Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_CHDR_P_CH_DIS_Pos           0                                              /**< (ISI_DMA_CHDR) Preview Channel Disable Request Position */
#define ISI_DMA_CHDR_P_CH_DIS_Msk           (0x1U << ISI_DMA_CHDR_P_CH_DIS_Pos)            /**< (ISI_DMA_CHDR) Preview Channel Disable Request Mask */
#define ISI_DMA_CHDR_P_CH_DIS               ISI_DMA_CHDR_P_CH_DIS_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_CHDR_P_CH_DIS_Msk instead */
#define ISI_DMA_CHDR_C_CH_DIS_Pos           1                                              /**< (ISI_DMA_CHDR) Codec Channel Disable Request Position */
#define ISI_DMA_CHDR_C_CH_DIS_Msk           (0x1U << ISI_DMA_CHDR_C_CH_DIS_Pos)            /**< (ISI_DMA_CHDR) Codec Channel Disable Request Mask */
#define ISI_DMA_CHDR_C_CH_DIS               ISI_DMA_CHDR_C_CH_DIS_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_CHDR_C_CH_DIS_Msk instead */
#define ISI_DMA_CHDR_MASK                   (0x03U)                                        /**< \deprecated (ISI_DMA_CHDR) Register MASK  (Use ISI_DMA_CHDR_Msk instead)  */
#define ISI_DMA_CHDR_Msk                    (0x03U)                                        /**< (ISI_DMA_CHDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_CHSR : (ISI Offset: 0x40) (R/ 32) DMA Channel Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P_CH_S:1;                  /**< bit:      0  Preview DMA Channel Status               */
    uint32_t C_CH_S:1;                  /**< bit:      1  Code DMA Channel Status                  */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_CHSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_CHSR_OFFSET                 (0x40)                                        /**<  (ISI_DMA_CHSR) DMA Channel Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_CHSR_P_CH_S_Pos             0                                              /**< (ISI_DMA_CHSR) Preview DMA Channel Status Position */
#define ISI_DMA_CHSR_P_CH_S_Msk             (0x1U << ISI_DMA_CHSR_P_CH_S_Pos)              /**< (ISI_DMA_CHSR) Preview DMA Channel Status Mask */
#define ISI_DMA_CHSR_P_CH_S                 ISI_DMA_CHSR_P_CH_S_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_CHSR_P_CH_S_Msk instead */
#define ISI_DMA_CHSR_C_CH_S_Pos             1                                              /**< (ISI_DMA_CHSR) Code DMA Channel Status Position */
#define ISI_DMA_CHSR_C_CH_S_Msk             (0x1U << ISI_DMA_CHSR_C_CH_S_Pos)              /**< (ISI_DMA_CHSR) Code DMA Channel Status Mask */
#define ISI_DMA_CHSR_C_CH_S                 ISI_DMA_CHSR_C_CH_S_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_CHSR_C_CH_S_Msk instead */
#define ISI_DMA_CHSR_MASK                   (0x03U)                                        /**< \deprecated (ISI_DMA_CHSR) Register MASK  (Use ISI_DMA_CHSR_Msk instead)  */
#define ISI_DMA_CHSR_Msk                    (0x03U)                                        /**< (ISI_DMA_CHSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_P_ADDR : (ISI Offset: 0x44) (R/W 32) DMA Preview Base Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t P_ADDR:30;                 /**< bit:  2..31  Preview Image Base Address               */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_P_ADDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_P_ADDR_OFFSET               (0x44)                                        /**<  (ISI_DMA_P_ADDR) DMA Preview Base Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_P_ADDR_P_ADDR_Pos           2                                              /**< (ISI_DMA_P_ADDR) Preview Image Base Address Position */
#define ISI_DMA_P_ADDR_P_ADDR_Msk           (0x3FFFFFFFU << ISI_DMA_P_ADDR_P_ADDR_Pos)     /**< (ISI_DMA_P_ADDR) Preview Image Base Address Mask */
#define ISI_DMA_P_ADDR_P_ADDR(value)        (ISI_DMA_P_ADDR_P_ADDR_Msk & ((value) << ISI_DMA_P_ADDR_P_ADDR_Pos))
#define ISI_DMA_P_ADDR_MASK                 (0xFFFFFFFCU)                                  /**< \deprecated (ISI_DMA_P_ADDR) Register MASK  (Use ISI_DMA_P_ADDR_Msk instead)  */
#define ISI_DMA_P_ADDR_Msk                  (0xFFFFFFFCU)                                  /**< (ISI_DMA_P_ADDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_P_CTRL : (ISI Offset: 0x48) (R/W 32) DMA Preview Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P_FETCH:1;                 /**< bit:      0  Descriptor Fetch Control Bit             */
    uint32_t P_WB:1;                    /**< bit:      1  Descriptor Writeback Control Bit         */
    uint32_t P_IEN:1;                   /**< bit:      2  Transfer Done Flag Control               */
    uint32_t P_DONE:1;                  /**< bit:      3  Preview Transfer Done                    */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_P_CTRL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_P_CTRL_OFFSET               (0x48)                                        /**<  (ISI_DMA_P_CTRL) DMA Preview Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_P_CTRL_P_FETCH_Pos          0                                              /**< (ISI_DMA_P_CTRL) Descriptor Fetch Control Bit Position */
#define ISI_DMA_P_CTRL_P_FETCH_Msk          (0x1U << ISI_DMA_P_CTRL_P_FETCH_Pos)           /**< (ISI_DMA_P_CTRL) Descriptor Fetch Control Bit Mask */
#define ISI_DMA_P_CTRL_P_FETCH              ISI_DMA_P_CTRL_P_FETCH_Msk                     /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_P_CTRL_P_FETCH_Msk instead */
#define ISI_DMA_P_CTRL_P_WB_Pos             1                                              /**< (ISI_DMA_P_CTRL) Descriptor Writeback Control Bit Position */
#define ISI_DMA_P_CTRL_P_WB_Msk             (0x1U << ISI_DMA_P_CTRL_P_WB_Pos)              /**< (ISI_DMA_P_CTRL) Descriptor Writeback Control Bit Mask */
#define ISI_DMA_P_CTRL_P_WB                 ISI_DMA_P_CTRL_P_WB_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_P_CTRL_P_WB_Msk instead */
#define ISI_DMA_P_CTRL_P_IEN_Pos            2                                              /**< (ISI_DMA_P_CTRL) Transfer Done Flag Control Position */
#define ISI_DMA_P_CTRL_P_IEN_Msk            (0x1U << ISI_DMA_P_CTRL_P_IEN_Pos)             /**< (ISI_DMA_P_CTRL) Transfer Done Flag Control Mask */
#define ISI_DMA_P_CTRL_P_IEN                ISI_DMA_P_CTRL_P_IEN_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_P_CTRL_P_IEN_Msk instead */
#define ISI_DMA_P_CTRL_P_DONE_Pos           3                                              /**< (ISI_DMA_P_CTRL) Preview Transfer Done Position */
#define ISI_DMA_P_CTRL_P_DONE_Msk           (0x1U << ISI_DMA_P_CTRL_P_DONE_Pos)            /**< (ISI_DMA_P_CTRL) Preview Transfer Done Mask */
#define ISI_DMA_P_CTRL_P_DONE               ISI_DMA_P_CTRL_P_DONE_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_P_CTRL_P_DONE_Msk instead */
#define ISI_DMA_P_CTRL_MASK                 (0x0FU)                                        /**< \deprecated (ISI_DMA_P_CTRL) Register MASK  (Use ISI_DMA_P_CTRL_Msk instead)  */
#define ISI_DMA_P_CTRL_Msk                  (0x0FU)                                        /**< (ISI_DMA_P_CTRL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_P_DSCR : (ISI Offset: 0x4c) (R/W 32) DMA Preview Descriptor Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t P_DSCR:30;                 /**< bit:  2..31  Preview Descriptor Base Address          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_P_DSCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_P_DSCR_OFFSET               (0x4C)                                        /**<  (ISI_DMA_P_DSCR) DMA Preview Descriptor Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_P_DSCR_P_DSCR_Pos           2                                              /**< (ISI_DMA_P_DSCR) Preview Descriptor Base Address Position */
#define ISI_DMA_P_DSCR_P_DSCR_Msk           (0x3FFFFFFFU << ISI_DMA_P_DSCR_P_DSCR_Pos)     /**< (ISI_DMA_P_DSCR) Preview Descriptor Base Address Mask */
#define ISI_DMA_P_DSCR_P_DSCR(value)        (ISI_DMA_P_DSCR_P_DSCR_Msk & ((value) << ISI_DMA_P_DSCR_P_DSCR_Pos))
#define ISI_DMA_P_DSCR_MASK                 (0xFFFFFFFCU)                                  /**< \deprecated (ISI_DMA_P_DSCR) Register MASK  (Use ISI_DMA_P_DSCR_Msk instead)  */
#define ISI_DMA_P_DSCR_Msk                  (0xFFFFFFFCU)                                  /**< (ISI_DMA_P_DSCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_C_ADDR : (ISI Offset: 0x50) (R/W 32) DMA Codec Base Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t C_ADDR:30;                 /**< bit:  2..31  Codec Image Base Address                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_C_ADDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_C_ADDR_OFFSET               (0x50)                                        /**<  (ISI_DMA_C_ADDR) DMA Codec Base Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_C_ADDR_C_ADDR_Pos           2                                              /**< (ISI_DMA_C_ADDR) Codec Image Base Address Position */
#define ISI_DMA_C_ADDR_C_ADDR_Msk           (0x3FFFFFFFU << ISI_DMA_C_ADDR_C_ADDR_Pos)     /**< (ISI_DMA_C_ADDR) Codec Image Base Address Mask */
#define ISI_DMA_C_ADDR_C_ADDR(value)        (ISI_DMA_C_ADDR_C_ADDR_Msk & ((value) << ISI_DMA_C_ADDR_C_ADDR_Pos))
#define ISI_DMA_C_ADDR_MASK                 (0xFFFFFFFCU)                                  /**< \deprecated (ISI_DMA_C_ADDR) Register MASK  (Use ISI_DMA_C_ADDR_Msk instead)  */
#define ISI_DMA_C_ADDR_Msk                  (0xFFFFFFFCU)                                  /**< (ISI_DMA_C_ADDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_C_CTRL : (ISI Offset: 0x54) (R/W 32) DMA Codec Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t C_FETCH:1;                 /**< bit:      0  Descriptor Fetch Control Bit             */
    uint32_t C_WB:1;                    /**< bit:      1  Descriptor Writeback Control Bit         */
    uint32_t C_IEN:1;                   /**< bit:      2  Transfer Done Flag Control               */
    uint32_t C_DONE:1;                  /**< bit:      3  Codec Transfer Done                      */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_C_CTRL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_C_CTRL_OFFSET               (0x54)                                        /**<  (ISI_DMA_C_CTRL) DMA Codec Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_C_CTRL_C_FETCH_Pos          0                                              /**< (ISI_DMA_C_CTRL) Descriptor Fetch Control Bit Position */
#define ISI_DMA_C_CTRL_C_FETCH_Msk          (0x1U << ISI_DMA_C_CTRL_C_FETCH_Pos)           /**< (ISI_DMA_C_CTRL) Descriptor Fetch Control Bit Mask */
#define ISI_DMA_C_CTRL_C_FETCH              ISI_DMA_C_CTRL_C_FETCH_Msk                     /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_C_CTRL_C_FETCH_Msk instead */
#define ISI_DMA_C_CTRL_C_WB_Pos             1                                              /**< (ISI_DMA_C_CTRL) Descriptor Writeback Control Bit Position */
#define ISI_DMA_C_CTRL_C_WB_Msk             (0x1U << ISI_DMA_C_CTRL_C_WB_Pos)              /**< (ISI_DMA_C_CTRL) Descriptor Writeback Control Bit Mask */
#define ISI_DMA_C_CTRL_C_WB                 ISI_DMA_C_CTRL_C_WB_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_C_CTRL_C_WB_Msk instead */
#define ISI_DMA_C_CTRL_C_IEN_Pos            2                                              /**< (ISI_DMA_C_CTRL) Transfer Done Flag Control Position */
#define ISI_DMA_C_CTRL_C_IEN_Msk            (0x1U << ISI_DMA_C_CTRL_C_IEN_Pos)             /**< (ISI_DMA_C_CTRL) Transfer Done Flag Control Mask */
#define ISI_DMA_C_CTRL_C_IEN                ISI_DMA_C_CTRL_C_IEN_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_C_CTRL_C_IEN_Msk instead */
#define ISI_DMA_C_CTRL_C_DONE_Pos           3                                              /**< (ISI_DMA_C_CTRL) Codec Transfer Done Position */
#define ISI_DMA_C_CTRL_C_DONE_Msk           (0x1U << ISI_DMA_C_CTRL_C_DONE_Pos)            /**< (ISI_DMA_C_CTRL) Codec Transfer Done Mask */
#define ISI_DMA_C_CTRL_C_DONE               ISI_DMA_C_CTRL_C_DONE_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_DMA_C_CTRL_C_DONE_Msk instead */
#define ISI_DMA_C_CTRL_MASK                 (0x0FU)                                        /**< \deprecated (ISI_DMA_C_CTRL) Register MASK  (Use ISI_DMA_C_CTRL_Msk instead)  */
#define ISI_DMA_C_CTRL_Msk                  (0x0FU)                                        /**< (ISI_DMA_C_CTRL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_DMA_C_DSCR : (ISI Offset: 0x58) (R/W 32) DMA Codec Descriptor Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t C_DSCR:30;                 /**< bit:  2..31  Codec Descriptor Base Address            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_DMA_C_DSCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_DMA_C_DSCR_OFFSET               (0x58)                                        /**<  (ISI_DMA_C_DSCR) DMA Codec Descriptor Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_DMA_C_DSCR_C_DSCR_Pos           2                                              /**< (ISI_DMA_C_DSCR) Codec Descriptor Base Address Position */
#define ISI_DMA_C_DSCR_C_DSCR_Msk           (0x3FFFFFFFU << ISI_DMA_C_DSCR_C_DSCR_Pos)     /**< (ISI_DMA_C_DSCR) Codec Descriptor Base Address Mask */
#define ISI_DMA_C_DSCR_C_DSCR(value)        (ISI_DMA_C_DSCR_C_DSCR_Msk & ((value) << ISI_DMA_C_DSCR_C_DSCR_Pos))
#define ISI_DMA_C_DSCR_MASK                 (0xFFFFFFFCU)                                  /**< \deprecated (ISI_DMA_C_DSCR) Register MASK  (Use ISI_DMA_C_DSCR_Msk instead)  */
#define ISI_DMA_C_DSCR_Msk                  (0xFFFFFFFCU)                                  /**< (ISI_DMA_C_DSCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_WPMR : (ISI Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key Password            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_WPMR_OFFSET                     (0xE4)                                        /**<  (ISI_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_WPMR_WPEN_Pos                   0                                              /**< (ISI_WPMR) Write Protection Enable Position */
#define ISI_WPMR_WPEN_Msk                   (0x1U << ISI_WPMR_WPEN_Pos)                    /**< (ISI_WPMR) Write Protection Enable Mask */
#define ISI_WPMR_WPEN                       ISI_WPMR_WPEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_WPMR_WPEN_Msk instead */
#define ISI_WPMR_WPKEY_Pos                  8                                              /**< (ISI_WPMR) Write Protection Key Password Position */
#define ISI_WPMR_WPKEY_Msk                  (0xFFFFFFU << ISI_WPMR_WPKEY_Pos)              /**< (ISI_WPMR) Write Protection Key Password Mask */
#define ISI_WPMR_WPKEY(value)               (ISI_WPMR_WPKEY_Msk & ((value) << ISI_WPMR_WPKEY_Pos))
#define   ISI_WPMR_WPKEY_PASSWD_Val         (0x495349U)                                    /**< (ISI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0.  */
#define ISI_WPMR_WPKEY_PASSWD               (ISI_WPMR_WPKEY_PASSWD_Val << ISI_WPMR_WPKEY_Pos)  /**< (ISI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. Position  */
#define ISI_WPMR_MASK                       (0xFFFFFF01U)                                  /**< \deprecated (ISI_WPMR) Register MASK  (Use ISI_WPMR_Msk instead)  */
#define ISI_WPMR_Msk                        (0xFFFFFF01U)                                  /**< (ISI_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ISI_WPSR : (ISI Offset: 0xe8) (R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:16;                 /**< bit:  8..23  Write Protection Violation Source        */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ISI_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ISI_WPSR_OFFSET                     (0xE8)                                        /**<  (ISI_WPSR) Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ISI_WPSR_WPVS_Pos                   0                                              /**< (ISI_WPSR) Write Protection Violation Status Position */
#define ISI_WPSR_WPVS_Msk                   (0x1U << ISI_WPSR_WPVS_Pos)                    /**< (ISI_WPSR) Write Protection Violation Status Mask */
#define ISI_WPSR_WPVS                       ISI_WPSR_WPVS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ISI_WPSR_WPVS_Msk instead */
#define ISI_WPSR_WPVSRC_Pos                 8                                              /**< (ISI_WPSR) Write Protection Violation Source Position */
#define ISI_WPSR_WPVSRC_Msk                 (0xFFFFU << ISI_WPSR_WPVSRC_Pos)               /**< (ISI_WPSR) Write Protection Violation Source Mask */
#define ISI_WPSR_WPVSRC(value)              (ISI_WPSR_WPVSRC_Msk & ((value) << ISI_WPSR_WPVSRC_Pos))
#define ISI_WPSR_MASK                       (0xFFFF01U)                                    /**< \deprecated (ISI_WPSR) Register MASK  (Use ISI_WPSR_Msk instead)  */
#define ISI_WPSR_Msk                        (0xFFFF01U)                                    /**< (ISI_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief ISI hardware registers */
typedef struct {  
  __IO uint32_t ISI_CFG1;       /**< (ISI Offset: 0x00) ISI Configuration 1 Register */
  __IO uint32_t ISI_CFG2;       /**< (ISI Offset: 0x04) ISI Configuration 2 Register */
  __IO uint32_t ISI_PSIZE;      /**< (ISI Offset: 0x08) ISI Preview Size Register */
  __IO uint32_t ISI_PDECF;      /**< (ISI Offset: 0x0C) ISI Preview Decimation Factor Register */
  __IO uint32_t ISI_Y2R_SET0;   /**< (ISI Offset: 0x10) ISI Color Space Conversion YCrCb To RGB Set 0 Register */
  __IO uint32_t ISI_Y2R_SET1;   /**< (ISI Offset: 0x14) ISI Color Space Conversion YCrCb To RGB Set 1 Register */
  __IO uint32_t ISI_R2Y_SET0;   /**< (ISI Offset: 0x18) ISI Color Space Conversion RGB To YCrCb Set 0 Register */
  __IO uint32_t ISI_R2Y_SET1;   /**< (ISI Offset: 0x1C) ISI Color Space Conversion RGB To YCrCb Set 1 Register */
  __IO uint32_t ISI_R2Y_SET2;   /**< (ISI Offset: 0x20) ISI Color Space Conversion RGB To YCrCb Set 2 Register */
  __O  uint32_t ISI_CR;         /**< (ISI Offset: 0x24) ISI Control Register */
  __I  uint32_t ISI_SR;         /**< (ISI Offset: 0x28) ISI Status Register */
  __O  uint32_t ISI_IER;        /**< (ISI Offset: 0x2C) ISI Interrupt Enable Register */
  __O  uint32_t ISI_IDR;        /**< (ISI Offset: 0x30) ISI Interrupt Disable Register */
  __I  uint32_t ISI_IMR;        /**< (ISI Offset: 0x34) ISI Interrupt Mask Register */
  __O  uint32_t ISI_DMA_CHER;   /**< (ISI Offset: 0x38) DMA Channel Enable Register */
  __O  uint32_t ISI_DMA_CHDR;   /**< (ISI Offset: 0x3C) DMA Channel Disable Register */
  __I  uint32_t ISI_DMA_CHSR;   /**< (ISI Offset: 0x40) DMA Channel Status Register */
  __IO uint32_t ISI_DMA_P_ADDR; /**< (ISI Offset: 0x44) DMA Preview Base Address Register */
  __IO uint32_t ISI_DMA_P_CTRL; /**< (ISI Offset: 0x48) DMA Preview Control Register */
  __IO uint32_t ISI_DMA_P_DSCR; /**< (ISI Offset: 0x4C) DMA Preview Descriptor Address Register */
  __IO uint32_t ISI_DMA_C_ADDR; /**< (ISI Offset: 0x50) DMA Codec Base Address Register */
  __IO uint32_t ISI_DMA_C_CTRL; /**< (ISI Offset: 0x54) DMA Codec Control Register */
  __IO uint32_t ISI_DMA_C_DSCR; /**< (ISI Offset: 0x58) DMA Codec Descriptor Address Register */
  __I  uint32_t Reserved1[34];
  __IO uint32_t ISI_WPMR;       /**< (ISI Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t ISI_WPSR;       /**< (ISI Offset: 0xE8) Write Protection Status Register */
} Isi;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief ISI hardware registers */
typedef struct {  
  __IO ISI_CFG1_Type                  ISI_CFG1;       /**< Offset: 0x00 (R/W  32) ISI Configuration 1 Register */
  __IO ISI_CFG2_Type                  ISI_CFG2;       /**< Offset: 0x04 (R/W  32) ISI Configuration 2 Register */
  __IO ISI_PSIZE_Type                 ISI_PSIZE;      /**< Offset: 0x08 (R/W  32) ISI Preview Size Register */
  __IO ISI_PDECF_Type                 ISI_PDECF;      /**< Offset: 0x0C (R/W  32) ISI Preview Decimation Factor Register */
  __IO ISI_Y2R_SET0_Type              ISI_Y2R_SET0;   /**< Offset: 0x10 (R/W  32) ISI Color Space Conversion YCrCb To RGB Set 0 Register */
  __IO ISI_Y2R_SET1_Type              ISI_Y2R_SET1;   /**< Offset: 0x14 (R/W  32) ISI Color Space Conversion YCrCb To RGB Set 1 Register */
  __IO ISI_R2Y_SET0_Type              ISI_R2Y_SET0;   /**< Offset: 0x18 (R/W  32) ISI Color Space Conversion RGB To YCrCb Set 0 Register */
  __IO ISI_R2Y_SET1_Type              ISI_R2Y_SET1;   /**< Offset: 0x1C (R/W  32) ISI Color Space Conversion RGB To YCrCb Set 1 Register */
  __IO ISI_R2Y_SET2_Type              ISI_R2Y_SET2;   /**< Offset: 0x20 (R/W  32) ISI Color Space Conversion RGB To YCrCb Set 2 Register */
  __O  ISI_CR_Type                    ISI_CR;         /**< Offset: 0x24 ( /W  32) ISI Control Register */
  __I  ISI_SR_Type                    ISI_SR;         /**< Offset: 0x28 (R/   32) ISI Status Register */
  __O  ISI_IER_Type                   ISI_IER;        /**< Offset: 0x2C ( /W  32) ISI Interrupt Enable Register */
  __O  ISI_IDR_Type                   ISI_IDR;        /**< Offset: 0x30 ( /W  32) ISI Interrupt Disable Register */
  __I  ISI_IMR_Type                   ISI_IMR;        /**< Offset: 0x34 (R/   32) ISI Interrupt Mask Register */
  __O  ISI_DMA_CHER_Type              ISI_DMA_CHER;   /**< Offset: 0x38 ( /W  32) DMA Channel Enable Register */
  __O  ISI_DMA_CHDR_Type              ISI_DMA_CHDR;   /**< Offset: 0x3C ( /W  32) DMA Channel Disable Register */
  __I  ISI_DMA_CHSR_Type              ISI_DMA_CHSR;   /**< Offset: 0x40 (R/   32) DMA Channel Status Register */
  __IO ISI_DMA_P_ADDR_Type            ISI_DMA_P_ADDR; /**< Offset: 0x44 (R/W  32) DMA Preview Base Address Register */
  __IO ISI_DMA_P_CTRL_Type            ISI_DMA_P_CTRL; /**< Offset: 0x48 (R/W  32) DMA Preview Control Register */
  __IO ISI_DMA_P_DSCR_Type            ISI_DMA_P_DSCR; /**< Offset: 0x4C (R/W  32) DMA Preview Descriptor Address Register */
  __IO ISI_DMA_C_ADDR_Type            ISI_DMA_C_ADDR; /**< Offset: 0x50 (R/W  32) DMA Codec Base Address Register */
  __IO ISI_DMA_C_CTRL_Type            ISI_DMA_C_CTRL; /**< Offset: 0x54 (R/W  32) DMA Codec Control Register */
  __IO ISI_DMA_C_DSCR_Type            ISI_DMA_C_DSCR; /**< Offset: 0x58 (R/W  32) DMA Codec Descriptor Address Register */
       RoReg8                         Reserved1[0x88];
  __IO ISI_WPMR_Type                  ISI_WPMR;       /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
  __I  ISI_WPSR_Type                  ISI_WPSR;       /**< Offset: 0xE8 (R/   32) Write Protection Status Register */
} Isi;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Image Sensor Interface */

#endif /* _SAME70_ISI_COMPONENT_H_ */
