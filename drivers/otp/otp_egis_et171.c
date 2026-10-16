/*
 * Copyright (c) 2026 Egis Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT egis_et171_otp
#define DEV_CFG(dev)  ((const struct egis_et171_config *)(dev)->config)

LOG_MODULE_REGISTER(egis_et171_otp);

struct egis_et171_config {
	DEVICE_MMIO_NAMED_ROM(control);
	DEVICE_MMIO_NAMED_ROM(mem);
};

static int otp_egis_et171_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const size_t valid_size = DT_REG_SIZE_BY_NAME(DT_DRV_INST(0), mem);
	const off_t offset_base = offset;
	mem_addr_t data_addr;
	union {
		uint32_t raw_word;
		uint8_t raw_byte[sizeof(uint32_t)];
	} buf;
	int32_t i;

	if (offset < 0 || offset > valid_size || len > valid_size - offset) {
		return -EACCES;
	}

	data_addr = (mem_addr_t)DEVICE_MMIO_NAMED_GET(dev, mem) +
		(offset / sizeof(uint32_t) * sizeof(uint32_t));

	while (len > 0) {
		/* This OTP is a 32-bit x N array of data. For safety, only 32 bits
		 * are read at a time.
		 */
		buf.raw_word = sys_read32(data_addr);
		data_addr += sizeof(uint32_t);
		i = offset % sizeof(uint32_t);
		if (i == 0 && len >= sizeof(uint32_t)) {
			*(uint32_t *)&((uint8_t *)data)[offset - offset_base] = buf.raw_word;
			offset += sizeof(uint32_t);
			len -= sizeof(uint32_t);
		} else {
			/* Unaligned head or tail data */
			do {
				((uint8_t *)data)[offset - offset_base] = buf.raw_byte[i];
				offset++;
				i++;
				len--;
			} while (len > 0 && i < sizeof(buf.raw_byte));
		}
	}

	return 0;
}

static const struct egis_et171_config otp_config = {
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(control, DT_DRV_INST(0)),
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(mem, DT_DRV_INST(0)),
};

static DEVICE_API(otp, otp_egis_171_api) = {
	/* This OTP is programmed during the SoC manufacturing process
	 * and is then used only in a read-only manner.
	 * Therefore, programming it is not supported.
	 */
	.read = otp_egis_et171_read,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &otp_config, PRE_KERNEL_1,
		      CONFIG_OTP_INIT_PRIORITY, &otp_egis_171_api);
