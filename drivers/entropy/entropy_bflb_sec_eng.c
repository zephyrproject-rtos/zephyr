/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bouffalo Lab SEC Engine TRNG entropy driver
 */

#define DT_DRV_COMPAT bflb_sec_eng_trng

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

#include <bflb_soc.h>
#include <bouffalolab/common/sec_eng_reg.h>

LOG_MODULE_REGISTER(entropy_bflb, CONFIG_ENTROPY_LOG_LEVEL);

/* TRNG produces 32 bytes (256 bits) per read */
#define TRNG_OUTPUT_BYTES 32

/* Timeout for TRNG busy-wait (matches vendor SDK: 100ms) */
#define TRNG_TIMEOUT_US 100000

struct entropy_bflb_data {
	struct k_spinlock lock;
};

struct entropy_bflb_config {
	uintptr_t base;
};

static inline void trng_nop_delay(void)
{
	/* 4 NOPs: busy will be set 1T after trigger */
	__asm__ volatile("nop; nop; nop; nop");
}

static int trng_wait_not_busy(uint32_t base)
{
	uint32_t timeout = TRNG_TIMEOUT_US;

	while (timeout--) {
		if (!(sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET) &
		      SEC_ENG_SE_TRNG_0_BUSY)) {
			return 0;
		}
		k_busy_wait(1);
	}

	return -ETIMEDOUT;
}

static int trng_read_32bytes(uint32_t base, uint8_t out[TRNG_OUTPUT_BYTES])
{
	uint32_t regval;
	uint32_t val;
	int ret;

	/* Enable TRNG */
	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval |= SEC_ENG_SE_TRNG_0_EN;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

	/* Clear interrupt */
	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval |= SEC_ENG_SE_TRNG_0_INT_CLR_1T;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

	trng_nop_delay();

	/* Wait for any prior operation */
	ret = trng_wait_not_busy(base);
	if (ret) {
		goto out_disable;
	}

	/* Clear interrupt again */
	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval |= SEC_ENG_SE_TRNG_0_INT_CLR_1T;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

	/* Trigger TRNG generation */
	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval |= SEC_ENG_SE_TRNG_0_TRIG_1T;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

	trng_nop_delay();

	/* Wait for generation to complete */
	ret = trng_wait_not_busy(base);
	if (ret) {
		goto out_disable;
	}

	/* Read 8 x 32-bit output registers */
	for (int i = 0; i < 8; i++) {
		val = sys_read32(base + SEC_ENG_SE_TRNG_0_DOUT_0_OFFSET + (i * 4));
		out[i * 4 + 0] = val & 0xff;
		out[i * 4 + 1] = (val >> 8) & 0xff;
		out[i * 4 + 2] = (val >> 16) & 0xff;
		out[i * 4 + 3] = (val >> 24) & 0xff;
	}

	/* Clear trigger */
	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval &= ~SEC_ENG_SE_TRNG_0_TRIG_1T;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

	/* Clear output */
	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval |= SEC_ENG_SE_TRNG_0_DOUT_CLR_1T;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval &= ~SEC_ENG_SE_TRNG_0_DOUT_CLR_1T;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

out_disable:
	/* Disable TRNG */
	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval &= ~SEC_ENG_SE_TRNG_0_EN;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

	/* Final interrupt clear */
	regval = sys_read32(base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);
	regval |= SEC_ENG_SE_TRNG_0_INT_CLR_1T;
	sys_write32(regval, base + SEC_ENG_SE_TRNG_0_CTRL_0_OFFSET);

	return ret;
}

static int entropy_bflb_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	struct entropy_bflb_data *data = dev->data;
	const struct entropy_bflb_config *cfg = dev->config;
	uint8_t tmp[TRNG_OUTPUT_BYTES];
	uint16_t remaining = length;
	int ret;

	while (remaining > 0) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);

		ret = trng_read_32bytes(cfg->base, tmp);

		k_spin_unlock(&data->lock, key);

		if (ret) {
			return ret;
		}

		uint16_t copy_len = MIN(remaining, TRNG_OUTPUT_BYTES);

		memcpy(buffer, tmp, copy_len);
		buffer += copy_len;
		remaining -= copy_len;
	}

	return 0;
}

static int entropy_bflb_get_entropy_isr(const struct device *dev, uint8_t *buffer, uint16_t length,
					uint32_t flags)
{
	struct entropy_bflb_data *data = dev->data;
	const struct entropy_bflb_config *cfg = dev->config;
	uint8_t tmp[TRNG_OUTPUT_BYTES];
	uint16_t remaining = length;
	int ret;

	if (!(flags & ENTROPY_BUSYWAIT)) {
		return -ENOTSUP;
	}

	while (remaining > 0) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);

		ret = trng_read_32bytes(cfg->base, tmp);

		k_spin_unlock(&data->lock, key);

		if (ret) {
			return ret;
		}

		uint16_t copy_len = MIN(remaining, TRNG_OUTPUT_BYTES);

		memcpy(buffer, tmp, copy_len);
		buffer += copy_len;
		remaining -= copy_len;
	}

	return length;
}

static int entropy_bflb_init(const struct device *dev)
{
	const struct entropy_bflb_config *cfg = dev->config;
	uint32_t regval;

	/* Enable ring oscillator — the hardware entropy source for the TRNG's
	 * internal DRBG. Without this, the DRBG reseeds from nothing when
	 * re-enabled, producing repeated output.
	 * (vendor SDK: SEC_Eng_Turn_On_Sec_Ring)
	 */
	regval = sys_read32(cfg->base + SEC_ENG_SE_TRNG_0_CTRL_3_OFFSET);
	regval |= SEC_ENG_SE_TRNG_0_ROSC_EN;
	sys_write32(regval, cfg->base + SEC_ENG_SE_TRNG_0_CTRL_3_OFFSET);

	return 0;
}

static DEVICE_API(entropy, entropy_bflb_api) = {
	.get_entropy = entropy_bflb_get_entropy,
	.get_entropy_isr = entropy_bflb_get_entropy_isr,
};

#define ENTROPY_BFLB_INIT(inst)                                                                    \
	static struct entropy_bflb_data entropy_bflb_data_##inst;                                  \
	static const struct entropy_bflb_config entropy_bflb_config_##inst = {                     \
		.base = DT_INST_REG_ADDR(inst),                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, entropy_bflb_init, NULL, &entropy_bflb_data_##inst,            \
			      &entropy_bflb_config_##inst, PRE_KERNEL_1,                           \
			      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_bflb_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_BFLB_INIT)
