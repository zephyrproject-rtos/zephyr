/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_FPGA_MANAGER_H_
#define ZEPHYR_INCLUDE_FPGA_MANAGER_H_

#include <zephyr/kernel.h>

#define FPGA_RECONFIG_STATUS_BUF_SIZE 400

/**
 * @file
 *
 * @brief API for FPGA Manager
 */

/**
 * @brief Check the status of fpga configuration
 *
 * @param[out] status Pointer to store the FPGA configuration status.
 * The value is valid only when the return value is 0.
 *
 * @return 0 on success, negative errno code on fail
 */
int32_t fpga_get_status(void *status);

/**
 * @brief Configures the FPGA with given pointer to location that contains the
 * bitstream.
 *
 * @param[in] image_ptr Pointer to memory location that contains the bitstream.
 * If the pointer and size is outside of reserved memory range, shall copy the bitstream into
 * the reserved memory.
 * @param[in] byte_size Bitstream size in byte
 *
 * @return 0 on success, negative errno code on fail
 */
int32_t fpga_load(char *image_ptr, uint32_t byte_size);

/**
 * @brief Configures the FPGA with given bitstream file in file system.
 *
 * @param[in] filename Bitstream file that will be used to configure the FPGA.
 * @param[in] config_type Configuration type Full and Partial configuration
 *
 * @return 0 on success, negative errno code on fail
 */
int32_t fpga_load_file(const char *filename, uint32_t config_type);

/**
 * @brief Get the memory address and size to store the bitstream.
 *
 * @param[out] phyaddr Pointer to store the start address
 * @param[out] size Pointer to store the reserved memory size
 *
 * @return 0 on success, negative errno code on fail
 */
int32_t fpga_get_memory(char **phyaddr, uint32_t *size);

#endif
