/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Polled driver for the Inside Secure SafeXcel EIP-76(d) true random number
 * generator on the TI AM62L.
 *
 * The core may already have been started by boot firmware or by the secure
 * world. Then its configuration is left untouched and the driver only
 * consumes 128-bit blocks from the output registers.
 */

#define DT_DRV_COMPAT inside_secure_safexcel_eip76

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(entropy_safexcel_eip76, CONFIG_ENTROPY_LOG_LEVEL);

/* Register offsets, AM62L TRM SPRUJB4A, section 14.8.1 */
#define EIP76_OUTPUT(n)    (0x00U + ((n) * 4U))
#define EIP76_STATUS       0x10U /* read */
#define EIP76_INTACK       0x10U /* write */
#define EIP76_CONTROL      0x14U
#define EIP76_CONFIG       0x18U
#define EIP76_FROENABLE    0x20U
#define EIP76_FRODETUNE    0x24U
#define EIP76_ALARMMASK    0x28U
#define EIP76_ALARMSTOP    0x2cU
#define EIP76_EIP_REV      0x7cU

#define EIP76_OUTPUT_WORDS 4U

#define EIP76_STATUS_READY         BIT(0)
#define EIP76_STATUS_SHUTDOWN_OFLO BIT(1)
#define EIP76_STATUS_STUCK_OUT     BIT(2)
#define EIP76_STATUS_NOISE_FAIL    BIT(3)
/* Fatal per the TRM: never acknowledged, INTACK bits [7:2] reset core logic */
#define EIP76_STATUS_FATAL (EIP76_STATUS_STUCK_OUT | EIP76_STATUS_NOISE_FAIL)

#define EIP76_INTACK_READY_ACK         BIT(0)
#define EIP76_INTACK_SHUTDOWN_OFLO_ACK BIT(1)

/* TRNG_FROENABLE, TRNG_FRODETUNE, TRNG_ALARMMASK, TRNG_ALARMSTOP: one bit per FRO */
#define EIP76_FRO_MASK GENMASK(7, 0)

#define EIP76_CONTROL_ENABLE_TRNG BIT(10)

#define EIP76_CONFIG_READ_TIMEOUT_MASK GENMASK(15, 12)

/*
 * CONFIG value used only when the core is found stopped: sample_cycles = 0x22,
 * noise_blocks = 5, secure read mode off. Boot firmware leaves this value
 * programmed on the AM62L EVM; the TRM defers the exact tuning rules to the
 * EIP-76d hardware reference manual.
 */
#define EIP76_CONFIG_DEFAULT 0x00220005U

/* TRNG_EIP_REV: basic EIP number and its one's complement */
#define EIP76_EIP_REV_NUMBER_MASK GENMASK(15, 0)
#define EIP76_EIP_REV_NUMBER      0xb34cU

#define EIP76_BLOCK_TIMEOUT_US 10000U

#define EIP76_RECOVERY_TIMEOUT_US 200000U

/* FRO recoveries allowed within one request before it fails with -EIO */
#define EIP76_MAX_RECOVERIES 4U

struct eip76_config {
	DEVICE_MMIO_ROM;
};

struct eip76_data {
	DEVICE_MMIO_RAM;
	struct k_spinlock lock;
};

/*
 * Recover from shutdown_oflo, TRM 14.8.1. Stopped FROs are logged in ALARMSTOP.
 * Toggle their tuning bit, allowed only while they are off. Clear ALARMSTOP, which
 * holds FROENABLE at 0.
 *
 * Must be called with the driver lock held. Returns the toggled FROs, or -EIO
 * when the core has stopped itself (shutdown_fatal).
 */
static int eip76_fro_recover(mem_addr_t base)
{
	uint32_t stop = sys_read32(base + EIP76_ALARMSTOP) & EIP76_FRO_MASK;
	uint32_t enable = sys_read32(base + EIP76_FROENABLE) & EIP76_FRO_MASK;
	uint32_t detune = sys_read32(base + EIP76_FRODETUNE) & EIP76_FRO_MASK;

	if ((sys_read32(base + EIP76_CONTROL) & EIP76_CONTROL_ENABLE_TRNG) == 0U) {
		return -EIO;
	}

	sys_write32(enable & ~stop, base + EIP76_FROENABLE);
	sys_write32(detune ^ stop, base + EIP76_FRODETUNE);
	sys_write32(0U, base + EIP76_ALARMMASK);
	sys_write32(0U, base + EIP76_ALARMSTOP);
	sys_write32(enable | stop, base + EIP76_FROENABLE);
	sys_write32(EIP76_INTACK_SHUTDOWN_OFLO_ACK, base + EIP76_INTACK);

	return (int)stop;
}

/*
 * Must be called with the driver lock held. Returns -EIO on a fatal health
 * test failure and -EBUSY on an FRO shutdown overflow, which
 * eip76_fro_recover() clears.
 */
