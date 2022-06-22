#ifndef __RISCV_ITE_SOC_H_
#define __RISCV_ITE_SOC_H_
/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <soc_common.h>

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(CONFIG_SRAM_SIZE)

/*
 * This define gets the total number of USBPD ports available on the
 * ITE EC chip from dtsi (include status disable). Both it81202 and
 * it81302 support two USBPD ports.
 */
#define SOC_USBPD_ITE_PHY_PORT_COUNT \
COND_CODE_1(DT_NODE_EXISTS(DT_INST(1, ite_it8xxx2_usbpd)), (2), (1))

/*
 * This define gets the number of active USB Power Delivery (USB PD)
 * ports in use on the ITE microcontroller from dts (only status okay).
 * The active port usage should follow the order of ITE TCPC port index,
 * ex. if we're active only one ITE USB PD port, then the port should be
 * 0x3700 (port0 register base), instead of 0x3800 (port1 register base).
 */
#define SOC_USBPD_ITE_ACTIVE_PORT_COUNT DT_NUM_INST_STATUS_OKAY(ite_it8xxx2_usbpd)

#ifndef _ASMLANGUAGE
void soc_interrupt_init(void);
#endif

#endif /* __RISCV_ITE_SOC_H_ */
