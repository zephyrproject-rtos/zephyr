/*
** ###################################################################
**     Version:             rev. 1.0, 2018-10-02
**     Build:               b180815
**
**     Abstract:
**         Chip specific module features.
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2018 NXP
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2018-10-02)
**         Initial version.
**
** ###################################################################
*/

#ifndef _RV32M1_zero_riscy_FEATURES_H_
#define _RV32M1_zero_riscy_FEATURES_H_

/* SOC module features */

/* @brief CAU3 availability on the SoC. */
#define FSL_FEATURE_SOC_CAU3_COUNT (1)
/* @brief CRC availability on the SoC. */
#define FSL_FEATURE_SOC_CRC_COUNT (1)
/* @brief DMAMUX availability on the SoC. */
#define FSL_FEATURE_SOC_DMAMUX_COUNT (1)
/* @brief EDMA availability on the SoC. */
#define FSL_FEATURE_SOC_EDMA_COUNT (1)
/* @brief EMVSIM availability on the SoC. */
#define FSL_FEATURE_SOC_EMVSIM_COUNT (1)
/* @brief EVENT availability on the SoC. */
#define FSL_FEATURE_SOC_EVENT_COUNT (1)
/* @brief EWM availability on the SoC. */
#define FSL_FEATURE_SOC_EWM_COUNT (1)
/* @brief FB availability on the SoC. */
#define FSL_FEATURE_SOC_FB_COUNT (1)
/* @brief FGPIO availability on the SoC. */
#define FSL_FEATURE_SOC_FGPIO_COUNT (1)
/* @brief FLASH availability on the SoC. */
#define FSL_FEATURE_SOC_FLASH_COUNT (1)
/* @brief FLEXIO availability on the SoC. */
#define FSL_FEATURE_SOC_FLEXIO_COUNT (1)
/* @brief GPIO availability on the SoC. */
#define FSL_FEATURE_SOC_GPIO_COUNT (5)
/* @brief I2S availability on the SoC. */
#define FSL_FEATURE_SOC_I2S_COUNT (1)
/* @brief INTMUX availability on the SoC. */
#define FSL_FEATURE_SOC_INTMUX_COUNT (1)
/* @brief LLWU availability on the SoC. */
#define FSL_FEATURE_SOC_LLWU_COUNT (2)
/* @brief LPADC availability on the SoC. */
#define FSL_FEATURE_SOC_LPADC_COUNT (1)
/* @brief LPCMP availability on the SoC. */
#define FSL_FEATURE_SOC_LPCMP_COUNT (2)
/* @brief LPDAC availability on the SoC. */
#define FSL_FEATURE_SOC_LPDAC_COUNT (1)
/* @brief LPI2C availability on the SoC. */
#define FSL_FEATURE_SOC_LPI2C_COUNT (4)
/* @brief LPIT availability on the SoC. */
#define FSL_FEATURE_SOC_LPIT_COUNT (2)
/* @brief LPSPI availability on the SoC. */
#define FSL_FEATURE_SOC_LPSPI_COUNT (4)
/* @brief LPTMR availability on the SoC. */
#define FSL_FEATURE_SOC_LPTMR_COUNT (3)
/* @brief LPUART availability on the SoC. */
#define FSL_FEATURE_SOC_LPUART_COUNT (4)
/* @brief MCM availability on the SoC. */
#define FSL_FEATURE_SOC_MCM_COUNT (1)
/* @brief MMDVSQ availability on the SoC. */
#define FSL_FEATURE_SOC_MMDVSQ_COUNT (1)
/* @brief MSCM availability on the SoC. */
#define FSL_FEATURE_SOC_MSCM_COUNT (1)
/* @brief MTB availability on the SoC. */
#define FSL_FEATURE_SOC_MTB_COUNT (1)
/* @brief MTBDWT availability on the SoC. */
#define FSL_FEATURE_SOC_MTBDWT_COUNT (1)
/* @brief MU availability on the SoC. */
#define FSL_FEATURE_SOC_MU_COUNT (1)
/* @brief PCC availability on the SoC. */
#define FSL_FEATURE_SOC_PCC_COUNT (2)
/* @brief PORT availability on the SoC. */
#define FSL_FEATURE_SOC_PORT_COUNT (5)
/* @brief ROM availability on the SoC. */
#define FSL_FEATURE_SOC_ROM_COUNT (1)
/* @brief RSIM availability on the SoC. */
#define FSL_FEATURE_SOC_RSIM_COUNT (1)
/* @brief RTC availability on the SoC. */
#define FSL_FEATURE_SOC_RTC_COUNT (1)
/* @brief SCG availability on the SoC. */
#define FSL_FEATURE_SOC_SCG_COUNT (1)
/* @brief SEMA42 availability on the SoC. */
#define FSL_FEATURE_SOC_SEMA42_COUNT (2)
/* @brief SIM availability on the SoC. */
#define FSL_FEATURE_SOC_SIM_COUNT (1)
/* @brief SMC availability on the SoC. */
#define FSL_FEATURE_SOC_SMC_COUNT (2)
/* @brief SPM availability on the SoC. */
#define FSL_FEATURE_SOC_SPM_COUNT (1)
/* @brief TPM availability on the SoC. */
#define FSL_FEATURE_SOC_TPM_COUNT (4)
/* @brief TRGMUX availability on the SoC. */
#define FSL_FEATURE_SOC_TRGMUX_COUNT (2)
/* @brief TRNG availability on the SoC. */
#define FSL_FEATURE_SOC_TRNG_COUNT (1)
/* @brief TSTMR availability on the SoC. */
#define FSL_FEATURE_SOC_TSTMR_COUNT (1)
/* @brief USB availability on the SoC. */
#define FSL_FEATURE_SOC_USB_COUNT (1)
/* @brief USBVREG availability on the SoC. */
#define FSL_FEATURE_SOC_USBVREG_COUNT (1)
/* @brief USDHC availability on the SoC. */
#define FSL_FEATURE_SOC_USDHC_COUNT (1)
/* @brief VREF availability on the SoC. */
#define FSL_FEATURE_SOC_VREF_COUNT (1)
/* @brief WDOG availability on the SoC. */
#define FSL_FEATURE_SOC_WDOG_COUNT (2)
/* @brief XRDC availability on the SoC. */
#define FSL_FEATURE_SOC_XRDC_COUNT (1)
/* @brief ZLL availability on the SoC. */
#define FSL_FEATURE_SOC_ZLL_COUNT (1)

/* LPADC module features */

/* @brief Has differential mode (bitfield CMDLn[DIFF]). */
#define FSL_FEATURE_LPADC_HAS_CMDL_DIFF (0)
/* @brief Has channel scale (bitfield CMDLn[CSCALE]). */
#define FSL_FEATURE_LPADC_HAS_CMDL_CSCALE (0)
/* @brief Has internal clock (bitfield CFG[ADCKEN]). */
#define FSL_FEATURE_LPADC_HAS_CFG_ADCKEN (1)
/* @brief Enable support for low voltage reference on option 1 reference (bitfield CFG[VREF1RNG]). */
#define FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG (1)
/* @brief Has calibration (bitfield CFG[CALOFS]). */
#define FSL_FEATURE_LPADC_HAS_CFG_CALOFS (1)
/* @brief Has offset trim (register OFSTRIM). */
#define FSL_FEATURE_LPADC_HAS_OFSTRIM (1)

/* CRC module features */

/* @brief Has data register with name CRC */
#define FSL_FEATURE_CRC_HAS_CRC_REG (0)

/* EDMA module features */

/* @brief Number of DMA channels (related to number of registers TCD, DCHPRI, bit fields ERQ[ERQn], EEI[EEIn], INT[INTn], ERR[ERRn], HRS[HRSn] and bit field widths ES[ERRCHN], CEEI[CEEI], SEEI[SEEI], CERQ[CERQ], SERQ[SERQ], CDNE[CDNE], SSRT[SSRT], CERR[CERR], CINT[CINT], TCDn_CITER_ELINKYES[LINKCH], TCDn_CSR[MAJORLINKCH], TCDn_BITER_ELINKYES[LINKCH]). (Valid only for eDMA modules.) */
#define FSL_FEATURE_EDMA_MODULE_CHANNEL (8)
/* @brief Total number of DMA channels on all modules. */
#define FSL_FEATURE_EDMA_DMAMUX_CHANNELS (8)
/* @brief Number of DMA channel groups (register bit fields CR[ERGA], CR[GRPnPRI], ES[GPE], DCHPRIn[GRPPRI]). (Valid only for eDMA modules.) */
#define FSL_FEATURE_EDMA_CHANNEL_GROUP_COUNT (1)
/* @brief Has DMA_Error interrupt vector. */
#define FSL_FEATURE_EDMA_HAS_ERROR_IRQ (0)
/* @brief Number of DMA channels with asynchronous request capability (register EARS). (Valid only for eDMA modules.) */
#define FSL_FEATURE_EDMA_ASYNCHRO_REQUEST_CHANNEL_COUNT (8)

/* DMAMUX module features */

/* @brief Number of DMA channels (related to number of register CHCFGn). */
#define FSL_FEATURE_DMAMUX_MODULE_CHANNEL (8)
/* @brief Total number of DMA channels on all modules. */
#define FSL_FEATURE_DMAMUX_DMAMUX_CHANNELS (FSL_FEATURE_SOC_DMAMUX_COUNT * 8)
/* @brief Has the periodic trigger capability for the triggered DMA channel (register bit CHCFG0[TRIG]). */
#define FSL_FEATURE_DMAMUX_HAS_TRIG (1)
/* @brief Has DMA Channel Always ON function (register bit CHCFG0[A_ON]). */
#define FSL_FEATURE_DMAMUX_HAS_A_ON (1)

/* EWM module features */

/* @brief Has clock select (register CLKCTRL). */
#define FSL_FEATURE_EWM_HAS_CLOCK_SELECT  (0)
/* @brief Has clock prescaler (register CLKPRESCALER). */
#define FSL_FEATURE_EWM_HAS_PRESCALER  (1)

/* FB module features */

/* No feature definitions */

/* FLEXIO module features */

/* @brief Has Shifter Status Register (FLEXIO_SHIFTSTAT) */
#define FSL_FEATURE_FLEXIO_HAS_SHIFTER_STATUS (1)
/* @brief Has Pin Data Input Register (FLEXIO_PIN) */
#define FSL_FEATURE_FLEXIO_HAS_PIN_STATUS (1)
/* @brief Has Shifter Buffer N Nibble Byte Swapped Register (FLEXIO_SHIFTBUFNBSn) */
#define FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_NIBBLE_BYTE_SWAP (1)
/* @brief Has Shifter Buffer N Half Word Swapped Register (FLEXIO_SHIFTBUFHWSn) */
#define FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_HALF_WORD_SWAP (1)
/* @brief Has Shifter Buffer N Nibble Swapped Register (FLEXIO_SHIFTBUFNISn) */
#define FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_NIBBLE_SWAP (1)
/* @brief Supports Shifter State Mode (FLEXIO_SHIFTCTLn[SMOD]) */
#define FSL_FEATURE_FLEXIO_HAS_STATE_MODE (1)
/* @brief Supports Shifter Logic Mode (FLEXIO_SHIFTCTLn[SMOD]) */
#define FSL_FEATURE_FLEXIO_HAS_LOGIC_MODE (1)
/* @brief Supports paralle width (FLEXIO_SHIFTCFGn[PWIDTH]) */
#define FSL_FEATURE_FLEXIO_HAS_PARALLEL_WIDTH (1)
/* @brief Reset value of the FLEXIO_VERID register */
#define FSL_FEATURE_FLEXIO_VERID_RESET_VALUE (0x1010001)
/* @brief Reset value of the FLEXIO_PARAM register */
#define FSL_FEATURE_FLEXIO_PARAM_RESET_VALUE (0x4200808)

/* FLASH module features */

