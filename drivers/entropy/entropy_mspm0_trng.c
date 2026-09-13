/*
 * Copyright (c) 2025 Linumiz GmbH
 * Copyright (c) 2026 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_trng

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>

#define TRNG_MSPM0_PWREN_MASK				BIT(0)
#define TRNG_MSPM0_PWREN_KEY_MASK			GENMASK(31, 24)
#define TRNG_MSPM0_PWREN_KEY				0x26

#define TRNG_MSPM0_CLKDIVIDE_MASK			BIT_MASK(3)
#define TRNG_MSPM0_MIN_FREQ				MHZ(9.5)
#define TRNG_MSPM0_MAX_FREQ				MHZ(25)

#define TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK		BIT(3)
#define TRNG_MSPM0_IIDX_CMD_DONE_MASK			BIT(2)
#define TRNG_MSPM0_IIDX_CMD_FAIL_MASK			BIT(1)
#define TRNG_MSPM0_IIDX_HEALTH_FAIL_MASK		BIT(0)

#define TRNG_MSPM0_CTL_CMD_MASK				BIT_MASK(2)
#define TRNG_MSPM0_CTL_CMD_OFF				0x0
#define TRNG_MSPM0_CTL_CMD_PWRUP_DIG			0x1
#define TRNG_MSPM0_CTL_CMD_PWRUP_ANA			0x2
#define TRNG_MSPM0_CTL_CMD_NORM_FUNC			0x3

#define TRNG_MSPM0_TEST_RESULTS_DIG_TEST_MASK		GENMASK(7, 0)
#define TRNG_MSPM0_TEST_RESULTS_DIG_TEST_SUCCESS	BIT_MASK(8)
#define TRNG_MSPM0_TEST_RESULTS_ANA_TEST_MASK		BIT(8)
#define TRNG_MSPM0_TEST_RESULTS_ANA_TEST_SUCCESS	BIT(8)


#define TRNG_MSPM0_DECIMATION_RATE		(CONFIG_ENTROPY_MSPM0_TRNG_DECIMATION_RATE - 1)
#define TRNG_MSPM0_DECIMATION_RATE_MASK		GENMASK(10, 8)
#define TRNG_SAMPLE_SIZE			4

#define BEST_CLK_DIV(freq) \
	((freq) <= MHZ(25) ? 1 : \
	(freq) <= MHZ(25) * 2  ? 2 : \
	(freq) <= MHZ(25) * 4  ? 4 : \
	(freq) <= MHZ(25) * 6  ? 6 : 8)
typedef struct {
	uint32_t reserved0[0x200];
	volatile uint32_t pwren;	/* Power Enable				@0x800h */
	volatile uint32_t rstctl;	/* Reset Control			@0x804h */
	uint32_t reserved1[3];
	volatile uint32_t stat;		/* Status Register			@0x814h */
	uint32_t reserved2[0x202];
	volatile uint32_t iidx;		/* Interrupt Index			@0x1020h */
	uint32_t reserved3[1];
	volatile uint32_t imask;	/* Interrupt Mask			@0x1028h */
	uint32_t reserved4[1];
	volatile uint32_t ris;		/* Raw Interrupt Status			@0x1030h */
	uint32_t reserved5[1];
	volatile uint32_t mis;		/* Masked Interrupt Status		@0x1038h */
	uint32_t reserved6[1];
	volatile uint32_t iset;		/* Interrupt Set			@0x1040h */
	uint32_t reserved7[1];
	volatile uint32_t iclr;		/* Interrupt Clear			@0x1048h */
	uint32_t reserved8[44];
	volatile uint32_t desc;		/* Module Description			@0x10FCh */
	volatile uint32_t ctl;		/* Control				@0x1100h */
	volatile uint32_t hlth_stat;	/* TRNG Status				@0x1104h */
	volatile uint32_t data_capture;	/* Captured RNG Word Buffer		@0x1108h */
	volatile uint32_t test_results;	/* TEST_ANA and TEST_DIG Results	@0x110Ch */
	volatile uint32_t clk_divide;	/* Clock Divider			@0x1110h */
} trng_ti_mspm0_reg_t;

struct entropy_mspm0_trng_config {
	trng_ti_mspm0_reg_t *regs;
	struct mspm0_sys_clock clock_subsys;
};

struct entropy_mspm0_trng_data {
	struct k_mutex mutex_lock;
	struct k_sem sem_sync;
	struct ring_buf entropy_pool;
	uint8_t pool_buffer[CONFIG_ENTROPY_MSPM0_TRNG_POOL_SIZE];
	uint32_t sample_generate_time_us;
};

