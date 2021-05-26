/*
 * Copyright (c) 2018 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <soc.h>

#if defined(CONFIG_SOC_NSIM_SEM)
#define NORMAL_FIRMWARE_ENTRY 0x40000
#elif defined(CONFIG_SOC_EMSK)
#define NORMAL_FIRMWARE_ENTRY 0x20000
#endif

void main(void)
{
	/* necessary configuration before go to normal */

	/* allocate timer 0 and timer1 to normal mode */
	z_arc_v2_irq_uinit_secure_set(IRQ_TIMER0, 0);
	z_arc_v2_irq_uinit_secure_set(IRQ_TIMER1, 0);

	/* disable the secure interrupts for debug purpose*/
	/* _arc_v2_irq_unit_int_disable(IRQ_S_TIMER0); */

	printk("Go to normal application\n");

	arc_go_to_normal(*((uint32_t *)(NORMAL_FIRMWARE_ENTRY)));

	printk("should not come here\n");

}
