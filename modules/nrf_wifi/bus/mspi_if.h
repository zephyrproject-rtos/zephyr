/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing MSPI device interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr/drivers/mspi.h>

/* MSPI driver config */

int mspi_init(struct qspi_config *config);

int mspi_deinit(void);

int mspi_write(unsigned int addr, const void *data, int len);

int mspi_read(unsigned int addr, void *data, int len);

int mspi_hl_read(unsigned int addr, void *data, int len);

int mspi_cmd_sleep_rpu_fn(void);

int mspi_validate_rpu_wake_writecmd(void);

int mspi_cmd_rpu_wakeup_fn(uint8_t data);

int mspi_wait_while_rpu_awake(void);

int mspi_write_reg(uint8_t reg_addr, const uint8_t reg_value);

int mpsi_read_reg(uint8_t reg_addr, uint8_t *reg_value);
