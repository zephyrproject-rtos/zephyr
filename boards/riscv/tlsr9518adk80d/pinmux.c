/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <drivers/pinmux.h>


/**
 *  @brief  Define UART0 pins : TX(PA3 PB2 PD2), RX(PA4 PB3 PD3)
 */
#define UART0_TX_PA3            GPIO_PA3
#define UART0_TX_PA3_FUNC       PINMUX_FUNC_B

#define UART0_TX_PB2            GPIO_PB2
#define UART0_TX_PB2_FUNC       PINMUX_FUNC_C

#define UART0_TX_PD2            GPIO_PD2
#define UART0_TX_PD2_FUNC       PINMUX_FUNC_A


#define UART0_RX_PA4            GPIO_PA4
#define UART0_RX_PA4_FUNC       PINMUX_FUNC_B

#define UART0_RX_PB3            GPIO_PB3
#define UART0_RX_PB3_FUNC       PINMUX_FUNC_C

#define UART0_RX_PD3            GPIO_PD3
#define UART0_RX_PD3_FUNC       PINMUX_FUNC_A


/**
 *  @brief  Define UART1 pins: TX(PC6 PD6 PE0), RX(PC7 PD7 PE2)
 */
#define UART1_TX_PC6            GPIO_PC6
#define UART1_TX_PC6_FUNC       PINMUX_FUNC_C

#define UART1_TX_PD6            GPIO_PD6
#define UART1_TX_PD6_FUNC       PINMUX_FUNC_A

#define UART1_TX_PE0            GPIO_PE0
#define UART1_TX_PE0_FUNC       PINMUX_FUNC_B


#define UART1_RX_PC7            GPIO_PC7
#define UART1_RX_PC7_FUNC       PINMUX_FUNC_C

#define UART1_RX_PD7            GPIO_PD7
#define UART1_RX_PD7_FUNC       PINMUX_FUNC_A

#define UART1_RX_PE2            GPIO_PE2
#define UART1_RX_PE2_FUNC       PINMUX_FUNC_B


/**
 *  @brief  Define PWM Channel 0 pins
 */
#define PWM_CH0_PB4             GPIO_PB4
#define PWM_CH0_PB4_FUNC        PINMUX_FUNC_B

#define PWM_CH0_PC0             GPIO_PC0
#define PWM_CH0_PC0_FUNC        PINMUX_FUNC_C

#define PWM_CH0_PE3             GPIO_PE3
#define PWM_CH0_PE3_FUNC        PINMUX_FUNC_C


/**
 *  @brief  Define PWM Channel 1 pins
 */
#define PWM_CH1_PB5             GPIO_PB5
#define PWM_CH1_PB5_FUNC        PINMUX_FUNC_C

#define PWM_CH1_PE1             GPIO_PE1
#define PWM_CH1_PE1_FUNC        PINMUX_FUNC_C


/**
 *  @brief  Define PWM Channel 2 pins
 */
#define PWM_CH2_PB7             GPIO_PB7
#define PWM_CH2_PB7_FUNC        PINMUX_FUNC_A

#define PWM_CH2_PE2             GPIO_PE2
#define PWM_CH2_PE2_FUNC        PINMUX_FUNC_C


/**
 *  @brief  Define PWM Channel 3 pins
 */
#define PWM_CH3_PB1             GPIO_PB1
#define PWM_CH3_PB1_FUNC        PINMUX_FUNC_C

#define PWM_CH3_PE0             GPIO_PE0
#define PWM_CH3_PE0_FUNC        PINMUX_FUNC_C


/**
 *  @brief  Define PWM Channel 4 pins
 */
#define PWM_CH4_PD7             GPIO_PD7
#define PWM_CH4_PD7_FUNC        PINMUX_FUNC_B

#define PWM_CH4_PE4             GPIO_PE4
#define PWM_CH4_PE4_FUNC        PINMUX_FUNC_C

/**
 *  @brief  Define PWM Channel 5 pins
 */
#define PWM_CH5_PB0             GPIO_PB0
#define PWM_CH5_PB0_FUNC        PINMUX_FUNC_C

#define PWM_CH5_PE5             GPIO_PE5
#define PWM_CH5_PE5_FUNC        PINMUX_FUNC_C

/**
 *  @brief  Define PSPI pins: CLK(PC5, PB5, PD1), MOSI(PC7, PB7, PD3), MISO(PC6, PB6, PD2)
 */
#define PSPI_CLK_PC5            GPIO_PC5
#define PSPI_CLK_PC5_FUNC       PINMUX_FUNC_A

#define PSPI_MOSI_IO0_PC7       GPIO_PC7
#define PSPI_MOSI_IO0_PC7_FUNC  PINMUX_FUNC_A

#define PSPI_MISO_IO1_PC6       GPIO_PC6
#define PSPI_MISO_IO1_PC6_FUNC  PINMUX_FUNC_A


#define PSPI_CLK_PB5            GPIO_PB5
#define PSPI_CLK_PB5_FUNC       PINMUX_FUNC_B

#define PSPI_MOSI_IO0_PB7       GPIO_PB7
#define PSPI_MOSI_IO0_PB7_FUNC  PINMUX_FUNC_B

#define PSPI_MISO_IO1_PB6       GPIO_PB6
#define PSPI_MISO_IO1_PB6_FUNC  PINMUX_FUNC_B


#define PSPI_CLK_PD1            GPIO_PD1
#define PSPI_CLK_PD1_FUNC       PINMUX_FUNC_A

#define PSPI_MOSI_IO0_PD3       GPIO_PD3
#define PSPI_MOSI_IO0_PD3_FUNC  PINMUX_FUNC_A

