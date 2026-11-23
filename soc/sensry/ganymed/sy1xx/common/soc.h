/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#ifndef GANYMED_SY1XX_SOC_H
#define GANYMED_SY1XX_SOC_H

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

/* SOC PERIPHERALS */

#define SY1XX_ARCHI_SOC_PERIPHERALS_ADDR 0x1A100000

#define SY1XX_ARCHI_UDMA_OFFSET         0x00002000
#define SY1XX_ARCHI_APB_SOC_CTRL_OFFSET 0x00004000
#define SY1XX_ARCHI_FC_ITC_OFFSET       0x00009800

#define SY1XX_ARCHI_UDMA_ADDR (SY1XX_ARCHI_SOC_PERIPHERALS_ADDR + SY1XX_ARCHI_UDMA_OFFSET)
#define SY1XX_ARCHI_FC_ITC_ADDR (SY1XX_ARCHI_SOC_PERIPHERALS_ADDR + SY1XX_ARCHI_FC_ITC_OFFSET)

uint32_t sy1xx_soc_get_rts_clock_frequency(void);
uint32_t sy1xx_soc_get_peripheral_clock(void);

void soc_enable_irq(uint32_t idx);
void soc_disable_irq(uint32_t idx);

#endif /* _ASMLANGUAGE */

#endif /* GANYMED_SY1XX_SOC_H */
