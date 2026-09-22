/*
 * Copyright (c) 2023 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_emsdp_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/emsdp-pinctrl.h>

/**
 * Mux Control Register Index
 */
#define PMOD_MUX_CTRL 0 /*!< 32-bits, offset 0x0 */
#define ARDUINO_MUX_CTRL 4 /*!< 32-bits, offset 0x4 */

#define EMSDP_CREG_BASE            DT_INST_REG_ADDR(0)
#define EMSDP_CREG_PMOD_MUX_OFFSET (0x0030)

#define MUX_SEL0_OFFSET (0)
#define MUX_SEL1_OFFSET (4)
#define MUX_SEL2_OFFSET (8)
#define MUX_SEL3_OFFSET (12)
#define MUX_SEL4_OFFSET (16)
#define MUX_SEL5_OFFSET (20)
#define MUX_SEL6_OFFSET (24)
#define MUX_SEL7_OFFSET (28)

#define MUX_SEL0_MASK (0xf << MUX_SEL0_OFFSET)
#define MUX_SEL1_MASK (0xf << MUX_SEL1_OFFSET)
#define MUX_SEL2_MASK (0xf << MUX_SEL2_OFFSET)
#define MUX_SEL3_MASK (0xf << MUX_SEL3_OFFSET)
#define MUX_SEL4_MASK (0xf << MUX_SEL4_OFFSET)
#define MUX_SEL5_MASK (0xf << MUX_SEL5_OFFSET)
#define MUX_SEL6_MASK (0xf << MUX_SEL6_OFFSET)
#define MUX_SEL7_MASK (0xf << MUX_SEL7_OFFSET)

/**
 * PMOD A Multiplexor
 */
#define PM_A_CFG0_GPIO	 ((0) << MUX_SEL0_OFFSET)
#define PM_A_CFG0_I2C	 ((1) << MUX_SEL0_OFFSET) /* io_i2c_mst2 */
#define PM_A_CFG0_SPI	 ((2) << MUX_SEL0_OFFSET) /* io_spi_mst1, cs_0 */
#define PM_A_CFG0_UART1a ((3) << MUX_SEL0_OFFSET) /* io_uart1 */
#define PM_A_CFG0_UART1b ((4) << MUX_SEL0_OFFSET) /* io_uart1 */
#define PM_A_CFG0_PWM1	 ((5) << MUX_SEL0_OFFSET)
#define PM_A_CFG0_PWM2	 ((6) << MUX_SEL0_OFFSET)

#define PM_A_CFG1_GPIO ((0) << MUX_SEL1_OFFSET)

/**
 * PMOD B Multiplexor
 */
#define PM_B_CFG0_GPIO	 ((0) << MUX_SEL2_OFFSET)
#define PM_B_CFG0_I2C	 ((1) << MUX_SEL2_OFFSET) /* io_i2c_mst2 */
#define PM_B_CFG0_SPI	 ((2) << MUX_SEL2_OFFSET) /* io_spi_mst1, cs_1 */
#define PM_B_CFG0_UART2a ((3) << MUX_SEL2_OFFSET) /* io_uart2 */
#define PM_B_CFG0_UART2b ((4) << MUX_SEL2_OFFSET) /* io_uart2 */
#define PM_B_CFG0_PWM1	 ((5) << MUX_SEL2_OFFSET)
#define PM_B_CFG0_PWM2	 ((6) << MUX_SEL2_OFFSET)

#define PM_B_CFG1_GPIO ((0) << MUX_SEL3_OFFSET)

/**
 * PMOD C Multiplexor
 */
#define PM_C_CFG0_GPIO	 ((0) << MUX_SEL4_OFFSET)
#define PM_C_CFG0_I2C	 ((1) << MUX_SEL4_OFFSET) /* io_i2c_mst2 */
#define PM_C_CFG0_SPI	 ((2) << MUX_SEL4_OFFSET) /* io_spi_mst1, cs_2 */
#define PM_C_CFG0_UART3a ((3) << MUX_SEL4_OFFSET) /* io_uart3 */
#define PM_C_CFG0_UART3b ((4) << MUX_SEL4_OFFSET) /* io_uart3 */
#define PM_C_CFG0_PWM1	 ((5) << MUX_SEL4_OFFSET)
#define PM_C_CFG0_PWM2	 ((6) << MUX_SEL4_OFFSET)

