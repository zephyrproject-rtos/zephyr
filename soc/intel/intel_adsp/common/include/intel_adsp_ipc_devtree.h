/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H
#define ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H

#include <zephyr/devicetree.h>
#include <stdint.h>

/**
 * @brief Retrieve node identifier for Intel ADSP host IPC.
 */
#define INTEL_ADSP_IPC_HOST_DTNODE DT_NODELABEL(adsp_host_ipc)

/** @brief Host IPC device pointer.
 *
 * This macro expands to the registered host IPC device from
 * devicetree (if one exists!). The device will be initialized and
 * ready at system startup.
 */
#define INTEL_ADSP_IPC_HOST_DEV DEVICE_DT_GET(INTEL_ADSP_IPC_HOST_DTNODE)

/**
 * @brief IPC register block.
 *
 * This macro retrieves host IPC register address from devicetree.
 */
#define INTEL_ADSP_IPC_REG_ADDRESS DT_REG_ADDR(INTEL_ADSP_IPC_HOST_DTNODE)

/**
 * @brief Retrieve node identifier for Intel ADSP IDC.
 */
#define INTEL_ADSP_IDC_DTNODE DT_NODELABEL(adsp_idc)

/**
 * @brief IDC register block.
 *
 * This macro retrieves IDC register address from devicetree.
 */
#define INTEL_ADSP_IDC_REG_ADDRESS DT_REG_ADDR(INTEL_ADSP_IDC_DTNODE)

#endif /* ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H */
