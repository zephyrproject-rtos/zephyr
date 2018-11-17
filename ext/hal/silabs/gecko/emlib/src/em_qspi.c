/***************************************************************************//**
 * @file em_qspi.c
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

#include "em_qspi.h"

#if defined(QSPI_COUNT) && (QSPI_COUNT > 0)

#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/* *INDENT-OFF* */
/***************************************************************************//**
 * @addtogroup QSPI
 * @brief QSPI Octal-SPI Controller API
 * @details
 *  These QSPI functions provide basic support for using the QSPI peripheral
 *  in the following configurations:
 *  @li @b Direct Read/Write, used for memory mapped access to external
 *    memory.
 *  @li @b STIG Command, used for configuring and executing commands on the
 *    external memory device.
 *
 *  Indirect read/write, PHY configuration, and Execute-In-Place (XIP)
 *  configurations are not supported.
 *
 * The example below shows how to set up the QSPI for direct read and write
 * operation:
 * @code
   CMU_ClockEnable(cmuClock_GPIO, true);
   CMU_ClockEnable(cmuClock_QSPI0, true);

   QSPI_Init_TypeDef initQspi = QSPI_INIT_DEFAULT;
   QSPI_Init(QSPI0, &initQspi);

   // Configure QSPI pins.
   GPIO_PinModeSet(EXTFLASH_PORT_CS,   EXTFLASH_PIN_CS,   gpioModePushPull, 0);
   GPIO_PinModeSet(EXTFLASH_PORT_SCLK, EXTFLASH_PIN_SCLK, gpioModePushPull, 0);
   GPIO_PinModeSet(EXTFLASH_PORT_DQ0,  EXTFLASH_PIN_DQ0,  gpioModePushPull, 0);
   GPIO_PinModeSet(EXTFLASH_PORT_DQ1,  EXTFLASH_PIN_DQ1,  gpioModePushPull, 0);
   GPIO_PinModeSet(EXTFLASH_PORT_DQ2,  EXTFLASH_PIN_DQ2,  gpioModePushPull, 0);
   GPIO_PinModeSet(EXTFLASH_PORT_DQ3,  EXTFLASH_PIN_DQ3,  gpioModePushPull, 0);

   // Configure QSPI routing to GPIO.
   QSPI0->ROUTELOC0 = EXTFLASH_QSPI_LOC;
   QSPI0->ROUTEPEN  = QSPI_ROUTEPEN_SCLKPEN
                      | EXTFLASH_QSPI_CSPEN
                      | QSPI_ROUTEPEN_DQ0PEN
                      | QSPI_ROUTEPEN_DQ1PEN
                      | QSPI_ROUTEPEN_DQ2PEN
                      | QSPI_ROUTEPEN_DQ3PEN;

   // Configure the direct read.
   QSPI_ReadConfig_TypeDef readConfig = QSPI_READCONFIG_DEFAULT;

   readConfig.dummyCycles  = 8;
   readConfig.opCode       = 0x6B;
   readConfig.instTransfer = qspiTransferSingle;
   readConfig.addrTransfer = qspiTransferSingle;
   readConfig.dataTransfer = qspiTransferQuad;

   QSPI_ReadConfig(QSPI0, &readConfig);

   // Configure the direct write.
   QSPI_WriteConfig_TypeDef writeConfig = QSPI_WRITECONFIG_DEFAULT;

   writeConfig.dummyCycles  = 0;
   writeConfig.opCode       = 0x38;
   writeConfig.addrTransfer = qspiTransferQuad;
   writeConfig.dataTransfer = qspiTransferQuad;
   writeConfig.autoWEL      = true;

   QSPI_WriteConfig(QSPI0, &writeConfig);@endcode
 *
 * To configure an external flash, commands can be set up and executed using the
 * Software Triggered Instruction Generator (STIG) function of the QSPI, as
 * shown in the example below:
 * @code
   uint8_t status;
   QSPI_StigCmd_TypeDef stigCmd = {0};
   stigCmd.cmdOpcode = EXTFLASH_OPCODE_READ_STATUS;
   stigCmd.readDataSize = 1;
   stigCmd.readBuffer = &status;
   QSPI_ExecStigCmd(QSPI0, &stigCmd);@endcode
 * @{
 ******************************************************************************/
