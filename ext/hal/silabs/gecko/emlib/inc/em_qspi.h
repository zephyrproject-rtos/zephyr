/***************************************************************************//**
 * @file em_qspi.h
 * @brief QSPI Octal-SPI Flash Controller API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
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

#ifndef EM_QSPI_H
#define EM_QSPI_H

#include "em_device.h"
#if defined(QSPI_COUNT) && (QSPI_COUNT > 0)

#ifdef __cplusplus
extern "C" {
#endif

#include "em_bus.h"
#include <stdbool.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup QSPI
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Transfer type. */
typedef enum {
  /** Single IO mode. DQ0 used for output and DQ1 as input. */
  qspiTransferSingle = 0,

  /** Dual I/O transfer. DQ0 and DQ1 are used as both inputs and outputs. */
  qspiTransferDual   = 1,

  /** Quad I/O transfer. DQ0, DQ1, DQ2 and DQ3 are used as both inputs and outputs. */
  qspiTransferQuad   = 2,

  /** Octal I/O transfer. DQ[7:0] are used as both inputs and outputs. */
  qspiTransferOctal  = 3
} QSPI_TransferType_TypeDef;

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** QSPI Device Read Instruction Configuration Structure. */
typedef struct {
  /** Read opcode in non-xip mode. */
  uint8_t                   opCode;

  /** Number of dummy read clock cycles. */
  uint8_t                   dummyCycles;

  /** Transfer type used for address. */
  QSPI_TransferType_TypeDef addrTransfer;

  /** Transfer type used for data. */
  QSPI_TransferType_TypeDef dataTransfer;

  /** Transfer type used for instruction. */
  QSPI_TransferType_TypeDef instTransfer;
} QSPI_ReadConfig_TypeDef;

/** Default Read Configuration Structure. */
#define QSPI_READCONFIG_DEFAULT                                  \
  {                                                              \
    0x03,                /* 0x03 is the standard read opcode. */ \
    0,                   /* 0 dummy cycles. */                   \
    qspiTransferSingle,  /* Single I/O mode. */                  \
    qspiTransferSingle,  /* Single I/O mode. */                  \
    qspiTransferSingle,  /* Single I/O mode. */                  \
  }

/** QSPI Device Write Instruction Configuration Structure. */
typedef struct {
  /** Write opcode. */
  uint8_t                   opCode;

  /** Number of dummy read clock cycles. */
  uint8_t                   dummyCycles;

  /** Transfer type used for address. */
  QSPI_TransferType_TypeDef addrTransfer;

  /** Transfer type used for data. */
  QSPI_TransferType_TypeDef dataTransfer;

  /**
   * @brief
   *   Enable/disable automatic issuing of WEL (Write Enable Latch)
   *   command before a write operation.
   *
   * @details
   *   When writing to a flash device, the WEL within the flash device must be
   *   high before a write sequence can be issued. The QSPI peripheral can
   *   automatically issue the WEL command before triggering a write sequence.
   *   The command used for enabling the WEL is WREN (0x06) and is common between
   *   devices. */
  bool                      autoWEL;
} QSPI_WriteConfig_TypeDef;

/** Default Write Configuration Structure. */
#define QSPI_WRITECONFIG_DEFAULT                                  \
  {                                                               \
    0x02,                /* 0x02 is the standard write opcode. */ \
    0,                   /* 0 dummy cycles. */                    \
    qspiTransferSingle,  /* Single I/O mode. */                   \
    qspiTransferSingle,  /* Single I/O mode. */                   \
    true,                /* Send WEL command automatically. */    \
  }

/** QSPI Device Delay Configuration Structure. */
typedef struct {
  /** The minimal delay to keep the chip select line de-asserted between
   *  two transactions. */
  uint8_t deassert;

  /** Delay between one chip select being de-activated and the
   * activation of another. */
  uint8_t deviceSwitch;

  /** Delay between last bit and chip select de-assert. */
  uint8_t lastBit;

  /** Delay chip select assert and first bit in a transaction. */
  uint8_t firstBit;
} QSPI_DelayConfig_TypeDef;

/** Defines command to be executed using STIG mechanism. */
typedef struct {
  /** Command op-code. */
  uint8_t  cmdOpcode;
  /** Number of Read Data Bytes. */
  uint16_t readDataSize;
  /** Number of Address Bytes. */
  uint8_t  addrSize;
  /** Number of Write Data Bytes. */
  uint8_t  writeDataSize;
  /** Number of dummy cycles. */
  uint8_t  dummyCycles;
  /** Mode Bit Configuration register are sent following the address bytes. */
  bool     modeBitEnable;
  /** Flash command address. */
  uint32_t address;
  /** Buffer for read data. */
  void *   readBuffer;
  /** Buffer with data to write. */
  void *   writeBuffer;
} QSPI_StigCmd_TypeDef;

/** QSPI initialization structure. */
typedef struct {
  /** Enable/disable Quad SPI when initialization is completed. */
  bool                       enable;

  /**
   * Master mode baude rate divisor. Values can be even numbers in the range
   * [2-32] inclusive. */
  uint8_t                    divisor;
} QSPI_Init_TypeDef;

