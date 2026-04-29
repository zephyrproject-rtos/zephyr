/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mecc

#include <stddef.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/barrier.h>

#include <zephyr/drivers/edac.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include "fsl_mecc.h"

#if defined(CONFIG_CACHE_MANAGEMENT)
#include <zephyr/cache.h>
#include "fsl_cache.h"
#endif

LOG_MODULE_REGISTER(edac_nxp_mecc, CONFIG_EDAC_LOG_LEVEL);

#define MECC_BANK_COUNT             4
#define MECC_DEFAULT_ERROR_BIT_POS  2

/*
 * OCRAM Bank address mapping:
 *   Bank0: ocram_base_address + 0x20*i
 *   Bank1: ocram_base_address + 0x20*i + 0x8
 *   Bank2: ocram_base_address + 0x20*i + 0x10
 *   Bank3: ocram_base_address + 0x20*i + 0x18
 */

struct edac_nxp_mecc_config {
	MECC_Type *base;
	uint32_t start_addr;
	uint32_t end_addr;
	void (*irq_config_func)(const struct device *dev);
};

struct edac_nxp_mecc_data {
	edac_notify_callback_f notify_cb;
	void *notify_data;
	uint32_t correctable_errors;
	uint32_t uncorrectable_errors;

#if defined(CONFIG_EDAC_ERROR_INJECT)
	uint64_t correct_value;
	uint64_t inject_addr;
	uint32_t error_bit_pos;
	uint32_t inject_error_type;
	bool injection_enabled;
#endif
};

#if defined(CONFIG_EDAC_ERROR_INJECT)
static uint8_t get_bank_from_address(uint32_t base_addr, uint32_t addr)
{
	uint32_t offset = addr - base_addr;

	return (offset >> 3) & 0x3;
}
#endif

static uint32_t get_single_error_status_flag(uint8_t bank)
{
	const uint32_t flags[] = {
		kMECC_SingleError0InterruptFlag,
		kMECC_SingleError1InterruptFlag,
		kMECC_SingleError2InterruptFlag,
		kMECC_SingleError3InterruptFlag
	};

	return (bank < MECC_BANK_COUNT) ? flags[bank] : 0;
}

