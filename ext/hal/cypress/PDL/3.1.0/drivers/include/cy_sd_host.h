/***************************************************************************//**
* \file cy_sd_host.h
* \version 1.0
*
*  This file provides constants and parameter values for 
*  the SD Host Controller driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_sd_host SD Host Controller (SD Host)
* \{
* This driver provides a user an easy method for accessing standard 
* Host Controller Interface (HCI) registers and provides some simple
* functionality on top of the HCI for reading and writing data to 
* an SD card, eMMc card or a SDIO device.
*
* Features:
* * Supports data transfer using CPU, SDMA, ADMA2 and ADMA3 modes
* * Supports configurable block size (1 to 65,535 Bytes)
* * Supports interrupt enabling and masking
* * Supports SD-HCI Host version 4 mode or less
* * Compliant with the SD 6.0, SDIO 4.10 and eMMC 5.1 specifications and earlier versions
* * SD interface features:
* * - Supports 4-bit interface
* * - Supports Ultra High Speed (UHS-I) mode
* * - Supports Default Speed (DS), High Speed (HS), SDR12, SDR25 and SDR50 speed modes
* * - Supports SDIO card interrupts in both 1-bit and 4-bit modes 
* * - Supports Standard capacity (SDSC) and High capacity (SDHC) memory
* * - Supports CRC and check for command and data packets
* * - Supports packet timeouts
* * - Handles SDIO card interrupt
* * eMMC interface features:
* * - Supports 4-bit/8-bit interface 
* * - Supports legacy and High Speed SDR speed modes
* * - Supports CRC and check for command and data packets
* * - Supports packet timeouts
*
* Unsupported Features:  
* * Wrap address transfers
* * eMMC boot operation
* * Suspend/Resume operation in an SDIO card
* * Operation in SDR104, UHS-II mode, High Speed DDR, HS200, and HS400
* * Serial Peripheral Interface (SPI) protocol mode
* * Interrupt input pins for embedded SD system
* * Auto-tuning
* * Command queuing
*
* The driver has low level and high level API. 
* The low level functions provide an easy method to read and write registers. 
* Also these functions allow valid interaction with an SD Card, eMMC card, 
* and SDIO card.
* The high level functions provide an easy mechanism to enumerate a device,
* read, write, and erase data. They are RTOS friendly. 
* When starting a command these functions do not wait for the command to finish. 
* The interrupt and flags are used to check when the transfer finishes. 
* This allows to put RTOS delays in their code and not have to hack our code.
*
* \section group_sd_host_section_Configuration_Considerations Configuration Considerations
* The SD Host driver configuration can be divided to number of sequential
* steps listed below:
* * \ref group_sd_host_enable
* * \ref group_sd_host_pins
* * \ref group_sd_host_clock
* * \ref group_sd_host_intr
* * \ref group_sd_host_config
* * \ref group_sd_host_card_init
*
* \note
* SD Host driver is built on top of the SDHC hardware block. The SDHC1 instance is
* used as an example for all code snippets. Modify the code to match your
* design.
*
* \subsection group_sd_host_enable Enable SD Host
* Enable the SDHC block calling \ref Cy_SD_Host_Enable.
*
* \subsection group_sd_host_pins Assign and Configure Pins
* Only dedicated SD Host pins can be used for SD Host operation. The HSIOM
* register must be configured to connect block to the pins. Also the SD Host 
* pins must be configured in Strong Drive, Input buffer on:
*
* \snippet sd_host\1.0\snippet\main.c SD_HOST_CFG_PINS
*
* \subsection group_sd_host_clock Assign Clock Source
* The SD Host is sourced from the PERI clock. The clock must be set to 100 MHz:
*
* \snippet sd_host\1.0\snippet\main.c SD_HOST_CFG_ASSIGN_CLOCK
*
* \subsection group_sd_host_intr Configure Interrupt (Optional)
* The user can set up the interrupt for SD Host operation. 
* The user is responsible for writing the own Interrupt handler.
* The Interrupt must be called in the interrupt handler for the selected SDHC
* instance. Also this interrupt must be enabled in the NVIC otherwise
* it will not work.
* It is the user responsibility to clear the normal and error interrupt statuses. 
* The interrupt statuses can be read using \ref Cy_SD_Host_GetNormalInterruptStatus 
* and \ref Cy_SD_Host_GetErrorInterruptStatus. 
* To clear the interrupt statuses, use \ref Cy_SD_Host_ClearNormalInterruptStatus 
* and \ref Cy_SD_Host_ClearErrorInterruptStatus.
*
* \snippet sd_host\1.0\snippet\main.c SD_HOST_INTR_A
* \snippet sd_host\1.0\snippet\main.c SD_HOST_INTR_B
*
* \subsection group_sd_host_config Configure SD Host
* To set up the SD Host driver, provide the configuration parameters in the
* \ref cy_stc_sd_host_init_config_t structure. Set to true emmc parameter for 
* the  eMMC device, otherwise set it to false. Set dmaType if the DMA mode 
* is used for read/write operations. The other parameters are optional for
* operation. To initialize the driver, call \ref Cy_SD_Host_Init
* function providing a pointer to the filled \ref cy_stc_sd_host_init_config_t
* structure and allocated \ref cy_stc_sd_host_context_t.
*
* \snippet sd_host\1.0\snippet\main.c SD_HOST_CONTEXT
* \snippet sd_host\1.0\snippet\main.c SD_HOST_CFG
* 
* The SD, eMMC or SDIO card can be configured using \ref Cy_SD_Host_InitCard
* function a pointer to the filled \ref cy_stc_sd_host_sd_card_config_t
* structure and allocated \ref cy_stc_sd_host_context_t.
*
* \subsection group_sd_host_card_init Initialize the card
* Finally, enable the card operation calling 
* calling \ref Cy_SD_Host_InitCard.
*
* \snippet sd_host\1.0\snippet\main.c SD_HOST_ENABLE_CARD_INIT
*
* \section group_sd_host_use_cases Common Use Cases
*
* \subsection group_sd_host_sd_card_mode SD card Operation
* The master API is divided into two categories:
* \ref group_sd_host_high_level_functions and
* \ref group_sd_host_low_level_functions. Therefore, there are two
* methods for initiating SD card transactions using either <b>Low-Level</b> or
* <b>High-Level</b> API.
*
* \subsubsection group_sd_host_master_hl Use High-Level Functions
* Call \ref Cy_SD_Host_Read or \ref Cy_SD_Host_Write to
* communicate with the SD memory device. These functions do not block 
* and only start a transaction. After a transaction starts, 
* the user should check the further data transaction complete event.
*
* \subsubsection group_sd_host_master_ll Use Low-Level Functions
* Call \ref Cy_SD_Host_InitDataTransfer to initialize the SD block  
* for a data transfer. It does not start a transfer. To start a transfer 
* call \ref Cy_SD_Host_SendCommand after calling this function. 
* If DMA is not used for Data transfer then the buffer needs to be filled 
* with data first if this is a write.
* Wait the transfer complete event.
*
* \section group_sd_host_lp Low Power Support
* The SD Host does not operate in Hibernate and Deep Sleep modes but it
* can automatically continue write/read operation after restoring from
* the Deep Sleep mode.
* To reduce the power consumption in the Active mode the user can be stop 
* clocking the SD bus but the following interrupts can be allowed: 
* Card Insert, Card Removal and SDIO Interrupt. 

* \section group_sd_host_remove_insert SD card removing and inserting
* SD card removing or inserting can be detected by calling 
* Cy_SD_Host_GetNormalInterruptStatus() which returns the card 
* removal or card insertion events 
* (CY_SD_HOST_CARD_REMOVAL or CY_SD_HOST_CARD_INSERTION bits). 
* These events should be reset using 
* Cy_SD_Host_ClearNormalInterruptStatus() when they occur.
* When card is removed the SDHC block disables the CMD/DAT output. 
* It is recommended to set DAT pins to the Digital High-Z
* (CY_GPIO_DM_HIGHZ) drive mode when the card removal is detected.
* \note If CARD_INTERRUPT is enabled and DAT pins are not set 
* into to the Digital High-Z drive mode then the interrupt will 
* continuously get triggered because the DAT1 line is driven low 
* upon card reinsertion. The user will have to detect the card 
* removal in the ISR handler, set to set DAT pins to 
* the Digital High-Z (CY_GPIO_DM_HIGHZ) drive mode and 
* clear CY_SD_HOST_CARD_INTERRUPT bit using 
* Cy_SD_Host_ClearNormalInterruptStatus().
*
* When card is inserted the SDHC block automatically disables 
* the card power and clock. The user should set the DAT pins drive 
* mode back to Strong Drive, Input buffer on (CY_GPIO_DM_STRONG) 
* and call Cy_SD_Host_InitCard().
*
* \section group_sd_host_more_information More Information
*
* Refer to the appropriate device technical reference manual (TRM) for 
* a detailed description of the registers.
*
* \section group_sd_host_MISRA MISRA-C Compliance
* TBD
*
* \section group_sd_host_Changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_sd_host_macros Macros
* \defgroup group_sd_host_functions Functions
* \{
* \defgroup group_sd_host_high_level_functions High-Level
* \defgroup group_sd_host_low_level_functions Low-Level
* \defgroup group_sd_host_interrupt_functions Interrupt
* \}
* \defgroup group_sd_host_data_structures Data Structures
* \defgroup group_sd_host_enums Enumerated Types
*/

#ifndef Cy_SD_Host_PDL_H
#define Cy_SD_Host_PDL_H

/******************************************************************************/
/* Include files                                                              */
/******************************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"

#if defined(CY_IP_MXSDHC)

#if defined (__CC_ARM)
    #pragma anon_unions    
#endif

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/**
* \addtogroup group_sd_host_macros
* \{
*/

/** Driver major version */
#define Cy_SD_Host_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define Cy_SD_Host_DRV_VERSION_MINOR       0

/******************************************************************************
* API Constants
******************************************************************************/


#define CY_SD_HOST_BLOCK_SIZE               (512u) /**< The SD memory card block size. */

/* The card states. */
#define CY_SD_HOST_CARD_IDLE                (0x0uL)  /**< The idle state. */
#define CY_SD_HOST_CARD_READY               (0x1uL)  /**< The card is ready state. */
#define CY_SD_HOST_CARD_IDENT               (0x2uL)  /**< The card identification state. */
#define CY_SD_HOST_CARD_STBY                (0x3uL)  /**< The card stand-by state. */
#define CY_SD_HOST_CARD_TRAN                (0x4uL)  /**< The card transition state. */
#define CY_SD_HOST_CARD_DATA                (0x5uL)  /**< The sending-data state. */
#define CY_SD_HOST_CARD_RCV                 (0x6uL)  /**< The receive-data state. */
#define CY_SD_HOST_CARD_PRG                 (0x7uL)  /**< The programming state. */
#define CY_SD_HOST_CARD_DIS                 (0x8uL)  /**< The disconnect state. */

