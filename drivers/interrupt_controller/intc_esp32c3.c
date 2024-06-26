/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <soc/periph_defs.h>
#include <limits.h>
#include <assert.h>
#include "soc/soc.h"
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#include <zephyr/sw_isr_table.h>
#include <riscv/interrupt.h>

#define ESP32C3_INTC_DEFAULT_PRIO			15

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_esp32c3, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * Define this to debug the choices made when allocating the interrupt. This leads to much debugging
 * output within a critical region, which can lead to weird effects like e.g. the interrupt watchdog
 * being triggered, that is why it is separate from the normal LOG* scheme.
 */
#ifdef CONFIG_INTC_ESP32C3_DECISIONS_LOG
# define INTC_LOG(...) LOG_INF(__VA_ARGS__)
#else
# define INTC_LOG(...) do {} while (0)
#endif

#define ESP32C3_INTC_DEFAULT_PRIORITY   15
#define ESP32C3_INTC_DEFAULT_THRESHOLD  1
#define ESP32C3_INTC_DISABLED_SLOT      31
#define ESP32C3_INTC_SRCS_PER_IRQ       2
#define ESP32C3_INTC_AVAILABLE_IRQS     30

#if defined(CONFIG_SOC_SERIES_ESP32C6)

#define IRQ_NA		0xFF	/* IRQ not available */
#define IRQ_FREE	0xFE

#define ESP32C6_INTC_SRCS_PER_IRQ       2
#define ESP32C6_INTC_AVAILABLE_IRQS     31

/* For ESP32C6 only CPU peripheral interrupts number
 * 1, 2, 5, 6, 8 ~ 31 are available.
 * IRQ 31 is reserved for disabled interrupts
 */
static uint8_t esp_intr_irq_alloc[ESP32C6_INTC_AVAILABLE_IRQS][ESP32C6_INTC_SRCS_PER_IRQ] = {
	[0] = {IRQ_NA, IRQ_NA},
	[3] = {IRQ_NA, IRQ_NA},
	[4] = {IRQ_NA, IRQ_NA},
	[7] = {IRQ_NA, IRQ_NA},
	[1 ... 2]  = {IRQ_FREE, IRQ_FREE},
	[5 ... 6]  = {IRQ_FREE, IRQ_FREE},
	[8 ... 30] = {IRQ_FREE, IRQ_FREE}
};
#endif

#define STATUS_MASK_NUM		3

static uint32_t esp_intr_enabled_mask[STATUS_MASK_NUM] = {0, 0, 0};

#if defined(CONFIG_SOC_SERIES_ESP32C3)

static uint32_t esp_intr_find_irq_for_source(uint32_t source)
{
	/* in general case, each 2 sources goes routed to
	 * 1 IRQ line.
	 */
	uint32_t irq = (source / ESP32C3_INTC_SRCS_PER_IRQ);

	if (irq > ESP32C3_INTC_AVAILABLE_IRQS) {
		INTC_LOG("Clamping the source: %d no more IRQs available", source);
		irq = ESP32C3_INTC_AVAILABLE_IRQS;
	} else if (irq == 0) {
		irq = 1;
	}

	INTC_LOG("Found IRQ: %d for source: %d", irq, source);

	return irq;
}

#elif defined(CONFIG_SOC_SERIES_ESP32C6)

static uint32_t esp_intr_find_irq_for_source(uint32_t source)
{
	uint32_t irq = 0;

	/* First allocate one source per IRQ, then two
	 * if there are more sources than free IRQs
	 */
	for (int j = 0; j < ESP32C6_INTC_SRCS_PER_IRQ; j++) {
		for (int i = 0; i < ESP32C6_INTC_AVAILABLE_IRQS; i++) {
			if (esp_intr_irq_alloc[i][j] == IRQ_FREE) {
				esp_intr_irq_alloc[i][j] = (uint8_t)source;
				irq = i;
				goto found;
			}
		}
	}

found:
	INTC_LOG("Found IRQ: %d for source: %d", irq, source);

	return irq;
}

#endif

void esp_intr_initialize(void)
{
	/* IRQ 31 is reserved for disabled interrupts,
	 * so route all sources to it
	 */
	for (int i = 0 ; i < ESP32C3_INTC_AVAILABLE_IRQS + 2; i++) {
		irq_disable(i);
	}

	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		esp_rom_intr_matrix_set(0, i, ESP32C3_INTC_DISABLED_SLOT);
	}

