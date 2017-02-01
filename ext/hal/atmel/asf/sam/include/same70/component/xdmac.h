/**
 * \file
 *
 * \brief Component description for XDMAC
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

#ifndef _SAME70_XDMAC_COMPONENT_H_
#define _SAME70_XDMAC_COMPONENT_H_
#define _SAME70_XDMAC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_XDMAC Extensible DMA Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR XDMAC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define XDMAC_11161                      /**< (XDMAC) Module ID */
#define REV_XDMAC G                      /**< (XDMAC) Module revision */

/* -------- XDMAC_CIE : (XDMAC Offset: 0x00) (/W 32) Channel Interrupt Enable Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BIE:1;                     /**< bit:      0  End of Block Interrupt Enable Bit        */
    uint32_t LIE:1;                     /**< bit:      1  End of Linked List Interrupt Enable Bit  */
    uint32_t DIE:1;                     /**< bit:      2  End of Disable Interrupt Enable Bit      */
    uint32_t FIE:1;                     /**< bit:      3  End of Flush Interrupt Enable Bit        */
    uint32_t RBIE:1;                    /**< bit:      4  Read Bus Error Interrupt Enable Bit      */
    uint32_t WBIE:1;                    /**< bit:      5  Write Bus Error Interrupt Enable Bit     */
    uint32_t ROIE:1;                    /**< bit:      6  Request Overflow Error Interrupt Enable Bit */
    uint32_t :25;                       /**< bit:  7..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CIE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CIE_OFFSET                    (0x00)                                        /**<  (XDMAC_CIE) Channel Interrupt Enable Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CIE_BIE_Pos                   0                                              /**< (XDMAC_CIE) End of Block Interrupt Enable Bit Position */
#define XDMAC_CIE_BIE_Msk                   (0x1U << XDMAC_CIE_BIE_Pos)                    /**< (XDMAC_CIE) End of Block Interrupt Enable Bit Mask */
#define XDMAC_CIE_BIE                       XDMAC_CIE_BIE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIE_BIE_Msk instead */
#define XDMAC_CIE_LIE_Pos                   1                                              /**< (XDMAC_CIE) End of Linked List Interrupt Enable Bit Position */
#define XDMAC_CIE_LIE_Msk                   (0x1U << XDMAC_CIE_LIE_Pos)                    /**< (XDMAC_CIE) End of Linked List Interrupt Enable Bit Mask */
#define XDMAC_CIE_LIE                       XDMAC_CIE_LIE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIE_LIE_Msk instead */
#define XDMAC_CIE_DIE_Pos                   2                                              /**< (XDMAC_CIE) End of Disable Interrupt Enable Bit Position */
#define XDMAC_CIE_DIE_Msk                   (0x1U << XDMAC_CIE_DIE_Pos)                    /**< (XDMAC_CIE) End of Disable Interrupt Enable Bit Mask */
#define XDMAC_CIE_DIE                       XDMAC_CIE_DIE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIE_DIE_Msk instead */
#define XDMAC_CIE_FIE_Pos                   3                                              /**< (XDMAC_CIE) End of Flush Interrupt Enable Bit Position */
#define XDMAC_CIE_FIE_Msk                   (0x1U << XDMAC_CIE_FIE_Pos)                    /**< (XDMAC_CIE) End of Flush Interrupt Enable Bit Mask */
#define XDMAC_CIE_FIE                       XDMAC_CIE_FIE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIE_FIE_Msk instead */
#define XDMAC_CIE_RBIE_Pos                  4                                              /**< (XDMAC_CIE) Read Bus Error Interrupt Enable Bit Position */
#define XDMAC_CIE_RBIE_Msk                  (0x1U << XDMAC_CIE_RBIE_Pos)                   /**< (XDMAC_CIE) Read Bus Error Interrupt Enable Bit Mask */
#define XDMAC_CIE_RBIE                      XDMAC_CIE_RBIE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIE_RBIE_Msk instead */
#define XDMAC_CIE_WBIE_Pos                  5                                              /**< (XDMAC_CIE) Write Bus Error Interrupt Enable Bit Position */
#define XDMAC_CIE_WBIE_Msk                  (0x1U << XDMAC_CIE_WBIE_Pos)                   /**< (XDMAC_CIE) Write Bus Error Interrupt Enable Bit Mask */
#define XDMAC_CIE_WBIE                      XDMAC_CIE_WBIE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIE_WBIE_Msk instead */
#define XDMAC_CIE_ROIE_Pos                  6                                              /**< (XDMAC_CIE) Request Overflow Error Interrupt Enable Bit Position */
#define XDMAC_CIE_ROIE_Msk                  (0x1U << XDMAC_CIE_ROIE_Pos)                   /**< (XDMAC_CIE) Request Overflow Error Interrupt Enable Bit Mask */
#define XDMAC_CIE_ROIE                      XDMAC_CIE_ROIE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIE_ROIE_Msk instead */
#define XDMAC_CIE_MASK                      (0x7FU)                                        /**< \deprecated (XDMAC_CIE) Register MASK  (Use XDMAC_CIE_Msk instead)  */
#define XDMAC_CIE_Msk                       (0x7FU)                                        /**< (XDMAC_CIE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CID : (XDMAC Offset: 0x04) (/W 32) Channel Interrupt Disable Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BID:1;                     /**< bit:      0  End of Block Interrupt Disable Bit       */
    uint32_t LID:1;                     /**< bit:      1  End of Linked List Interrupt Disable Bit */
    uint32_t DID:1;                     /**< bit:      2  End of Disable Interrupt Disable Bit     */
    uint32_t FID:1;                     /**< bit:      3  End of Flush Interrupt Disable Bit       */
    uint32_t RBEID:1;                   /**< bit:      4  Read Bus Error Interrupt Disable Bit     */
    uint32_t WBEID:1;                   /**< bit:      5  Write Bus Error Interrupt Disable Bit    */
    uint32_t ROID:1;                    /**< bit:      6  Request Overflow Error Interrupt Disable Bit */
    uint32_t :25;                       /**< bit:  7..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CID_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CID_OFFSET                    (0x04)                                        /**<  (XDMAC_CID) Channel Interrupt Disable Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CID_BID_Pos                   0                                              /**< (XDMAC_CID) End of Block Interrupt Disable Bit Position */
#define XDMAC_CID_BID_Msk                   (0x1U << XDMAC_CID_BID_Pos)                    /**< (XDMAC_CID) End of Block Interrupt Disable Bit Mask */
#define XDMAC_CID_BID                       XDMAC_CID_BID_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CID_BID_Msk instead */
#define XDMAC_CID_LID_Pos                   1                                              /**< (XDMAC_CID) End of Linked List Interrupt Disable Bit Position */
#define XDMAC_CID_LID_Msk                   (0x1U << XDMAC_CID_LID_Pos)                    /**< (XDMAC_CID) End of Linked List Interrupt Disable Bit Mask */
#define XDMAC_CID_LID                       XDMAC_CID_LID_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CID_LID_Msk instead */
#define XDMAC_CID_DID_Pos                   2                                              /**< (XDMAC_CID) End of Disable Interrupt Disable Bit Position */
#define XDMAC_CID_DID_Msk                   (0x1U << XDMAC_CID_DID_Pos)                    /**< (XDMAC_CID) End of Disable Interrupt Disable Bit Mask */
#define XDMAC_CID_DID                       XDMAC_CID_DID_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CID_DID_Msk instead */
#define XDMAC_CID_FID_Pos                   3                                              /**< (XDMAC_CID) End of Flush Interrupt Disable Bit Position */
#define XDMAC_CID_FID_Msk                   (0x1U << XDMAC_CID_FID_Pos)                    /**< (XDMAC_CID) End of Flush Interrupt Disable Bit Mask */
#define XDMAC_CID_FID                       XDMAC_CID_FID_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CID_FID_Msk instead */
#define XDMAC_CID_RBEID_Pos                 4                                              /**< (XDMAC_CID) Read Bus Error Interrupt Disable Bit Position */
#define XDMAC_CID_RBEID_Msk                 (0x1U << XDMAC_CID_RBEID_Pos)                  /**< (XDMAC_CID) Read Bus Error Interrupt Disable Bit Mask */
#define XDMAC_CID_RBEID                     XDMAC_CID_RBEID_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CID_RBEID_Msk instead */
#define XDMAC_CID_WBEID_Pos                 5                                              /**< (XDMAC_CID) Write Bus Error Interrupt Disable Bit Position */
#define XDMAC_CID_WBEID_Msk                 (0x1U << XDMAC_CID_WBEID_Pos)                  /**< (XDMAC_CID) Write Bus Error Interrupt Disable Bit Mask */
#define XDMAC_CID_WBEID                     XDMAC_CID_WBEID_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CID_WBEID_Msk instead */
#define XDMAC_CID_ROID_Pos                  6                                              /**< (XDMAC_CID) Request Overflow Error Interrupt Disable Bit Position */
#define XDMAC_CID_ROID_Msk                  (0x1U << XDMAC_CID_ROID_Pos)                   /**< (XDMAC_CID) Request Overflow Error Interrupt Disable Bit Mask */
#define XDMAC_CID_ROID                      XDMAC_CID_ROID_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CID_ROID_Msk instead */
#define XDMAC_CID_MASK                      (0x7FU)                                        /**< \deprecated (XDMAC_CID) Register MASK  (Use XDMAC_CID_Msk instead)  */
#define XDMAC_CID_Msk                       (0x7FU)                                        /**< (XDMAC_CID) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CIM : (XDMAC Offset: 0x08) (/W 32) Channel Interrupt Mask Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BIM:1;                     /**< bit:      0  End of Block Interrupt Mask Bit          */
    uint32_t LIM:1;                     /**< bit:      1  End of Linked List Interrupt Mask Bit    */
    uint32_t DIM:1;                     /**< bit:      2  End of Disable Interrupt Mask Bit        */
    uint32_t FIM:1;                     /**< bit:      3  End of Flush Interrupt Mask Bit          */
    uint32_t RBEIM:1;                   /**< bit:      4  Read Bus Error Interrupt Mask Bit        */
    uint32_t WBEIM:1;                   /**< bit:      5  Write Bus Error Interrupt Mask Bit       */
    uint32_t ROIM:1;                    /**< bit:      6  Request Overflow Error Interrupt Mask Bit */
    uint32_t :25;                       /**< bit:  7..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CIM_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CIM_OFFSET                    (0x08)                                        /**<  (XDMAC_CIM) Channel Interrupt Mask Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CIM_BIM_Pos                   0                                              /**< (XDMAC_CIM) End of Block Interrupt Mask Bit Position */
#define XDMAC_CIM_BIM_Msk                   (0x1U << XDMAC_CIM_BIM_Pos)                    /**< (XDMAC_CIM) End of Block Interrupt Mask Bit Mask */
#define XDMAC_CIM_BIM                       XDMAC_CIM_BIM_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIM_BIM_Msk instead */
#define XDMAC_CIM_LIM_Pos                   1                                              /**< (XDMAC_CIM) End of Linked List Interrupt Mask Bit Position */
#define XDMAC_CIM_LIM_Msk                   (0x1U << XDMAC_CIM_LIM_Pos)                    /**< (XDMAC_CIM) End of Linked List Interrupt Mask Bit Mask */
#define XDMAC_CIM_LIM                       XDMAC_CIM_LIM_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIM_LIM_Msk instead */
#define XDMAC_CIM_DIM_Pos                   2                                              /**< (XDMAC_CIM) End of Disable Interrupt Mask Bit Position */
#define XDMAC_CIM_DIM_Msk                   (0x1U << XDMAC_CIM_DIM_Pos)                    /**< (XDMAC_CIM) End of Disable Interrupt Mask Bit Mask */
#define XDMAC_CIM_DIM                       XDMAC_CIM_DIM_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIM_DIM_Msk instead */
#define XDMAC_CIM_FIM_Pos                   3                                              /**< (XDMAC_CIM) End of Flush Interrupt Mask Bit Position */
#define XDMAC_CIM_FIM_Msk                   (0x1U << XDMAC_CIM_FIM_Pos)                    /**< (XDMAC_CIM) End of Flush Interrupt Mask Bit Mask */
#define XDMAC_CIM_FIM                       XDMAC_CIM_FIM_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIM_FIM_Msk instead */
#define XDMAC_CIM_RBEIM_Pos                 4                                              /**< (XDMAC_CIM) Read Bus Error Interrupt Mask Bit Position */
#define XDMAC_CIM_RBEIM_Msk                 (0x1U << XDMAC_CIM_RBEIM_Pos)                  /**< (XDMAC_CIM) Read Bus Error Interrupt Mask Bit Mask */
#define XDMAC_CIM_RBEIM                     XDMAC_CIM_RBEIM_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIM_RBEIM_Msk instead */
#define XDMAC_CIM_WBEIM_Pos                 5                                              /**< (XDMAC_CIM) Write Bus Error Interrupt Mask Bit Position */
#define XDMAC_CIM_WBEIM_Msk                 (0x1U << XDMAC_CIM_WBEIM_Pos)                  /**< (XDMAC_CIM) Write Bus Error Interrupt Mask Bit Mask */
#define XDMAC_CIM_WBEIM                     XDMAC_CIM_WBEIM_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIM_WBEIM_Msk instead */
#define XDMAC_CIM_ROIM_Pos                  6                                              /**< (XDMAC_CIM) Request Overflow Error Interrupt Mask Bit Position */
#define XDMAC_CIM_ROIM_Msk                  (0x1U << XDMAC_CIM_ROIM_Pos)                   /**< (XDMAC_CIM) Request Overflow Error Interrupt Mask Bit Mask */
#define XDMAC_CIM_ROIM                      XDMAC_CIM_ROIM_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIM_ROIM_Msk instead */
#define XDMAC_CIM_MASK                      (0x7FU)                                        /**< \deprecated (XDMAC_CIM) Register MASK  (Use XDMAC_CIM_Msk instead)  */
#define XDMAC_CIM_Msk                       (0x7FU)                                        /**< (XDMAC_CIM) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CIS : (XDMAC Offset: 0x0c) (R/ 32) Channel Interrupt Status Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BIS:1;                     /**< bit:      0  End of Block Interrupt Status Bit        */
    uint32_t LIS:1;                     /**< bit:      1  End of Linked List Interrupt Status Bit  */
    uint32_t DIS:1;                     /**< bit:      2  End of Disable Interrupt Status Bit      */
    uint32_t FIS:1;                     /**< bit:      3  End of Flush Interrupt Status Bit        */
    uint32_t RBEIS:1;                   /**< bit:      4  Read Bus Error Interrupt Status Bit      */
    uint32_t WBEIS:1;                   /**< bit:      5  Write Bus Error Interrupt Status Bit     */
    uint32_t ROIS:1;                    /**< bit:      6  Request Overflow Error Interrupt Status Bit */
    uint32_t :25;                       /**< bit:  7..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CIS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CIS_OFFSET                    (0x0C)                                        /**<  (XDMAC_CIS) Channel Interrupt Status Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CIS_BIS_Pos                   0                                              /**< (XDMAC_CIS) End of Block Interrupt Status Bit Position */
#define XDMAC_CIS_BIS_Msk                   (0x1U << XDMAC_CIS_BIS_Pos)                    /**< (XDMAC_CIS) End of Block Interrupt Status Bit Mask */
#define XDMAC_CIS_BIS                       XDMAC_CIS_BIS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIS_BIS_Msk instead */
#define XDMAC_CIS_LIS_Pos                   1                                              /**< (XDMAC_CIS) End of Linked List Interrupt Status Bit Position */
#define XDMAC_CIS_LIS_Msk                   (0x1U << XDMAC_CIS_LIS_Pos)                    /**< (XDMAC_CIS) End of Linked List Interrupt Status Bit Mask */
#define XDMAC_CIS_LIS                       XDMAC_CIS_LIS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIS_LIS_Msk instead */
#define XDMAC_CIS_DIS_Pos                   2                                              /**< (XDMAC_CIS) End of Disable Interrupt Status Bit Position */
#define XDMAC_CIS_DIS_Msk                   (0x1U << XDMAC_CIS_DIS_Pos)                    /**< (XDMAC_CIS) End of Disable Interrupt Status Bit Mask */
#define XDMAC_CIS_DIS                       XDMAC_CIS_DIS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIS_DIS_Msk instead */
#define XDMAC_CIS_FIS_Pos                   3                                              /**< (XDMAC_CIS) End of Flush Interrupt Status Bit Position */
#define XDMAC_CIS_FIS_Msk                   (0x1U << XDMAC_CIS_FIS_Pos)                    /**< (XDMAC_CIS) End of Flush Interrupt Status Bit Mask */
#define XDMAC_CIS_FIS                       XDMAC_CIS_FIS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIS_FIS_Msk instead */
#define XDMAC_CIS_RBEIS_Pos                 4                                              /**< (XDMAC_CIS) Read Bus Error Interrupt Status Bit Position */
#define XDMAC_CIS_RBEIS_Msk                 (0x1U << XDMAC_CIS_RBEIS_Pos)                  /**< (XDMAC_CIS) Read Bus Error Interrupt Status Bit Mask */
#define XDMAC_CIS_RBEIS                     XDMAC_CIS_RBEIS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIS_RBEIS_Msk instead */
#define XDMAC_CIS_WBEIS_Pos                 5                                              /**< (XDMAC_CIS) Write Bus Error Interrupt Status Bit Position */
#define XDMAC_CIS_WBEIS_Msk                 (0x1U << XDMAC_CIS_WBEIS_Pos)                  /**< (XDMAC_CIS) Write Bus Error Interrupt Status Bit Mask */
#define XDMAC_CIS_WBEIS                     XDMAC_CIS_WBEIS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIS_WBEIS_Msk instead */
#define XDMAC_CIS_ROIS_Pos                  6                                              /**< (XDMAC_CIS) Request Overflow Error Interrupt Status Bit Position */
#define XDMAC_CIS_ROIS_Msk                  (0x1U << XDMAC_CIS_ROIS_Pos)                   /**< (XDMAC_CIS) Request Overflow Error Interrupt Status Bit Mask */
#define XDMAC_CIS_ROIS                      XDMAC_CIS_ROIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CIS_ROIS_Msk instead */
#define XDMAC_CIS_MASK                      (0x7FU)                                        /**< \deprecated (XDMAC_CIS) Register MASK  (Use XDMAC_CIS_Msk instead)  */
#define XDMAC_CIS_Msk                       (0x7FU)                                        /**< (XDMAC_CIS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CSA : (XDMAC Offset: 0x10) (R/W 32) Channel Source Address Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SA:32;                     /**< bit:  0..31  Channel x Source Address                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CSA_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CSA_OFFSET                    (0x10)                                        /**<  (XDMAC_CSA) Channel Source Address Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CSA_SA_Pos                    0                                              /**< (XDMAC_CSA) Channel x Source Address Position */
#define XDMAC_CSA_SA_Msk                    (0xFFFFFFFFU << XDMAC_CSA_SA_Pos)              /**< (XDMAC_CSA) Channel x Source Address Mask */
#define XDMAC_CSA_SA(value)                 (XDMAC_CSA_SA_Msk & ((value) << XDMAC_CSA_SA_Pos))
#define XDMAC_CSA_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (XDMAC_CSA) Register MASK  (Use XDMAC_CSA_Msk instead)  */
#define XDMAC_CSA_Msk                       (0xFFFFFFFFU)                                  /**< (XDMAC_CSA) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CDA : (XDMAC Offset: 0x14) (R/W 32) Channel Destination Address Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DA:32;                     /**< bit:  0..31  Channel x Destination Address            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CDA_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CDA_OFFSET                    (0x14)                                        /**<  (XDMAC_CDA) Channel Destination Address Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CDA_DA_Pos                    0                                              /**< (XDMAC_CDA) Channel x Destination Address Position */
#define XDMAC_CDA_DA_Msk                    (0xFFFFFFFFU << XDMAC_CDA_DA_Pos)              /**< (XDMAC_CDA) Channel x Destination Address Mask */
#define XDMAC_CDA_DA(value)                 (XDMAC_CDA_DA_Msk & ((value) << XDMAC_CDA_DA_Pos))
#define XDMAC_CDA_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (XDMAC_CDA) Register MASK  (Use XDMAC_CDA_Msk instead)  */
#define XDMAC_CDA_Msk                       (0xFFFFFFFFU)                                  /**< (XDMAC_CDA) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CNDA : (XDMAC Offset: 0x18) (R/W 32) Channel Next Descriptor Address Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NDAIF:1;                   /**< bit:      0  Channel x Next Descriptor Interface      */
    uint32_t :1;                        /**< bit:      1  Reserved */
    uint32_t NDA:30;                    /**< bit:  2..31  Channel x Next Descriptor Address        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CNDA_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CNDA_OFFSET                   (0x18)                                        /**<  (XDMAC_CNDA) Channel Next Descriptor Address Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CNDA_NDAIF_Pos                0                                              /**< (XDMAC_CNDA) Channel x Next Descriptor Interface Position */
#define XDMAC_CNDA_NDAIF_Msk                (0x1U << XDMAC_CNDA_NDAIF_Pos)                 /**< (XDMAC_CNDA) Channel x Next Descriptor Interface Mask */
#define XDMAC_CNDA_NDAIF                    XDMAC_CNDA_NDAIF_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CNDA_NDAIF_Msk instead */
#define XDMAC_CNDA_NDA_Pos                  2                                              /**< (XDMAC_CNDA) Channel x Next Descriptor Address Position */
#define XDMAC_CNDA_NDA_Msk                  (0x3FFFFFFFU << XDMAC_CNDA_NDA_Pos)            /**< (XDMAC_CNDA) Channel x Next Descriptor Address Mask */
#define XDMAC_CNDA_NDA(value)               (XDMAC_CNDA_NDA_Msk & ((value) << XDMAC_CNDA_NDA_Pos))
#define XDMAC_CNDA_MASK                     (0xFFFFFFFDU)                                  /**< \deprecated (XDMAC_CNDA) Register MASK  (Use XDMAC_CNDA_Msk instead)  */
#define XDMAC_CNDA_Msk                      (0xFFFFFFFDU)                                  /**< (XDMAC_CNDA) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CNDC : (XDMAC Offset: 0x1c) (R/W 32) Channel Next Descriptor Control Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NDE:1;                     /**< bit:      0  Channel x Next Descriptor Enable         */
    uint32_t NDSUP:1;                   /**< bit:      1  Channel x Next Descriptor Source Update  */
    uint32_t NDDUP:1;                   /**< bit:      2  Channel x Next Descriptor Destination Update */
    uint32_t NDVIEW:2;                  /**< bit:   3..4  Channel x Next Descriptor View           */
    uint32_t :27;                       /**< bit:  5..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CNDC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CNDC_OFFSET                   (0x1C)                                        /**<  (XDMAC_CNDC) Channel Next Descriptor Control Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CNDC_NDE_Pos                  0                                              /**< (XDMAC_CNDC) Channel x Next Descriptor Enable Position */
#define XDMAC_CNDC_NDE_Msk                  (0x1U << XDMAC_CNDC_NDE_Pos)                   /**< (XDMAC_CNDC) Channel x Next Descriptor Enable Mask */
#define XDMAC_CNDC_NDE                      XDMAC_CNDC_NDE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CNDC_NDE_Msk instead */
#define   XDMAC_CNDC_NDE_DSCR_FETCH_DIS_Val (0x0U)                                         /**< (XDMAC_CNDC) Descriptor fetch is disabled.  */
#define   XDMAC_CNDC_NDE_DSCR_FETCH_EN_Val  (0x1U)                                         /**< (XDMAC_CNDC) Descriptor fetch is enabled.  */
#define XDMAC_CNDC_NDE_DSCR_FETCH_DIS       (XDMAC_CNDC_NDE_DSCR_FETCH_DIS_Val << XDMAC_CNDC_NDE_Pos)  /**< (XDMAC_CNDC) Descriptor fetch is disabled. Position  */
#define XDMAC_CNDC_NDE_DSCR_FETCH_EN        (XDMAC_CNDC_NDE_DSCR_FETCH_EN_Val << XDMAC_CNDC_NDE_Pos)  /**< (XDMAC_CNDC) Descriptor fetch is enabled. Position  */
#define XDMAC_CNDC_NDSUP_Pos                1                                              /**< (XDMAC_CNDC) Channel x Next Descriptor Source Update Position */
#define XDMAC_CNDC_NDSUP_Msk                (0x1U << XDMAC_CNDC_NDSUP_Pos)                 /**< (XDMAC_CNDC) Channel x Next Descriptor Source Update Mask */
#define XDMAC_CNDC_NDSUP                    XDMAC_CNDC_NDSUP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CNDC_NDSUP_Msk instead */
#define   XDMAC_CNDC_NDSUP_SRC_PARAMS_UNCHANGED_Val (0x0U)                                         /**< (XDMAC_CNDC) Source parameters remain unchanged.  */
#define   XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED_Val (0x1U)                                         /**< (XDMAC_CNDC) Source parameters are updated when the descriptor is retrieved.  */
#define XDMAC_CNDC_NDSUP_SRC_PARAMS_UNCHANGED (XDMAC_CNDC_NDSUP_SRC_PARAMS_UNCHANGED_Val << XDMAC_CNDC_NDSUP_Pos)  /**< (XDMAC_CNDC) Source parameters remain unchanged. Position  */
#define XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED (XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED_Val << XDMAC_CNDC_NDSUP_Pos)  /**< (XDMAC_CNDC) Source parameters are updated when the descriptor is retrieved. Position  */
#define XDMAC_CNDC_NDDUP_Pos                2                                              /**< (XDMAC_CNDC) Channel x Next Descriptor Destination Update Position */
#define XDMAC_CNDC_NDDUP_Msk                (0x1U << XDMAC_CNDC_NDDUP_Pos)                 /**< (XDMAC_CNDC) Channel x Next Descriptor Destination Update Mask */
#define XDMAC_CNDC_NDDUP                    XDMAC_CNDC_NDDUP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CNDC_NDDUP_Msk instead */
#define   XDMAC_CNDC_NDDUP_DST_PARAMS_UNCHANGED_Val (0x0U)                                         /**< (XDMAC_CNDC) Destination parameters remain unchanged.  */
#define   XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED_Val (0x1U)                                         /**< (XDMAC_CNDC) Destination parameters are updated when the descriptor is retrieved.  */
#define XDMAC_CNDC_NDDUP_DST_PARAMS_UNCHANGED (XDMAC_CNDC_NDDUP_DST_PARAMS_UNCHANGED_Val << XDMAC_CNDC_NDDUP_Pos)  /**< (XDMAC_CNDC) Destination parameters remain unchanged. Position  */
#define XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED (XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED_Val << XDMAC_CNDC_NDDUP_Pos)  /**< (XDMAC_CNDC) Destination parameters are updated when the descriptor is retrieved. Position  */
#define XDMAC_CNDC_NDVIEW_Pos               3                                              /**< (XDMAC_CNDC) Channel x Next Descriptor View Position */
#define XDMAC_CNDC_NDVIEW_Msk               (0x3U << XDMAC_CNDC_NDVIEW_Pos)                /**< (XDMAC_CNDC) Channel x Next Descriptor View Mask */
#define XDMAC_CNDC_NDVIEW(value)            (XDMAC_CNDC_NDVIEW_Msk & ((value) << XDMAC_CNDC_NDVIEW_Pos))
#define   XDMAC_CNDC_NDVIEW_NDV0_Val        (0x0U)                                         /**< (XDMAC_CNDC) Next Descriptor View 0  */
#define   XDMAC_CNDC_NDVIEW_NDV1_Val        (0x1U)                                         /**< (XDMAC_CNDC) Next Descriptor View 1  */
#define   XDMAC_CNDC_NDVIEW_NDV2_Val        (0x2U)                                         /**< (XDMAC_CNDC) Next Descriptor View 2  */
#define   XDMAC_CNDC_NDVIEW_NDV3_Val        (0x3U)                                         /**< (XDMAC_CNDC) Next Descriptor View 3  */
#define XDMAC_CNDC_NDVIEW_NDV0              (XDMAC_CNDC_NDVIEW_NDV0_Val << XDMAC_CNDC_NDVIEW_Pos)  /**< (XDMAC_CNDC) Next Descriptor View 0 Position  */
#define XDMAC_CNDC_NDVIEW_NDV1              (XDMAC_CNDC_NDVIEW_NDV1_Val << XDMAC_CNDC_NDVIEW_Pos)  /**< (XDMAC_CNDC) Next Descriptor View 1 Position  */
#define XDMAC_CNDC_NDVIEW_NDV2              (XDMAC_CNDC_NDVIEW_NDV2_Val << XDMAC_CNDC_NDVIEW_Pos)  /**< (XDMAC_CNDC) Next Descriptor View 2 Position  */
#define XDMAC_CNDC_NDVIEW_NDV3              (XDMAC_CNDC_NDVIEW_NDV3_Val << XDMAC_CNDC_NDVIEW_Pos)  /**< (XDMAC_CNDC) Next Descriptor View 3 Position  */
#define XDMAC_CNDC_MASK                     (0x1FU)                                        /**< \deprecated (XDMAC_CNDC) Register MASK  (Use XDMAC_CNDC_Msk instead)  */
#define XDMAC_CNDC_Msk                      (0x1FU)                                        /**< (XDMAC_CNDC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CUBC : (XDMAC Offset: 0x20) (R/W 32) Channel Microblock Control Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t UBLEN:24;                  /**< bit:  0..23  Channel x Microblock Length              */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CUBC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CUBC_OFFSET                   (0x20)                                        /**<  (XDMAC_CUBC) Channel Microblock Control Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CUBC_UBLEN_Pos                0                                              /**< (XDMAC_CUBC) Channel x Microblock Length Position */
#define XDMAC_CUBC_UBLEN_Msk                (0xFFFFFFU << XDMAC_CUBC_UBLEN_Pos)            /**< (XDMAC_CUBC) Channel x Microblock Length Mask */
#define XDMAC_CUBC_UBLEN(value)             (XDMAC_CUBC_UBLEN_Msk & ((value) << XDMAC_CUBC_UBLEN_Pos))
#define XDMAC_CUBC_MASK                     (0xFFFFFFU)                                    /**< \deprecated (XDMAC_CUBC) Register MASK  (Use XDMAC_CUBC_Msk instead)  */
#define XDMAC_CUBC_Msk                      (0xFFFFFFU)                                    /**< (XDMAC_CUBC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CBC : (XDMAC Offset: 0x24) (R/W 32) Channel Block Control Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BLEN:12;                   /**< bit:  0..11  Channel x Block Length                   */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CBC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CBC_OFFSET                    (0x24)                                        /**<  (XDMAC_CBC) Channel Block Control Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CBC_BLEN_Pos                  0                                              /**< (XDMAC_CBC) Channel x Block Length Position */
#define XDMAC_CBC_BLEN_Msk                  (0xFFFU << XDMAC_CBC_BLEN_Pos)                 /**< (XDMAC_CBC) Channel x Block Length Mask */
#define XDMAC_CBC_BLEN(value)               (XDMAC_CBC_BLEN_Msk & ((value) << XDMAC_CBC_BLEN_Pos))
#define XDMAC_CBC_MASK                      (0xFFFU)                                       /**< \deprecated (XDMAC_CBC) Register MASK  (Use XDMAC_CBC_Msk instead)  */
#define XDMAC_CBC_Msk                       (0xFFFU)                                       /**< (XDMAC_CBC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CC : (XDMAC Offset: 0x28) (R/W 32) Channel Configuration Register (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TYPE:1;                    /**< bit:      0  Channel x Transfer Type                  */
    uint32_t MBSIZE:2;                  /**< bit:   1..2  Channel x Memory Burst Size              */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t DSYNC:1;                   /**< bit:      4  Channel x Synchronization                */
    uint32_t :1;                        /**< bit:      5  Reserved */
    uint32_t SWREQ:1;                   /**< bit:      6  Channel x Software Request Trigger       */
    uint32_t MEMSET:1;                  /**< bit:      7  Channel x Fill Block of memory           */
    uint32_t CSIZE:3;                   /**< bit:  8..10  Channel x Chunk Size                     */
    uint32_t DWIDTH:2;                  /**< bit: 11..12  Channel x Data Width                     */
    uint32_t SIF:1;                     /**< bit:     13  Channel x Source Interface Identifier    */
    uint32_t DIF:1;                     /**< bit:     14  Channel x Destination Interface Identifier */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t SAM:2;                     /**< bit: 16..17  Channel x Source Addressing Mode         */
    uint32_t DAM:2;                     /**< bit: 18..19  Channel x Destination Addressing Mode    */
    uint32_t :1;                        /**< bit:     20  Reserved */
    uint32_t INITD:1;                   /**< bit:     21  Channel Initialization Terminated (this bit is read-only) */
    uint32_t RDIP:1;                    /**< bit:     22  Read in Progress (this bit is read-only) */
    uint32_t WRIP:1;                    /**< bit:     23  Write in Progress (this bit is read-only) */
    uint32_t PERID:7;                   /**< bit: 24..30  Channel x Peripheral Hardware Request Line Identifier */
    uint32_t :1;                        /**< bit:     31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CC_OFFSET                     (0x28)                                        /**<  (XDMAC_CC) Channel Configuration Register (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CC_TYPE_Pos                   0                                              /**< (XDMAC_CC) Channel x Transfer Type Position */