/** \cond INTERNAL_MACROS */

/* The card status (CMD13) bits. */
#define CY_SD_HOST_CMD13_OUT_OF_RANGE       (31u) /* The command's argument is out of range. */ 
#define CY_SD_HOST_CMD13_ADDRESS_ERROR      (30u) /* The address does not match the block lenght. */ 
#define CY_SD_HOST_CMD13_BLOCK_LEN_ERROR    (29u) /* The block lenght is not allowed for this card. */ 
#define CY_SD_HOST_CMD13_ERASE_SEQ__ERROR   (28u) /* An error in the sequence of erase commands occured. */ 
#define CY_SD_HOST_CMD13_ERASE_PARAM        (27u) /* An invalid selection of write blocks for erase occured. */ 
#define CY_SD_HOST_CMD13_WP_VIOLATION       (26u) /* The host attempts to write to a proteckted block 
                                                    * or to the permanent write proteckted card. 
                                                    */ 
#define CY_SD_HOST_CMD13_CARD_IS_LOCKED     (25u) /* The card is locked by the host. */
#define CY_SD_HOST_CMD13_LOCK_ULOCK_FAILED  (24u) /* A sequence or password error occured 
                                                    * has been detecked in the lock/unlock card command. 
                                                    */
#define CY_SD_HOST_CMD13_COM_CRC_ERROR      (23u) /* The CRCD of the previous command failed. */
#define CY_SD_HOST_CMD13_ILLEGAL_COMMAND    (22u) /* The command is not legal for the card state. */
#define CY_SD_HOST_CMD13_CARD_ECC_FAILED    (21u) /* The card internal ECC failed. */
#define CY_SD_HOST_CMD13_CC_ERROR           (20u) /* The internal card controller error. */
#define CY_SD_HOST_CMD13_ERROR              (19u) /* A general or an unknown error occured. */
#define CY_SD_HOST_CMD13_CURRENT_STATE      (9u)  /* The state of the card. */
#define CY_SD_HOST_CMD13_CURRENT_STATE_MSK  (0x00001E00uL)  /* The current state mask of the card. */
#define CY_SD_HOST_CMD13_READY_FOR_DATA     (8u)  /* The buffer is empty on the bus. */

/* Timeouts. */ 
#define CY_SD_HOST_DAT30_WAIT_TIME_US       (1000u)  /* The DAT[3:0] level wait time. */
#define CY_SD_HOST_INT_CLK_STABLE_TIMEOUT_MS (150u)/* The Internal Clock Stable timout. */
#define CY_SD_HOST_1_8_REG_STABLE_TIME_MS   (10u)  /* The 1.8 voltage regulator stable time. */
#define CY_SD_HOST_SUPPLY_RAMP_UP_TIME_MS   (1000u)  /* The host supply ramp up time. */
#define CY_SD_HOST_CLK_RAMP_UP_TIME_MS      (100u) /* The host power ramp up time. */
#define CY_SD_HOST_EMMC_CMD6_TIMEOUT_MULT   (10u)    /* The 10x multiplier of GENERIC_CMD6_TIME[248]. */
#define CY_SD_HOST_RETRY_TIME               (1000u)  /* The number loops to make the timeout in msec. */
#define CY_SD_HOST_VOLTAGE_CHECK_RETRY      (2u)      /* The number loops for voltage check. */
#define CY_SD_HOST_SDIO_CMD5_TIMEOUT_MS     (1000u)  /* The SDIO CMD5 timeout. */
#define CY_SD_HOST_ACMD41_TIMEOUT_MS        (1000u)  /* The ACMD41 timeout. */
#define CY_SD_HOST_CMD1_TIMEOUT_MS          (1000u) /* The eMMC device timeout to complete its initialization. */
#define CY_SD_HOST_CMD_TIMEOUT_MS           (3u)    /* The Command complete timeout. */
#define CY_SD_HOST_BUFFER_RDY_TIMEOUT_MS    (150u)  /* The Buffer read ready timeout. */
#define CY_SD_HOST_READ_TIMEOUT_MS          (100u)  /* The Read timeout for one block. */
#define CY_SD_HOST_WRITE_TIMEOUT_MS         (250u)  /* The Write timeout for one block. */
#define CY_SD_HOST_ERASE_TIMEOUT_MS         (250u)  /* The Erase timeout for one block. */
#define CY_SD_HOST_DISCARD_TIMEOUT_MS       (250u)  /* The Discard timeout for one block. */
#define CY_SD_HOST_FULE_TIMEOUT_MS          (1000u) /* The Fule timeout for one block. */
#define CY_SD_HOST_MAX_TIMEOUT              (0x0Eu) /* The data max timeout for TOUT_CTRL_R. */
#define CY_SD_HOST_NCC_MIN_CYCLES           (8u)    /* The period (clock cycles) between an end bit d 
                                                     *   of comman and a start bit of next command. 
                                                     */
#define CY_SD_HOST_INIT_CLK_FREQUENCY_KHZ   (400u)  /* The CLK frequency in kHz during card initialization. */
#define CY_SD_HOST_NCC_MIN_US               (1000u * CY_SD_HOST_NCC_MIN_CYCLES / CY_SD_HOST_INIT_CLK_FREQUENCY_KHZ)
#define CY_SD_HOST_3_PERIODS_US             ((1000u * 3u / CY_SD_HOST_INIT_CLK_FREQUENCY_KHZ) + 1u)


/* Commands codes. */ 
#define CY_SD_HOST_SD_CMD0                  (0u)
#define CY_SD_HOST_SD_CMD1                  (1u)
#define CY_SD_HOST_SD_CMD2                  (2u)
#define CY_SD_HOST_SD_CMD3                  (3u)
#define CY_SD_HOST_SD_CMD5                  (5u)
#define CY_SD_HOST_SD_CMD6                  (6u)

#define CY_SD_HOST_SD_CMD7                  (7u)
#define CY_SD_HOST_SD_CMD8                  (8u)
#define CY_SD_HOST_SD_CMD9                  (9u)
#define CY_SD_HOST_SD_CMD10                 (10u)
#define CY_SD_HOST_SD_CMD11                 (11u)
#define CY_SD_HOST_SD_CMD12                 (12u)
#define CY_SD_HOST_SD_CMD13                 (13u)

#define CY_SD_HOST_SD_CMD16                 (16u)
#define CY_SD_HOST_SD_CMD17                 (17u)
#define CY_SD_HOST_SD_CMD18                 (18u)

#define CY_SD_HOST_SD_CMD23                 (23u)
#define CY_SD_HOST_SD_CMD24                 (24u)
#define CY_SD_HOST_SD_CMD25                 (25u)

#define CY_SD_HOST_SD_CMD27                 (27u)
#define CY_SD_HOST_SD_CMD28                 (28u)
#define CY_SD_HOST_SD_CMD29                 (29u)
#define CY_SD_HOST_SD_CMD30                 (30u)

#define CY_SD_HOST_SD_CMD32                 (32u)
#define CY_SD_HOST_SD_CMD33                 (33u)
#define CY_SD_HOST_SD_CMD35                 (35u)
#define CY_SD_HOST_SD_CMD36                 (36u)
#define CY_SD_HOST_SD_CMD38                 (38u)


#define CY_SD_HOST_SD_CMD42                 (42u)
#define CY_SD_HOST_SD_CMD52                 (52u)

#define CY_SD_HOST_SD_CMD55                 (55u)

#define CY_SD_HOST_MMC_CMD8                 (8u | CY_SD_HOST_MMC_CMD_TAG)

#define CY_SD_HOST_SD_ACMD_OFFSET           (0x40u)
#define CY_SD_HOST_SD_ACMD6                 (CY_SD_HOST_SD_ACMD_OFFSET + 6u)
#define CY_SD_HOST_SD_ACMD13                (CY_SD_HOST_SD_ACMD_OFFSET + 13u)
#define CY_SD_HOST_SD_ACMD41                (CY_SD_HOST_SD_ACMD_OFFSET + 41u)
#define CY_SD_HOST_SD_ACMD51                (CY_SD_HOST_SD_ACMD_OFFSET + 51u)

/* Other constants. */
#define CY_SD_HOST_CMD5_IO_NUM              (28u) /* The Number of IO functions offset. */
#define CY_SD_HOST_CMD5_IO_NUM_MASK         (0x70000000u) /* The Number of IO functions mask. */
#define CY_SD_HOST_CMD5_MP                  (27u) /* The Memory Present offset. */
#define CY_SD_HOST_CMD5_MP_MASK             (0x08000000u) /* The Memory Present mask. */
#define CY_SD_HOST_IO_OCR_MASK              (0x00FFFFFFu) /* The IO OCR register mask. */
#define CY_SD_HOST_IO_OCR_C                 (0x80000000u) /* The IO power up status (IORDY). */

#define CY_SD_HOST_MMC_CMD_TAG              (0x80u)
#define CY_SD_HOST_MMC_LEGACY_SIZE_BYTES    (0x200000u)

#define CY_SD_HOST_CMD8_VHS_27_36           (0x100u) /* CMD8 voltage supplied 2.7-3.6 V. */
#define CY_SD_HOST_CMD8_PATTERN_MASK        (0xFFu)
#define CY_SD_HOST_CMD8_CHECK_PATTERN       (0xAAu)
#define CY_SD_HOST_ACMD41_S18R              (1uL << 24u) /* The 1.8 V request. */
#define CY_SD_HOST_ACMD41_HCS               (1uL << 30u) /* SDHC or SDXC supported. */
#define CY_SD_HOST_OCR_S18A                 (1uL << 24u) /* The 1.8 V accepted. */
#define CY_SD_HOST_OCR_35_36_V              (1uL << 23u) /* The 3.5 - 3.6 voltage window. */ 
#define CY_SD_HOST_OCR_34_35_V              (1uL << 22u) /* The 3.4 - 3.5 voltage window. */
#define CY_SD_HOST_OCR_33_34_V              (1uL << 21u) /* The 3.3 - 3.4 voltage window. */
#define CY_SD_HOST_OCR_32_33_V              (1uL << 20u) /* The 3.2 - 3.3 voltage window. */
#define CY_SD_HOST_OCR_31_32_V              (1uL << 19u) /* The 3.1 - 3.2 voltage window. */
#define CY_SD_HOST_OCR_30_31_V              (1uL << 18u) /* The 3.0 - 3.1 voltage window. */
#define CY_SD_HOST_OCR_29_30_V              (1uL << 17u) /* The 2.9 - 3.0 voltage window. */
#define CY_SD_HOST_OCR_28_29_V              (1uL << 16u) /* The 2.8 - 2.9 voltage window. */
#define CY_SD_HOST_OCR_27_28_V              (1uL << 15u) /* The 2.7 - 2.8 voltage window. */
#define CY_SD_HOST_ACMD41_VOLTAGE           (CY_SD_HOST_OCR_35_36_V |\
                                             CY_SD_HOST_OCR_34_35_V |\
                                             CY_SD_HOST_OCR_33_34_V |\
                                             CY_SD_HOST_OCR_32_33_V |\
                                             CY_SD_HOST_OCR_31_32_V |\
                                             CY_SD_HOST_OCR_30_31_V |\
                                             CY_SD_HOST_OCR_29_30_V |\
                                             CY_SD_HOST_OCR_28_29_V |\
                                             CY_SD_HOST_OCR_27_28_V)
