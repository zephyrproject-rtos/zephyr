/* SPDX-License-Identifier: Apache-2.0 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS	DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_WDT_0_NAME			DT_LABEL(DT_INST(0, st_stm32_watchdog))

#define DT_ADC_1_NAME			DT_ST_STM32_ADC_40012000_LABEL

/* End of SoC Level DTS fixup file */