#define XDMAC_CC_TYPE_Msk                   (0x1U << XDMAC_CC_TYPE_Pos)                    /**< (XDMAC_CC) Channel x Transfer Type Mask */
#define XDMAC_CC_TYPE                       XDMAC_CC_TYPE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_TYPE_Msk instead */
#define   XDMAC_CC_TYPE_MEM_TRAN_Val        (0x0U)                                         /**< (XDMAC_CC) Self triggered mode (Memory to Memory Transfer).  */
#define   XDMAC_CC_TYPE_PER_TRAN_Val        (0x1U)                                         /**< (XDMAC_CC) Synchronized mode (Peripheral to Memory or Memory to Peripheral Transfer).  */
#define XDMAC_CC_TYPE_MEM_TRAN              (XDMAC_CC_TYPE_MEM_TRAN_Val << XDMAC_CC_TYPE_Pos)  /**< (XDMAC_CC) Self triggered mode (Memory to Memory Transfer). Position  */
#define XDMAC_CC_TYPE_PER_TRAN              (XDMAC_CC_TYPE_PER_TRAN_Val << XDMAC_CC_TYPE_Pos)  /**< (XDMAC_CC) Synchronized mode (Peripheral to Memory or Memory to Peripheral Transfer). Position  */
#define XDMAC_CC_MBSIZE_Pos                 1                                              /**< (XDMAC_CC) Channel x Memory Burst Size Position */
#define XDMAC_CC_MBSIZE_Msk                 (0x3U << XDMAC_CC_MBSIZE_Pos)                  /**< (XDMAC_CC) Channel x Memory Burst Size Mask */
#define XDMAC_CC_MBSIZE(value)              (XDMAC_CC_MBSIZE_Msk & ((value) << XDMAC_CC_MBSIZE_Pos))
#define   XDMAC_CC_MBSIZE_SINGLE_Val        (0x0U)                                         /**< (XDMAC_CC) The memory burst size is set to one.  */
#define   XDMAC_CC_MBSIZE_FOUR_Val          (0x1U)                                         /**< (XDMAC_CC) The memory burst size is set to four.  */
#define   XDMAC_CC_MBSIZE_EIGHT_Val         (0x2U)                                         /**< (XDMAC_CC) The memory burst size is set to eight.  */
#define   XDMAC_CC_MBSIZE_SIXTEEN_Val       (0x3U)                                         /**< (XDMAC_CC) The memory burst size is set to sixteen.  */
#define XDMAC_CC_MBSIZE_SINGLE              (XDMAC_CC_MBSIZE_SINGLE_Val << XDMAC_CC_MBSIZE_Pos)  /**< (XDMAC_CC) The memory burst size is set to one. Position  */
#define XDMAC_CC_MBSIZE_FOUR                (XDMAC_CC_MBSIZE_FOUR_Val << XDMAC_CC_MBSIZE_Pos)  /**< (XDMAC_CC) The memory burst size is set to four. Position  */
#define XDMAC_CC_MBSIZE_EIGHT               (XDMAC_CC_MBSIZE_EIGHT_Val << XDMAC_CC_MBSIZE_Pos)  /**< (XDMAC_CC) The memory burst size is set to eight. Position  */
#define XDMAC_CC_MBSIZE_SIXTEEN             (XDMAC_CC_MBSIZE_SIXTEEN_Val << XDMAC_CC_MBSIZE_Pos)  /**< (XDMAC_CC) The memory burst size is set to sixteen. Position  */
#define XDMAC_CC_DSYNC_Pos                  4                                              /**< (XDMAC_CC) Channel x Synchronization Position */
#define XDMAC_CC_DSYNC_Msk                  (0x1U << XDMAC_CC_DSYNC_Pos)                   /**< (XDMAC_CC) Channel x Synchronization Mask */
#define XDMAC_CC_DSYNC                      XDMAC_CC_DSYNC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_DSYNC_Msk instead */
#define   XDMAC_CC_DSYNC_PER2MEM_Val        (0x0U)                                         /**< (XDMAC_CC) Peripheral to Memory transfer.  */
#define   XDMAC_CC_DSYNC_MEM2PER_Val        (0x1U)                                         /**< (XDMAC_CC) Memory to Peripheral transfer.  */
#define XDMAC_CC_DSYNC_PER2MEM              (XDMAC_CC_DSYNC_PER2MEM_Val << XDMAC_CC_DSYNC_Pos)  /**< (XDMAC_CC) Peripheral to Memory transfer. Position  */
#define XDMAC_CC_DSYNC_MEM2PER              (XDMAC_CC_DSYNC_MEM2PER_Val << XDMAC_CC_DSYNC_Pos)  /**< (XDMAC_CC) Memory to Peripheral transfer. Position  */
#define XDMAC_CC_SWREQ_Pos                  6                                              /**< (XDMAC_CC) Channel x Software Request Trigger Position */
#define XDMAC_CC_SWREQ_Msk                  (0x1U << XDMAC_CC_SWREQ_Pos)                   /**< (XDMAC_CC) Channel x Software Request Trigger Mask */
#define XDMAC_CC_SWREQ                      XDMAC_CC_SWREQ_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_SWREQ_Msk instead */
#define   XDMAC_CC_SWREQ_HWR_CONNECTED_Val  (0x0U)                                         /**< (XDMAC_CC) Hardware request line is connected to the peripheral request line.  */
#define   XDMAC_CC_SWREQ_SWR_CONNECTED_Val  (0x1U)                                         /**< (XDMAC_CC) Software request is connected to the peripheral request line.  */
#define XDMAC_CC_SWREQ_HWR_CONNECTED        (XDMAC_CC_SWREQ_HWR_CONNECTED_Val << XDMAC_CC_SWREQ_Pos)  /**< (XDMAC_CC) Hardware request line is connected to the peripheral request line. Position  */
#define XDMAC_CC_SWREQ_SWR_CONNECTED        (XDMAC_CC_SWREQ_SWR_CONNECTED_Val << XDMAC_CC_SWREQ_Pos)  /**< (XDMAC_CC) Software request is connected to the peripheral request line. Position  */
#define XDMAC_CC_MEMSET_Pos                 7                                              /**< (XDMAC_CC) Channel x Fill Block of memory Position */
#define XDMAC_CC_MEMSET_Msk                 (0x1U << XDMAC_CC_MEMSET_Pos)                  /**< (XDMAC_CC) Channel x Fill Block of memory Mask */
#define XDMAC_CC_MEMSET                     XDMAC_CC_MEMSET_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_MEMSET_Msk instead */
#define   XDMAC_CC_MEMSET_NORMAL_MODE_Val   (0x0U)                                         /**< (XDMAC_CC) Memset is not activated.  */
#define   XDMAC_CC_MEMSET_HW_MODE_Val       (0x1U)                                         /**< (XDMAC_CC) Sets the block of memory pointed by DA field to the specified value. This operation is performed on 8, 16 or 32 bits basis.  */
#define XDMAC_CC_MEMSET_NORMAL_MODE         (XDMAC_CC_MEMSET_NORMAL_MODE_Val << XDMAC_CC_MEMSET_Pos)  /**< (XDMAC_CC) Memset is not activated. Position  */
#define XDMAC_CC_MEMSET_HW_MODE             (XDMAC_CC_MEMSET_HW_MODE_Val << XDMAC_CC_MEMSET_Pos)  /**< (XDMAC_CC) Sets the block of memory pointed by DA field to the specified value. This operation is performed on 8, 16 or 32 bits basis. Position  */
#define XDMAC_CC_CSIZE_Pos                  8                                              /**< (XDMAC_CC) Channel x Chunk Size Position */
#define XDMAC_CC_CSIZE_Msk                  (0x7U << XDMAC_CC_CSIZE_Pos)                   /**< (XDMAC_CC) Channel x Chunk Size Mask */
#define XDMAC_CC_CSIZE(value)               (XDMAC_CC_CSIZE_Msk & ((value) << XDMAC_CC_CSIZE_Pos))
#define   XDMAC_CC_CSIZE_CHK_1_Val          (0x0U)                                         /**< (XDMAC_CC) 1 data transferred  */
#define   XDMAC_CC_CSIZE_CHK_2_Val          (0x1U)                                         /**< (XDMAC_CC) 2 data transferred  */
#define   XDMAC_CC_CSIZE_CHK_4_Val          (0x2U)                                         /**< (XDMAC_CC) 4 data transferred  */
#define   XDMAC_CC_CSIZE_CHK_8_Val          (0x3U)                                         /**< (XDMAC_CC) 8 data transferred  */
#define   XDMAC_CC_CSIZE_CHK_16_Val         (0x4U)                                         /**< (XDMAC_CC) 16 data transferred  */
#define XDMAC_CC_CSIZE_CHK_1                (XDMAC_CC_CSIZE_CHK_1_Val << XDMAC_CC_CSIZE_Pos)  /**< (XDMAC_CC) 1 data transferred Position  */
#define XDMAC_CC_CSIZE_CHK_2                (XDMAC_CC_CSIZE_CHK_2_Val << XDMAC_CC_CSIZE_Pos)  /**< (XDMAC_CC) 2 data transferred Position  */
#define XDMAC_CC_CSIZE_CHK_4                (XDMAC_CC_CSIZE_CHK_4_Val << XDMAC_CC_CSIZE_Pos)  /**< (XDMAC_CC) 4 data transferred Position  */
#define XDMAC_CC_CSIZE_CHK_8                (XDMAC_CC_CSIZE_CHK_8_Val << XDMAC_CC_CSIZE_Pos)  /**< (XDMAC_CC) 8 data transferred Position  */
#define XDMAC_CC_CSIZE_CHK_16               (XDMAC_CC_CSIZE_CHK_16_Val << XDMAC_CC_CSIZE_Pos)  /**< (XDMAC_CC) 16 data transferred Position  */
#define XDMAC_CC_DWIDTH_Pos                 11                                             /**< (XDMAC_CC) Channel x Data Width Position */
#define XDMAC_CC_DWIDTH_Msk                 (0x3U << XDMAC_CC_DWIDTH_Pos)                  /**< (XDMAC_CC) Channel x Data Width Mask */
#define XDMAC_CC_DWIDTH(value)              (XDMAC_CC_DWIDTH_Msk & ((value) << XDMAC_CC_DWIDTH_Pos))
#define   XDMAC_CC_DWIDTH_BYTE_Val          (0x0U)                                         /**< (XDMAC_CC) The data size is set to 8 bits  */
#define   XDMAC_CC_DWIDTH_HALFWORD_Val      (0x1U)                                         /**< (XDMAC_CC) The data size is set to 16 bits  */
#define   XDMAC_CC_DWIDTH_WORD_Val          (0x2U)                                         /**< (XDMAC_CC) The data size is set to 32 bits  */
#define XDMAC_CC_DWIDTH_BYTE                (XDMAC_CC_DWIDTH_BYTE_Val << XDMAC_CC_DWIDTH_Pos)  /**< (XDMAC_CC) The data size is set to 8 bits Position  */
#define XDMAC_CC_DWIDTH_HALFWORD            (XDMAC_CC_DWIDTH_HALFWORD_Val << XDMAC_CC_DWIDTH_Pos)  /**< (XDMAC_CC) The data size is set to 16 bits Position  */
#define XDMAC_CC_DWIDTH_WORD                (XDMAC_CC_DWIDTH_WORD_Val << XDMAC_CC_DWIDTH_Pos)  /**< (XDMAC_CC) The data size is set to 32 bits Position  */
#define XDMAC_CC_SIF_Pos                    13                                             /**< (XDMAC_CC) Channel x Source Interface Identifier Position */
#define XDMAC_CC_SIF_Msk                    (0x1U << XDMAC_CC_SIF_Pos)                     /**< (XDMAC_CC) Channel x Source Interface Identifier Mask */
#define XDMAC_CC_SIF                        XDMAC_CC_SIF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_SIF_Msk instead */
#define   XDMAC_CC_SIF_AHB_IF0_Val          (0x0U)                                         /**< (XDMAC_CC) The data is read through the system bus interface 0.  */
#define   XDMAC_CC_SIF_AHB_IF1_Val          (0x1U)                                         /**< (XDMAC_CC) The data is read through the system bus interface 1.  */
#define XDMAC_CC_SIF_AHB_IF0                (XDMAC_CC_SIF_AHB_IF0_Val << XDMAC_CC_SIF_Pos)  /**< (XDMAC_CC) The data is read through the system bus interface 0. Position  */
#define XDMAC_CC_SIF_AHB_IF1                (XDMAC_CC_SIF_AHB_IF1_Val << XDMAC_CC_SIF_Pos)  /**< (XDMAC_CC) The data is read through the system bus interface 1. Position  */
#define XDMAC_CC_DIF_Pos                    14                                             /**< (XDMAC_CC) Channel x Destination Interface Identifier Position */
#define XDMAC_CC_DIF_Msk                    (0x1U << XDMAC_CC_DIF_Pos)                     /**< (XDMAC_CC) Channel x Destination Interface Identifier Mask */
#define XDMAC_CC_DIF                        XDMAC_CC_DIF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_DIF_Msk instead */
#define   XDMAC_CC_DIF_AHB_IF0_Val          (0x0U)                                         /**< (XDMAC_CC) The data is written through the system bus interface 0.  */
#define   XDMAC_CC_DIF_AHB_IF1_Val          (0x1U)                                         /**< (XDMAC_CC) The data is written though the system bus interface 1.  */
#define XDMAC_CC_DIF_AHB_IF0                (XDMAC_CC_DIF_AHB_IF0_Val << XDMAC_CC_DIF_Pos)  /**< (XDMAC_CC) The data is written through the system bus interface 0. Position  */
#define XDMAC_CC_DIF_AHB_IF1                (XDMAC_CC_DIF_AHB_IF1_Val << XDMAC_CC_DIF_Pos)  /**< (XDMAC_CC) The data is written though the system bus interface 1. Position  */
#define XDMAC_CC_SAM_Pos                    16                                             /**< (XDMAC_CC) Channel x Source Addressing Mode Position */
#define XDMAC_CC_SAM_Msk                    (0x3U << XDMAC_CC_SAM_Pos)                     /**< (XDMAC_CC) Channel x Source Addressing Mode Mask */
#define XDMAC_CC_SAM(value)                 (XDMAC_CC_SAM_Msk & ((value) << XDMAC_CC_SAM_Pos))
#define   XDMAC_CC_SAM_FIXED_AM_Val         (0x0U)                                         /**< (XDMAC_CC) The address remains unchanged.  */
#define   XDMAC_CC_SAM_INCREMENTED_AM_Val   (0x1U)                                         /**< (XDMAC_CC) The addressing mode is incremented (the increment size is set to the data size).  */
#define   XDMAC_CC_SAM_UBS_AM_Val           (0x2U)                                         /**< (XDMAC_CC) The microblock stride is added at the microblock boundary.  */
#define   XDMAC_CC_SAM_UBS_DS_AM_Val        (0x3U)                                         /**< (XDMAC_CC) The microblock stride is added at the microblock boundary, the data stride is added at the data boundary.  */
#define XDMAC_CC_SAM_FIXED_AM               (XDMAC_CC_SAM_FIXED_AM_Val << XDMAC_CC_SAM_Pos)  /**< (XDMAC_CC) The address remains unchanged. Position  */
#define XDMAC_CC_SAM_INCREMENTED_AM         (XDMAC_CC_SAM_INCREMENTED_AM_Val << XDMAC_CC_SAM_Pos)  /**< (XDMAC_CC) The addressing mode is incremented (the increment size is set to the data size). Position  */
#define XDMAC_CC_SAM_UBS_AM                 (XDMAC_CC_SAM_UBS_AM_Val << XDMAC_CC_SAM_Pos)  /**< (XDMAC_CC) The microblock stride is added at the microblock boundary. Position  */
#define XDMAC_CC_SAM_UBS_DS_AM              (XDMAC_CC_SAM_UBS_DS_AM_Val << XDMAC_CC_SAM_Pos)  /**< (XDMAC_CC) The microblock stride is added at the microblock boundary, the data stride is added at the data boundary. Position  */
#define XDMAC_CC_DAM_Pos                    18                                             /**< (XDMAC_CC) Channel x Destination Addressing Mode Position */
#define XDMAC_CC_DAM_Msk                    (0x3U << XDMAC_CC_DAM_Pos)                     /**< (XDMAC_CC) Channel x Destination Addressing Mode Mask */
#define XDMAC_CC_DAM(value)                 (XDMAC_CC_DAM_Msk & ((value) << XDMAC_CC_DAM_Pos))
#define   XDMAC_CC_DAM_FIXED_AM_Val         (0x0U)                                         /**< (XDMAC_CC) The address remains unchanged.  */
#define   XDMAC_CC_DAM_INCREMENTED_AM_Val   (0x1U)                                         /**< (XDMAC_CC) The addressing mode is incremented (the increment size is set to the data size).  */
#define   XDMAC_CC_DAM_UBS_AM_Val           (0x2U)                                         /**< (XDMAC_CC) The microblock stride is added at the microblock boundary.  */
#define   XDMAC_CC_DAM_UBS_DS_AM_Val        (0x3U)                                         /**< (XDMAC_CC) The microblock stride is added at the microblock boundary, the data stride is added at the data boundary.  */
#define XDMAC_CC_DAM_FIXED_AM               (XDMAC_CC_DAM_FIXED_AM_Val << XDMAC_CC_DAM_Pos)  /**< (XDMAC_CC) The address remains unchanged. Position  */
#define XDMAC_CC_DAM_INCREMENTED_AM         (XDMAC_CC_DAM_INCREMENTED_AM_Val << XDMAC_CC_DAM_Pos)  /**< (XDMAC_CC) The addressing mode is incremented (the increment size is set to the data size). Position  */
#define XDMAC_CC_DAM_UBS_AM                 (XDMAC_CC_DAM_UBS_AM_Val << XDMAC_CC_DAM_Pos)  /**< (XDMAC_CC) The microblock stride is added at the microblock boundary. Position  */
#define XDMAC_CC_DAM_UBS_DS_AM              (XDMAC_CC_DAM_UBS_DS_AM_Val << XDMAC_CC_DAM_Pos)  /**< (XDMAC_CC) The microblock stride is added at the microblock boundary, the data stride is added at the data boundary. Position  */
#define XDMAC_CC_INITD_Pos                  21                                             /**< (XDMAC_CC) Channel Initialization Terminated (this bit is read-only) Position */
#define XDMAC_CC_INITD_Msk                  (0x1U << XDMAC_CC_INITD_Pos)                   /**< (XDMAC_CC) Channel Initialization Terminated (this bit is read-only) Mask */
#define XDMAC_CC_INITD                      XDMAC_CC_INITD_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_INITD_Msk instead */
#define   XDMAC_CC_INITD_IN_PROGRESS_Val    (0x0U)                                         /**< (XDMAC_CC) Channel initialization is in progress.  */
#define   XDMAC_CC_INITD_TERMINATED_Val     (0x1U)                                         /**< (XDMAC_CC) Channel initialization is completed.  */
#define XDMAC_CC_INITD_IN_PROGRESS          (XDMAC_CC_INITD_IN_PROGRESS_Val << XDMAC_CC_INITD_Pos)  /**< (XDMAC_CC) Channel initialization is in progress. Position  */
#define XDMAC_CC_INITD_TERMINATED           (XDMAC_CC_INITD_TERMINATED_Val << XDMAC_CC_INITD_Pos)  /**< (XDMAC_CC) Channel initialization is completed. Position  */
#define XDMAC_CC_RDIP_Pos                   22                                             /**< (XDMAC_CC) Read in Progress (this bit is read-only) Position */
#define XDMAC_CC_RDIP_Msk                   (0x1U << XDMAC_CC_RDIP_Pos)                    /**< (XDMAC_CC) Read in Progress (this bit is read-only) Mask */
#define XDMAC_CC_RDIP                       XDMAC_CC_RDIP_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_RDIP_Msk instead */
#define   XDMAC_CC_RDIP_DONE_Val            (0x0U)                                         /**< (XDMAC_CC) No Active read transaction on the bus.  */
#define   XDMAC_CC_RDIP_IN_PROGRESS_Val     (0x1U)                                         /**< (XDMAC_CC) A read transaction is in progress.  */
#define XDMAC_CC_RDIP_DONE                  (XDMAC_CC_RDIP_DONE_Val << XDMAC_CC_RDIP_Pos)  /**< (XDMAC_CC) No Active read transaction on the bus. Position  */
#define XDMAC_CC_RDIP_IN_PROGRESS           (XDMAC_CC_RDIP_IN_PROGRESS_Val << XDMAC_CC_RDIP_Pos)  /**< (XDMAC_CC) A read transaction is in progress. Position  */
#define XDMAC_CC_WRIP_Pos                   23                                             /**< (XDMAC_CC) Write in Progress (this bit is read-only) Position */
#define XDMAC_CC_WRIP_Msk                   (0x1U << XDMAC_CC_WRIP_Pos)                    /**< (XDMAC_CC) Write in Progress (this bit is read-only) Mask */
#define XDMAC_CC_WRIP                       XDMAC_CC_WRIP_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_CC_WRIP_Msk instead */
#define   XDMAC_CC_WRIP_DONE_Val            (0x0U)                                         /**< (XDMAC_CC) No Active write transaction on the bus.  */
#define   XDMAC_CC_WRIP_IN_PROGRESS_Val     (0x1U)                                         /**< (XDMAC_CC) A Write transaction is in progress.  */
#define XDMAC_CC_WRIP_DONE                  (XDMAC_CC_WRIP_DONE_Val << XDMAC_CC_WRIP_Pos)  /**< (XDMAC_CC) No Active write transaction on the bus. Position  */
#define XDMAC_CC_WRIP_IN_PROGRESS           (XDMAC_CC_WRIP_IN_PROGRESS_Val << XDMAC_CC_WRIP_Pos)  /**< (XDMAC_CC) A Write transaction is in progress. Position  */
#define XDMAC_CC_PERID_Pos                  24                                             /**< (XDMAC_CC) Channel x Peripheral Hardware Request Line Identifier Position */
#define XDMAC_CC_PERID_Msk                  (0x7FU << XDMAC_CC_PERID_Pos)                  /**< (XDMAC_CC) Channel x Peripheral Hardware Request Line Identifier Mask */
#define XDMAC_CC_PERID(value)               (XDMAC_CC_PERID_Msk & ((value) << XDMAC_CC_PERID_Pos))
#define XDMAC_CC_MASK                       (0x7FEF7FD7U)                                  /**< \deprecated (XDMAC_CC) Register MASK  (Use XDMAC_CC_Msk instead)  */
#define XDMAC_CC_Msk                        (0x7FEF7FD7U)                                  /**< (XDMAC_CC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CDS_MSP : (XDMAC Offset: 0x2c) (R/W 32) Channel Data Stride Memory Set Pattern (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SDS_MSP:16;                /**< bit:  0..15  Channel x Source Data stride or Memory Set Pattern */
    uint32_t DDS_MSP:16;                /**< bit: 16..31  Channel x Destination Data Stride or Memory Set Pattern */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CDS_MSP_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CDS_MSP_OFFSET                (0x2C)                                        /**<  (XDMAC_CDS_MSP) Channel Data Stride Memory Set Pattern (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CDS_MSP_SDS_MSP_Pos           0                                              /**< (XDMAC_CDS_MSP) Channel x Source Data stride or Memory Set Pattern Position */
#define XDMAC_CDS_MSP_SDS_MSP_Msk           (0xFFFFU << XDMAC_CDS_MSP_SDS_MSP_Pos)         /**< (XDMAC_CDS_MSP) Channel x Source Data stride or Memory Set Pattern Mask */
#define XDMAC_CDS_MSP_SDS_MSP(value)        (XDMAC_CDS_MSP_SDS_MSP_Msk & ((value) << XDMAC_CDS_MSP_SDS_MSP_Pos))
#define XDMAC_CDS_MSP_DDS_MSP_Pos           16                                             /**< (XDMAC_CDS_MSP) Channel x Destination Data Stride or Memory Set Pattern Position */
#define XDMAC_CDS_MSP_DDS_MSP_Msk           (0xFFFFU << XDMAC_CDS_MSP_DDS_MSP_Pos)         /**< (XDMAC_CDS_MSP) Channel x Destination Data Stride or Memory Set Pattern Mask */
#define XDMAC_CDS_MSP_DDS_MSP(value)        (XDMAC_CDS_MSP_DDS_MSP_Msk & ((value) << XDMAC_CDS_MSP_DDS_MSP_Pos))
#define XDMAC_CDS_MSP_MASK                  (0xFFFFFFFFU)                                  /**< \deprecated (XDMAC_CDS_MSP) Register MASK  (Use XDMAC_CDS_MSP_Msk instead)  */
#define XDMAC_CDS_MSP_Msk                   (0xFFFFFFFFU)                                  /**< (XDMAC_CDS_MSP) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CSUS : (XDMAC Offset: 0x30) (R/W 32) Channel Source Microblock Stride (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SUBS:24;                   /**< bit:  0..23  Channel x Source Microblock Stride       */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CSUS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CSUS_OFFSET                   (0x30)                                        /**<  (XDMAC_CSUS) Channel Source Microblock Stride (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CSUS_SUBS_Pos                 0                                              /**< (XDMAC_CSUS) Channel x Source Microblock Stride Position */
#define XDMAC_CSUS_SUBS_Msk                 (0xFFFFFFU << XDMAC_CSUS_SUBS_Pos)             /**< (XDMAC_CSUS) Channel x Source Microblock Stride Mask */
#define XDMAC_CSUS_SUBS(value)              (XDMAC_CSUS_SUBS_Msk & ((value) << XDMAC_CSUS_SUBS_Pos))
#define XDMAC_CSUS_MASK                     (0xFFFFFFU)                                    /**< \deprecated (XDMAC_CSUS) Register MASK  (Use XDMAC_CSUS_Msk instead)  */
#define XDMAC_CSUS_Msk                      (0xFFFFFFU)                                    /**< (XDMAC_CSUS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_CDUS : (XDMAC Offset: 0x34) (R/W 32) Channel Destination Microblock Stride (chid = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DUBS:24;                   /**< bit:  0..23  Channel x Destination Microblock Stride  */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_CDUS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_CDUS_OFFSET                   (0x34)                                        /**<  (XDMAC_CDUS) Channel Destination Microblock Stride (chid = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_CDUS_DUBS_Pos                 0                                              /**< (XDMAC_CDUS) Channel x Destination Microblock Stride Position */
#define XDMAC_CDUS_DUBS_Msk                 (0xFFFFFFU << XDMAC_CDUS_DUBS_Pos)             /**< (XDMAC_CDUS) Channel x Destination Microblock Stride Mask */
#define XDMAC_CDUS_DUBS(value)              (XDMAC_CDUS_DUBS_Msk & ((value) << XDMAC_CDUS_DUBS_Pos))
#define XDMAC_CDUS_MASK                     (0xFFFFFFU)                                    /**< \deprecated (XDMAC_CDUS) Register MASK  (Use XDMAC_CDUS_Msk instead)  */
#define XDMAC_CDUS_Msk                      (0xFFFFFFU)                                    /**< (XDMAC_CDUS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GTYPE : (XDMAC Offset: 0x00) (R/W 32) Global Type Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NB_CH:5;                   /**< bit:   0..4  Number of Channels Minus One             */
    uint32_t FIFO_SZ:11;                /**< bit:  5..15  Number of Bytes                          */
    uint32_t NB_REQ:7;                  /**< bit: 16..22  Number of Peripheral Requests Minus One  */
    uint32_t :9;                        /**< bit: 23..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GTYPE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GTYPE_OFFSET                  (0x00)                                        /**<  (XDMAC_GTYPE) Global Type Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GTYPE_NB_CH_Pos               0                                              /**< (XDMAC_GTYPE) Number of Channels Minus One Position */
