/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing SPI device interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

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