#define PSPI_MISO_IO1_PD2       GPIO_PD2
#define PSPI_MISO_IO1_PD2_FUNC  PINMUX_FUNC_A


/**
 *  @brief  Define HSPI pins: CLK(PA2, PB4), MOSI(PA4, PB3), MISO(PA3, PB2), IO02(PB1), IO03(PB0)
 */
#define HSPI_CLK_PA2            GPIO_PA2
#define HSPI_CLK_PA2_FUNC       PINMUX_FUNC_C

#define HSPI_MOSI_IO0_PA4       GPIO_PA4
#define HSPI_MOSI_IO0_PA4_FUNC  PINMUX_FUNC_C

#define HSPI_MISO_IO1_PA3       GPIO_PA3
#define HSPI_MISO_IO1_PA3_FUNC  PINMUX_FUNC_C


#define HSPI_CLK_PB4            GPIO_PB4
#define HSPI_CLK_PB4_FUNC       PINMUX_FUNC_A

#define HSPI_MOSI_IO0_PB3       GPIO_PB3
#define HSPI_MOSI_IO0_PB3_FUNC  PINMUX_FUNC_A

#define HSPI_MISO_IO1_PB2       GPIO_PB2
#define HSPI_MISO_IO1_PB2_FUNC  PINMUX_FUNC_A

#define HSPI_IO2_PB1            GPIO_PB1
#define HSPI_IO2_PB1_FUNC       PINMUX_FUNC_A

#define HSPI_IO3_PB0            GPIO_PB0
#define HSPI_IO3_PB0_FUNC       PINMUX_FUNC_A


/**
 *  @brief  Define I2C pins: SCL(PB2, PC1, PE0, PE1), SDA(PB3, PC2, PE2, PE3)
 */
#define I2C_SCL_PB2             GPIO_PB2
#define I2C_SCL_PB2_FUNC        PINMUX_FUNC_B

#define I2C_SCL_PC1             GPIO_PC1
#define I2C_SCL_PC1_FUNC        PINMUX_FUNC_A

#define I2C_SCL_PE0             GPIO_PE0
#define I2C_SCL_PE0_FUNC        PINMUX_FUNC_A

#define I2C_SCL_PE1             GPIO_PE1
#define I2C_SCL_PE1_FUNC        PINMUX_FUNC_A


#define I2C_SDA_PB3             GPIO_PB3
#define I2C_SDA_PB3_FUNC        PINMUX_FUNC_B

#define I2C_SDA_PC2             GPIO_PC2
#define I2C_SDA_PC2_FUNC        PINMUX_FUNC_A

#define I2C_SDA_PE2             GPIO_PE2
#define I2C_SDA_PE2_FUNC        PINMUX_FUNC_A

#define I2C_SDA_PE3             GPIO_PE3
#define I2C_SDA_PE3_FUNC        PINMUX_FUNC_A


/**
 *  @brief  Config board pins
 */
static int b91_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct device *pinmux =
		device_get_binding(DT_LABEL(DT_NODELABEL(pinmux)));

	/* select func2_sub_select */
	reg_gpio_pad_mul_sel |= BIT(0);

	/* UART0 RX, TX */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) && CONFIG_UART_B91
	pinmux_pin_set(pinmux, UART0_TX_PB2, UART0_TX_PB2_FUNC);
	pinmux_pin_set(pinmux, UART0_RX_PB3, UART0_RX_PB3_FUNC);
#endif

	/* UART1 RX, TX */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) && CONFIG_UART_B91
	pinmux_pin_set(pinmux, UART1_TX_PC6, UART1_TX_PC6_FUNC);
	pinmux_pin_set(pinmux, UART1_RX_PC7, UART1_RX_PC7_FUNC);
#endif

	/* PWM */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm0), okay) && CONFIG_PWM_B91
	pinmux_pin_set(pinmux, PWM_CH0_PB4, PWM_CH0_PB4_FUNC);
#endif

	/* PSPI */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pspi), okay) && CONFIG_SPI_B91
	/* CS pin is configured by SPI driver based on Device Tree selection */
	pinmux_pin_set(pinmux, PSPI_CLK_PC5,  PSPI_CLK_PC5_FUNC);
	pinmux_pin_set(pinmux, PSPI_MOSI_IO0_PC7, PSPI_MOSI_IO0_PC7_FUNC);
	pinmux_pin_set(pinmux, PSPI_MISO_IO1_PC6, PSPI_MISO_IO1_PC6_FUNC);
#endif

	/* HSPI */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(hspi), okay) && CONFIG_SPI_B91
	reg_gpio_pad_mul_sel |= BIT(1);
	/* CS pin is configured by SPI driver based on Device Tree selection */
	pinmux_pin_set(pinmux, HSPI_CLK_PA2,  HSPI_CLK_PA2_FUNC);
	pinmux_pin_set(pinmux, HSPI_MOSI_IO0_PA4, HSPI_MOSI_IO0_PA4_FUNC);
	pinmux_pin_set(pinmux, HSPI_MISO_IO1_PA3, HSPI_MISO_IO1_PA3_FUNC);
#endif

	/* I2C */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c), okay) && CONFIG_I2C_B91
	pinmux_pin_set(pinmux, I2C_SCL_PE1, I2C_SCL_PE1_FUNC);
	pinmux_pin_set(pinmux, I2C_SDA_PE3, I2C_SDA_PE3_FUNC);
	pinmux_pin_pullup(pinmux, I2C_SCL_PE1, PINMUX_PULLUP_ENABLE);
	pinmux_pin_pullup(pinmux, I2C_SDA_PE3, PINMUX_PULLUP_ENABLE);
#endif

	return 0;
}

SYS_INIT(b91_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