#define XDMAC_GTYPE_NB_CH_Msk               (0x1FU << XDMAC_GTYPE_NB_CH_Pos)               /**< (XDMAC_GTYPE) Number of Channels Minus One Mask */
#define XDMAC_GTYPE_NB_CH(value)            (XDMAC_GTYPE_NB_CH_Msk & ((value) << XDMAC_GTYPE_NB_CH_Pos))
#define XDMAC_GTYPE_FIFO_SZ_Pos             5                                              /**< (XDMAC_GTYPE) Number of Bytes Position */
#define XDMAC_GTYPE_FIFO_SZ_Msk             (0x7FFU << XDMAC_GTYPE_FIFO_SZ_Pos)            /**< (XDMAC_GTYPE) Number of Bytes Mask */
#define XDMAC_GTYPE_FIFO_SZ(value)          (XDMAC_GTYPE_FIFO_SZ_Msk & ((value) << XDMAC_GTYPE_FIFO_SZ_Pos))
#define XDMAC_GTYPE_NB_REQ_Pos              16                                             /**< (XDMAC_GTYPE) Number of Peripheral Requests Minus One Position */
#define XDMAC_GTYPE_NB_REQ_Msk              (0x7FU << XDMAC_GTYPE_NB_REQ_Pos)              /**< (XDMAC_GTYPE) Number of Peripheral Requests Minus One Mask */
#define XDMAC_GTYPE_NB_REQ(value)           (XDMAC_GTYPE_NB_REQ_Msk & ((value) << XDMAC_GTYPE_NB_REQ_Pos))
#define XDMAC_GTYPE_MASK                    (0x7FFFFFU)                                    /**< \deprecated (XDMAC_GTYPE) Register MASK  (Use XDMAC_GTYPE_Msk instead)  */
#define XDMAC_GTYPE_Msk                     (0x7FFFFFU)                                    /**< (XDMAC_GTYPE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GCFG : (XDMAC Offset: 0x04) (R/ 32) Global Configuration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CGDISREG:1;                /**< bit:      0  Configuration Registers Clock Gating Disable */
    uint32_t CGDISPIPE:1;               /**< bit:      1  Pipeline Clock Gating Disable            */
    uint32_t CGDISFIFO:1;               /**< bit:      2  FIFO Clock Gating Disable                */
    uint32_t CGDISIF:1;                 /**< bit:      3  Bus Interface Clock Gating Disable       */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t BXKBEN:1;                  /**< bit:      8  Boundary X Kilobyte Enable               */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GCFG_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GCFG_OFFSET                   (0x04)                                        /**<  (XDMAC_GCFG) Global Configuration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GCFG_CGDISREG_Pos             0                                              /**< (XDMAC_GCFG) Configuration Registers Clock Gating Disable Position */
#define XDMAC_GCFG_CGDISREG_Msk             (0x1U << XDMAC_GCFG_CGDISREG_Pos)              /**< (XDMAC_GCFG) Configuration Registers Clock Gating Disable Mask */
#define XDMAC_GCFG_CGDISREG                 XDMAC_GCFG_CGDISREG_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GCFG_CGDISREG_Msk instead */
#define XDMAC_GCFG_CGDISPIPE_Pos            1                                              /**< (XDMAC_GCFG) Pipeline Clock Gating Disable Position */
#define XDMAC_GCFG_CGDISPIPE_Msk            (0x1U << XDMAC_GCFG_CGDISPIPE_Pos)             /**< (XDMAC_GCFG) Pipeline Clock Gating Disable Mask */
#define XDMAC_GCFG_CGDISPIPE                XDMAC_GCFG_CGDISPIPE_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GCFG_CGDISPIPE_Msk instead */
#define XDMAC_GCFG_CGDISFIFO_Pos            2                                              /**< (XDMAC_GCFG) FIFO Clock Gating Disable Position */
#define XDMAC_GCFG_CGDISFIFO_Msk            (0x1U << XDMAC_GCFG_CGDISFIFO_Pos)             /**< (XDMAC_GCFG) FIFO Clock Gating Disable Mask */
#define XDMAC_GCFG_CGDISFIFO                XDMAC_GCFG_CGDISFIFO_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GCFG_CGDISFIFO_Msk instead */
#define XDMAC_GCFG_CGDISIF_Pos              3                                              /**< (XDMAC_GCFG) Bus Interface Clock Gating Disable Position */
#define XDMAC_GCFG_CGDISIF_Msk              (0x1U << XDMAC_GCFG_CGDISIF_Pos)               /**< (XDMAC_GCFG) Bus Interface Clock Gating Disable Mask */
#define XDMAC_GCFG_CGDISIF                  XDMAC_GCFG_CGDISIF_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GCFG_CGDISIF_Msk instead */
#define XDMAC_GCFG_BXKBEN_Pos               8                                              /**< (XDMAC_GCFG) Boundary X Kilobyte Enable Position */
#define XDMAC_GCFG_BXKBEN_Msk               (0x1U << XDMAC_GCFG_BXKBEN_Pos)                /**< (XDMAC_GCFG) Boundary X Kilobyte Enable Mask */
#define XDMAC_GCFG_BXKBEN                   XDMAC_GCFG_BXKBEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GCFG_BXKBEN_Msk instead */
#define XDMAC_GCFG_MASK                     (0x10FU)                                       /**< \deprecated (XDMAC_GCFG) Register MASK  (Use XDMAC_GCFG_Msk instead)  */
#define XDMAC_GCFG_Msk                      (0x10FU)                                       /**< (XDMAC_GCFG) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GWAC : (XDMAC Offset: 0x08) (R/W 32) Global Weighted Arbiter Configuration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PW0:4;                     /**< bit:   0..3  Pool Weight 0                            */
    uint32_t PW1:4;                     /**< bit:   4..7  Pool Weight 1                            */
    uint32_t PW2:4;                     /**< bit:  8..11  Pool Weight 2                            */
    uint32_t PW3:4;                     /**< bit: 12..15  Pool Weight 3                            */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GWAC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GWAC_OFFSET                   (0x08)                                        /**<  (XDMAC_GWAC) Global Weighted Arbiter Configuration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GWAC_PW0_Pos                  0                                              /**< (XDMAC_GWAC) Pool Weight 0 Position */
#define XDMAC_GWAC_PW0_Msk                  (0xFU << XDMAC_GWAC_PW0_Pos)                   /**< (XDMAC_GWAC) Pool Weight 0 Mask */
#define XDMAC_GWAC_PW0(value)               (XDMAC_GWAC_PW0_Msk & ((value) << XDMAC_GWAC_PW0_Pos))
#define XDMAC_GWAC_PW1_Pos                  4                                              /**< (XDMAC_GWAC) Pool Weight 1 Position */
#define XDMAC_GWAC_PW1_Msk                  (0xFU << XDMAC_GWAC_PW1_Pos)                   /**< (XDMAC_GWAC) Pool Weight 1 Mask */
#define XDMAC_GWAC_PW1(value)               (XDMAC_GWAC_PW1_Msk & ((value) << XDMAC_GWAC_PW1_Pos))
#define XDMAC_GWAC_PW2_Pos                  8                                              /**< (XDMAC_GWAC) Pool Weight 2 Position */
#define XDMAC_GWAC_PW2_Msk                  (0xFU << XDMAC_GWAC_PW2_Pos)                   /**< (XDMAC_GWAC) Pool Weight 2 Mask */
#define XDMAC_GWAC_PW2(value)               (XDMAC_GWAC_PW2_Msk & ((value) << XDMAC_GWAC_PW2_Pos))
#define XDMAC_GWAC_PW3_Pos                  12                                             /**< (XDMAC_GWAC) Pool Weight 3 Position */
#define XDMAC_GWAC_PW3_Msk                  (0xFU << XDMAC_GWAC_PW3_Pos)                   /**< (XDMAC_GWAC) Pool Weight 3 Mask */
#define XDMAC_GWAC_PW3(value)               (XDMAC_GWAC_PW3_Msk & ((value) << XDMAC_GWAC_PW3_Pos))
#define XDMAC_GWAC_MASK                     (0xFFFFU)                                      /**< \deprecated (XDMAC_GWAC) Register MASK  (Use XDMAC_GWAC_Msk instead)  */
#define XDMAC_GWAC_Msk                      (0xFFFFU)                                      /**< (XDMAC_GWAC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GIE : (XDMAC Offset: 0x0c) (/W 32) Global Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IE0:1;                     /**< bit:      0  XDMAC Channel 0 Interrupt Enable Bit     */
    uint32_t IE1:1;                     /**< bit:      1  XDMAC Channel 1 Interrupt Enable Bit     */
    uint32_t IE2:1;                     /**< bit:      2  XDMAC Channel 2 Interrupt Enable Bit     */
    uint32_t IE3:1;                     /**< bit:      3  XDMAC Channel 3 Interrupt Enable Bit     */
    uint32_t IE4:1;                     /**< bit:      4  XDMAC Channel 4 Interrupt Enable Bit     */
    uint32_t IE5:1;                     /**< bit:      5  XDMAC Channel 5 Interrupt Enable Bit     */
    uint32_t IE6:1;                     /**< bit:      6  XDMAC Channel 6 Interrupt Enable Bit     */
    uint32_t IE7:1;                     /**< bit:      7  XDMAC Channel 7 Interrupt Enable Bit     */
    uint32_t IE8:1;                     /**< bit:      8  XDMAC Channel 8 Interrupt Enable Bit     */
    uint32_t IE9:1;                     /**< bit:      9  XDMAC Channel 9 Interrupt Enable Bit     */
    uint32_t IE10:1;                    /**< bit:     10  XDMAC Channel 10 Interrupt Enable Bit    */
    uint32_t IE11:1;                    /**< bit:     11  XDMAC Channel 11 Interrupt Enable Bit    */
    uint32_t IE12:1;                    /**< bit:     12  XDMAC Channel 12 Interrupt Enable Bit    */
    uint32_t IE13:1;                    /**< bit:     13  XDMAC Channel 13 Interrupt Enable Bit    */
    uint32_t IE14:1;                    /**< bit:     14  XDMAC Channel 14 Interrupt Enable Bit    */
    uint32_t IE15:1;                    /**< bit:     15  XDMAC Channel 15 Interrupt Enable Bit    */
    uint32_t IE16:1;                    /**< bit:     16  XDMAC Channel 16 Interrupt Enable Bit    */
    uint32_t IE17:1;                    /**< bit:     17  XDMAC Channel 17 Interrupt Enable Bit    */
    uint32_t IE18:1;                    /**< bit:     18  XDMAC Channel 18 Interrupt Enable Bit    */
    uint32_t IE19:1;                    /**< bit:     19  XDMAC Channel 19 Interrupt Enable Bit    */
    uint32_t IE20:1;                    /**< bit:     20  XDMAC Channel 20 Interrupt Enable Bit    */
    uint32_t IE21:1;                    /**< bit:     21  XDMAC Channel 21 Interrupt Enable Bit    */
    uint32_t IE22:1;                    /**< bit:     22  XDMAC Channel 22 Interrupt Enable Bit    */
    uint32_t IE23:1;                    /**< bit:     23  XDMAC Channel 23 Interrupt Enable Bit    */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t IE:24;                     /**< bit:  0..23  XDMAC Channel 23 Interrupt Enable Bit    */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GIE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GIE_OFFSET                    (0x0C)                                        /**<  (XDMAC_GIE) Global Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GIE_IE0_Pos                   0                                              /**< (XDMAC_GIE) XDMAC Channel 0 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE0_Msk                   (0x1U << XDMAC_GIE_IE0_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 0 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE0                       XDMAC_GIE_IE0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE0_Msk instead */
#define XDMAC_GIE_IE1_Pos                   1                                              /**< (XDMAC_GIE) XDMAC Channel 1 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE1_Msk                   (0x1U << XDMAC_GIE_IE1_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 1 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE1                       XDMAC_GIE_IE1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE1_Msk instead */
#define XDMAC_GIE_IE2_Pos                   2                                              /**< (XDMAC_GIE) XDMAC Channel 2 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE2_Msk                   (0x1U << XDMAC_GIE_IE2_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 2 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE2                       XDMAC_GIE_IE2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE2_Msk instead */
#define XDMAC_GIE_IE3_Pos                   3                                              /**< (XDMAC_GIE) XDMAC Channel 3 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE3_Msk                   (0x1U << XDMAC_GIE_IE3_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 3 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE3                       XDMAC_GIE_IE3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE3_Msk instead */
#define XDMAC_GIE_IE4_Pos                   4                                              /**< (XDMAC_GIE) XDMAC Channel 4 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE4_Msk                   (0x1U << XDMAC_GIE_IE4_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 4 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE4                       XDMAC_GIE_IE4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE4_Msk instead */
#define XDMAC_GIE_IE5_Pos                   5                                              /**< (XDMAC_GIE) XDMAC Channel 5 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE5_Msk                   (0x1U << XDMAC_GIE_IE5_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 5 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE5                       XDMAC_GIE_IE5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE5_Msk instead */
#define XDMAC_GIE_IE6_Pos                   6                                              /**< (XDMAC_GIE) XDMAC Channel 6 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE6_Msk                   (0x1U << XDMAC_GIE_IE6_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 6 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE6                       XDMAC_GIE_IE6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE6_Msk instead */
#define XDMAC_GIE_IE7_Pos                   7                                              /**< (XDMAC_GIE) XDMAC Channel 7 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE7_Msk                   (0x1U << XDMAC_GIE_IE7_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 7 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE7                       XDMAC_GIE_IE7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE7_Msk instead */
#define XDMAC_GIE_IE8_Pos                   8                                              /**< (XDMAC_GIE) XDMAC Channel 8 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE8_Msk                   (0x1U << XDMAC_GIE_IE8_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 8 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE8                       XDMAC_GIE_IE8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE8_Msk instead */
#define XDMAC_GIE_IE9_Pos                   9                                              /**< (XDMAC_GIE) XDMAC Channel 9 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE9_Msk                   (0x1U << XDMAC_GIE_IE9_Pos)                    /**< (XDMAC_GIE) XDMAC Channel 9 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE9                       XDMAC_GIE_IE9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE9_Msk instead */
#define XDMAC_GIE_IE10_Pos                  10                                             /**< (XDMAC_GIE) XDMAC Channel 10 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE10_Msk                  (0x1U << XDMAC_GIE_IE10_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 10 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE10                      XDMAC_GIE_IE10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE10_Msk instead */
#define XDMAC_GIE_IE11_Pos                  11                                             /**< (XDMAC_GIE) XDMAC Channel 11 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE11_Msk                  (0x1U << XDMAC_GIE_IE11_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 11 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE11                      XDMAC_GIE_IE11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE11_Msk instead */
#define XDMAC_GIE_IE12_Pos                  12                                             /**< (XDMAC_GIE) XDMAC Channel 12 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE12_Msk                  (0x1U << XDMAC_GIE_IE12_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 12 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE12                      XDMAC_GIE_IE12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE12_Msk instead */
#define XDMAC_GIE_IE13_Pos                  13                                             /**< (XDMAC_GIE) XDMAC Channel 13 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE13_Msk                  (0x1U << XDMAC_GIE_IE13_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 13 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE13                      XDMAC_GIE_IE13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE13_Msk instead */
#define XDMAC_GIE_IE14_Pos                  14                                             /**< (XDMAC_GIE) XDMAC Channel 14 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE14_Msk                  (0x1U << XDMAC_GIE_IE14_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 14 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE14                      XDMAC_GIE_IE14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE14_Msk instead */
#define XDMAC_GIE_IE15_Pos                  15                                             /**< (XDMAC_GIE) XDMAC Channel 15 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE15_Msk                  (0x1U << XDMAC_GIE_IE15_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 15 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE15                      XDMAC_GIE_IE15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE15_Msk instead */
#define XDMAC_GIE_IE16_Pos                  16                                             /**< (XDMAC_GIE) XDMAC Channel 16 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE16_Msk                  (0x1U << XDMAC_GIE_IE16_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 16 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE16                      XDMAC_GIE_IE16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE16_Msk instead */
#define XDMAC_GIE_IE17_Pos                  17                                             /**< (XDMAC_GIE) XDMAC Channel 17 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE17_Msk                  (0x1U << XDMAC_GIE_IE17_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 17 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE17                      XDMAC_GIE_IE17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE17_Msk instead */
#define XDMAC_GIE_IE18_Pos                  18                                             /**< (XDMAC_GIE) XDMAC Channel 18 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE18_Msk                  (0x1U << XDMAC_GIE_IE18_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 18 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE18                      XDMAC_GIE_IE18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE18_Msk instead */
#define XDMAC_GIE_IE19_Pos                  19                                             /**< (XDMAC_GIE) XDMAC Channel 19 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE19_Msk                  (0x1U << XDMAC_GIE_IE19_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 19 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE19                      XDMAC_GIE_IE19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE19_Msk instead */
#define XDMAC_GIE_IE20_Pos                  20                                             /**< (XDMAC_GIE) XDMAC Channel 20 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE20_Msk                  (0x1U << XDMAC_GIE_IE20_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 20 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE20                      XDMAC_GIE_IE20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE20_Msk instead */
#define XDMAC_GIE_IE21_Pos                  21                                             /**< (XDMAC_GIE) XDMAC Channel 21 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE21_Msk                  (0x1U << XDMAC_GIE_IE21_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 21 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE21                      XDMAC_GIE_IE21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE21_Msk instead */
#define XDMAC_GIE_IE22_Pos                  22                                             /**< (XDMAC_GIE) XDMAC Channel 22 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE22_Msk                  (0x1U << XDMAC_GIE_IE22_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 22 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE22                      XDMAC_GIE_IE22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE22_Msk instead */
#define XDMAC_GIE_IE23_Pos                  23                                             /**< (XDMAC_GIE) XDMAC Channel 23 Interrupt Enable Bit Position */
#define XDMAC_GIE_IE23_Msk                  (0x1U << XDMAC_GIE_IE23_Pos)                   /**< (XDMAC_GIE) XDMAC Channel 23 Interrupt Enable Bit Mask */
#define XDMAC_GIE_IE23                      XDMAC_GIE_IE23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIE_IE23_Msk instead */
#define XDMAC_GIE_IE_Pos                    0                                              /**< (XDMAC_GIE Position) XDMAC Channel 23 Interrupt Enable Bit */
#define XDMAC_GIE_IE_Msk                    (0xFFFFFFU << XDMAC_GIE_IE_Pos)                /**< (XDMAC_GIE Mask) IE */
#define XDMAC_GIE_IE(value)                 (XDMAC_GIE_IE_Msk & ((value) << XDMAC_GIE_IE_Pos))  
#define XDMAC_GIE_MASK                      (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GIE) Register MASK  (Use XDMAC_GIE_Msk instead)  */
#define XDMAC_GIE_Msk                       (0xFFFFFFU)                                    /**< (XDMAC_GIE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GID : (XDMAC Offset: 0x10) (/W 32) Global Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ID0:1;                     /**< bit:      0  XDMAC Channel 0 Interrupt Disable Bit    */
    uint32_t ID1:1;                     /**< bit:      1  XDMAC Channel 1 Interrupt Disable Bit    */
    uint32_t ID2:1;                     /**< bit:      2  XDMAC Channel 2 Interrupt Disable Bit    */
    uint32_t ID3:1;                     /**< bit:      3  XDMAC Channel 3 Interrupt Disable Bit    */
    uint32_t ID4:1;                     /**< bit:      4  XDMAC Channel 4 Interrupt Disable Bit    */
    uint32_t ID5:1;                     /**< bit:      5  XDMAC Channel 5 Interrupt Disable Bit    */
    uint32_t ID6:1;                     /**< bit:      6  XDMAC Channel 6 Interrupt Disable Bit    */
    uint32_t ID7:1;                     /**< bit:      7  XDMAC Channel 7 Interrupt Disable Bit    */
    uint32_t ID8:1;                     /**< bit:      8  XDMAC Channel 8 Interrupt Disable Bit    */
    uint32_t ID9:1;                     /**< bit:      9  XDMAC Channel 9 Interrupt Disable Bit    */
    uint32_t ID10:1;                    /**< bit:     10  XDMAC Channel 10 Interrupt Disable Bit   */
    uint32_t ID11:1;                    /**< bit:     11  XDMAC Channel 11 Interrupt Disable Bit   */
    uint32_t ID12:1;                    /**< bit:     12  XDMAC Channel 12 Interrupt Disable Bit   */
    uint32_t ID13:1;                    /**< bit:     13  XDMAC Channel 13 Interrupt Disable Bit   */
    uint32_t ID14:1;                    /**< bit:     14  XDMAC Channel 14 Interrupt Disable Bit   */
    uint32_t ID15:1;                    /**< bit:     15  XDMAC Channel 15 Interrupt Disable Bit   */
    uint32_t ID16:1;                    /**< bit:     16  XDMAC Channel 16 Interrupt Disable Bit   */
    uint32_t ID17:1;                    /**< bit:     17  XDMAC Channel 17 Interrupt Disable Bit   */
    uint32_t ID18:1;                    /**< bit:     18  XDMAC Channel 18 Interrupt Disable Bit   */
    uint32_t ID19:1;                    /**< bit:     19  XDMAC Channel 19 Interrupt Disable Bit   */
    uint32_t ID20:1;                    /**< bit:     20  XDMAC Channel 20 Interrupt Disable Bit   */
    uint32_t ID21:1;                    /**< bit:     21  XDMAC Channel 21 Interrupt Disable Bit   */
    uint32_t ID22:1;                    /**< bit:     22  XDMAC Channel 22 Interrupt Disable Bit   */
    uint32_t ID23:1;                    /**< bit:     23  XDMAC Channel 23 Interrupt Disable Bit   */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t ID:24;                     /**< bit:  0..23  XDMAC Channel 23 Interrupt Disable Bit   */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GID_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GID_OFFSET                    (0x10)                                        /**<  (XDMAC_GID) Global Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GID_ID0_Pos                   0                                              /**< (XDMAC_GID) XDMAC Channel 0 Interrupt Disable Bit Position */
#define XDMAC_GID_ID0_Msk                   (0x1U << XDMAC_GID_ID0_Pos)                    /**< (XDMAC_GID) XDMAC Channel 0 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID0                       XDMAC_GID_ID0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID0_Msk instead */
#define XDMAC_GID_ID1_Pos                   1                                              /**< (XDMAC_GID) XDMAC Channel 1 Interrupt Disable Bit Position */
#define XDMAC_GID_ID1_Msk                   (0x1U << XDMAC_GID_ID1_Pos)                    /**< (XDMAC_GID) XDMAC Channel 1 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID1                       XDMAC_GID_ID1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID1_Msk instead */
#define XDMAC_GID_ID2_Pos                   2                                              /**< (XDMAC_GID) XDMAC Channel 2 Interrupt Disable Bit Position */
#define XDMAC_GID_ID2_Msk                   (0x1U << XDMAC_GID_ID2_Pos)                    /**< (XDMAC_GID) XDMAC Channel 2 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID2                       XDMAC_GID_ID2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID2_Msk instead */
#define XDMAC_GID_ID3_Pos                   3                                              /**< (XDMAC_GID) XDMAC Channel 3 Interrupt Disable Bit Position */
#define XDMAC_GID_ID3_Msk                   (0x1U << XDMAC_GID_ID3_Pos)                    /**< (XDMAC_GID) XDMAC Channel 3 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID3                       XDMAC_GID_ID3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID3_Msk instead */
#define XDMAC_GID_ID4_Pos                   4                                              /**< (XDMAC_GID) XDMAC Channel 4 Interrupt Disable Bit Position */
#define XDMAC_GID_ID4_Msk                   (0x1U << XDMAC_GID_ID4_Pos)                    /**< (XDMAC_GID) XDMAC Channel 4 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID4                       XDMAC_GID_ID4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID4_Msk instead */
#define XDMAC_GID_ID5_Pos                   5                                              /**< (XDMAC_GID) XDMAC Channel 5 Interrupt Disable Bit Position */
#define XDMAC_GID_ID5_Msk                   (0x1U << XDMAC_GID_ID5_Pos)                    /**< (XDMAC_GID) XDMAC Channel 5 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID5                       XDMAC_GID_ID5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID5_Msk instead */
#define XDMAC_GID_ID6_Pos                   6                                              /**< (XDMAC_GID) XDMAC Channel 6 Interrupt Disable Bit Position */
#define XDMAC_GID_ID6_Msk                   (0x1U << XDMAC_GID_ID6_Pos)                    /**< (XDMAC_GID) XDMAC Channel 6 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID6                       XDMAC_GID_ID6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID6_Msk instead */
#define XDMAC_GID_ID7_Pos                   7                                              /**< (XDMAC_GID) XDMAC Channel 7 Interrupt Disable Bit Position */
#define XDMAC_GID_ID7_Msk                   (0x1U << XDMAC_GID_ID7_Pos)                    /**< (XDMAC_GID) XDMAC Channel 7 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID7                       XDMAC_GID_ID7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID7_Msk instead */
#define XDMAC_GID_ID8_Pos                   8                                              /**< (XDMAC_GID) XDMAC Channel 8 Interrupt Disable Bit Position */
#define XDMAC_GID_ID8_Msk                   (0x1U << XDMAC_GID_ID8_Pos)                    /**< (XDMAC_GID) XDMAC Channel 8 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID8                       XDMAC_GID_ID8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID8_Msk instead */
#define XDMAC_GID_ID9_Pos                   9                                              /**< (XDMAC_GID) XDMAC Channel 9 Interrupt Disable Bit Position */
#define XDMAC_GID_ID9_Msk                   (0x1U << XDMAC_GID_ID9_Pos)                    /**< (XDMAC_GID) XDMAC Channel 9 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID9                       XDMAC_GID_ID9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID9_Msk instead */
#define XDMAC_GID_ID10_Pos                  10                                             /**< (XDMAC_GID) XDMAC Channel 10 Interrupt Disable Bit Position */
#define XDMAC_GID_ID10_Msk                  (0x1U << XDMAC_GID_ID10_Pos)                   /**< (XDMAC_GID) XDMAC Channel 10 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID10                      XDMAC_GID_ID10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID10_Msk instead */
#define XDMAC_GID_ID11_Pos                  11                                             /**< (XDMAC_GID) XDMAC Channel 11 Interrupt Disable Bit Position */
#define XDMAC_GID_ID11_Msk                  (0x1U << XDMAC_GID_ID11_Pos)                   /**< (XDMAC_GID) XDMAC Channel 11 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID11                      XDMAC_GID_ID11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID11_Msk instead */
#define XDMAC_GID_ID12_Pos                  12                                             /**< (XDMAC_GID) XDMAC Channel 12 Interrupt Disable Bit Position */
#define XDMAC_GID_ID12_Msk                  (0x1U << XDMAC_GID_ID12_Pos)                   /**< (XDMAC_GID) XDMAC Channel 12 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID12                      XDMAC_GID_ID12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID12_Msk instead */
#define XDMAC_GID_ID13_Pos                  13                                             /**< (XDMAC_GID) XDMAC Channel 13 Interrupt Disable Bit Position */
#define XDMAC_GID_ID13_Msk                  (0x1U << XDMAC_GID_ID13_Pos)                   /**< (XDMAC_GID) XDMAC Channel 13 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID13                      XDMAC_GID_ID13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID13_Msk instead */
#define XDMAC_GID_ID14_Pos                  14                                             /**< (XDMAC_GID) XDMAC Channel 14 Interrupt Disable Bit Position */
#define XDMAC_GID_ID14_Msk                  (0x1U << XDMAC_GID_ID14_Pos)                   /**< (XDMAC_GID) XDMAC Channel 14 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID14                      XDMAC_GID_ID14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID14_Msk instead */
#define XDMAC_GID_ID15_Pos                  15                                             /**< (XDMAC_GID) XDMAC Channel 15 Interrupt Disable Bit Position */
#define XDMAC_GID_ID15_Msk                  (0x1U << XDMAC_GID_ID15_Pos)                   /**< (XDMAC_GID) XDMAC Channel 15 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID15                      XDMAC_GID_ID15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID15_Msk instead */
#define XDMAC_GID_ID16_Pos                  16                                             /**< (XDMAC_GID) XDMAC Channel 16 Interrupt Disable Bit Position */
#define XDMAC_GID_ID16_Msk                  (0x1U << XDMAC_GID_ID16_Pos)                   /**< (XDMAC_GID) XDMAC Channel 16 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID16                      XDMAC_GID_ID16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID16_Msk instead */
#define XDMAC_GID_ID17_Pos                  17                                             /**< (XDMAC_GID) XDMAC Channel 17 Interrupt Disable Bit Position */
#define XDMAC_GID_ID17_Msk                  (0x1U << XDMAC_GID_ID17_Pos)                   /**< (XDMAC_GID) XDMAC Channel 17 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID17                      XDMAC_GID_ID17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID17_Msk instead */
#define XDMAC_GID_ID18_Pos                  18                                             /**< (XDMAC_GID) XDMAC Channel 18 Interrupt Disable Bit Position */
#define XDMAC_GID_ID18_Msk                  (0x1U << XDMAC_GID_ID18_Pos)                   /**< (XDMAC_GID) XDMAC Channel 18 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID18                      XDMAC_GID_ID18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID18_Msk instead */
#define XDMAC_GID_ID19_Pos                  19                                             /**< (XDMAC_GID) XDMAC Channel 19 Interrupt Disable Bit Position */
#define XDMAC_GID_ID19_Msk                  (0x1U << XDMAC_GID_ID19_Pos)                   /**< (XDMAC_GID) XDMAC Channel 19 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID19                      XDMAC_GID_ID19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID19_Msk instead */
#define XDMAC_GID_ID20_Pos                  20                                             /**< (XDMAC_GID) XDMAC Channel 20 Interrupt Disable Bit Position */
#define XDMAC_GID_ID20_Msk                  (0x1U << XDMAC_GID_ID20_Pos)                   /**< (XDMAC_GID) XDMAC Channel 20 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID20                      XDMAC_GID_ID20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID20_Msk instead */
#define XDMAC_GID_ID21_Pos                  21                                             /**< (XDMAC_GID) XDMAC Channel 21 Interrupt Disable Bit Position */
#define XDMAC_GID_ID21_Msk                  (0x1U << XDMAC_GID_ID21_Pos)                   /**< (XDMAC_GID) XDMAC Channel 21 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID21                      XDMAC_GID_ID21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID21_Msk instead */
#define XDMAC_GID_ID22_Pos                  22                                             /**< (XDMAC_GID) XDMAC Channel 22 Interrupt Disable Bit Position */
#define XDMAC_GID_ID22_Msk                  (0x1U << XDMAC_GID_ID22_Pos)                   /**< (XDMAC_GID) XDMAC Channel 22 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID22                      XDMAC_GID_ID22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID22_Msk instead */
#define XDMAC_GID_ID23_Pos                  23                                             /**< (XDMAC_GID) XDMAC Channel 23 Interrupt Disable Bit Position */
#define XDMAC_GID_ID23_Msk                  (0x1U << XDMAC_GID_ID23_Pos)                   /**< (XDMAC_GID) XDMAC Channel 23 Interrupt Disable Bit Mask */
#define XDMAC_GID_ID23                      XDMAC_GID_ID23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GID_ID23_Msk instead */
#define XDMAC_GID_ID_Pos                    0                                              /**< (XDMAC_GID Position) XDMAC Channel 23 Interrupt Disable Bit */
#define XDMAC_GID_ID_Msk                    (0xFFFFFFU << XDMAC_GID_ID_Pos)                /**< (XDMAC_GID Mask) ID */
#define XDMAC_GID_ID(value)                 (XDMAC_GID_ID_Msk & ((value) << XDMAC_GID_ID_Pos))  
#define XDMAC_GID_MASK                      (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GID) Register MASK  (Use XDMAC_GID_Msk instead)  */
#define XDMAC_GID_Msk                       (0xFFFFFFU)                                    /**< (XDMAC_GID) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GIM : (XDMAC Offset: 0x14) (R/ 32) Global Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IM0:1;                     /**< bit:      0  XDMAC Channel 0 Interrupt Mask Bit       */
    uint32_t IM1:1;                     /**< bit:      1  XDMAC Channel 1 Interrupt Mask Bit       */
    uint32_t IM2:1;                     /**< bit:      2  XDMAC Channel 2 Interrupt Mask Bit       */
    uint32_t IM3:1;                     /**< bit:      3  XDMAC Channel 3 Interrupt Mask Bit       */
    uint32_t IM4:1;                     /**< bit:      4  XDMAC Channel 4 Interrupt Mask Bit       */
    uint32_t IM5:1;                     /**< bit:      5  XDMAC Channel 5 Interrupt Mask Bit       */
    uint32_t IM6:1;                     /**< bit:      6  XDMAC Channel 6 Interrupt Mask Bit       */
    uint32_t IM7:1;                     /**< bit:      7  XDMAC Channel 7 Interrupt Mask Bit       */
    uint32_t IM8:1;                     /**< bit:      8  XDMAC Channel 8 Interrupt Mask Bit       */
    uint32_t IM9:1;                     /**< bit:      9  XDMAC Channel 9 Interrupt Mask Bit       */
    uint32_t IM10:1;                    /**< bit:     10  XDMAC Channel 10 Interrupt Mask Bit      */
    uint32_t IM11:1;                    /**< bit:     11  XDMAC Channel 11 Interrupt Mask Bit      */
    uint32_t IM12:1;                    /**< bit:     12  XDMAC Channel 12 Interrupt Mask Bit      */
    uint32_t IM13:1;                    /**< bit:     13  XDMAC Channel 13 Interrupt Mask Bit      */
    uint32_t IM14:1;                    /**< bit:     14  XDMAC Channel 14 Interrupt Mask Bit      */
    uint32_t IM15:1;                    /**< bit:     15  XDMAC Channel 15 Interrupt Mask Bit      */
    uint32_t IM16:1;                    /**< bit:     16  XDMAC Channel 16 Interrupt Mask Bit      */
    uint32_t IM17:1;                    /**< bit:     17  XDMAC Channel 17 Interrupt Mask Bit      */
    uint32_t IM18:1;                    /**< bit:     18  XDMAC Channel 18 Interrupt Mask Bit      */
    uint32_t IM19:1;                    /**< bit:     19  XDMAC Channel 19 Interrupt Mask Bit      */
    uint32_t IM20:1;                    /**< bit:     20  XDMAC Channel 20 Interrupt Mask Bit      */
    uint32_t IM21:1;                    /**< bit:     21  XDMAC Channel 21 Interrupt Mask Bit      */
    uint32_t IM22:1;                    /**< bit:     22  XDMAC Channel 22 Interrupt Mask Bit      */
    uint32_t IM23:1;                    /**< bit:     23  XDMAC Channel 23 Interrupt Mask Bit      */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t IM:24;                     /**< bit:  0..23  XDMAC Channel 23 Interrupt Mask Bit      */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GIM_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GIM_OFFSET                    (0x14)                                        /**<  (XDMAC_GIM) Global Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GIM_IM0_Pos                   0                                              /**< (XDMAC_GIM) XDMAC Channel 0 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM0_Msk                   (0x1U << XDMAC_GIM_IM0_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 0 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM0                       XDMAC_GIM_IM0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM0_Msk instead */