static int eip76_read_block(mem_addr_t base, uint32_t block[EIP76_OUTPUT_WORDS])
{
	uint32_t status = sys_read32(base + EIP76_STATUS);

	if ((status & EIP76_STATUS_FATAL) != 0U) {
		return -EIO;
	}

	if ((status & EIP76_STATUS_SHUTDOWN_OFLO) != 0U) {
		return -EBUSY;
	}

	if ((status & EIP76_STATUS_READY) == 0U) {
		return -EAGAIN;
	}

	for (uint32_t i = 0U; i < EIP76_OUTPUT_WORDS; i++) {
		block[i] = sys_read32(base + EIP76_OUTPUT(i));
	}

	sys_write32(EIP76_INTACK_READY_ACK, base + EIP76_INTACK);

	return 0;
}

static int eip76_fill(const struct device *dev, uint8_t *buffer, uint16_t length, bool wait)
{
	struct eip76_data *data = dev->data;
	mem_addr_t base = DEVICE_MMIO_GET(dev);
	uint32_t block[EIP76_OUTPUT_WORDS];
	uint32_t budget = EIP76_BLOCK_TIMEOUT_US;
	uint32_t recoveries = 0U;
	uint16_t filled = 0U;
	int ret = 0;

	while (filled < length) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		int toggled = -EIO;

		ret = eip76_read_block(base, block);
		if ((ret == -EBUSY) && (recoveries < EIP76_MAX_RECOVERIES)) {
			toggled = eip76_fro_recover(base);
		}
		k_spin_unlock(&data->lock, key);

		if (ret == -EBUSY) {
			if (toggled < 0) {
				ret = -EIO;
			} else {
				LOG_WRN("FRO shutdown, toggled FROs 0x%02x", (uint32_t)toggled);
				recoveries++;
				budget = EIP76_RECOVERY_TIMEOUT_US;
				continue;
			}
		}

		if (ret == 0) {
			uint16_t n = (uint16_t)MIN((size_t)(length - filled), sizeof(block));

			memcpy(&buffer[filled], block, n);
			filled += n;
			budget = EIP76_BLOCK_TIMEOUT_US;
			continue;
		}

		if (ret == -EIO) {
			LOG_ERR("TRNG failure, status 0x%08x", sys_read32(base + EIP76_STATUS));
			break;
		}

		if (!wait) {
			ret = 0;
			break;
		}

		if (budget == 0U) {
			if ((sys_read32(base + EIP76_CONTROL) & EIP76_CONTROL_ENABLE_TRNG) == 0U) {
				LOG_ERR("TRNG was disabled by another agent");
				ret = -EIO;
			} else {
				ret = -ETIMEDOUT;
			}
			break;
		}

		/*
		 * The EVM delivers a 128-bit block in about 17 us, so 10 ms per block is a
		 * wide bound; 200 ms after an FRO recovery leaves the re-enabled FROs time to
		 * refill the output.
		 */
		budget--;
		k_busy_wait(1);
	}

	memset(block, 0, sizeof(block));

	return (ret < 0) ? ret : filled;
}

static int eip76_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	int ret = eip76_fill(dev, buffer, length, true);

	return (ret < 0) ? ret : 0;
}

static int eip76_get_entropy_isr(const struct device *dev, uint8_t *buffer, uint16_t length,
				 uint32_t flags)
{
	return eip76_fill(dev, buffer, length, (flags & ENTROPY_BUSYWAIT) != 0U);
}

static int eip76_init(const struct device *dev)
{
	mem_addr_t base;
	uint32_t rev;
	uint32_t ctrl;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	base = DEVICE_MMIO_GET(dev);

	rev = sys_read32(base + EIP76_EIP_REV);
	if ((rev & EIP76_EIP_REV_NUMBER_MASK) != EIP76_EIP_REV_NUMBER) {
		LOG_ERR("unexpected EIP_REV 0x%08x", rev);
		return -ENODEV;
	}

	ctrl = sys_read32(base + EIP76_CONTROL);
	if ((ctrl & EIP76_CONTROL_ENABLE_TRNG) != 0U) {
		/* Started by firmware: keep its configuration */
		if ((sys_read32(base + EIP76_CONFIG) & EIP76_CONFIG_READ_TIMEOUT_MASK) != 0U) {
			LOG_ERR("secure read mode is enabled, not supported");
			return -ENOTSUP;
		}
		LOG_DBG("EIP-76 rev 0x%08x already running", rev);
		return 0;
	}

	sys_write32(EIP76_CONFIG_DEFAULT, base + EIP76_CONFIG);
	sys_write32(EIP76_CONTROL_ENABLE_TRNG, base + EIP76_CONTROL);
	LOG_DBG("EIP-76 rev 0x%08x started", rev);

	return 0;
}

static DEVICE_API(entropy, eip76_api) = {
	.get_entropy = eip76_get_entropy,
	.get_entropy_isr = eip76_get_entropy_isr,
};

#define EIP76_INIT(n)                                                                              \
	static const struct eip76_config eip76_config_##n = {                                      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
	};                                                                                         \
	static struct eip76_data eip76_data_##n;                                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, eip76_init, NULL, &eip76_data_##n, &eip76_config_##n,             \
			      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY, &eip76_api);

DT_INST_FOREACH_STATUS_OKAY(EIP76_INIT)