static inline bool entropy_mspm0_trng_run_dig_test(trng_ti_mspm0_reg_t *regs)
{
	uint32_t dig_test = (regs->test_results & TRNG_MSPM0_TEST_RESULTS_DIG_TEST_MASK);

	if (dig_test == TRNG_MSPM0_TEST_RESULTS_DIG_TEST_SUCCESS) {
		return true;
	}

	regs->ctl = (regs->ctl & ~TRNG_MSPM0_CTL_CMD_MASK) | TRNG_MSPM0_CTL_CMD_PWRUP_DIG;
	/* Test needs to run, return false to indicate ISR should return */
	return false;
}

static inline bool entropy_mspm0_trng_run_ana_test(trng_ti_mspm0_reg_t *regs)
{
	uint32_t ana_test = (regs->test_results & TRNG_MSPM0_TEST_RESULTS_ANA_TEST_MASK);

	if (ana_test == TRNG_MSPM0_TEST_RESULTS_ANA_TEST_SUCCESS) {
		return true;
	}

	regs->ctl = (regs->ctl & ~TRNG_MSPM0_CTL_CMD_MASK) | TRNG_MSPM0_CTL_CMD_PWRUP_ANA;
	/* Test needs to run, return false to indicate ISR should return */
	return false;
}

static void entropy_mspm0_trng_isr(const struct device *dev)
{
	const struct entropy_mspm0_trng_config *config = dev->config;
	struct entropy_mspm0_trng_data *data = dev->data;
	uint32_t status;
	uint32_t entropy_data;
	uint32_t bytes_written;
	bool dig_test;
	bool ana_test;

	status = config->regs->mis & (TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK |
				      TRNG_MSPM0_IIDX_CMD_DONE_MASK	|
				      TRNG_MSPM0_IIDX_HEALTH_FAIL_MASK);

	if (status & TRNG_MSPM0_IIDX_HEALTH_FAIL_MASK) {
		config->regs->iclr = TRNG_MSPM0_IIDX_HEALTH_FAIL_MASK |
					TRNG_MSPM0_IIDX_CMD_DONE_MASK;
		config->regs->ctl = (config->regs->ctl & ~TRNG_MSPM0_CTL_CMD_MASK) |
					TRNG_MSPM0_CTL_CMD_OFF;
		return;
	}

	if (status & TRNG_MSPM0_IIDX_CMD_DONE_MASK) {
		config->regs->iclr = TRNG_MSPM0_IIDX_CMD_DONE_MASK;

		/* Run DIG test */
		dig_test = entropy_mspm0_trng_run_dig_test(config->regs);
		if (!dig_test) {
			return;
		}

		/* Run ANALOG test */
		ana_test = entropy_mspm0_trng_run_ana_test(config->regs);
		if (!ana_test) {
			return;
		}

		/*
		 * If both tests are successful, discard first sample from DATA_CAPTURE register
		 * and set DECIM RATE, enable IRQ_CAPTURE_RDY
		 */
		if (dig_test && ana_test) {
			(void)config->regs->data_capture;
			config->regs->iclr = TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK;
			config->regs->ctl = (config->regs->ctl &
					     ~TRNG_MSPM0_DECIMATION_RATE_MASK) |
					     FIELD_PREP(TRNG_MSPM0_DECIMATION_RATE_MASK,
					     TRNG_MSPM0_DECIMATION_RATE);
			config->regs->imask = (config->regs->imask &
					       ~TRNG_MSPM0_IIDX_CMD_DONE_MASK) |
					       TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK;
			return;
		}
	}

	if (status & TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK) {
		entropy_data = config->regs->data_capture;
		bytes_written = ring_buf_put(&data->entropy_pool, (uint8_t *)&entropy_data,
					     TRNG_SAMPLE_SIZE);

		/* If the ring buf is exhausted, disable the interrupt in IMASK */
		if (bytes_written < TRNG_SAMPLE_SIZE) {
			config->regs->imask &= ~TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK;
		}

		k_sem_give(&data->sem_sync);
	}
}

static int entropy_mspm0_trng_get_entropy(const struct device *dev,
					  uint8_t *buffer, uint16_t length)
{
	const struct entropy_mspm0_trng_config *config = dev->config;
	struct entropy_mspm0_trng_data *data = dev->data;
	uint16_t bytes_read;

	k_mutex_lock(&data->mutex_lock, K_FOREVER);

	while (length) {
		bytes_read = ring_buf_get(&data->entropy_pool, buffer, length);

		/*
		 * If no bytes read, i.e ring buf is exhausted, enable the interrupt and
		 * wait until the additional entropy is available in ring buf.
		 */
		if (bytes_read == 0U) {
			config->regs->imask |= TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK;
			k_sem_take(&data->sem_sync, K_FOREVER);
			continue;
		}
		buffer += bytes_read;
		length -= bytes_read;
	}

	k_mutex_unlock(&data->mutex_lock);

	return 0;
}

