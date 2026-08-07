/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/edac.h>
#include <zephyr/drivers/edac/edac_mcux_erm.h>
#include <zephyr/logging/log.h>
#include <fsl_erm.h>

LOG_MODULE_REGISTER(edac_mcux_erm, CONFIG_EDAC_LOG_LEVEL);

#ifdef CONFIG_EDAC_NXP_ERROR_INJECT
#include <fsl_eim.h>

#define EDAC_NXP_SINGLE_BIT_ERROR_MASK 0x1
#define EDAC_NXP_DOUBLE_BIT_ERROR_MASK 0x3

#define EIM_CHANNEL_ENABLE(CHANNEL_ID) (0x80000000U >> (CHANNEL_ID))

struct edac_nxp_eim_channel {
	uint32_t start_address;
	uint32_t size;
	uint32_t ecc_enable;
	uint8_t channel_id;
	uint8_t erm_channel_id;
};
#endif /* CONFIG_EDAC_NXP_ERROR_INJECT */

struct edac_nxp_config {
	ERM_Type *erm_base;
#ifdef CONFIG_EDAC_NXP_ERROR_INJECT
	EIM_Type *eim_base;
	const struct edac_nxp_eim_channel *eim_channels;
	uint8_t eim_channel_num;
#endif /* CONFIG_EDAC_NXP_ERROR_INJECT */
	const int *erm_channels;
	uint8_t erm_channel_num;
	void (*irq_config_func)(const struct device *dev);
};

struct edac_nxp_data {
	edac_notify_callback_f cb;
#ifdef CONFIG_EDAC_NXP_ERROR_INJECT
	uint32_t eim_channel;
	uint32_t eim_channel_word;
	uint32_t inject_error_type;
#endif /* CONFIG_EDAC_NXP_ERROR_INJECT */
	uint32_t erm_channel;
};

__weak void enable_ecc(uint32_t mask)
{
	ARG_UNUSED(mask);
}

static bool check_erm_channel(const int *erm_channels, size_t size, uint32_t value)
{
	for (size_t i = 0; i < size; i++) {
		if (erm_channels[i] == value) {
			return true;
		}
	}
	return false;
}

#ifdef CONFIG_EDAC_NXP_ERROR_INJECT
static bool check_eim_channel(const struct edac_nxp_eim_channel *eim_channels, size_t size,
			      uint32_t value)
{
	for (size_t i = 0; i < size; i++) {
		if (eim_channels[i].channel_id == value) {
			return true;
		}
	}
	return false;
}

static inline const struct edac_nxp_eim_channel *
get_eim_channel(const struct edac_nxp_eim_channel *eim_channels, size_t size, uint32_t value)
{
	for (size_t i = 0; i < size; i++) {
		if (eim_channels[i].channel_id == value) {
			return &eim_channels[i];
		}
	}
	return NULL;
}

static int inject_set_param1(const struct device *dev, uint64_t channel)
{
	struct edac_nxp_data *data = dev->data;
	const struct edac_nxp_config *config = dev->config;

	if (!check_eim_channel(config->eim_channels, config->eim_channel_num, (uint32_t)channel)) {
		LOG_ERR("Invalid EIM channel %llx", channel);
		return -EINVAL;
	}

	data->eim_channel = (uint32_t)(channel & 0xFFFFFFFFU);
	return 0;
}

static int inject_get_param1(const struct device *dev, uint64_t *value)
{
	struct edac_nxp_data *data = dev->data;

	*value = (uint64_t)data->eim_channel;
	return 0;
}

static int inject_set_param2(const struct device *dev, uint64_t word)
{
	struct edac_nxp_data *data = dev->data;

	data->eim_channel_word = (uint32_t)(word & 0xFU);
	return 0;
}

static int inject_get_param2(const struct device *dev, uint64_t *word)
{
	struct edac_nxp_data *data = dev->data;

	*word = (uint64_t)data->eim_channel_word;
	return 0;
}

static int inject_set_error_type(const struct device *dev, uint32_t inject_error_type)
{
	struct edac_nxp_data *data = dev->data;

	data->inject_error_type = inject_error_type;
	return 0;
}

static int inject_get_error_type(const struct device *dev, uint32_t *inject_error_type)
{
	struct edac_nxp_data *data = dev->data;

	*inject_error_type = data->inject_error_type;
	return 0;
}

