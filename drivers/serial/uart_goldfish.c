/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT google_goldfish_tty

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/byteorder.h>

#define GOLDFISH_TTY_REG_PUT_CHAR    0x00U
#define GOLDFISH_TTY_REG_BYTES_READY 0x04U
#define GOLDFISH_TTY_REG_CMD         0x08U
#define GOLDFISH_TTY_REG_DATA_PTR    0x10U
#define GOLDFISH_TTY_REG_DATA_LEN    0x14U
#define GOLDFISH_TTY_MMIO_SIZE       0x18U

#define GOLDFISH_TTY_CMD_READ_BUFFER 3U

struct goldfish_tty_config {
	mm_reg_t base;
	bool big_endian;
};

static uint32_t goldfish_tty_read32(const struct goldfish_tty_config *config,
				    uint32_t offset)
{
	uint32_t value = sys_read32(config->base + offset);

	if (config->big_endian) {
		return sys_be32_to_cpu(value);
	}

	return sys_le32_to_cpu(value);
}

static void goldfish_tty_write32(const struct goldfish_tty_config *config,
				 uint32_t value, uint32_t offset)
{
	if (config->big_endian) {
		value = sys_cpu_to_be32(value);
	} else {
		value = sys_cpu_to_le32(value);
	}

	sys_write32(value, config->base + offset);
}

static int goldfish_tty_poll_in(const struct device *dev, unsigned char *c)
{
	const struct goldfish_tty_config *config = dev->config;

	if (goldfish_tty_read32(config, GOLDFISH_TTY_REG_BYTES_READY) == 0U) {
		return -1;
	}

	goldfish_tty_write32(config, (uint32_t)k_mem_phys_addr(c),
			     GOLDFISH_TTY_REG_DATA_PTR);
	goldfish_tty_write32(config, 1U, GOLDFISH_TTY_REG_DATA_LEN);
	goldfish_tty_write32(config, GOLDFISH_TTY_CMD_READ_BUFFER,
			     GOLDFISH_TTY_REG_CMD);

	return 0;
}

static void goldfish_tty_poll_out(const struct device *dev, unsigned char c)
{
	goldfish_tty_write32(dev->config, c, GOLDFISH_TTY_REG_PUT_CHAR);
}

static DEVICE_API(uart, goldfish_tty_api) = {
	.poll_in = goldfish_tty_poll_in,
	.poll_out = goldfish_tty_poll_out,
};

#define GOLDFISH_TTY_INIT(inst)					\
	BUILD_ASSERT(DT_INST_REG_SIZE(inst) >= GOLDFISH_TTY_MMIO_SIZE, \
		     "Goldfish TTY MMIO region is too small");	\
	static const struct goldfish_tty_config goldfish_tty_config_##inst = { \
		.base = DT_INST_REG_ADDR(inst),				\
		.big_endian = DT_INST_PROP(inst, big_endian),		\
	};								\
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL,			\
			      &goldfish_tty_config_##inst, PRE_KERNEL_1, \
			      CONFIG_SERIAL_INIT_PRIORITY,		\
			      &goldfish_tty_api);

DT_INST_FOREACH_STATUS_OKAY(GOLDFISH_TTY_INIT)