/* @brief Current core ID. */
#define FSL_FEATURE_FLASH_CURRENT_CORE_ID (1)
/* @brief Is of type FTFA. */
#define FSL_FEATURE_FLASH_IS_FTFA (0)
/* @brief Is of type FTFE. */
#define FSL_FEATURE_FLASH_IS_FTFE (1)
/* @brief Is of type FTFL. */
#define FSL_FEATURE_FLASH_IS_FTFL (0)
/* @brief Has flags indicating the status of the FlexRAM (register bits FCNFG[EEERDY], FCNFG[RAMRDY] and FCNFG[PFLSH]). */
#define FSL_FEATURE_FLASH_HAS_FLEX_RAM_FLAGS (1)
/* @brief Has program flash swapping status flag (register bit FCNFG[SWAP]). */
#define FSL_FEATURE_FLASH_HAS_PFLASH_SWAPPING_STATUS_FLAG (1)
/* @brief Has EEPROM region protection (register FEPROT). */
#define FSL_FEATURE_FLASH_HAS_EEROM_REGION_PROTECTION (0)
/* @brief Has data flash region protection (register FDPROT). */
#define FSL_FEATURE_FLASH_HAS_DATA_FLASH_REGION_PROTECTION (0)
/* @brief Has flash access control (registers XACCHn, SACCHn, where n is a number, FACSS and FACSN). */
#define FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL (1)
/* @brief Has flash cache control in FMC module. */
#define FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS (0)
/* @brief Has flash cache control in MCM module. */
#define FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS (0)
/* @brief Has flash cache control in MSCM module. */
#define FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS (1)
/* @brief Has prefetch speculation control in flash, such as kv5x. */
#define FSL_FEATURE_FLASH_PREFETCH_SPECULATION_CONTROL_IN_FLASH (0)
/* @brief P-Flash flash size coding rule version, value 0 for K1 and K2, value 1 for RV32M1 */
#define FSL_FEATURE_FLASH_SIZE_ENCODING_RULE_VERSION (1)
/* @brief P-Flash start address. */
#define FSL_FEATURE_FLASH_PFLASH_START_ADDRESS (0x01000000)
/* @brief P-Flash block count. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT (1)
/* @brief P-Flash block size. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE (262144)
/* @brief P-Flash sector size. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE (2048)
/* @brief P-Flash write unit size. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE (8)
/* @brief P-Flash data path width. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_DATA_PATH_WIDTH (8)
/* @brief P-Flash block swap feature. */
#define FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP (0)
/* @brief P-Flash protection region count. */
#define FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT (16)
/* @brief Has multiple flash. */
#define FSL_FEATURE_FLASH_HAS_MULTIPLE_FLASH (1)
/* @brief Flash memory count. */
#define FSL_FEATURE_FLASH_MEMORY_COUNT (2)
/* @brief P-Flash start address. */
#define FSL_FEATURE_FLASH_PFLASH_1_START_ADDRESS (0x00000000)
/* @brief P-Flash block count. */
#define FSL_FEATURE_FLASH_PFLASH_1_BLOCK_COUNT (2)
/* @brief P-Flash block size. */
#define FSL_FEATURE_FLASH_PFLASH_1_BLOCK_SIZE (524288)
/* @brief P-Flash sector size. */
#define FSL_FEATURE_FLASH_PFLASH_1_BLOCK_SECTOR_SIZE (4096)
/* @brief P-Flash write unit size. */
#define FSL_FEATURE_FLASH_PFLASH_1_BLOCK_WRITE_UNIT_SIZE (8)
/* @brief P-Flash data path width. */
#define FSL_FEATURE_FLASH_PFLASH_1_BLOCK_DATA_PATH_WIDTH (16)
/* @brief P-Flash protection region count. */
#define FSL_FEATURE_FLASH_PFLASH_1_PROTECTION_REGION_COUNT (64)
/* @brief P-Flash block swap feature. */
#define FSL_FEATURE_FLASH_HAS_1_PFLASH_BLOCK_SWAP (1)
/* @brief Has FlexNVM memory. */
#define FSL_FEATURE_FLASH_HAS_FLEX_NVM (0)
/* @brief FlexNVM start address. (Valid only if FlexNVM is available.) */
#define FSL_FEATURE_FLASH_FLEX_NVM_START_ADDRESS (0x00000000)
/* @brief FlexNVM block count. */
#define FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_COUNT (0)
/* @brief FlexNVM block size. */
#define FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_SIZE (0)
/* @brief FlexNVM sector size. */
#define FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_SECTOR_SIZE (0)
/* @brief FlexNVM write unit size. */
#define FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_WRITE_UNIT_SIZE (0)
/* @brief FlexNVM data path width. */
#define FSL_FEATURE_FLASH_FLEX_BLOCK_DATA_PATH_WIDTH (0)
/* @brief Has FlexRAM memory. */
#define FSL_FEATURE_FLASH_HAS_FLEX_RAM (1)
/* @brief FlexRAM start address. (Valid only if FlexRAM is available.) */
#define FSL_FEATURE_FLASH_FLEX_RAM_START_ADDRESS (0x48000000)
/* @brief FlexRAM size. */
#define FSL_FEATURE_FLASH_FLEX_RAM_SIZE (4096)
/* @brief Has 0x00 Read 1s Block command. */
#define FSL_FEATURE_FLASH_HAS_READ_1S_BLOCK_CMD (0)
/* @brief Flash 1 has 0x00 Read 1s Block command. */
#define FSL_FEATURE_FLASH_HAS_1_READ_1S_BLOCK_CMD (1)
/* @brief Has 0x01 Read 1s Section command. */
#define FSL_FEATURE_FLASH_HAS_READ_1S_SECTION_CMD (1)
/* @brief Has 0x02 Program Check command. */
#define FSL_FEATURE_FLASH_HAS_PROGRAM_CHECK_CMD (1)
/* @brief Has 0x03 Read Resource command. */
#define FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD (0)
/* @brief Has 0x06 Program Longword command. */
#define FSL_FEATURE_FLASH_HAS_PROGRAM_LONGWORD_CMD (0)
/* @brief Has 0x07 Program Phrase command. */
#define FSL_FEATURE_FLASH_HAS_PROGRAM_PHRASE_CMD (1)
/* @brief Has 0x08 Erase Flash Block command. */
#define FSL_FEATURE_FLASH_HAS_ERASE_FLASH_BLOCK_CMD (0)
/* @brief Flash 1 has 0x08 Erase Flash Block command. */
#define FSL_FEATURE_FLASH_HAS_1_ERASE_FLASH_BLOCK_CMD (1)
/* @brief Has 0x09 Erase Flash Sector command. */
#define FSL_FEATURE_FLASH_HAS_ERASE_FLASH_SECTOR_CMD (1)
/* @brief Has 0x0B Program Section command. */
#define FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD (1)
/* @brief Has 0x0C Generate CRC signature for selected program flash sectors. */
#define FSL_FEATURE_FLASH_HAS_GENERATE_CRC_CMD (1)
/* @brief Has 0x40 Read 1s All Blocks command. */
#define FSL_FEATURE_FLASH_HAS_READ_1S_ALL_BLOCKS_CMD (1)
/* @brief Has 0x41 Read Once command. */
#define FSL_FEATURE_FLASH_HAS_READ_ONCE_CMD (1)
/* @brief Has 0x43 Program Once command. */
#define FSL_FEATURE_FLASH_HAS_PROGRAM_ONCE_CMD (1)
/* @brief Has 0x44 Erase All Blocks command. */
#define FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_CMD (1)
/* @brief Has 0x45 Verify Backdoor Access Key command. */
#define FSL_FEATURE_FLASH_HAS_VERIFY_BACKDOOR_ACCESS_KEY_CMD (1)
/* @brief Has 0x46 Swap Control command. */
#define FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD (0)
/* @brief Flash 1 has 0x46 Swap Control command. */
#define FSL_FEATURE_FLASH_HAS_1_SWAP_CONTROL_CMD (1)
/* @brief Has 0x49 Erase All Blocks Unsecure command. */
#define FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD (1)
/* @brief Has 0x4A Read 1s All Execute-only Segments command. */
#define FSL_FEATURE_FLASH_HAS_READ_1S_ALL_EXECUTE_ONLY_SEGMENTS_CMD (0)
/* @brief Has 0x4B Erase All Execute-only Segments command. */
#define FSL_FEATURE_FLASH_HAS_ERASE_ALL_EXECUTE_ONLY_SEGMENTS_CMD (0)
/* @brief Has 0x80 Program Partition command. */
#define FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD (0)
/* @brief Has 0x81 Set FlexRAM Function command. */
#define FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD (0)
/* @brief P-Flash Erase/Read 1st all block command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_CMD_ADDRESS_ALIGMENT (16)
/* @brief P-Flash Erase sector command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_SECTOR_CMD_ADDRESS_ALIGMENT (8)
/* @brief P-Flash Erase sector command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_1_SECTOR_CMD_ADDRESS_ALIGMENT (16)
/* @brief P-Flash Program/Verify section command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_SECTION_CMD_ADDRESS_ALIGMENT (8)
/* @brief P-Flash Program/Verify section command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_1_SECTION_CMD_ADDRESS_ALIGMENT (16)
/* @brief P-Flash Read resource command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_RESOURCE_CMD_ADDRESS_ALIGMENT (8)
/* @brief P-Flash Program check command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_CHECK_CMD_ADDRESS_ALIGMENT (4)
/* @brief P-Flash Program check command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_SWAP_CONTROL_CMD_ADDRESS_ALIGMENT (0)
/* @brief P-Flash 1 Program check command address alignment. */
#define FSL_FEATURE_FLASH_PFLASH_1_SWAP_CONTROL_CMD_ADDRESS_ALIGMENT (16)
/* @brief FlexNVM Erase/Read 1st all block command address alignment. */
#define FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_CMD_ADDRESS_ALIGMENT (0)
/* @brief FlexNVM Erase sector command address alignment. */
#define FSL_FEATURE_FLASH_FLEX_NVM_SECTOR_CMD_ADDRESS_ALIGMENT (0)
/* @brief FlexNVM Rrogram/Verify section command address alignment. */
#define FSL_FEATURE_FLASH_FLEX_NVM_SECTION_CMD_ADDRESS_ALIGMENT (0)
/* @brief FlexNVM Read resource command address alignment. */
#define FSL_FEATURE_FLASH_FLEX_NVM_RESOURCE_CMD_ADDRESS_ALIGMENT (0)
/* @brief FlexNVM Program check command address alignment. */
#define FSL_FEATURE_FLASH_FLEX_NVM_CHECK_CMD_ADDRESS_ALIGMENT (0)
/* @brief FlexNVM partition code 0000 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0000 (0xFFFFFFFF)
/* @brief FlexNVM partition code 0001 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0001 (0xFFFFFFFF)
/* @brief FlexNVM partition code 0010 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0010 (0xFFFFFFFF)
/* @brief FlexNVM partition code 0011 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0011 (0xFFFFFFFF)
/* @brief FlexNVM partition code 0100 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0100 (0xFFFFFFFF)
/* @brief FlexNVM partition code 0101 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0101 (0xFFFFFFFF)
/* @brief FlexNVM partition code 0110 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0110 (0xFFFFFFFF)
/* @brief FlexNVM partition code 0111 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0111 (0xFFFFFFFF)
/* @brief FlexNVM partition code 1000 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1000 (0xFFFFFFFF)
/* @brief FlexNVM partition code 1001 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1001 (0xFFFFFFFF)
/* @brief FlexNVM partition code 1010 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1010 (0xFFFFFFFF)
/* @brief FlexNVM partition code 1011 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1011 (0xFFFFFFFF)
/* @brief FlexNVM partition code 1100 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1100 (0xFFFFFFFF)
/* @brief FlexNVM partition code 1101 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1101 (0xFFFFFFFF)
/* @brief FlexNVM partition code 1110 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1110 (0xFFFFFFFF)
/* @brief FlexNVM partition code 1111 mapping to data flash size in bytes (0xFFFFFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1111 (0xFFFFFFFF)
/* @brief Emulated eeprom size code 0000 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0000 (0xFFFF)
/* @brief Emulated eeprom size code 0001 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0001 (0xFFFF)
/* @brief Emulated eeprom size code 0010 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0010 (0x1000)
/* @brief Emulated eeprom size code 0011 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0011 (0x0800)
/* @brief Emulated eeprom size code 0100 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0100 (0x0400)
/* @brief Emulated eeprom size code 0101 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0101 (0x0200)
/* @brief Emulated eeprom size code 0110 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0110 (0x0100)
/* @brief Emulated eeprom size code 0111 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0111 (0x0080)
/* @brief Emulated eeprom size code 1000 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1000 (0x0040)
/* @brief Emulated eeprom size code 1001 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1001 (0x0020)
/* @brief Emulated eeprom size code 1010 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1010 (0xFFFF)
/* @brief Emulated eeprom size code 1011 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1011 (0xFFFF)
/* @brief Emulated eeprom size code 1100 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1100 (0xFFFF)
/* @brief Emulated eeprom size code 1101 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1101 (0xFFFF)
/* @brief Emulated eeprom size code 1110 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1110 (0xFFFF)
/* @brief Emulated eeprom size code 1111 mapping to emulated eeprom size in bytes (0xFFFF = reserved). */
#define FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1111 (0x0000)