/* *INDENT-OFF* */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize QSPI.
 *
 * @param[in] qspi
 *   A pointer to the QSPI peripheral register block.
 *
 * @param[in] init
 *   A pointer to the initialization structure used to configure QSPI.
 ******************************************************************************/
void QSPI_Init(QSPI_TypeDef * qspi, const QSPI_Init_TypeDef * init)
{
  uint32_t divisor;

  EFM_ASSERT((init->divisor >= 2) && (init->divisor <= 32));
  divisor = init->divisor / 2 - 1;

  qspi->CONFIG = (qspi->CONFIG & ~_QSPI_CONFIG_MSTRBAUDDIV_MASK)
                 | (divisor << _QSPI_CONFIG_MSTRBAUDDIV_SHIFT);
  QSPI_Enable(qspi, init->enable);
}

/***************************************************************************//**
 * @brief
 *   Configure Read Operations.
 *
 * @param[in] qspi
 *   A pointer to the QSPI peripheral register block.
 *
 * @param[in] config
 *   A pointer to the configuration structure for QSPI read operations.
 ******************************************************************************/
void QSPI_ReadConfig(QSPI_TypeDef * qspi, const QSPI_ReadConfig_TypeDef * config)
{
  EFM_ASSERT(config->dummyCycles < 31);

  QSPI_WaitForIdle(qspi);
  qspi->DEVINSTRRDCONFIG = (config->opCode << _QSPI_DEVINSTRRDCONFIG_RDOPCODENONXIP_SHIFT)
                           | (config->dummyCycles << _QSPI_DEVINSTRRDCONFIG_DUMMYRDCLKCYCLES_SHIFT)
                           | (config->addrTransfer << _QSPI_DEVINSTRRDCONFIG_ADDRXFERTYPESTDMODE_SHIFT)
                           | (config->dataTransfer << _QSPI_DEVINSTRRDCONFIG_DATAXFERTYPEEXTMODE_SHIFT)
                           | (config->instTransfer << _QSPI_DEVINSTRRDCONFIG_INSTRTYPE_SHIFT);
}

/***************************************************************************//**
 * @brief
 *   Configure Write Operations.
 *
 * @param[in] qspi
 *   A pointer to the QSPI peripheral register block.
 *
 * @param[in] config
 *   A pointer to the configuration structure for QSPI write operations.
 ******************************************************************************/
void QSPI_WriteConfig(QSPI_TypeDef * qspi, const QSPI_WriteConfig_TypeDef * config)
{
  EFM_ASSERT(config->dummyCycles < 31);

  QSPI_WaitForIdle(qspi);
  qspi->DEVINSTRWRCONFIG = (config->opCode << _QSPI_DEVINSTRWRCONFIG_WROPCODE_SHIFT)
                           | (config->dummyCycles << _QSPI_DEVINSTRWRCONFIG_DUMMYWRCLKCYCLES_SHIFT)
                           | (config->addrTransfer << _QSPI_DEVINSTRWRCONFIG_ADDRXFERTYPESTDMODE_SHIFT)
                           | (config->dataTransfer << _QSPI_DEVINSTRWRCONFIG_DATAXFERTYPEEXTMODE_SHIFT)
                           | ((config->autoWEL ? 0 : 1) << _QSPI_DEVINSTRWRCONFIG_WELDIS_SHIFT);
}