#define XDMAC_GIM_IM1_Pos                   1                                              /**< (XDMAC_GIM) XDMAC Channel 1 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM1_Msk                   (0x1U << XDMAC_GIM_IM1_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 1 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM1                       XDMAC_GIM_IM1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM1_Msk instead */
#define XDMAC_GIM_IM2_Pos                   2                                              /**< (XDMAC_GIM) XDMAC Channel 2 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM2_Msk                   (0x1U << XDMAC_GIM_IM2_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 2 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM2                       XDMAC_GIM_IM2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM2_Msk instead */
#define XDMAC_GIM_IM3_Pos                   3                                              /**< (XDMAC_GIM) XDMAC Channel 3 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM3_Msk                   (0x1U << XDMAC_GIM_IM3_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 3 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM3                       XDMAC_GIM_IM3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM3_Msk instead */
#define XDMAC_GIM_IM4_Pos                   4                                              /**< (XDMAC_GIM) XDMAC Channel 4 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM4_Msk                   (0x1U << XDMAC_GIM_IM4_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 4 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM4                       XDMAC_GIM_IM4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM4_Msk instead */
#define XDMAC_GIM_IM5_Pos                   5                                              /**< (XDMAC_GIM) XDMAC Channel 5 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM5_Msk                   (0x1U << XDMAC_GIM_IM5_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 5 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM5                       XDMAC_GIM_IM5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM5_Msk instead */
#define XDMAC_GIM_IM6_Pos                   6                                              /**< (XDMAC_GIM) XDMAC Channel 6 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM6_Msk                   (0x1U << XDMAC_GIM_IM6_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 6 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM6                       XDMAC_GIM_IM6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM6_Msk instead */
#define XDMAC_GIM_IM7_Pos                   7                                              /**< (XDMAC_GIM) XDMAC Channel 7 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM7_Msk                   (0x1U << XDMAC_GIM_IM7_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 7 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM7                       XDMAC_GIM_IM7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM7_Msk instead */
#define XDMAC_GIM_IM8_Pos                   8                                              /**< (XDMAC_GIM) XDMAC Channel 8 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM8_Msk                   (0x1U << XDMAC_GIM_IM8_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 8 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM8                       XDMAC_GIM_IM8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM8_Msk instead */
#define XDMAC_GIM_IM9_Pos                   9                                              /**< (XDMAC_GIM) XDMAC Channel 9 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM9_Msk                   (0x1U << XDMAC_GIM_IM9_Pos)                    /**< (XDMAC_GIM) XDMAC Channel 9 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM9                       XDMAC_GIM_IM9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM9_Msk instead */
#define XDMAC_GIM_IM10_Pos                  10                                             /**< (XDMAC_GIM) XDMAC Channel 10 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM10_Msk                  (0x1U << XDMAC_GIM_IM10_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 10 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM10                      XDMAC_GIM_IM10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM10_Msk instead */
#define XDMAC_GIM_IM11_Pos                  11                                             /**< (XDMAC_GIM) XDMAC Channel 11 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM11_Msk                  (0x1U << XDMAC_GIM_IM11_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 11 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM11                      XDMAC_GIM_IM11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM11_Msk instead */
#define XDMAC_GIM_IM12_Pos                  12                                             /**< (XDMAC_GIM) XDMAC Channel 12 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM12_Msk                  (0x1U << XDMAC_GIM_IM12_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 12 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM12                      XDMAC_GIM_IM12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM12_Msk instead */
#define XDMAC_GIM_IM13_Pos                  13                                             /**< (XDMAC_GIM) XDMAC Channel 13 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM13_Msk                  (0x1U << XDMAC_GIM_IM13_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 13 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM13                      XDMAC_GIM_IM13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM13_Msk instead */
#define XDMAC_GIM_IM14_Pos                  14                                             /**< (XDMAC_GIM) XDMAC Channel 14 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM14_Msk                  (0x1U << XDMAC_GIM_IM14_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 14 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM14                      XDMAC_GIM_IM14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM14_Msk instead */
#define XDMAC_GIM_IM15_Pos                  15                                             /**< (XDMAC_GIM) XDMAC Channel 15 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM15_Msk                  (0x1U << XDMAC_GIM_IM15_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 15 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM15                      XDMAC_GIM_IM15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM15_Msk instead */
#define XDMAC_GIM_IM16_Pos                  16                                             /**< (XDMAC_GIM) XDMAC Channel 16 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM16_Msk                  (0x1U << XDMAC_GIM_IM16_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 16 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM16                      XDMAC_GIM_IM16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM16_Msk instead */
#define XDMAC_GIM_IM17_Pos                  17                                             /**< (XDMAC_GIM) XDMAC Channel 17 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM17_Msk                  (0x1U << XDMAC_GIM_IM17_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 17 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM17                      XDMAC_GIM_IM17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM17_Msk instead */
#define XDMAC_GIM_IM18_Pos                  18                                             /**< (XDMAC_GIM) XDMAC Channel 18 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM18_Msk                  (0x1U << XDMAC_GIM_IM18_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 18 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM18                      XDMAC_GIM_IM18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM18_Msk instead */
#define XDMAC_GIM_IM19_Pos                  19                                             /**< (XDMAC_GIM) XDMAC Channel 19 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM19_Msk                  (0x1U << XDMAC_GIM_IM19_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 19 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM19                      XDMAC_GIM_IM19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM19_Msk instead */
#define XDMAC_GIM_IM20_Pos                  20                                             /**< (XDMAC_GIM) XDMAC Channel 20 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM20_Msk                  (0x1U << XDMAC_GIM_IM20_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 20 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM20                      XDMAC_GIM_IM20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM20_Msk instead */
#define XDMAC_GIM_IM21_Pos                  21                                             /**< (XDMAC_GIM) XDMAC Channel 21 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM21_Msk                  (0x1U << XDMAC_GIM_IM21_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 21 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM21                      XDMAC_GIM_IM21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM21_Msk instead */
#define XDMAC_GIM_IM22_Pos                  22                                             /**< (XDMAC_GIM) XDMAC Channel 22 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM22_Msk                  (0x1U << XDMAC_GIM_IM22_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 22 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM22                      XDMAC_GIM_IM22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM22_Msk instead */
#define XDMAC_GIM_IM23_Pos                  23                                             /**< (XDMAC_GIM) XDMAC Channel 23 Interrupt Mask Bit Position */
#define XDMAC_GIM_IM23_Msk                  (0x1U << XDMAC_GIM_IM23_Pos)                   /**< (XDMAC_GIM) XDMAC Channel 23 Interrupt Mask Bit Mask */
#define XDMAC_GIM_IM23                      XDMAC_GIM_IM23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIM_IM23_Msk instead */
#define XDMAC_GIM_IM_Pos                    0                                              /**< (XDMAC_GIM Position) XDMAC Channel 23 Interrupt Mask Bit */
#define XDMAC_GIM_IM_Msk                    (0xFFFFFFU << XDMAC_GIM_IM_Pos)                /**< (XDMAC_GIM Mask) IM */
#define XDMAC_GIM_IM(value)                 (XDMAC_GIM_IM_Msk & ((value) << XDMAC_GIM_IM_Pos))  
#define XDMAC_GIM_MASK                      (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GIM) Register MASK  (Use XDMAC_GIM_Msk instead)  */
#define XDMAC_GIM_Msk                       (0xFFFFFFU)                                    /**< (XDMAC_GIM) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GIS : (XDMAC Offset: 0x18) (R/ 32) Global Interrupt Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IS0:1;                     /**< bit:      0  XDMAC Channel 0 Interrupt Status Bit     */
    uint32_t IS1:1;                     /**< bit:      1  XDMAC Channel 1 Interrupt Status Bit     */
    uint32_t IS2:1;                     /**< bit:      2  XDMAC Channel 2 Interrupt Status Bit     */
    uint32_t IS3:1;                     /**< bit:      3  XDMAC Channel 3 Interrupt Status Bit     */
    uint32_t IS4:1;                     /**< bit:      4  XDMAC Channel 4 Interrupt Status Bit     */
    uint32_t IS5:1;                     /**< bit:      5  XDMAC Channel 5 Interrupt Status Bit     */
    uint32_t IS6:1;                     /**< bit:      6  XDMAC Channel 6 Interrupt Status Bit     */
    uint32_t IS7:1;                     /**< bit:      7  XDMAC Channel 7 Interrupt Status Bit     */
    uint32_t IS8:1;                     /**< bit:      8  XDMAC Channel 8 Interrupt Status Bit     */
    uint32_t IS9:1;                     /**< bit:      9  XDMAC Channel 9 Interrupt Status Bit     */
    uint32_t IS10:1;                    /**< bit:     10  XDMAC Channel 10 Interrupt Status Bit    */
    uint32_t IS11:1;                    /**< bit:     11  XDMAC Channel 11 Interrupt Status Bit    */
    uint32_t IS12:1;                    /**< bit:     12  XDMAC Channel 12 Interrupt Status Bit    */
    uint32_t IS13:1;                    /**< bit:     13  XDMAC Channel 13 Interrupt Status Bit    */
    uint32_t IS14:1;                    /**< bit:     14  XDMAC Channel 14 Interrupt Status Bit    */
    uint32_t IS15:1;                    /**< bit:     15  XDMAC Channel 15 Interrupt Status Bit    */
    uint32_t IS16:1;                    /**< bit:     16  XDMAC Channel 16 Interrupt Status Bit    */
    uint32_t IS17:1;                    /**< bit:     17  XDMAC Channel 17 Interrupt Status Bit    */
    uint32_t IS18:1;                    /**< bit:     18  XDMAC Channel 18 Interrupt Status Bit    */
    uint32_t IS19:1;                    /**< bit:     19  XDMAC Channel 19 Interrupt Status Bit    */
    uint32_t IS20:1;                    /**< bit:     20  XDMAC Channel 20 Interrupt Status Bit    */
    uint32_t IS21:1;                    /**< bit:     21  XDMAC Channel 21 Interrupt Status Bit    */
    uint32_t IS22:1;                    /**< bit:     22  XDMAC Channel 22 Interrupt Status Bit    */
    uint32_t IS23:1;                    /**< bit:     23  XDMAC Channel 23 Interrupt Status Bit    */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t IS:24;                     /**< bit:  0..23  XDMAC Channel 23 Interrupt Status Bit    */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GIS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GIS_OFFSET                    (0x18)                                        /**<  (XDMAC_GIS) Global Interrupt Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GIS_IS0_Pos                   0                                              /**< (XDMAC_GIS) XDMAC Channel 0 Interrupt Status Bit Position */
#define XDMAC_GIS_IS0_Msk                   (0x1U << XDMAC_GIS_IS0_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 0 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS0                       XDMAC_GIS_IS0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS0_Msk instead */
#define XDMAC_GIS_IS1_Pos                   1                                              /**< (XDMAC_GIS) XDMAC Channel 1 Interrupt Status Bit Position */
#define XDMAC_GIS_IS1_Msk                   (0x1U << XDMAC_GIS_IS1_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 1 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS1                       XDMAC_GIS_IS1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS1_Msk instead */
#define XDMAC_GIS_IS2_Pos                   2                                              /**< (XDMAC_GIS) XDMAC Channel 2 Interrupt Status Bit Position */
#define XDMAC_GIS_IS2_Msk                   (0x1U << XDMAC_GIS_IS2_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 2 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS2                       XDMAC_GIS_IS2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS2_Msk instead */
#define XDMAC_GIS_IS3_Pos                   3                                              /**< (XDMAC_GIS) XDMAC Channel 3 Interrupt Status Bit Position */
#define XDMAC_GIS_IS3_Msk                   (0x1U << XDMAC_GIS_IS3_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 3 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS3                       XDMAC_GIS_IS3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS3_Msk instead */
#define XDMAC_GIS_IS4_Pos                   4                                              /**< (XDMAC_GIS) XDMAC Channel 4 Interrupt Status Bit Position */
#define XDMAC_GIS_IS4_Msk                   (0x1U << XDMAC_GIS_IS4_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 4 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS4                       XDMAC_GIS_IS4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS4_Msk instead */
#define XDMAC_GIS_IS5_Pos                   5                                              /**< (XDMAC_GIS) XDMAC Channel 5 Interrupt Status Bit Position */
#define XDMAC_GIS_IS5_Msk                   (0x1U << XDMAC_GIS_IS5_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 5 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS5                       XDMAC_GIS_IS5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS5_Msk instead */
#define XDMAC_GIS_IS6_Pos                   6                                              /**< (XDMAC_GIS) XDMAC Channel 6 Interrupt Status Bit Position */
#define XDMAC_GIS_IS6_Msk                   (0x1U << XDMAC_GIS_IS6_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 6 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS6                       XDMAC_GIS_IS6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS6_Msk instead */
#define XDMAC_GIS_IS7_Pos                   7                                              /**< (XDMAC_GIS) XDMAC Channel 7 Interrupt Status Bit Position */
#define XDMAC_GIS_IS7_Msk                   (0x1U << XDMAC_GIS_IS7_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 7 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS7                       XDMAC_GIS_IS7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS7_Msk instead */
#define XDMAC_GIS_IS8_Pos                   8                                              /**< (XDMAC_GIS) XDMAC Channel 8 Interrupt Status Bit Position */
#define XDMAC_GIS_IS8_Msk                   (0x1U << XDMAC_GIS_IS8_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 8 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS8                       XDMAC_GIS_IS8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS8_Msk instead */
#define XDMAC_GIS_IS9_Pos                   9                                              /**< (XDMAC_GIS) XDMAC Channel 9 Interrupt Status Bit Position */
#define XDMAC_GIS_IS9_Msk                   (0x1U << XDMAC_GIS_IS9_Pos)                    /**< (XDMAC_GIS) XDMAC Channel 9 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS9                       XDMAC_GIS_IS9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS9_Msk instead */
#define XDMAC_GIS_IS10_Pos                  10                                             /**< (XDMAC_GIS) XDMAC Channel 10 Interrupt Status Bit Position */
#define XDMAC_GIS_IS10_Msk                  (0x1U << XDMAC_GIS_IS10_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 10 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS10                      XDMAC_GIS_IS10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS10_Msk instead */
#define XDMAC_GIS_IS11_Pos                  11                                             /**< (XDMAC_GIS) XDMAC Channel 11 Interrupt Status Bit Position */
#define XDMAC_GIS_IS11_Msk                  (0x1U << XDMAC_GIS_IS11_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 11 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS11                      XDMAC_GIS_IS11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS11_Msk instead */
#define XDMAC_GIS_IS12_Pos                  12                                             /**< (XDMAC_GIS) XDMAC Channel 12 Interrupt Status Bit Position */
#define XDMAC_GIS_IS12_Msk                  (0x1U << XDMAC_GIS_IS12_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 12 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS12                      XDMAC_GIS_IS12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS12_Msk instead */
#define XDMAC_GIS_IS13_Pos                  13                                             /**< (XDMAC_GIS) XDMAC Channel 13 Interrupt Status Bit Position */
#define XDMAC_GIS_IS13_Msk                  (0x1U << XDMAC_GIS_IS13_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 13 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS13                      XDMAC_GIS_IS13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS13_Msk instead */
#define XDMAC_GIS_IS14_Pos                  14                                             /**< (XDMAC_GIS) XDMAC Channel 14 Interrupt Status Bit Position */
#define XDMAC_GIS_IS14_Msk                  (0x1U << XDMAC_GIS_IS14_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 14 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS14                      XDMAC_GIS_IS14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS14_Msk instead */
#define XDMAC_GIS_IS15_Pos                  15                                             /**< (XDMAC_GIS) XDMAC Channel 15 Interrupt Status Bit Position */
#define XDMAC_GIS_IS15_Msk                  (0x1U << XDMAC_GIS_IS15_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 15 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS15                      XDMAC_GIS_IS15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS15_Msk instead */
#define XDMAC_GIS_IS16_Pos                  16                                             /**< (XDMAC_GIS) XDMAC Channel 16 Interrupt Status Bit Position */
#define XDMAC_GIS_IS16_Msk                  (0x1U << XDMAC_GIS_IS16_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 16 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS16                      XDMAC_GIS_IS16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS16_Msk instead */
#define XDMAC_GIS_IS17_Pos                  17                                             /**< (XDMAC_GIS) XDMAC Channel 17 Interrupt Status Bit Position */
#define XDMAC_GIS_IS17_Msk                  (0x1U << XDMAC_GIS_IS17_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 17 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS17                      XDMAC_GIS_IS17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS17_Msk instead */
#define XDMAC_GIS_IS18_Pos                  18                                             /**< (XDMAC_GIS) XDMAC Channel 18 Interrupt Status Bit Position */
#define XDMAC_GIS_IS18_Msk                  (0x1U << XDMAC_GIS_IS18_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 18 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS18                      XDMAC_GIS_IS18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS18_Msk instead */
#define XDMAC_GIS_IS19_Pos                  19                                             /**< (XDMAC_GIS) XDMAC Channel 19 Interrupt Status Bit Position */
#define XDMAC_GIS_IS19_Msk                  (0x1U << XDMAC_GIS_IS19_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 19 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS19                      XDMAC_GIS_IS19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS19_Msk instead */
#define XDMAC_GIS_IS20_Pos                  20                                             /**< (XDMAC_GIS) XDMAC Channel 20 Interrupt Status Bit Position */
#define XDMAC_GIS_IS20_Msk                  (0x1U << XDMAC_GIS_IS20_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 20 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS20                      XDMAC_GIS_IS20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS20_Msk instead */
#define XDMAC_GIS_IS21_Pos                  21                                             /**< (XDMAC_GIS) XDMAC Channel 21 Interrupt Status Bit Position */
#define XDMAC_GIS_IS21_Msk                  (0x1U << XDMAC_GIS_IS21_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 21 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS21                      XDMAC_GIS_IS21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS21_Msk instead */
#define XDMAC_GIS_IS22_Pos                  22                                             /**< (XDMAC_GIS) XDMAC Channel 22 Interrupt Status Bit Position */
#define XDMAC_GIS_IS22_Msk                  (0x1U << XDMAC_GIS_IS22_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 22 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS22                      XDMAC_GIS_IS22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS22_Msk instead */
#define XDMAC_GIS_IS23_Pos                  23                                             /**< (XDMAC_GIS) XDMAC Channel 23 Interrupt Status Bit Position */
#define XDMAC_GIS_IS23_Msk                  (0x1U << XDMAC_GIS_IS23_Pos)                   /**< (XDMAC_GIS) XDMAC Channel 23 Interrupt Status Bit Mask */
#define XDMAC_GIS_IS23                      XDMAC_GIS_IS23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GIS_IS23_Msk instead */
#define XDMAC_GIS_IS_Pos                    0                                              /**< (XDMAC_GIS Position) XDMAC Channel 23 Interrupt Status Bit */
#define XDMAC_GIS_IS_Msk                    (0xFFFFFFU << XDMAC_GIS_IS_Pos)                /**< (XDMAC_GIS Mask) IS */
#define XDMAC_GIS_IS(value)                 (XDMAC_GIS_IS_Msk & ((value) << XDMAC_GIS_IS_Pos))  
#define XDMAC_GIS_MASK                      (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GIS) Register MASK  (Use XDMAC_GIS_Msk instead)  */
#define XDMAC_GIS_Msk                       (0xFFFFFFU)                                    /**< (XDMAC_GIS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GE : (XDMAC Offset: 0x1c) (/W 32) Global Channel Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t EN0:1;                     /**< bit:      0  XDMAC Channel 0 Enable Bit               */
    uint32_t EN1:1;                     /**< bit:      1  XDMAC Channel 1 Enable Bit               */
    uint32_t EN2:1;                     /**< bit:      2  XDMAC Channel 2 Enable Bit               */
    uint32_t EN3:1;                     /**< bit:      3  XDMAC Channel 3 Enable Bit               */
    uint32_t EN4:1;                     /**< bit:      4  XDMAC Channel 4 Enable Bit               */
    uint32_t EN5:1;                     /**< bit:      5  XDMAC Channel 5 Enable Bit               */
    uint32_t EN6:1;                     /**< bit:      6  XDMAC Channel 6 Enable Bit               */
    uint32_t EN7:1;                     /**< bit:      7  XDMAC Channel 7 Enable Bit               */
    uint32_t EN8:1;                     /**< bit:      8  XDMAC Channel 8 Enable Bit               */
    uint32_t EN9:1;                     /**< bit:      9  XDMAC Channel 9 Enable Bit               */
    uint32_t EN10:1;                    /**< bit:     10  XDMAC Channel 10 Enable Bit              */
    uint32_t EN11:1;                    /**< bit:     11  XDMAC Channel 11 Enable Bit              */
    uint32_t EN12:1;                    /**< bit:     12  XDMAC Channel 12 Enable Bit              */
    uint32_t EN13:1;                    /**< bit:     13  XDMAC Channel 13 Enable Bit              */
    uint32_t EN14:1;                    /**< bit:     14  XDMAC Channel 14 Enable Bit              */
    uint32_t EN15:1;                    /**< bit:     15  XDMAC Channel 15 Enable Bit              */
    uint32_t EN16:1;                    /**< bit:     16  XDMAC Channel 16 Enable Bit              */
    uint32_t EN17:1;                    /**< bit:     17  XDMAC Channel 17 Enable Bit              */
    uint32_t EN18:1;                    /**< bit:     18  XDMAC Channel 18 Enable Bit              */
    uint32_t EN19:1;                    /**< bit:     19  XDMAC Channel 19 Enable Bit              */
    uint32_t EN20:1;                    /**< bit:     20  XDMAC Channel 20 Enable Bit              */
    uint32_t EN21:1;                    /**< bit:     21  XDMAC Channel 21 Enable Bit              */
    uint32_t EN22:1;                    /**< bit:     22  XDMAC Channel 22 Enable Bit              */
    uint32_t EN23:1;                    /**< bit:     23  XDMAC Channel 23 Enable Bit              */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t EN:24;                     /**< bit:  0..23  XDMAC Channel 23 Enable Bit              */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GE_OFFSET                     (0x1C)                                        /**<  (XDMAC_GE) Global Channel Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GE_EN0_Pos                    0                                              /**< (XDMAC_GE) XDMAC Channel 0 Enable Bit Position */