#define CY_SD_HOST_ARG_ACMD41_BUSY          (0x80000000uL)
#define CY_SD_HOST_OCR_BUSY_BIT             (0x80000000uL) /* The Card power up status bit (busy). */ 
#define CY_SD_HOST_OCR_CAPACITY_MASK        (0x40000000uL) /* The OCR sector access mode bit. */                        
#define CY_SD_HOST_BUS_VOLTAGE_3_3V         (0x7u)
#define CY_SD_HOST_BUS_VOLTAGE_3_0V         (0x6u)
#define CY_SD_HOST_BUS_VOLTAGE_1_8V         (0x5u)
#define CY_SD_HOST_ACMD_OFFSET_MASK         (0x3FuL)
#define CY_SD_HOST_EMMC_DUAL_VOLTAGE        (0x00000080uL)
#define CY_SD_HOST_EMMC_VOLTAGE_WINDOW      (CY_SD_HOST_OCR_CAPACITY_MASK |\
                                             CY_SD_HOST_EMMC_DUAL_VOLTAGE |\
                                             CY_SD_HOST_ACMD41_VOLTAGE)

#define CY_SD_HOST_SWITCH_FUNCTION_BIT      (31u)
#define CY_SD_HOST_DEFAULT_FUNCTION_MASK    (0xFFF0u)
#define CY_SD_HOST_DEFAULT_SPEED            (0u)
#define CY_SD_HOST_HIGH_SPEED               (1u)
#define CY_SD_HOST_SDR12_SPEED              (0u)  /* The SDR12/Legacy speed. */ 
#define CY_SD_HOST_SDR25_SPEED              (1u)  /* The SDR25/High Speed SDR speed. */ 
#define CY_SD_HOST_SDR50_SPEED              (2u)  /* The SDR50 speed. */ 

#define CY_SD_HOST_EMMC_BUS_WIDTH_ADDR      (0xB7uL)
#define CY_SD_HOST_EMMC_HS_TIMING_ADDR      (0xB9uL)
#define CY_SD_HOST_CMD23_BLOCKS_NUM_MASK    (0xFFFFu)
#define CY_SD_HOST_CMD23_RELIABLE_WRITE_POS (31u)

#define CY_SD_HOST_SDSC_ADDR_SHIFT          (9u)
#define CY_SD_HOST_RCA_SHIFT                (16u)
#define CY_SD_HOST_RESPONSE_SIZE            (4u)
#define CY_SD_HOST_CID_SIZE                 (4u)
#define CY_SD_HOST_CSD_SIZE                 (4u)
#define CY_SD_HOST_SCR_BLOCKS               (8u)
#define CY_SD_HOST_CSD_BLOCKS               (16u)
#define CY_SD_HOST_SD_STATUS_BLOCKS         (64u)

#define CY_SD_HOST_PERI_FREQUENCY           (24000000u)
#define CY_SD_HOST_FREQ_SEL_MSK             (0xFFu)
#define CY_SD_HOST_UPPER_FREQ_SEL_MSK       (0x03u)
#define CY_SD_HOST_UPPER_FREQ_SEL_POS       (8u)

/* The CMD6 SWITCH command bitfields. */ 
#define CY_SD_HOST_EMMC_CMD6_ACCESS_OFFSET  (24u)
#define CY_SD_HOST_EMMC_CMD6_IDX_OFFSET     (16u)
#define CY_SD_HOST_EMMC_CMD6_VALUE_OFFSET   (8u)
#define CY_SD_HOST_EMMC_CMD6_CMD_SET_OFFSET (0u)

/* The EMMC EXTCSD register bitfields. */
#define CY_SD_HOST_EXTCSD_SEC_COUNT         (53u)
#define CY_SD_HOST_EXTCSD_GENERIC_CMD6_TIME (62u)
#define CY_SD_HOST_EXTCSD_SIZE              (128u)

/* The CSD register masks/positions. */ 
#define CY_SD_HOST_CSD_V1_C_SIZE_MSB_MASK   (0x00000003uL)
#define CY_SD_HOST_CSD_V1_C_SIZE_ISB_MASK   (0xFF000000uL)
#define CY_SD_HOST_CSD_V1_C_SIZE_LSB_MASK   (0x00C00000uL)
#define CY_SD_HOST_CSD_V1_C_SIZE_MSB_POS    (0u)
#define CY_SD_HOST_CSD_V1_C_SIZE_ISB_POS    (24u)
#define CY_SD_HOST_CSD_V1_C_SIZE_LSB_POS    (22u)
#define CY_SD_HOST_CSD_V1_C_SIZE_MSB_MULT   (10u)
#define CY_SD_HOST_CSD_V1_C_SIZE_ISB_MULT   (2u)
#define CY_SD_HOST_CSD_V1_C_SIZE_MULT_MASK  (0x00000380uL)
#define CY_SD_HOST_CSD_V1_C_SIZE_MULT_POS   (7u)
#define CY_SD_HOST_CSD_V1_READ_BL_LEN_MASK  (0x00000F00uL)
#define CY_SD_HOST_CSD_V1_READ_BL_LEN_POS   (8u)
#define CY_SD_HOST_CSD_V1_BL_LEN_512        (9u)
#define CY_SD_HOST_CSD_V1_BL_LEN_1024       (10u)
#define CY_SD_HOST_CSD_V1_BL_LEN_2048       (11u)
#define CY_SD_HOST_CSD_V1_1024_SECT_FACTOR  (2u)
#define CY_SD_HOST_CSD_V1_2048_SECT_FACTOR  (4u)
#define CY_SD_HOST_CSD_V2_C_SIZE_MASK       (0x3FFFFF00uL)
#define CY_SD_HOST_CSD_V2_C_SIZE_POS        (8u)
#define CY_SD_HOST_CSD_V2_SECTOR_MULT       (9u)
#define CY_SD_HOST_CSD_TEMP_WRITE_PROTECT   (20u)
#define CY_SD_HOST_CSD_PERM_WRITE_PROTECT   (21u)
#define CY_SD_HOST_CSD_COPY                 (22u)
#define CY_SD_HOST_CSD_LSB_MASK             (0x000000FFuL)
#define CY_SD_HOST_CSD_ISBR_MASK            (0x0000FF00uL)
#define CY_SD_HOST_CSD_ISBL_MASK            (0x00FF0000uL)
#define CY_SD_HOST_CSD_MSB_MASK             (0xFF000000uL)
#define CY_SD_HOST_CSD_ISB_SHIFT            (16u)

/* The CMD6 EXT_CSD access mode. */ 
#define CY_SD_HOST_EMMC_ACCESS_COMMAND_SET  (0x0u)
#define CY_SD_HOST_EMMC_ACCESS_SET_BITS     (0x1u)
#define CY_SD_HOST_EMMC_ACCESS_CLEAR_BITS   (0x2u)
#define CY_SD_HOST_EMMC_ACCESS_WRITE_BYTE   (0x3uL)

/* The CCCR register values. */ 
#define CY_SD_HOST_CCCR_SPEED_CONTROL       (0x00013u)
#define CY_SD_HOST_CCCR_IO_ABORT            (0x00006u)
#define CY_SD_HOST_CCCR_IO_ABORT_RES_MASK   (0x8u)
#define CY_SD_HOST_CCCR_SPEED_SHS_MASK      (0x1u)
#define CY_SD_HOST_CCCR_SPEED_EHS_MASK      (0x2u)
#define CY_SD_HOST_CCCR_SPEED_BSS0_MASK     (0x2u)
#define CY_SD_HOST_CCCR_SPEED_BSS1_MASK     (0x4u)
#define CY_SD_HOST_CCCR_BUS_INTERFACE_CTR   (0x00007u)

#define CY_SD_HOST_CCCR_BUS_WIDTH_0         (0x1u)
#define CY_SD_HOST_CCCR_BUS_WIDTH_1         (0x2u)
#define CY_SD_HOST_CCCR_S8B                 (0x4u)

/* The SDMA constants. */ 
#define CY_SD_HOST_SDMA_BUF_BYTES_4K        (0x0u) /* 4K bytes SDMA Buffer Boundary. */
#define CY_SD_HOST_SDMA_BUF_BYTES_8K        (0x1u) /* 8K bytes SDMA Buffer Boundary. */
#define CY_SD_HOST_SDMA_BUF_BYTES_16K       (0x2u) /* 16K bytes SDMA Buffer Boundary. */
#define CY_SD_HOST_SDMA_BUF_BYTES_32K       (0x3u) /* 32K bytes SDMA Buffer Boundary. */
#define CY_SD_HOST_SDMA_BUF_BYTES_64K       (0x4u) /* 64K bytes SDMA Buffer Boundary. */
#define CY_SD_HOST_SDMA_BUF_BYTES_128K      (0x5u) /* 128K bytes SDMA Buffer Boundary. */
#define CY_SD_HOST_SDMA_BUF_BYTES_256K      (0x6u) /* 256K bytes SDMA Buffer Boundary. */
#define CY_SD_HOST_SDMA_BUF_BYTES_512K      (0x7u) /* 512K bytes SDMA Buffer Boundary. */

/* The ADMA constants. */ 
#define CY_SD_HOST_ADMA_NOP                 (0x0uL) /* Do not execute current line and go to next line. */
#define CY_SD_HOST_ADMA_RSV                 (0x2uL) /* Reserved. */
#define CY_SD_HOST_ADMA_TRAN                (0x4uL) /* Transfer data of one descriptor line. */
#define CY_SD_HOST_ADMA_LINK                (0x6uL) /* Link to another descriptor. */
#define CY_SD_HOST_ADMA3_CMD                (0x1uL) /* The Command descriptor. */
#define CY_SD_HOST_ADMA3_INTERGRATED        (0x7uL) /* The Integrated descriptor. */
#define CY_SD_HOST_ADMA2_DESCR_SIZE         (0x2uL) /* The ADMA2 descriptor size. */

