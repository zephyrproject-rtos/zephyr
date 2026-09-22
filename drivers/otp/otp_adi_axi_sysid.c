/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Read-only OTP driver for the Analog Devices AXI System ID core.
 *
 * The core exposes an FPGA build-info ROM behind a small MMIO register block.
 * The ROM content (board / product / git information) is presented here as
 * read-only OTP memory: otp_read() returns the raw ROM bytes. There is no
 * program() — the ROM is synthesised into the bitstream at build time, not
 * written by software. Knowledge of the ROM *format* (header, build-info,
 * checksum) is intentionally kept out of the transport driver — a consumer
 * layers a decode helper on top of otp_read() (see samples/sensor/ad463x).
 *
 * Based on the no-OS reference driver by Analog Devices.
 */

#define DT_DRV_COMPAT adi_axi_sysid

#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otp_axi_sysid, CONFIG_OTP_LOG_LEVEL);

#define AXI_SYSID_REG_VERSION        0x0000
#define AXI_SYSID_REG_ROM_ADDR_WIDTH 0x0040

#define AXI_SYSID_VER_1_00_A 0x10061
#define AXI_SYSID_VER_1_01_A 0x10161

/* addr_width guard: max 10 keeps (1 << addr_width) words = 4 KiB per window;
 * total mapped span (register window + ROM window) is up to 8 KiB.
 */
#define AXI_SYSID_MAX_ADDR_WIDTH 10

struct axi_sysid_config {
	DEVICE_MMIO_ROM;
	size_t reg_size; /* size of the `reg` block from DT, for bounds checking */
};

struct axi_sysid_data {
	DEVICE_MMIO_RAM;
	size_t rom_size;  /* ROM size in bytes */
	off_t rom_offset; /* MMIO byte offset where ROM content begins */
};

static uint32_t axi_sysid_read_reg(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static int axi_sysid_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	struct axi_sysid_data *data = dev->data;
	mm_reg_t rom = DEVICE_MMIO_GET(dev) + (uintptr_t)data->rom_offset;
	uint8_t *out = buf;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (offset < 0 || (size_t)offset + len > data->rom_size) {
		LOG_ERR("read [%ld, +%zu] past ROM size %zu", (long)offset, len, data->rom_size);
		return -EINVAL;
	}

	/*
	 * The ROM is 32-bit word addressable MMIO. Assemble arbitrary byte
	 * ranges from little-endian word reads so callers see a flat byte
	 * array matching the on-ROM layout.
	 */
	while (len != 0) {
		off_t word_off = ROUND_DOWN(offset, sizeof(uint32_t));
		size_t byte_in_word = offset & (sizeof(uint32_t) - 1);
		size_t n = MIN(sizeof(uint32_t) - byte_in_word, len);
		uint8_t word[sizeof(uint32_t)];

		sys_put_le32(sys_read32(rom + (uintptr_t)word_off), word);
		memcpy(out, &word[byte_in_word], n);

		out += n;
		offset += n;
		len -= n;
	}

	return 0;
}

static int axi_sysid_init(const struct device *dev)
{
	const struct axi_sysid_config *config = dev->config;
	struct axi_sysid_data *data = dev->data;
	uint32_t version;
	uint32_t addr_width;
	size_t rom_end;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	version = axi_sysid_read_reg(dev, AXI_SYSID_REG_VERSION);
	if (version != AXI_SYSID_VER_1_00_A && version != AXI_SYSID_VER_1_01_A) {
		LOG_ERR("unsupported version: 0x%08x", version);
		return -ENOTSUP;
	}

	addr_width = axi_sysid_read_reg(dev, AXI_SYSID_REG_ROM_ADDR_WIDTH);
	if (addr_width > AXI_SYSID_MAX_ADDR_WIDTH) {
		LOG_ERR("ROM addr width %u out of range (max %u)", addr_width,
			AXI_SYSID_MAX_ADDR_WIDTH);
		return -ENOTSUP;
	}

	data->rom_size = (1U << addr_width) * sizeof(uint32_t);
	/* ROM content is mapped immediately after the register/ROM window. */
	data->rom_offset = data->rom_size;

	rom_end = (size_t)data->rom_offset + data->rom_size;
	if (rom_end > config->reg_size) {
		LOG_ERR("ROM [%ld, +%zu) exceeds DT reg size %zu", (long)data->rom_offset,
			data->rom_size, config->reg_size);
		return -ENOTSUP;
	}

	LOG_INF("AXI SYSID v%u.%u.%c, ROM %zu bytes (read-only)", version >> 16,
		(version >> 8) & 0xff, version & 0xff, data->rom_size);

	return 0;
}

static DEVICE_API(otp, axi_sysid_driver_api) = {
	.read = axi_sysid_read,
};

#define AXI_SYSID_INIT(n)                                                                          \
	static struct axi_sysid_data axi_sysid_data_##n;                                           \
	static const struct axi_sysid_config axi_sysid_config_##n = {                              \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.reg_size = DT_REG_SIZE(DT_DRV_INST(n)),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, axi_sysid_init, NULL, &axi_sysid_data_##n, &axi_sysid_config_##n, \
			      POST_KERNEL, CONFIG_OTP_INIT_PRIORITY, &axi_sysid_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AXI_SYSID_INIT)
