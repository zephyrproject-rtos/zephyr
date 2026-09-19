/*
 * Copyright (c) 2026 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing MSPI device interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>

/* MSPI driver config */

int mspi_if_init(struct qspi_config *config);

int mspi_if_deinit(void);

int mspi_if_write(unsigned int addr, const void *data, int len);

int mspi_if_read(unsigned int addr, void *data, int len);

int mspi_if_hl_read(unsigned int addr, void *data, int len);

int mspi_if_cmd_rpu_wakeup_fn(uint32_t data);

int mspi_if_wait_while_rpu_awake(void);

int mspi_if_validate_rpu_wake_writecmd(void);

int mspi_if_cmd_sleep_rpu_fn(void);

int mspi_if_RDSR1(const struct device *dev, uint8_t *rdsr1);

int mspi_if_RDSR2(const struct device *dev, uint8_t *rdsr2);

int mspi_if_WRSR2(const struct device *dev, const uint8_t wrsr2);

/**
 * @brief Read a register via MSPI
 *
 * @param dev MSPI device (unused, kept for compatibility)
 * @param reg_addr Register address (opcode)
 * @param reg_value Pointer to store the read value
 * @return int 0 on success, negative error code on failure
 */
int mspi_if_read_reg_wrapper(const struct device *dev, uint8_t reg_addr, uint8_t *reg_value);

/**
 * @brief Write a register via MSPI
 *
 * @param dev MSPI device (unused, kept for compatibility)
 * @param reg_addr Register address (opcode)
 * @param reg_value Value to write
 * @return int 0 on success, negative error code on failure
 */
int mspi_if_write_reg_wrapper(const struct device *dev, uint8_t reg_addr, uint8_t reg_value);