#define XDMAC_GE_EN0_Msk                    (0x1U << XDMAC_GE_EN0_Pos)                     /**< (XDMAC_GE) XDMAC Channel 0 Enable Bit Mask */
#define XDMAC_GE_EN0                        XDMAC_GE_EN0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN0_Msk instead */
#define XDMAC_GE_EN1_Pos                    1                                              /**< (XDMAC_GE) XDMAC Channel 1 Enable Bit Position */
#define XDMAC_GE_EN1_Msk                    (0x1U << XDMAC_GE_EN1_Pos)                     /**< (XDMAC_GE) XDMAC Channel 1 Enable Bit Mask */
#define XDMAC_GE_EN1                        XDMAC_GE_EN1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN1_Msk instead */
#define XDMAC_GE_EN2_Pos                    2                                              /**< (XDMAC_GE) XDMAC Channel 2 Enable Bit Position */
#define XDMAC_GE_EN2_Msk                    (0x1U << XDMAC_GE_EN2_Pos)                     /**< (XDMAC_GE) XDMAC Channel 2 Enable Bit Mask */
#define XDMAC_GE_EN2                        XDMAC_GE_EN2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN2_Msk instead */
#define XDMAC_GE_EN3_Pos                    3                                              /**< (XDMAC_GE) XDMAC Channel 3 Enable Bit Position */
#define XDMAC_GE_EN3_Msk                    (0x1U << XDMAC_GE_EN3_Pos)                     /**< (XDMAC_GE) XDMAC Channel 3 Enable Bit Mask */
#define XDMAC_GE_EN3                        XDMAC_GE_EN3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN3_Msk instead */
#define XDMAC_GE_EN4_Pos                    4                                              /**< (XDMAC_GE) XDMAC Channel 4 Enable Bit Position */
#define XDMAC_GE_EN4_Msk                    (0x1U << XDMAC_GE_EN4_Pos)                     /**< (XDMAC_GE) XDMAC Channel 4 Enable Bit Mask */
#define XDMAC_GE_EN4                        XDMAC_GE_EN4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN4_Msk instead */
#define XDMAC_GE_EN5_Pos                    5                                              /**< (XDMAC_GE) XDMAC Channel 5 Enable Bit Position */
#define XDMAC_GE_EN5_Msk                    (0x1U << XDMAC_GE_EN5_Pos)                     /**< (XDMAC_GE) XDMAC Channel 5 Enable Bit Mask */
#define XDMAC_GE_EN5                        XDMAC_GE_EN5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN5_Msk instead */
#define XDMAC_GE_EN6_Pos                    6                                              /**< (XDMAC_GE) XDMAC Channel 6 Enable Bit Position */
#define XDMAC_GE_EN6_Msk                    (0x1U << XDMAC_GE_EN6_Pos)                     /**< (XDMAC_GE) XDMAC Channel 6 Enable Bit Mask */
#define XDMAC_GE_EN6                        XDMAC_GE_EN6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN6_Msk instead */
#define XDMAC_GE_EN7_Pos                    7                                              /**< (XDMAC_GE) XDMAC Channel 7 Enable Bit Position */
#define XDMAC_GE_EN7_Msk                    (0x1U << XDMAC_GE_EN7_Pos)                     /**< (XDMAC_GE) XDMAC Channel 7 Enable Bit Mask */
#define XDMAC_GE_EN7                        XDMAC_GE_EN7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN7_Msk instead */
#define XDMAC_GE_EN8_Pos                    8                                              /**< (XDMAC_GE) XDMAC Channel 8 Enable Bit Position */
#define XDMAC_GE_EN8_Msk                    (0x1U << XDMAC_GE_EN8_Pos)                     /**< (XDMAC_GE) XDMAC Channel 8 Enable Bit Mask */
#define XDMAC_GE_EN8                        XDMAC_GE_EN8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN8_Msk instead */
#define XDMAC_GE_EN9_Pos                    9                                              /**< (XDMAC_GE) XDMAC Channel 9 Enable Bit Position */
#define XDMAC_GE_EN9_Msk                    (0x1U << XDMAC_GE_EN9_Pos)                     /**< (XDMAC_GE) XDMAC Channel 9 Enable Bit Mask */
#define XDMAC_GE_EN9                        XDMAC_GE_EN9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN9_Msk instead */
#define XDMAC_GE_EN10_Pos                   10                                             /**< (XDMAC_GE) XDMAC Channel 10 Enable Bit Position */
#define XDMAC_GE_EN10_Msk                   (0x1U << XDMAC_GE_EN10_Pos)                    /**< (XDMAC_GE) XDMAC Channel 10 Enable Bit Mask */
#define XDMAC_GE_EN10                       XDMAC_GE_EN10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN10_Msk instead */
#define XDMAC_GE_EN11_Pos                   11                                             /**< (XDMAC_GE) XDMAC Channel 11 Enable Bit Position */
#define XDMAC_GE_EN11_Msk                   (0x1U << XDMAC_GE_EN11_Pos)                    /**< (XDMAC_GE) XDMAC Channel 11 Enable Bit Mask */
#define XDMAC_GE_EN11                       XDMAC_GE_EN11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN11_Msk instead */
#define XDMAC_GE_EN12_Pos                   12                                             /**< (XDMAC_GE) XDMAC Channel 12 Enable Bit Position */
#define XDMAC_GE_EN12_Msk                   (0x1U << XDMAC_GE_EN12_Pos)                    /**< (XDMAC_GE) XDMAC Channel 12 Enable Bit Mask */
#define XDMAC_GE_EN12                       XDMAC_GE_EN12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN12_Msk instead */
#define XDMAC_GE_EN13_Pos                   13                                             /**< (XDMAC_GE) XDMAC Channel 13 Enable Bit Position */
#define XDMAC_GE_EN13_Msk                   (0x1U << XDMAC_GE_EN13_Pos)                    /**< (XDMAC_GE) XDMAC Channel 13 Enable Bit Mask */
#define XDMAC_GE_EN13                       XDMAC_GE_EN13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN13_Msk instead */
#define XDMAC_GE_EN14_Pos                   14                                             /**< (XDMAC_GE) XDMAC Channel 14 Enable Bit Position */
#define XDMAC_GE_EN14_Msk                   (0x1U << XDMAC_GE_EN14_Pos)                    /**< (XDMAC_GE) XDMAC Channel 14 Enable Bit Mask */
#define XDMAC_GE_EN14                       XDMAC_GE_EN14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN14_Msk instead */
#define XDMAC_GE_EN15_Pos                   15                                             /**< (XDMAC_GE) XDMAC Channel 15 Enable Bit Position */
#define XDMAC_GE_EN15_Msk                   (0x1U << XDMAC_GE_EN15_Pos)                    /**< (XDMAC_GE) XDMAC Channel 15 Enable Bit Mask */
#define XDMAC_GE_EN15                       XDMAC_GE_EN15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN15_Msk instead */
#define XDMAC_GE_EN16_Pos                   16                                             /**< (XDMAC_GE) XDMAC Channel 16 Enable Bit Position */
#define XDMAC_GE_EN16_Msk                   (0x1U << XDMAC_GE_EN16_Pos)                    /**< (XDMAC_GE) XDMAC Channel 16 Enable Bit Mask */
#define XDMAC_GE_EN16                       XDMAC_GE_EN16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN16_Msk instead */
#define XDMAC_GE_EN17_Pos                   17                                             /**< (XDMAC_GE) XDMAC Channel 17 Enable Bit Position */
#define XDMAC_GE_EN17_Msk                   (0x1U << XDMAC_GE_EN17_Pos)                    /**< (XDMAC_GE) XDMAC Channel 17 Enable Bit Mask */
#define XDMAC_GE_EN17                       XDMAC_GE_EN17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN17_Msk instead */
#define XDMAC_GE_EN18_Pos                   18                                             /**< (XDMAC_GE) XDMAC Channel 18 Enable Bit Position */
#define XDMAC_GE_EN18_Msk                   (0x1U << XDMAC_GE_EN18_Pos)                    /**< (XDMAC_GE) XDMAC Channel 18 Enable Bit Mask */
#define XDMAC_GE_EN18                       XDMAC_GE_EN18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN18_Msk instead */
#define XDMAC_GE_EN19_Pos                   19                                             /**< (XDMAC_GE) XDMAC Channel 19 Enable Bit Position */
#define XDMAC_GE_EN19_Msk                   (0x1U << XDMAC_GE_EN19_Pos)                    /**< (XDMAC_GE) XDMAC Channel 19 Enable Bit Mask */
#define XDMAC_GE_EN19                       XDMAC_GE_EN19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN19_Msk instead */
#define XDMAC_GE_EN20_Pos                   20                                             /**< (XDMAC_GE) XDMAC Channel 20 Enable Bit Position */
#define XDMAC_GE_EN20_Msk                   (0x1U << XDMAC_GE_EN20_Pos)                    /**< (XDMAC_GE) XDMAC Channel 20 Enable Bit Mask */
#define XDMAC_GE_EN20                       XDMAC_GE_EN20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN20_Msk instead */
#define XDMAC_GE_EN21_Pos                   21                                             /**< (XDMAC_GE) XDMAC Channel 21 Enable Bit Position */
#define XDMAC_GE_EN21_Msk                   (0x1U << XDMAC_GE_EN21_Pos)                    /**< (XDMAC_GE) XDMAC Channel 21 Enable Bit Mask */
#define XDMAC_GE_EN21                       XDMAC_GE_EN21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN21_Msk instead */
#define XDMAC_GE_EN22_Pos                   22                                             /**< (XDMAC_GE) XDMAC Channel 22 Enable Bit Position */
#define XDMAC_GE_EN22_Msk                   (0x1U << XDMAC_GE_EN22_Pos)                    /**< (XDMAC_GE) XDMAC Channel 22 Enable Bit Mask */
#define XDMAC_GE_EN22                       XDMAC_GE_EN22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN22_Msk instead */
#define XDMAC_GE_EN23_Pos                   23                                             /**< (XDMAC_GE) XDMAC Channel 23 Enable Bit Position */
#define XDMAC_GE_EN23_Msk                   (0x1U << XDMAC_GE_EN23_Pos)                    /**< (XDMAC_GE) XDMAC Channel 23 Enable Bit Mask */
#define XDMAC_GE_EN23                       XDMAC_GE_EN23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GE_EN23_Msk instead */
#define XDMAC_GE_EN_Pos                     0                                              /**< (XDMAC_GE Position) XDMAC Channel 23 Enable Bit */
#define XDMAC_GE_EN_Msk                     (0xFFFFFFU << XDMAC_GE_EN_Pos)                 /**< (XDMAC_GE Mask) EN */
#define XDMAC_GE_EN(value)                  (XDMAC_GE_EN_Msk & ((value) << XDMAC_GE_EN_Pos))  
#define XDMAC_GE_MASK                       (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GE) Register MASK  (Use XDMAC_GE_Msk instead)  */
#define XDMAC_GE_Msk                        (0xFFFFFFU)                                    /**< (XDMAC_GE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GD : (XDMAC Offset: 0x20) (/W 32) Global Channel Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DI0:1;                     /**< bit:      0  XDMAC Channel 0 Disable Bit              */
    uint32_t DI1:1;                     /**< bit:      1  XDMAC Channel 1 Disable Bit              */
    uint32_t DI2:1;                     /**< bit:      2  XDMAC Channel 2 Disable Bit              */
    uint32_t DI3:1;                     /**< bit:      3  XDMAC Channel 3 Disable Bit              */
    uint32_t DI4:1;                     /**< bit:      4  XDMAC Channel 4 Disable Bit              */
    uint32_t DI5:1;                     /**< bit:      5  XDMAC Channel 5 Disable Bit              */
    uint32_t DI6:1;                     /**< bit:      6  XDMAC Channel 6 Disable Bit              */
    uint32_t DI7:1;                     /**< bit:      7  XDMAC Channel 7 Disable Bit              */
    uint32_t DI8:1;                     /**< bit:      8  XDMAC Channel 8 Disable Bit              */
    uint32_t DI9:1;                     /**< bit:      9  XDMAC Channel 9 Disable Bit              */
    uint32_t DI10:1;                    /**< bit:     10  XDMAC Channel 10 Disable Bit             */
    uint32_t DI11:1;                    /**< bit:     11  XDMAC Channel 11 Disable Bit             */
    uint32_t DI12:1;                    /**< bit:     12  XDMAC Channel 12 Disable Bit             */
    uint32_t DI13:1;                    /**< bit:     13  XDMAC Channel 13 Disable Bit             */
    uint32_t DI14:1;                    /**< bit:     14  XDMAC Channel 14 Disable Bit             */
    uint32_t DI15:1;                    /**< bit:     15  XDMAC Channel 15 Disable Bit             */
    uint32_t DI16:1;                    /**< bit:     16  XDMAC Channel 16 Disable Bit             */
    uint32_t DI17:1;                    /**< bit:     17  XDMAC Channel 17 Disable Bit             */
    uint32_t DI18:1;                    /**< bit:     18  XDMAC Channel 18 Disable Bit             */
    uint32_t DI19:1;                    /**< bit:     19  XDMAC Channel 19 Disable Bit             */
    uint32_t DI20:1;                    /**< bit:     20  XDMAC Channel 20 Disable Bit             */
    uint32_t DI21:1;                    /**< bit:     21  XDMAC Channel 21 Disable Bit             */
    uint32_t DI22:1;                    /**< bit:     22  XDMAC Channel 22 Disable Bit             */
    uint32_t DI23:1;                    /**< bit:     23  XDMAC Channel 23 Disable Bit             */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t DI:24;                     /**< bit:  0..23  XDMAC Channel 23 Disable Bit             */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GD_OFFSET                     (0x20)                                        /**<  (XDMAC_GD) Global Channel Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GD_DI0_Pos                    0                                              /**< (XDMAC_GD) XDMAC Channel 0 Disable Bit Position */
#define XDMAC_GD_DI0_Msk                    (0x1U << XDMAC_GD_DI0_Pos)                     /**< (XDMAC_GD) XDMAC Channel 0 Disable Bit Mask */
#define XDMAC_GD_DI0                        XDMAC_GD_DI0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI0_Msk instead */
#define XDMAC_GD_DI1_Pos                    1                                              /**< (XDMAC_GD) XDMAC Channel 1 Disable Bit Position */
#define XDMAC_GD_DI1_Msk                    (0x1U << XDMAC_GD_DI1_Pos)                     /**< (XDMAC_GD) XDMAC Channel 1 Disable Bit Mask */
#define XDMAC_GD_DI1                        XDMAC_GD_DI1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI1_Msk instead */
#define XDMAC_GD_DI2_Pos                    2                                              /**< (XDMAC_GD) XDMAC Channel 2 Disable Bit Position */
#define XDMAC_GD_DI2_Msk                    (0x1U << XDMAC_GD_DI2_Pos)                     /**< (XDMAC_GD) XDMAC Channel 2 Disable Bit Mask */
#define XDMAC_GD_DI2                        XDMAC_GD_DI2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI2_Msk instead */
#define XDMAC_GD_DI3_Pos                    3                                              /**< (XDMAC_GD) XDMAC Channel 3 Disable Bit Position */
#define XDMAC_GD_DI3_Msk                    (0x1U << XDMAC_GD_DI3_Pos)                     /**< (XDMAC_GD) XDMAC Channel 3 Disable Bit Mask */
#define XDMAC_GD_DI3                        XDMAC_GD_DI3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI3_Msk instead */
#define XDMAC_GD_DI4_Pos                    4                                              /**< (XDMAC_GD) XDMAC Channel 4 Disable Bit Position */
#define XDMAC_GD_DI4_Msk                    (0x1U << XDMAC_GD_DI4_Pos)                     /**< (XDMAC_GD) XDMAC Channel 4 Disable Bit Mask */
#define XDMAC_GD_DI4                        XDMAC_GD_DI4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI4_Msk instead */
#define XDMAC_GD_DI5_Pos                    5                                              /**< (XDMAC_GD) XDMAC Channel 5 Disable Bit Position */
#define XDMAC_GD_DI5_Msk                    (0x1U << XDMAC_GD_DI5_Pos)                     /**< (XDMAC_GD) XDMAC Channel 5 Disable Bit Mask */
#define XDMAC_GD_DI5                        XDMAC_GD_DI5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI5_Msk instead */
#define XDMAC_GD_DI6_Pos                    6                                              /**< (XDMAC_GD) XDMAC Channel 6 Disable Bit Position */
#define XDMAC_GD_DI6_Msk                    (0x1U << XDMAC_GD_DI6_Pos)                     /**< (XDMAC_GD) XDMAC Channel 6 Disable Bit Mask */
#define XDMAC_GD_DI6                        XDMAC_GD_DI6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI6_Msk instead */
#define XDMAC_GD_DI7_Pos                    7                                              /**< (XDMAC_GD) XDMAC Channel 7 Disable Bit Position */
#define XDMAC_GD_DI7_Msk                    (0x1U << XDMAC_GD_DI7_Pos)                     /**< (XDMAC_GD) XDMAC Channel 7 Disable Bit Mask */
#define XDMAC_GD_DI7                        XDMAC_GD_DI7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI7_Msk instead */
#define XDMAC_GD_DI8_Pos                    8                                              /**< (XDMAC_GD) XDMAC Channel 8 Disable Bit Position */
#define XDMAC_GD_DI8_Msk                    (0x1U << XDMAC_GD_DI8_Pos)                     /**< (XDMAC_GD) XDMAC Channel 8 Disable Bit Mask */
#define XDMAC_GD_DI8                        XDMAC_GD_DI8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI8_Msk instead */
#define XDMAC_GD_DI9_Pos                    9                                              /**< (XDMAC_GD) XDMAC Channel 9 Disable Bit Position */
#define XDMAC_GD_DI9_Msk                    (0x1U << XDMAC_GD_DI9_Pos)                     /**< (XDMAC_GD) XDMAC Channel 9 Disable Bit Mask */
#define XDMAC_GD_DI9                        XDMAC_GD_DI9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI9_Msk instead */
#define XDMAC_GD_DI10_Pos                   10                                             /**< (XDMAC_GD) XDMAC Channel 10 Disable Bit Position */
#define XDMAC_GD_DI10_Msk                   (0x1U << XDMAC_GD_DI10_Pos)                    /**< (XDMAC_GD) XDMAC Channel 10 Disable Bit Mask */
#define XDMAC_GD_DI10                       XDMAC_GD_DI10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI10_Msk instead */
#define XDMAC_GD_DI11_Pos                   11                                             /**< (XDMAC_GD) XDMAC Channel 11 Disable Bit Position */
#define XDMAC_GD_DI11_Msk                   (0x1U << XDMAC_GD_DI11_Pos)                    /**< (XDMAC_GD) XDMAC Channel 11 Disable Bit Mask */
#define XDMAC_GD_DI11                       XDMAC_GD_DI11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI11_Msk instead */
#define XDMAC_GD_DI12_Pos                   12                                             /**< (XDMAC_GD) XDMAC Channel 12 Disable Bit Position */
#define XDMAC_GD_DI12_Msk                   (0x1U << XDMAC_GD_DI12_Pos)                    /**< (XDMAC_GD) XDMAC Channel 12 Disable Bit Mask */
#define XDMAC_GD_DI12                       XDMAC_GD_DI12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI12_Msk instead */
#define XDMAC_GD_DI13_Pos                   13                                             /**< (XDMAC_GD) XDMAC Channel 13 Disable Bit Position */
#define XDMAC_GD_DI13_Msk                   (0x1U << XDMAC_GD_DI13_Pos)                    /**< (XDMAC_GD) XDMAC Channel 13 Disable Bit Mask */
#define XDMAC_GD_DI13                       XDMAC_GD_DI13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI13_Msk instead */
#define XDMAC_GD_DI14_Pos                   14                                             /**< (XDMAC_GD) XDMAC Channel 14 Disable Bit Position */
#define XDMAC_GD_DI14_Msk                   (0x1U << XDMAC_GD_DI14_Pos)                    /**< (XDMAC_GD) XDMAC Channel 14 Disable Bit Mask */
#define XDMAC_GD_DI14                       XDMAC_GD_DI14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI14_Msk instead */
#define XDMAC_GD_DI15_Pos                   15                                             /**< (XDMAC_GD) XDMAC Channel 15 Disable Bit Position */
#define XDMAC_GD_DI15_Msk                   (0x1U << XDMAC_GD_DI15_Pos)                    /**< (XDMAC_GD) XDMAC Channel 15 Disable Bit Mask */
#define XDMAC_GD_DI15                       XDMAC_GD_DI15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI15_Msk instead */
#define XDMAC_GD_DI16_Pos                   16                                             /**< (XDMAC_GD) XDMAC Channel 16 Disable Bit Position */
#define XDMAC_GD_DI16_Msk                   (0x1U << XDMAC_GD_DI16_Pos)                    /**< (XDMAC_GD) XDMAC Channel 16 Disable Bit Mask */
#define XDMAC_GD_DI16                       XDMAC_GD_DI16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI16_Msk instead */
#define XDMAC_GD_DI17_Pos                   17                                             /**< (XDMAC_GD) XDMAC Channel 17 Disable Bit Position */
#define XDMAC_GD_DI17_Msk                   (0x1U << XDMAC_GD_DI17_Pos)                    /**< (XDMAC_GD) XDMAC Channel 17 Disable Bit Mask */
#define XDMAC_GD_DI17                       XDMAC_GD_DI17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI17_Msk instead */
#define XDMAC_GD_DI18_Pos                   18                                             /**< (XDMAC_GD) XDMAC Channel 18 Disable Bit Position */
#define XDMAC_GD_DI18_Msk                   (0x1U << XDMAC_GD_DI18_Pos)                    /**< (XDMAC_GD) XDMAC Channel 18 Disable Bit Mask */
#define XDMAC_GD_DI18                       XDMAC_GD_DI18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI18_Msk instead */
#define XDMAC_GD_DI19_Pos                   19                                             /**< (XDMAC_GD) XDMAC Channel 19 Disable Bit Position */
#define XDMAC_GD_DI19_Msk                   (0x1U << XDMAC_GD_DI19_Pos)                    /**< (XDMAC_GD) XDMAC Channel 19 Disable Bit Mask */
#define XDMAC_GD_DI19                       XDMAC_GD_DI19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI19_Msk instead */
#define XDMAC_GD_DI20_Pos                   20                                             /**< (XDMAC_GD) XDMAC Channel 20 Disable Bit Position */
#define XDMAC_GD_DI20_Msk                   (0x1U << XDMAC_GD_DI20_Pos)                    /**< (XDMAC_GD) XDMAC Channel 20 Disable Bit Mask */
#define XDMAC_GD_DI20                       XDMAC_GD_DI20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI20_Msk instead */
#define XDMAC_GD_DI21_Pos                   21                                             /**< (XDMAC_GD) XDMAC Channel 21 Disable Bit Position */
#define XDMAC_GD_DI21_Msk                   (0x1U << XDMAC_GD_DI21_Pos)                    /**< (XDMAC_GD) XDMAC Channel 21 Disable Bit Mask */
#define XDMAC_GD_DI21                       XDMAC_GD_DI21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI21_Msk instead */
#define XDMAC_GD_DI22_Pos                   22                                             /**< (XDMAC_GD) XDMAC Channel 22 Disable Bit Position */
#define XDMAC_GD_DI22_Msk                   (0x1U << XDMAC_GD_DI22_Pos)                    /**< (XDMAC_GD) XDMAC Channel 22 Disable Bit Mask */
#define XDMAC_GD_DI22                       XDMAC_GD_DI22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI22_Msk instead */
#define XDMAC_GD_DI23_Pos                   23                                             /**< (XDMAC_GD) XDMAC Channel 23 Disable Bit Position */
#define XDMAC_GD_DI23_Msk                   (0x1U << XDMAC_GD_DI23_Pos)                    /**< (XDMAC_GD) XDMAC Channel 23 Disable Bit Mask */
#define XDMAC_GD_DI23                       XDMAC_GD_DI23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GD_DI23_Msk instead */
#define XDMAC_GD_DI_Pos                     0                                              /**< (XDMAC_GD Position) XDMAC Channel 23 Disable Bit */
#define XDMAC_GD_DI_Msk                     (0xFFFFFFU << XDMAC_GD_DI_Pos)                 /**< (XDMAC_GD Mask) DI */
#define XDMAC_GD_DI(value)                  (XDMAC_GD_DI_Msk & ((value) << XDMAC_GD_DI_Pos))  
#define XDMAC_GD_MASK                       (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GD) Register MASK  (Use XDMAC_GD_Msk instead)  */
#define XDMAC_GD_Msk                        (0xFFFFFFU)                                    /**< (XDMAC_GD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GS : (XDMAC Offset: 0x24) (R/ 32) Global Channel Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ST0:1;                     /**< bit:      0  XDMAC Channel 0 Status Bit               */
    uint32_t ST1:1;                     /**< bit:      1  XDMAC Channel 1 Status Bit               */
    uint32_t ST2:1;                     /**< bit:      2  XDMAC Channel 2 Status Bit               */
    uint32_t ST3:1;                     /**< bit:      3  XDMAC Channel 3 Status Bit               */
    uint32_t ST4:1;                     /**< bit:      4  XDMAC Channel 4 Status Bit               */
    uint32_t ST5:1;                     /**< bit:      5  XDMAC Channel 5 Status Bit               */
    uint32_t ST6:1;                     /**< bit:      6  XDMAC Channel 6 Status Bit               */
    uint32_t ST7:1;                     /**< bit:      7  XDMAC Channel 7 Status Bit               */
    uint32_t ST8:1;                     /**< bit:      8  XDMAC Channel 8 Status Bit               */
    uint32_t ST9:1;                     /**< bit:      9  XDMAC Channel 9 Status Bit               */
    uint32_t ST10:1;                    /**< bit:     10  XDMAC Channel 10 Status Bit              */
    uint32_t ST11:1;                    /**< bit:     11  XDMAC Channel 11 Status Bit              */
    uint32_t ST12:1;                    /**< bit:     12  XDMAC Channel 12 Status Bit              */
    uint32_t ST13:1;                    /**< bit:     13  XDMAC Channel 13 Status Bit              */
    uint32_t ST14:1;                    /**< bit:     14  XDMAC Channel 14 Status Bit              */
    uint32_t ST15:1;                    /**< bit:     15  XDMAC Channel 15 Status Bit              */
    uint32_t ST16:1;                    /**< bit:     16  XDMAC Channel 16 Status Bit              */
    uint32_t ST17:1;                    /**< bit:     17  XDMAC Channel 17 Status Bit              */
    uint32_t ST18:1;                    /**< bit:     18  XDMAC Channel 18 Status Bit              */
    uint32_t ST19:1;                    /**< bit:     19  XDMAC Channel 19 Status Bit              */
    uint32_t ST20:1;                    /**< bit:     20  XDMAC Channel 20 Status Bit              */
    uint32_t ST21:1;                    /**< bit:     21  XDMAC Channel 21 Status Bit              */
    uint32_t ST22:1;                    /**< bit:     22  XDMAC Channel 22 Status Bit              */
    uint32_t ST23:1;                    /**< bit:     23  XDMAC Channel 23 Status Bit              */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t ST:24;                     /**< bit:  0..23  XDMAC Channel 23 Status Bit              */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GS_OFFSET                     (0x24)                                        /**<  (XDMAC_GS) Global Channel Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GS_ST0_Pos                    0                                              /**< (XDMAC_GS) XDMAC Channel 0 Status Bit Position */
#define XDMAC_GS_ST0_Msk                    (0x1U << XDMAC_GS_ST0_Pos)                     /**< (XDMAC_GS) XDMAC Channel 0 Status Bit Mask */
#define XDMAC_GS_ST0                        XDMAC_GS_ST0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST0_Msk instead */
#define XDMAC_GS_ST1_Pos                    1                                              /**< (XDMAC_GS) XDMAC Channel 1 Status Bit Position */
#define XDMAC_GS_ST1_Msk                    (0x1U << XDMAC_GS_ST1_Pos)                     /**< (XDMAC_GS) XDMAC Channel 1 Status Bit Mask */
#define XDMAC_GS_ST1                        XDMAC_GS_ST1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST1_Msk instead */
#define XDMAC_GS_ST2_Pos                    2                                              /**< (XDMAC_GS) XDMAC Channel 2 Status Bit Position */
#define XDMAC_GS_ST2_Msk                    (0x1U << XDMAC_GS_ST2_Pos)                     /**< (XDMAC_GS) XDMAC Channel 2 Status Bit Mask */
#define XDMAC_GS_ST2                        XDMAC_GS_ST2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST2_Msk instead */
#define XDMAC_GS_ST3_Pos                    3                                              /**< (XDMAC_GS) XDMAC Channel 3 Status Bit Position */
#define XDMAC_GS_ST3_Msk                    (0x1U << XDMAC_GS_ST3_Pos)                     /**< (XDMAC_GS) XDMAC Channel 3 Status Bit Mask */
#define XDMAC_GS_ST3                        XDMAC_GS_ST3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST3_Msk instead */
#define XDMAC_GS_ST4_Pos                    4                                              /**< (XDMAC_GS) XDMAC Channel 4 Status Bit Position */
#define XDMAC_GS_ST4_Msk                    (0x1U << XDMAC_GS_ST4_Pos)                     /**< (XDMAC_GS) XDMAC Channel 4 Status Bit Mask */
#define XDMAC_GS_ST4                        XDMAC_GS_ST4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST4_Msk instead */
#define XDMAC_GS_ST5_Pos                    5                                              /**< (XDMAC_GS) XDMAC Channel 5 Status Bit Position */
#define XDMAC_GS_ST5_Msk                    (0x1U << XDMAC_GS_ST5_Pos)                     /**< (XDMAC_GS) XDMAC Channel 5 Status Bit Mask */
#define XDMAC_GS_ST5                        XDMAC_GS_ST5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST5_Msk instead */
#define XDMAC_GS_ST6_Pos                    6                                              /**< (XDMAC_GS) XDMAC Channel 6 Status Bit Position */
#define XDMAC_GS_ST6_Msk                    (0x1U << XDMAC_GS_ST6_Pos)                     /**< (XDMAC_GS) XDMAC Channel 6 Status Bit Mask */
#define XDMAC_GS_ST6                        XDMAC_GS_ST6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST6_Msk instead */
#define XDMAC_GS_ST7_Pos                    7                                              /**< (XDMAC_GS) XDMAC Channel 7 Status Bit Position */
#define XDMAC_GS_ST7_Msk                    (0x1U << XDMAC_GS_ST7_Pos)                     /**< (XDMAC_GS) XDMAC Channel 7 Status Bit Mask */
#define XDMAC_GS_ST7                        XDMAC_GS_ST7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST7_Msk instead */
#define XDMAC_GS_ST8_Pos                    8                                              /**< (XDMAC_GS) XDMAC Channel 8 Status Bit Position */
#define XDMAC_GS_ST8_Msk                    (0x1U << XDMAC_GS_ST8_Pos)                     /**< (XDMAC_GS) XDMAC Channel 8 Status Bit Mask */
#define XDMAC_GS_ST8                        XDMAC_GS_ST8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST8_Msk instead */
#define XDMAC_GS_ST9_Pos                    9                                              /**< (XDMAC_GS) XDMAC Channel 9 Status Bit Position */
#define XDMAC_GS_ST9_Msk                    (0x1U << XDMAC_GS_ST9_Pos)                     /**< (XDMAC_GS) XDMAC Channel 9 Status Bit Mask */
#define XDMAC_GS_ST9                        XDMAC_GS_ST9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST9_Msk instead */
#define XDMAC_GS_ST10_Pos                   10                                             /**< (XDMAC_GS) XDMAC Channel 10 Status Bit Position */
#define XDMAC_GS_ST10_Msk                   (0x1U << XDMAC_GS_ST10_Pos)                    /**< (XDMAC_GS) XDMAC Channel 10 Status Bit Mask */
#define XDMAC_GS_ST10                       XDMAC_GS_ST10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST10_Msk instead */
#define XDMAC_GS_ST11_Pos                   11                                             /**< (XDMAC_GS) XDMAC Channel 11 Status Bit Position */
#define XDMAC_GS_ST11_Msk                   (0x1U << XDMAC_GS_ST11_Pos)                    /**< (XDMAC_GS) XDMAC Channel 11 Status Bit Mask */
#define XDMAC_GS_ST11                       XDMAC_GS_ST11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST11_Msk instead */
#define XDMAC_GS_ST12_Pos                   12                                             /**< (XDMAC_GS) XDMAC Channel 12 Status Bit Position */
#define XDMAC_GS_ST12_Msk                   (0x1U << XDMAC_GS_ST12_Pos)                    /**< (XDMAC_GS) XDMAC Channel 12 Status Bit Mask */
#define XDMAC_GS_ST12                       XDMAC_GS_ST12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST12_Msk instead */
#define XDMAC_GS_ST13_Pos                   13                                             /**< (XDMAC_GS) XDMAC Channel 13 Status Bit Position */
#define XDMAC_GS_ST13_Msk                   (0x1U << XDMAC_GS_ST13_Pos)                    /**< (XDMAC_GS) XDMAC Channel 13 Status Bit Mask */
#define XDMAC_GS_ST13                       XDMAC_GS_ST13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST13_Msk instead */
#define XDMAC_GS_ST14_Pos                   14                                             /**< (XDMAC_GS) XDMAC Channel 14 Status Bit Position */
#define XDMAC_GS_ST14_Msk                   (0x1U << XDMAC_GS_ST14_Pos)                    /**< (XDMAC_GS) XDMAC Channel 14 Status Bit Mask */
#define XDMAC_GS_ST14                       XDMAC_GS_ST14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST14_Msk instead */
#define XDMAC_GS_ST15_Pos                   15                                             /**< (XDMAC_GS) XDMAC Channel 15 Status Bit Position */
#define XDMAC_GS_ST15_Msk                   (0x1U << XDMAC_GS_ST15_Pos)                    /**< (XDMAC_GS) XDMAC Channel 15 Status Bit Mask */
#define XDMAC_GS_ST15                       XDMAC_GS_ST15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST15_Msk instead */
#define XDMAC_GS_ST16_Pos                   16                                             /**< (XDMAC_GS) XDMAC Channel 16 Status Bit Position */
#define XDMAC_GS_ST16_Msk                   (0x1U << XDMAC_GS_ST16_Pos)                    /**< (XDMAC_GS) XDMAC Channel 16 Status Bit Mask */
#define XDMAC_GS_ST16                       XDMAC_GS_ST16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST16_Msk instead */
#define XDMAC_GS_ST17_Pos                   17                                             /**< (XDMAC_GS) XDMAC Channel 17 Status Bit Position */
#define XDMAC_GS_ST17_Msk                   (0x1U << XDMAC_GS_ST17_Pos)                    /**< (XDMAC_GS) XDMAC Channel 17 Status Bit Mask */
#define XDMAC_GS_ST17                       XDMAC_GS_ST17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST17_Msk instead */
#define XDMAC_GS_ST18_Pos                   18                                             /**< (XDMAC_GS) XDMAC Channel 18 Status Bit Position */
#define XDMAC_GS_ST18_Msk                   (0x1U << XDMAC_GS_ST18_Pos)                    /**< (XDMAC_GS) XDMAC Channel 18 Status Bit Mask */
#define XDMAC_GS_ST18                       XDMAC_GS_ST18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST18_Msk instead */
#define XDMAC_GS_ST19_Pos                   19                                             /**< (XDMAC_GS) XDMAC Channel 19 Status Bit Position */
#define XDMAC_GS_ST19_Msk                   (0x1U << XDMAC_GS_ST19_Pos)                    /**< (XDMAC_GS) XDMAC Channel 19 Status Bit Mask */
#define XDMAC_GS_ST19                       XDMAC_GS_ST19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST19_Msk instead */
#define XDMAC_GS_ST20_Pos                   20                                             /**< (XDMAC_GS) XDMAC Channel 20 Status Bit Position */
#define XDMAC_GS_ST20_Msk                   (0x1U << XDMAC_GS_ST20_Pos)                    /**< (XDMAC_GS) XDMAC Channel 20 Status Bit Mask */
#define XDMAC_GS_ST20                       XDMAC_GS_ST20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST20_Msk instead */
#define XDMAC_GS_ST21_Pos                   21                                             /**< (XDMAC_GS) XDMAC Channel 21 Status Bit Position */
#define XDMAC_GS_ST21_Msk                   (0x1U << XDMAC_GS_ST21_Pos)                    /**< (XDMAC_GS) XDMAC Channel 21 Status Bit Mask */
#define XDMAC_GS_ST21                       XDMAC_GS_ST21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST21_Msk instead */
#define XDMAC_GS_ST22_Pos                   22                                             /**< (XDMAC_GS) XDMAC Channel 22 Status Bit Position */
#define XDMAC_GS_ST22_Msk                   (0x1U << XDMAC_GS_ST22_Pos)                    /**< (XDMAC_GS) XDMAC Channel 22 Status Bit Mask */
#define XDMAC_GS_ST22                       XDMAC_GS_ST22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST22_Msk instead */
#define XDMAC_GS_ST23_Pos                   23                                             /**< (XDMAC_GS) XDMAC Channel 23 Status Bit Position */
#define XDMAC_GS_ST23_Msk                   (0x1U << XDMAC_GS_ST23_Pos)                    /**< (XDMAC_GS) XDMAC Channel 23 Status Bit Mask */
#define XDMAC_GS_ST23                       XDMAC_GS_ST23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GS_ST23_Msk instead */
#define XDMAC_GS_ST_Pos                     0                                              /**< (XDMAC_GS Position) XDMAC Channel 23 Status Bit */
#define XDMAC_GS_ST_Msk                     (0xFFFFFFU << XDMAC_GS_ST_Pos)                 /**< (XDMAC_GS Mask) ST */
#define XDMAC_GS_ST(value)                  (XDMAC_GS_ST_Msk & ((value) << XDMAC_GS_ST_Pos))  
#define XDMAC_GS_MASK                       (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GS) Register MASK  (Use XDMAC_GS_Msk instead)  */
#define XDMAC_GS_Msk                        (0xFFFFFFU)                                    /**< (XDMAC_GS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GRS : (XDMAC Offset: 0x28) (R/W 32) Global Channel Read Suspend Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RS0:1;                     /**< bit:      0  XDMAC Channel 0 Read Suspend Bit         */
    uint32_t RS1:1;                     /**< bit:      1  XDMAC Channel 1 Read Suspend Bit         */
    uint32_t RS2:1;                     /**< bit:      2  XDMAC Channel 2 Read Suspend Bit         */
    uint32_t RS3:1;                     /**< bit:      3  XDMAC Channel 3 Read Suspend Bit         */
    uint32_t RS4:1;                     /**< bit:      4  XDMAC Channel 4 Read Suspend Bit         */
    uint32_t RS5:1;                     /**< bit:      5  XDMAC Channel 5 Read Suspend Bit         */
    uint32_t RS6:1;                     /**< bit:      6  XDMAC Channel 6 Read Suspend Bit         */
    uint32_t RS7:1;                     /**< bit:      7  XDMAC Channel 7 Read Suspend Bit         */
    uint32_t RS8:1;                     /**< bit:      8  XDMAC Channel 8 Read Suspend Bit         */
    uint32_t RS9:1;                     /**< bit:      9  XDMAC Channel 9 Read Suspend Bit         */
    uint32_t RS10:1;                    /**< bit:     10  XDMAC Channel 10 Read Suspend Bit        */
    uint32_t RS11:1;                    /**< bit:     11  XDMAC Channel 11 Read Suspend Bit        */
    uint32_t RS12:1;                    /**< bit:     12  XDMAC Channel 12 Read Suspend Bit        */
    uint32_t RS13:1;                    /**< bit:     13  XDMAC Channel 13 Read Suspend Bit        */
    uint32_t RS14:1;                    /**< bit:     14  XDMAC Channel 14 Read Suspend Bit        */
    uint32_t RS15:1;                    /**< bit:     15  XDMAC Channel 15 Read Suspend Bit        */
    uint32_t RS16:1;                    /**< bit:     16  XDMAC Channel 16 Read Suspend Bit        */
    uint32_t RS17:1;                    /**< bit:     17  XDMAC Channel 17 Read Suspend Bit        */
    uint32_t RS18:1;                    /**< bit:     18  XDMAC Channel 18 Read Suspend Bit        */
    uint32_t RS19:1;                    /**< bit:     19  XDMAC Channel 19 Read Suspend Bit        */
    uint32_t RS20:1;                    /**< bit:     20  XDMAC Channel 20 Read Suspend Bit        */
    uint32_t RS21:1;                    /**< bit:     21  XDMAC Channel 21 Read Suspend Bit        */
    uint32_t RS22:1;                    /**< bit:     22  XDMAC Channel 22 Read Suspend Bit        */
    uint32_t RS23:1;                    /**< bit:     23  XDMAC Channel 23 Read Suspend Bit        */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t RS:24;                     /**< bit:  0..23  XDMAC Channel 23 Read Suspend Bit        */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GRS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GRS_OFFSET                    (0x28)                                        /**<  (XDMAC_GRS) Global Channel Read Suspend Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GRS_RS0_Pos                   0                                              /**< (XDMAC_GRS) XDMAC Channel 0 Read Suspend Bit Position */
