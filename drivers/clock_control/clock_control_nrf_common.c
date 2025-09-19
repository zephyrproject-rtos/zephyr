#include "clock_control_nrf_common.h"
#include <nrfx.h>
#include <nrfx_clock.h>

#if NRFX_CHECK(NRFX_POWER_ENABLED)
#include <nrfx_power.h>
#endif

#define DT_DRV_COMPAT nordic_nrf_clock

static bool irq_connected = false;

static void clock_irq_handler(void)
{
#if NRFX_CHECK(NRFX_POWER_ENABLED)
	nrfx_power_irq_handler();
#endif

	STRUCT_SECTION_FOREACH(clock_control_nrf_irq_handler, irq) {
		irq->handler();
	}

    /* temporary fix, it will be removed when all the clocks are moved to their files */
    nrfx_clock_irq_handler();
}

void clock_control_nrf_common_connect_irq( void )
{
	if (irq_connected) {
		return;
	}
	irq_connected = true;

#if NRF_LFRC_HAS_CALIBRATION
	IRQ_CONNECT(LFRC_IRQn, DT_INST_IRQ(0, priority), nrfx_isr, clock_irq_handler, 0);
#endif

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), nrfx_isr, clock_irq_handler, 0);
}