/* GPIO module features */

/* @brief Has fast (single cycle) access capability via a dedicated memory region. */
#define FSL_FEATURE_GPIO_HAS_FAST_GPIO (1)
/* @brief Has port input disable register (PIDR). */
#define FSL_FEATURE_GPIO_HAS_INPUT_DISABLE (0)
/* @brief Has dedicated interrupt vector. */
#define FSL_FEATURE_GPIO_HAS_PORT_INTERRUPT_VECTOR (1)

/* SAI module features */

/* @brief Receive/transmit FIFO size in item count (register bit fields TCSR[FRDE], TCSR[FRIE], TCSR[FRF], TCR1[TFW], RCSR[FRDE], RCSR[FRIE], RCSR[FRF], RCR1[RFW], registers TFRn, RFRn). */
#define FSL_FEATURE_SAI_FIFO_COUNT (8)
/* @brief Receive/transmit channel number (register bit fields TCR3[TCE], RCR3[RCE], registers TDRn and RDRn). */
#define FSL_FEATURE_SAI_CHANNEL_COUNT (2)
/* @brief Maximum words per frame (register bit fields TCR3[WDFL], TCR4[FRSZ], TMR[TWM], RCR3[WDFL], RCR4[FRSZ], RMR[RWM]). */
#define FSL_FEATURE_SAI_MAX_WORDS_PER_FRAME (32)
/* @brief Has support of combining multiple data channel FIFOs into single channel FIFO (register bit fields TCR3[CFR], TCR4[FCOMB], TFR0[WCP], TFR1[WCP], RCR3[CFR], RCR4[FCOMB], RFR0[RCP], RFR1[RCP]). */
#define FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE (1)
/* @brief Has packing of 8-bit and 16-bit data into each 32-bit FIFO word (register bit fields TCR4[FPACK], RCR4[FPACK]). */
#define FSL_FEATURE_SAI_HAS_FIFO_PACKING (1)
/* @brief Configures when the SAI will continue transmitting after a FIFO error has been detected (register bit fields TCR4[FCONT], RCR4[FCONT]). */
#define FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR (1)
/* @brief Configures if the frame sync is generated internally, a frame sync is only generated when the FIFO warning flag is clear or continuously (register bit fields TCR4[ONDEM], RCR4[ONDEM]). */
#define FSL_FEATURE_SAI_HAS_ON_DEMAND_MODE (1)
/* @brief Simplified bit clock source and asynchronous/synchronous mode selection (register bit fields TCR2[CLKMODE], RCR2[CLKMODE]), in comparison with the exclusively implemented TCR2[SYNC,BCS,BCI,MSEL], RCR2[SYNC,BCS,BCI,MSEL]. */
#define FSL_FEATURE_SAI_HAS_CLOCKING_MODE (0)
/* @brief Has register for configuration of the MCLK divide ratio (register bit fields MDR[FRACT], MDR[DIVIDE]). */
#define FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER (0)
/* @brief Interrupt source number */
#define FSL_FEATURE_SAI_INT_SOURCE_NUM (1)
/* @brief Has register of MCR. */
#define FSL_FEATURE_SAI_HAS_MCR (0)
/* @brief Has bit field MICS of the MCR register. */
#define FSL_FEATURE_SAI_HAS_NO_MCR_MICS (1)
/* @brief Has register of MDR */
#define FSL_FEATURE_SAI_HAS_MDR (0)
/* @brief Has support the BCLK bypass mode when BCLK = MCLK. */
#define FSL_FEATURE_SAI_HAS_BCLK_BYPASS (0)

/* INTMUX module features */

/* @brief Number of INTMUX channels (related to number of register CHn_CSR). */
#define FSL_FEATURE_INTMUX_CHANNEL_COUNT (8)
/* @brief Number of INTMUX IRQ source. */
#define FSL_FEATURE_INTMUX_IRQ_COUNT (32)
/* @brief The start IRQ index of first INTMUX source IRQ. */
#define FSL_FEATURE_INTMUX_IRQ_START_INDEX (32)
/* @brief The direction of INTMUX. OUT, route the CM4 subsystem IRQ to System. */
#define FSL_FEATURE_INTMUX_DIRECTION_OUT (0)
/* @brief The total number of level1 interrupt vectors. */
#define FSL_FEATURE_NUMBER_OF_LEVEL1_INT_VECTORS (32)

/* LLWU module features */

/* @brief Maximum number of pins connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN (32)
/* @brief Maximum number of internal modules connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE (8)
/* @brief Number of digital filters. */
#define FSL_FEATURE_LLWU_HAS_PIN_FILTER (2)
/* @brief Has MF register. */
#define FSL_FEATURE_LLWU_HAS_MF (0)
/* @brief Has PF register. */
#define FSL_FEATURE_LLWU_HAS_PF (1)
/* @brief Has possibility to enable reset in low leakage power mode and enable digital filter for RESET pin (register LLWU_RST). */
#define FSL_FEATURE_LLWU_HAS_RESET_ENABLE (0)
/* @brief Has no internal module wakeup flag register. */
#define FSL_FEATURE_LLWU_HAS_NO_INTERNAL_MODULE_WAKEUP_FLAG_REG (1)
/* @brief Has external pin 0 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN0 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN0_GPIO_IDX (GPIOA_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN0_GPIO_PIN (1)
/* @brief Has external pin 1 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN1 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN1_GPIO_IDX (GPIOA_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN1_GPIO_PIN (2)
/* @brief Has external pin 2 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN2 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN2_GPIO_IDX (GPIOA_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN2_GPIO_PIN (22)
/* @brief Has external pin 3 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN3 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN3_GPIO_IDX (GPIOA_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN3_GPIO_PIN (30)
/* @brief Has external pin 4 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN4 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN4_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN4_GPIO_PIN (1)
/* @brief Has external pin 5 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN5 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN5_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN5_GPIO_PIN (2)
/* @brief Has external pin 6 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN6 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN6_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN6_GPIO_PIN (4)
/* @brief Has external pin 7 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN7 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN7_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN7_GPIO_PIN (6)
/* @brief Has external pin 8 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN8 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN8_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN8_GPIO_PIN (7)
/* @brief Has external pin 9 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN9 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN9_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN9_GPIO_PIN (8)
/* @brief Has external pin 10 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN10 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN10_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN10_GPIO_PIN (16)
/* @brief Has external pin 11 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN11 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN11_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN11_GPIO_PIN (20)
/* @brief Has external pin 12 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN12 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN12_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN12_GPIO_PIN (22)
/* @brief Has external pin 13 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN13 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN13_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN13_GPIO_PIN (25)
/* @brief Has external pin 14 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN14 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN14_GPIO_IDX (GPIOB_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN14_GPIO_PIN (28)
/* @brief Has external pin 15 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN15 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN15_GPIO_IDX (GPIOC_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN15_GPIO_PIN (7)
/* @brief Has external pin 16 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN16 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN16_GPIO_IDX (GPIOC_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN16_GPIO_PIN (9)
/* @brief Has external pin 17 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN17 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN17_GPIO_IDX (GPIOC_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN17_GPIO_PIN (11)
/* @brief Has external pin 18 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN18 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN18_GPIO_IDX (GPIOC_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN18_GPIO_PIN (12)
/* @brief Has external pin 19 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN19 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN19_GPIO_IDX (GPIOD_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN19_GPIO_PIN (8)
/* @brief Has external pin 20 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN20 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN20_GPIO_IDX (GPIOD_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN20_GPIO_PIN (10)
/* @brief Has external pin 21 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN21 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN21_GPIO_IDX (GPIOE_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN21_GPIO_PIN (1)
/* @brief Has external pin 22 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN22 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN22_GPIO_IDX (GPIOE_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN22_GPIO_PIN (3)
/* @brief Has external pin 23 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN23 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN23_GPIO_IDX (GPIOE_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN23_GPIO_PIN (8)
/* @brief Has external pin 24 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN24 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN24_GPIO_IDX (GPIOE_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN24_GPIO_PIN (9)
/* @brief Has external pin 25 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN25 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN25_GPIO_IDX (GPIOE_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN25_GPIO_PIN (10)
/* @brief Has external pin 26 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN26 (1)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN26_GPIO_IDX (GPIOE_IDX)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN26_GPIO_PIN (12)
/* @brief Has external pin 27 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN27 (0)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN27_GPIO_IDX (0)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN27_GPIO_PIN (0)
/* @brief Has external pin 28 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN28 (0)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN28_GPIO_IDX (0)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN28_GPIO_PIN (0)
/* @brief Has external pin 29 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN29 (0)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN29_GPIO_IDX (0)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN29_GPIO_PIN (0)
/* @brief Has external pin 30 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN30 (0)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN30_GPIO_IDX (0)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN30_GPIO_PIN (0)
/* @brief Has external pin 31 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN31 (0)
/* @brief Index of port of external pin. */
#define FSL_FEATURE_LLWU_PIN31_GPIO_IDX (0)
/* @brief Number of external pin port on specified port. */
#define FSL_FEATURE_LLWU_PIN31_GPIO_PIN (0)
/* @brief Has internal module 0 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE0 (1)
/* @brief Has internal module 1 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE1 (1)
/* @brief Has internal module 2 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE2 (1)
/* @brief Has internal module 3 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE3 (1)
/* @brief Has internal module 4 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE4 (1)
/* @brief Has internal module 5 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE5 (1)
/* @brief Has internal module 6 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE6 (1)
/* @brief Has internal module 7 connected to LLWU device. */
#define FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE7 (1)
/* @brief Has LLWU_VERID. */
#define FSL_FEATURE_LLWU_HAS_VERID (1)
/* @brief Has LLWU_PARAM. */
#define FSL_FEATURE_LLWU_HAS_PARAM (1)
/* @brief LLWU register bit width. */
#define FSL_FEATURE_LLWU_REG_BITWIDTH (32)
/* @brief Has DMA Enable register LLWU_DE. */
#define FSL_FEATURE_LLWU_HAS_DMA_ENABLE_REG (1)

/* LPDAC module features */

/* @brief FIFO size. */
#define FSL_FEATURE_LPDAC_FIFO_SIZE (16)

/* LPI2C module features */

/* @brief Has separate DMA RX and TX requests. */
#define FSL_FEATURE_LPI2C_HAS_SEPARATE_DMA_RX_TX_REQn(x) (1)
/* @brief Capacity (number of entries) of the transmit/receive FIFO (or zero if no FIFO is available). */
#define FSL_FEATURE_LPI2C_FIFO_SIZEn(x) (4)

/* LPIT module features */

/* @brief Number of channels (related to number of registers LDVALn, CVALn, TCTRLn, TFLGn). */
#define FSL_FEATURE_LPIT_TIMER_COUNT (4)
/* @brief Has lifetime timer (related to existence of registers LTMR64L and LTMR64H). */
#define FSL_FEATURE_LPIT_HAS_LIFETIME_TIMER (0)
/* @brief Has chain mode (related to existence of register bit field TCTRLn[CHN]). */
#define FSL_FEATURE_LPIT_HAS_CHAIN_MODE (0)
/* @brief Has shared interrupt handler (has not individual interrupt handler for each channel). */
#define FSL_FEATURE_LPIT_HAS_SHARED_IRQ_HANDLER (0)

