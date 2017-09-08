#ifndef _HAL_SDMMC_H_
#define _HAL_SDMMC_H_

// =============================================================================
// MACROS
// =============================================================================
// =============================================================================
// HAL_SDMMC_ACMD_SEL
// -----------------------------------------------------------------------------
/// Macro to mark a command as application specific
// =============================================================================
#define HAL_SDMMC_ACMD_SEL 0x80000000

// =============================================================================
// HAL_SDMMC_CMD_MASK
// -----------------------------------------------------------------------------
/// Mask to get from a HAL_SDMMC_CMD_T value the corresponding
/// command index
// =============================================================================
#define HAL_SDMMC_CMD_MASK 0x3F

// =============================================================================
// HAL_SDMMC_CMD_T
// -----------------------------------------------------------------------------
// SD commands
// =============================================================================
typedef enum
{
	HAL_SDMMC_CMD_GO_IDLE_STATE         = 0,
	HAL_SDMMC_CMD_MMC_SEND_OP_COND      = 1,
	HAL_SDMMC_CMD_ALL_SEND_CID            = 2,
	HAL_SDMMC_CMD_SEND_RELATIVE_ADDR    = 3,
	HAL_SDMMC_CMD_SET_DSR                = 4,
	HAL_SDMMC_CMD_SWITCH           = 6,
	HAL_SDMMC_CMD_SELECT_CARD            = 7,
	HAL_SDMMC_CMD_SEND_IF_COND          = 8,
	HAL_SDMMC_CMD_SEND_CSD                = 9,
	HAL_SDMMC_CMD_STOP_TRANSMISSION        = 12,
	HAL_SDMMC_CMD_SEND_STATUS            = 13,
	HAL_SDMMC_CMD_SET_BLOCKLEN            = 16,
	HAL_SDMMC_CMD_READ_SINGLE_BLOCK        = 17,
	HAL_SDMMC_CMD_READ_MULT_BLOCK        = 18,
	HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK    = 24,
	HAL_SDMMC_CMD_WRITE_MULT_BLOCK        = 25,
	HAL_SDMMC_CMD_APP_CMD                = 55,
	HAL_SDMMC_CMD_SET_BUS_WIDTH            = (6 | HAL_SDMMC_ACMD_SEL),
	HAL_SDMMC_CMD_SEND_NUM_WR_BLOCKS    = (22| HAL_SDMMC_ACMD_SEL),
	HAL_SDMMC_CMD_SET_WR_BLK_COUNT        = (23| HAL_SDMMC_ACMD_SEL),
	HAL_SDMMC_CMD_SEND_OP_COND            = (41| HAL_SDMMC_ACMD_SEL)
} HAL_SDMMC_CMD_T;

// =============================================================================
// HAL_SDMMC_OP_STATUS_T
// -----------------------------------------------------------------------------
/// This structure is used the module operation status. It is different from the
/// IRQ status.
// =============================================================================
typedef union
{
	uint32_t reg;
	struct
	{
		uint32_t operation_not_over     :1;
		/// module is busy ?
		uint32_t busy                 :1;
		uint32_t data_line_busy         :1;
		/// '1' means the controller will not perform the new command when sdmmc_sendcmd= '1'.
		uint32_t suspend              :1;
		uint32_t                      :4;
		uint32_t response_crc_error     :1;
		/// 1 as long as no reponse to a command has been received.
		uint32_t noresponse_received   :1;
		uint32_t                      :2;
		/// crc check for sd/mmc write operation
		/// "101" transmission error
		/// "010" transmission right
		/// "111" flash programming error
		uint32_t crc_status            :3;
		uint32_t                      :1;
		/// 8 bits data crc check, "00000000" means no data error,
		/// "00000001" means data0 crc check error, "10000000" means
		/// data7 crc check error, each bit match one data line.
		uint32_t data_error            :8;
	} fields;
} hal_sdmmc_op_status_t;

// =============================================================================
// hal_sdmmc_dir_t
// -----------------------------------------------------------------------------
/// Describe the direction of a transfer between the SDmmc controller and a
/// pluggued card.
// =============================================================================
typedef enum
{
	HAL_SDMMC_DIRECTION_READ,
	HAL_SDMMC_DIRECTION_WRITE,
	HAL_SDMMC_DIRECTION_QTY
}hal_sdmmc_dir_t;