/** Default configuration for QSPI_Init_TypeDef structure. */
#define QSPI_INIT_DEFAULT                               \
  {                                                     \
    true,                /* Enable Quad SPI. */         \
    32,                  /* Divide QSPI clock by 32. */ \
  }

/*******************************************************************************
 ******************************   PROTOTYPES   *********************************
 ******************************************************************************/

void QSPI_Init(QSPI_TypeDef * qspi, const QSPI_Init_TypeDef * init);
void QSPI_ReadConfig(QSPI_TypeDef * qspi, const QSPI_ReadConfig_TypeDef * config);
void QSPI_WriteConfig(QSPI_TypeDef * qspi, const QSPI_WriteConfig_TypeDef * config);
void QSPI_ExecStigCmd(QSPI_TypeDef * qspi, const QSPI_StigCmd_TypeDef * stigCmd);

/***************************************************************************//**
 * @brief
 *   Wait for the QSPI to go into idle state.
 *
 * @param[in] qspi
 *   Pointer to QSPI peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void QSPI_WaitForIdle(QSPI_TypeDef * qspi)
{
  while ((qspi->CONFIG & _QSPI_CONFIG_IDLE_MASK) == 0)
    ;
}

/***************************************************************************//**
 * @brief
 *   Get the fill level of the write partition of the QSPI internal SRAM.
 *
 * @param[in] qspi
 *   Pointer to QSPI peripheral register block.
 *
 * @return
 *   SRAM fill level of the write partition. The value is the number of 4 byte
 *   words in the write partition.
 ******************************************************************************/
__STATIC_INLINE uint16_t QSPI_GetWriteLevel(QSPI_TypeDef * qspi)
{
  return (qspi->SRAMFILL & _QSPI_SRAMFILL_SRAMFILLINDACWRITE_MASK)
         >> _QSPI_SRAMFILL_SRAMFILLINDACWRITE_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   Get the fill level of the read partition of the QSPI internal SRAM.
 *
 * @param[in] qspi
 *   Pointer to QSPI peripheral register block.
 *
 * @return
 *   SRAM fill level of the read partition. The value is the number of 4 byte
 *   words in the read partition.
 ******************************************************************************/
__STATIC_INLINE uint16_t QSPI_GetReadLevel(QSPI_TypeDef * qspi)
{
  return (qspi->SRAMFILL & _QSPI_SRAMFILL_SRAMFILLINDACREAD_MASK)
         >> _QSPI_SRAMFILL_SRAMFILLINDACREAD_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   Enable/disable Quad SPI.
 *
 * @param[in] qspi
 *   Pointer to QSPI peripheral register block.
 *
 * @param[in] enable
 *   true to enable quad spi, false to disable quad spi.
 ******************************************************************************/
__STATIC_INLINE void QSPI_Enable(QSPI_TypeDef * qspi, bool enable)
{
  BUS_RegBitWrite(&qspi->CONFIG, _QSPI_CONFIG_ENBSPI_SHIFT, enable ? 1 : 0);
}

/***************************************************************************//**
 * @brief
 *   Get the current interrupt flags.
 *
 * @param[in] qspi
 *   Pointer to QSPI peripheral register block.
 *
 * @return
 *   This functions returns the current interrupt flags that are set.
 ******************************************************************************/
__STATIC_INLINE uint32_t QSPI_IntGet(QSPI_TypeDef * qspi)
{
  return qspi->IRQSTATUS;
}

/***************************************************************************//**
 * @brief
 *   Clear interrupt flags.
 *
 * @param[in] qspi
 *   Pointer to QSPI peripheral register block.
 *
 * @param[in] flags
 *   The interrupt flags to clear.
 ******************************************************************************/
__STATIC_INLINE void QSPI_IntClear(QSPI_TypeDef * qspi, uint32_t flags)
{
  qspi->IRQSTATUS = flags;
}

/***************************************************************************//**
 * @brief
 *   Enable interrupts.
 *
 * @param[in] qspi
 *   Pointer to QSPI peripheral register block.
 *
 * @param[in] flags
 *   The interrupt flags to enable.
 ******************************************************************************/
__STATIC_INLINE void QSPI_IntEnable(QSPI_TypeDef * qspi, uint32_t flags)
{
  qspi->IRQMASK = flags & (~_QSPI_IRQMASK_MASK);
}

/***************************************************************************//**
 * @brief
 *   Disable interrupts.
 *
 * @param[in] qspi
 *   Pointer to QSPI peripheral register block.
 *
 * @param[in] flags
 *   The interrupt flags to disable.
 ******************************************************************************/
__STATIC_INLINE void QSPI_IntDisable(QSPI_TypeDef * qspi, uint32_t flags)
{
  qspi->IRQMASK = ~flags & (~_QSPI_IRQMASK_MASK);
}

/** @} (end addtogroup QSPI) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(QSPI_COUNT) && (QSPI_COUNT > 0) */
#endif /* EM_QSPI_H */
