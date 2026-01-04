/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BOOT_HAL_CFG_H
#define BOOT_HAL_CFG_H

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* RTC clock */
#define  RTC_CLOCK_SOURCE_LSI
#ifdef RTC_CLOCK_SOURCE_LSI
#define RTC_ASYNCH_PREDIV  0x7F
#define RTC_SYNCH_PREDIV   0x00F9
#endif
#ifdef RTC_CLOCK_SOURCE_LSE
#define RTC_ASYNCH_PREDIV  0x7F
#define RTC_SYNCH_PREDIV   0x00FF
#endif

/* ICache */
#define TFM_ICACHE_ENABLE /*!< Instruction cache enable */

/* Static protections */
#define TFM_WRP_PROTECT_ENABLE  /*!< Write Protection  */
#define TFM_HDP_PROTECT_ENABLE /*!< HDP protection   */
#define TFM_SECURE_USER_SRAM2_ERASE_AT_RESET /*!< SRAM2 clear at Reset  */


#define TFM_OB_RDP_LEVEL_VALUE OB_RDP_LEVEL_0 /*!< RDP level */


#define NO_TAMPER		(0)
#define INTERNAL_TAMPER_ONLY	(1)
#define ALL_TAMPER		(2)
#define TFM_TAMPER_ENABLE	NO_TAMPER

#define TFM_OB_BOOT_LOCK	0
#define TFM_ENABLE_SET_OB
#define TFM_ERROR_HANDLER_NON_SECURE

/* Run time protections */
#define TFM_FLASH_PRIVONLY_ENABLE
#define TFM_BOOT_MPU_PROTECTION

/* Exported types ------------------------------------------------------------*/
typedef enum {
	TFM_SUCCESS = 0U,
	TFM_FAILED
} TFM_ErrorStatus;

void Error_Handler(void);
#ifndef TFM_ERROR_HANDLER_NON_SECURE
void Error_Handler_rdp(void);
#else
#define Error_Handler_rdp Error_Handler
#endif
#endif /* GENERATOR_RDP_PASSWORD_AVAILABLE */
