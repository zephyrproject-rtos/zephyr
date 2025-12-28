/*
 * Copyright (c) 2025 Aleksandr Senin <al@meshium.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_trng

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>

/* GD32 HAL headers */
#include <gd32f4xx_rcu.h>
#include <gd32f4xx_trng.h>

LOG_MODULE_REGISTER(entropy_gd32, CONFIG_ENTROPY_LOG_LEVEL);

/* Prevent infinite wait in case TRNG never asserts DRDY */
#define GD32_TRNG_DRDY_TIMEOUT_MS 100U

struct entropy_gd32_config {
	uint16_t clkid;
};

static inline void entropy_gd32_clear_int_flags(void)
{
	trng_interrupt_flag_clear(TRNG_INT_FLAG_CEIF);
	trng_interrupt_flag_clear(TRNG_INT_FLAG_SEIF);
}

static bool entropy_gd32_ck48m_ready(void)
{
	/*
	 * CK48M domain is used by TRNG (and also SDIO/USBFS/USBHS).
	 *
	 * - If CK48MSEL=1 => CK48M = IRC48M, require IRC48MSTB.
	 * - If CK48MSEL=0 => CK48M = PLL48M (PLLQ or PLLSAIP), require the
	 *   selected PLL block to be stable. This does not guarantee 48MHz, but
	 *   indicates the clock path is at least running.
	 */
	if ((RCU_ADDCTL & RCU_ADDCTL_CK48MSEL) != 0U) {
		return (RCU_ADDCTL & RCU_ADDCTL_IRC48MSTB) != 0U;
	}

	if ((RCU_ADDCTL & RCU_ADDCTL_PLL48MSEL) != 0U) {
		/* PLLSAIP selected */
		return (RCU_CTL & RCU_CTL_PLLSAISTB) != 0U;
	}

	/* PLLQ selected */
	return (RCU_CTL & RCU_CTL_PLLSTB) != 0U;
}

static entropy_gd32_recover(void)
{
	/*
	 * For GD32F4xx TRNG the HAL exposes status bits (CECS/SECS) but provides
	 * only interrupt-flag clearing (CEIF/SEIF). To reliably recover from seed
	 * errors, do a peripheral reset (trng_deinit()) and re-enable.
	 */
	trng_disable();
	trng_deinit();
	entropy_gd32_clear_int_flags();
	trng_enable();

	return 0;
}

static int entropy_gd32_wait_drdy(void)
{
	uint32_t start = k_cycle_get_32();
	uint32_t timeout_cycles = (uint32_t)((uint64_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC *
					     (uint64_t)GD32_TRNG_DRDY_TIMEOUT_MS / 1000ULL);

	while (SET != trng_flag_get(TRNG_FLAG_DRDY)) {
		if (SET == trng_flag_get(TRNG_FLAG_CECS)) {
			/* Clock error: indicates a misconfigured/too-slow TRNG clock */
			return -EIO;
		}

		if (SET == trng_flag_get(TRNG_FLAG_SECS)) {
			entropy_gd32_recover();
		}

		if ((k_cycle_get_32() - start) > timeout_cycles) {
			return -ETIMEDOUT;
		}

		/* Never yield/sleep in ISR paths */
		if (!k_is_in_isr() && !k_is_pre_kernel()) {
			k_yield();
		}
	}

	return 0;
}

static int entropy_gd32_init(const struct device *dev)
{
	const struct entropy_gd32_config *cfg = dev->config;

	/* Ensure bus clock gate is enabled via Zephyr clock controller */
	int ret = clock_control_on(GD32_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);

	if (ret < 0) {
		return ret;
	}

	rcu_periph_clock_enable(RCU_TRNG);
	trng_deinit();
	entropy_gd32_clear_int_flags();
	trng_enable();

	if (!entropy_gd32_ck48m_ready()) {
		LOG_ERR("CK48M is not configured/running; configure gd,ck48m-source in DT "
			"(gd,gd32-rcu) for TRNG");
		return -EIO;
	}

	/*
	 * If CECS is set here, TRNG domain clock is misconfigured and random data
	 * may never become ready. We don't hard-fail init, but runtime calls will
	 * report -EIO.
	 */

	return 0;
}

static int entropy_gd32_fetch_busywait(uint8_t *dst, uint16_t len)
{
	while (len > 0U) {
		int ret = entropy_gd32_wait_drdy();

		if (ret < 0) {
			return ret;
		}

		uint32_t word = trng_get_true_random_data();

		if (len >= sizeof(word)) {
			memcpy(dst, &word, sizeof(word));
			dst += sizeof(word);
			len -= sizeof(word);
		} else {
			memcpy(dst, &word, len);
			len = 0U;
		}
	}

	return 0;
}

static int entropy_gd32_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);
	if ((buffer == NULL) || (length == 0U)) {
		return -EINVAL;
	}

	return entropy_gd32_fetch_busywait(buffer, length);
}

static int entropy_gd32_get_entropy_isr(const struct device *dev, uint8_t *buffer, uint16_t length,
					uint32_t flags)
{
	ARG_UNUSED(dev);

	if ((buffer == NULL) || (length == 0U)) {
		return -EINVAL;
	}

	if (SET == trng_flag_get(TRNG_FLAG_CECS)) {
		return -EIO;
	}
	if (SET == trng_flag_get(TRNG_FLAG_SECS)) {
		entropy_gd32_recover();
		return -EIO;
	}

	if ((flags & ENTROPY_BUSYWAIT) == 0U) {
		uint16_t written = 0U;

		while ((written < length) && (SET == trng_flag_get(TRNG_FLAG_DRDY))) {
			uint32_t word = trng_get_true_random_data();
			uint16_t chunk = MIN((uint16_t)sizeof(word), (uint16_t)(length - written));

			memcpy(&buffer[written], &word, chunk);
			written += chunk;
		}

		return written;
	}

	/* Busy-wait (ISR-safe): fill the whole buffer, return bytes written */
	int ret = entropy_gd32_fetch_busywait(buffer, length);

	if (ret < 0) {
		return ret;
	}

	return length;
}

static DEVICE_API(entropy, entropy_gd32_api) = {
	.get_entropy = entropy_gd32_get_entropy,
	.get_entropy_isr = entropy_gd32_get_entropy_isr,
};

static const struct entropy_gd32_config entropy_gd32_cfg = {
	.clkid = DT_INST_CLOCKS_CELL(0, id),
};

DEVICE_DT_INST_DEFINE(0, entropy_gd32_init, NULL, NULL, &entropy_gd32_cfg, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_gd32_api);
