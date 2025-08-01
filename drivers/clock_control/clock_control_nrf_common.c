#include "clock_control_nrf_common.h"

#if NRFX_CHECK(NRFX_POWER_ENABLED)
#include <nrfx_power.h>
#endif

static bool irq_connected = false;

static void clock_irq_handler(void)
{
#if NRFX_CHECK(NRFX_POWER_ENABLED)
	nrfx_power_irq_handler();
#endif

	STRUCT_SECTION_FOREACH(clock_control_nrf_irq_handler, irq) {
		irq->handler();
	}
}

void clock_control_nrf_common_connect_irq( void )
{
	if (irq_connected) {
		return;
	}
	irq_connected = true;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, clock_irq_handler, 0);
}