/* The ADMA decriptor table postitons. */
#define CY_SD_HOST_ADMA_ATTR_VALID_POS      (0u) /* ADMA Attr Valid position. */
#define CY_SD_HOST_ADMA_ATTR_END_POS        (1u) /* ADMA Attr End position. */
#define CY_SD_HOST_ADMA_ATTR_INT_POS        (2u) /* ADMA Attr Int position. */
#define CY_SD_HOST_ADMA_ACT_POS             (3u) /* ADMA Act position. */
#define CY_SD_HOST_ADMA_RESERVED2_POS       (6u) /* ADMA Reserved2 position. */
#define CY_SD_HOST_ADMA_LEN_POS             (16u) /* ADMA Len position. */

/* The CMD38 arguments. */
#define CY_SD_HOST_ERASE                    (0x00000000uL)  /* The SD/eMMC Erase. */
#define CY_SD_HOST_SD_DISCARD               (0x00000001uL)  /* The SD Discard. */
#define CY_SD_HOST_SD_FUJE                  (0x00000002uL)  /* The SD Fuge. */
#define CY_SD_HOST_EMMC_DISCARD             (0x00000003uL)  /* The eMMC Discard. */
#define CY_SD_HOST_EMMC_SECURE_ERASE        (0x80000000uL)  /* The eMMC Secure Erase. */
#define CY_SD_HOST_EMMC_SECURE_TRIM_STEP_2  (0x80008000uL)  /* The eMMC Secure Trim Step 2. */
#define CY_SD_HOST_EMMC_SECURE_TRIM_STEP_1  (0x80000001uL)  /* The eMMC Secure Trim Step 1. */
#define CY_SD_HOST_EMMC_TRIM                (0x00000001uL)  /* The eMMC Trim. */

/* The CMD52 constants. */
#define CY_SD_HOST_CMD52_RWFLAG_POS         (31u)          /* The CMD52 RW Flag position. */
#define CY_SD_HOST_CMD52_FUNCT_NUM_POS      (28u)          /* The CMD52 Function Number position. */
#define CY_SD_HOST_CMD52_FUNCT_NUM_MSK      (0x07uL)       /* The CMD52 Function Number mask. */
#define CY_SD_HOST_CMD52_RAWFLAG_POS        (27u)          /* The CMD52 Raw Flag position. */
#define CY_SD_HOST_CMD52_REG_ADDR_POS       (9u)           /* The CMD52 Register Aaddress position. */
#define CY_SD_HOST_CMD52_REG_ADDR_MSK       (0x1FFFFuL)    /* The CMD52 Register Aaddress mask. */
#define CY_SD_HOST_CMD52_DATA_MSK           (0xFFuL)       /* The CMD52 data mask. */

/* The Interrupt masks. */
#define CY_SD_HOST_NORMAL_INT_MSK           (0x1FFFu)  /* The enabled Normal Interrupts. */
#define CY_SD_HOST_ERROR_INT_MSK            (0x07FFu)  /* The enabled Error Interrupts. */         
                                                         
/* The SD output clock. */
#define CY_SD_HOST_CLK_100K                 (100u * 1000u)    /* 100 kHz. */ 
#define CY_SD_HOST_CLK_400K                 (400u * 1000u)    /* 400 kHz. */
#define CY_SD_HOST_CLK_10M                  (10u * 1000u * 1000u) /* 10 MHz. */
#define CY_SD_HOST_CLK_20M                  (20u * 1000u * 1000u) /* 20 MHz. */
#define CY_SD_HOST_CLK_25M                  (25u * 1000u * 1000u) /* 25 MHz. */
#define CY_SD_HOST_CLK_40M                  (40u * 1000u * 1000u) /* 40 MHz. */
#define CY_SD_HOST_CLK_50M                  (50u * 1000u * 1000u)  /* 50 MHz. */

/** \endcond */

/** \} group_sd_host_macros */

/**
* \defgroup group_sd_host_macros_events SD Host Events
* \{
* The constants below can be used with 
* \ref Cy_SD_Host_GetNormalInterruptStatus, 
* \ref Cy_SD_Host_ClearNormalInterruptStatus, 
* \ref Cy_SD_Host_GetErrorInterruptStatus and  
* \ref Cy_SD_Host_ClearErrorInterruptStatus functions. 
* Each event is encoded in a separate bit, and therefore it is possible to
* notify about multiple events.
*/
/**
* The Command Complete. In an SD/eMMC Mode, this event is set 
* when the end bit of a response except for Auto CMD12 and Auto CMD23.
* This event is not generated when the Response Interrupt is disabled.
*/
#define CY_SD_HOST_CMD_COMPLETE            (0x0001U)

/**
* The Transfer complete. This event is set when a read/write 
* transfer and a command with status busy is completed.
*/
#define CY_SD_HOST_XFER_COMPLETE           (0x0002U)

/**
* The Block gap. This event is set when both read/write 
* transaction is stopped at block gap due to a Stop
* at Block Gap Request.
*/
#define CY_SD_HOST_BGAP                    (0x0004U)

/**
* The DMA Interrupt. This event is set if the Host Controller 
* detects the SDMA Buffer Boundary during transfer. 
* In case of ADMA, by setting the Int field in the 
* descriptor table, the Host controller generates this
* interrupt. This interrupt is not generated after a Transfer
* Complete.
*/
#define CY_SD_HOST_DMA_INTERRUPT           (0x0008U)

/**
* The Buffer write ready. This event is set if 
* the Buffer Write Enable changes from 0 to 1.
*/
#define CY_SD_HOST_BUF_WR_READY            (0x0010U)

/**
* The Buffer read ready. This event is set if 
* the Buffer Read Enable changes from 0 to 1.
*/
#define CY_SD_HOST_BUF_RD_READY            (0x0020U)

/**
* The Card insertion. This event is set if 
* the Card Inserted in the Present State
* register changes from 0 to 1.
*/
#define CY_SD_HOST_CARD_INSERTION          (0x0040U)

/**
* The Card removal. This event is set if 
* the Card Inserted in the Present State
* register changes from 1 to 0.
*/
#define CY_SD_HOST_CARD_REMOVAL            (0x0080U)

/**
* The Card interrupt. This event reflects the
* synchronized value of DAT[1] Interrupt Input for SD Mode
*/
#define CY_SD_HOST_CARD_INTERRUPT          (0x0100U)

/**
* The Command timeout error. In SD/eMMC Mode, 
* this event is set only if no response is returned 
* within 64 SD clock cycles from the end bit of the
* command. If the Host Controller detects a CMD line conflict,
* along with Command CRC Error bit, this event is set to 1,
* without waiting for 64 SD/eMMC card clock cycles. 
*/
#define CY_SD_HOST_CMD_TOUT_ERR            (0x0001U)

/**
* The Command CRC error. Command CRC Error is generated 
* in SD/eMMC mode for following two cases.
* 1. If a response is returned and the Command Timeout
* Error is set to 0 (indicating no timeout),
* this bit is set to 1 when detecting a CRC error 
* in the command response.
* 1. The Host Controller detects a CMD line conflict by
* monitoring the CMD line when a command is issued. If
* the Host Controller drives the CMD line to 1 level, but
* detects 0 level on the CMD line at the next SD clock
* edge, then the Host Controller aborts the command (stop
* driving CMD line) and set this bit to 1. The Command
* Timeout Error is also set to 1 to distinguish a CMD line
* conflict.
*/
#define CY_SD_HOST_CMD_CRC_ERR             (0x0002U)

/**
* The Command End Bit error.
* This bit is set when detecting that the end bit of a command
* response is 0 in SD/eMMC mode. 
*/
#define CY_SD_HOST_CMD_END_BIT_ERR         (0x0004U)

/**
* The Command Index error.
* This bit is set if a Command Index error occurs in the
* command respons in SD/eMMC mode. 
*/
#define CY_SD_HOST_CMD_IDX_ERR             (0x0008U)

/**
* The Data Timeout error.
* This bit is set in SD/eMMC mode when detecting one of the
* following timeout conditions:
* *  Busy timeout for R1b, R5b type
* *  Busy timeout after Write CRC status
* *  Write CRC Status timeout
* *  Read Data timeout.
*/
#define CY_SD_HOST_DATA_TOUT_ERR           (0x0010U)

/**
* The Data CRC error.
* This error occurs in SD/eMMC mode when detecting CRC
* error when transferring read data which uses the DAT line,
* when detecting the Write CRC status having a value of other
* than 010 or when write CRC status timeout.
*/
#define CY_SD_HOST_DATA_CRC_ERR            (0x0020U)

/**
* The Data End Bit error.
* This error occurs in SD/eMMC mode either when detecting 0
* at the end bit position of read data that uses the DAT line or
* at the end bit position of the CRC status.
*/
#define CY_SD_HOST_DATA_END_BIT_ERR        (0x0040U)

/**
* The Current Limit error.
* The SD Host driver does not support this function,
* this bit is always set to 0.
*/
#define CY_SD_HOST_CUR_LMT_ERR             (0x0080U)

/**
* The Auto CMD error.
* This error status is used by Auto CMD12 and Auto CMD23 in
* SD/eMMC mode. This bit is set when detecting that any of
* the bits D00 to D05 in Auto CMD Error Status register has
* changed from 0 to 1. D07 is effective in case of Auto CMD12.
* Auto CMD Error Status register is valid while this bit is set to
* 1 and may be cleared by clearing of this bit.
*/
#define CY_SD_HOST_AUTO_CMD_ERR            (0x0100U)

/**
* The ADMA error.
* This bit is set when the Host Controller detects error during
* ADMA-based data transfer. The error could be due to
* following reasons:
* * Error response received from System bus;
* * ADMA3,ADMA2 Descriptors invalid;
* * CQE Task or Transfer descriptors invalid.
* When the error occurs, the state of the ADMA is saved in the
* ADMA Error Status register.
*/
#define CY_SD_HOST_ADMA_ERR                (0x0200U)

/**
* The Tuning error.
* The SD Host driver does not support this function.
*/
#define CY_SD_HOST_TUNING_ERR              (0x0400U)

/**
* The Response error.
* Host Controller Version 4.00 supports response error check
* function to avoid overhead of response error check by Host
* Driver during DMA execution. If Response Error Check
* Enable is set to 1 in the Transfer Mode register, Host
* Controller Checks R1 or R5 response. If an error is detected
* in a response, this bit is set to 1.This is applicable in
* SD/eMMC mode.
*/
#define CY_SD_HOST_RESP_ERR                (0x0800U)


/** \} group_sd_host_macros_events */

/**
* \addtogroup group_sd_host_enums
* \{
*/

/******************************************************************************
 * Enumerations
 *****************************************************************************/
 
