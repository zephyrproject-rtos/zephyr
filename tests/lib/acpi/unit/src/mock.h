/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>

/* Exclude PCIE header */
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_H_

/* Exclude Logging macrobatic */
#define ZEPHYR_INCLUDE_LOGGING_LOG_H_
#define LOG_MODULE_REGISTER(...)
#define LOG_DBG(...)
#define LOG_ERR(...)
#define LOG_WRN(...)

#define CONFIG_ACPI_IRQ_VECTOR_MAX 32
#define CONFIG_ACPI_MMIO_ENTRIES_MAX 32

typedef uint32_t pcie_bdf_t;
