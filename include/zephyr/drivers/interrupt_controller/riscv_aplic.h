/*
 * Copyright (c) 2025 Institute of Software, Chinese Academy of Sciences (ISCAS)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RISCV_APLIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RISCV_APLIC_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

/**
 * @brief Enable interrupt
 */
void riscv_aplic_irq_enable(uint32_t irq);
/**
 * @brief Disable interrupt
 */
void riscv_aplic_irq_disable(uint32_t irq);
/**
 * @brief Check enabled
 */
int riscv_aplic_irq_is_enabled(uint32_t irq);
/**
 * @brief Set priority
 */
void riscv_aplic_set_priority(uint32_t irq, uint32_t prio);
/**
 * @brief Set pending
 */
void riscv_aplic_irq_set_pending(uint32_t irq);
/**
 * @brief Check pending
 */
int riscv_aplic_irq_is_pending(uint32_t irq);
/**
 * @brief Get active IRQ
 */
unsigned int riscv_aplic_get_irq(void);
/**
 * @brief Get active device
 */
const struct device *riscv_aplic_get_dev(void);
/**
 * @brief Configure source
 */
int riscv_aplic_configure_source(uint32_t irq, uint32_t mode, uint32_t hart, uint32_t priority);

/* Advanced interrupt management */
enum riscv_aplic_trigger_type {
	RISCV_APLIC_TRIGGER_EDGE_RISING = 4,
	RISCV_APLIC_TRIGGER_EDGE_FALLING = 5,
	RISCV_APLIC_TRIGGER_LEVEL_HIGH = 6,
	RISCV_APLIC_TRIGGER_LEVEL_LOW = 7,
};

/**
 * @brief Set hart threshold
 */
int riscv_aplic_set_hart_threshold(uint32_t hart_id, uint32_t threshold);
/**
 * @brief Get direct count
 */
uint32_t riscv_aplic_get_direct_interrupts(void);
/**
 * @brief Route source to hart
 */
int riscv_aplic_route_source(uint32_t irq, uint32_t hart);
/**
 * @brief Get handler calls
 */
uint32_t riscv_aplic_get_handler_calls(void);

/**
 * @brief Get total count
 */
uint32_t riscv_aplic_get_total_interrupts(void);
/**
 * @brief Reset statistics
 */
void riscv_aplic_reset_stats(void);

/**
 * @brief Read DOMAINCFG
 */
uint32_t aplic_read_domain_config(void);
/**
 * @brief Read IDC IDELIVERY
 */
uint32_t aplic_read_idc_delivery(void);
/**
 * @brief Read IDC ITHRESHOLD
 */
uint32_t aplic_read_idc_threshold(void);
/**
 * @brief Read IDC TOPI
 */
uint32_t aplic_read_idc_topi(void);



#endif /* ZEPHYR_INCLUDE_DRIVERS_RISCV_APLIC_H_ */
