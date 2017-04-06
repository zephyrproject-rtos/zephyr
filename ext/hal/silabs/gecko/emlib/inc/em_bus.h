/***************************************************************************//**
 * @file em_bus.h
 * @brief RAM and peripheral bit-field set and clear API
 * @version 5.1.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#ifndef EM_BUS_H
#define EM_BUS_H

#include "em_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup BUS
 * @brief BUS register and RAM bit/field read/write API
 * @details
 *  API to perform bit-band and field set/clear access to RAM and peripherals.
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Perform a single-bit write operation on a 32-bit word in RAM
 *
 * @details
 *   This function uses Cortex-M bit-banding hardware to perform an atomic
 *   read-modify-write operation on a single bit write on a 32-bit word in RAM.
 *   Please refer to the reference manual for further details about bit-banding.
 *
 * @note
 *   This function is atomic on Cortex-M cores with bit-banding support. Bit-
 *   banding is a multicycle read-modify-write bus operation. RAM bit-banding is
 *   performed using the memory alias region at BITBAND_RAM_BASE.
 *
 * @param[in] addr Address of 32-bit word in RAM
 *
 * @param[in] bit Bit position to write, 0-31
 *
 * @param[in] val Value to set bit to, 0 or 1
 ******************************************************************************/
__STATIC_INLINE void BUS_RamBitWrite(volatile uint32_t *addr,
                                     unsigned int bit,
                                     unsigned int val)
{
#if defined( BITBAND_RAM_BASE )
  uint32_t aliasAddr =
    BITBAND_RAM_BASE + (((uint32_t)addr - SRAM_BASE) * 32) + (bit * 4);

  *(volatile uint32_t *)aliasAddr = (uint32_t)val;
#else
  uint32_t tmp = *addr;

  /* Make sure val is not more than 1, because we only want to set one bit. */
  *addr = (tmp & ~(1 << bit)) | ((val & 1) << bit);
#endif
}


/***************************************************************************//**
 * @brief
 *   Perform a single-bit read operation on a 32-bit word in RAM
 *
 * @details
 *   This function uses Cortex-M bit-banding hardware to perform an atomic
 *   read operation on a single register bit. Please refer to the
 *   reference manual for further details about bit-banding.
 *
 * @note
 *   This function is atomic on Cortex-M cores with bit-banding support.
 *   RAM bit-banding is performed using the memory alias region
 *   at BITBAND_RAM_BASE.
 *
 * @param[in] addr RAM address
 *
 * @param[in] bit Bit position to read, 0-31
 *
 * @return
 *     The requested bit shifted to bit position 0 in the return value
 ******************************************************************************/
__STATIC_INLINE unsigned int BUS_RamBitRead(volatile const uint32_t *addr,
                                            unsigned int bit)
{
#if defined( BITBAND_RAM_BASE )
  uint32_t aliasAddr =
    BITBAND_RAM_BASE + (((uint32_t)addr - SRAM_BASE) * 32) + (bit * 4);

  return *(volatile uint32_t *)aliasAddr;
#else
  return ((*addr) >> bit) & 1;
#endif
}


/***************************************************************************//**
 * @brief
 *   Perform a single-bit write operation on a peripheral register
 *
 * @details
 *   This function uses Cortex-M bit-banding hardware to perform an atomic
 *   read-modify-write operation on a single register bit. Please refer to the
 *   reference manual for further details about bit-banding.
 *
 * @note
 *   This function is atomic on Cortex-M cores with bit-banding support. Bit-
 *   banding is a multicycle read-modify-write bus operation. Peripheral register
 *   bit-banding is performed using the memory alias region at BITBAND_PER_BASE.
 *
 * @param[in] addr Peripheral register address
 *
 * @param[in] bit Bit position to write, 0-31
 *
 * @param[in] val Value to set bit to, 0 or 1
 ******************************************************************************/
__STATIC_INLINE void BUS_RegBitWrite(volatile uint32_t *addr,
                                     unsigned int bit,
                                     unsigned int val)
{
#if defined( BITBAND_PER_BASE )
  uint32_t aliasAddr =
    BITBAND_PER_BASE + (((uint32_t)addr - PER_MEM_BASE) * 32) + (bit * 4);

  *(volatile uint32_t *)aliasAddr = (uint32_t)val;
#else
  uint32_t tmp = *addr;

  /* Make sure val is not more than 1, because we only want to set one bit. */
  *addr = (tmp & ~(1 << bit)) | ((val & 1) << bit);
#endif
}


/***************************************************************************//**
 * @brief
 *   Perform a single-bit read operation on a peripheral register
 *
 * @details
 *   This function uses Cortex-M bit-banding hardware to perform an atomic
 *   read operation on a single register bit. Please refer to the
 *   reference manual for further details about bit-banding.
 *
 * @note
 *   This function is atomic on Cortex-M cores with bit-banding support.
 *   Peripheral register bit-banding is performed using the memory alias
 *   region at BITBAND_PER_BASE.
 *
 * @param[in] addr Peripheral register address
 *
 * @param[in] bit Bit position to read, 0-31
 *
 * @return
 *     The requested bit shifted to bit position 0 in the return value
 ******************************************************************************/