static int inject_error_trigger(const struct device *dev)
{
	const struct edac_nxp_config *config = dev->config;
	struct edac_nxp_data *data = dev->data;
	uint32_t inject_data;
	const struct edac_nxp_eim_channel *eim_channel_data =
		get_eim_channel(config->eim_channels, config->eim_channel_num, data->eim_channel);

	switch (data->inject_error_type) {
	case EDAC_ERROR_TYPE_DRAM_COR:
		inject_data = EDAC_NXP_SINGLE_BIT_ERROR_MASK;
		break;
	case EDAC_ERROR_TYPE_DRAM_UC:
		inject_data = EDAC_NXP_DOUBLE_BIT_ERROR_MASK;
		break;
	default:
		LOG_ERR("No error type found.");
		return -EINVAL;
	}

#if defined(CONFIG_EDAC_NXP_ERM_VARY_WITH_EIM_CHANNEL) && CONFIG_EDAC_NXP_ERM_VARY_WITH_EIM_CHANNEL
	if (!check_erm_channel(config->erm_channels, config->erm_channel_num,
			       eim_channel_data->erm_channel_id)) {
		LOG_WRN("Invalid ERM channel %d", eim_channel_data->erm_channel_id);
	} else {
		LOG_DBG("Setting ERM channel %d for error reporting",
			eim_channel_data->erm_channel_id);
		data->erm_channel = eim_channel_data->erm_channel_id;
	}
#endif

	if (eim_channel_data->ecc_enable) {
		enable_ecc(eim_channel_data->ecc_enable);
	}

	if (data->eim_channel_word == 0U) {
		EIM_InjectCheckBitError(config->eim_base, data->eim_channel, inject_data);
	} else {
		EIM_InjectDataWordBitError(config->eim_base, data->eim_channel, inject_data,
					   data->eim_channel_word);
	}

	EIM_EnableErrorInjectionChannels(config->eim_base, EIM_CHANNEL_ENABLE(data->eim_channel));
	ERM_EnableInterrupts(config->erm_base, data->erm_channel, kERM_AllInterruptsEnable);
	LOG_INF("EIM channel %d, range 0x%x - 0x%x ECC error injection triggered.",
		data->eim_channel, eim_channel_data->start_address,
		eim_channel_data->start_address + eim_channel_data->size - 1);
	return 0;
}
#endif /* CONFIG_EDAC_NXP_ERROR_INJECT */

static int errors_cor_get(const struct device *dev)
{
#if defined(ERM_CORR_ERR_CNT0_COUNT_MASK) && ERM_CORR_ERR_CNT0_COUNT_MASK
	const struct edac_nxp_config *config = dev->config;
	struct edac_nxp_data *data = dev->data;

	return (int)ERM_GetErrorCount(config->erm_base, data->erm_channel);
#else
	return -ENOSYS;
#endif
}

static int notify_callback_set(const struct device *dev, edac_notify_callback_f cb)
{
	struct edac_nxp_data *data = dev->data;
	unsigned int key = irq_lock();

	data->cb = cb;
	irq_unlock(key);
	return 0;
}

static void edac_nxp_isr(const struct device *dev)
{
	const struct edac_nxp_config *config = dev->config;
	struct edac_nxp_data *data = dev->data;
	uint32_t status = ERM_GetInterruptStatus(config->erm_base, data->erm_channel);
#if defined(ERM_SYN0_SYNDROME_MASK) && ERM_SYN0_SYNDROME_MASK
	uint32_t syndrome = ERM_GetSyndrome(config->erm_base, data->erm_channel);
#else
	uint32_t syndrome = -ENOSYS;
#endif
	struct edac_nxp_callback_data cb_data = {
		.corr_err_count = errors_cor_get(dev),
		.err_syndrome = syndrome,
		.err_addr = ERM_GetMemoryErrorAddr(config->erm_base, data->erm_channel),
		.err_status = status,
	};

	if (kERM_SingleBitCorrectionIntFlag == (status & kERM_SingleBitCorrectionIntFlag)) {
		LOG_ERR("ERM channel %d correctable ECC error detected, address/offset 0x%x, "
			"syndrome "
			"0x%02x, correctable ECC count %d",
			data->erm_channel, cb_data.err_addr, cb_data.err_syndrome,
			cb_data.corr_err_count);
		ERM_ClearInterruptStatus(config->erm_base, data->erm_channel,
					 kERM_SingleBitCorrectionIntFlag);
	} else if (kERM_NonCorrectableErrorIntFlag == (status & kERM_NonCorrectableErrorIntFlag)) {
		LOG_ERR("ERM channel %d uncorrectable ECC error detected, address/offset 0x%x",
			data->erm_channel, cb_data.err_addr);
		ERM_ClearInterruptStatus(config->erm_base, data->erm_channel,
					 kERM_NonCorrectableErrorIntFlag);
	} else {
		LOG_ERR("ERM unknown ECC error status detected, it may caused by unaligned ERM "
			"channel");
		ERM_ClearInterruptStatus(config->erm_base, data->erm_channel, kERM_AllIntsFlag);
	}
	if (data->cb) {
		data->cb(dev, &cb_data);
	}
}

