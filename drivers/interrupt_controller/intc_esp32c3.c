/*
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_esp32, CONFIG_LOG_DEFAULT_LEVEL);

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

#define ESP32_INTC_DEFAULT_PRIORITY  15
#define ESP32_INTC_DEFAULT_THRESHOLD 1
#define ESP32_INTC_DISABLED_SLOT     31
#define ESP32_INTC_SRCS_PER_IRQ      2

/* Define maximum interrupt sources per SoC */
#if defined(CONFIG_SOC_SERIES_ESP32C6)
/*
 * Interrupt reserved mask
 * 0 is reserved
 * 1 is for Wi-Fi
 * 3, 4 and 7 are unavailable for PULP CPU as they are bound to Core-Local Interrupts (CLINT)
 */
#define RSVD_MASK (BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(7))
#define ESP_INTC_AVAILABLE_IRQS 31
#else
/*
 * Interrupt reserved mask
 * 1 is for Wi-Fi
 */
#define RSVD_MASK (BIT(0) | BIT(1))
#define ESP_INTC_AVAILABLE_IRQS 30
#endif

/* Single array for IRQ allocation */
static uint8_t esp_intr_irq_alloc[ESP_INTC_AVAILABLE_IRQS * ESP32_INTC_SRCS_PER_IRQ];

#define ESP_INTR_IDX(irq, slot) ((irq % ESP_INTC_AVAILABLE_IRQS) * ESP32_INTC_SRCS_PER_IRQ + slot)

#define STATUS_MASK_NUM 3

static uint32_t esp_intr_enabled_mask[STATUS_MASK_NUM] = {0, 0, 0};

static uint32_t esp_intr_find_irq_for_source(uint32_t source)
{
	if (source >= ETS_MAX_INTR_SOURCE) {
		return IRQ_NA;
	}

	uint32_t irq = source / ESP32_INTC_SRCS_PER_IRQ;

	/* Check if the derived IRQ is usable first */
	for (int j = 0; j < ESP32_INTC_SRCS_PER_IRQ; j++) {
		int idx = ESP_INTR_IDX(irq, j);

		/* Ensure idx is within a valid range */
		if (idx >= ARRAY_SIZE(esp_intr_irq_alloc)) {
			continue;
		}

		/* If source is already assigned, return the IRQ */
		if (esp_intr_irq_alloc[idx] == source) {
			return irq;
		}

		/* If slot is free, allocate it */
		if (esp_intr_irq_alloc[idx] == IRQ_FREE) {
			esp_intr_irq_alloc[idx] = source;
			return irq;
		}
	}

	/* If derived IRQ is full, search for another available IRQ */
	for (irq = 0; irq < ESP_INTC_AVAILABLE_IRQS; irq++) {
		if (RSVD_MASK & (1U << irq)) {
			continue;
		}
		for (int j = 0; j < ESP32_INTC_SRCS_PER_IRQ; j++) {
			int idx = ESP_INTR_IDX(irq, j);

			/* Ensure idx is within a valid range */
			if (idx >= ARRAY_SIZE(esp_intr_irq_alloc)) {
				continue;
			}

			/* If source is already assigned, return this IRQ */
			if (esp_intr_irq_alloc[idx] == source) {
				return irq;
			}

			/* If slot is free, allocate it */
			if (esp_intr_irq_alloc[idx] == IRQ_FREE) {
				esp_intr_irq_alloc[idx] = source;
				return irq;
			}
		}
	}

	/* No available slot found */
	return IRQ_NA;
}

void esp_intr_initialize(void)
{
	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		esp_rom_intr_matrix_set(0, i, ESP32_INTC_DISABLED_SLOT);
	}

	for (int irq = 0; irq < ESP_INTC_AVAILABLE_IRQS; irq++) {
		for (int j = 0; j < ESP32_INTC_SRCS_PER_IRQ; j++) {
			int idx = ESP_INTR_IDX(irq, j);

			if (RSVD_MASK & (1U << irq)) {
				esp_intr_irq_alloc[idx] = IRQ_NA;
			} else {
				esp_intr_irq_alloc[idx] = IRQ_FREE;
			}
		}
	}

	/* set global INTC masking level */
	esprv_intc_int_set_threshold(ESP32_INTC_DEFAULT_THRESHOLD);
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
	uint32_t irq = esp_intr_find_irq_for_source(source);

	if (irq == IRQ_NA) {
		irq_unlock(key);
		return -ENOMEM;
	}

	irq_connect_dynamic(source,
		ESP32_INTC_DEFAULT_PRIORITY,
		handler,
		arg,
		0);

	INTC_LOG("Enabled ISRs -- 0: 0x%X -- 1: 0x%X -- 2: 0x%X",
		esp_intr_enabled_mask[0], esp_intr_enabled_mask[1], esp_intr_enabled_mask[2]);

	irq_unlock(key);
	int ret = esp_intr_enable(source);

	return ret;
}

int esp_intr_disable(int source)
{
	if (source < 0 || source >= ETS_MAX_INTR_SOURCE) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	esp_rom_intr_matrix_set(0,
		source,
		ESP32_INTC_DISABLED_SLOT);

	for (int i = 0; i < ESP_INTC_AVAILABLE_IRQS; i++) {
		if (RSVD_MASK & (1U << i)) {
			continue;
		}
		for (int j = 0; j < ESP32_INTC_SRCS_PER_IRQ; j++) {
			int idx = ESP_INTR_IDX(i, j);

			if (esp_intr_irq_alloc[idx] == source) {
				esp_intr_irq_alloc[idx] = IRQ_FREE;
			}
		}
	}

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

	if (irq == IRQ_NA) {
		irq_unlock(key);
		return -ENOMEM;
	}

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

	esprv_intc_int_set_priority(irq, ESP32_INTC_DEFAULT_PRIORITY);
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
	}

	return 0;
}