#define XDMAC_GRS_RS0_Msk                   (0x1U << XDMAC_GRS_RS0_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 0 Read Suspend Bit Mask */
#define XDMAC_GRS_RS0                       XDMAC_GRS_RS0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS0_Msk instead */
#define XDMAC_GRS_RS1_Pos                   1                                              /**< (XDMAC_GRS) XDMAC Channel 1 Read Suspend Bit Position */
#define XDMAC_GRS_RS1_Msk                   (0x1U << XDMAC_GRS_RS1_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 1 Read Suspend Bit Mask */
#define XDMAC_GRS_RS1                       XDMAC_GRS_RS1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS1_Msk instead */
#define XDMAC_GRS_RS2_Pos                   2                                              /**< (XDMAC_GRS) XDMAC Channel 2 Read Suspend Bit Position */
#define XDMAC_GRS_RS2_Msk                   (0x1U << XDMAC_GRS_RS2_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 2 Read Suspend Bit Mask */
#define XDMAC_GRS_RS2                       XDMAC_GRS_RS2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS2_Msk instead */
#define XDMAC_GRS_RS3_Pos                   3                                              /**< (XDMAC_GRS) XDMAC Channel 3 Read Suspend Bit Position */
#define XDMAC_GRS_RS3_Msk                   (0x1U << XDMAC_GRS_RS3_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 3 Read Suspend Bit Mask */
#define XDMAC_GRS_RS3                       XDMAC_GRS_RS3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS3_Msk instead */
#define XDMAC_GRS_RS4_Pos                   4                                              /**< (XDMAC_GRS) XDMAC Channel 4 Read Suspend Bit Position */
#define XDMAC_GRS_RS4_Msk                   (0x1U << XDMAC_GRS_RS4_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 4 Read Suspend Bit Mask */
#define XDMAC_GRS_RS4                       XDMAC_GRS_RS4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS4_Msk instead */
#define XDMAC_GRS_RS5_Pos                   5                                              /**< (XDMAC_GRS) XDMAC Channel 5 Read Suspend Bit Position */
#define XDMAC_GRS_RS5_Msk                   (0x1U << XDMAC_GRS_RS5_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 5 Read Suspend Bit Mask */
#define XDMAC_GRS_RS5                       XDMAC_GRS_RS5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS5_Msk instead */
#define XDMAC_GRS_RS6_Pos                   6                                              /**< (XDMAC_GRS) XDMAC Channel 6 Read Suspend Bit Position */
#define XDMAC_GRS_RS6_Msk                   (0x1U << XDMAC_GRS_RS6_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 6 Read Suspend Bit Mask */
#define XDMAC_GRS_RS6                       XDMAC_GRS_RS6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS6_Msk instead */
#define XDMAC_GRS_RS7_Pos                   7                                              /**< (XDMAC_GRS) XDMAC Channel 7 Read Suspend Bit Position */
#define XDMAC_GRS_RS7_Msk                   (0x1U << XDMAC_GRS_RS7_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 7 Read Suspend Bit Mask */
#define XDMAC_GRS_RS7                       XDMAC_GRS_RS7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS7_Msk instead */
#define XDMAC_GRS_RS8_Pos                   8                                              /**< (XDMAC_GRS) XDMAC Channel 8 Read Suspend Bit Position */
#define XDMAC_GRS_RS8_Msk                   (0x1U << XDMAC_GRS_RS8_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 8 Read Suspend Bit Mask */
#define XDMAC_GRS_RS8                       XDMAC_GRS_RS8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS8_Msk instead */
#define XDMAC_GRS_RS9_Pos                   9                                              /**< (XDMAC_GRS) XDMAC Channel 9 Read Suspend Bit Position */
#define XDMAC_GRS_RS9_Msk                   (0x1U << XDMAC_GRS_RS9_Pos)                    /**< (XDMAC_GRS) XDMAC Channel 9 Read Suspend Bit Mask */
#define XDMAC_GRS_RS9                       XDMAC_GRS_RS9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS9_Msk instead */
#define XDMAC_GRS_RS10_Pos                  10                                             /**< (XDMAC_GRS) XDMAC Channel 10 Read Suspend Bit Position */
#define XDMAC_GRS_RS10_Msk                  (0x1U << XDMAC_GRS_RS10_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 10 Read Suspend Bit Mask */
#define XDMAC_GRS_RS10                      XDMAC_GRS_RS10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS10_Msk instead */
#define XDMAC_GRS_RS11_Pos                  11                                             /**< (XDMAC_GRS) XDMAC Channel 11 Read Suspend Bit Position */
#define XDMAC_GRS_RS11_Msk                  (0x1U << XDMAC_GRS_RS11_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 11 Read Suspend Bit Mask */
#define XDMAC_GRS_RS11                      XDMAC_GRS_RS11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS11_Msk instead */
#define XDMAC_GRS_RS12_Pos                  12                                             /**< (XDMAC_GRS) XDMAC Channel 12 Read Suspend Bit Position */
#define XDMAC_GRS_RS12_Msk                  (0x1U << XDMAC_GRS_RS12_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 12 Read Suspend Bit Mask */
#define XDMAC_GRS_RS12                      XDMAC_GRS_RS12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS12_Msk instead */
#define XDMAC_GRS_RS13_Pos                  13                                             /**< (XDMAC_GRS) XDMAC Channel 13 Read Suspend Bit Position */
#define XDMAC_GRS_RS13_Msk                  (0x1U << XDMAC_GRS_RS13_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 13 Read Suspend Bit Mask */
#define XDMAC_GRS_RS13                      XDMAC_GRS_RS13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS13_Msk instead */
#define XDMAC_GRS_RS14_Pos                  14                                             /**< (XDMAC_GRS) XDMAC Channel 14 Read Suspend Bit Position */
#define XDMAC_GRS_RS14_Msk                  (0x1U << XDMAC_GRS_RS14_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 14 Read Suspend Bit Mask */
#define XDMAC_GRS_RS14                      XDMAC_GRS_RS14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS14_Msk instead */
#define XDMAC_GRS_RS15_Pos                  15                                             /**< (XDMAC_GRS) XDMAC Channel 15 Read Suspend Bit Position */
#define XDMAC_GRS_RS15_Msk                  (0x1U << XDMAC_GRS_RS15_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 15 Read Suspend Bit Mask */
#define XDMAC_GRS_RS15                      XDMAC_GRS_RS15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS15_Msk instead */
#define XDMAC_GRS_RS16_Pos                  16                                             /**< (XDMAC_GRS) XDMAC Channel 16 Read Suspend Bit Position */
#define XDMAC_GRS_RS16_Msk                  (0x1U << XDMAC_GRS_RS16_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 16 Read Suspend Bit Mask */
#define XDMAC_GRS_RS16                      XDMAC_GRS_RS16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS16_Msk instead */
#define XDMAC_GRS_RS17_Pos                  17                                             /**< (XDMAC_GRS) XDMAC Channel 17 Read Suspend Bit Position */
#define XDMAC_GRS_RS17_Msk                  (0x1U << XDMAC_GRS_RS17_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 17 Read Suspend Bit Mask */
#define XDMAC_GRS_RS17                      XDMAC_GRS_RS17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS17_Msk instead */
#define XDMAC_GRS_RS18_Pos                  18                                             /**< (XDMAC_GRS) XDMAC Channel 18 Read Suspend Bit Position */
#define XDMAC_GRS_RS18_Msk                  (0x1U << XDMAC_GRS_RS18_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 18 Read Suspend Bit Mask */
#define XDMAC_GRS_RS18                      XDMAC_GRS_RS18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS18_Msk instead */
#define XDMAC_GRS_RS19_Pos                  19                                             /**< (XDMAC_GRS) XDMAC Channel 19 Read Suspend Bit Position */
#define XDMAC_GRS_RS19_Msk                  (0x1U << XDMAC_GRS_RS19_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 19 Read Suspend Bit Mask */
#define XDMAC_GRS_RS19                      XDMAC_GRS_RS19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS19_Msk instead */
#define XDMAC_GRS_RS20_Pos                  20                                             /**< (XDMAC_GRS) XDMAC Channel 20 Read Suspend Bit Position */
#define XDMAC_GRS_RS20_Msk                  (0x1U << XDMAC_GRS_RS20_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 20 Read Suspend Bit Mask */
#define XDMAC_GRS_RS20                      XDMAC_GRS_RS20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS20_Msk instead */
#define XDMAC_GRS_RS21_Pos                  21                                             /**< (XDMAC_GRS) XDMAC Channel 21 Read Suspend Bit Position */
#define XDMAC_GRS_RS21_Msk                  (0x1U << XDMAC_GRS_RS21_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 21 Read Suspend Bit Mask */
#define XDMAC_GRS_RS21                      XDMAC_GRS_RS21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS21_Msk instead */
#define XDMAC_GRS_RS22_Pos                  22                                             /**< (XDMAC_GRS) XDMAC Channel 22 Read Suspend Bit Position */
#define XDMAC_GRS_RS22_Msk                  (0x1U << XDMAC_GRS_RS22_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 22 Read Suspend Bit Mask */
#define XDMAC_GRS_RS22                      XDMAC_GRS_RS22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS22_Msk instead */
#define XDMAC_GRS_RS23_Pos                  23                                             /**< (XDMAC_GRS) XDMAC Channel 23 Read Suspend Bit Position */
#define XDMAC_GRS_RS23_Msk                  (0x1U << XDMAC_GRS_RS23_Pos)                   /**< (XDMAC_GRS) XDMAC Channel 23 Read Suspend Bit Mask */
#define XDMAC_GRS_RS23                      XDMAC_GRS_RS23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRS_RS23_Msk instead */
#define XDMAC_GRS_RS_Pos                    0                                              /**< (XDMAC_GRS Position) XDMAC Channel 23 Read Suspend Bit */
#define XDMAC_GRS_RS_Msk                    (0xFFFFFFU << XDMAC_GRS_RS_Pos)                /**< (XDMAC_GRS Mask) RS */
#define XDMAC_GRS_RS(value)                 (XDMAC_GRS_RS_Msk & ((value) << XDMAC_GRS_RS_Pos))  
#define XDMAC_GRS_MASK                      (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GRS) Register MASK  (Use XDMAC_GRS_Msk instead)  */
#define XDMAC_GRS_Msk                       (0xFFFFFFU)                                    /**< (XDMAC_GRS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GWS : (XDMAC Offset: 0x2c) (R/W 32) Global Channel Write Suspend Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WS0:1;                     /**< bit:      0  XDMAC Channel 0 Write Suspend Bit        */
    uint32_t WS1:1;                     /**< bit:      1  XDMAC Channel 1 Write Suspend Bit        */
    uint32_t WS2:1;                     /**< bit:      2  XDMAC Channel 2 Write Suspend Bit        */
    uint32_t WS3:1;                     /**< bit:      3  XDMAC Channel 3 Write Suspend Bit        */
    uint32_t WS4:1;                     /**< bit:      4  XDMAC Channel 4 Write Suspend Bit        */
    uint32_t WS5:1;                     /**< bit:      5  XDMAC Channel 5 Write Suspend Bit        */
    uint32_t WS6:1;                     /**< bit:      6  XDMAC Channel 6 Write Suspend Bit        */
    uint32_t WS7:1;                     /**< bit:      7  XDMAC Channel 7 Write Suspend Bit        */
    uint32_t WS8:1;                     /**< bit:      8  XDMAC Channel 8 Write Suspend Bit        */
    uint32_t WS9:1;                     /**< bit:      9  XDMAC Channel 9 Write Suspend Bit        */
    uint32_t WS10:1;                    /**< bit:     10  XDMAC Channel 10 Write Suspend Bit       */
    uint32_t WS11:1;                    /**< bit:     11  XDMAC Channel 11 Write Suspend Bit       */
    uint32_t WS12:1;                    /**< bit:     12  XDMAC Channel 12 Write Suspend Bit       */
    uint32_t WS13:1;                    /**< bit:     13  XDMAC Channel 13 Write Suspend Bit       */
    uint32_t WS14:1;                    /**< bit:     14  XDMAC Channel 14 Write Suspend Bit       */
    uint32_t WS15:1;                    /**< bit:     15  XDMAC Channel 15 Write Suspend Bit       */
    uint32_t WS16:1;                    /**< bit:     16  XDMAC Channel 16 Write Suspend Bit       */
    uint32_t WS17:1;                    /**< bit:     17  XDMAC Channel 17 Write Suspend Bit       */
    uint32_t WS18:1;                    /**< bit:     18  XDMAC Channel 18 Write Suspend Bit       */
    uint32_t WS19:1;                    /**< bit:     19  XDMAC Channel 19 Write Suspend Bit       */
    uint32_t WS20:1;                    /**< bit:     20  XDMAC Channel 20 Write Suspend Bit       */
    uint32_t WS21:1;                    /**< bit:     21  XDMAC Channel 21 Write Suspend Bit       */
    uint32_t WS22:1;                    /**< bit:     22  XDMAC Channel 22 Write Suspend Bit       */
    uint32_t WS23:1;                    /**< bit:     23  XDMAC Channel 23 Write Suspend Bit       */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t WS:24;                     /**< bit:  0..23  XDMAC Channel 23 Write Suspend Bit       */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GWS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GWS_OFFSET                    (0x2C)                                        /**<  (XDMAC_GWS) Global Channel Write Suspend Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GWS_WS0_Pos                   0                                              /**< (XDMAC_GWS) XDMAC Channel 0 Write Suspend Bit Position */
#define XDMAC_GWS_WS0_Msk                   (0x1U << XDMAC_GWS_WS0_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 0 Write Suspend Bit Mask */
#define XDMAC_GWS_WS0                       XDMAC_GWS_WS0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS0_Msk instead */
#define XDMAC_GWS_WS1_Pos                   1                                              /**< (XDMAC_GWS) XDMAC Channel 1 Write Suspend Bit Position */
#define XDMAC_GWS_WS1_Msk                   (0x1U << XDMAC_GWS_WS1_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 1 Write Suspend Bit Mask */
#define XDMAC_GWS_WS1                       XDMAC_GWS_WS1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS1_Msk instead */
#define XDMAC_GWS_WS2_Pos                   2                                              /**< (XDMAC_GWS) XDMAC Channel 2 Write Suspend Bit Position */
#define XDMAC_GWS_WS2_Msk                   (0x1U << XDMAC_GWS_WS2_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 2 Write Suspend Bit Mask */
#define XDMAC_GWS_WS2                       XDMAC_GWS_WS2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS2_Msk instead */
#define XDMAC_GWS_WS3_Pos                   3                                              /**< (XDMAC_GWS) XDMAC Channel 3 Write Suspend Bit Position */
#define XDMAC_GWS_WS3_Msk                   (0x1U << XDMAC_GWS_WS3_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 3 Write Suspend Bit Mask */
#define XDMAC_GWS_WS3                       XDMAC_GWS_WS3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS3_Msk instead */
#define XDMAC_GWS_WS4_Pos                   4                                              /**< (XDMAC_GWS) XDMAC Channel 4 Write Suspend Bit Position */
#define XDMAC_GWS_WS4_Msk                   (0x1U << XDMAC_GWS_WS4_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 4 Write Suspend Bit Mask */
#define XDMAC_GWS_WS4                       XDMAC_GWS_WS4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS4_Msk instead */
#define XDMAC_GWS_WS5_Pos                   5                                              /**< (XDMAC_GWS) XDMAC Channel 5 Write Suspend Bit Position */
#define XDMAC_GWS_WS5_Msk                   (0x1U << XDMAC_GWS_WS5_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 5 Write Suspend Bit Mask */
#define XDMAC_GWS_WS5                       XDMAC_GWS_WS5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS5_Msk instead */
#define XDMAC_GWS_WS6_Pos                   6                                              /**< (XDMAC_GWS) XDMAC Channel 6 Write Suspend Bit Position */
#define XDMAC_GWS_WS6_Msk                   (0x1U << XDMAC_GWS_WS6_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 6 Write Suspend Bit Mask */
#define XDMAC_GWS_WS6                       XDMAC_GWS_WS6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS6_Msk instead */
#define XDMAC_GWS_WS7_Pos                   7                                              /**< (XDMAC_GWS) XDMAC Channel 7 Write Suspend Bit Position */
#define XDMAC_GWS_WS7_Msk                   (0x1U << XDMAC_GWS_WS7_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 7 Write Suspend Bit Mask */
#define XDMAC_GWS_WS7                       XDMAC_GWS_WS7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS7_Msk instead */
#define XDMAC_GWS_WS8_Pos                   8                                              /**< (XDMAC_GWS) XDMAC Channel 8 Write Suspend Bit Position */
#define XDMAC_GWS_WS8_Msk                   (0x1U << XDMAC_GWS_WS8_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 8 Write Suspend Bit Mask */
#define XDMAC_GWS_WS8                       XDMAC_GWS_WS8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS8_Msk instead */
#define XDMAC_GWS_WS9_Pos                   9                                              /**< (XDMAC_GWS) XDMAC Channel 9 Write Suspend Bit Position */
#define XDMAC_GWS_WS9_Msk                   (0x1U << XDMAC_GWS_WS9_Pos)                    /**< (XDMAC_GWS) XDMAC Channel 9 Write Suspend Bit Mask */
#define XDMAC_GWS_WS9                       XDMAC_GWS_WS9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS9_Msk instead */
#define XDMAC_GWS_WS10_Pos                  10                                             /**< (XDMAC_GWS) XDMAC Channel 10 Write Suspend Bit Position */
#define XDMAC_GWS_WS10_Msk                  (0x1U << XDMAC_GWS_WS10_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 10 Write Suspend Bit Mask */
#define XDMAC_GWS_WS10                      XDMAC_GWS_WS10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS10_Msk instead */
#define XDMAC_GWS_WS11_Pos                  11                                             /**< (XDMAC_GWS) XDMAC Channel 11 Write Suspend Bit Position */
#define XDMAC_GWS_WS11_Msk                  (0x1U << XDMAC_GWS_WS11_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 11 Write Suspend Bit Mask */
#define XDMAC_GWS_WS11                      XDMAC_GWS_WS11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS11_Msk instead */
#define XDMAC_GWS_WS12_Pos                  12                                             /**< (XDMAC_GWS) XDMAC Channel 12 Write Suspend Bit Position */
#define XDMAC_GWS_WS12_Msk                  (0x1U << XDMAC_GWS_WS12_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 12 Write Suspend Bit Mask */
#define XDMAC_GWS_WS12                      XDMAC_GWS_WS12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS12_Msk instead */
#define XDMAC_GWS_WS13_Pos                  13                                             /**< (XDMAC_GWS) XDMAC Channel 13 Write Suspend Bit Position */
#define XDMAC_GWS_WS13_Msk                  (0x1U << XDMAC_GWS_WS13_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 13 Write Suspend Bit Mask */
#define XDMAC_GWS_WS13                      XDMAC_GWS_WS13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS13_Msk instead */
#define XDMAC_GWS_WS14_Pos                  14                                             /**< (XDMAC_GWS) XDMAC Channel 14 Write Suspend Bit Position */
#define XDMAC_GWS_WS14_Msk                  (0x1U << XDMAC_GWS_WS14_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 14 Write Suspend Bit Mask */
#define XDMAC_GWS_WS14                      XDMAC_GWS_WS14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS14_Msk instead */
#define XDMAC_GWS_WS15_Pos                  15                                             /**< (XDMAC_GWS) XDMAC Channel 15 Write Suspend Bit Position */
#define XDMAC_GWS_WS15_Msk                  (0x1U << XDMAC_GWS_WS15_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 15 Write Suspend Bit Mask */
#define XDMAC_GWS_WS15                      XDMAC_GWS_WS15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS15_Msk instead */
#define XDMAC_GWS_WS16_Pos                  16                                             /**< (XDMAC_GWS) XDMAC Channel 16 Write Suspend Bit Position */
#define XDMAC_GWS_WS16_Msk                  (0x1U << XDMAC_GWS_WS16_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 16 Write Suspend Bit Mask */
#define XDMAC_GWS_WS16                      XDMAC_GWS_WS16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS16_Msk instead */
#define XDMAC_GWS_WS17_Pos                  17                                             /**< (XDMAC_GWS) XDMAC Channel 17 Write Suspend Bit Position */
#define XDMAC_GWS_WS17_Msk                  (0x1U << XDMAC_GWS_WS17_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 17 Write Suspend Bit Mask */
#define XDMAC_GWS_WS17                      XDMAC_GWS_WS17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS17_Msk instead */
#define XDMAC_GWS_WS18_Pos                  18                                             /**< (XDMAC_GWS) XDMAC Channel 18 Write Suspend Bit Position */
#define XDMAC_GWS_WS18_Msk                  (0x1U << XDMAC_GWS_WS18_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 18 Write Suspend Bit Mask */
#define XDMAC_GWS_WS18                      XDMAC_GWS_WS18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS18_Msk instead */
#define XDMAC_GWS_WS19_Pos                  19                                             /**< (XDMAC_GWS) XDMAC Channel 19 Write Suspend Bit Position */
#define XDMAC_GWS_WS19_Msk                  (0x1U << XDMAC_GWS_WS19_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 19 Write Suspend Bit Mask */
#define XDMAC_GWS_WS19                      XDMAC_GWS_WS19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS19_Msk instead */
#define XDMAC_GWS_WS20_Pos                  20                                             /**< (XDMAC_GWS) XDMAC Channel 20 Write Suspend Bit Position */
#define XDMAC_GWS_WS20_Msk                  (0x1U << XDMAC_GWS_WS20_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 20 Write Suspend Bit Mask */
#define XDMAC_GWS_WS20                      XDMAC_GWS_WS20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS20_Msk instead */
#define XDMAC_GWS_WS21_Pos                  21                                             /**< (XDMAC_GWS) XDMAC Channel 21 Write Suspend Bit Position */
#define XDMAC_GWS_WS21_Msk                  (0x1U << XDMAC_GWS_WS21_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 21 Write Suspend Bit Mask */
#define XDMAC_GWS_WS21                      XDMAC_GWS_WS21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS21_Msk instead */
#define XDMAC_GWS_WS22_Pos                  22                                             /**< (XDMAC_GWS) XDMAC Channel 22 Write Suspend Bit Position */
#define XDMAC_GWS_WS22_Msk                  (0x1U << XDMAC_GWS_WS22_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 22 Write Suspend Bit Mask */
#define XDMAC_GWS_WS22                      XDMAC_GWS_WS22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS22_Msk instead */
#define XDMAC_GWS_WS23_Pos                  23                                             /**< (XDMAC_GWS) XDMAC Channel 23 Write Suspend Bit Position */
#define XDMAC_GWS_WS23_Msk                  (0x1U << XDMAC_GWS_WS23_Pos)                   /**< (XDMAC_GWS) XDMAC Channel 23 Write Suspend Bit Mask */
#define XDMAC_GWS_WS23                      XDMAC_GWS_WS23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GWS_WS23_Msk instead */
#define XDMAC_GWS_WS_Pos                    0                                              /**< (XDMAC_GWS Position) XDMAC Channel 23 Write Suspend Bit */
#define XDMAC_GWS_WS_Msk                    (0xFFFFFFU << XDMAC_GWS_WS_Pos)                /**< (XDMAC_GWS Mask) WS */
#define XDMAC_GWS_WS(value)                 (XDMAC_GWS_WS_Msk & ((value) << XDMAC_GWS_WS_Pos))  
#define XDMAC_GWS_MASK                      (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GWS) Register MASK  (Use XDMAC_GWS_Msk instead)  */
#define XDMAC_GWS_Msk                       (0xFFFFFFU)                                    /**< (XDMAC_GWS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GRWS : (XDMAC Offset: 0x30) (/W 32) Global Channel Read Write Suspend Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RWS0:1;                    /**< bit:      0  XDMAC Channel 0 Read Write Suspend Bit   */
    uint32_t RWS1:1;                    /**< bit:      1  XDMAC Channel 1 Read Write Suspend Bit   */
    uint32_t RWS2:1;                    /**< bit:      2  XDMAC Channel 2 Read Write Suspend Bit   */
    uint32_t RWS3:1;                    /**< bit:      3  XDMAC Channel 3 Read Write Suspend Bit   */
    uint32_t RWS4:1;                    /**< bit:      4  XDMAC Channel 4 Read Write Suspend Bit   */
    uint32_t RWS5:1;                    /**< bit:      5  XDMAC Channel 5 Read Write Suspend Bit   */
    uint32_t RWS6:1;                    /**< bit:      6  XDMAC Channel 6 Read Write Suspend Bit   */
    uint32_t RWS7:1;                    /**< bit:      7  XDMAC Channel 7 Read Write Suspend Bit   */
    uint32_t RWS8:1;                    /**< bit:      8  XDMAC Channel 8 Read Write Suspend Bit   */
    uint32_t RWS9:1;                    /**< bit:      9  XDMAC Channel 9 Read Write Suspend Bit   */
    uint32_t RWS10:1;                   /**< bit:     10  XDMAC Channel 10 Read Write Suspend Bit  */
    uint32_t RWS11:1;                   /**< bit:     11  XDMAC Channel 11 Read Write Suspend Bit  */
    uint32_t RWS12:1;                   /**< bit:     12  XDMAC Channel 12 Read Write Suspend Bit  */
    uint32_t RWS13:1;                   /**< bit:     13  XDMAC Channel 13 Read Write Suspend Bit  */
    uint32_t RWS14:1;                   /**< bit:     14  XDMAC Channel 14 Read Write Suspend Bit  */
    uint32_t RWS15:1;                   /**< bit:     15  XDMAC Channel 15 Read Write Suspend Bit  */
    uint32_t RWS16:1;                   /**< bit:     16  XDMAC Channel 16 Read Write Suspend Bit  */
    uint32_t RWS17:1;                   /**< bit:     17  XDMAC Channel 17 Read Write Suspend Bit  */
    uint32_t RWS18:1;                   /**< bit:     18  XDMAC Channel 18 Read Write Suspend Bit  */
    uint32_t RWS19:1;                   /**< bit:     19  XDMAC Channel 19 Read Write Suspend Bit  */
    uint32_t RWS20:1;                   /**< bit:     20  XDMAC Channel 20 Read Write Suspend Bit  */
    uint32_t RWS21:1;                   /**< bit:     21  XDMAC Channel 21 Read Write Suspend Bit  */
    uint32_t RWS22:1;                   /**< bit:     22  XDMAC Channel 22 Read Write Suspend Bit  */
    uint32_t RWS23:1;                   /**< bit:     23  XDMAC Channel 23 Read Write Suspend Bit  */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t RWS:24;                    /**< bit:  0..23  XDMAC Channel 23 Read Write Suspend Bit  */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GRWS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GRWS_OFFSET                   (0x30)                                        /**<  (XDMAC_GRWS) Global Channel Read Write Suspend Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GRWS_RWS0_Pos                 0                                              /**< (XDMAC_GRWS) XDMAC Channel 0 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS0_Msk                 (0x1U << XDMAC_GRWS_RWS0_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 0 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS0                     XDMAC_GRWS_RWS0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS0_Msk instead */