static DEVICE_API(edac, edac_nxp_api) = {
#ifdef CONFIG_EDAC_NXP_ERROR_INJECT
	/* Error Injection functions */
	.inject_set_param1 = inject_set_param1,
	.inject_get_param1 = inject_get_param1,
	.inject_set_param2 = inject_set_param2,
	.inject_get_param2 = inject_get_param2,
	.inject_set_error_type = inject_set_error_type,
	.inject_get_error_type = inject_get_error_type,
	.inject_error_trigger = inject_error_trigger,
#endif /* CONFIG_EDAC_NXP_ERROR_INJECT */

	/* Get error stats */
	.errors_cor_get = errors_cor_get,

	/* Notification callback set */
	.notify_cb_set = notify_callback_set,
};

static int edac_nxp_init(const struct device *dev)
{
	const struct edac_nxp_config *config = dev->config;
	struct edac_nxp_data *data = dev->data;

#ifdef CONFIG_EDAC_NXP_ERROR_INJECT
	EIM_Init(config->eim_base);
	EIM_EnableGlobalErrorInjection(config->eim_base, true);
	data->eim_channel_word = 1U;
	LOG_INF("EIM driver initialized");
#endif /* CONFIG_EDAC_NXP_ERROR_INJECT */

	ERM_Init(config->erm_base);
	if (!check_erm_channel(config->erm_channels, config->erm_channel_num,
			       CONFIG_EDAC_NXP_ERM_DEFAULT_CHANNEL)) {
		LOG_ERR("Invalid ERM channel %d", CONFIG_EDAC_NXP_ERM_DEFAULT_CHANNEL);
		return -EINVAL;
	}
	data->erm_channel = CONFIG_EDAC_NXP_ERM_DEFAULT_CHANNEL;

	/* Clear any latched status before enabling interrupts */
	ERM_ClearInterruptStatus(config->erm_base, data->erm_channel, kERM_AllIntsFlag);
	config->irq_config_func(dev);
	ERM_EnableInterrupts(config->erm_base, data->erm_channel, kERM_AllInterruptsEnable);
	LOG_INF("ERM driver initialized");

	return 0;
}

#define DT_DRV_COMPAT nxp_erm

#ifdef CONFIG_EDAC_NXP_ERROR_INJECT
/* Initializes an element of the eim channel device pointer array */
#define NXP_EIM_CHANNEL_DEV_ARRAY_INIT(node)                                                       \
	{                                                                                          \
		.channel_id = DT_PROP(node, channel_id),                                           \
		.erm_channel_id = DT_PROP_OR(node, erm_channel_id, 0xFFU),                         \
		.start_address = DT_PROP(node, start_address),                                     \
		.ecc_enable = DT_PROP_OR(node, ecc_enable, 0),                                     \
		.size = DT_PROP(node, size),                                                       \
	},
const struct edac_nxp_eim_channel edac_nxp_eim_0_channels[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(eim0), NXP_EIM_CHANNEL_DEV_ARRAY_INIT)};

#define EDAC_NXP_EIM_CONFIG_FIELDS                                                                 \
	.eim_base = (EIM_Type *)DT_REG_ADDR(DT_NODELABEL(eim0)),                                   \
	.eim_channels = edac_nxp_eim_0_channels,                                                   \
	.eim_channel_num = ARRAY_SIZE(edac_nxp_eim_0_channels),
#else
#define EDAC_NXP_EIM_CHANNELS
#define EDAC_NXP_EIM_CONFIG_FIELDS
#endif

static const int edac_nxp_erm_0_channels[] = DT_INST_PROP(0, channels);

static void edac_nxp_irq_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 0, irq), DT_INST_IRQ_BY_IDX(0, 0, priority), edac_nxp_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 0, irq));
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 1, irq), DT_INST_IRQ_BY_IDX(0, 1, priority), edac_nxp_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 1, irq));
}

static const struct edac_nxp_config edac_nxp_config_0 = {
	.erm_base = (ERM_Type *)DT_INST_REG_ADDR(0),
	EDAC_NXP_EIM_CONFIG_FIELDS.erm_channels = edac_nxp_erm_0_channels,
	.erm_channel_num = ARRAY_SIZE(edac_nxp_erm_0_channels),
	.irq_config_func = edac_nxp_irq_0,
};

static struct edac_nxp_data edac_nxp_data_0;

DEVICE_DT_INST_DEFINE(0, &edac_nxp_init, NULL, &edac_nxp_data_0, &edac_nxp_config_0, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &edac_nxp_api);
