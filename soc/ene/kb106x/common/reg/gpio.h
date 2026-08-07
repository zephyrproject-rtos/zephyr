/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_GPIO_H
#define ENE_KB106X_GPIO_H

/**
 *  Structure type to access General Purpose Input/Output (GPIO).
 */
struct gpio_regs {
	volatile uint32_t GPIOFS; /*Function Selection Register */
	volatile uint32_t Reserved1[3];
	volatile uint32_t GPIOOE; /*Output Enable Register */
	volatile uint32_t Reserved2[3];
	volatile uint32_t GPIOD; /*Output Data Register */
	volatile uint32_t Reserved3[3];
	volatile uint32_t GPIOIN; /*Input Data Register */
	volatile uint32_t Reserved4[3];
	volatile uint32_t GPIOPU; /*Pull Up Register */
	volatile uint32_t Reserved5[3];
	volatile uint32_t GPIOOD; /*Open Drain Register */
	volatile uint32_t Reserved6[3];
	volatile uint32_t GPIOIE; /*Input Enable Register */
	volatile uint32_t Reserved7[3];
	volatile uint32_t GPIODC; /*Driving Control Register */
	volatile uint32_t Reserved8[3];
	volatile uint32_t GPIOLV; /*Low Voltage Mode Enable Register */
	volatile uint32_t Reserved9[3];
	volatile uint32_t GPIOPD; /*Pull Down Register */
	volatile uint32_t Reserved10[3];
	volatile uint32_t GPIOFL; /*Function Lock Register */
	volatile uint32_t Reserved11[3];
};

#define NUM_KB106X_GPIO_PORTS 4

/*-- Constant Define --------------------------------------------*/
#define GPIO0B_ESBCLK_SCL5             0x0B
#define GPIO0C_ESBDAT_SDA5             0x0C
#define GPIO0D_RLCTX2_SDA4             0x0D
#define GPIO16_SERTXD_UARTSOUT_SBUDCLK 0x16
#define GPIO17_SERRXD_UARTSIN_SBUDDAT  0x17
#define GPIO19_PWM3_PWMLED0            0x19
#define GPIO30_SERTXD_NKROKSI0         0x30
#define GPIO48_KSO16_UART_SOUT2        0x48
#define GPIO4C_PSCLK2_SCL3             0x4C
#define GPIO4D_SDAT2_SDA3              0x4D
#define GPIO4E_PSCLK3_KSO18            0x4E
#define GPIO4F_PSDAT3_KSO19            0x4F
#define GPIO4A_PSCLK1_SCL2_USBDM       0x4A
#define GPIO4B_PSDAT1_SDA2_USBDP       0x4B

#define GPIO60_SHICS      0x60
#define GPIO61_SHICLK     0x61
#define GPIO62_SHIDO      0x62
#define GPIO78_SHIDI      0x78
#define GPIO5A_SHR_SPICS  0x5A
#define GPIO58_SHR_SPICLK 0x58
#define GPIO5C_SHR_MOSI   0x5C
#define GPIO5B_SHR_MISO   0x5B

#define GPIO01_ESPI_ALERT 0x01
#define GPIO03_ESPI_CS    0x03
#define GPIO07_ESPI_RST   0x07

#endif /* ENE_KB106X_GPIO_H */