/* LPSPI module features */

/* @brief Capacity (number of entries) of the transmit/receive FIFO (or zero if no FIFO is available). */
#define FSL_FEATURE_LPSPI_FIFO_SIZEn(x) (4)
/* @brief Has separate DMA RX and TX requests. */
#define FSL_FEATURE_LPSPI_HAS_SEPARATE_DMA_RX_TX_REQn(x) (1)

/* LPTMR module features */

/* @brief Has shared interrupt handler with another LPTMR module. */
#define FSL_FEATURE_LPTMR_HAS_SHARED_IRQ_HANDLER (0)
/* @brief Whether LPTMR counter is 32 bits width. */
#define FSL_FEATURE_LPTMR_CNR_WIDTH_IS_32B (1)
/* @brief Has timer DMA request enable (register bit CSR[TDRE]). */
#define FSL_FEATURE_LPTMR_HAS_CSR_TDRE (1)

/* LPUART module features */

/* @brief Has receive FIFO overflow detection (bit field CFIFO[RXOFE]). */
#define FSL_FEATURE_LPUART_HAS_IRQ_EXTENDED_FUNCTIONS (0)
/* @brief Has low power features (can be enabled in wait mode via register bit C1[DOZEEN] or CTRL[DOZEEN] if the registers are 32-bit wide). */
#define FSL_FEATURE_LPUART_HAS_LOW_POWER_UART_SUPPORT (1)
/* @brief Has extended data register ED (or extra flags in the DATA register if the registers are 32-bit wide). */
#define FSL_FEATURE_LPUART_HAS_EXTENDED_DATA_REGISTER_FLAGS (1)
/* @brief Capacity (number of entries) of the transmit/receive FIFO (or zero if no FIFO is available). */
#define FSL_FEATURE_LPUART_HAS_FIFO (1)
/* @brief Has 32-bit register MODIR */
#define FSL_FEATURE_LPUART_HAS_MODIR (1)
/* @brief Hardware flow control (RTS, CTS) is supported. */
#define FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT (1)
/* @brief Infrared (modulation) is supported. */
#define FSL_FEATURE_LPUART_HAS_IR_SUPPORT (1)
/* @brief 2 bits long stop bit is available. */
#define FSL_FEATURE_LPUART_HAS_STOP_BIT_CONFIG_SUPPORT (1)
/* @brief If 10-bit mode is supported. */
#define FSL_FEATURE_LPUART_HAS_10BIT_DATA_SUPPORT (1)
/* @brief If 7-bit mode is supported. */
#define FSL_FEATURE_LPUART_HAS_7BIT_DATA_SUPPORT (1)
/* @brief Baud rate fine adjustment is available. */
#define FSL_FEATURE_LPUART_HAS_BAUD_RATE_FINE_ADJUST_SUPPORT (0)
/* @brief Baud rate oversampling is available (has bit fields C4[OSR], C5[BOTHEDGE], C5[RESYNCDIS] or BAUD[OSR], BAUD[BOTHEDGE], BAUD[RESYNCDIS] if the registers are 32-bit wide). */
#define FSL_FEATURE_LPUART_HAS_BAUD_RATE_OVER_SAMPLING_SUPPORT (1)
/* @brief Baud rate oversampling is available. */
#define FSL_FEATURE_LPUART_HAS_RX_RESYNC_SUPPORT (1)
/* @brief Baud rate oversampling is available. */
#define FSL_FEATURE_LPUART_HAS_BOTH_EDGE_SAMPLING_SUPPORT (1)
/* @brief Peripheral type. */
#define FSL_FEATURE_LPUART_IS_SCI (1)
/* @brief Capacity (number of entries) of the transmit/receive FIFO (or zero if no FIFO is available). */
#define FSL_FEATURE_LPUART_FIFO_SIZEn(x) (8)
/* @brief Maximal data width without parity bit. */
#define FSL_FEATURE_LPUART_MAX_DATA_WIDTH_WITH_NO_PARITY (10)
/* @brief Maximal data width with parity bit. */
#define FSL_FEATURE_LPUART_MAX_DATA_WIDTH_WITH_PARITY (9)
/* @brief Supports two match addresses to filter incoming frames. */
#define FSL_FEATURE_LPUART_HAS_ADDRESS_MATCHING (1)
/* @brief Has transmitter/receiver DMA enable bits C5[TDMAE]/C5[RDMAE] (or BAUD[TDMAE]/BAUD[RDMAE] if the registers are 32-bit wide). */
#define FSL_FEATURE_LPUART_HAS_DMA_ENABLE (1)
/* @brief Has transmitter/receiver DMA select bits C4[TDMAS]/C4[RDMAS], resp. C5[TDMAS]/C5[RDMAS] if IS_SCI = 0. */
#define FSL_FEATURE_LPUART_HAS_DMA_SELECT (0)
/* @brief Data character bit order selection is supported (bit field S2[MSBF] or STAT[MSBF] if the registers are 32-bit wide). */
#define FSL_FEATURE_LPUART_HAS_BIT_ORDER_SELECT (1)
/* @brief Has smart card (ISO7816 protocol) support and no improved smart card support. */
#define FSL_FEATURE_LPUART_HAS_SMART_CARD_SUPPORT (0)
/* @brief Has improved smart card (ISO7816 protocol) support. */
#define FSL_FEATURE_LPUART_HAS_IMPROVED_SMART_CARD_SUPPORT (0)
/* @brief Has local operation network (CEA709.1-B protocol) support. */
#define FSL_FEATURE_LPUART_HAS_LOCAL_OPERATION_NETWORK_SUPPORT (0)
/* @brief Has 32-bit registers (BAUD, STAT, CTRL, DATA, MATCH, MODIR) instead of 8-bit (BDH, BDL, C1, S1, D, etc.). */
#define FSL_FEATURE_LPUART_HAS_32BIT_REGISTERS (1)
/* @brief Lin break detect available (has bit BAUD[LBKDIE]). */
#define FSL_FEATURE_LPUART_HAS_LIN_BREAK_DETECT (1)
/* @brief UART stops in Wait mode available (has bit C1[UARTSWAI]). */
#define FSL_FEATURE_LPUART_HAS_WAIT_MODE_OPERATION (0)
/* @brief Has separate DMA RX and TX requests. */
#define FSL_FEATURE_LPUART_HAS_SEPARATE_DMA_RX_TX_REQn(x) (1)
/* @brief Has separate RX and TX interrupts. */
#define FSL_FEATURE_LPUART_HAS_SEPARATE_RX_TX_IRQ (0)
/* @brief Has LPAURT_PARAM. */
#define FSL_FEATURE_LPUART_HAS_PARAM (1)
/* @brief Has LPUART_VERID. */
#define FSL_FEATURE_LPUART_HAS_VERID (1)
/* @brief Has LPUART_GLOBAL. */
#define FSL_FEATURE_LPUART_HAS_GLOBAL (1)
/* @brief Has LPUART_PINCFG. */
#define FSL_FEATURE_LPUART_HAS_PINCFG (1)

/* MCM module features */

/* @brief Has L1 cache. */
#define FSL_FEATURE_HAS_L1CACHE (1)

/* MSCM module features */

/* @brief Number of configuration information for processors. */
#define FSL_FEATURE_MSCM_HAS_CP_COUNT (2)
/* @brief Has data cache. */
#define FSL_FEATURE_MSCM_HAS_DATACACHE (0)

/* MU module features */

/* @brief MU side for current core */
#define FSL_FEATURE_MU_SIDE_B (1)
/* @brief MU Has register CCR */
#define FSL_FEATURE_MU_HAS_CCR (1)
/* @brief MU Has register SR[RS], BSR[ARS] */
#define FSL_FEATURE_MU_HAS_SR_RS (0)
/* @brief MU Has register CR[RDIE], CR[RAIE], SR[RDIP], SR[RAIP] */
#define FSL_FEATURE_MU_HAS_RESET_INT (1)
/* @brief MU Has register SR[MURIP] */
#define FSL_FEATURE_MU_HAS_SR_MURIP (1)
/* @brief brief MU Has register SR[HRIP] */
#define FSL_FEATURE_MU_HAS_SR_HRIP (1)
/* @brief brief MU does not support enable clock of the other core, CR[CLKE] or CCR[CLKE]. */
#define FSL_FEATURE_MU_NO_CLKE (0)
/* @brief brief MU does not support NMI, CR[NMI]. */
#define FSL_FEATURE_MU_NO_NMI (0)
/* @brief brief MU does not support hold the other core reset. CR[RSTH] or CCR[RSTH]. */
#define FSL_FEATURE_MU_NO_RSTH (0)
/* @brief brief MU does not supports MU reset, CR[MUR]. */
#define FSL_FEATURE_MU_NO_MUR (0)
/* @brief brief MU does not supports hardware reset, CR[HR] or CCR[HR]. */
#define FSL_FEATURE_MU_NO_HR (0)
/* @brief brief MU supports mask the hardware reset. CR[HRM] or CCR[HRM]. */
#define FSL_FEATURE_MU_HAS_HRM (1)

/* interrupt module features */

/* @brief Lowest interrupt request number. */
#define FSL_FEATURE_INTERRUPT_IRQ_MIN (-14)
/* @brief Highest interrupt request number. */
#define FSL_FEATURE_INTERRUPT_IRQ_MAX (31)

/* PCC module features */

/* @brief Has CLOCK GATE CONTROL bit (e.g PCC_CGC) */
#define FSL_FEATURE_PCC_HAS_FGPIO_CLOCK_GATE_CONTROL (1)

/* PORT module features */

/* @brief Has control lock (register bit PCR[LK]). */
#define FSL_FEATURE_PORT_HAS_PIN_CONTROL_LOCK (1)
/* @brief Has open drain control (register bit PCR[ODE]). */
#define FSL_FEATURE_PORT_HAS_OPEN_DRAIN (1)
/* @brief Has digital filter (registers DFER, DFCR and DFWR). */
#define FSL_FEATURE_PORT_HAS_DIGITAL_FILTER (1)
/* @brief Has DMA request (register bit field PCR[IRQC] or ICR[IRQC] values). */
#define FSL_FEATURE_PORT_HAS_DMA_REQUEST (1)
/* @brief Has pull resistor selection available. */
#define FSL_FEATURE_PORT_HAS_PULL_SELECTION (1)
/* @brief Has pull resistor enable (register bit PCR[PE]). */
#define FSL_FEATURE_PORT_HAS_PULL_ENABLE (1)
/* @brief Has slew rate control (register bit PCR[SRE]). */
#define FSL_FEATURE_PORT_HAS_SLEW_RATE (1)
/* @brief Has passive filter (register bit field PCR[PFE]). */
#define FSL_FEATURE_PORT_HAS_PASSIVE_FILTER (1)
/* @brief Has drive strength control (register bit PCR[DSE]). */
#define FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH (1)
/* @brief Defines width of PCR[MUX] field. */
#define FSL_FEATURE_PORT_PCR_MUX_WIDTH (3)
/* @brief Has dedicated interrupt vector. */
#define FSL_FEATURE_PORT_HAS_INTERRUPT_VECTOR (1)
/* @brief Has independent interrupt control(register ICR). */
#define FSL_FEATURE_PORT_HAS_INDEPENDENT_INTERRUPT_CONTROL (0)
/* @brief Has multiple pin IRQ configuration (register GICLR and GICHR). */
#define FSL_FEATURE_PORT_HAS_MULTIPLE_IRQ_CONFIG (1)
/* @brief Defines whether PCR[IRQC] bit-field has flag states. */
#define FSL_FEATURE_PORT_HAS_IRQC_FLAG (1)
/* @brief Defines whether PCR[IRQC] bit-field has trigger states. */
#define FSL_FEATURE_PORT_HAS_IRQC_TRIGGER (1)

/* RADIO module features */

