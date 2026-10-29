/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree values of the espressif,esp32-l3-intc binding
 * @ingroup dt_espressif_intmux_common
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESPRESSIF_ESP32_L3_INTC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESPRESSIF_ESP32_L3_INTC_H_

/**
 * @brief "bits" entry of an "espressif,esp32-l3-intc" aggregator that owns no
 * status bit.
 *
 * The input it stands for is called on every interrupt of the aggregator's
 * source, after the inputs listed before it, so the handler must check its own
 * peripheral state and return when it has nothing to do. Use it for a handler
 * that shares a source with others but has no bit in the aggregator's status
 * register, and list it last.
 */
#define ESP_L3_CATCH_ALL 0xFF

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESPRESSIF_ESP32_L3_INTC_H_ */
