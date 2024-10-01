/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing QSPI device interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __QSPI_IF_H__
#define __QSPI_IF_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_NRF70_ON_QSPI
#include <nrfx_qspi.h>
#endif

#define RPU_WAKEUP_NOW BIT(0) /* WAKEUP RPU - RW */
#define RPU_AWAKE_BIT BIT(1) /* RPU AWAKE FROM SLEEP - RO */
#define RPU_READY_BIT BIT(2) /* RPU IS READY - RO*/

struct qspi_config {
#ifdef CONFIG_NRF70_ON_QSPI
	nrf_qspi_addrmode_t addrmode;
	nrf_qspi_readoc_t readoc;
	nrf_qspi_writeoc_t writeoc;
	nrf_qspi_frequency_t sckfreq;
#endif
	unsigned char RDC4IO;
	bool easydma;
	bool single_op;
	bool quad_spi;
	bool encryption;
	bool CMD_CNONCE;
	bool enc_enabled;
	struct k_sem lock;
	unsigned int addrmask;
	unsigned char qspi_slave_latency;
#if defined(CONFIG_NRF70_ON_QSPI) && (NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC)
	nrf_qspi_encryption_t p_cfg;
#endif /*CONFIG_NRF70_ON_QSPI && (NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC)*/
	int test_hlread;
	char *test_name;
	int test_start;
	int test_end;
	int test_iterations;
	int test_timediff_read;
	int test_timediff_write;
	int test_status;
	int test_iteration;
};
struct qspi_dev {
	int (*deinit)(void);
	void *config;
	int (*init)(struct qspi_config *config);
	int (*write)(unsigned int addr, const void *data, int len);
	int (*read)(unsigned int addr, void *data, int len);
	int (*hl_read)(unsigned int addr, void *data, int len);
	void (*hard_reset)(void);
};

int qspi_cmd_wakeup_rpu(const struct device *dev, uint8_t data);

int qspi_init(struct qspi_config *config);

int qspi_write(unsigned int addr, const void *data, int len);

int qspi_read(unsigned int addr, void *data, int len);

int qspi_hl_read(unsigned int addr, void *data, int len);

int qspi_deinit(void);

void gpio_free_irq(int pin, struct gpio_callback *button_cb_data);

int gpio_request_irq(int pin, struct gpio_callback *button_cb_data, void (*irq_handler)());

struct qspi_config *qspi_defconfig(void);

struct qspi_dev *qspi_dev(void);
struct qspi_config *qspi_get_config(void);

int qspi_cmd_sleep_rpu(const struct device *dev);

void hard_reset(void);
void get_sleep_stats(uint32_t addr, uint32_t *buff, uint32_t wrd_len);

extern struct device qspi_perip;

int qspi_validate_rpu_wake_writecmd(const struct device *dev);
int qspi_cmd_wakeup_rpu(const struct device *dev, uint8_t data);
int qspi_wait_while_rpu_awake(const struct device *dev);

int qspi_RDSR1(const struct device *dev, uint8_t *rdsr1);
int qspi_RDSR2(const struct device *dev, uint8_t *rdsr2);
int qspi_WRSR2(const struct device *dev, const uint8_t wrsr2);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
int func_rpu_sleep(void);
int func_rpu_wake(void);
int func_rpu_sleep_status(void);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

#define QSPI_KEY_LEN_BYTES 16

/*! \brief Enable encryption
 *
 *  \param key Pointer to the 128-bit key
 *  \return 0 on success, negative errno code on failure.
 */
int qspi_enable_encryption(uint8_t *key);

#endif /* __QSPI_IF_H__ */
