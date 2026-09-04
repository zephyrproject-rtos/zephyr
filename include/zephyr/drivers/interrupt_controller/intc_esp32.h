/*
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_ESP32_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * Interrupt allocation flags - These flags can be used to specify
 * which interrupt qualities the code calling esp_intr_alloc* needs.
 *
 */

/* Keep the LEVELx values as they are here; they match up with (1<<level) */
#define ESP_INTR_FLAG_LEVEL1		(1<<1)	/* Accept a Level 1 int vector, lowest priority */
#define ESP_INTR_FLAG_LEVEL2		(1<<2)	/* Accept a Level 2 int vector */
#define ESP_INTR_FLAG_LEVEL3		(1<<3)	/* Accept a Level 3 int vector */
#define ESP_INTR_FLAG_LEVEL4		(1<<4)	/* Accept a Level 4 int vector */
#define ESP_INTR_FLAG_LEVEL5		(1<<5)	/* Accept a Level 5 int vector */
#define ESP_INTR_FLAG_LEVEL6		(1<<6)	/* Accept a Level 6 int vector */
#define ESP_INTR_FLAG_NMI		(1<<7)	/* Accept a Level 7 int vector, highest priority */
#define ESP_INTR_FLAG_SHARED		(1<<8)	/* Interrupt can be shared between ISRs */
#define ESP_INTR_FLAG_EDGE		(1<<9)	/* Edge-triggered interrupt */
#define ESP_INTR_FLAG_IRAM		(1<<10)	/* ISR can be called if cache is disabled */
#define ESP_INTR_FLAG_INTRDISABLED	(1<<11)	/* Return with this interrupt disabled */

/* Low and medium prio interrupts. These can be handled in C. */
#define ESP_INTR_FLAG_LOWMED	(ESP_INTR_FLAG_LEVEL1|ESP_INTR_FLAG_LEVEL2|ESP_INTR_FLAG_LEVEL3)

/* High level interrupts. Need to be handled in assembly. */
#define ESP_INTR_FLAG_HIGH	(ESP_INTR_FLAG_LEVEL4|ESP_INTR_FLAG_LEVEL5|ESP_INTR_FLAG_LEVEL6| \
				 ESP_INTR_FLAG_NMI)

/* Mask for all level flags */
#define ESP_INTR_FLAG_LEVELMASK	(ESP_INTR_FLAG_LEVEL1|ESP_INTR_FLAG_LEVEL2|ESP_INTR_FLAG_LEVEL3| \
				 ESP_INTR_FLAG_LEVEL4|ESP_INTR_FLAG_LEVEL5|ESP_INTR_FLAG_LEVEL6| \
				 ESP_INTR_FLAG_NMI)

/*
 * Default CPU-line interrupt priority for IRQ_CONNECT(). Mirrors the
 * IRQ_DEFAULT_PRIORITY used by the intmux devicetree nodes and is fixed per
 * architecture: ignored on Xtensa (the priority is determined by the CPU
 * interrupt line), and a valid controller priority on RISC-V, where 0 is not
 * usable. Guarded so it yields to the dt-bindings definition if both are seen.
 */
#ifndef IRQ_DEFAULT_PRIORITY
#if defined(CONFIG_RISCV)
#define IRQ_DEFAULT_PRIORITY 1
#else
#define IRQ_DEFAULT_PRIORITY 0
#endif
#endif

/*
 * Get the interrupt flags from the supplied priority.
 */
#define ESP_PRIO_TO_FLAGS(priority) \
	((priority) > 0 ? ((1 << (priority)) & ESP_INTR_FLAG_LEVELMASK) : 0)

/*
 * Check interrupt flags from input and filter unallowed values.
 */
#define ESP_INT_FLAGS_CHECK(int_flags) ((int_flags) & ESP_INTR_FLAG_SHARED)

/*
 * The esp_intr_alloc* functions can allocate an int for all *_INTR_SOURCE int sources that
 * are routed through the interrupt mux. Apart from these sources, each core also has some internal
 * sources that do not pass through the interrupt mux. To allocate an interrupt for these sources,
 * pass these pseudo-sources to the functions.
 */
#define ETS_INTERNAL_TIMER0_INTR_SOURCE         -1 /* Xtensa timer 0 interrupt source */
#define ETS_INTERNAL_TIMER1_INTR_SOURCE         -2 /* Xtensa timer 1 interrupt source */
#define ETS_INTERNAL_TIMER2_INTR_SOURCE         -3 /* Xtensa timer 2 interrupt source */
#define ETS_INTERNAL_SW0_INTR_SOURCE            -4 /* Software int source 1 */
#define ETS_INTERNAL_SW1_INTR_SOURCE            -5 /* Software int source 2 */
#define ETS_INTERNAL_PROFILING_INTR_SOURCE      -6 /* Int source for profiling */

/* Function prototype for interrupt handler function */
typedef void (*intr_handler_t)(void *arg);

/* Interrupt handler associated data structure */
typedef struct intr_handle_data_t intr_handle_data_t;

/* Handle to an interrupt handler */
typedef intr_handle_data_t *intr_handle_t;

/**
 * @brief Disable the interrupts that cannot run with the flash cache disabled.
 *
 * Called by the HAL around every flash operation (spi_flash/cache_utils.c).
 * These two are the only part of the legacy esp_intr_* API still in use; the
 * rest went away with the allocator the multi-level model replaced.
 */
void esp_intr_noniram_disable(void);

/**
 * @brief Re-enable the interrupts disabled by esp_intr_noniram_disable().
 */
void esp_intr_noniram_enable(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_ESP32_H_ */