#define XDMAC_GRWS_RWS1_Pos                 1                                              /**< (XDMAC_GRWS) XDMAC Channel 1 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS1_Msk                 (0x1U << XDMAC_GRWS_RWS1_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 1 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS1                     XDMAC_GRWS_RWS1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS1_Msk instead */
#define XDMAC_GRWS_RWS2_Pos                 2                                              /**< (XDMAC_GRWS) XDMAC Channel 2 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS2_Msk                 (0x1U << XDMAC_GRWS_RWS2_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 2 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS2                     XDMAC_GRWS_RWS2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS2_Msk instead */
#define XDMAC_GRWS_RWS3_Pos                 3                                              /**< (XDMAC_GRWS) XDMAC Channel 3 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS3_Msk                 (0x1U << XDMAC_GRWS_RWS3_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 3 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS3                     XDMAC_GRWS_RWS3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS3_Msk instead */
#define XDMAC_GRWS_RWS4_Pos                 4                                              /**< (XDMAC_GRWS) XDMAC Channel 4 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS4_Msk                 (0x1U << XDMAC_GRWS_RWS4_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 4 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS4                     XDMAC_GRWS_RWS4_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS4_Msk instead */
#define XDMAC_GRWS_RWS5_Pos                 5                                              /**< (XDMAC_GRWS) XDMAC Channel 5 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS5_Msk                 (0x1U << XDMAC_GRWS_RWS5_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 5 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS5                     XDMAC_GRWS_RWS5_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS5_Msk instead */
#define XDMAC_GRWS_RWS6_Pos                 6                                              /**< (XDMAC_GRWS) XDMAC Channel 6 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS6_Msk                 (0x1U << XDMAC_GRWS_RWS6_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 6 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS6                     XDMAC_GRWS_RWS6_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS6_Msk instead */
#define XDMAC_GRWS_RWS7_Pos                 7                                              /**< (XDMAC_GRWS) XDMAC Channel 7 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS7_Msk                 (0x1U << XDMAC_GRWS_RWS7_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 7 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS7                     XDMAC_GRWS_RWS7_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS7_Msk instead */
#define XDMAC_GRWS_RWS8_Pos                 8                                              /**< (XDMAC_GRWS) XDMAC Channel 8 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS8_Msk                 (0x1U << XDMAC_GRWS_RWS8_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 8 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS8                     XDMAC_GRWS_RWS8_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS8_Msk instead */
#define XDMAC_GRWS_RWS9_Pos                 9                                              /**< (XDMAC_GRWS) XDMAC Channel 9 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS9_Msk                 (0x1U << XDMAC_GRWS_RWS9_Pos)                  /**< (XDMAC_GRWS) XDMAC Channel 9 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS9                     XDMAC_GRWS_RWS9_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS9_Msk instead */
#define XDMAC_GRWS_RWS10_Pos                10                                             /**< (XDMAC_GRWS) XDMAC Channel 10 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS10_Msk                (0x1U << XDMAC_GRWS_RWS10_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 10 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS10                    XDMAC_GRWS_RWS10_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS10_Msk instead */
#define XDMAC_GRWS_RWS11_Pos                11                                             /**< (XDMAC_GRWS) XDMAC Channel 11 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS11_Msk                (0x1U << XDMAC_GRWS_RWS11_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 11 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS11                    XDMAC_GRWS_RWS11_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS11_Msk instead */
#define XDMAC_GRWS_RWS12_Pos                12                                             /**< (XDMAC_GRWS) XDMAC Channel 12 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS12_Msk                (0x1U << XDMAC_GRWS_RWS12_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 12 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS12                    XDMAC_GRWS_RWS12_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS12_Msk instead */
#define XDMAC_GRWS_RWS13_Pos                13                                             /**< (XDMAC_GRWS) XDMAC Channel 13 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS13_Msk                (0x1U << XDMAC_GRWS_RWS13_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 13 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS13                    XDMAC_GRWS_RWS13_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS13_Msk instead */
#define XDMAC_GRWS_RWS14_Pos                14                                             /**< (XDMAC_GRWS) XDMAC Channel 14 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS14_Msk                (0x1U << XDMAC_GRWS_RWS14_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 14 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS14                    XDMAC_GRWS_RWS14_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS14_Msk instead */
#define XDMAC_GRWS_RWS15_Pos                15                                             /**< (XDMAC_GRWS) XDMAC Channel 15 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS15_Msk                (0x1U << XDMAC_GRWS_RWS15_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 15 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS15                    XDMAC_GRWS_RWS15_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS15_Msk instead */
#define XDMAC_GRWS_RWS16_Pos                16                                             /**< (XDMAC_GRWS) XDMAC Channel 16 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS16_Msk                (0x1U << XDMAC_GRWS_RWS16_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 16 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS16                    XDMAC_GRWS_RWS16_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS16_Msk instead */
#define XDMAC_GRWS_RWS17_Pos                17                                             /**< (XDMAC_GRWS) XDMAC Channel 17 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS17_Msk                (0x1U << XDMAC_GRWS_RWS17_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 17 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS17                    XDMAC_GRWS_RWS17_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS17_Msk instead */
#define XDMAC_GRWS_RWS18_Pos                18                                             /**< (XDMAC_GRWS) XDMAC Channel 18 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS18_Msk                (0x1U << XDMAC_GRWS_RWS18_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 18 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS18                    XDMAC_GRWS_RWS18_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS18_Msk instead */
#define XDMAC_GRWS_RWS19_Pos                19                                             /**< (XDMAC_GRWS) XDMAC Channel 19 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS19_Msk                (0x1U << XDMAC_GRWS_RWS19_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 19 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS19                    XDMAC_GRWS_RWS19_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS19_Msk instead */
#define XDMAC_GRWS_RWS20_Pos                20                                             /**< (XDMAC_GRWS) XDMAC Channel 20 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS20_Msk                (0x1U << XDMAC_GRWS_RWS20_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 20 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS20                    XDMAC_GRWS_RWS20_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS20_Msk instead */
#define XDMAC_GRWS_RWS21_Pos                21                                             /**< (XDMAC_GRWS) XDMAC Channel 21 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS21_Msk                (0x1U << XDMAC_GRWS_RWS21_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 21 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS21                    XDMAC_GRWS_RWS21_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS21_Msk instead */
#define XDMAC_GRWS_RWS22_Pos                22                                             /**< (XDMAC_GRWS) XDMAC Channel 22 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS22_Msk                (0x1U << XDMAC_GRWS_RWS22_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 22 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS22                    XDMAC_GRWS_RWS22_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS22_Msk instead */
#define XDMAC_GRWS_RWS23_Pos                23                                             /**< (XDMAC_GRWS) XDMAC Channel 23 Read Write Suspend Bit Position */
#define XDMAC_GRWS_RWS23_Msk                (0x1U << XDMAC_GRWS_RWS23_Pos)                 /**< (XDMAC_GRWS) XDMAC Channel 23 Read Write Suspend Bit Mask */
#define XDMAC_GRWS_RWS23                    XDMAC_GRWS_RWS23_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWS_RWS23_Msk instead */
#define XDMAC_GRWS_RWS_Pos                  0                                              /**< (XDMAC_GRWS Position) XDMAC Channel 23 Read Write Suspend Bit */
#define XDMAC_GRWS_RWS_Msk                  (0xFFFFFFU << XDMAC_GRWS_RWS_Pos)              /**< (XDMAC_GRWS Mask) RWS */
#define XDMAC_GRWS_RWS(value)               (XDMAC_GRWS_RWS_Msk & ((value) << XDMAC_GRWS_RWS_Pos))  
#define XDMAC_GRWS_MASK                     (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GRWS) Register MASK  (Use XDMAC_GRWS_Msk instead)  */
#define XDMAC_GRWS_Msk                      (0xFFFFFFU)                                    /**< (XDMAC_GRWS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GRWR : (XDMAC Offset: 0x34) (/W 32) Global Channel Read Write Resume Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RWR0:1;                    /**< bit:      0  XDMAC Channel 0 Read Write Resume Bit    */
    uint32_t RWR1:1;                    /**< bit:      1  XDMAC Channel 1 Read Write Resume Bit    */
    uint32_t RWR2:1;                    /**< bit:      2  XDMAC Channel 2 Read Write Resume Bit    */
    uint32_t RWR3:1;                    /**< bit:      3  XDMAC Channel 3 Read Write Resume Bit    */
    uint32_t RWR4:1;                    /**< bit:      4  XDMAC Channel 4 Read Write Resume Bit    */
    uint32_t RWR5:1;                    /**< bit:      5  XDMAC Channel 5 Read Write Resume Bit    */
    uint32_t RWR6:1;                    /**< bit:      6  XDMAC Channel 6 Read Write Resume Bit    */
    uint32_t RWR7:1;                    /**< bit:      7  XDMAC Channel 7 Read Write Resume Bit    */
    uint32_t RWR8:1;                    /**< bit:      8  XDMAC Channel 8 Read Write Resume Bit    */
    uint32_t RWR9:1;                    /**< bit:      9  XDMAC Channel 9 Read Write Resume Bit    */
    uint32_t RWR10:1;                   /**< bit:     10  XDMAC Channel 10 Read Write Resume Bit   */
    uint32_t RWR11:1;                   /**< bit:     11  XDMAC Channel 11 Read Write Resume Bit   */
    uint32_t RWR12:1;                   /**< bit:     12  XDMAC Channel 12 Read Write Resume Bit   */
    uint32_t RWR13:1;                   /**< bit:     13  XDMAC Channel 13 Read Write Resume Bit   */
    uint32_t RWR14:1;                   /**< bit:     14  XDMAC Channel 14 Read Write Resume Bit   */
    uint32_t RWR15:1;                   /**< bit:     15  XDMAC Channel 15 Read Write Resume Bit   */
    uint32_t RWR16:1;                   /**< bit:     16  XDMAC Channel 16 Read Write Resume Bit   */
    uint32_t RWR17:1;                   /**< bit:     17  XDMAC Channel 17 Read Write Resume Bit   */
    uint32_t RWR18:1;                   /**< bit:     18  XDMAC Channel 18 Read Write Resume Bit   */
    uint32_t RWR19:1;                   /**< bit:     19  XDMAC Channel 19 Read Write Resume Bit   */
    uint32_t RWR20:1;                   /**< bit:     20  XDMAC Channel 20 Read Write Resume Bit   */
    uint32_t RWR21:1;                   /**< bit:     21  XDMAC Channel 21 Read Write Resume Bit   */
    uint32_t RWR22:1;                   /**< bit:     22  XDMAC Channel 22 Read Write Resume Bit   */
    uint32_t RWR23:1;                   /**< bit:     23  XDMAC Channel 23 Read Write Resume Bit   */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t RWR:24;                    /**< bit:  0..23  XDMAC Channel 23 Read Write Resume Bit   */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GRWR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GRWR_OFFSET                   (0x34)                                        /**<  (XDMAC_GRWR) Global Channel Read Write Resume Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GRWR_RWR0_Pos                 0                                              /**< (XDMAC_GRWR) XDMAC Channel 0 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR0_Msk                 (0x1U << XDMAC_GRWR_RWR0_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 0 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR0                     XDMAC_GRWR_RWR0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR0_Msk instead */
#define XDMAC_GRWR_RWR1_Pos                 1                                              /**< (XDMAC_GRWR) XDMAC Channel 1 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR1_Msk                 (0x1U << XDMAC_GRWR_RWR1_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 1 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR1                     XDMAC_GRWR_RWR1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR1_Msk instead */
#define XDMAC_GRWR_RWR2_Pos                 2                                              /**< (XDMAC_GRWR) XDMAC Channel 2 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR2_Msk                 (0x1U << XDMAC_GRWR_RWR2_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 2 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR2                     XDMAC_GRWR_RWR2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR2_Msk instead */
#define XDMAC_GRWR_RWR3_Pos                 3                                              /**< (XDMAC_GRWR) XDMAC Channel 3 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR3_Msk                 (0x1U << XDMAC_GRWR_RWR3_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 3 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR3                     XDMAC_GRWR_RWR3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR3_Msk instead */
#define XDMAC_GRWR_RWR4_Pos                 4                                              /**< (XDMAC_GRWR) XDMAC Channel 4 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR4_Msk                 (0x1U << XDMAC_GRWR_RWR4_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 4 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR4                     XDMAC_GRWR_RWR4_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR4_Msk instead */
#define XDMAC_GRWR_RWR5_Pos                 5                                              /**< (XDMAC_GRWR) XDMAC Channel 5 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR5_Msk                 (0x1U << XDMAC_GRWR_RWR5_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 5 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR5                     XDMAC_GRWR_RWR5_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR5_Msk instead */
#define XDMAC_GRWR_RWR6_Pos                 6                                              /**< (XDMAC_GRWR) XDMAC Channel 6 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR6_Msk                 (0x1U << XDMAC_GRWR_RWR6_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 6 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR6                     XDMAC_GRWR_RWR6_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR6_Msk instead */
#define XDMAC_GRWR_RWR7_Pos                 7                                              /**< (XDMAC_GRWR) XDMAC Channel 7 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR7_Msk                 (0x1U << XDMAC_GRWR_RWR7_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 7 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR7                     XDMAC_GRWR_RWR7_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR7_Msk instead */
#define XDMAC_GRWR_RWR8_Pos                 8                                              /**< (XDMAC_GRWR) XDMAC Channel 8 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR8_Msk                 (0x1U << XDMAC_GRWR_RWR8_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 8 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR8                     XDMAC_GRWR_RWR8_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR8_Msk instead */
#define XDMAC_GRWR_RWR9_Pos                 9                                              /**< (XDMAC_GRWR) XDMAC Channel 9 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR9_Msk                 (0x1U << XDMAC_GRWR_RWR9_Pos)                  /**< (XDMAC_GRWR) XDMAC Channel 9 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR9                     XDMAC_GRWR_RWR9_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR9_Msk instead */
#define XDMAC_GRWR_RWR10_Pos                10                                             /**< (XDMAC_GRWR) XDMAC Channel 10 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR10_Msk                (0x1U << XDMAC_GRWR_RWR10_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 10 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR10                    XDMAC_GRWR_RWR10_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR10_Msk instead */
#define XDMAC_GRWR_RWR11_Pos                11                                             /**< (XDMAC_GRWR) XDMAC Channel 11 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR11_Msk                (0x1U << XDMAC_GRWR_RWR11_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 11 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR11                    XDMAC_GRWR_RWR11_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR11_Msk instead */
#define XDMAC_GRWR_RWR12_Pos                12                                             /**< (XDMAC_GRWR) XDMAC Channel 12 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR12_Msk                (0x1U << XDMAC_GRWR_RWR12_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 12 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR12                    XDMAC_GRWR_RWR12_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR12_Msk instead */
#define XDMAC_GRWR_RWR13_Pos                13                                             /**< (XDMAC_GRWR) XDMAC Channel 13 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR13_Msk                (0x1U << XDMAC_GRWR_RWR13_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 13 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR13                    XDMAC_GRWR_RWR13_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR13_Msk instead */
#define XDMAC_GRWR_RWR14_Pos                14                                             /**< (XDMAC_GRWR) XDMAC Channel 14 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR14_Msk                (0x1U << XDMAC_GRWR_RWR14_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 14 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR14                    XDMAC_GRWR_RWR14_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR14_Msk instead */
#define XDMAC_GRWR_RWR15_Pos                15                                             /**< (XDMAC_GRWR) XDMAC Channel 15 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR15_Msk                (0x1U << XDMAC_GRWR_RWR15_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 15 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR15                    XDMAC_GRWR_RWR15_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR15_Msk instead */
#define XDMAC_GRWR_RWR16_Pos                16                                             /**< (XDMAC_GRWR) XDMAC Channel 16 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR16_Msk                (0x1U << XDMAC_GRWR_RWR16_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 16 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR16                    XDMAC_GRWR_RWR16_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR16_Msk instead */
#define XDMAC_GRWR_RWR17_Pos                17                                             /**< (XDMAC_GRWR) XDMAC Channel 17 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR17_Msk                (0x1U << XDMAC_GRWR_RWR17_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 17 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR17                    XDMAC_GRWR_RWR17_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR17_Msk instead */
#define XDMAC_GRWR_RWR18_Pos                18                                             /**< (XDMAC_GRWR) XDMAC Channel 18 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR18_Msk                (0x1U << XDMAC_GRWR_RWR18_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 18 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR18                    XDMAC_GRWR_RWR18_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR18_Msk instead */
#define XDMAC_GRWR_RWR19_Pos                19                                             /**< (XDMAC_GRWR) XDMAC Channel 19 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR19_Msk                (0x1U << XDMAC_GRWR_RWR19_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 19 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR19                    XDMAC_GRWR_RWR19_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR19_Msk instead */
#define XDMAC_GRWR_RWR20_Pos                20                                             /**< (XDMAC_GRWR) XDMAC Channel 20 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR20_Msk                (0x1U << XDMAC_GRWR_RWR20_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 20 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR20                    XDMAC_GRWR_RWR20_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR20_Msk instead */
#define XDMAC_GRWR_RWR21_Pos                21                                             /**< (XDMAC_GRWR) XDMAC Channel 21 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR21_Msk                (0x1U << XDMAC_GRWR_RWR21_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 21 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR21                    XDMAC_GRWR_RWR21_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR21_Msk instead */
#define XDMAC_GRWR_RWR22_Pos                22                                             /**< (XDMAC_GRWR) XDMAC Channel 22 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR22_Msk                (0x1U << XDMAC_GRWR_RWR22_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 22 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR22                    XDMAC_GRWR_RWR22_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR22_Msk instead */
#define XDMAC_GRWR_RWR23_Pos                23                                             /**< (XDMAC_GRWR) XDMAC Channel 23 Read Write Resume Bit Position */
#define XDMAC_GRWR_RWR23_Msk                (0x1U << XDMAC_GRWR_RWR23_Pos)                 /**< (XDMAC_GRWR) XDMAC Channel 23 Read Write Resume Bit Mask */
#define XDMAC_GRWR_RWR23                    XDMAC_GRWR_RWR23_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GRWR_RWR23_Msk instead */
#define XDMAC_GRWR_RWR_Pos                  0                                              /**< (XDMAC_GRWR Position) XDMAC Channel 23 Read Write Resume Bit */
#define XDMAC_GRWR_RWR_Msk                  (0xFFFFFFU << XDMAC_GRWR_RWR_Pos)              /**< (XDMAC_GRWR Mask) RWR */
#define XDMAC_GRWR_RWR(value)               (XDMAC_GRWR_RWR_Msk & ((value) << XDMAC_GRWR_RWR_Pos))  
#define XDMAC_GRWR_MASK                     (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GRWR) Register MASK  (Use XDMAC_GRWR_Msk instead)  */
#define XDMAC_GRWR_Msk                      (0xFFFFFFU)                                    /**< (XDMAC_GRWR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GSWR : (XDMAC Offset: 0x38) (/W 32) Global Channel Software Request Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SWREQ0:1;                  /**< bit:      0  XDMAC Channel 0 Software Request Bit     */
    uint32_t SWREQ1:1;                  /**< bit:      1  XDMAC Channel 1 Software Request Bit     */
    uint32_t SWREQ2:1;                  /**< bit:      2  XDMAC Channel 2 Software Request Bit     */
    uint32_t SWREQ3:1;                  /**< bit:      3  XDMAC Channel 3 Software Request Bit     */
    uint32_t SWREQ4:1;                  /**< bit:      4  XDMAC Channel 4 Software Request Bit     */
    uint32_t SWREQ5:1;                  /**< bit:      5  XDMAC Channel 5 Software Request Bit     */
    uint32_t SWREQ6:1;                  /**< bit:      6  XDMAC Channel 6 Software Request Bit     */
    uint32_t SWREQ7:1;                  /**< bit:      7  XDMAC Channel 7 Software Request Bit     */
    uint32_t SWREQ8:1;                  /**< bit:      8  XDMAC Channel 8 Software Request Bit     */
    uint32_t SWREQ9:1;                  /**< bit:      9  XDMAC Channel 9 Software Request Bit     */
    uint32_t SWREQ10:1;                 /**< bit:     10  XDMAC Channel 10 Software Request Bit    */
    uint32_t SWREQ11:1;                 /**< bit:     11  XDMAC Channel 11 Software Request Bit    */
    uint32_t SWREQ12:1;                 /**< bit:     12  XDMAC Channel 12 Software Request Bit    */
    uint32_t SWREQ13:1;                 /**< bit:     13  XDMAC Channel 13 Software Request Bit    */
    uint32_t SWREQ14:1;                 /**< bit:     14  XDMAC Channel 14 Software Request Bit    */
    uint32_t SWREQ15:1;                 /**< bit:     15  XDMAC Channel 15 Software Request Bit    */
    uint32_t SWREQ16:1;                 /**< bit:     16  XDMAC Channel 16 Software Request Bit    */
    uint32_t SWREQ17:1;                 /**< bit:     17  XDMAC Channel 17 Software Request Bit    */
    uint32_t SWREQ18:1;                 /**< bit:     18  XDMAC Channel 18 Software Request Bit    */
    uint32_t SWREQ19:1;                 /**< bit:     19  XDMAC Channel 19 Software Request Bit    */
    uint32_t SWREQ20:1;                 /**< bit:     20  XDMAC Channel 20 Software Request Bit    */
    uint32_t SWREQ21:1;                 /**< bit:     21  XDMAC Channel 21 Software Request Bit    */
    uint32_t SWREQ22:1;                 /**< bit:     22  XDMAC Channel 22 Software Request Bit    */
    uint32_t SWREQ23:1;                 /**< bit:     23  XDMAC Channel 23 Software Request Bit    */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t SWREQ:24;                  /**< bit:  0..23  XDMAC Channel 23 Software Request Bit    */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GSWR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GSWR_OFFSET                   (0x38)                                        /**<  (XDMAC_GSWR) Global Channel Software Request Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GSWR_SWREQ0_Pos               0                                              /**< (XDMAC_GSWR) XDMAC Channel 0 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ0_Msk               (0x1U << XDMAC_GSWR_SWREQ0_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 0 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ0                   XDMAC_GSWR_SWREQ0_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ0_Msk instead */
#define XDMAC_GSWR_SWREQ1_Pos               1                                              /**< (XDMAC_GSWR) XDMAC Channel 1 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ1_Msk               (0x1U << XDMAC_GSWR_SWREQ1_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 1 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ1                   XDMAC_GSWR_SWREQ1_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ1_Msk instead */
#define XDMAC_GSWR_SWREQ2_Pos               2                                              /**< (XDMAC_GSWR) XDMAC Channel 2 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ2_Msk               (0x1U << XDMAC_GSWR_SWREQ2_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 2 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ2                   XDMAC_GSWR_SWREQ2_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ2_Msk instead */
#define XDMAC_GSWR_SWREQ3_Pos               3                                              /**< (XDMAC_GSWR) XDMAC Channel 3 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ3_Msk               (0x1U << XDMAC_GSWR_SWREQ3_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 3 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ3                   XDMAC_GSWR_SWREQ3_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ3_Msk instead */
#define XDMAC_GSWR_SWREQ4_Pos               4                                              /**< (XDMAC_GSWR) XDMAC Channel 4 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ4_Msk               (0x1U << XDMAC_GSWR_SWREQ4_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 4 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ4                   XDMAC_GSWR_SWREQ4_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ4_Msk instead */
#define XDMAC_GSWR_SWREQ5_Pos               5                                              /**< (XDMAC_GSWR) XDMAC Channel 5 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ5_Msk               (0x1U << XDMAC_GSWR_SWREQ5_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 5 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ5                   XDMAC_GSWR_SWREQ5_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ5_Msk instead */
#define XDMAC_GSWR_SWREQ6_Pos               6                                              /**< (XDMAC_GSWR) XDMAC Channel 6 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ6_Msk               (0x1U << XDMAC_GSWR_SWREQ6_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 6 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ6                   XDMAC_GSWR_SWREQ6_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ6_Msk instead */
#define XDMAC_GSWR_SWREQ7_Pos               7                                              /**< (XDMAC_GSWR) XDMAC Channel 7 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ7_Msk               (0x1U << XDMAC_GSWR_SWREQ7_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 7 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ7                   XDMAC_GSWR_SWREQ7_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ7_Msk instead */
#define XDMAC_GSWR_SWREQ8_Pos               8                                              /**< (XDMAC_GSWR) XDMAC Channel 8 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ8_Msk               (0x1U << XDMAC_GSWR_SWREQ8_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 8 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ8                   XDMAC_GSWR_SWREQ8_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ8_Msk instead */
#define XDMAC_GSWR_SWREQ9_Pos               9                                              /**< (XDMAC_GSWR) XDMAC Channel 9 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ9_Msk               (0x1U << XDMAC_GSWR_SWREQ9_Pos)                /**< (XDMAC_GSWR) XDMAC Channel 9 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ9                   XDMAC_GSWR_SWREQ9_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ9_Msk instead */
#define XDMAC_GSWR_SWREQ10_Pos              10                                             /**< (XDMAC_GSWR) XDMAC Channel 10 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ10_Msk              (0x1U << XDMAC_GSWR_SWREQ10_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 10 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ10                  XDMAC_GSWR_SWREQ10_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ10_Msk instead */
#define XDMAC_GSWR_SWREQ11_Pos              11                                             /**< (XDMAC_GSWR) XDMAC Channel 11 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ11_Msk              (0x1U << XDMAC_GSWR_SWREQ11_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 11 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ11                  XDMAC_GSWR_SWREQ11_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ11_Msk instead */
#define XDMAC_GSWR_SWREQ12_Pos              12                                             /**< (XDMAC_GSWR) XDMAC Channel 12 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ12_Msk              (0x1U << XDMAC_GSWR_SWREQ12_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 12 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ12                  XDMAC_GSWR_SWREQ12_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ12_Msk instead */
#define XDMAC_GSWR_SWREQ13_Pos              13                                             /**< (XDMAC_GSWR) XDMAC Channel 13 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ13_Msk              (0x1U << XDMAC_GSWR_SWREQ13_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 13 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ13                  XDMAC_GSWR_SWREQ13_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ13_Msk instead */
#define XDMAC_GSWR_SWREQ14_Pos              14                                             /**< (XDMAC_GSWR) XDMAC Channel 14 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ14_Msk              (0x1U << XDMAC_GSWR_SWREQ14_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 14 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ14                  XDMAC_GSWR_SWREQ14_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ14_Msk instead */
#define XDMAC_GSWR_SWREQ15_Pos              15                                             /**< (XDMAC_GSWR) XDMAC Channel 15 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ15_Msk              (0x1U << XDMAC_GSWR_SWREQ15_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 15 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ15                  XDMAC_GSWR_SWREQ15_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ15_Msk instead */
#define XDMAC_GSWR_SWREQ16_Pos              16                                             /**< (XDMAC_GSWR) XDMAC Channel 16 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ16_Msk              (0x1U << XDMAC_GSWR_SWREQ16_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 16 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ16                  XDMAC_GSWR_SWREQ16_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ16_Msk instead */
#define XDMAC_GSWR_SWREQ17_Pos              17                                             /**< (XDMAC_GSWR) XDMAC Channel 17 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ17_Msk              (0x1U << XDMAC_GSWR_SWREQ17_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 17 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ17                  XDMAC_GSWR_SWREQ17_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ17_Msk instead */
#define XDMAC_GSWR_SWREQ18_Pos              18                                             /**< (XDMAC_GSWR) XDMAC Channel 18 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ18_Msk              (0x1U << XDMAC_GSWR_SWREQ18_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 18 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ18                  XDMAC_GSWR_SWREQ18_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ18_Msk instead */
#define XDMAC_GSWR_SWREQ19_Pos              19                                             /**< (XDMAC_GSWR) XDMAC Channel 19 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ19_Msk              (0x1U << XDMAC_GSWR_SWREQ19_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 19 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ19                  XDMAC_GSWR_SWREQ19_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ19_Msk instead */
#define XDMAC_GSWR_SWREQ20_Pos              20                                             /**< (XDMAC_GSWR) XDMAC Channel 20 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ20_Msk              (0x1U << XDMAC_GSWR_SWREQ20_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 20 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ20                  XDMAC_GSWR_SWREQ20_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ20_Msk instead */
#define XDMAC_GSWR_SWREQ21_Pos              21                                             /**< (XDMAC_GSWR) XDMAC Channel 21 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ21_Msk              (0x1U << XDMAC_GSWR_SWREQ21_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 21 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ21                  XDMAC_GSWR_SWREQ21_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ21_Msk instead */
#define XDMAC_GSWR_SWREQ22_Pos              22                                             /**< (XDMAC_GSWR) XDMAC Channel 22 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ22_Msk              (0x1U << XDMAC_GSWR_SWREQ22_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 22 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ22                  XDMAC_GSWR_SWREQ22_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ22_Msk instead */
#define XDMAC_GSWR_SWREQ23_Pos              23                                             /**< (XDMAC_GSWR) XDMAC Channel 23 Software Request Bit Position */
#define XDMAC_GSWR_SWREQ23_Msk              (0x1U << XDMAC_GSWR_SWREQ23_Pos)               /**< (XDMAC_GSWR) XDMAC Channel 23 Software Request Bit Mask */
#define XDMAC_GSWR_SWREQ23                  XDMAC_GSWR_SWREQ23_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWR_SWREQ23_Msk instead */
#define XDMAC_GSWR_SWREQ_Pos                0                                              /**< (XDMAC_GSWR Position) XDMAC Channel 23 Software Request Bit */
#define XDMAC_GSWR_SWREQ_Msk                (0xFFFFFFU << XDMAC_GSWR_SWREQ_Pos)            /**< (XDMAC_GSWR Mask) SWREQ */
#define XDMAC_GSWR_SWREQ(value)             (XDMAC_GSWR_SWREQ_Msk & ((value) << XDMAC_GSWR_SWREQ_Pos))  
#define XDMAC_GSWR_MASK                     (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GSWR) Register MASK  (Use XDMAC_GSWR_Msk instead)  */
#define XDMAC_GSWR_Msk                      (0xFFFFFFU)                                    /**< (XDMAC_GSWR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GSWS : (XDMAC Offset: 0x3c) (R/ 32) Global Channel Software Request Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SWRS0:1;                   /**< bit:      0  XDMAC Channel 0 Software Request Status Bit */
    uint32_t SWRS1:1;                   /**< bit:      1  XDMAC Channel 1 Software Request Status Bit */
    uint32_t SWRS2:1;                   /**< bit:      2  XDMAC Channel 2 Software Request Status Bit */
    uint32_t SWRS3:1;                   /**< bit:      3  XDMAC Channel 3 Software Request Status Bit */
    uint32_t SWRS4:1;                   /**< bit:      4  XDMAC Channel 4 Software Request Status Bit */
    uint32_t SWRS5:1;                   /**< bit:      5  XDMAC Channel 5 Software Request Status Bit */
    uint32_t SWRS6:1;                   /**< bit:      6  XDMAC Channel 6 Software Request Status Bit */
    uint32_t SWRS7:1;                   /**< bit:      7  XDMAC Channel 7 Software Request Status Bit */
    uint32_t SWRS8:1;                   /**< bit:      8  XDMAC Channel 8 Software Request Status Bit */
    uint32_t SWRS9:1;                   /**< bit:      9  XDMAC Channel 9 Software Request Status Bit */
    uint32_t SWRS10:1;                  /**< bit:     10  XDMAC Channel 10 Software Request Status Bit */
    uint32_t SWRS11:1;                  /**< bit:     11  XDMAC Channel 11 Software Request Status Bit */
    uint32_t SWRS12:1;                  /**< bit:     12  XDMAC Channel 12 Software Request Status Bit */
    uint32_t SWRS13:1;                  /**< bit:     13  XDMAC Channel 13 Software Request Status Bit */
    uint32_t SWRS14:1;                  /**< bit:     14  XDMAC Channel 14 Software Request Status Bit */
    uint32_t SWRS15:1;                  /**< bit:     15  XDMAC Channel 15 Software Request Status Bit */
    uint32_t SWRS16:1;                  /**< bit:     16  XDMAC Channel 16 Software Request Status Bit */
    uint32_t SWRS17:1;                  /**< bit:     17  XDMAC Channel 17 Software Request Status Bit */
    uint32_t SWRS18:1;                  /**< bit:     18  XDMAC Channel 18 Software Request Status Bit */
    uint32_t SWRS19:1;                  /**< bit:     19  XDMAC Channel 19 Software Request Status Bit */
    uint32_t SWRS20:1;                  /**< bit:     20  XDMAC Channel 20 Software Request Status Bit */
    uint32_t SWRS21:1;                  /**< bit:     21  XDMAC Channel 21 Software Request Status Bit */
    uint32_t SWRS22:1;                  /**< bit:     22  XDMAC Channel 22 Software Request Status Bit */
    uint32_t SWRS23:1;                  /**< bit:     23  XDMAC Channel 23 Software Request Status Bit */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t SWRS:24;                   /**< bit:  0..23  XDMAC Channel 23 Software Request Status Bit */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GSWS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GSWS_OFFSET                   (0x3C)                                        /**<  (XDMAC_GSWS) Global Channel Software Request Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GSWS_SWRS0_Pos                0                                              /**< (XDMAC_GSWS) XDMAC Channel 0 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS0_Msk                (0x1U << XDMAC_GSWS_SWRS0_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 0 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS0                    XDMAC_GSWS_SWRS0_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS0_Msk instead */
