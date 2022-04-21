/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESP_INTR_ALLOC_H__
#define ZEPHYR_INCLUDE_DRIVERS_ESP_INTR_ALLOC_H__

#include <stdint.h>
#include <stdbool.h>
#include <soc.h>
/*
 * Interrupt allocation flags - These flags can be used to specify
 * which interrupt qualities the code calling esp_intr_alloc* needs.
 */

/* Keep the LEVELx values as they are here; they match up with (1<<level) */
#define ESP_INTR_FLAG_LEVEL1		(1<<1)	/* Accept a Level 1 int vector, lowest priority */
#define ESP_INTR_FLAG_EDGE		    (1<<9)	/* Edge-triggered interrupt */

/* Function prototype for interrupt handler function */
typedef void (*isr_handler_t)(const void *arg);

/**
 * @brief Initializes interrupt table to its defaults
 */
void esp_intr_initialize(void);

/**
 * @brief Allocate an interrupt with the given parameters.
 *
 * This finds an interrupt that matches the restrictions as given in the flags
 * parameter, maps the given interrupt source to it and hooks up the given
 * interrupt handler (with optional argument) as well. If needed, it can return
 * a handle for the interrupt as well.
 *
 * @param source The interrupt source.
 * @param flags An ORred mask of the ESP_INTR_FLAG_* defines. These restrict the
 *               choice of interrupts that this routine can choose from. If this value
 *               is 0, it will default to allocating a non-shared interrupt of level
 *               1, 2 or 3. If this is ESP_INTR_FLAG_SHARED, it will allocate a shared
 *               interrupt of level 1. Setting ESP_INTR_FLAG_INTRDISABLED will return
 *               from this function with the interrupt disabled.
 * @param handler The interrupt handler.
 * @param arg    Optional argument for passed to the interrupt handler
 * @param ret_handle Pointer to a struct intr_handle_data_t pointer to store a handle that can
 *               later be used to request details or free the interrupt. Can be NULL if no handle
 *               is required.
 *
 * @return -EINVAL if the combination of arguments is invalid.
 *         -ENODEV No free interrupt found with the specified flags
 *         0 otherwise
 */
int esp_intr_alloc(int source,
		int flags,
		isr_handler_t handler,
		void *arg,
		void **ret_handle);

/**
 * @brief Disable the interrupt associated with the source
 *
 * @param source The interrupt source
 *
 * @return -EINVAL if the combination of arguments is invalid.
 *         0 otherwise
 */
int esp_intr_disable(int source);

/**
 * @brief Enable the interrupt associated with the source
 *
 * @param source The interrupt source
 * @return -EINVAL if the combination of arguments is invalid.
 *         0 otherwise
 */
int esp_intr_enable(int source);

/**
 * @brief Gets the current enabled interrupts
 *
 * @param status_mask_number the status mask can be 0 or 1
 * @return bitmask of enabled interrupt sources
 */
uint32_t esp_intr_get_enabled_intmask(int status_mask_number);

#endif