/** SD command types. */                 
typedef enum
{
    CY_SD_HOST_CMD_NORMAL     = 0u,  /**< Other commands */
    CY_SD_HOST_CMD_SUSPEND    = 1u,  /**< CMD52 for writing "Bus Suspend" in CCCR */
    CY_SD_HOST_CMD_RESUME     = 2u,  /**< CMD52 for writing "Function Select" in CCCR */
    CY_SD_HOST_CMD_ABORT      = 3u  /**< CMD12, CMD52 for writing "I/O Abort" in CCCR */
}cy_en_sd_host_cmd_type_t;

/** The SD Host auto command enable selection. */
typedef enum
{
    CY_SD_HOST_AUTO_CMD_NONE  = 0u,  /**< Auto command disable. */ 
    CY_SD_HOST_AUTO_CMD_12    = 1u,  /**< Auto command 12 enable. */
    CY_SD_HOST_AUTO_CMD_23    = 2u,  /**< Auto command 23 enable. */
    CY_SD_HOST_AUTO_CMD_AUTO  = 3u   /**< Auto command Auto enable. */
}cy_en_sd_host_auto_cmd_t;

/** SD Host reset types. */
typedef enum
{
    CY_SD_HOST_RESET_DATALINE = 0u, /**< Reset the data circuit only. */
    CY_SD_HOST_RESET_CMD_LINE = 1u, /**< Reset the command circuit only. */  
    CY_SD_HOST_RESET_ALL      = 2u  /**< Reset the whole SD Host controller. */ 
}cy_en_sd_host_reset_t;

/** SD Host error interrupt types. */
typedef enum
{
    Cy_SD_Host_ADMA_ST_STOP     = 0u,   /**< Stop DMA - The SYS_ADR register points to
                                        * a location next to the error descriptor.  
                                        */
    Cy_SD_Host_ADMA_ST_FDS      = 1u,   /**< Fetch Descriptor - The SYS_ADR register
                                        * points to the error descriptor. 
                                        */
    Cy_SD_Host_ADMA_ST_TFR      = 3u,   /**< Transfer Data - SYS_ADR register points
                                        * to a location next to the error descriptor. 
                                        */
    Cy_SD_Host_ADMA_LEN_ERR     = 4u,   /**< The ADMA Length Mismatch error. */
}cy_en_sd_host_adma_error_t;

/** Auto CMD Status error codes. */
typedef enum 
{
    Cy_SD_Host_AUTO_CMD12_NOT_EXEC         = 0u,  /**< Auto CMD12 Not Executed. */
    Cy_SD_Host_AUTO_CMD_TOUT_ERR           = 1u,  /**< Auto CMD Timeout Error. */
    Cy_SD_Host_AUTO_CMD_CRC_ERR            = 2u,  /**< Auto CMD CRC Error. */
    Cy_SD_Host_AUTO_CMD_EBIT_ERR           = 3u,  /**< Auto CMD End Bit Error. */
    Cy_SD_Host_AUTO_CMD_IDX_ERR            = 4u,  /**< Auto CMD Index Error. */
    Cy_SD_Host_AUTO_CMD_RESP_ERR           = 5u,  /**< Auto CMD Response Error. */
    Cy_SD_Host_CMD_NOT_ISSUED_AUTO_CMD12   = 7u,  /**< Command Not Issued By Auto CMD12 Error. */ 
} cy_en_sd_host_auto_cmd_status_t;

/** SD host error codes. */
typedef enum 
{
    Cy_SD_Host_SUCCESS                     = 0u,  /**< Successful. */
    Cy_SD_Host_ERROR                       = 1u,  /**< Non-specific error code. */
    Cy_SD_Host_ERROR_INVALID_PARAMETER     = 2u,  /**< Provided parameter is not valid. */
    Cy_SD_Host_ERROR_OPERATION_IN_PROGRESS = 3u,  /**< A conflicting or requested operation is still in progress. */
    Cy_SD_Host_ERROR_UNINITIALIZED         = 4u,  /**< Module (or part of it) was not initialized properly. */
    Cy_SD_Host_ERROR_TIMEOUT               = 5u,  /**< Time Out error occurred */
    Cy_SD_Host_OPERATION_INPROGRESS        = 6u,  /**< Indicator for operation in progress. */
    Cy_SD_Host_ERROR_UNUSABLE_CARD         = 7u, /**< The card is unusable. */
    Cy_SD_Host_ERROR_DISCONNECTED          = 8u /**< The card is disconnected. */
} cy_en_sd_host_status_t;

/** The widths of the data bus. */
typedef enum 
{
    CY_SD_HOST_BUS_WIDTH_1_BIT              = 0u,  /**< The 1-bit mode data transfer width. */
    CY_SD_HOST_BUS_WIDTH_4_BIT              = 1u,  /**< The 4-bit mode data transfer width. */
    CY_SD_HOST_BUS_WIDTH_8_BIT              = 2u  /**< The 8-bit mode data transfer width. */ 
} cy_en_sd_host_bus_width_t;

/** The bus speed modes. */
typedef enum 
{
    CY_SD_HOST_BUS_SPEED_DEFAULT            = 0u,  /**< The Default Speed mode: 3.3V signaling at 25 MHz SDClk. */
    CY_SD_HOST_BUS_SPEED_HIGHSPEED          = 1u,  /**< The High Speed mode: 3.3V signaling at 50 MHz SDClk. */
    CY_SD_HOST_BUS_SPEED_SDR12_5            = 2u,  /**< SDR12: UHS-I (1.8V signaling) at 25 MHz SDClk (12.5 MB/sec). */
    CY_SD_HOST_BUS_SPEED_SDR25              = 3u,  /**< SD25: UHS-I (1.8V signaling) at 50 MHz SDClk (25 MB/sec). */
    CY_SD_HOST_BUS_SPEED_SDR50              = 4u,  /**< SD50: UHS-I (1.8V signaling) at 100 MHz SDClk (50 MB/sec). */
    CY_SD_HOST_BUS_SPEED_EMMC_LEGACY        = 5u,  /**< Backwards Compatibility with legacy MMC card (26MB/sec max). */
    CY_SD_HOST_BUS_SPEED_EMMC_HIGHSPEED_SDR = 6u   /**< eMMC High speed SDR (52MB/sec max) */
} cy_en_sd_host_bus_speed_mode_t;

/** The SD bus voltage select. */
typedef enum 
{
    CY_SD_HOST_IO_VOLT_3_3V                 = 0u,  /**< 3.3V.*/   
    CY_SD_HOST_IO_VOLT_1_8V                 = 1u   /**< 1.8V. */
} cy_en_sd_host_io_voltage_t;

/** The erase type. */
typedef enum 
{
    CY_SD_HOST_ERASE_ERASE               = 0u,  /**< The ERASE operation.*/   
    CY_SD_HOST_ERASE_DISCARD             = 1u,  /**< The DISCARD operation. */
    CY_SD_HOST_ERASE_FUJE                = 2u,  /**< The FUJE operation. */
    CY_SD_HOST_ERASE_SECURE              = 3u,  /**< The secure purge according to 
                                                * Secure Removal Type in EXT_CSD 
                                                * on the erase groups identified by 
                                                * startAddr&endAddr parameters and 
                                                * any copies of those erase groups. 
                                                */
    CY_SD_HOST_ERASE_SECURE_TRIM_STEP_2  = 4u,  /**< The secure purge operation on 
                                                * the write blocks according to 
                                                * Secure Removal Type in EXT_CSD 
                                                * and copies of those write blocks 
                                                * that were previously identified
                                                * using \ref Cy_SD_Host_Erase with 
                                                * CY_SD_HOST_ERASE_SECURE_TRIM_STEP_1
                                                */
    CY_SD_HOST_ERASE_SECURE_TRIM_STEP_1  = 5u,  /**< Mark the write blocks, indicated 
                                                * by startAddr&endAddr parameters, 
                                                * for secure erase.
                                                */
    CY_SD_HOST_ERASE_TRIM                = 6u  /**< Trim the write blocks identified by 
                                                * startAddr&endAddr parameters. The controller 
                                                * can perform the actual erase at a convenient time.
                                                */                             
} cy_en_sd_host_erase_type_t;

/** The card type. */
typedef enum 
{
    CY_SD_HOST_SD               = 0u,  /**< The Sequre Digital card (SD). */
    CY_SD_HOST_SDIO             = 1u,  /**< The CD Input Output card (SDIO). */
    CY_SD_HOST_EMMC             = 2u,  /**< The Embedded Multimedia card (eMMC). */
    CY_SD_HOST_COMBO            = 3u,  /**< The Combo card (SD + SDIO). */
    CY_SD_HOST_UNUSABLE         = 4u   /**< The unusable or not supported. */
} cy_en_sd_host_card_type_t;

/** The card capacity type. */
typedef enum
{
    CY_SD_HOST_SDSC             = 0u,   /**< SDSC - Secure Digital Standard Capacity (up to 2 GB). */
    CY_SD_HOST_SDHC             = 1u,   /**< SDHC - Secure Digital High Capacity (up to 32 GB). */
    CY_SD_HOST_EMMC_LESS_2G     = 0u,   /**< The eMMC block addressing for less than 2GB. */
    CY_SD_HOST_EMMC_GREATER_2G  = 1u,   /**< The eMMC block addressing for greater than 2GB. */
    CY_SD_HOST_UNSUPPORTED      = 4u    /**< Not supported. */
}cy_en_sd_host_card_capacity_t;

/** SDHC response types. */ 
typedef enum
{
    CY_SD_HOST_RESPONSE_NONE    = 0u, /**< No Response. */ 
    CY_SD_HOST_RESPONSE_LEN_136 = 1u, /**< Response Length 136. */  
    CY_SD_HOST_RESPONSE_LEN_48  = 2u, /**< Response Length 48. */  
    CY_SD_HOST_RESPONSE_LEN_48B = 3u  /**< Response Length 48. Check Busy after response. */ 
}cy_en_sd_host_response_type_t;

/** The DMA type enum. */
typedef enum
{
    CY_SD_HOST_DMA_SDMA                 = 0u, /**< The SDMA mode. */          
    CY_SD_HOST_DMA_ADMA2                = 2u, /**< The ADMA2 mode. */
    CY_SD_HOST_DMA_ADMA2_ADMA3          = 3u /**< The ADMA2-ADMA3 mode. */
}cy_en_sd_host_dma_type_t;

/** \} group_sd_host_enums */


/** \cond PARAM_CHECK_MACROS */

#define CY_SD_HOST_FREQ_MAX                 (50000000uL)  /* The maximum clk frequency. */
#define CY_SD_HOST_IS_FREQ_VALID(frequency) ((0UL < (frequency)) && (CY_SD_HOST_FREQ_MAX >= (frequency)))

