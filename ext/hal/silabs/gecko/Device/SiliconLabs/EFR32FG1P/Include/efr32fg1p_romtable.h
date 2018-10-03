/**************************************************************************//**
 * @file efr32fg1p_romtable.h
 * @brief EFR32FG1P_ROMTABLE register and bit field definitions
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
 * @defgroup EFR32FG1P_ROMTABLE
 * @{
 * @brief Chip Information, Revision numbers
 *****************************************************************************/
typedef struct
{
  __IM uint32_t PID4; /**< JEP_106_BANK */
  __IM uint32_t PID5; /**< Unused */
  __IM uint32_t PID6; /**< Unused */
  __IM uint32_t PID7; /**< Unused */
  __IM uint32_t PID0; /**< Chip family LSB, chip major revision */
  __IM uint32_t PID1; /**< JEP_106_NO, Chip family MSB */
  __IM uint32_t PID2; /**< Chip minor rev MSB, JEP_106_PRESENT, JEP_106_NO */
  __IM uint32_t PID3; /**< Chip minor rev LSB */
  __IM uint32_t CID0; /**< Unused */
} ROMTABLE_TypeDef;   /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_ROMTABLE_BitFields
 * @{
 *****************************************************************************/
/* Bit fields for EFR32FG1P_ROMTABLE */
#define _ROMTABLE_PID0_FAMILYLSB_MASK       0x000000C0UL /**< Least Significant Bits [1:0] of CHIP FAMILY, mask */
#define _ROMTABLE_PID0_FAMILYLSB_SHIFT      6            /**< Least Significant Bits [1:0] of CHIP FAMILY, shift */
#define _ROMTABLE_PID0_REVMAJOR_MASK        0x0000003FUL /**< CHIP MAJOR Revison, mask */
#define _ROMTABLE_PID0_REVMAJOR_SHIFT       0            /**< CHIP MAJOR Revison, shift */
#define _ROMTABLE_PID1_FAMILYMSB_MASK       0x0000000FUL /**< Most Significant Bits [5:2] of CHIP FAMILY, mask */
#define _ROMTABLE_PID1_FAMILYMSB_SHIFT      0            /**< Most Significant Bits [5:2] of CHIP FAMILY, shift */
#define _ROMTABLE_PID2_REVMINORMSB_MASK     0x000000F0UL /**< Most Significant Bits [7:4] of CHIP MINOR revision, mask */
#define _ROMTABLE_PID2_REVMINORMSB_SHIFT    4            /**< Most Significant Bits [7:4] of CHIP MINOR revision, mask */
#define _ROMTABLE_PID3_REVMINORLSB_MASK     0x000000F0UL /**< Least Significant Bits [3:0] of CHIP MINOR revision, mask */
#define _ROMTABLE_PID3_REVMINORLSB_SHIFT    4            /**< Least Significant Bits [3:0] of CHIP MINOR revision, shift */

/** @} End of group EFR32FG1P_ROMTABLE */
/** @} End of group Parts */

