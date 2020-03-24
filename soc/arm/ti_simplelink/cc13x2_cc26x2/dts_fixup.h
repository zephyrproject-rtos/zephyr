/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS
#define CONFIG_ENTROPY_NAME DT_LABEL(DT_INST(0, ti_cc13xx_cc26xx_trng))

#define DT_CPU_CLOCK_FREQUENCY DT_ARM_CORTEX_M4_0_CLOCK_FREQUENCY

/* End of SoC Level DTS fixup file */