/***************************************************************************//**
 * @brief
 *   Execute a STIG command.
 *
 * @details
 *   STIG is used when the
 *   application needs to access status registers, configuration registers or
 *   perform erase functions. STIG commands can be used to perform any
 *   instruction that the flash device supports.
 *
 * @param[in] qspi
 *   A pointer to the QSPI peripheral register block.
 *
 * @param[in] stigCmd
 *   A pointer to a structure that describes the STIG command.
 ******************************************************************************/
void QSPI_ExecStigCmd(QSPI_TypeDef * qspi, const QSPI_StigCmd_TypeDef * stigCmd)
{
  uint32_t i;

  EFM_ASSERT(stigCmd->addrSize <= 4);
  EFM_ASSERT(stigCmd->writeDataSize <= 8);
  EFM_ASSERT(stigCmd->readDataSize <= 8);
  EFM_ASSERT(stigCmd->dummyCycles < 32);

  if (stigCmd->writeDataSize) {
    EFM_ASSERT(stigCmd->writeBuffer);
  }

  if (stigCmd->readDataSize) {
    EFM_ASSERT(stigCmd->readBuffer);
  }

  QSPI_WaitForIdle(qspi);

  qspi->FLASHCMDCTRL = (stigCmd->cmdOpcode << _QSPI_FLASHCMDCTRL_CMDOPCODE_SHIFT)
                       | (stigCmd->dummyCycles << _QSPI_FLASHCMDCTRL_NUMDUMMYCYCLES_SHIFT);

  if (stigCmd->writeDataSize) {
    uint32_t buffer[2] = { 0, 0 };
    uint8_t * dst = (uint8_t *) buffer;
    uint8_t * src = stigCmd->writeBuffer;

    qspi->FLASHCMDCTRL |= QSPI_FLASHCMDCTRL_ENBWRITEDATA
                          | ((stigCmd->writeDataSize - 1)
                             << _QSPI_FLASHCMDCTRL_NUMWRDATABYTES_SHIFT);

    for (i = 0; i < stigCmd->writeDataSize; i++) {
      dst[i] = src[i];
    }

    qspi->FLASHWRDATALOWER = buffer[0];
    qspi->FLASHWRDATAUPPER = buffer[1];
  }

  if (stigCmd->addrSize) {
    qspi->FLASHCMDCTRL |= QSPI_FLASHCMDCTRL_ENBCOMDADDR
                          | ((stigCmd->addrSize - 1)
                             << _QSPI_FLASHCMDCTRL_NUMADDRBYTES_SHIFT);
    qspi->FLASHCMDADDR = stigCmd->address;
  }

  if (stigCmd->modeBitEnable) {
    qspi->FLASHCMDCTRL |= QSPI_FLASHCMDCTRL_ENBMODEBIT;
  }

  if (stigCmd->readDataSize) {
    qspi->FLASHCMDCTRL |= QSPI_FLASHCMDCTRL_ENBREADDATA
                          | ((stigCmd->readDataSize - 1)
                             << _QSPI_FLASHCMDCTRL_NUMRDDATABYTES_SHIFT);
  }

  // Start command execution
  qspi->FLASHCMDCTRL |= QSPI_FLASHCMDCTRL_CMDEXEC;

  while (qspi->FLASHCMDCTRL & QSPI_FLASHCMDCTRL_CMDEXECSTATUS)
    ;

  // Read data if any
  if (stigCmd->readDataSize) {
    uint32_t buffer[2] = { 0, 0 };
    const uint8_t * src = (const uint8_t *)buffer;
    uint8_t * dst = stigCmd->readBuffer;

    buffer[0] = qspi->FLASHRDDATALOWER;
    buffer[1] = qspi->FLASHRDDATAUPPER;

    for (i = 0; i < stigCmd->readDataSize; i++) {
      dst[i] = src[i];
    }
  }
}

/** @} (end addtogroup QSPI) */
/** @} (end addtogroup emlib) */

#endif /* defined(QSPI_COUNT) && (QSPI_COUNT > 0) */
