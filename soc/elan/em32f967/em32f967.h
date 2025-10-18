#ifndef _EM32F867_H
#define _EM32F867_H

#define FLASH_BASE ((uint32_t)0x10000000) /*!< FLASH(up to 640K) base address in the alias region  \
					   */
#define FLASHINFO_BASE                                                                             \
	((uint32_t)0x100A0000)             /*!< Info area(32 KB) base address in the alias region */
#define IDRAM_BASE  ((uint32_t)0x20000000) /*!< IDRAM(64 KB) base address in the alias region */
#define SRAM0_BASE  ((uint32_t)0x20010000) /*!< SRAM0(176 KB) base address in the alias region */
#define SRAM1_BASE  ((uint32_t)0x2002C000) /*!< SRAM1(208 KB) base address in the alias region */
#define PERIPH_BASE ((uint32_t)0x40000000) /*!< Peripheral base address in the alias region */

/* Legacy defines */
#define SRAM_BASE IDRAM_BASE

/*!< Peripheral memory map */
#define APB1PERIPH_BASE PERIPH_BASE
#define APB2PERIPH_BASE (PERIPH_BASE + 0x00010000)
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000)
#define APB3PERIPH_BASE (PERIPH_BASE + 0x00030000)

/*!< APB1 peripherals */
#define TIMER1_BASE   (APB1PERIPH_BASE + 0x0000)
#define TIMER2_BASE   (APB1PERIPH_BASE + 0x1000)
#define UART1_BASE    (APB1PERIPH_BASE + 0x2000)
// #define SSP1_BASE             (APB1PERIPH_BASE + 0x3000)
#define I2C1_BASE     (APB1PERIPH_BASE + 0x4000)
// #define USB_BASE              (APB1PERIPH_BASE + 0x5000)
#define PWM_BASE      (APB1PERIPH_BASE + 0x6000)
// #define UART3_BASE            (APB1PERIPH_BASE + 0x7000)
// #define BLDC_BASE             (APB1PERIPH_BASE + 0x8000)
// #define SPI1_BASE             (APB1PERIPH_BASE + 0x9000)
#define ISO78161_BASE (APB1PERIPH_BASE + 0xA000)
#define SWSPI_BASE    (APB1PERIPH_BASE + 0xB000)

/*!< APB2 peripherals */
#define TIMER3_BASE   (APB2PERIPH_BASE + 0x0000)
#define TIMER4_BASE   (APB2PERIPH_BASE + 0x1000)
#define UART2_BASE    (APB2PERIPH_BASE + 0x2000)
#define SSP2_BASE     (APB2PERIPH_BASE + 0x3000)
#define I2C2_BASE     (APB2PERIPH_BASE + 0x4000)
// #define AES256_BASE           (APB2PERIPH_BASE + 0x5000)
#define ENCRYPT_BASE  (APB2PERIPH_BASE + 0x6000)
#define ECC256_BASE   (APB2PERIPH_BASE + 0x7000)
#define TRNG_BASE     (APB2PERIPH_BASE + 0x8000)
#define USART_BASE    (APB2PERIPH_BASE + 0x9000)
#define ISO78162_BASE (APB2PERIPH_BASE + 0xA000)

/*!< APB3 peripherals */
#define SYSCFG_BASE   (APB3PERIPH_BASE + 0x0000)
#define PWR_BASE      (APB3PERIPH_BASE + 0x1000)
#define RTC_BASE      (APB3PERIPH_BASE + 0x2000)
#define BACKUP_BASE   (APB3PERIPH_BASE + 0x3000)
#define FLASHIFR_BASE (APB3PERIPH_BASE + 0x4000)
#define WDG_BASE      (APB3PERIPH_BASE + 0x5000)
#define AIP_BASE      (AHB3PERIPH_BASE + 0x6000)
#define CACHE_BASE    (AHB3PERIPH_BASE + 0x7000)
#define USB_BASE      (APB3PERIPH_BASE + 0x8000)

/*!< AHB1 peripherals */
#define GPIOA_BASE   (AHB1PERIPH_BASE + 0x0000)
#define GPIOB_BASE   (AHB1PERIPH_BASE + 0x1000)
#define DMA_BASE     (AHB1PERIPH_BASE + 0x2000)
#define SYSCTRL_BASE (AHB1PERIPH_BASE + 0x3000)
#define QSPI_BASE    (AHB1PERIPH_BASE + 0x4000)
#define SPI1_BASE    (AHB1PERIPH_BASE + 0x7000)

// WDT register
#define WDOGLOAD    (*(__IO uint32_t *)0x40035000)
#define WDOGVALUE   (*(__IO uint32_t *)0x40035004)
#define WDOGCONTROL (*(__IO uint32_t *)0x40035008)
#define WDOGINTCLR  (*(__IO uint32_t *)0x4003500c)
#define WDOGRIS     (*(__IO uint32_t *)0x40035010)
#define WDOGMIS     (*(__IO uint32_t *)0x40035014)
#define WDOGLOCK    (*(__IO uint32_t *)0x40035c00)

#endif