#define CY_SD_HOST_IS_SD_BUS_WIDTH_VALID(width)         ((CY_SD_HOST_BUS_WIDTH_1_BIT == (width)) || \
                                                         (CY_SD_HOST_BUS_WIDTH_4_BIT == (width)))
                                                     
#define CY_SD_HOST_IS_EMMC_BUS_WIDTH_VALID(width)       ((CY_SD_HOST_BUS_WIDTH_1_BIT == (width)) || \
                                                         (CY_SD_HOST_BUS_WIDTH_4_BIT == (width)) || \
                                                         (CY_SD_HOST_BUS_WIDTH_8_BIT == (width)))

#define CY_SD_HOST_IS_BUS_WIDTH_VALID(width, cardType)  ((CY_SD_HOST_EMMC == cardType) ? \
                                                         CY_SD_HOST_IS_EMMC_BUS_WIDTH_VALID(width) : \
                                                         CY_SD_HOST_IS_SD_BUS_WIDTH_VALID(width))

#define CY_SD_HOST_IS_AUTO_CMD_VALID(cmd)               ((CY_SD_HOST_AUTO_CMD_NONE == (cmd)) || \
                                                         (CY_SD_HOST_AUTO_CMD_12 == (cmd)) || \
                                                         (CY_SD_HOST_AUTO_CMD_23 == (cmd)) || \
                                                         (CY_SD_HOST_AUTO_CMD_AUTO == (cmd)))
                                                         
#define CY_SD_HOST_IS_TIMEOUT_VALID(timeout)            (CY_SD_HOST_MAX_TIMEOUT >= (timeout))   

#define CY_SD_HOST_BLK_SIZE_MAX                         (2048u)  /* The maximum block size. */
#define CY_SD_HOST_IS_BLK_SIZE_VALID(size)              (CY_SD_HOST_BLK_SIZE_MAX >= (size))

#define CY_SD_HOST_IS_ERASE_VALID(eraseType)            ((CY_SD_HOST_ERASE_ERASE == (eraseType)) || \
                                                         (CY_SD_HOST_ERASE_DISCARD == (eraseType)) || \
                                                         (CY_SD_HOST_ERASE_FUJE == (eraseType)) || \
                                                         (CY_SD_HOST_ERASE_SECURE == (eraseType)) || \
                                                         (CY_SD_HOST_ERASE_SECURE_TRIM_STEP_2 == (eraseType)) || \
                                                         (CY_SD_HOST_ERASE_SECURE_TRIM_STEP_1 == (eraseType)) || \
                                                         (CY_SD_HOST_ERASE_TRIM == (eraseType)))

#define CY_SD_HOST_IS_CMD_IDX_VALID(cmd)                (0U == ((cmd) & (uint32_t)~(CY_SD_HOST_ACMD_OFFSET_MASK | \
                                                                                    CY_SD_HOST_MMC_CMD_TAG | \
                                                                                    CY_SD_HOST_SD_ACMD_OFFSET)))

#define CY_SD_HOST_IS_DMA_VALID(dmaType)                ((CY_SD_HOST_DMA_SDMA == (dmaType)) || \
                                                         (CY_SD_HOST_DMA_ADMA2 == (dmaType)) || \
                                                         (CY_SD_HOST_DMA_ADMA2_ADMA3 == (dmaType)))

#define CY_SD_HOST_IS_SPEED_MODE_VALID(speedMode)       ((CY_SD_HOST_BUS_SPEED_DEFAULT == (speedMode)) || \
                                                         (CY_SD_HOST_BUS_SPEED_HIGHSPEED == (speedMode)) || \
                                                         (CY_SD_HOST_BUS_SPEED_SDR12_5 == (speedMode)) || \
                                                         (CY_SD_HOST_BUS_SPEED_SDR25 == (speedMode)) || \
                                                         (CY_SD_HOST_BUS_SPEED_SDR50 == (speedMode)) || \
                                                         (CY_SD_HOST_BUS_SPEED_EMMC_LEGACY == (speedMode)) || \
                                                         (CY_SD_HOST_BUS_SPEED_EMMC_HIGHSPEED_SDR == (speedMode)))
                                                         
#define CY_SD_HOST_IS_DMA_WR_RD_VALID(dmaType)          ((CY_SD_HOST_DMA_SDMA == (dmaType)) || \
                                                         (CY_SD_HOST_DMA_ADMA2 == (dmaType)))
                                                         
#define CY_SD_HOST_IS_CMD_TYPE_VALID(cmdType)            ((CY_SD_HOST_CMD_NORMAL == (cmdType)) || \
                                                         (CY_SD_HOST_CMD_SUSPEND == (cmdType)) || \
                                                         (CY_SD_HOST_CMD_RESUME == (cmdType)) || \
                                                         (CY_SD_HOST_CMD_ABORT == (cmdType)))

/** \endcond */

/**
* \addtogroup group_sd_host_data_structures
* \{
*/

/******************************************************************************
 * Structures
 *****************************************************************************/

/** The SD Host initialization configuration structure. */
typedef struct
{
    bool            emmc;             /**< Set to true of eMMC otherwise false. */
    cy_en_sd_host_dma_type_t dmaType; /**< Selects the DMA type to be used. */
    bool            enableLedControl; /**< If true the SD clock controls one IO 
                                      *    which is used to indicate when card 
                                      *    is being accessed. 
                                      */
} cy_stc_sd_host_init_config_t;

/** The SD/eMMC card configuration structure. */
typedef struct
{
    bool                          lowVoltageSignaling; /**< If true then the host supports the 1.8V signaling. */ 
    cy_en_sd_host_bus_width_t     busWidth;            /**< The desired bus width. */
    cy_en_sd_host_card_type_t     *cardType;            /**< The card type. */
    uint32_t                      *rca;                /**< The pointer to where to store the cards relative card address. */
    cy_en_sd_host_card_capacity_t *cardCapacity;       /**< Stores the card capacity. */
}cy_stc_sd_host_sd_card_config_t;

/** The SD Host command configuration structure. */
typedef struct
{
    uint32_t                   commandIndex;      /**< The index of the command. */
    uint32_t                   commandArgument;   /**< The argument for the command. */
    bool                       enableCrcCheck;    /**< Enables the CRC check on the response. */
    bool                       enableAutoResponseErrorCheck; /**< If true the hardware checks the response for errors. */
    cy_en_sd_host_response_type_t  respType;      /**< The response type. */
    bool                       enableIdxCheck;    /**< Checks the index of the response. */
    bool                       dataPresent;       /**< true: Data is present and shall 
                                                  * be transferred using the DAT line, 
                                                  * false: Commands use the CMD line only. 
                                                  */
    cy_en_sd_host_cmd_type_t   cmdType;           /**< The command type. */
} cy_stc_sd_host_cmd_config_t;

/** The SD Host data transfer configuration structure. */
typedef struct
{
    uint32_t                    blockSize;       /**< The size of the data block. */ 
    uint32_t                    numberOfBlock;   /**< The number of blocks to send. */ 
    bool                        enableDma;       /**< Enables DMA for the transaction. */ 
    cy_en_sd_host_auto_cmd_t    autoCommand;     /**< Selects which auto commands are used if any. */ 
    bool                        read;            /**< true = Read from card, false = Write to card. */ 
    uint32_t*                   data;            /**< The pointer to data to send/receive or 
                                                 *   the pointer to DMA descriptor. 
                                                 */ 
    uint32_t                    dataTimeout;     /**< The timeout value for the transfer. */ 
    bool                        enableIntAtBlockGap; /**< Enables the interrupt generation at the block gap. */ 
    bool                        enReliableWrite; /**< For EMMC enables the reliable write. */ 
}cy_stc_sd_host_data_config_t;

/** The SD Host write/read structure. */
typedef struct
{
    uint32_t*                  data;            /**< The pointer to data. */
    uint32_t                   address;         /**< The address to write/read data on the card or eMMC. */
    uint32_t                   numberOfBlocks;  /**< The number of blocks to write/read. */
    cy_en_sd_host_auto_cmd_t   autoCommand;     /**< Selects which auto commands are used if any. */
    uint32_t                   dataTimeout;     /**< The timeout value for the transfer. */
    bool                       enReliableWrite; /**< For EMMC cards enable reliable write. */
} cy_stc_sd_host_write_read_config_t;

/** The context structure. */
typedef struct
{   
    cy_en_sd_host_dma_type_t      dmaType;      /**< Defines the DMA type to be used. */
    cy_en_sd_host_card_capacity_t cardCapacity; /**< The standard card or the card with the high capacity. */
    uint32_t                      maxSectorNum; /**< The SD card maximum number of the sectors. */  
    uint32_t                      RCA;          /**< The relative card address. */
    cy_en_sd_host_card_type_t     cardType;     /**< The card type. */
    uint32_t                      csd[4];       /**< The Card-Specific Data register. */
}cy_stc_sd_host_context_t;

/** \} group_sd_host_data_structures */


/**
* \addtogroup group_sd_host_high_level_functions
* \{
*/

/******************************************************************************
* Functions
*******************************************************************************/

/* The High level section */

cy_en_sd_host_status_t Cy_SD_Host_InitCard(SDHC_Type *base, 
                                           cy_stc_sd_host_sd_card_config_t *config, 
                                           cy_stc_sd_host_context_t *context);
cy_en_sd_host_status_t Cy_SD_Host_Read(SDHC_Type *base, 
                                       cy_stc_sd_host_write_read_config_t *config,
                                       cy_stc_sd_host_context_t const *context);
cy_en_sd_host_status_t Cy_SD_Host_Write(SDHC_Type *base,
                                        cy_stc_sd_host_write_read_config_t *config,
                                        cy_stc_sd_host_context_t const *context);
cy_en_sd_host_status_t Cy_SD_Host_Erase(SDHC_Type *base, 
                                        uint32_t startAddr, 
                                        uint32_t endAddr, 
                                        cy_en_sd_host_erase_type_t eraseType, 
                                        cy_stc_sd_host_context_t const *context);
                                        
/** \} group_sd_host_high_level_functions */ 
                                        
/**
* \addtogroup group_sd_host_low_level_functions
* \{
*/

/** \cond INTERNAL_API */
                                                   

/** \endcond */

cy_en_sd_host_status_t Cy_SD_Host_SdCardChangeClock(SDHC_Type *base, uint32_t frequency);
cy_en_sd_host_status_t Cy_SD_Host_Init(SDHC_Type *base, 
                                       const cy_stc_sd_host_init_config_t* config, 
                                       cy_stc_sd_host_context_t *context);
