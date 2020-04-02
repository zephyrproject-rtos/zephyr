/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_CPU_CLOCK_FREQUENCY DT_CADENCE_TENSILICA_XTENSA_LX6_0_CLOCK_FREQUENCY
#define CONFIG_ENTROPY_NAME DT_LABEL(DT_INST(0, espressif_esp32_trng))
#define DT_WDT_0_NAME DT_LABEL(DT_INST(0, espressif_esp32_watchdog))
/* End of SoC Level DTS fixup file */
