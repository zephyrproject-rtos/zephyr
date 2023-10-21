/*
 * Copyright (c) 2016-2017 Piotr Mienkowski
 * Copyright (c) 2020-2023 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM0 MCU family I/O Pin Controller (PORT)
 */

#ifndef ATMEL_SAM0_SOC_PORT_H_
#define ATMEL_SAM0_SOC_PORT_H_

#include <soc.h>

/*
 * Pin flags/attributes
 */
#define SOC_PORT_DEFAULT                (0)

#define SOC_PORT_FLAGS_POS              (0)
#define SOC_PORT_FLAGS_MASK             (0xF3 << SOC_PORT_FLAGS_POS)
#define SOC_PORT_PULLUP_POS             (SOC_PORT_FLAGS_POS)
#define SOC_PORT_PULLUP                 (1 << SOC_PORT_PULLUP_POS)
#define SOC_PORT_PULLDOWN_POS           (SOC_PORT_PULLUP_POS + 1U)
#define SOC_PORT_PULLDOWN               (1 << SOC_PORT_PULLDOWN_POS)
/* Open-Drain is a reserved entry at pinctrl driver */
#define SOC_GPIO_OPENDRAIN_POS          (SOC_PORT_PULLDOWN_POS + 1U)
#define SOC_GPIO_OPENDRAIN		(0)
/* Wake-Up is a reserved entry at pinctrl driver */
#define SOC_GPIO_WAKEUP_POS             (SOC_GPIO_OPENDRAIN_POS + 1U)
#define SOC_GPIO_WAKEUP			(0)
/* Input-Enable means Input-Buffer, see dts/pinctrl/pincfg-node.yaml */
#define SOC_PORT_INPUT_ENABLE_POS       (SOC_GPIO_WAKEUP_POS + 1U)
#define SOC_PORT_INPUT_ENABLE           (1 << SOC_PORT_INPUT_ENABLE_POS)
/* Output-Enable, see dts/pinctrl/pincfg-node.yaml */
#define SOC_PORT_OUTPUT_ENABLE_POS      (SOC_PORT_INPUT_ENABLE_POS + 1U)
#define SOC_PORT_OUTPUT_ENABLE          (1 << SOC_PORT_OUTPUT_ENABLE_POS)
/* Drive-Strength, 0mA means normal, any other value means stronger */
#define SOC_PORT_STRENGTH_STRONGER_POS  (SOC_PORT_OUTPUT_ENABLE_POS + 1U)
#define SOC_PORT_STRENGTH_STRONGER      (1 << SOC_PORT_STRENGTH_STRONGER_POS)
/* Peripheral Multiplexer Enable */
#define SOC_PORT_PMUXEN_ENABLE_POS      (SOC_PORT_STRENGTH_STRONGER_POS + 1U)
#define SOC_PORT_PMUXEN_ENABLE          (1 << SOC_PORT_PMUXEN_ENABLE_POS)

/* Bit field: SOC_PORT_FUNC */
#define SOC_PORT_FUNC_POS               (16U)
#define SOC_PORT_FUNC_MASK              (0xF << SOC_PORT_FUNC_POS)

/** Connect pin to peripheral A. */
#define SOC_PORT_FUNC_A                 (0x0 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral B. */
#define SOC_PORT_FUNC_B                 (0x1 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral C. */
#define SOC_PORT_FUNC_C                 (0x2 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral D. */
#define SOC_PORT_FUNC_D                 (0x3 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral E. */
#define SOC_PORT_FUNC_E                 (0x4 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral F. */
#define SOC_PORT_FUNC_F                 (0x5 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral G. */
#define SOC_PORT_FUNC_G                 (0x6 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral H. */
#define SOC_PORT_FUNC_H                 (0x7 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral I. */
#define SOC_PORT_FUNC_I                 (0x8 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral J. */
#define SOC_PORT_FUNC_J                 (0x9 << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral K. */
#define SOC_PORT_FUNC_K                 (0xa << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral L. */
#define SOC_PORT_FUNC_L                 (0xb << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral M. */
#define SOC_PORT_FUNC_M                 (0xc << SOC_PORT_FUNC_POS)
/** Connect pin to peripheral N. */
#define SOC_PORT_FUNC_N                 (0xd << SOC_PORT_FUNC_POS)

struct soc_port_pin {
	PortGroup *regs;   /** pointer to registers of the I/O Pin Controller */
	uint32_t pinum;    /** pin number */
	uint32_t flags;    /** pin flags/attributes */
};

/**
 * @brief Configure PORT pin muxing.
 *
 * Configure one pin muxing belonging to some PORT.
 *
 * @param pg   PortGroup register
 * @param pin  Pin number
 * @param func Pin Function
 */
int soc_port_pinmux_set(PortGroup *pg, uint32_t pin, uint32_t func);

/**
 * @brief Configure PORT pin.
 *
 * Configure one pin belonging to some PORT.
 * Example scenarios:
 * - configure pin(s) as input.
 * - connect pin(s) to a peripheral B and enable pull-up.
 *
 * @remark During Reset, all PORT lines are configured as inputs with input
 * buffers, output buffers and pull disabled.  When the device is set to the
 * BACKUP sleep mode, even if the PORT configuration registers and input
 * synchronizers will lose their contents (these will not be restored when
 * PORT is powered up again), the latches in the pads will keep their current
 * configuration, such as the output value and pull settings.  Refer to the
 * Power Manager documentation for more features related to the I/O lines
 * configuration in and out of BACKUP mode.  The PORT peripheral will continue
 * operating in any Sleep mode where its source clock is running.
 *
 * @param pin  pin's configuration data such as pin mask, pin attributes, etc.
 */
void soc_port_configure(const struct soc_port_pin *pin);

/**
 * @brief Configure a list of PORT pin(s).
 *
 * Configure an arbitrary amount of pins in an arbitrary way.  Each
 * configuration entry is a single item in an array passed as an argument to
 * the function.
 *
 * @param pins an array where each item contains pin's configuration data.
 * @param size size of the pin list.
 */
void soc_port_list_configure(const struct soc_port_pin pins[],
			     unsigned int size);

#endif /* ATMEL_SAM0_SOC_PORT_H_ */