#define PM_C_CFG1_GPIO ((0) << MUX_SEL5_OFFSET)

/**
 * Arduino Multiplexor
 */
#define ARDUINO_CFG0_GPIO ((0) << MUX_SEL0_OFFSET)
#define ARDUINO_CFG0_UART ((1) << MUX_SEL0_OFFSET) /* io_uart0 */

#define ARDUINO_CFG1_GPIO ((0) << MUX_SEL1_OFFSET)
#define ARDUINO_CFG1_PWM  ((1) << MUX_SEL1_OFFSET)

#define ARDUINO_CFG2_GPIO ((0) << MUX_SEL2_OFFSET)
#define ARDUINO_CFG2_PWM  ((1) << MUX_SEL2_OFFSET)

#define ARDUINO_CFG3_GPIO ((0) << MUX_SEL3_OFFSET)
#define ARDUINO_CFG3_PWM  ((1) << MUX_SEL3_OFFSET)

#define ARDUINO_CFG4_GPIO ((0) << MUX_SEL4_OFFSET)
#define ARDUINO_CFG4_PWM  ((1) << MUX_SEL4_OFFSET)

#define ARDUINO_CFG5_GPIO ((0) << MUX_SEL5_OFFSET)
#define ARDUINO_CFG5_SPI  ((1) << MUX_SEL5_OFFSET) /* io_spi_mst0, cs_0 */
#define ARDUINO_CFG5_PWM1 ((2) << MUX_SEL5_OFFSET)
#define ARDUINO_CFG5_PWM2 ((3) << MUX_SEL5_OFFSET)
#define ARDUINO_CFG5_PWM3 ((4) << MUX_SEL5_OFFSET)

#define ARDUINO_CFG6_GPIO ((0) << MUX_SEL6_OFFSET)
#define ARDUINO_CFG6_I2C  ((1) << MUX_SEL6_OFFSET) /* io_i2c_mst1 */