#define XDMAC_GSWS_SWRS1_Pos                1                                              /**< (XDMAC_GSWS) XDMAC Channel 1 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS1_Msk                (0x1U << XDMAC_GSWS_SWRS1_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 1 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS1                    XDMAC_GSWS_SWRS1_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS1_Msk instead */
#define XDMAC_GSWS_SWRS2_Pos                2                                              /**< (XDMAC_GSWS) XDMAC Channel 2 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS2_Msk                (0x1U << XDMAC_GSWS_SWRS2_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 2 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS2                    XDMAC_GSWS_SWRS2_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS2_Msk instead */
#define XDMAC_GSWS_SWRS3_Pos                3                                              /**< (XDMAC_GSWS) XDMAC Channel 3 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS3_Msk                (0x1U << XDMAC_GSWS_SWRS3_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 3 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS3                    XDMAC_GSWS_SWRS3_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS3_Msk instead */
#define XDMAC_GSWS_SWRS4_Pos                4                                              /**< (XDMAC_GSWS) XDMAC Channel 4 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS4_Msk                (0x1U << XDMAC_GSWS_SWRS4_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 4 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS4                    XDMAC_GSWS_SWRS4_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS4_Msk instead */
#define XDMAC_GSWS_SWRS5_Pos                5                                              /**< (XDMAC_GSWS) XDMAC Channel 5 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS5_Msk                (0x1U << XDMAC_GSWS_SWRS5_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 5 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS5                    XDMAC_GSWS_SWRS5_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS5_Msk instead */
#define XDMAC_GSWS_SWRS6_Pos                6                                              /**< (XDMAC_GSWS) XDMAC Channel 6 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS6_Msk                (0x1U << XDMAC_GSWS_SWRS6_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 6 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS6                    XDMAC_GSWS_SWRS6_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS6_Msk instead */
#define XDMAC_GSWS_SWRS7_Pos                7                                              /**< (XDMAC_GSWS) XDMAC Channel 7 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS7_Msk                (0x1U << XDMAC_GSWS_SWRS7_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 7 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS7                    XDMAC_GSWS_SWRS7_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS7_Msk instead */
#define XDMAC_GSWS_SWRS8_Pos                8                                              /**< (XDMAC_GSWS) XDMAC Channel 8 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS8_Msk                (0x1U << XDMAC_GSWS_SWRS8_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 8 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS8                    XDMAC_GSWS_SWRS8_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS8_Msk instead */
#define XDMAC_GSWS_SWRS9_Pos                9                                              /**< (XDMAC_GSWS) XDMAC Channel 9 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS9_Msk                (0x1U << XDMAC_GSWS_SWRS9_Pos)                 /**< (XDMAC_GSWS) XDMAC Channel 9 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS9                    XDMAC_GSWS_SWRS9_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS9_Msk instead */
#define XDMAC_GSWS_SWRS10_Pos               10                                             /**< (XDMAC_GSWS) XDMAC Channel 10 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS10_Msk               (0x1U << XDMAC_GSWS_SWRS10_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 10 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS10                   XDMAC_GSWS_SWRS10_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS10_Msk instead */
#define XDMAC_GSWS_SWRS11_Pos               11                                             /**< (XDMAC_GSWS) XDMAC Channel 11 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS11_Msk               (0x1U << XDMAC_GSWS_SWRS11_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 11 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS11                   XDMAC_GSWS_SWRS11_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS11_Msk instead */
#define XDMAC_GSWS_SWRS12_Pos               12                                             /**< (XDMAC_GSWS) XDMAC Channel 12 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS12_Msk               (0x1U << XDMAC_GSWS_SWRS12_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 12 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS12                   XDMAC_GSWS_SWRS12_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS12_Msk instead */
#define XDMAC_GSWS_SWRS13_Pos               13                                             /**< (XDMAC_GSWS) XDMAC Channel 13 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS13_Msk               (0x1U << XDMAC_GSWS_SWRS13_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 13 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS13                   XDMAC_GSWS_SWRS13_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS13_Msk instead */
#define XDMAC_GSWS_SWRS14_Pos               14                                             /**< (XDMAC_GSWS) XDMAC Channel 14 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS14_Msk               (0x1U << XDMAC_GSWS_SWRS14_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 14 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS14                   XDMAC_GSWS_SWRS14_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS14_Msk instead */
#define XDMAC_GSWS_SWRS15_Pos               15                                             /**< (XDMAC_GSWS) XDMAC Channel 15 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS15_Msk               (0x1U << XDMAC_GSWS_SWRS15_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 15 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS15                   XDMAC_GSWS_SWRS15_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS15_Msk instead */
#define XDMAC_GSWS_SWRS16_Pos               16                                             /**< (XDMAC_GSWS) XDMAC Channel 16 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS16_Msk               (0x1U << XDMAC_GSWS_SWRS16_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 16 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS16                   XDMAC_GSWS_SWRS16_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS16_Msk instead */
#define XDMAC_GSWS_SWRS17_Pos               17                                             /**< (XDMAC_GSWS) XDMAC Channel 17 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS17_Msk               (0x1U << XDMAC_GSWS_SWRS17_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 17 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS17                   XDMAC_GSWS_SWRS17_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS17_Msk instead */
#define XDMAC_GSWS_SWRS18_Pos               18                                             /**< (XDMAC_GSWS) XDMAC Channel 18 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS18_Msk               (0x1U << XDMAC_GSWS_SWRS18_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 18 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS18                   XDMAC_GSWS_SWRS18_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS18_Msk instead */
#define XDMAC_GSWS_SWRS19_Pos               19                                             /**< (XDMAC_GSWS) XDMAC Channel 19 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS19_Msk               (0x1U << XDMAC_GSWS_SWRS19_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 19 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS19                   XDMAC_GSWS_SWRS19_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS19_Msk instead */
#define XDMAC_GSWS_SWRS20_Pos               20                                             /**< (XDMAC_GSWS) XDMAC Channel 20 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS20_Msk               (0x1U << XDMAC_GSWS_SWRS20_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 20 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS20                   XDMAC_GSWS_SWRS20_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS20_Msk instead */
#define XDMAC_GSWS_SWRS21_Pos               21                                             /**< (XDMAC_GSWS) XDMAC Channel 21 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS21_Msk               (0x1U << XDMAC_GSWS_SWRS21_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 21 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS21                   XDMAC_GSWS_SWRS21_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS21_Msk instead */
#define XDMAC_GSWS_SWRS22_Pos               22                                             /**< (XDMAC_GSWS) XDMAC Channel 22 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS22_Msk               (0x1U << XDMAC_GSWS_SWRS22_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 22 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS22                   XDMAC_GSWS_SWRS22_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS22_Msk instead */
#define XDMAC_GSWS_SWRS23_Pos               23                                             /**< (XDMAC_GSWS) XDMAC Channel 23 Software Request Status Bit Position */
#define XDMAC_GSWS_SWRS23_Msk               (0x1U << XDMAC_GSWS_SWRS23_Pos)                /**< (XDMAC_GSWS) XDMAC Channel 23 Software Request Status Bit Mask */
#define XDMAC_GSWS_SWRS23                   XDMAC_GSWS_SWRS23_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWS_SWRS23_Msk instead */
#define XDMAC_GSWS_SWRS_Pos                 0                                              /**< (XDMAC_GSWS Position) XDMAC Channel 23 Software Request Status Bit */
#define XDMAC_GSWS_SWRS_Msk                 (0xFFFFFFU << XDMAC_GSWS_SWRS_Pos)             /**< (XDMAC_GSWS Mask) SWRS */
#define XDMAC_GSWS_SWRS(value)              (XDMAC_GSWS_SWRS_Msk & ((value) << XDMAC_GSWS_SWRS_Pos))  
#define XDMAC_GSWS_MASK                     (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GSWS) Register MASK  (Use XDMAC_GSWS_Msk instead)  */
#define XDMAC_GSWS_Msk                      (0xFFFFFFU)                                    /**< (XDMAC_GSWS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- XDMAC_GSWF : (XDMAC Offset: 0x40) (/W 32) Global Channel Software Flush Request Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SWF0:1;                    /**< bit:      0  XDMAC Channel 0 Software Flush Request Bit */
    uint32_t SWF1:1;                    /**< bit:      1  XDMAC Channel 1 Software Flush Request Bit */
    uint32_t SWF2:1;                    /**< bit:      2  XDMAC Channel 2 Software Flush Request Bit */
    uint32_t SWF3:1;                    /**< bit:      3  XDMAC Channel 3 Software Flush Request Bit */
    uint32_t SWF4:1;                    /**< bit:      4  XDMAC Channel 4 Software Flush Request Bit */
    uint32_t SWF5:1;                    /**< bit:      5  XDMAC Channel 5 Software Flush Request Bit */
    uint32_t SWF6:1;                    /**< bit:      6  XDMAC Channel 6 Software Flush Request Bit */
    uint32_t SWF7:1;                    /**< bit:      7  XDMAC Channel 7 Software Flush Request Bit */
    uint32_t SWF8:1;                    /**< bit:      8  XDMAC Channel 8 Software Flush Request Bit */
    uint32_t SWF9:1;                    /**< bit:      9  XDMAC Channel 9 Software Flush Request Bit */
    uint32_t SWF10:1;                   /**< bit:     10  XDMAC Channel 10 Software Flush Request Bit */
    uint32_t SWF11:1;                   /**< bit:     11  XDMAC Channel 11 Software Flush Request Bit */
    uint32_t SWF12:1;                   /**< bit:     12  XDMAC Channel 12 Software Flush Request Bit */
    uint32_t SWF13:1;                   /**< bit:     13  XDMAC Channel 13 Software Flush Request Bit */
    uint32_t SWF14:1;                   /**< bit:     14  XDMAC Channel 14 Software Flush Request Bit */
    uint32_t SWF15:1;                   /**< bit:     15  XDMAC Channel 15 Software Flush Request Bit */
    uint32_t SWF16:1;                   /**< bit:     16  XDMAC Channel 16 Software Flush Request Bit */
    uint32_t SWF17:1;                   /**< bit:     17  XDMAC Channel 17 Software Flush Request Bit */
    uint32_t SWF18:1;                   /**< bit:     18  XDMAC Channel 18 Software Flush Request Bit */
    uint32_t SWF19:1;                   /**< bit:     19  XDMAC Channel 19 Software Flush Request Bit */
    uint32_t SWF20:1;                   /**< bit:     20  XDMAC Channel 20 Software Flush Request Bit */
    uint32_t SWF21:1;                   /**< bit:     21  XDMAC Channel 21 Software Flush Request Bit */
    uint32_t SWF22:1;                   /**< bit:     22  XDMAC Channel 22 Software Flush Request Bit */
    uint32_t SWF23:1;                   /**< bit:     23  XDMAC Channel 23 Software Flush Request Bit */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t SWF:24;                    /**< bit:  0..23  XDMAC Channel 23 Software Flush Request Bit */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} XDMAC_GSWF_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define XDMAC_GSWF_OFFSET                   (0x40)                                        /**<  (XDMAC_GSWF) Global Channel Software Flush Request Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define XDMAC_GSWF_SWF0_Pos                 0                                              /**< (XDMAC_GSWF) XDMAC Channel 0 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF0_Msk                 (0x1U << XDMAC_GSWF_SWF0_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 0 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF0                     XDMAC_GSWF_SWF0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF0_Msk instead */
#define XDMAC_GSWF_SWF1_Pos                 1                                              /**< (XDMAC_GSWF) XDMAC Channel 1 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF1_Msk                 (0x1U << XDMAC_GSWF_SWF1_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 1 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF1                     XDMAC_GSWF_SWF1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF1_Msk instead */
#define XDMAC_GSWF_SWF2_Pos                 2                                              /**< (XDMAC_GSWF) XDMAC Channel 2 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF2_Msk                 (0x1U << XDMAC_GSWF_SWF2_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 2 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF2                     XDMAC_GSWF_SWF2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF2_Msk instead */
#define XDMAC_GSWF_SWF3_Pos                 3                                              /**< (XDMAC_GSWF) XDMAC Channel 3 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF3_Msk                 (0x1U << XDMAC_GSWF_SWF3_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 3 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF3                     XDMAC_GSWF_SWF3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF3_Msk instead */
#define XDMAC_GSWF_SWF4_Pos                 4                                              /**< (XDMAC_GSWF) XDMAC Channel 4 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF4_Msk                 (0x1U << XDMAC_GSWF_SWF4_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 4 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF4                     XDMAC_GSWF_SWF4_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF4_Msk instead */
#define XDMAC_GSWF_SWF5_Pos                 5                                              /**< (XDMAC_GSWF) XDMAC Channel 5 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF5_Msk                 (0x1U << XDMAC_GSWF_SWF5_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 5 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF5                     XDMAC_GSWF_SWF5_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF5_Msk instead */
#define XDMAC_GSWF_SWF6_Pos                 6                                              /**< (XDMAC_GSWF) XDMAC Channel 6 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF6_Msk                 (0x1U << XDMAC_GSWF_SWF6_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 6 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF6                     XDMAC_GSWF_SWF6_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF6_Msk instead */
#define XDMAC_GSWF_SWF7_Pos                 7                                              /**< (XDMAC_GSWF) XDMAC Channel 7 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF7_Msk                 (0x1U << XDMAC_GSWF_SWF7_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 7 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF7                     XDMAC_GSWF_SWF7_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF7_Msk instead */
#define XDMAC_GSWF_SWF8_Pos                 8                                              /**< (XDMAC_GSWF) XDMAC Channel 8 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF8_Msk                 (0x1U << XDMAC_GSWF_SWF8_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 8 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF8                     XDMAC_GSWF_SWF8_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF8_Msk instead */
#define XDMAC_GSWF_SWF9_Pos                 9                                              /**< (XDMAC_GSWF) XDMAC Channel 9 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF9_Msk                 (0x1U << XDMAC_GSWF_SWF9_Pos)                  /**< (XDMAC_GSWF) XDMAC Channel 9 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF9                     XDMAC_GSWF_SWF9_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF9_Msk instead */
#define XDMAC_GSWF_SWF10_Pos                10                                             /**< (XDMAC_GSWF) XDMAC Channel 10 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF10_Msk                (0x1U << XDMAC_GSWF_SWF10_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 10 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF10                    XDMAC_GSWF_SWF10_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF10_Msk instead */
#define XDMAC_GSWF_SWF11_Pos                11                                             /**< (XDMAC_GSWF) XDMAC Channel 11 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF11_Msk                (0x1U << XDMAC_GSWF_SWF11_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 11 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF11                    XDMAC_GSWF_SWF11_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF11_Msk instead */
#define XDMAC_GSWF_SWF12_Pos                12                                             /**< (XDMAC_GSWF) XDMAC Channel 12 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF12_Msk                (0x1U << XDMAC_GSWF_SWF12_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 12 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF12                    XDMAC_GSWF_SWF12_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF12_Msk instead */
#define XDMAC_GSWF_SWF13_Pos                13                                             /**< (XDMAC_GSWF) XDMAC Channel 13 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF13_Msk                (0x1U << XDMAC_GSWF_SWF13_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 13 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF13                    XDMAC_GSWF_SWF13_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF13_Msk instead */
#define XDMAC_GSWF_SWF14_Pos                14                                             /**< (XDMAC_GSWF) XDMAC Channel 14 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF14_Msk                (0x1U << XDMAC_GSWF_SWF14_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 14 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF14                    XDMAC_GSWF_SWF14_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF14_Msk instead */
#define XDMAC_GSWF_SWF15_Pos                15                                             /**< (XDMAC_GSWF) XDMAC Channel 15 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF15_Msk                (0x1U << XDMAC_GSWF_SWF15_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 15 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF15                    XDMAC_GSWF_SWF15_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF15_Msk instead */
#define XDMAC_GSWF_SWF16_Pos                16                                             /**< (XDMAC_GSWF) XDMAC Channel 16 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF16_Msk                (0x1U << XDMAC_GSWF_SWF16_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 16 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF16                    XDMAC_GSWF_SWF16_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF16_Msk instead */
#define XDMAC_GSWF_SWF17_Pos                17                                             /**< (XDMAC_GSWF) XDMAC Channel 17 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF17_Msk                (0x1U << XDMAC_GSWF_SWF17_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 17 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF17                    XDMAC_GSWF_SWF17_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF17_Msk instead */
#define XDMAC_GSWF_SWF18_Pos                18                                             /**< (XDMAC_GSWF) XDMAC Channel 18 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF18_Msk                (0x1U << XDMAC_GSWF_SWF18_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 18 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF18                    XDMAC_GSWF_SWF18_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF18_Msk instead */
#define XDMAC_GSWF_SWF19_Pos                19                                             /**< (XDMAC_GSWF) XDMAC Channel 19 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF19_Msk                (0x1U << XDMAC_GSWF_SWF19_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 19 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF19                    XDMAC_GSWF_SWF19_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF19_Msk instead */
#define XDMAC_GSWF_SWF20_Pos                20                                             /**< (XDMAC_GSWF) XDMAC Channel 20 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF20_Msk                (0x1U << XDMAC_GSWF_SWF20_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 20 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF20                    XDMAC_GSWF_SWF20_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF20_Msk instead */
#define XDMAC_GSWF_SWF21_Pos                21                                             /**< (XDMAC_GSWF) XDMAC Channel 21 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF21_Msk                (0x1U << XDMAC_GSWF_SWF21_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 21 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF21                    XDMAC_GSWF_SWF21_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF21_Msk instead */
#define XDMAC_GSWF_SWF22_Pos                22                                             /**< (XDMAC_GSWF) XDMAC Channel 22 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF22_Msk                (0x1U << XDMAC_GSWF_SWF22_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 22 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF22                    XDMAC_GSWF_SWF22_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF22_Msk instead */
#define XDMAC_GSWF_SWF23_Pos                23                                             /**< (XDMAC_GSWF) XDMAC Channel 23 Software Flush Request Bit Position */
#define XDMAC_GSWF_SWF23_Msk                (0x1U << XDMAC_GSWF_SWF23_Pos)                 /**< (XDMAC_GSWF) XDMAC Channel 23 Software Flush Request Bit Mask */
#define XDMAC_GSWF_SWF23                    XDMAC_GSWF_SWF23_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use XDMAC_GSWF_SWF23_Msk instead */
#define XDMAC_GSWF_SWF_Pos                  0                                              /**< (XDMAC_GSWF Position) XDMAC Channel 23 Software Flush Request Bit */
#define XDMAC_GSWF_SWF_Msk                  (0xFFFFFFU << XDMAC_GSWF_SWF_Pos)              /**< (XDMAC_GSWF Mask) SWF */
#define XDMAC_GSWF_SWF(value)               (XDMAC_GSWF_SWF_Msk & ((value) << XDMAC_GSWF_SWF_Pos))  
#define XDMAC_GSWF_MASK                     (0xFFFFFFU)                                    /**< \deprecated (XDMAC_GSWF) Register MASK  (Use XDMAC_GSWF_Msk instead)  */
#define XDMAC_GSWF_Msk                      (0xFFFFFFU)                                    /**< (XDMAC_GSWF) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
#define XDMACCHID_NUMBER 24
/** \brief XDMAC_CHID hardware registers */
typedef struct {  
  __O  uint32_t XDMAC_CIE;      /**< (XDMAC_CHID Offset: 0x00) Channel Interrupt Enable Register (chid = 0) */
  __O  uint32_t XDMAC_CID;      /**< (XDMAC_CHID Offset: 0x04) Channel Interrupt Disable Register (chid = 0) */
  __O  uint32_t XDMAC_CIM;      /**< (XDMAC_CHID Offset: 0x08) Channel Interrupt Mask Register (chid = 0) */
  __I  uint32_t XDMAC_CIS;      /**< (XDMAC_CHID Offset: 0x0C) Channel Interrupt Status Register (chid = 0) */
  __IO uint32_t XDMAC_CSA;      /**< (XDMAC_CHID Offset: 0x10) Channel Source Address Register (chid = 0) */
  __IO uint32_t XDMAC_CDA;      /**< (XDMAC_CHID Offset: 0x14) Channel Destination Address Register (chid = 0) */
  __IO uint32_t XDMAC_CNDA;     /**< (XDMAC_CHID Offset: 0x18) Channel Next Descriptor Address Register (chid = 0) */
  __IO uint32_t XDMAC_CNDC;     /**< (XDMAC_CHID Offset: 0x1C) Channel Next Descriptor Control Register (chid = 0) */
  __IO uint32_t XDMAC_CUBC;     /**< (XDMAC_CHID Offset: 0x20) Channel Microblock Control Register (chid = 0) */
  __IO uint32_t XDMAC_CBC;      /**< (XDMAC_CHID Offset: 0x24) Channel Block Control Register (chid = 0) */
  __IO uint32_t XDMAC_CC;       /**< (XDMAC_CHID Offset: 0x28) Channel Configuration Register (chid = 0) */
  __IO uint32_t XDMAC_CDS_MSP;  /**< (XDMAC_CHID Offset: 0x2C) Channel Data Stride Memory Set Pattern (chid = 0) */
  __IO uint32_t XDMAC_CSUS;     /**< (XDMAC_CHID Offset: 0x30) Channel Source Microblock Stride (chid = 0) */
  __IO uint32_t XDMAC_CDUS;     /**< (XDMAC_CHID Offset: 0x34) Channel Destination Microblock Stride (chid = 0) */
       RoReg8                         Reserved1[0x08];
} XdmacChid;

/** \brief XDMAC hardware registers */
typedef struct {  
  __IO uint32_t XDMAC_GTYPE;    /**< (XDMAC Offset: 0x00) Global Type Register */
  __I  uint32_t XDMAC_GCFG;     /**< (XDMAC Offset: 0x04) Global Configuration Register */
  __IO uint32_t XDMAC_GWAC;     /**< (XDMAC Offset: 0x08) Global Weighted Arbiter Configuration Register */
  __O  uint32_t XDMAC_GIE;      /**< (XDMAC Offset: 0x0C) Global Interrupt Enable Register */
  __O  uint32_t XDMAC_GID;      /**< (XDMAC Offset: 0x10) Global Interrupt Disable Register */
  __I  uint32_t XDMAC_GIM;      /**< (XDMAC Offset: 0x14) Global Interrupt Mask Register */
  __I  uint32_t XDMAC_GIS;      /**< (XDMAC Offset: 0x18) Global Interrupt Status Register */
  __O  uint32_t XDMAC_GE;       /**< (XDMAC Offset: 0x1C) Global Channel Enable Register */
  __O  uint32_t XDMAC_GD;       /**< (XDMAC Offset: 0x20) Global Channel Disable Register */
  __I  uint32_t XDMAC_GS;       /**< (XDMAC Offset: 0x24) Global Channel Status Register */
  __IO uint32_t XDMAC_GRS;      /**< (XDMAC Offset: 0x28) Global Channel Read Suspend Register */
  __IO uint32_t XDMAC_GWS;      /**< (XDMAC Offset: 0x2C) Global Channel Write Suspend Register */
  __O  uint32_t XDMAC_GRWS;     /**< (XDMAC Offset: 0x30) Global Channel Read Write Suspend Register */
  __O  uint32_t XDMAC_GRWR;     /**< (XDMAC Offset: 0x34) Global Channel Read Write Resume Register */
  __O  uint32_t XDMAC_GSWR;     /**< (XDMAC Offset: 0x38) Global Channel Software Request Register */
  __I  uint32_t XDMAC_GSWS;     /**< (XDMAC Offset: 0x3C) Global Channel Software Request Status Register */
  __O  uint32_t XDMAC_GSWF;     /**< (XDMAC Offset: 0x40) Global Channel Software Flush Request Register */
  __I  uint32_t Reserved1[3];
       XdmacChid XDMAC_CHID[XDMACCHID_NUMBER]; /**< Offset: 0x50 Channel Interrupt Enable Register (chid = 0) */
} Xdmac;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief XDMAC_CHID hardware registers */
typedef struct {  
  __O  XDMAC_CIE_Type                 XDMAC_CIE;      /**< Offset: 0x00 ( /W  32) Channel Interrupt Enable Register (chid = 0) */
  __O  XDMAC_CID_Type                 XDMAC_CID;      /**< Offset: 0x04 ( /W  32) Channel Interrupt Disable Register (chid = 0) */
  __O  XDMAC_CIM_Type                 XDMAC_CIM;      /**< Offset: 0x08 ( /W  32) Channel Interrupt Mask Register (chid = 0) */
  __I  XDMAC_CIS_Type                 XDMAC_CIS;      /**< Offset: 0x0C (R/   32) Channel Interrupt Status Register (chid = 0) */
  __IO XDMAC_CSA_Type                 XDMAC_CSA;      /**< Offset: 0x10 (R/W  32) Channel Source Address Register (chid = 0) */
  __IO XDMAC_CDA_Type                 XDMAC_CDA;      /**< Offset: 0x14 (R/W  32) Channel Destination Address Register (chid = 0) */
  __IO XDMAC_CNDA_Type                XDMAC_CNDA;     /**< Offset: 0x18 (R/W  32) Channel Next Descriptor Address Register (chid = 0) */
  __IO XDMAC_CNDC_Type                XDMAC_CNDC;     /**< Offset: 0x1C (R/W  32) Channel Next Descriptor Control Register (chid = 0) */
  __IO XDMAC_CUBC_Type                XDMAC_CUBC;     /**< Offset: 0x20 (R/W  32) Channel Microblock Control Register (chid = 0) */
  __IO XDMAC_CBC_Type                 XDMAC_CBC;      /**< Offset: 0x24 (R/W  32) Channel Block Control Register (chid = 0) */
  __IO XDMAC_CC_Type                  XDMAC_CC;       /**< Offset: 0x28 (R/W  32) Channel Configuration Register (chid = 0) */
  __IO XDMAC_CDS_MSP_Type             XDMAC_CDS_MSP;  /**< Offset: 0x2C (R/W  32) Channel Data Stride Memory Set Pattern (chid = 0) */
  __IO XDMAC_CSUS_Type                XDMAC_CSUS;     /**< Offset: 0x30 (R/W  32) Channel Source Microblock Stride (chid = 0) */
  __IO XDMAC_CDUS_Type                XDMAC_CDUS;     /**< Offset: 0x34 (R/W  32) Channel Destination Microblock Stride (chid = 0) */
       RoReg8                         Reserved1[0x08];
} XdmacChid;

/** \brief XDMAC hardware registers */
typedef struct {  
  __IO XDMAC_GTYPE_Type               XDMAC_GTYPE;    /**< Offset: 0x00 (R/W  32) Global Type Register */
  __I  XDMAC_GCFG_Type                XDMAC_GCFG;     /**< Offset: 0x04 (R/   32) Global Configuration Register */
  __IO XDMAC_GWAC_Type                XDMAC_GWAC;     /**< Offset: 0x08 (R/W  32) Global Weighted Arbiter Configuration Register */
  __O  XDMAC_GIE_Type                 XDMAC_GIE;      /**< Offset: 0x0C ( /W  32) Global Interrupt Enable Register */
  __O  XDMAC_GID_Type                 XDMAC_GID;      /**< Offset: 0x10 ( /W  32) Global Interrupt Disable Register */
  __I  XDMAC_GIM_Type                 XDMAC_GIM;      /**< Offset: 0x14 (R/   32) Global Interrupt Mask Register */
  __I  XDMAC_GIS_Type                 XDMAC_GIS;      /**< Offset: 0x18 (R/   32) Global Interrupt Status Register */
  __O  XDMAC_GE_Type                  XDMAC_GE;       /**< Offset: 0x1C ( /W  32) Global Channel Enable Register */
  __O  XDMAC_GD_Type                  XDMAC_GD;       /**< Offset: 0x20 ( /W  32) Global Channel Disable Register */
  __I  XDMAC_GS_Type                  XDMAC_GS;       /**< Offset: 0x24 (R/   32) Global Channel Status Register */
  __IO XDMAC_GRS_Type                 XDMAC_GRS;      /**< Offset: 0x28 (R/W  32) Global Channel Read Suspend Register */
  __IO XDMAC_GWS_Type                 XDMAC_GWS;      /**< Offset: 0x2C (R/W  32) Global Channel Write Suspend Register */
  __O  XDMAC_GRWS_Type                XDMAC_GRWS;     /**< Offset: 0x30 ( /W  32) Global Channel Read Write Suspend Register */
  __O  XDMAC_GRWR_Type                XDMAC_GRWR;     /**< Offset: 0x34 ( /W  32) Global Channel Read Write Resume Register */
  __O  XDMAC_GSWR_Type                XDMAC_GSWR;     /**< Offset: 0x38 ( /W  32) Global Channel Software Request Register */
  __I  XDMAC_GSWS_Type                XDMAC_GSWS;     /**< Offset: 0x3C (R/   32) Global Channel Software Request Status Register */
  __O  XDMAC_GSWF_Type                XDMAC_GSWF;     /**< Offset: 0x40 ( /W  32) Global Channel Software Flush Request Register */
       RoReg8                         Reserved1[0x0C];
       XdmacChid                      XDMAC_CHID[24]; /**< Offset: 0x50 Channel Interrupt Enable Register (chid = 0) */
} Xdmac;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Extensible DMA Controller */

#endif /* _SAME70_XDMAC_COMPONENT_H_ */