// =============================================================================
// HAL_SDMMC_TRANSFER_T
// -----------------------------------------------------------------------------
/// Describe a transfer between the SDmmc module and the SD card
// =============================================================================
struct hal_sdmmc_transfer {
	/// This address in the system memory
	uint8_t* sys_mem_addr;
	/// Address in the SD card
	uint8_t* sdcard_addr;
	/// Quantity of data to transfer, in blocks
	uint32_t block_num;
	/// Block size
	uint32_t block_size;
	hal_sdmmc_dir_t direction;
} ;

// =============================================================================
// HAL_SDMMC_DATA_BUS_WIDTH_T
// -----------------------------------------------------------------------------
/// Cf spec v2 pdf p. 76 for ACMD6 argument
/// That type is used to describe how many data lines are used to transfer data
/// to and from the SD card.
// =============================================================================
typedef enum
{
	HAL_SDMMC_DATA_BUS_WIDTH_1 = 0x0,
	HAL_SDMMC_DATA_BUS_WIDTH_4 = 0x2,
	HAL_SDMMC_DATA_BUS_WIDTH_8 = 0x4
} HAL_SDMMC_DATA_BUS_WIDTH_T;

//=============================================================================

//
/**
 * @brief Configuration of the Cortex-M4 Processor and Core Peripherals
 */
#define __CM4_REV                 0x0001U  /*!< Core revision r0p1                            */
#define __MPU_PRESENT             0U       /*!< STM32F301x8 devices do not provide an MPU */
#define __NVIC_PRIO_BITS          4U       /*!< STM32F301x8 devices use 4 Bits for the Priority Levels */
#define __Vendor_SysTickConfig    0U       /*!< Set to 1 if different SysTick Config is used */
#define __FPU_PRESENT             1U       /*!< STM32F301x8 devices provide an FPU */


typedef enum IRQn
{
/******  Cortex-M4 Processor Exceptions Numbers ***************************************************/
	NonMaskableInt_IRQn           = -14,      /*!< 2 Non Maskable Interrupt                         */
	MemoryManagement_IRQn         = -12,      /*!< 4 Cortex-M4 Memory Management Interrupt          */
	BusFault_IRQn                 = -11,      /*!< 5 Cortex-M4 Bus Fault Interrupt                  */
	UsageFault_IRQn               = -10,      /*!< 6 Cortex-M4 Usage Fault Interrupt                */
	SVCall_IRQn                   = -5,       /*!< 11 Cortex-M4 SV Call Interrupt                   */
	DebugMonitor_IRQn             = -4,       /*!< 12 Cortex-M4 Debug Monitor Interrupt             */
	PendSV_IRQn                   = -2,       /*!< 14 Cortex-M4 Pend SV Interrupt                   */
	SysTick_IRQn                  = -1,       /*!< 15 Cortex-M4 System Tick Interrupt               */

/******  RDA5981A Specific Interrupt Numbers ******************************************************/
	SPIFLASH_IRQn                 = 0,        /*!< SPI Flash Interrupt                              */
	PTA_IRQn                      = 1,        /*!< PTA Interrupt                                    */
	SDIO_IRQn                     = 2,        /*!< SDIO Interrupt                                   */
	USBDMA_IRQn                   = 3,        /*!< USBDMA Interrupt                                 */
	USB_IRQn                      = 4,        /*!< USB Interrupt                                    */
	GPIO_IRQn                     = 5,        /*!< GPIO Interrupt                                   */
	TIMER_IRQn                    = 6,        /*!< Timer Interrupt                                  */
	UART0_IRQn                    = 7,        /*!< UART0 Interrupt                                  */
	MACHW_IRQn                    = 8,        /*!< MAC Hardware Interrupt                           */
	UART1_IRQn                    = 9,        /*!< UART1 Interrupt                                  */
	AHBDMA_IRQn                   = 10,       /*!< AHBDMA Interrupt                                 */
	PSRAM_IRQn                    = 11,       /*!< PSRAM Interrupt                                  */
	SDMMC_IRQn                    = 12,       /*!< SDMMC Interrupt                                  */
	EXIF_IRQn                     = 13,       /*!< EXIF Interrupt                                   */
	I2C_IRQn                      = 14        /*!< I2C Interrupt                                    */
} IRQn_Type;

#include "core_cm4.h"

#endif // _HAL_SDMMC_H_