cy_en_sd_host_status_t Cy_SD_Host_DeInit(SDHC_Type *base);
cy_en_sd_host_status_t Cy_SD_Host_Enable(SDHC_Type *base);
cy_en_sd_host_status_t Cy_SD_Host_Disable(SDHC_Type *base);
__STATIC_INLINE void Cy_SD_Host_EnableSdClk(SDHC_Type *base);
__STATIC_INLINE void Cy_SD_Host_DisableSdClk(SDHC_Type *base);
cy_en_sd_host_status_t Cy_SD_Host_SetSdClkDiv(SDHC_Type *base, uint16_t clkDiv);
__STATIC_INLINE bool Cy_SD_Host_IsWpSet(SDHC_Type const *base);
cy_en_sd_host_status_t Cy_SD_Host_SetHostBusWidth(SDHC_Type *base, 
                                                  cy_en_sd_host_bus_width_t width);
cy_en_sd_host_status_t Cy_SD_Host_SetBusWidth(SDHC_Type *base, 
                                              cy_en_sd_host_bus_width_t width,
                                              cy_stc_sd_host_context_t const *context);
cy_en_sd_host_status_t Cy_SD_Host_SetHostSpeedMode(SDHC_Type *base, 
                                                  cy_en_sd_host_bus_speed_mode_t speedMode);
cy_en_sd_host_status_t Cy_SD_Host_SetBusSpeedMode(SDHC_Type *base, 
                                                  cy_en_sd_host_bus_speed_mode_t speedMode, 
                                                  cy_stc_sd_host_context_t const *context);
cy_en_sd_host_status_t Cy_SD_Host_SelBusVoltage(SDHC_Type *base, 
                                                bool enable18VSignal, 
                                                cy_stc_sd_host_context_t *context);
__STATIC_INLINE void Cy_SD_Host_EnableCardVoltage(SDHC_Type *base);
__STATIC_INLINE void Cy_SD_Host_DisableCardVoltage(SDHC_Type *base);
cy_en_sd_host_status_t Cy_SD_Host_GetResponse(SDHC_Type const *base, 
                                              uint32_t *responsePtr, 
                                              bool largeResponse);
cy_en_sd_host_status_t Cy_SD_Host_SendCommand(SDHC_Type *base, 
                                              cy_stc_sd_host_cmd_config_t const *config);
cy_en_sd_host_status_t Cy_SD_Host_InitDataTransfer(SDHC_Type *base, 
                                                   cy_stc_sd_host_data_config_t const *dataConfig, 
                                                   cy_stc_sd_host_context_t const *context);
__STATIC_INLINE uint32_t Cy_SD_Host_BufferRead(SDHC_Type const *base);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_BufferWrite(SDHC_Type *base, uint32_t data);
__STATIC_INLINE void Cy_SD_Host_ChangeIoVoltage(SDHC_Type *base, cy_en_sd_host_io_voltage_t ioVoltage);
__STATIC_INLINE void Cy_SD_Host_StopAtBlockGap(SDHC_Type *base);
__STATIC_INLINE void Cy_SD_Host_ContinueFromBlockGap(SDHC_Type *base);
__STATIC_INLINE uint32_t Cy_SD_Host_GetAutoCmdErrStatus(SDHC_Type const *base);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_EnableAutoCmd23(SDHC_Type *base);
__STATIC_INLINE void Cy_SD_Host_DisableAutoCmd23(SDHC_Type *base);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_EnableAsyncInterrupt(SDHC_Type *base);
__STATIC_INLINE void Cy_SD_Host_DisableAsyncInterrupt(SDHC_Type *base);
__STATIC_INLINE uint32_t Cy_SD_Host_GetAdmaErrorStatus(SDHC_Type const *base);
__STATIC_INLINE void Cy_SD_Host_EMMC_Reset(SDHC_Type *base);
cy_en_sd_host_status_t Cy_SD_Host_AbortTransfer(SDHC_Type *base,
                                                cy_stc_sd_host_context_t const *context);
cy_en_sd_host_status_t Cy_SD_Host_WriteProtect(SDHC_Type *base, 
                                               bool permenantWriteProtect,
                                               cy_stc_sd_host_context_t *context);
uint32_t Cy_SD_Host_GetCardStatus(SDHC_Type *base, cy_stc_sd_host_context_t const *context);
cy_en_sd_host_status_t Cy_SD_Host_GetSdStatus(SDHC_Type *base, 
                                              uint32_t *sdStatus,
                                              cy_stc_sd_host_context_t const *context);
uint32_t Cy_SD_Host_GetOcr(SDHC_Type *base,
                           cy_stc_sd_host_context_t *context);
cy_en_sd_host_status_t Cy_SD_Host_GetCid(SDHC_Type *base, uint32_t *cid);
cy_en_sd_host_status_t Cy_SD_Host_GetCsd(SDHC_Type *base, uint32_t *csd, cy_stc_sd_host_context_t *context);
cy_en_sd_host_status_t Cy_SD_Host_GetExtCsd(SDHC_Type *base, uint32_t *extCsd, cy_stc_sd_host_context_t *context);
uint32_t Cy_SD_Host_GetRca(SDHC_Type *base);
cy_en_sd_host_status_t Cy_SD_Host_GetScr(SDHC_Type *base, uint32_t *scr, cy_stc_sd_host_context_t const *context);
uint32_t Cy_SD_Host_GetPresentState(SDHC_Type const *base);
__STATIC_INLINE bool Cy_SD_Host_IsCardConnected(SDHC_Type const *base);
void Cy_SD_Host_SoftwareReset(SDHC_Type *base, cy_en_sd_host_reset_t reset);

/** \} group_sd_host_low_level_functions */

/**
* \addtogroup group_sd_host_interrupt_functions
* \{
*/

__STATIC_INLINE uint32_t Cy_SD_Host_GetNormalInterruptStatus(SDHC_Type const *base);
__STATIC_INLINE void Cy_SD_Host_ClearNormalInterruptStatus(SDHC_Type *base, uint32_t status);
__STATIC_INLINE void Cy_SD_Host_SetNormalInterruptEnable(SDHC_Type *base, uint32_t interrupt);
__STATIC_INLINE uint32_t Cy_SD_Host_GetNormalInterruptEnable(SDHC_Type const *base);
__STATIC_INLINE void Cy_SD_Host_SetNormalInterruptMask(SDHC_Type *base, uint32_t interruptMask);
__STATIC_INLINE uint32_t Cy_SD_Host_GetNormalInterruptMask(SDHC_Type const *base);
__STATIC_INLINE uint32_t Cy_SD_Host_GetErrorInterruptStatus(SDHC_Type const *base);
__STATIC_INLINE void Cy_SD_Host_ClearErrorInterruptStatus(SDHC_Type *base, uint32_t status);
__STATIC_INLINE void Cy_SD_Host_SetErrorInterruptEnable(SDHC_Type *base, uint32_t interrupt);
__STATIC_INLINE uint32_t Cy_SD_Host_GetErrorInterruptEnable(SDHC_Type const *base);
__STATIC_INLINE void Cy_SD_Host_SetErrorInterruptMask(SDHC_Type *base, uint32_t interruptMask);
__STATIC_INLINE uint32_t Cy_SD_Host_GetErrorInterruptMask(SDHC_Type const *base);

/** \} group_sd_host_interrupt_functions */        

/**
* \addtogroup group_sd_host_low_level_functions
* \{
*/


/*******************************************************************************
* Function Name: Cy_SD_Host_EnableSdClk
****************************************************************************//**
*
*  Enables the SDCLK output (SD host drives the SDCLK line).
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_EnableSdClk(SDHC_Type *base)
{
    /* Check for NULL pointer */
    if (NULL != base) 
    {
        SDHC_CORE_CLK_CTRL_R(base) = _CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base), 
                                        SDHC_CORE_CLK_CTRL_R_SD_CLK_EN, 1u);
        
        SDHC_CORE_CLK_CTRL_R(base) = _CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base), 
                                        SDHC_CORE_CLK_CTRL_R_PLL_ENABLE, 1u);
    }
}


/*******************************************************************************
* Function Name: Cy_SD_Host_DisableSdClk
****************************************************************************//**
*
*  Disables the SDCLK output (SD host doesn't drive the SDCLK line).
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_DisableSdClk(SDHC_Type *base)
{
    /* Check for NULL pointer */
    if (NULL != base) 
    {
        SDHC_CORE_CLK_CTRL_R(base) = _CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base), 
                                        SDHC_CORE_CLK_CTRL_R_PLL_ENABLE, 0u);
                                        
        /* Wait for at least 3 card clock periods */
        Cy_SysLib_DelayUs(CY_SD_HOST_3_PERIODS_US);
        
        SDHC_CORE_CLK_CTRL_R(base) = _CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base), 
                                        SDHC_CORE_CLK_CTRL_R_SD_CLK_EN, 0u);
    }
}