static int pinctrl_emsdp_set(uint32_t pin, uint32_t type)
{
	const uint32_t mux_regs = (EMSDP_CREG_BASE + EMSDP_CREG_PMOD_MUX_OFFSET);
	uint32_t reg;

	if (pin == UNMUXED_PIN) {
		return 0;
	}

	if (pin <= PMOD_C) {
		reg = sys_read32(mux_regs + PMOD_MUX_CTRL);
	} else {
		reg = sys_read32(mux_regs + ARDUINO_MUX_CTRL);
	}

	switch (pin) {
	case PMOD_A:
		reg &= ~(MUX_SEL0_MASK);
		switch (type) {
		case PMOD_GPIO:
			reg |= PM_A_CFG0_GPIO;
			break;
		case PMOD_UARTA:
			reg |= PM_A_CFG0_UART1a;
			break;
		case PMOD_UARTB:
			reg |= PM_A_CFG0_UART1b;
			break;
		case PMOD_SPI:
			reg |= PM_A_CFG0_SPI;
			break;
		case PMOD_I2C:
			reg |= PM_A_CFG0_I2C;
			break;
		case PMOD_PWM_MODE1:
			reg |= PM_A_CFG0_PWM1;
			break;
		case PMOD_PWM_MODE2:
			reg |= PM_A_CFG0_PWM2;
			break;
		default:
			break;
		}
		break;
	case PMOD_B:
		reg &= ~(MUX_SEL2_MASK);
		switch (type) {
		case PMOD_GPIO:
			reg |= PM_B_CFG0_GPIO;
			break;
		case PMOD_UARTA:
			reg |= PM_B_CFG0_UART2a;
			break;
		case PMOD_UARTB:
			reg |= PM_A_CFG0_UART1b;
			break;
		case PMOD_SPI:
			reg |= PM_B_CFG0_SPI;
			break;
		case PMOD_I2C:
			reg |= PM_B_CFG0_I2C;
			break;
		case PMOD_PWM_MODE1:
			reg |= PM_B_CFG0_PWM1;
			break;
		case PMOD_PWM_MODE2:
			reg |= PM_B_CFG0_PWM2;
			break;
		default:
			break;
		}
		break;
	case PMOD_C:
		reg &= ~(MUX_SEL4_MASK);
		switch (type) {
		case PMOD_GPIO:
			reg |= PM_C_CFG0_GPIO;
			break;
		case PMOD_UARTA:
			reg |= PM_C_CFG0_UART3a;
			break;
		case PMOD_UARTB:
			reg |= PM_C_CFG0_UART3b;
			break;
		case PMOD_SPI:
			reg |= PM_C_CFG0_SPI;
			break;
		case PMOD_I2C:
			reg |= PM_C_CFG0_I2C;
			break;
		case PMOD_PWM_MODE1:
			reg |= PM_C_CFG0_PWM1;
			break;
		case PMOD_PWM_MODE2:
			reg |= PM_C_CFG0_PWM2;
			break;
		default:
			break;
		}
		break;
	case ARDUINO_PIN_0:
	case ARDUINO_PIN_1:
		reg &= ~MUX_SEL0_MASK;
		if (type == ARDUINO_GPIO) {
			reg |= ARDUINO_CFG0_GPIO;
		} else if (type == ARDUINO_UART) {
			reg |= ARDUINO_CFG0_UART;
		}
		break;
	case ARDUINO_PIN_2:
	case ARDUINO_PIN_3:
		reg &= ~MUX_SEL1_MASK;
		if (type == ARDUINO_GPIO) {
			reg |= ARDUINO_CFG1_GPIO;
		} else if (type == ARDUINO_PWM) {
			reg |= ARDUINO_CFG1_PWM;
		}
		break;
	case ARDUINO_PIN_4:
	case ARDUINO_PIN_5:
		reg &= ~MUX_SEL2_MASK;
		if (type == ARDUINO_GPIO) {
			reg |= ARDUINO_CFG2_GPIO;
		} else if (type == ARDUINO_PWM) {
			reg |= ARDUINO_CFG2_PWM;
		}
		break;
	case ARDUINO_PIN_6:
	case ARDUINO_PIN_7:
		reg &= ~MUX_SEL3_MASK;
		if (type == ARDUINO_GPIO) {
			reg |= ARDUINO_CFG3_GPIO;
		} else if (type == ARDUINO_PWM) {
			reg |= ARDUINO_CFG3_PWM;
		}
		break;
	case ARDUINO_PIN_8:
	case ARDUINO_PIN_9:
		reg &= ~MUX_SEL4_MASK;
		if (type == ARDUINO_GPIO) {
			reg |= ARDUINO_CFG4_GPIO;
		} else if (type == ARDUINO_PWM) {
			reg |= ARDUINO_CFG4_PWM;
		}
		break;
	case ARDUINO_PIN_10:
	case ARDUINO_PIN_11:
	case ARDUINO_PIN_12:
	case ARDUINO_PIN_13:
		reg &= ~MUX_SEL5_MASK;
		if (type == ARDUINO_GPIO) {
			reg |= ARDUINO_CFG5_GPIO;
		} else if (type == ARDUINO_SPI) {
			reg |= ARDUINO_CFG5_SPI;
		} else if (type == ARDUINO_PWM) {
			reg |= ARDUINO_CFG5_PWM1;
		}
		break;
	case ARDUINO_PIN_AD4:
	case ARDUINO_PIN_AD5:
		reg &= ~MUX_SEL6_MASK;
		if (type == ARDUINO_GPIO) {
			reg |= ARDUINO_CFG6_GPIO;
		} else if (type == ARDUINO_I2C) {
			reg |= ARDUINO_CFG6_I2C;
		}
		break;
	default:
		break;
	}

	if (pin <= PMOD_C) {
		sys_write32(reg, mux_regs + PMOD_MUX_CTRL);
	} else {
		sys_write32(reg, mux_regs + ARDUINO_MUX_CTRL);
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	int i;

	for (i = 0; i < pin_cnt; i++) {
		pinctrl_emsdp_set(pins[i].pin, pins[i].type);
	}

	return 0;
}