static int entropy_mspm0_trng_get_entropy_isr(const struct device *dev, uint8_t *buffer,
					      uint16_t length, uint32_t flags)
{
	const struct entropy_mspm0_trng_config *config = dev->config;
	struct entropy_mspm0_trng_data *data = dev->data;
	uint16_t bytes_read;
	uint16_t total_read;
	uint32_t entropy_data;
	unsigned int key;

	/* Try to get entropy from existing ring buffer */
	key = irq_lock();
	bytes_read = ring_buf_get(&data->entropy_pool, buffer, length);
	total_read = bytes_read;

	if ((bytes_read == length) || ((flags & ENTROPY_BUSYWAIT) == 0U)) {
		/* Either we got all requested data, or busy-waiting is not allowed */
		irq_unlock(key);
		return total_read;
	}

	/* Busy-wait for additional data (only if ENTROPY_BUSYWAIT is set) */
	buffer += bytes_read;
	length -= bytes_read;

	while (length) {
		/* Check if data is ready by checking IRQ_CAPTURED_RDY */
		if (config->regs->ris & TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK) {
			entropy_data = config->regs->data_capture;
			config->regs->iclr = TRNG_MSPM0_IIDX_CAPTURED_RDY_MASK;
			bytes_read = (length >= TRNG_SAMPLE_SIZE) ? TRNG_SAMPLE_SIZE : length;

			for (uint8_t i = 0; i < bytes_read; i++) {
				buffer[i] = ((uint8_t *)&entropy_data)[i];
			}

			buffer += bytes_read;
			length -= bytes_read;
			total_read += bytes_read;
		} else {
			k_busy_wait(data->sample_generate_time_us);
		}
	}

	irq_unlock(key);

	return total_read;
}

static int entropy_mspm0_trng_init(const struct device *dev)
{
	const struct entropy_mspm0_trng_config *config = dev->config;
	struct entropy_mspm0_trng_data *data = dev->data;
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	uint32_t mclk_freq;
	uint8_t clk_div;
	int ret;

	/* Initialize ring buffer for entropy storage */
	ring_buf_init(&data->entropy_pool, sizeof(data->pool_buffer), data->pool_buffer);

	/* Enable TRNG power */
	if (!(config->regs->pwren & TRNG_MSPM0_PWREN_MASK)) {
		/* Write Power enable key and set Power enable bit simultaneously */
		config->regs->pwren =
			FIELD_PREP(TRNG_MSPM0_PWREN_KEY_MASK, TRNG_MSPM0_PWREN_KEY) |
			TRNG_MSPM0_PWREN_MASK;
	}

	ret = clock_control_get_rate(clk_dev,
				    (clock_control_subsys_t)(uintptr_t)&config->clock_subsys,
				    &mclk_freq);
	if (ret < 0) {
		return ret;
	}

	clk_div = BEST_CLK_DIV(mclk_freq);
	data->sample_generate_time_us = 1000000U * 32U * (TRNG_MSPM0_DECIMATION_RATE + 1U) /
					(mclk_freq / clk_div);

	/* Configure TRNG clock divider */
	/* Register values are 1 less than divide by values */
	config->regs->clk_divide = (clk_div - 1) & TRNG_MSPM0_CLKDIVIDE_MASK;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    entropy_mspm0_trng_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* Enable CMD_DONE and HEALTH_FAIL interrupts */
	config->regs->imask = TRNG_MSPM0_IIDX_CMD_DONE_MASK | TRNG_MSPM0_IIDX_HEALTH_FAIL_MASK;

	/* Move TRNG from OFF to NORM FUNC state */
	config->regs->ctl = (config->regs->ctl & ~TRNG_MSPM0_CTL_CMD_MASK) |
			     TRNG_MSPM0_CTL_CMD_NORM_FUNC;

	return 0;
}

static DEVICE_API(entropy, entropy_mspm0_trng_driver_api) = {
	.get_entropy = entropy_mspm0_trng_get_entropy,
	.get_entropy_isr = entropy_mspm0_trng_get_entropy_isr,
};

static const struct entropy_mspm0_trng_config entropy_mspm0_trng_config = {
	.regs = (trng_ti_mspm0_reg_t *)DT_INST_REG_ADDR(0),
	.clock_subsys = MSPM0_CLOCK_SUBSYS_FN(0),
};

static struct entropy_mspm0_trng_data entropy_mspm0_trng_data = {
	.mutex_lock = Z_MUTEX_INITIALIZER(entropy_mspm0_trng_data.mutex_lock),
	.sem_sync = Z_SEM_INITIALIZER(entropy_mspm0_trng_data.sem_sync, 0, 1),
};

DEVICE_DT_INST_DEFINE(0, entropy_mspm0_trng_init, NULL,
		      &entropy_mspm0_trng_data,
		      &entropy_mspm0_trng_config, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY,
		      &entropy_mspm0_trng_driver_api);
