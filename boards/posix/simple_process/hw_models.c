/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Barebones HW model sufficient to run some of the sample apps
 */

#include "soc.h"
#include "hw_models.h"
#include <stdint.h>

/*set this to 1 to run in real time, to 0 to run as fast as possible*/
#define CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME 1
#define STOP_AFTER_5_SECONDS 1

#if (CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME)
#include <time.h>
#endif

/**
 * Entry point for the HW models
 */
void hw_models_main_loop(void)
{
	/*just a trivial system tick model*/
#if (CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME)
	struct timespec tv;
	uint64_t Expected_time;
#endif
	uint64_t tick_p = 10000;
	uint64_t running_time = 0;
/*
 * Note regarding types: This file is not suppossed to see the kernel types
 * Therefore we use stdint.h types
 */


#if (CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME)
	clock_gettime(CLOCK_MONOTONIC, &tv);
	Expected_time = tv.tv_sec*1e6 + tv.tv_nsec/1000;
#endif

	while (1) {
		hw_irq_controller(TIMER);
		running_time += tick_p;

#if (CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME)
		clock_gettime(CLOCK_MONOTONIC, &tv);
		Expected_time += tick_p;
		uint64_t RealCurrentTime = tv.tv_sec*1e6 + tv.tv_nsec/1000;
		int64_t diff = Expected_time - RealCurrentTime;

		if (diff > 0) { /*we slow down*/
			struct timespec requested_time;
			struct timespec remaining;

			requested_time.tv_sec = diff / 1e6;
			requested_time.tv_nsec =
				(diff - requested_time.tv_sec*1e6)*1e3;

			int s = nanosleep(&requested_time, &remaining);

			if (s == -1) {
				simulation_engine_print_trace(
						"Interrupted or error\n");
				break;
			}
		}
#endif
/*
 *		if ( running_time % 1000000 == 0 ) {
 *			printf( "Trivial timer: We have run for %llus\n",
 *				running_time/1000000);
 *		}
 */
#if (STOP_AFTER_5_SECONDS)
		if (running_time > 5e6) {
			simulation_engine_print_trace(
					"\n\n\n\n\n\nAutostopped after 5s\n");
			main_clean_up();
		}
#endif
	} /*while (1)*/
}

/**
 * Function to initialize the hw models
 */
void hw_init(void)
{
	/*nothing to be done*/
}

/**
 * Function to free the hw models
 */
void hw_cleanup(void)
{
	/*nothing to be done*/
}

/********************************
 * Trivial IRQ controller model *
 ********************************/

static uint64_t irq_status;
/**
 * HW IRQ controller model provided by this board
 * (just throws the interrupt to the CPU)
 */
void hw_irq_controller(irq_type_t irq)
{
	/*No interrupt masking or any other fancy feature*/

	irq_status |= 1 << irq;
	posix_soc_interrupt_raised();
}

/**
 * Function for SW to clear interrupts in this interrupt controller
 */
void hw_irq_controller_clear_irqs(void)
{
	irq_status = 0;
}


/**
 * Function for SW to get the status in the interrupt controller
 */
uint64_t hw_irq_controller_get_irq_status(void)
{
	return irq_status;
}
/*
 * End of IRQ controller
 */





