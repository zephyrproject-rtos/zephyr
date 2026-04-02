/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing SPI device interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr/drivers/spi.h>

/* SPIM driver config */

int spim_init(struct qspi_config *config);

int spim_deinit(void);

int spim_write(unsigned int addr, const void *data, int len);

int spim_read(unsigned int addr, void *data, int len);

int spim_hl_read(unsigned int addr, void *data, int len);

int spim_cmd_rpu_wakeup_fn(uint32_t data);

int spim_wait_while_rpu_awake(void);

int spi_validate_rpu_wake_writecmd(void);

int spim_cmd_sleep_rpu_fn(void);

int spim_RDSR1(const struct device *dev, uint8_t *rdsr1);

int spim_RDSR2(const struct device *dev, uint8_t *rdsr2);

int spim_WRSR2(const struct device *dev, const uint8_t wrsr2);

/**
 * @brief Read a register via SPI
 *
 * @param reg_addr Register address (opcode)
 * @param reg_value Pointer to store the read value
 * @return int 0 on success, negative error code on failure
 */
int spim_read_reg(uint32_t reg_addr, uint8_t *reg_value);

/**
 * @brief Write a register via SPI
 *
 * @param spi_spec SPI device specification
 * @param reg_addr Register address (opcode)
 * @param reg_value Value to write
 * @return int 0 on success, negative error code on failure
 */
int spim_write_reg(const struct spi_dt_spec *spi_spec, uint32_t reg_addr, const uint8_t reg_value);

/**
 * @brief Read a register via SPI (wrapper for compatibility)
 *
 * @param dev SPI device (unused, kept for compatibility)
 * @param reg_addr Register address (opcode)
 * @param reg_value Pointer to store the read value
 * @return int 0 on success, negative error code on failure
 */
int spim_read_reg_wrapper(const struct device *dev, uint8_t reg_addr, uint8_t *reg_value);

/**
 * @brief Write a register via SPI (wrapper for compatibility)
 *
 * @param dev SPI device (unused, kept for compatibility)
 * @param reg_addr Register address (opcode)
 * @param reg_value Value to write
 * @return int 0 on success, negative error code on failure
 */
int spim_write_reg_wrapper(const struct device *dev, uint8_t reg_addr, uint8_t reg_value);