__STATIC_INLINE unsigned int BUS_RegBitRead(volatile const uint32_t *addr,
                                            unsigned int bit)
{
#if defined( BITBAND_PER_BASE )
  uint32_t aliasAddr =
    BITBAND_PER_BASE + (((uint32_t)addr - PER_MEM_BASE) * 32) + (bit * 4);

  return *(volatile uint32_t *)aliasAddr;
#else
  return ((*addr) >> bit) & 1;
#endif
}


/***************************************************************************//**
 * @brief
 *   Perform a masked set operation on peripheral register address.
 *
 * @details
 *   Peripheral register masked set provides a single-cycle and atomic set
 *   operation of a bit-mask in a peripheral register. All 1's in the mask are
 *   set to 1 in the register. All 0's in the mask are not changed in the
 *   register.
 *   RAMs and special peripherals are not supported. Please refer to the
 *   reference manual for further details about peripheral register field set.
 *
 * @note
 *   This function is single-cycle and atomic on cores with peripheral bit set
 *   and clear support. It uses the memory alias region at PER_BITSET_MEM_BASE.
 *
 * @param[in] addr Peripheral register address
 *
 * @param[in] mask Mask to set
 ******************************************************************************/
__STATIC_INLINE void BUS_RegMaskedSet(volatile uint32_t *addr,
                                      uint32_t mask)
{
#if defined( PER_BITSET_MEM_BASE )
  uint32_t aliasAddr = PER_BITSET_MEM_BASE + ((uint32_t)addr - PER_MEM_BASE);
  *(volatile uint32_t *)aliasAddr = mask;
#else
  *addr |= mask;
#endif
}


/***************************************************************************//**
 * @brief
 *   Perform a masked clear operation on peripheral register address.
 *
 * @details
 *   Peripheral register masked clear provides a single-cycle and atomic clear
 *   operation of a bit-mask in a peripheral register. All 1's in the mask are
 *   set to 0 in the register.
 *   All 0's in the mask are not changed in the register.
 *   RAMs and special peripherals are not supported. Please refer to the
 *   reference manual for further details about peripheral register field clear.
 *
 * @note
 *   This function is single-cycle and atomic on cores with peripheral bit set
 *   and clear support. It uses the memory alias region at PER_BITCLR_MEM_BASE.
 *
 * @param[in] addr Peripheral register address
 *
 * @param[in] mask Mask to clear
 ******************************************************************************/
__STATIC_INLINE void BUS_RegMaskedClear(volatile uint32_t *addr,
                                        uint32_t mask)
{
#if defined( PER_BITCLR_MEM_BASE )
  uint32_t aliasAddr = PER_BITCLR_MEM_BASE + ((uint32_t)addr - PER_MEM_BASE);
  *(volatile uint32_t *)aliasAddr = mask;
#else
  *addr &= ~mask;
#endif
}


/***************************************************************************//**
 * @brief
 *   Perform peripheral register masked clear and value write.
 *
 * @details
 *   This function first clears the mask in the peripheral register, then
 *   writes the value. Typically the mask is a bit-field in the register, and
 *   the value val is within the mask.
 *
 * @note
 *   This operation is not atomic. Note that the mask is first set to 0 before
 *   the val is set.
 *
 * @param[in] addr Peripheral register address
 *
 * @param[in] mask Peripheral register mask
 *
 * @param[in] val Peripheral register value. The value must be shifted to the
                  correct bit position in the register corresponding to the field
                  defined by the mask parameter. The register value must be
                  contained in the field defined by the mask parameter. This
                  function is not performing masking of val internally.
 ******************************************************************************/
__STATIC_INLINE void BUS_RegMaskedWrite(volatile uint32_t *addr,
                                        uint32_t mask,
                                        uint32_t val)
{
#if defined( PER_BITCLR_MEM_BASE )
  BUS_RegMaskedClear(addr, mask);
  BUS_RegMaskedSet(addr, val);
#else
  *addr = (*addr & ~mask) | val;
#endif
}


/***************************************************************************//**
 * @brief
 *   Perform a peripheral register masked read
 *
 * @details
 *   Read an unshifted and masked value from a peripheral register.
 *
 * @note
 *   This operation is not hardware accelerated.
 *
 * @param[in] addr Peripheral register address
 *
 * @param[in] mask Peripheral register mask
 *
 * @return
 *   Unshifted and masked register value
 ******************************************************************************/
__STATIC_INLINE uint32_t BUS_RegMaskedRead(volatile const uint32_t *addr,
                                           uint32_t mask)
{
  return *addr & mask;
}


/** @} (end addtogroup BUS) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* EM_BUS_H */
