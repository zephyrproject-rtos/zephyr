/***************************************************************************//**
 * @file
 * @brief General Purpose Cyclic Redundancy Check (GPCRC) API.
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

#ifndef EM_GPCRC_H
#define EM_GPCRC_H

#include "em_bus.h"
#include "em_device.h"
#if defined(GPCRC_PRESENT) && (GPCRC_COUNT > 0)

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup GPCRC
 * @brief General Purpose Cyclic Redundancy Check (GPCRC) API.
 *
 * @details
 * The GPCRC API functions provide full support for the GPCRC peripheral.
 *
 * The GPCRC module is a peripheral that implements a Cyclic Redundancy Check
 * (CRC) function. It supports a fixed 32-bit polynomial and a user
 * configurable 16-bit polynomial. The fixed 32-bit polynomial is the commonly
 * used IEEE 802.3 polynomial 0x04C11DB7.
 *
 * When using a 16-bit polynomial it's up to the user to choose a polynomial
 * that fits the application. Commonly used 16-bit polynomial are 0x1021
 * (CCITT-16), 0x3D65 (IEC16-MBus), and 0x8005 (ZigBee, 802.15.4, and USB).
 * See this link for other polynomials:
 * https://en.wikipedia.org/wiki/Cyclic_redundancy_check
 *
 * Before a CRC calculation can begin it is important to call the
 * @ref GPCRC_Start function. This function will reset the CRC calculation
 * by copying the configured initialization value over to the CRC data register.
 *
 * There are two ways of sending input data to the GPCRC. You can either write
 * the input data into the input data register using the input functions
 * @ref GPCRC_InputU32, @ref GPCRC_InputU16 and @ref GPCRC_InputU8, or the
 * user can setup the @ref LDMA to transfer data directly to one of the GPCRC
 * input data registers.
 *
 * <b> Examples of GPCRC usage: </b>
 *
 * A CRC-32 Calculation:
 *
 * @include em_gpcrc_crc32.c
 *
 * A CRC-16 Calculation:
 *
 * @include em_gpcrc_crc16.c
 *
 * A CRC-CCITT calculation:
 *
 * @include em_gpcrc_ccit.c
 *
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** CRC initialization structure. */
typedef struct
{
  /**
   * CRC polynomial value. The GPCRC support either a fixed 32-bit polynomial
   * or a user configurable 16 bit polynomial. The fixed 32-bit polynomial
   * is the one used in IEEE 802.3, which has the value 0x04C11DB7. To use the
   * 32-bit fixed polynomial just assign 0x04C11DB7 to the crcPoly field. To use
   * a 16-bit polynomial assign a value to crcPoly where the upper 16 bit's are
   * zero.
   *
   * The polynomial should be written in normal bit order. So for instance
   * if you want to use the CRC-16 polynomial X^16 + X^15 + X^2 + 1 then we
   * can first convert it to hex representation and remove the highest order term
   * of the polynomial. This would give us 0x8005 as the value to write into
   * crcPoly.
   */
  uint32_t                   crcPoly;

  /**
   * CRC initialization value. This value is assigned to the GPCRC_INIT register.
   * The initValue is loaded into the data register when calling the
   * @ref GPCRC_Start function or when one of the data registers are read
   * while @ref autoInit is enabled.
   */
  uint32_t                   initValue;

  /**
   * Reverse byte order. This has an effect when sending a 32-bit word or
   * 16-bit half word input to the CRC calculation. When set to true the input
   * bytes are reversed before entering the CRC calculation. When set to
   * false the input bytes stay in the same order.
   */
  bool                       reverseByteOrder;

  /**
   * Reverse bits within each input byte. This setting enables or disable byte
   * level bit reversal. When byte-level bit reversal is enabled then each byte
   * of input data will be reversed before entering the CRC calculation.
   */
  bool                       reverseBits;

  /**
   * Enable/disable byte mode. When byte mode is enabled then all input
   * is treated as single byte input, even though the input is a 32-bit word
   * or a 16-bit half word. Only the least significant byte of the data-word
   * will be used for CRC calculation for all writes.
   */
  bool                       enableByteMode;

  /**
   * Enable automatic initialization by re-seeding the CRC result based on
   * the init value after reading one of the CRC data registers
   */
  bool                       autoInit;

  /** Enable/disable GPCRC when initialization is completed. */
  bool                       enable;
} GPCRC_Init_TypeDef;

/** Default configuration for GPCRC_Init_TypeDef structure. */
#define GPCRC_INIT_DEFAULT                                             \
{                                                                      \
  0x04C11DB7UL,          /* CRC32 Polynomial value. */                 \
  0x00000000UL,          /* Initialization value. */                   \
  false,                 /* Byte order is normal. */                   \
  false,                 /* Bit order is not reversed on output. */    \
  false,                 /* Disable byte mode. */                      \
  false,                 /* Disable automatic init on data read. */    \
  true,                  /* Enable GPCRC. */                           \
}

