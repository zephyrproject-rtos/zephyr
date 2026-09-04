/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/devicetree.h>
#include <soc/soc_caps.h>

/*
 * Per-CPU-line runtime state, the single record of how a CPU interrupt line is
 * being used. Indexed [core][cpu_line] and sized by the number of CPU interrupt
 * lines (32 on every Espressif SoC), NOT by CONFIG_NUM_IRQS: every index into it
 * is a CPU line, never an interrupt source and never an ISR-table slot. Sources
 * need no per-source array of their own - a source's enabled state is a bit in
 * its owning line's status_mask.
 *
 * Defined in drivers/interrupt_controller/intc_esp32.c and published here
 * because the connect path (z_soc_irq_flags_apply/clear in
 * soc/espressif/common/irq.c) maintains the client counters while the level-2
 * dispatcher in the driver reads the mask. The dispatcher is IRAM-resident and
 * hot, so it indexes the array directly rather than going through accessors.
 *
 * NOT const and deliberately in DRAM: the IRAM_ATTR dispatcher and
 * esp_intr_noniram_disable() both read it while the flash cache may be off.
 */
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
/*
 * Number of INTSTATUS words, from the intc node's pending-status reg size. The
 * compat-explicit DT_INST() form is required here rather than DT_INST_*, which
 * would need a DT_DRV_COMPAT this header cannot assume.
 */
#define ESP_INTR_STATUS_WORDS (DT_REG_SIZE_BY_IDX(DT_INST(0, espressif_esp32_intc), 1) / 4)
#endif

struct esp_intr_line {
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	/* Sources enabled on this line. Bits follows the INTSTATUS registers */
	uint32_t status_mask[ESP_INTR_STATUS_WORDS];
	/* Number of set bits across status_mask */
	uint8_t shares_count;
#endif
	/* Connect-path clients on this line */
	uint8_t total_clients;
	/* Subset of total_clients that are not IRAM-safe */
	uint8_t non_iram_clients;
	/* Line may keep running with the flash cache disabled */
	bool iram_capable;
};

extern struct esp_intr_line esp_intr_clients[CONFIG_MP_MAX_NUM_CPUS][SOC_CPU_INTR_NUM];

/**
 * @brief Register IRAM flags for a CPU interrupt line connect path client.
 *
 * All clients on the same CPU IRQ line must agree on IRAM capability.
 * Returns -EINVAL if registration would mix IRAM and non-IRAM clients.
 * Updates non_iram_int_mask when any non-IRAM client is present.
 */
int z_soc_irq_flags_apply(unsigned int irq, uint32_t flags);

/**
 * @brief Undo a prior z_soc_irq_flags_apply() for the same irq/flags pair.
 */
int z_soc_irq_flags_clear(unsigned int irq, uint32_t flags);

/**
 * @brief Validate ISR routine and flags.
 */
int z_soc_irq_validate(void (*isr)(const void *parameter), uint32_t flags);

#if defined(CONFIG_ZTEST)
uint8_t z_soc_irq_line_total_clients_get(unsigned int irq);
uint8_t z_soc_irq_line_non_iram_clients_get(unsigned int irq);
uint32_t z_soc_irq_non_iram_int_mask_get(unsigned int irq);
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
uint8_t z_soc_irq_mli_shares_count_get(unsigned int cpu_line);
bool z_soc_irq_mli_source_enabled(unsigned int cpu_line, unsigned int source);
#endif
#endif

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_ */