/* @brief Zigbee availability. */
#define FSL_FEATURE_RADIO_HAS_ZIGBEE (1)
/* @brief Bluetooth availability. */
#define FSL_FEATURE_RADIO_HAS_BLE (1)
/* @brief ANT availability */
#define FSL_FEATURE_RADIO_HAS_ANT (0)
/* @brief Generic FSK module availability */
#define FSL_FEATURE_RADIO_HAS_GENFSK (1)
/* @brief Major version of the radio submodule */
#define FSL_FEATURE_RADIO_VERSION_MAJOR (3)
/* @brief Minor version of the radio submodule */
#define FSL_FEATURE_RADIO_VERSION_MINOR (0)

/* RTC module features */

/* @brief Has wakeup pin. */
#define FSL_FEATURE_RTC_HAS_WAKEUP_PIN (1)
/* @brief Has wakeup pin selection (bit field CR[WPS]). */
#define FSL_FEATURE_RTC_HAS_WAKEUP_PIN_SELECTION (1)
/* @brief Has low power features (registers MER, MCLR and MCHR). */
#define FSL_FEATURE_RTC_HAS_MONOTONIC (1)
/* @brief Has read/write access control (registers WAR and RAR). */
#define FSL_FEATURE_RTC_HAS_ACCESS_CONTROL (1)
/* @brief Has security features (registers TTSR, MER, MCLR and MCHR). */
#define FSL_FEATURE_RTC_HAS_SECURITY (1)
/* @brief Has RTC_CLKIN available. */
#define FSL_FEATURE_RTC_HAS_RTC_CLKIN (0)
/* @brief Has prescaler adjust for LPO. */
#define FSL_FEATURE_RTC_HAS_LPO_ADJUST (1)
/* @brief Has Clock Pin Enable field. */
#define FSL_FEATURE_RTC_HAS_CPE (1)
/* @brief Has Timer Seconds Interrupt Configuration field. */
#define FSL_FEATURE_RTC_HAS_TSIC (1)
/* @brief Has OSC capacitor setting RTC_CR[SC2P ~ SC16P] */
#define FSL_FEATURE_RTC_HAS_OSC_SCXP (1)
/* @brief Has Tamper Interrupt Register (register TIR). */
#define FSL_FEATURE_RTC_HAS_TIR (1)
/* @brief Has Tamper Pin Interrupt Enable (bitfield TIR[TPIE]). */
#define FSL_FEATURE_RTC_HAS_TIR_TPIE (1)
/* @brief Has Security Interrupt Enable (bitfield TIR[SIE]). */
#define FSL_FEATURE_RTC_HAS_TIR_SIE (1)
/* @brief Has Loss of Clock Interrupt Enable (bitfield TIR[LCIE]). */
#define FSL_FEATURE_RTC_HAS_TIR_LCIE (1)
/* @brief Has Tamper Interrupt Detect Flag (bitfield SR[TIDF]). */
#define FSL_FEATURE_RTC_HAS_SR_TIDF (1)
/* @brief Has Tamper Detect Register (register TDR). */
#define FSL_FEATURE_RTC_HAS_TDR (1)
/* @brief Has Tamper Pin Flag (bitfield TDR[TPF]). */
#define FSL_FEATURE_RTC_HAS_TDR_TPF (1)
/* @brief Has Security Tamper Flag (bitfield TDR[STF]). */
#define FSL_FEATURE_RTC_HAS_TDR_STF (1)
/* @brief Has Loss of Clock Tamper Flag (bitfield TDR[LCTF]). */
#define FSL_FEATURE_RTC_HAS_TDR_LCTF (1)
/* @brief Has Tamper Time Seconds Register (register TTSR). */
#define FSL_FEATURE_RTC_HAS_TTSR (1)
/* @brief Has Pin Configuration Register (register PCR). */
#define FSL_FEATURE_RTC_HAS_PCR (1)
/* @brief Has Oscillator Enable(bitfield CR[OSCE]). */
#define FSL_FEATURE_RTC_HAS_NO_CR_OSCE (0)

/* SCG module features */

/* @brief Has platform clock divider SCG_CSR[DIVPLAT]. */
#define FSL_FEATURE_SCG_HAS_DIVPLAT (0)
/* @brief Has bus clock divider SCG_CSR[DIVBUS]. */
#define FSL_FEATURE_SCG_HAS_DIVBUS (1)
/* @brief Has external clock divide ratio SCG_CSR[DIVEXT]. */
#define FSL_FEATURE_SCG_HAS_DIVEXT (1)
/* @brief Has OSC capacitor setting SOSCCFG[SC2P ~ SC16P]. */
#define FSL_FEATURE_SCG_HAS_OSC_SCXP (0)
/* @brief Has SOSCCSR[SOSCERCLKEN]. */
#define FSL_FEATURE_SCG_HAS_OSC_ERCLK (0)
/* @brief Has OSC freq range SOSCCFG[RANGE]. */
#define FSL_FEATURE_SCG_HAS_SOSC_RANGE (0)
/* @brief Has CLKOUT configure register SCG_CLKOUTCNFG. */
#define FSL_FEATURE_SCG_HAS_CLKOUTCNFG (1)
/* @brief Has SCG_SOSCDIV[SOSCDIV3]. */
#define FSL_FEATURE_SCG_HAS_SOSCDIV3 (1)
/* @brief Has SCG_SIRCDIV[SIRCDIV3]. */
#define FSL_FEATURE_SCG_HAS_SIRCDIV3 (1)
/* @brief Has SCG_SIRCCSR[LPOPO]. */
#define FSL_FEATURE_SCG_HAS_SIRC_LPOPO (0)
/* @brief Has SCG_FIRCDIV[FIRCDIV3]. */
#define FSL_FEATURE_SCG_HAS_FIRCDIV3 (1)
/* @brief Has SCG_FIRCCSR[FIRCLPEN]. */
#define FSL_FEATURE_SCG_HAS_FIRCLPEN (1)
/* @brief Has SCG_FIRCCSR[FIRCREGOFF]. */
#define FSL_FEATURE_SCG_HAS_FIRCREGOFF (1)
/* @brief Has SCG_SPLLDIV[SPLLDIV3]. */
#define FSL_FEATURE_SCG_HAS_SPLLDIV3 (0)
/* @brief Has SCG_SPLLCFG[PLLPOSTDIV1]. */
#define FSL_FEATURE_SCG_HAS_SPLLPOSTDIV1 (0)
/* @brief Has SCG_SPLLCFG[PLLPOSTDIV2]. */
#define FSL_FEATURE_SCG_HAS_SPLLPOSTDIV2 (0)
/* @brief Has SCG_SPLLCFG[PLLS]. */
#define FSL_FEATURE_SCG_HAS_SPLL_PLLS (0)
/* @brief Has SCG_SPLLCFG[BYPASS]. */
#define FSL_FEATURE_SCG_HAS_SPLL_BYPASS (0)
/* @brief Has SCG_SPLLCFG[PFDSEL]. */
#define FSL_FEATURE_SCG_HAS_SPLL_PFDSEL (0)
/* @brief Has SCG_SPLLCSR[SPLLCM]. */
#define FSL_FEATURE_SCG_HAS_SPLL_MONITOR (0)
/* @brief Has SCG_LPFLLDIV[FLLDIV3]. */
#define FSL_FEATURE_SCG_HAS_FLLDIV3 (1)
/* @brief Has low power FLL, SCG_LPFLLCSR. */
#define FSL_FEATURE_SCG_HAS_LPFLL (1)
/* @brief Has system PLL, SCG_SPLLCSR. */
#define FSL_FEATURE_SCG_HAS_SPLL (0)
/* @brief Has system PLL PFD, SCG_SPLLPFD. */
#define FSL_FEATURE_SCG_HAS_SPLLPFD (0)
/* @brief Has auxiliary PLL, SCG_APLLCSR. */
#define FSL_FEATURE_SCG_HAS_APLL (0)
/* @brief Has RTC OSC control, SCG_ROSCCSR. */
#define FSL_FEATURE_SCG_HAS_ROSC (1)
/* @brief Has RTC OSC clock source. */
#define FSL_FEATURE_SCG_HAS_ROSC_SYS_CLK_SRC (1)
/* @brief Has RTC OSC clock out select. */
#define FSL_FEATURE_SCG_HAS_ROSC_CLKOUT (1)
/* @brief Has EXTERNAL clock out select. */
#define FSL_FEATURE_SCG_HAS_EXT_CLKOUT (1)
/* @brief Has no System OSC configuration register, SCG_SOSCCFG. */
#define FSL_FEATURE_SCG_HAS_NO_SOSCCFG (1)
/* @brief Has no SCG_SOSCCSR[SOSCEN]. */
#define FSL_FEATURE_SCG_HAS_NO_SOSCCSR_SOSCEN (0)
/* @brief Has no SCG_SOSCCSR[SOSCSTEN]. */
#define FSL_FEATURE_SCG_HAS_NO_SOSCCSR_SOSCSTEN (0)
/* @brief Has no SCG_SOSCCSR[SOSCLPEN]. */
#define FSL_FEATURE_SCG_HAS_NO_SOSCCSR_SOSCLPEN (0)
/* @brief Has no FIRC trim configuration register, SCG_FIRCTCFG. */
#define FSL_FEATURE_SCG_HAS_NO_FIRCTCFG (0)
/* @brief Has FIRC trim source USB0 Start of Frame. */
#define FSL_FEATURE_SCG_HAS_FIRC_TRIMSRC_USB0 (0)
/* @brief Has FIRC trim source USB1 Start of Frame. */
#define FSL_FEATURE_SCG_HAS_FIRC_TRIMSRC_USB1 (0)
/* @brief Has FIRC trim source system OSC. */
#define FSL_FEATURE_SCG_HAS_FIRC_TRIMSRC_SOSC (1)
/* @brief Has FIRC trim source RTC OSC. */
#define FSL_FEATURE_SCG_HAS_FIRC_TRIMSRC_RTCOSC (1)

/* SEMA42 module features */

/* @brief Gate counts */
#define FSL_FEATURE_SEMA42_GATE_COUNT (16)

/* SIM module features */

