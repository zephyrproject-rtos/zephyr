/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PINCTRL_ZYNQ_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PINCTRL_ZYNQ_H_

/**
 * @name IO buffer type
 *
 * Definitions for Xilinx Zynq-7000 pinctrl `power-source` devicetree property values. The value
 * corresponds to what is written to the IO_Type field in the MIO_PIN_xx SLCR register.
 *
 * @{
 */
#define IO_STANDARD_LVCMOS18 1
#define IO_STANDARD_LVCMOS25 2
#define IO_STANDARD_LVCMOS33 3
#define IO_STANDARD_HSTL     4
/** @} */

/**
 * @name IO buffer edge rate
 *
 * Definitions for Xilinx Zynq-7000 pinctrl `slew-rate` devicetree property values. The value
 * corresponds to what is written to the Speed field in the MIO_PIN_xx SLCR register.
 *
 * @{
 */
#define IO_SPEED_SLOW 0
#define IO_SPEED_FAST 1
/** @} */


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PINCTRL_ZYNQ_H_ */
