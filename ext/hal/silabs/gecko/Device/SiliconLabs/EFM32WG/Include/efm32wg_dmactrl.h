/**************************************************************************//**
 * @file efm32wg_dmactrl.h
 * @brief EFM32WG_DMACTRL register and bit field definitions
 * @version 5.1.2
 ******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/
/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/

/**************************************************************************//**
 * @defgroup EFM32WG_DMACTRL_BitFields
 * @{
 *****************************************************************************/
#define _DMA_CTRL_DST_INC_MASK                         0xC0000000UL  /**< Data increment for destination, bit mask */
#define _DMA_CTRL_DST_INC_SHIFT                        30            /**< Data increment for destination, shift value */
#define _DMA_CTRL_DST_INC_BYTE                         0x00          /**< Byte/8-bit increment */
#define _DMA_CTRL_DST_INC_HALFWORD                     0x01          /**< Half word/16-bit increment */
#define _DMA_CTRL_DST_INC_WORD                         0x02          /**< Word/32-bit increment */
#define _DMA_CTRL_DST_INC_NONE                         0x03          /**< No increment */
#define DMA_CTRL_DST_INC_BYTE                          0x00000000UL  /**< Byte/8-bit increment */
#define DMA_CTRL_DST_INC_HALFWORD                      0x40000000UL  /**< Half word/16-bit increment */
#define DMA_CTRL_DST_INC_WORD                          0x80000000UL  /**< Word/32-bit increment */
#define DMA_CTRL_DST_INC_NONE                          0xC0000000UL  /**< No increment */
#define _DMA_CTRL_DST_SIZE_MASK                        0x30000000UL  /**< Data size for destination - MUST be the same as source, bit mask */
#define _DMA_CTRL_DST_SIZE_SHIFT                       28            /**< Data size for destination - MUST be the same as source, shift value */
#define _DMA_CTRL_DST_SIZE_BYTE                        0x00          /**< Byte/8-bit data size */
#define _DMA_CTRL_DST_SIZE_HALFWORD                    0x01          /**< Half word/16-bit data size */
#define _DMA_CTRL_DST_SIZE_WORD                        0x02          /**< Word/32-bit data size */
#define _DMA_CTRL_DST_SIZE_RSVD                        0x03          /**< Reserved */
#define DMA_CTRL_DST_SIZE_BYTE                         0x00000000UL  /**< Byte/8-bit data size */
#define DMA_CTRL_DST_SIZE_HALFWORD                     0x10000000UL  /**< Half word/16-bit data size */
#define DMA_CTRL_DST_SIZE_WORD                         0x20000000UL  /**< Word/32-bit data size */
#define DMA_CTRL_DST_SIZE_RSVD                         0x30000000UL  /**< Reserved - do not use */
#define _DMA_CTRL_SRC_INC_MASK                         0x0C000000UL  /**< Data increment for source, bit mask */
#define _DMA_CTRL_SRC_INC_SHIFT                        26            /**< Data increment for source, shift value */
#define _DMA_CTRL_SRC_INC_BYTE                         0x00          /**< Byte/8-bit increment */
#define _DMA_CTRL_SRC_INC_HALFWORD                     0x01          /**< Half word/16-bit increment */
#define _DMA_CTRL_SRC_INC_WORD                         0x02          /**< Word/32-bit increment */
#define _DMA_CTRL_SRC_INC_NONE                         0x03          /**< No increment */
#define DMA_CTRL_SRC_INC_BYTE                          0x00000000UL  /**< Byte/8-bit increment */
#define DMA_CTRL_SRC_INC_HALFWORD                      0x04000000UL  /**< Half word/16-bit increment */
#define DMA_CTRL_SRC_INC_WORD                          0x08000000UL  /**< Word/32-bit increment */
#define DMA_CTRL_SRC_INC_NONE                          0x0C000000UL  /**< No increment */
#define _DMA_CTRL_SRC_SIZE_MASK                        0x03000000UL  /**< Data size for source - MUST be the same as destination, bit mask */
#define _DMA_CTRL_SRC_SIZE_SHIFT                       24            /**< Data size for source - MUST be the same as destination, shift value */
#define _DMA_CTRL_SRC_SIZE_BYTE                        0x00          /**< Byte/8-bit data size */
#define _DMA_CTRL_SRC_SIZE_HALFWORD                    0x01          /**< Half word/16-bit data size */
#define _DMA_CTRL_SRC_SIZE_WORD                        0x02          /**< Word/32-bit data size */
#define _DMA_CTRL_SRC_SIZE_RSVD                        0x03          /**< Reserved */
#define DMA_CTRL_SRC_SIZE_BYTE                         0x00000000UL  /**< Byte/8-bit data size */
#define DMA_CTRL_SRC_SIZE_HALFWORD                     0x01000000UL  /**< Half word/16-bit data size */
#define DMA_CTRL_SRC_SIZE_WORD                         0x02000000UL  /**< Word/32-bit data size */
#define DMA_CTRL_SRC_SIZE_RSVD                         0x03000000UL  /**< Reserved - do not use */
#define _DMA_CTRL_DST_PROT_CTRL_MASK                   0x00E00000UL  /**< Protection flag for destination, bit mask */
#define _DMA_CTRL_DST_PROT_CTRL_SHIFT                  21            /**< Protection flag for destination, shift value */
#define DMA_CTRL_DST_PROT_PRIVILEGED                   0x00200000UL  /**< Privileged mode for destination */
#define DMA_CTRL_DST_PROT_NON_PRIVILEGED               0x00000000UL  /**< Non-privileged mode for destination */
#define _DMA_CTRL_SRC_PROT_CTRL_MASK                   0x001C0000UL  /**< Protection flag for source, bit mask */
#define _DMA_CTRL_SRC_PROT_CTRL_SHIFT                  18            /**< Protection flag for source, shift value */
#define DMA_CTRL_SRC_PROT_PRIVILEGED                   0x00040000UL  /**< Privileged mode for destination */
#define DMA_CTRL_SRC_PROT_NON_PRIVILEGED               0x00000000UL  /**< Non-privileged mode for destination */
#define _DMA_CTRL_PROT_NON_PRIVILEGED                  0x00          /**< Protection bits to indicate non-privileged access */
#define _DMA_CTRL_PROT_PRIVILEGED                      0x01          /**< Protection bits to indicate privileged access */
#define _DMA_CTRL_R_POWER_MASK                         0x0003C000UL  /**< DMA arbitration mask */
#define _DMA_CTRL_R_POWER_SHIFT                        14            /**< Number of DMA cycles before controller does new arbitration in 2^R */
#define _DMA_CTRL_R_POWER_1                            0x00          /**< Arbitrate after each transfer */
#define _DMA_CTRL_R_POWER_2                            0x01          /**< Arbitrate after every 2 transfers */
#define _DMA_CTRL_R_POWER_4                            0x02          /**< Arbitrate after every 4 transfers */
#define _DMA_CTRL_R_POWER_8                            0x03          /**< Arbitrate after every 8 transfers */
#define _DMA_CTRL_R_POWER_16                           0x04          /**< Arbitrate after every 16 transfers */
#define _DMA_CTRL_R_POWER_32                           0x05          /**< Arbitrate after every 32 transfers */
#define _DMA_CTRL_R_POWER_64                           0x06          /**< Arbitrate after every 64 transfers */
#define _DMA_CTRL_R_POWER_128                          0x07          /**< Arbitrate after every 128 transfers */
#define _DMA_CTRL_R_POWER_256                          0x08          /**< Arbitrate after every 256 transfers */
#define _DMA_CTRL_R_POWER_512                          0x09          /**< Arbitrate after every 512 transfers */
#define _DMA_CTRL_R_POWER_1024                         0x0a          /**< Arbitrate after every 1024 transfers */
#define DMA_CTRL_R_POWER_1                             0x00000000UL  /**< Arbitrate after each transfer */
#define DMA_CTRL_R_POWER_2                             0x00004000UL  /**< Arbitrate after every 2 transfers */
#define DMA_CTRL_R_POWER_4                             0x00008000UL  /**< Arbitrate after every 4 transfers */
#define DMA_CTRL_R_POWER_8                             0x0000c000UL  /**< Arbitrate after every 8 transfers */
#define DMA_CTRL_R_POWER_16                            0x00010000UL  /**< Arbitrate after every 16 transfers */
#define DMA_CTRL_R_POWER_32                            0x00014000UL  /**< Arbitrate after every 32 transfers */
#define DMA_CTRL_R_POWER_64                            0x00018000UL  /**< Arbitrate after every 64 transfers */
#define DMA_CTRL_R_POWER_128                           0x0001c000UL  /**< Arbitrate after every 128 transfers */
#define DMA_CTRL_R_POWER_256                           0x00020000UL  /**< Arbitrate after every 256 transfers */
#define DMA_CTRL_R_POWER_512                           0x00024000UL  /**< Arbitrate after every 512 transfers */
#define DMA_CTRL_R_POWER_1024                          0x00028000UL  /**< Arbitrate after every 1024 transfers */
#define _DMA_CTRL_N_MINUS_1_MASK                       0x00003FF0UL  /**< Number of DMA transfers minus 1, bit mask. See PL230 documentation */
#define _DMA_CTRL_N_MINUS_1_SHIFT                      4             /**< Number of DMA transfers minus 1, shift mask. See PL230 documentation */
#define _DMA_CTRL_NEXT_USEBURST_MASK                   0x00000008UL  /**< DMA useburst_set[C] is 1 when using scatter-gather DMA and using alternate data */
#define _DMA_CTRL_NEXT_USEBURST_SHIFT                  3             /**< DMA useburst shift */
#define _DMA_CTRL_CYCLE_CTRL_MASK                      0x00000007UL  /**< DMA Cycle control bit mask - basic/auto/ping-poing/scath-gath */
#define _DMA_CTRL_CYCLE_CTRL_SHIFT                     0             /**< DMA Cycle control bit shift */
#define _DMA_CTRL_CYCLE_CTRL_INVALID                   0x00          /**< Invalid cycle type  */
#define _DMA_CTRL_CYCLE_CTRL_BASIC                     0x01          /**< Basic cycle type */
#define _DMA_CTRL_CYCLE_CTRL_AUTO                      0x02          /**< Auto cycle type */
#define _DMA_CTRL_CYCLE_CTRL_PINGPONG                  0x03          /**< PingPong cycle type */
#define _DMA_CTRL_CYCLE_CTRL_MEM_SCATTER_GATHER        0x04          /**< Memory scatter gather cycle type */
#define _DMA_CTRL_CYCLE_CTRL_MEM_SCATTER_GATHER_ALT    0x05          /**< Memory scatter gather using alternate structure  */
#define _DMA_CTRL_CYCLE_CTRL_PER_SCATTER_GATHER        0x06          /**< Peripheral scatter gather cycle type */
#define _DMA_CTRL_CYCLE_CTRL_PER_SCATTER_GATHER_ALT    0x07          /**< Peripheral scatter gather cycle type using alternate structure */
#define DMA_CTRL_CYCLE_CTRL_INVALID                    0x00000000UL  /**< Invalid cycle type */
#define DMA_CTRL_CYCLE_CTRL_BASIC                      0x00000001UL  /**< Basic cycle type */
#define DMA_CTRL_CYCLE_CTRL_AUTO                       0x00000002UL  /**< Auto cycle type */
#define DMA_CTRL_CYCLE_CTRL_PINGPONG                   0x00000003UL  /**< PingPong cycle type */
#define DMA_CTRL_CYCLE_CTRL_MEM_SCATTER_GATHER         0x000000004UL /**< Memory scatter gather cycle type */
#define DMA_CTRL_CYCLE_CTRL_MEM_SCATTER_GATHER_ALT     0x000000005UL /**< Memory scatter gather using alternate structure  */
#define DMA_CTRL_CYCLE_CTRL_PER_SCATTER_GATHER         0x000000006UL /**< Peripheral scatter gather cycle type */
#define DMA_CTRL_CYCLE_CTRL_PER_SCATTER_GATHER_ALT     0x000000007UL /**< Peripheral scatter gather cycle type using alternate structure */

/** @} End of group EFM32WG_DMA */
/** @} End of group Parts */