/* @brief Has USB FS divider. */
#define FSL_FEATURE_SIM_USBFS_USE_SPECIAL_DIVIDER (0)
/* @brief Is PLL clock divided by 2 before MCG PLL/FLL clock selection. */
#define FSL_FEATURE_SIM_PLLCLK_USE_SPECIAL_DIVIDER (0)
/* @brief Has RAM size specification (register bit field SOPT1[RAMSIZE]). */
#define FSL_FEATURE_SIM_OPT_HAS_RAMSIZE (0)
/* @brief Has 32k oscillator clock output (register bit SOPT1[OSC32KOUT]). */
#define FSL_FEATURE_SIM_OPT_HAS_OSC32K_OUT (0)
/* @brief Has 32k oscillator clock selection (register bit field SOPT1[OSC32KSEL]). */
#define FSL_FEATURE_SIM_OPT_HAS_OSC32K_SELECTION (0)
/* @brief 32k oscillator clock selection width (width of register bit field SOPT1[OSC32KSEL]). */
#define FSL_FEATURE_SIM_OPT_OSC32K_SELECTION_WIDTH (0)
/* @brief Has RTC clock output selection (register bit SOPT2[RTCCLKOUTSEL]). */
#define FSL_FEATURE_SIM_OPT_HAS_RTC_CLOCK_OUT_SELECTION (0)
/* @brief Has USB voltage regulator (register bits SOPT1[USBVSTBY], SOPT1[USBSSTBY], SOPT1[USBREGEN], SOPT1CFG[URWE], SOPT1CFG[UVSWE], SOPT1CFG[USSWE]). */
#define FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR (0)
/* @brief USB has integrated PHY (register bits USBPHYCTL[USBVREGSEL], USBPHYCTL[USBVREGPD], USBPHYCTL[USB3VOUTTRG], USBPHYCTL[USBDISILIM], SOPT2[USBSLSRC], SOPT2[USBREGEN]). */
#define FSL_FEATURE_SIM_OPT_HAS_USB_PHY (0)
/* @brief Has PTD7 pad drive strength control (register bit SOPT2[PTD7PAD]). */
#define FSL_FEATURE_SIM_OPT_HAS_PTD7PAD (0)
/* @brief Has FlexBus security level selection (register bit SOPT2[FBSL]). */
#define FSL_FEATURE_SIM_OPT_HAS_FBSL (0)
/* @brief Has number of FlexBus hold cycle before FlexBus can release bus (register bit SOPT6[PCR]). */
#define FSL_FEATURE_SIM_OPT_HAS_PCR (0)
/* @brief Has number of NFC hold cycle in case of FlexBus request (register bit SOPT6[MCC]). */
#define FSL_FEATURE_SIM_OPT_HAS_MCC (0)
/* @brief Has UART open drain enable (register bits UARTnODE, where n is a number, in register SOPT5). */
#define FSL_FEATURE_SIM_OPT_HAS_ODE (0)
/* @brief Number of LPUART modules (number of register bits LPUARTn, where n is a number, in register SCGC5). */
#define FSL_FEATURE_SIM_OPT_LPUART_COUNT (0)
/* @brief Number of UART modules (number of register bits UARTn, where n is a number, in register SCGC4). */
#define FSL_FEATURE_SIM_OPT_UART_COUNT (0)
/* @brief Has UART0 open drain enable (register bit SOPT5[UART0ODE]). */
#define FSL_FEATURE_SIM_OPT_HAS_UART0_ODE (0)
/* @brief Has UART1 open drain enable (register bit SOPT5[UART1ODE]). */
#define FSL_FEATURE_SIM_OPT_HAS_UART1_ODE (0)
/* @brief Has UART2 open drain enable (register bit SOPT5[UART2ODE]). */
#define FSL_FEATURE_SIM_OPT_HAS_UART2_ODE (0)
/* @brief Has LPUART0 open drain enable (register bit SOPT5[LPUART0ODE]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUART0_ODE (0)
/* @brief Has LPUART1 open drain enable (register bit SOPT5[LPUART1ODE]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUART1_ODE (0)
/* @brief Has CMT/UART pad drive strength control (register bit SOPT2[CMTUARTPAD]). */
#define FSL_FEATURE_SIM_OPT_HAS_CMTUARTPAD (0)
/* @brief Has LPUART0 transmit data source selection (register bit SOPT5[LPUART0TXSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUART0_TX_SRC (0)
/* @brief Has LPUART0 receive data source selection (register bit SOPT5[LPUART0RXSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUART0_RX_SRC (0)
/* @brief Has LPUART1 transmit data source selection (register bit SOPT5[LPUART1TXSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUART1_TX_SRC (0)
/* @brief Has LPUART1 receive data source selection (register bit SOPT5[LPUART1RXSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUART1_RX_SRC (0)
/* @brief Has UART0 transmit data source selection (register bit SOPT5[UART0TXSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_UART0_TX_SRC (0)
/* @brief UART0 transmit data source selection width (width of register bit SOPT5[UART0TXSRC]). */
#define FSL_FEATURE_SIM_OPT_UART0_TX_SRC_WIDTH (0)
/* @brief Has UART0 receive data source selection (register bit SOPT5[UART0RXSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_UART0_RX_SRC (0)
/* @brief UART0 receive data source selection width (width of register bit SOPT5[UART0RXSRC]). */
#define FSL_FEATURE_SIM_OPT_UART0_RX_SRC_WIDTH (0)
/* @brief Has UART1 transmit data source selection (register bit SOPT5[UART1TXSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_UART1_TX_SRC (0)
/* @brief Has UART1 receive data source selection (register bit SOPT5[UART1RXSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_UART1_RX_SRC (0)
/* @brief UART1 receive data source selection width (width of register bit SOPT5[UART1RXSRC]). */
#define FSL_FEATURE_SIM_OPT_UART1_RX_SRC_WIDTH (0)
/* @brief Has FTM module(s) configuration. */
#define FSL_FEATURE_SIM_OPT_HAS_FTM (0)
/* @brief Number of FTM modules. */
#define FSL_FEATURE_SIM_OPT_FTM_COUNT (0)
/* @brief Number of FTM triggers with selectable source. */
#define FSL_FEATURE_SIM_OPT_FTM_TRIGGER_COUNT (0)
/* @brief Has FTM0 triggers source selection (register bits SOPT4[FTM0TRGnSRC], where n is a number). */
#define FSL_FEATURE_SIM_OPT_HAS_FTM0_TRIGGER (0)
/* @brief Has FTM3 triggers source selection (register bits SOPT4[FTM3TRGnSRC], where n is a number). */
#define FSL_FEATURE_SIM_OPT_HAS_FTM3_TRIGGER (0)
/* @brief Has FTM1 channel 0 input capture source selection (register bit SOPT4[FTM1CH0SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_FTM1_CHANNELS (0)
/* @brief Has FTM2 channel 0 input capture source selection (register bit SOPT4[FTM2CH0SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_FTM2_CHANNELS (0)
/* @brief Has FTM3 channel 0 input capture source selection (register bit SOPT4[FTM3CH0SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_FTM3_CHANNELS (0)
/* @brief Has FTM2 channel 1 input capture source selection (register bit SOPT4[FTM2CH1SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_FTM2_CHANNEL1 (0)
/* @brief Number of configurable FTM0 fault detection input (number of register bits SOPT4[FTM0FLTn], where n is a number starting from zero). */
#define FSL_FEATURE_SIM_OPT_FTM0_FAULT_COUNT (0)
/* @brief Number of configurable FTM1 fault detection input (number of register bits SOPT4[FTM1FLTn], where n is a number starting from zero). */
#define FSL_FEATURE_SIM_OPT_FTM1_FAULT_COUNT (0)
/* @brief Number of configurable FTM2 fault detection input (number of register bits SOPT4[FTM2FLTn], where n is a number starting from zero). */
#define FSL_FEATURE_SIM_OPT_FTM2_FAULT_COUNT (0)
/* @brief Number of configurable FTM3 fault detection input (number of register bits SOPT4[FTM3FLTn], where n is a number starting from zero). */
#define FSL_FEATURE_SIM_OPT_FTM3_FAULT_COUNT (0)
/* @brief Has FTM hardware trigger 0 software synchronization (register bit SOPT8[FTMnSYNCBIT], where n is a module instance index). */
#define FSL_FEATURE_SIM_OPT_HAS_FTM_TRIGGER_SYNC (0)
/* @brief Has FTM channels output source selection (register bit SOPT8[FTMxOCHnSRC], where x is a module instance index and n is a channel index). */
#define FSL_FEATURE_SIM_OPT_HAS_FTM_CHANNELS_OUTPUT_SRC (0)
/* @brief Has TPM module(s) configuration. */
#define FSL_FEATURE_SIM_OPT_HAS_TPM (0)
/* @brief The highest TPM module index. */
#define FSL_FEATURE_SIM_OPT_MAX_TPM_INDEX (0)
/* @brief Has TPM module with index 0. */
#define FSL_FEATURE_SIM_OPT_HAS_TPM0 (0)
/* @brief Has TPM0 clock selection (register bit field SOPT4[TPM0CLKSEL]). */
#define FSL_FEATURE_SIM_OPT_HAS_TPM0_CLK_SEL (0)
/* @brief Is TPM channels configuration in the SOPT4 (not SOPT9) register (register bits TPMnCH0SRC, TPMnCLKSEL, where n is a module instance index). */
#define FSL_FEATURE_SIM_OPT_HAS_TPM_CHANNELS_CONFIG_IN_SOPT4_REG (0)
/* @brief Has TPM1 channel 0 input capture source selection (register bit field SOPT4[TPM1CH0SRC] or SOPT9[TPM1CH0SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_TPM1_CH0_SRC_SELECTION (0)
/* @brief Has TPM1 clock selection (register bit field SOPT4[TPM1CLKSEL]). */
#define FSL_FEATURE_SIM_OPT_HAS_TPM1_CLK_SEL (0)
/* @brief TPM1 channel 0 input capture source selection width (width of register bit field SOPT4[TPM1CH0SRC] or SOPT9[TPM1CH0SRC]). */
#define FSL_FEATURE_SIM_OPT_TPM1_CH0_SRC_SELECTION_WIDTH (0)
/* @brief Has TPM2 channel 0 input capture source selection (register bit field SOPT4[TPM2CH0SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_TPM2_CH0_SRC_SELECTION (0)
/* @brief Has TPM2 clock selection (register bit field SOPT4[TPM2CLKSEL]). */
#define FSL_FEATURE_SIM_OPT_HAS_TPM2_CLK_SEL (0)
/* @brief Has PLL/FLL clock selection (register bit field SOPT2[PLLFLLSEL]). */
#define FSL_FEATURE_SIM_OPT_HAS_PLL_FLL_SELECTION (0)
/* @brief PLL/FLL clock selection width (width of register bit field SOPT2[PLLFLLSEL]). */
#define FSL_FEATURE_SIM_OPT_PLL_FLL_SELECTION_WIDTH (0)
/* @brief Has NFC clock source selection (register bit SOPT2[NFCSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_NFCSRC (0)
/* @brief Has eSDHC clock source selection (register bit SOPT2[ESDHCSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_ESDHCSRC (0)
/* @brief Has SDHC clock source selection (register bit SOPT2[SDHCSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_SDHCSRC (0)
/* @brief Has LCDC clock source selection (register bits SOPT2[LCDCSRC], SOPT2[LCDC_CLKSEL]). */
#define FSL_FEATURE_SIM_OPT_HAS_LCDCSRC (0)
/* @brief Has ENET timestamp clock source selection (register bit SOPT2[TIMESRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_TIMESRC (0)
/* @brief Has ENET RMII clock source selection (register bit SOPT2[RMIISRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_RMIISRC (0)
/* @brief Has USB clock source selection (register bit SOPT2[USBSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_USBSRC (0)
/* @brief Has USB FS clock source selection (register bit SOPT2[USBFSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_USBFSRC (0)
/* @brief Has USB HS clock source selection (register bit SOPT2[USBHSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_USBHSRC (0)
/* @brief Has LPUART clock source selection (register bit SOPT2[LPUARTSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUARTSRC (0)
/* @brief Has LPUART0 clock source selection (register bit SOPT2[LPUART0SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUART0SRC (0)
/* @brief Has LPUART1 clock source selection (register bit SOPT2[LPUART1SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_LPUART1SRC (0)
/* @brief Has FLEXIOSRC clock source selection (register bit SOPT2[FLEXIOSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_FLEXIOSRC (0)
/* @brief Has UART0 clock source selection (register bit SOPT2[UART0SRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_UART0SRC (0)
/* @brief Has TPM clock source selection (register bit SOPT2[TPMSRC]). */
#define FSL_FEATURE_SIM_OPT_HAS_TPMSRC (0)
/* @brief Has debug trace clock selection (register bit SOPT2[TRACECLKSEL]). */
#define FSL_FEATURE_SIM_OPT_HAS_TRACE_CLKSEL (0)
/* @brief Number of ADC modules (register bits SOPT7[ADCnTRGSEL], SOPT7[ADCnPRETRGSEL], SOPT7[ADCnALTTRGSEL], where n is a module instance index). */
#define FSL_FEATURE_SIM_OPT_ADC_COUNT (0)
/* @brief ADC module has alternate trigger (register bit SOPT7[ADC0ALTTRGEN]). */
#define FSL_FEATURE_SIM_OPT_ADC_HAS_ALTERNATE_TRIGGER (0)
/* @brief ADC0 alternate trigger enable width (width of bit field ADC0ALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_ADC0ALTTRGEN_WIDTH (0)
/* @brief ADC1 alternate trigger enable width (width of bit field ADC1ALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_ADC1ALTTRGEN_WIDTH (0)
/* @brief ADC2 alternate trigger enable width (width of bit field ADC2ALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_ADC2ALTTRGEN_WIDTH (0)
/* @brief ADC3 alternate trigger enable width (width of bit field ADC3ALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_ADC3ALTTRGEN_WIDTH (0)
/* @brief HSADC0 converter A alternate trigger enable width (width of bit field HSADC0AALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_HSADC0AALTTRGEN_WIDTH (0)
/* @brief HSADC1 converter A alternate trigger enable width (width of bit field HSADC1AALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_HSADC1AALTTRGEN_WIDTH (0)
/* @brief ADC converter A alternate trigger enable width (width of bit field ADCAALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_ADCAALTTRGEN_WIDTH (0)
/* @brief HSADC0 converter B alternate trigger enable width (width of bit field HSADC0BALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_HSADC0BALTTRGEN_WIDTH (0)
/* @brief HSADC1 converter B alternate trigger enable width (width of bit field HSADC1BALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_HSADC1BALTTRGEN_WIDTH (0)
/* @brief ADC converter B alternate trigger enable width (width of bit field ADCBALTTRGEN of register SOPT7). */
#define FSL_FEATURE_SIM_OPT_ADCBALTTRGEN_WIDTH (0)
/* @brief Has clock 2 output divider (register bit field CLKDIV1[OUTDIV2]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_OUTDIV2 (0)
/* @brief Has clock 3 output divider (register bit field CLKDIV1[OUTDIV3]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_OUTDIV3 (0)
/* @brief Has clock 4 output divider (register bit field CLKDIV1[OUTDIV4]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_OUTDIV4 (0)
/* @brief Clock 4 output divider width (width of register bit field CLKDIV1[OUTDIV4]). */
#define FSL_FEATURE_SIM_DIVIDER_OUTDIV4_WIDTH (0)
/* @brief Has clock 5 output divider (register bit field CLKDIV1[OUTDIV5]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_OUTDIV5 (0)
/* @brief Has USB clock divider (register bit field CLKDIV2[USBDIV] and CLKDIV2[USBFRAC]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_USBDIV (0)
/* @brief Has USB FS clock divider (register bit field CLKDIV2[USBFSDIV] and CLKDIV2[USBFSFRAC]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_USBFSDIV (0)
/* @brief Has USB HS clock divider (register bit field CLKDIV2[USBHSDIV] and CLKDIV2[USBHSFRAC]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_USBHSDIV (0)
/* @brief Has PLL/FLL clock divider (register bit field CLKDIV3[PLLFLLDIV] and CLKDIV3[PLLFLLFRAC]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_PLLFLLDIV (0)
/* @brief Has LCDC clock divider (register bit field CLKDIV3[LCDCDIV] and CLKDIV3[LCDCFRAC]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_LCDCDIV (0)
/* @brief Has trace clock divider (register bit field CLKDIV4[TRACEDIV] and CLKDIV4[TRACEFRAC]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_TRACEDIV (0)
/* @brief Has NFC clock divider (register bit field CLKDIV4[NFCDIV] and CLKDIV4[NFCFRAC]). */
#define FSL_FEATURE_SIM_DIVIDER_HAS_NFCDIV (0)
/* @brief Has Kinetis family ID (register bit field SDID[FAMILYID]). */
#define FSL_FEATURE_SIM_SDID_HAS_FAMILYID (0)
/* @brief Has Kinetis family ID (register bit field SDID[FAMID]). */
#define FSL_FEATURE_SIM_SDID_HAS_FAMID (1)
/* @brief Has Kinetis sub-family ID (register bit field SDID[SUBFAMID]). */
#define FSL_FEATURE_SIM_SDID_HAS_SUBFAMID (1)
/* @brief Has Kinetis series ID (register bit field SDID[SERIESID]). */
#define FSL_FEATURE_SIM_SDID_HAS_SERIESID (1)
/* @brief Has device die ID (register bit field SDID[DIEID]). */
#define FSL_FEATURE_SIM_SDID_HAS_DIEID (1)
/* @brief Has system SRAM size specifier (register bit field SDID[SRAMSIZE]). */
#define FSL_FEATURE_SIM_SDID_HAS_SRAMSIZE (0)
/* @brief Has flash mode (register bit FCFG1[FLASHDOZE]). */
#define FSL_FEATURE_SIM_FCFG_HAS_FLASHDOZE (1)
/* @brief Has flash disable (register bit FCFG1[FLASHDIS]). */
#define FSL_FEATURE_SIM_FCFG_HAS_FLASHDIS (1)
/* @brief Has FTFE disable (register bit FCFG1[FTFDIS]). */
#define FSL_FEATURE_SIM_FCFG_HAS_FTFDIS (0)
/* @brief Has FlexNVM size specifier (register bit field FCFG1[NVMSIZE]). */
#define FSL_FEATURE_SIM_FCFG_HAS_NVMSIZE (0)
/* @brief Has EEPROM size specifier (register bit field FCFG1[EESIZE]). */
#define FSL_FEATURE_SIM_FCFG_HAS_EESIZE (0)
/* @brief Has FlexNVM partition (register bit field FCFG1[DEPART]). */
#define FSL_FEATURE_SIM_FCFG_HAS_DEPART (0)
/* @brief Has flash for core0(CM4) (register bit field FCFG1[CORE0_PFSIZE]). */
#define FSL_FEATURE_SIM_FCFG_HAS_CORE0_PFSIZE (1)
/* @brief Has flash for core1(CM0) (register bit field FCFG1[CORE1_PFSIZE]). */
#define FSL_FEATURE_SIM_FCFG_HAS_CORE1_PFSIZE (1)
/* @brief Has sram for core0(CM4) (register bit field FCFG1[CORE0_SRAMSIZE]). */
#define FSL_FEATURE_SIM_FCFG_HAS_CORE0_SRAMSIZE (1)
/* @brief Has sram for core1(CM0) (register bit field FCFG1[CORE1_SRAMSIZE]). */
#define FSL_FEATURE_SIM_FCFG_HAS_CORE1_SRAMSIZE (1)
/* @brief Maximum flash address block 0 address specifier (register bit field FCFG2[MAXADDR0]). */
#define FSL_FEATURE_SIM_FCFG_HAS_MAXADDR0 (0)
/* @brief Maximum flash address block 1 address specifier (register bit field FCFG2[MAXADDR1]). */
#define FSL_FEATURE_SIM_FCFG_HAS_MAXADDR1 (0)
/* @brief Maximum flash address block 0 or 1 address specifier (register bit field FCFG2[MAXADDR01]). */
#define FSL_FEATURE_SIM_FCFG_HAS_MAXADDR01 (1)
/* @brief Maximum flash address block 2 or 3 address specifier (register bit field FCFG2[MAXADDR23]). */
#define FSL_FEATURE_SIM_FCFG_HAS_MAXADDR23 (0)
/* @brief Has program flash availability specifier (register bit FCFG2[PFLSH]). */
#define FSL_FEATURE_SIM_FCFG_HAS_PFLSH (0)
/* @brief Has program flash swapping (register bit FCFG2[SWAPPFLSH]). */
#define FSL_FEATURE_SIM_FCFG_HAS_PFLSH_SWAP (0)
/* @brief Has miscellanious control register (register MCR). */
#define FSL_FEATURE_SIM_HAS_MISC_CONTROLS (0)
/* @brief Has COP watchdog (registers COPC and SRVCOP). */
#define FSL_FEATURE_SIM_HAS_COP_WATCHDOG (0)
/* @brief Has COP watchdog stop (register bits COPC[COPSTPEN], COPC[COPDBGEN] and COPC[COPCLKSEL]). */
#define FSL_FEATURE_SIM_HAS_COP_STOP (0)
/* @brief Has LLWU clock gate bit (e.g SIM_SCGC4). */
#define FSL_FEATURE_SIM_HAS_SCGC_LLWU (0)
/* @brief Has MISCCTRL reg. */
#define FSL_FEATURE_SIM_HAS_MISCCTRL (0)
/* @brief Has LTCEN bit (e.g SIM_MISCCTRL). */
#define FSL_FEATURE_SIM_HAS_MISCCTRL_LTCEN (0)
/* @brief Has DMAINTSEL0 bit (e.g SIM_MISCCTRL). */
#define FSL_FEATURE_SIM_HAS_MISCCTRL_DMAINTSEL0 (0)
/* @brief Has DMAINTSEL1 bit (e.g SIM_MISCCTRL). */
#define FSL_FEATURE_SIM_HAS_MISCCTRL_DMAINTSEL1 (0)
/* @brief Has DMAINTSEL2 bit (e.g SIM_MISCCTRL). */
#define FSL_FEATURE_SIM_HAS_MISCCTRL_DMAINTSEL2 (0)
/* @brief Has DMAINTSEL3 bit (e.g SIM_MISCCTRL). */
#define FSL_FEATURE_SIM_HAS_MISCCTRL_DMAINTSEL3 (0)
/* @brief Has SECKEY0 reg. */
#define FSL_FEATURE_SIM_HAS_SECKEY0 (0)
/* @brief Has SECKEY bit (e.g SIM_SECKEY0). */
#define FSL_FEATURE_SIM_HAS_SECKEY0_SECKEY (0)
/* @brief Has SECKEY1 reg. */
#define FSL_FEATURE_SIM_HAS_SECKEY1 (0)
/* @brief Has SECKEY bit (e.g SIM_SECKEY1). */
#define FSL_FEATURE_SIM_HAS_SECKEY1_SECKEY (0)
/* @brief Has SECKEY2 reg. */
#define FSL_FEATURE_SIM_HAS_SECKEY2 (0)
/* @brief Has SECKEY bit (e.g SIM_SECKEY2). */
#define FSL_FEATURE_SIM_HAS_SECKEY2_SECKEY (0)
/* @brief Has SECKEY3 reg. */
#define FSL_FEATURE_SIM_HAS_SECKEY3 (0)
/* @brief Has SECKEY bit (e.g SIM_SECKEY3). */
#define FSL_FEATURE_SIM_HAS_SECKEY3_SECKEY (0)
/* @brief Has no SDID reg. */
#define FSL_FEATURE_SIM_HAS_NO_SDID (0)
/* @brief Has no UID reg. */
#define FSL_FEATURE_SIM_HAS_NO_UID (0)
/* @brief Has RFADDRL and RFADDRH registers. */
#define FSL_FEATURE_SIM_HAS_RF_MAC_ADDR (1)
/* @brief Has SYSTICK_CLK_EN bit in SIM_MISC2 register. */
#define FSL_FEATURE_SIM_MISC2_HAS_SYSTICK_CLK_EN (1)
/* @brief Has UIDM registers. */
#define FSL_FEATURE_SIM_HAS_UIDM (1)

