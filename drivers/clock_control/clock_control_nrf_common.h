/*$$$LICENCE_NORDIC_STANDARD<2016>$$$*/

#ifndef CLOCK_CONTROL_NRF_COMMON_H__
#define CLOCK_CONTROL_NRF_COMMON_H__

struct clock_control_nrf_irq_handler {
	void (*handler)(void);	/* Clock interrupt handler */
};

#define CLOCK_CONTROL_NRF_IRQ_HANDLERS_ITERABLE(name, _a) \
	STRUCT_SECTION_ITERABLE(clock_control_nrf_irq_handler, name) = { \
				.handler = _a, \
	}

void clock_control_nrf_common_connect_irq( void );

#endif // CLOCK_CONTROL_NRF_COMMON_H__