static uint32_t get_multi_error_status_flag(uint8_t bank)
{
	const uint32_t flags[] = {
		kMECC_MultiError0InterruptFlag,
		kMECC_MultiError1InterruptFlag,
		kMECC_MultiError2InterruptFlag,
		kMECC_MultiError3InterruptFlag
	};

	return (bank < MECC_BANK_COUNT) ? flags[bank] : 0;
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static int edac_nxp_mecc_inject_set_param1(const struct device *dev, uint64_t value)
{
	const struct edac_nxp_mecc_config *cfg = dev->config;
	struct edac_nxp_mecc_data *data = dev->data;
	uint32_t addr = (uint32_t)(value & 0xFFFFFFFFULL);

	if (addr < cfg->start_addr || addr >= cfg->end_addr) {
		LOG_ERR("Address 0x%08x out of MECC range [0x%08x - 0x%08x]",
			addr, cfg->start_addr, cfg->end_addr);
		return -EINVAL;
	}

	if (addr & 0x7) {
		LOG_WRN("Address 0x%08x not 8-byte aligned, aligning to 0x%08x",
			addr, addr & ~0x7);
		addr &= ~0x7;
	}

	data->inject_addr = addr;

	return 0;
}

static int edac_nxp_mecc_inject_get_param1(const struct device *dev, uint64_t *value)
{
	struct edac_nxp_mecc_data *data = dev->data;

	if (value == NULL) {
		return -EINVAL;
	}

	*value = data->inject_addr;

	return 0;
}

static int edac_nxp_mecc_inject_set_param2(const struct device *dev, uint64_t value)
{
	struct edac_nxp_mecc_data *data = dev->data;
	uint32_t bit_pos = (uint32_t)(value & 0xFFFFFFFFULL);

	if (bit_pos > 31) {
		LOG_ERR("Invalid bit position %d, must be 0-31", bit_pos);
		return -EINVAL;
	}

	data->error_bit_pos = bit_pos;

	return 0;
}

static int edac_nxp_mecc_inject_get_param2(const struct device *dev, uint64_t *value)
{
	struct edac_nxp_mecc_data *data = dev->data;

	if (value == NULL) {
		return -EINVAL;
	}

	*value = data->error_bit_pos;

	return 0;
}

static int edac_nxp_mecc_inject_set_error_type(const struct device *dev, uint32_t error_type)
{
	struct edac_nxp_mecc_data *data = dev->data;

	if (error_type != EDAC_ERROR_TYPE_DRAM_COR &&
	    error_type != EDAC_ERROR_TYPE_DRAM_UC) {
		LOG_ERR("Invalid error type %d", error_type);
		return -EINVAL;
	}

	data->inject_error_type = error_type;

	return 0;
}

static int edac_nxp_mecc_inject_get_error_type(const struct device *dev, uint32_t *error_type)
{
	struct edac_nxp_mecc_data *data = dev->data;

	if (error_type == NULL) {
		return -EINVAL;
	}

	*error_type = data->inject_error_type;

	return 0;
}

static int edac_nxp_mecc_inject_error_trigger(const struct device *dev)
{
	const struct edac_nxp_mecc_config *cfg = dev->config;
	struct edac_nxp_mecc_data *data = dev->data;
	status_t status;
	uint32_t low_error_data, high_error_data;
	uint8_t ecc_data = 0;
	uint8_t target_bank;
	uint32_t addr;
	uint32_t bit_pos;
	uint32_t int_status;

	addr = (uint32_t)data->inject_addr;
	bit_pos = data->error_bit_pos;

	/* Save the current value at the target address for restoration in ISR */
	data->correct_value = *((volatile uint64_t *)addr);

	target_bank = get_bank_from_address(cfg->start_addr, addr);

	/*
	 * Set error injection pattern based on error type and bit position
	 *
	 * Single error (correctable):
	 *   low_error_data  = 1 << bit_pos  (flip 1 bit in low 32-bit)
	 *   high_error_data = 0             (no error in high 32-bit)
	 *   Total: 1 bit error -> correctable
	 *
	 * Multi error (uncorrectable):
	 *   low_error_data  = 1 << bit_pos  (flip 1 bit in low 32-bit)
	 *   high_error_data = 1 << bit_pos  (flip 1 bit in high 32-bit)
	 *   Total: 2 bit errors -> uncorrectable
	 */
	if (data->inject_error_type == EDAC_ERROR_TYPE_DRAM_COR) {
		low_error_data = BIT(bit_pos);
		high_error_data = 0;
	} else {
		low_error_data = BIT(bit_pos);
		high_error_data = BIT(bit_pos);
	}

	int_status = MECC_GetStatusFlags(cfg->base);
	if (int_status != 0) {
		MECC_ClearStatusFlags(cfg->base, int_status);
	}

	MECC_ErrorInjection(cfg->base, 0, 0, 0, target_bank);

	status = MECC_ErrorInjection(cfg->base, low_error_data, high_error_data,
				     ecc_data, target_bank);
	if (status != kStatus_Success) {
		LOG_ERR("MECC_ErrorInjection failed: %d", status);
		return -EIO;
	}

	data->injection_enabled = true;

#if defined(CONFIG_CACHE_MANAGEMENT)
	sys_cache_data_flush_and_invd_range((void *)addr, 32);
#endif

	return 0;
}

#endif /* CONFIG_EDAC_ERROR_INJECT */

static int edac_nxp_mecc_ecc_error_log_get(const struct device *dev, uint64_t *value)
{
	const struct edac_nxp_mecc_config *cfg = dev->config;

	if (value == NULL) {
		return -EINVAL;
	}

	*value = MECC_GetStatusFlags(cfg->base);

	return 0;
}

static int edac_nxp_mecc_ecc_error_log_clear(const struct device *dev)
{
	const struct edac_nxp_mecc_config *cfg = dev->config;
	struct edac_nxp_mecc_data *data = dev->data;

	MECC_ClearStatusFlags(cfg->base, kMECC_AllInterruptsFlag);

	/* Clear error injection registers for all banks */
	for (uint8_t bank = 0; bank < MECC_BANK_COUNT; bank++) {
		MECC_ErrorInjection(cfg->base, 0, 0, 0, bank);
	}

	data->correctable_errors = 0;
	data->uncorrectable_errors = 0;
#if defined(CONFIG_EDAC_ERROR_INJECT)
	data->injection_enabled = false;
#endif

	return 0;
}

static int edac_nxp_mecc_errors_cor_get(const struct device *dev)
{
	struct edac_nxp_mecc_data *data = dev->data;

	return data->correctable_errors;
}

static int edac_nxp_mecc_errors_uc_get(const struct device *dev)
{
	struct edac_nxp_mecc_data *data = dev->data;

	return data->uncorrectable_errors;
}

static int edac_nxp_mecc_notify_cb_set(const struct device *dev, edac_notify_callback_f cb)
{
	struct edac_nxp_mecc_data *data = dev->data;

	data->notify_cb = cb;

	return 0;
}

static void edac_nxp_mecc_isr(const struct device *dev)
{
	const struct edac_nxp_mecc_config *cfg = dev->config;
	struct edac_nxp_mecc_data *data = dev->data;
	uint32_t int_status;
	bool error_detected = false;

	int_status = MECC_GetStatusFlags(cfg->base);

	LOG_DBG("MECC ISR: status=0x%08x", int_status);

	for (int bank = 0; bank < MECC_BANK_COUNT; bank++) {
		if (int_status & get_single_error_status_flag(bank)) {
			data->correctable_errors++;
			error_detected = true;
		}

		if (int_status & get_multi_error_status_flag(bank)) {
			data->uncorrectable_errors++;
			error_detected = true;
		}
	}

	MECC_ClearStatusFlags(cfg->base, int_status);

	/*
	 * Only restore memory and clear injection registers when injection was
	 * explicitly enabled.
	 */
#if defined(CONFIG_EDAC_ERROR_INJECT)
	if (data->injection_enabled) {
		MECC_ErrorInjection(cfg->base, 0, 0, 0,
				get_bank_from_address(cfg->start_addr,
					(uint32_t)data->inject_addr));

		*(volatile uint64_t *)(uintptr_t)data->inject_addr = data->correct_value;
		barrier_dsync_fence_full();
		data->injection_enabled = false;
	}
#endif /* CONFIG_EDAC_ERROR_INJECT */

	if (error_detected && (data->notify_cb != NULL)) {
		data->notify_cb(dev, data->notify_data);
	}
}

static const struct edac_driver_api edac_nxp_mecc_api = {
#if defined(CONFIG_EDAC_ERROR_INJECT)
	.inject_set_param1 = edac_nxp_mecc_inject_set_param1,
	.inject_get_param1 = edac_nxp_mecc_inject_get_param1,
	.inject_set_param2 = edac_nxp_mecc_inject_set_param2,
	.inject_get_param2 = edac_nxp_mecc_inject_get_param2,
	.inject_set_error_type = edac_nxp_mecc_inject_set_error_type,
	.inject_get_error_type = edac_nxp_mecc_inject_get_error_type,
	.inject_error_trigger = edac_nxp_mecc_inject_error_trigger,
#endif
	.ecc_error_log_get = edac_nxp_mecc_ecc_error_log_get,
	.ecc_error_log_clear = edac_nxp_mecc_ecc_error_log_clear,
	.errors_cor_get = edac_nxp_mecc_errors_cor_get,
	.errors_uc_get = edac_nxp_mecc_errors_uc_get,
	.notify_cb_set = edac_nxp_mecc_notify_cb_set,
};

static int edac_nxp_mecc_init(const struct device *dev)
{
	const struct edac_nxp_mecc_config *cfg = dev->config;
	struct edac_nxp_mecc_data *data = dev->data;
	mecc_config_t mecc_config;

	data->correctable_errors = 0;
	data->uncorrectable_errors = 0;
	data->notify_cb = NULL;
	data->notify_data = NULL;

#if defined(CONFIG_EDAC_ERROR_INJECT)
	data->correct_value = 0;
	data->inject_addr = 0;
	data->error_bit_pos = MECC_DEFAULT_ERROR_BIT_POS;
	data->inject_error_type = EDAC_ERROR_TYPE_DRAM_COR;
	data->injection_enabled = false;
#endif

	MECC_GetDefaultConfig(&mecc_config);
	mecc_config.enableMecc = true;
	mecc_config.startAddress = cfg->start_addr;
	mecc_config.endAddress = cfg->end_addr;

	MECC_Init(cfg->base, &mecc_config);

	MECC_EnableInterrupts(cfg->base,
			      kMECC_SingleError0InterruptEnable |
			      kMECC_SingleError1InterruptEnable |
			      kMECC_SingleError2InterruptEnable |
			      kMECC_SingleError3InterruptEnable |
			      kMECC_MultiError0InterruptEnable |
			      kMECC_MultiError1InterruptEnable |
			      kMECC_MultiError2InterruptEnable |
			      kMECC_MultiError3InterruptEnable);

	cfg->irq_config_func(dev);

	LOG_INF("MECC EDAC ready: range=0x%08x-0x%08x", cfg->start_addr, cfg->end_addr);

	return 0;
}

#define EDAC_NXP_MECC_IRQ_CONFIG(n)						\
	static void edac_nxp_mecc_irq_config_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    edac_nxp_mecc_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));					\
	}

#define EDAC_NXP_MECC_DEVICE(n)							\
	EDAC_NXP_MECC_IRQ_CONFIG(n)						\
										\
	static const struct edac_nxp_mecc_config edac_nxp_mecc_config_##n = {	\
		.base = (MECC_Type *)DT_INST_REG_ADDR(n),			\
		.start_addr = DT_INST_PROP(n, memory_start_address),		\
		.end_addr = DT_INST_PROP(n, memory_end_address),		\
		.irq_config_func = edac_nxp_mecc_irq_config_##n,		\
	};									\
										\
	static struct edac_nxp_mecc_data edac_nxp_mecc_data_##n;		\
										\
	DEVICE_DT_INST_DEFINE(n, edac_nxp_mecc_init, NULL,			\
			      &edac_nxp_mecc_data_##n,				\
			      &edac_nxp_mecc_config_##n,			\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &edac_nxp_mecc_api);

DT_INST_FOREACH_STATUS_OKAY(EDAC_NXP_MECC_DEVICE)