/* SMC module features */

/* @brief Has partial stop option (register bit STOPCTRL[PSTOPO]). */
#define FSL_FEATURE_SMC_HAS_PSTOPO (0)
/* @brief Has LPO power option (register bit STOPCTRL[LPOPO]). */
#define FSL_FEATURE_SMC_HAS_LPOPO (0)
/* @brief Has POR power option (register bit STOPCTRL[PORPO] or VLLSCTRL[PORPO]). */
#define FSL_FEATURE_SMC_HAS_PORPO (0)
/* @brief Has low power wakeup on interrupt (register bit PMCTRL[LPWUI]). */
#define FSL_FEATURE_SMC_HAS_LPWUI (0)
/* @brief Has LLS or VLLS mode control (register bit STOPCTRL[LLSM]). */
#define FSL_FEATURE_SMC_HAS_LLS_SUBMODE (0)
/* @brief Has RAM partition 2 power option (register bit STOPCTRL[RAM2PO]). */
#define FSL_FEATURE_SMC_HAS_RAM2_POWER_OPTION (0)
/* @brief Has VLLS mode control (register bit VLLSCTRL[VLLSM]). */
#define FSL_FEATURE_SMC_USE_VLLSCTRL_REG (0)
/* @brief Has VLLS mode control (register bit STOPCTRL[VLLSM]). */
#define FSL_FEATURE_SMC_USE_STOPCTRL_VLLSM (0)
/* @brief Has high speed run mode (register bit PMPROT[AHSRUN]). */
#define FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE (1)
/* @brief Has low leakage stop mode (register bit PMPROT[ALLS]). */
#define FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE (1)
/* @brief Has very low leakage stop mode (register bit PMPROT[AVLLS]). */
#define FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE (1)
/* @brief Has stop submode. */
#define FSL_FEATURE_SMC_HAS_SUB_STOP_MODE (1)
/* @brief Has stop submode 0(VLLS0). */
#define FSL_FEATURE_SMC_HAS_STOP_SUBMODE0 (1)
/* @brief Has stop submode 2(VLLS2). */
#define FSL_FEATURE_SMC_HAS_STOP_SUBMODE2 (1)
/* @brief Has SMC_PARAM. */
#define FSL_FEATURE_SMC_HAS_PARAM (1)
/* @brief Has SMC_VERID. */
#define FSL_FEATURE_SMC_HAS_VERID (1)
/* @brief Has SMC_CSRE. */
#define FSL_FEATURE_SMC_HAS_CSRE (0)
/* @brief Has stop abort flag (register bit PMCTRL[STOPA]). */
#define FSL_FEATURE_SMC_HAS_PMCTRL_STOPA (0)
/* @brief Has tamper reset (register bit SRS[TAMPER]). */
#define FSL_FEATURE_SMC_HAS_SRS_TAMPER (0)
/* @brief Has security violation reset (register bit SRS[SECVIO]). */
#define FSL_FEATURE_SMC_HAS_SRS_SECVIO (0)
/* @brief Has security violation reset (register bit SRS[VBAT]). */
#define FSL_FEATURE_SMC_HAS_SRS_VBAT (0)
/* @brief Has security violation reset (register bit SRS[CORE0]). */
#define FSL_FEATURE_SMC_HAS_SRS_CORE0 (1)
/* @brief Has security violation reset (register bit SRS[CORE1]). */
#define FSL_FEATURE_SMC_HAS_SRS_CORE1 (1)
/* @brief Has security violation reset (register bit SRIE[VBAT]). */
#define FSL_FEATURE_SMC_HAS_SRIE_VBAT (0)
/* @brief Has security violation reset (register bit SRIE[CORE0]). */
#define FSL_FEATURE_SMC_HAS_SRIE_CORE0 (1)
/* @brief Has security violation reset (register bit SRIE[CORE1]). */
#define FSL_FEATURE_SMC_HAS_SRIE_CORE1 (1)