#if defined(CONFIG_SOC_SERIES_ESP32C6)
	/* Clear up IRQ allocation */
	for (int j = 0; j < ESP32C6_INTC_SRCS_PER_IRQ; j++) {
		for (int i = 0; i < ESP32C6_INTC_AVAILABLE_IRQS; i++) {
			/* screen out reserved IRQs */
			if (esp_intr_irq_alloc[i][j] != IRQ_NA) {
				esp_intr_irq_alloc[i][j] = IRQ_FREE;
			}
		}
	}
#endif

	/* set global esp32c3's INTC masking level */
	esprv_intc_int_set_threshold(ESP32C3_INTC_DEFAULT_THRESHOLD);
}

int esp_intr_alloc(int source,
		int flags,
		isr_handler_t handler,
		void *arg,
		void **ret_handle)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(ret_handle);

	if (handler == NULL) {
		return -EINVAL;
	}

	if (source < 0 || source >= ETS_MAX_INTR_SOURCE) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	irq_connect_dynamic(source,
		ESP32C3_INTC_DEFAULT_PRIORITY,
		handler,
		arg,
		0);

	if (source < 32) {
		esp_intr_enabled_mask[0] |= (1 << source);
	} else if (source < 64) {
		esp_intr_enabled_mask[1] |= (1 << (source - 32));
	} else if (source < 96) {
		esp_intr_enabled_mask[2] |= (1 << (source - 64));
	}

	INTC_LOG("Enabled ISRs -- 0: 0x%X -- 1: 0x%X -- 2: 0x%X",
		esp_intr_enabled_mask[0], esp_intr_enabled_mask[1], esp_intr_enabled_mask[2]);

	irq_unlock(key);
	irq_enable(source);

	return 0;
}

int esp_intr_disable(int source)
{
	if (source < 0 || source >= ETS_MAX_INTR_SOURCE) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	esp_rom_intr_matrix_set(0,
		source,
		ESP32C3_INTC_DISABLED_SLOT);

#if defined(CONFIG_SOC_SERIES_ESP32C6)
	for (int j = 0; j < ESP32C6_INTC_SRCS_PER_IRQ; j++) {
		for (int i = 0; i < ESP32C6_INTC_AVAILABLE_IRQS; i++) {
			if (esp_intr_irq_alloc[i][j] == source) {
				esp_intr_irq_alloc[i][j] = IRQ_FREE;
				goto freed;
			}
		}
	}
freed:
#endif

	if (source < 32) {
		esp_intr_enabled_mask[0] &= ~(1 << source);
	} else if (source < 64) {
		esp_intr_enabled_mask[1] &= ~(1 << (source - 32));
	} else if (source < 96) {
		esp_intr_enabled_mask[2] &= ~(1 << (source - 64));
	}

	INTC_LOG("Enabled ISRs -- 0: 0x%X -- 1: 0x%X -- 2: 0x%X",
		esp_intr_enabled_mask[0], esp_intr_enabled_mask[1], esp_intr_enabled_mask[2]);

	irq_unlock(key);

	return 0;
}

int esp_intr_enable(int source)
{
	if (source < 0 || source >= ETS_MAX_INTR_SOURCE) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();
	uint32_t irq = esp_intr_find_irq_for_source(source);

	esp_rom_intr_matrix_set(0, source, irq);

	if (source < 32) {
		esp_intr_enabled_mask[0] |= (1 << source);
	} else if (source < 64) {
		esp_intr_enabled_mask[1] |= (1 << (source - 32));
	} else if (source < 96) {
		esp_intr_enabled_mask[2] |= (1 << (source - 64));
	}

	INTC_LOG("Enabled ISRs -- 0: 0x%X -- 1: 0x%X -- 2: 0x%X",
		esp_intr_enabled_mask[0], esp_intr_enabled_mask[1], esp_intr_enabled_mask[2]);

	esprv_intc_int_set_priority(irq, ESP32C3_INTC_DEFAULT_PRIO);
	esprv_intc_int_set_type(irq, INTR_TYPE_LEVEL);
	esprv_intc_int_enable(1 << irq);

	irq_unlock(key);

	return 0;
}

uint32_t esp_intr_get_enabled_intmask(int status_mask_number)
{
	INTC_LOG("Enabled ISRs -- 0: 0x%X -- 1: 0x%X -- 2: 0x%X",
		esp_intr_enabled_mask[0], esp_intr_enabled_mask[1], esp_intr_enabled_mask[2]);

	if (status_mask_number < STATUS_MASK_NUM) {
		return esp_intr_enabled_mask[status_mask_number];
	} else {
		return 0;	/* error */
	}
}
