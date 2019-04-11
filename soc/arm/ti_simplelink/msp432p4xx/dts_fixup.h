/* SPDX-License-Identifier: Apache-2.0 */

/* This file is a temporary workaround for mapping of the generated information
 * to the current driver definitions.  This will be removed when the drivers
 * are modified to handle the generated information, or the mapping of
 * generated data matches the driver definitions.
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS               DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_UART_MSP432P4XX_NAME            DT_TI_MSP432P4XX_UART_40001000_LABEL
#define DT_UART_MSP432P4XX_BASE_ADDRESS    DT_TI_MSP432P4XX_UART_40001000_BASE_ADDRESS
#define DT_UART_MSP432P4XX_CLOCK_FREQUENCY DT_TI_MSP432P4XX_UART_40001000_CLOCKS_CLOCK_FREQUENCY
#define DT_UART_MSP432P4XX_BAUD_RATE       DT_TI_MSP432P4XX_UART_40001000_CURRENT_SPEED

/* End of SoC Level DTS fixup file */