/* SysTick module features */

/* @brief Systick has external reference clock. */
#define FSL_FEATURE_SYSTICK_HAS_EXT_REF (1)
/* @brief Systick external reference clock is core clock divided by this value. */
#define FSL_FEATURE_SYSTICK_EXT_REF_CORE_DIV (16)

/* TPM module features */

/* @brief Number of channels. */
#define FSL_FEATURE_TPM_CHANNEL_COUNTn(x) \
    ((x) == TPM0 ? (6) : \
    ((x) == TPM1 ? (2) : \
    ((x) == TPM2 ? (6) : \
    ((x) == TPM3 ? (2) : (-1)))))
/* @brief Has counter reset by the selected input capture event (register bits C0SC[ICRST], C1SC[ICRST], ...). */
#define FSL_FEATURE_TPM_HAS_COUNTER_RESET_BY_CAPTURE_EVENT (0)
/* @brief Has TPM_PARAM. */
#define FSL_FEATURE_TPM_HAS_PARAM (1)
/* @brief Has TPM_VERID. */
#define FSL_FEATURE_TPM_HAS_VERID (1)
/* @brief Has TPM_GLOBAL. */
#define FSL_FEATURE_TPM_HAS_GLOBAL (1)
/* @brief Has TPM_TRIG. */
#define FSL_FEATURE_TPM_HAS_TRIG (1)
/* @brief Has counter pause on trigger. */
#define FSL_FEATURE_TPM_HAS_PAUSE_COUNTER_ON_TRIGGER (1)
/* @brief Has external trigger selection. */
#define FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION (1)
/* @brief Has TPM_COMBINE register. */
#define FSL_FEATURE_TPM_HAS_COMBINE (1)
/* @brief Whether COMBINE register has effect. */
#define FSL_FEATURE_TPM_COMBINE_HAS_EFFECTn(x) (1)
/* @brief Has TPM_POL. */
#define FSL_FEATURE_TPM_HAS_POL (1)
/* @brief Has TPM_FILTER register. */
#define FSL_FEATURE_TPM_HAS_FILTER (1)
/* @brief Whether FILTER register has effect. */
#define FSL_FEATURE_TPM_FILTER_HAS_EFFECTn(x) (1)
/* @brief Has TPM_QDCTRL register. */
#define FSL_FEATURE_TPM_HAS_QDCTRL (1)
/* @brief Whether QDCTRL register has effect. */
#define FSL_FEATURE_TPM_QDCTRL_HAS_EFFECTn(x) (1)

/* TRGMUX module features */

/* No feature definitions */

/* TRNG module features */

/* No feature definitions */

/* TSTMR module features */

/* @brief TSTMR clock frequency is 1MHZ. */
#define FSL_FEATURE_TSTMR_CLOCK_FREQUENCY_1MHZ (1)

/* USB module features */

/* @brief KHCI module instance count */
#define FSL_FEATURE_USB_KHCI_COUNT (1)
/* @brief HOST mode enabled */
#define FSL_FEATURE_USB_KHCI_HOST_ENABLED (0)
/* @brief OTG mode enabled */
#define FSL_FEATURE_USB_KHCI_OTG_ENABLED (0)
/* @brief Size of the USB dedicated RAM */
#define FSL_FEATURE_USB_KHCI_USB_RAM (2048)
/* @brief Base address of the USB dedicated RAM */
#define FSL_FEATURE_USB_KHCI_USB_RAM_BASE_ADDRESS (1208025088)
/* @brief Has KEEP_ALIVE_CTRL register */
#define FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED (1)
/* @brief Mode control of the USB Keep Alive */
#define FSL_FEATURE_USB_KHCI_KEEP_ALIVE_MODE_CONTROL (USB_KEEP_ALIVE_CTRL_WAKE_REQ_EN_MASK)
/* @brief Has the Dynamic SOF threshold compare support */
#define FSL_FEATURE_USB_KHCI_DYNAMIC_SOF_THRESHOLD_COMPARE_ENABLED (1)
/* @brief Has the VBUS detect support */
#define FSL_FEATURE_USB_KHCI_VBUS_DETECT_ENABLED (1)
/* @brief Has the IRC48M module clock support */
#define FSL_FEATURE_USB_KHCI_IRC48M_MODULE_CLOCK_ENABLED (1)
/* @brief Number of endpoints supported */
#define FSL_FEATURE_USB_ENDPT_COUNT (16)
/* @brief Has STALL_IL/OL_DIS registers */
#define FSL_FEATURE_USB_KHCI_HAS_STALL_LOW (1)
/* @brief Has STALL_IH/OH_DIS registers */
#define FSL_FEATURE_USB_KHCI_HAS_STALL_HIGH (1)

/* USDHC module features */

/* @brief Has external DMA support (VEND_SPEC[EXT_DMA_EN]) */
#define FSL_FEATURE_USDHC_HAS_EXT_DMA (0)
/* @brief Has HS400 mode (MIX_CTRL[HS400_MODE]) */
#define FSL_FEATURE_USDHC_HAS_HS400_MODE (0)
/* @brief Has SDR50 support (HOST_CTRL_CAP[SDR50_SUPPORT]) */
#define FSL_FEATURE_USDHC_HAS_SDR50_MODE (0)
/* @brief Has SDR104 support (HOST_CTRL_CAP[SDR104_SUPPORT]) */
#define FSL_FEATURE_USDHC_HAS_SDR104_MODE (0)

/* VREF module features */

/* @brief Has chop oscillator (bit TRM[CHOPEN]) */
#define FSL_FEATURE_VREF_HAS_CHOP_OSC (1)
/* @brief Has second order curvature compensation (bit SC[ICOMPEN]) */
#define FSL_FEATURE_VREF_HAS_COMPENSATION (1)
/* @brief If high/low buffer mode supported */
#define FSL_FEATURE_VREF_MODE_LV_TYPE (1)
/* @brief Module has also low reference (registers VREFL/VREFH) */
#define FSL_FEATURE_VREF_HAS_LOW_REFERENCE (0)
/* @brief Has VREF_TRM4. */
#define FSL_FEATURE_VREF_HAS_TRM4 (1)

/* WDOG module features */

/* @brief Watchdog is available. */
#define FSL_FEATURE_WDOG_HAS_WATCHDOG (1)
/* @brief WDOG_CNT can be 32-bit written. */
#define FSL_FEATURE_WDOG_HAS_32BIT_ACCESS (1)

/* XRDC module features */

/* @brief Does not have global valid (register bit CR[GVLD]). */
#define FSL_FEATURE_XRDC_HAS_NO_CR_GVLD (1)
/* @brief Has domain ID of faulted access (register bit FDID[FDID]). */
#define FSL_FEATURE_XRDC_HAS_FDID (1)
/* @brief Has special 4-state model option (register bit PID[SP4SM]). */
#define FSL_FEATURE_XRDC_PID_SP4SM (1)
/* @brief Does not have logical partition identifier (register bit MDA_W[LPID]). */
#define FSL_FEATURE_XRDC_NO_MDA_LPID (1)
/* @brief Does not have logical partition enable option (register bit MDA_W[LPE]). */
#define FSL_FEATURE_XRDC_NO_MDA_LPE (1)
/* @brief Does not have peripheral semaphore enable option (register bit PDAC_W0[SE]). */
#define FSL_FEATURE_XRDC_NO_PDAC_SE (1)
/* @brief Does not have peripheral semaphore number (register bit PDAC_W0[SNUM]). */
#define FSL_FEATURE_XRDC_NO_PDAC_SNUM (1)
/* @brief Has peripheral excessive access lock owner (register bit PDAC_W0[EALO]). */
#define FSL_FEATURE_XRDC_HAS_PDAC_EALO (1)
/* @brief Has peripheral excessive access lock option (register bit PDAC_W1[EAL]). */
#define FSL_FEATURE_XRDC_HAS_PDAC_EAL (1)
/* @brief Has memory region end address (register bit MRGD_W1[ENDADDR]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR (1)
/* @brief Does not have memory region semaphore enable option (register bit MRGD_W2[SE]). */
#define FSL_FEATURE_XRDC_NO_MRGD_SE (1)
/* @brief Does not have memory region semaphore number (register bit MRGD_W2[SNUM]). */
#define FSL_FEATURE_XRDC_NO_MRGD_SNUM (1)
/* @brief Does not domain x access control policy option (register bit MRGD_W2[DxACP]). */
#define FSL_FEATURE_XRDC_NO_MRGD_DXACP (1)
/* @brief Does not have region size configuration (register bit MRGD_W2[SZ]). */
#define FSL_FEATURE_XRDC_NO_MRGD_SZ (1)
/* @brief Does not have subregion disable option (register bit MRGD_W2[SRD]). */
#define FSL_FEATURE_XRDC_NO_MRGD_SRD (1)
/* @brief Has memory region excessive access lock owner (register bit MRGD_W2[EALO]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_EALO (1)
/* @brief Has domain x access policy select option (register bit MRGD_W2[DxSEL]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_DXSEL (1)
/* @brief Has memory region excessive access lock option (register bit MRGD_W3[EAL]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_EAL (1)
/* @brief Does not have lock option in MRGD_W3 register (register bit MRGD_W3[LK2]). */
#define FSL_FEATURE_XRDC_NO_MRGD_W3_LK2 (1)
/* @brief Does not have valid option in MRGD_W3 register (register bit MRGD_W3[VLD]). */
#define FSL_FEATURE_XRDC_NO_MRGD_W3_VLD (1)
/* @brief Has code region indicator select option (register bit MRGD_W3[CR]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_CR (1)
/* @brief Has ASSSET lock option (register bit MRGD_W4[LKAS1]/[LKAS2]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_LKAS (1)
/* @brief Has programmable access flags (register bit MRGD_W4[ACCSET1]/[ACCSET2]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_ACCSET (1)
/* @brief Has lock option in MRGD_W4 register (register bit MRGD_W4[LK2]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_W4_LK2 (1)
/* @brief Has valid option in MRGD_W4 register (register bit MRGD_W4[VLD]). */
#define FSL_FEATURE_XRDC_HAS_MRGD_W4_VLD (1)
/* @brief XRDC domain number (reset value of HWCFG0[NDID] plus 1). */
#define FSL_FEATURE_XRDC_DOMAIN_COUNT (3)

#endif /* _RV32M1_zero_riscy_FEATURES_H_ */