/*******************************************************************************
 ******************************   PROTOTYPES   *********************************
 ******************************************************************************/

void GPCRC_Init(GPCRC_TypeDef * gpcrc, const GPCRC_Init_TypeDef * init);
void GPCRC_Reset(GPCRC_TypeDef * gpcrc);

/***************************************************************************//**
 * @brief
 *   Enable/disable GPCRC.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 *
 * @param[in] enable
 *   True to enable GPCRC, false to disable.
 ******************************************************************************/
__STATIC_INLINE void GPCRC_Enable(GPCRC_TypeDef * gpcrc, bool enable)
{
  BUS_RegBitWrite(&gpcrc->CTRL, _GPCRC_CTRL_EN_SHIFT, enable);
}

/***************************************************************************//**
 * @brief
 *   Issues a command to initialize the CRC calculation.
 *
 * @details
 *   This function issues the command INIT in GPCRC_CMD that initializes the
 *   CRC calculation by writing the initial values to the DATA register.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void GPCRC_Start(GPCRC_TypeDef * gpcrc)
{
  gpcrc->CMD = GPCRC_CMD_INIT;
}

/***************************************************************************//**
 * @brief
 *   Set the initialization value of the CRC.
 *
 * @param [in] initValue
 *   The value to use to initialize a CRC calculation. This value is moved into
 *   the data register when calling @ref GPCRC_Start
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void GPCRC_InitValueSet(GPCRC_TypeDef * gpcrc, uint32_t initValue)
{
  gpcrc->INIT = initValue;
}

/***************************************************************************//**
 * @brief
 *   Writes a 32 bit value to the input data register of the CRC.
 *
 * @details
 *   Use this function to write a 32 bit input data to the CRC. The CRC
 *   calculation is based on the provided input data using the configured
 *   CRC polynomial.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 *
 * @param[in] data
 *   Data to be written to the input data register.
 ******************************************************************************/
__STATIC_INLINE void GPCRC_InputU32(GPCRC_TypeDef * gpcrc, uint32_t data)
{
  gpcrc->INPUTDATA = data;
}

/***************************************************************************//**
 * @brief
 *   Writes a 16 bit value to the input data register of the CRC.
 *
 * @details
 *   Use this function to write a 16 bit input data to the CRC. The CRC
 *   calculation is based on the provided input data using the configured
 *   CRC polynomial.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 *
 * @param[in] data
 *   Data to be written to the input data register.
 ******************************************************************************/
__STATIC_INLINE void GPCRC_InputU16(GPCRC_TypeDef * gpcrc, uint16_t data)
{
  gpcrc->INPUTDATAHWORD = data;
}

/***************************************************************************//**
 * @brief
 *   Writes an 8 bit value to the input data register of the CRC.
 *
 * @details
 *   Use this function to write a 8 bit input data to the CRC. The CRC
 *   calculation is based on the provided input data using the configured
 *   CRC polynomial.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 *
 * @param[in] data
 *   Data to be written to the input data register.
 ******************************************************************************/
__STATIC_INLINE void GPCRC_InputU8(GPCRC_TypeDef * gpcrc, uint8_t data)
{
  gpcrc->INPUTDATABYTE = data;
}

/***************************************************************************//**
 * @brief
 *   Reads the data register of the CRC.
 *
 * @details
 *   Use this function to read the calculated CRC value.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 *
 * @return
 *   Content of the CRC data register.
 ******************************************************************************/
__STATIC_INLINE uint32_t GPCRC_DataRead(GPCRC_TypeDef * gpcrc)
{
  return gpcrc->DATA;
}

/***************************************************************************//**
 * @brief
 *   Reads the data register of the CRC bit reversed.
 *
 * @details
 *   Use this function to read the calculated CRC value bit reversed. When
 *   using a 32-bit polynomial then bits [31:0] are reversed, when using a
 *   16-bit polynomial then bits [15:0] are reversed.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 *
 * @return
 *   Content of the CRC data register bit reversed.
 ******************************************************************************/
__STATIC_INLINE uint32_t GPCRC_DataReadBitReversed(GPCRC_TypeDef * gpcrc)
{
  return gpcrc->DATAREV;
}

/***************************************************************************//**
 * @brief
 *   Reads the data register of the CRC byte reversed.
 *
 * @details
 *   Use this function to read the calculated CRC value byte reversed.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 *
 * @return
 *   Content of the CRC data register byte reversed.
 ******************************************************************************/
__STATIC_INLINE uint32_t GPCRC_DataReadByteReversed(GPCRC_TypeDef * gpcrc)
{
  return gpcrc->DATABYTEREV;
}

/** @} (end addtogroup GPCRC) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(GPCRC_COUNT) && (GPCRC_COUNT > 0) */
#endif /* EM_GPCRC_H */