/*******************************************************************************
* Function Name: Cy_SD_Host_IsWpSet
****************************************************************************//**
*
*  Returns the state of the write protect switch on the SD card.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return bool
*     true - the write protect is set, false - the write protect is not set.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SD_Host_IsWpSet(SDHC_Type const *base)
{
    return _FLD2BOOL(SDHC_CORE_PSTATE_REG_WR_PROTECT_SW_LVL, SDHC_CORE_PSTATE_REG(base));
}



/*******************************************************************************
* Function Name: Cy_SD_Host_EnableCardVoltage
****************************************************************************//**
*
*  Starts power supply on SD bus.
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_EnableCardVoltage(SDHC_Type *base)
{  
    SDHC_CORE_PWR_CTRL_R(base) = _CLR_SET_FLD8U(SDHC_CORE_PWR_CTRL_R(base), SDHC_CORE_PWR_CTRL_R_SD_BUS_PWR_VDD1, 1u);
} 


/*******************************************************************************
* Function Name: Cy_SD_Host_DisableCardVoltage
****************************************************************************//**
*
*  Stops power supply on SD bus.
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_DisableCardVoltage(SDHC_Type *base)
{
   
    SDHC_CORE_PWR_CTRL_R(base) = _CLR_SET_FLD8U(SDHC_CORE_PWR_CTRL_R(base), SDHC_CORE_PWR_CTRL_R_SD_BUS_PWR_VDD1, 0u);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_BufferRead
****************************************************************************//**
*
*  Reads 32-bit data from SD data buffer port.
*
* \param *base
*     SD host registers structure pointer.
*
* \return uint32_t
*     Data that is read.
*
*******************************************************************************/
__STATIC_INLINE uint32_t  Cy_SD_Host_BufferRead(SDHC_Type const *base)
{
    /* Return the Buffer Data Port Register value */ 
    return SDHC_CORE_BUF_DATA_R(base);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_BufferWrite
****************************************************************************//**
*
*  Writes 32-bit data to the SD data buffer port.
*
* \param *base
*     SD host registers structure pointer.
*
* \param data
*     Data to be written.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t  Cy_SD_Host_BufferWrite(SDHC_Type *base, uint32_t data)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;

    /* Check for NULL pointer */ 
    if (NULL != base)
    {
        SDHC_CORE_BUF_DATA_R(base) = data; 
        
        ret = Cy_SD_Host_SUCCESS;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_ChangeIoVoltage
****************************************************************************//**
*
*  Changes the logic level on the sd_io_volt_sel line. It assumes that 
*  this line is used to control a regulator connected to the VIDDO of the PSoC. 
*  This regulator allows for switching between 3.3V and 1.8V signaling.
*
* \param *base
*     SD host registers structure pointer.
*
* \param ioVoltage
*     The voltage for IO.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_ChangeIoVoltage(SDHC_Type *base, cy_en_sd_host_io_voltage_t ioVoltage)
{
    /* Set the 1.8V signaling enable. */
    SDHC_CORE_HOST_CTRL2_R(base) = _CLR_SET_FLD16U(SDHC_CORE_HOST_CTRL2_R(base), 
                                              SDHC_CORE_HOST_CTRL2_R_SIGNALING_EN, 
                                              (CY_SD_HOST_IO_VOLT_1_8V == ioVoltage) ? 1u : 0u);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_StopAtBlockGap
****************************************************************************//**
*
*  Stops data trasnfer during block gap (of multi-block transfer).
*
* \param *base
*     SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_StopAtBlockGap(SDHC_Type *base)
{   
    SDHC_CORE_BGAP_CTRL_R(base) = _CLR_SET_FLD8U(SDHC_CORE_BGAP_CTRL_R(base), 
                                            SDHC_CORE_BGAP_CTRL_R_STOP_BG_REQ, 
                                            1u);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_ContinueFromBlockGap
****************************************************************************//**
*
*  Restarts data transfer when transfer is pending.
*
* \param *base
*     SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_ContinueFromBlockGap(SDHC_Type *base)
{    
    SDHC_CORE_BGAP_CTRL_R(base) = _CLR_SET_FLD8U(SDHC_CORE_BGAP_CTRL_R(base), 
                                            SDHC_CORE_BGAP_CTRL_R_CONTINUE_REQ, 
                                            1u);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetAutoCmdErrStatus
****************************************************************************//**
*
*  Gets the SD host error status of the auto command.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The bit mask of the Auto Command status.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SD_Host_GetAutoCmdErrStatus(SDHC_Type const *base)
{
    uint32_t ret;

    ret = (uint32_t)SDHC_CORE_AUTO_CMD_STAT_R(base);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_EnableAutoCmd23
****************************************************************************//**
*
*  If the card supports AutoCmd23 call this function to enable it in the host.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t  Cy_SD_Host_EnableAutoCmd23(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;

    /* Check for NULL pointer */
    if (NULL != base) 
    {  
        SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base), 
                                      SDHC_CORE_XFER_MODE_R_AUTO_CMD_ENABLE, 
                                      2u);

        ret = Cy_SD_Host_SUCCESS;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_DisableAutoCmd23
****************************************************************************//**
*
* Removes support for AutoCmd23 if it was previously set.
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_DisableAutoCmd23(SDHC_Type *base)
{ 
    SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base), 
                                  SDHC_CORE_XFER_MODE_R_AUTO_CMD_ENABLE, 
                                  0u);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_EnableAsyncInterrupt
****************************************************************************//**
*
*  Enables Asynchronous Interrupt for SDIO cards. Only set this 
*  if the card supports this feature.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t  Cy_SD_Host_EnableAsyncInterrupt(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;

    /* Check for NULL pointer */
    if (NULL != base) 
    {
        SDHC_CORE_HOST_CTRL2_R(base) = _CLR_SET_FLD16U(SDHC_CORE_HOST_CTRL2_R(base), 
                              SDHC_CORE_HOST_CTRL2_R_ASYNC_INT_ENABLE, 
                              1u);
    }

    return ret;
}



/*******************************************************************************
* Function Name: Cy_SD_Host_DisableAsyncInterrupt
****************************************************************************//**
*
* Removes the support for Asynchronous Interrupt if it was previously set.
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_DisableAsyncInterrupt(SDHC_Type *base)
{ 
    SDHC_CORE_HOST_CTRL2_R(base) = _CLR_SET_FLD16U(SDHC_CORE_HOST_CTRL2_R(base), 
                                              SDHC_CORE_HOST_CTRL2_R_ASYNC_INT_ENABLE, 
                                              0u);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetAdmaErrorStatus
****************************************************************************//**
*
*  Returns the ADMA Error Status register.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The bit mask of ADMA Error Status.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SD_Host_GetAdmaErrorStatus(SDHC_Type const *base)
{
    uint32_t ret;

    ret = (uint32_t)SDHC_CORE_ADMA_ERR_STAT_R(base);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_EMMC_Reset
****************************************************************************//**
*
* Resets the eMMC card.
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_EMMC_Reset(SDHC_Type *base)
{ 
        SDHC_CORE_EMMC_CTRL_R(base) = _CLR_SET_FLD16U(SDHC_CORE_EMMC_CTRL_R(base), 
                                                 SDHC_CORE_EMMC_CTRL_R_EMMC_RST_N, 
                                                 0u);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_IsCardConnected
****************************************************************************//**
*
*  Checks to see if a card is currently connected.
*
* \param *base
*     SD host registers structure pointer.
*
* \return bool
*     true - the card is connected, false - the card is removed (not connected).
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SD_Host_IsCardConnected(SDHC_Type const *base)
{
    /* Wait until the card is stable. */
    while(true != _FLD2BOOL(SDHC_CORE_PSTATE_REG_CARD_STABLE, SDHC_CORE_PSTATE_REG(base)));

    return _FLD2BOOL(SDHC_CORE_PSTATE_REG_CARD_INSERTED, SDHC_CORE_PSTATE_REG(base));
}

/** \} group_sd_host_low_level_functions */

/**
* \addtogroup group_sd_host_interrupt_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_SD_Host_GetNormalInterruptStatus
****************************************************************************//**
*
*  Returns the normal Int Status register.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The Bit mask of the normal Int Status.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SD_Host_GetNormalInterruptStatus(SDHC_Type const *base)
{
    uint32_t ret;

    ret = (uint32_t)SDHC_CORE_NORMAL_INT_STAT_R(base);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_ClearNormalInterruptStatus
****************************************************************************//**
*
*  Clears the selected SD host normal status.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param status
*     The bitmask of statuses to clear.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_ClearNormalInterruptStatus(SDHC_Type *base, uint32_t status)
{ 
    SDHC_CORE_NORMAL_INT_STAT_R(base) = (uint16_t)status;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetNormalInterruptEnable
****************************************************************************//**
*
*  Sets the bit to be active in the Int status register.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param interrupt
*     The Bit field of which normal interrupt status to enable.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_SetNormalInterruptEnable(SDHC_Type *base, uint32_t interrupt)
{ 
    SDHC_CORE_NORMAL_INT_STAT_EN_R(base) = (uint16_t)interrupt;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetNormalInterruptEnable
****************************************************************************//**
*
*  Returns which normal interrupts are enabled.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The Bit field of which normal interrupt statuses are enabled.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SD_Host_GetNormalInterruptEnable(SDHC_Type const *base)
{
    uint32_t ret;

    ret = (uint32_t)SDHC_CORE_NORMAL_INT_STAT_EN_R(base);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetNormalInterruptMask
****************************************************************************//**
*
*  Setting a bit in this register allows the enabled status to cause an interrupt.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param interruptMask
*     The Bit filed of which normal interrupts can cause an interrupt.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_SetNormalInterruptMask(SDHC_Type *base, uint32_t interruptMask)
{ 
    SDHC_CORE_NORMAL_INT_SIGNAL_EN_R(base) = (uint16_t)interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetNormalInterruptMask
****************************************************************************//**
*
*  Returns which normal interrupts are masked to cause an interrupt.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The Bit field of which normal interrupts are masked to cause an interrupt.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SD_Host_GetNormalInterruptMask(SDHC_Type const *base)
{
    uint32_t ret;

    ret = (uint32_t)SDHC_CORE_NORMAL_INT_SIGNAL_EN_R(base);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetErrorInterruptStatus
****************************************************************************//**
*
*  Returns the error Int Status register.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The bit mask of the error Int status.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SD_Host_GetErrorInterruptStatus(SDHC_Type const *base)
{
    uint32_t ret;

    ret = (uint32_t)SDHC_CORE_ERROR_INT_STAT_R(base);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_ClearErrorInterruptStatus
****************************************************************************//**
*
*  Clears the error interrupt status.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param status
*     The bitmask of statuses to clear.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_ClearErrorInterruptStatus(SDHC_Type *base, uint32_t status)
{
    SDHC_CORE_ERROR_INT_STAT_R(base) = (uint16_t)status;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetErrorInterruptEnable
****************************************************************************//**
*
*  Setting a bit in this register allows for the bit to be active in 
*  the Int status register.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param interrupt
*     The Bit field of which error interrupt status to enable.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_SetErrorInterruptEnable(SDHC_Type *base, uint32_t interrupt)
{ 
    SDHC_CORE_ERROR_INT_STAT_EN_R(base) = (uint16_t)interrupt;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetErrorInterruptEnable
****************************************************************************//**
*
*  Returns which error interrupts are enabled.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The Bit field of which error interrupt statuses are enabled.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SD_Host_GetErrorInterruptEnable(SDHC_Type const *base)
{
    uint32_t ret;

    ret = (uint32_t)SDHC_CORE_ERROR_INT_STAT_EN_R(base);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetErrorInterruptMask
****************************************************************************//**
*
* Setting a bit in this register allows the enabled status to cause an interrupt.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param interruptMask
*     The Bit filed of which error interrupts can cause an interrupt.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SD_Host_SetErrorInterruptMask(SDHC_Type *base, uint32_t interruptMask)
{ 
    SDHC_CORE_ERROR_INT_SIGNAL_EN_R(base) = (uint16_t)interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetErrorInterruptMask
****************************************************************************//**
*
*  Returns which error interrupts are masked to cause an interrupt.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The Bit field of which error interrupts are masked to cause an interrupt.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SD_Host_GetErrorInterruptMask(SDHC_Type const *base)
{
    uint32_t ret;

    ret = (uint32_t)SDHC_CORE_ERROR_INT_SIGNAL_EN_R(base);

    return ret;
}          

/** \} group_sd_host_interrupt_functions */

#ifdef __cplusplus
}
#endif

/** \} group_sd_host */

#endif /* defined(CY_IP_MXSDHC) */

#endif /* Cy_SD_Host_PDL_H */


/* [] END OF FILE */
