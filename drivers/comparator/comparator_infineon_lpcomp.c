/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Low Power Comparator (LPComp)
 * driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_lp_comp

#include <infineon_kconfig.h>
#include <cy_lpcomp.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/device_mmio.h>

LOG_MODULE_REGISTER(infineon_lp_comp, CONFIG_COMPARATOR_LOG_LEVEL);

/**
 * @brief Low Power Comparator (LPComp) Channel Configuration Structure
 *
 * Basic configuration for the LPComp channel.
 */
struct ifx_lpcomp_channel_config {
	DEVICE_MMIO_ROM;
	/* Channel number, note: 0-based index but used as 1-based in PDL functions */
	uint8_t channel;
	/* Output mode */
	uint8_t output_mode;
	/* Enable/disable Hysteresis */
	bool hysteresis;
	/* Power mode */
	uint8_t power;
	/* Interrupt type */
	uint8_t intr_type;
	/* Input configuration */
	uint8_t input_pos;
	uint8_t input_neg;
};

/**
 * @brief Low Power Comparator (LPComp) device data
 */
struct ifx_lpcomp_data {
	DEVICE_MMIO_RAM;
	/* LPComp context */
	cy_stc_lpcomp_context_t lpcomp_context;
	/* callback */
	comparator_callback_t callback;
	/* User data for callback */
	void *user_data;
	/* Atomic flag for pending trigger */
	atomic_t pending;
};

/*
 * Holds the number of enabled LPComp channels and an array of pointers to
 * the corresponding device structures, indexed by channel number.
 * Disabled channels will have NULL entries in the ch_dev array.
 */
struct ifx_lpcomp_instance {
	size_t num_channels;
	const struct device *ch_dev[];
};

static int lpcomp_init_inputs(const struct device *dev)
{
	const struct ifx_lpcomp_channel_config *cfg = dev->config;
	LPCOMP_Type *base = (LPCOMP_Type *)DEVICE_MMIO_GET(dev);

	/* Enables the local reference generator circuit to generate the local Vref and Ibias. */
	Cy_LPComp_UlpReferenceEnable(base);

	if (cfg->input_pos == CY_LPCOMP_SW_LOCAL_VREF) {
		LOG_ERR("Local Vref cannot be connected to positive input");
		return -EINVAL;
	}
	/* Connect the local reference if enabled */
	if (cfg->input_neg == CY_LPCOMP_SW_LOCAL_VREF) {
		/* Connect the local reference to the output of the comparator negative input */
		Cy_LPComp_ConnectULPReference(base, (cfg->channel + 1));
	}

	/* Set LPComp inputs */
	if (cfg->channel == 0) {
		LPCOMP_CMP0_SW_CLEAR(base) = CY_LPCOMP_CMP0_SW_POS_Msk | CY_LPCOMP_CMP0_SW_NEG_Msk;
		LPCOMP_CMP0_SW(base) =
			(cfg->input_pos | (cfg->input_neg << LPCOMP_CMP0_SW_CMP0_IN0_Pos));
	} else if (cfg->channel == 1) {
		LPCOMP_CMP1_SW_CLEAR(base) = CY_LPCOMP_CMP1_SW_POS_Msk | CY_LPCOMP_CMP1_SW_NEG_Msk;
		LPCOMP_CMP1_SW(base) =
			(cfg->input_pos | (cfg->input_neg << LPCOMP_CMP1_SW_CMP1_IN1_Pos));
	} else {
		return -EINVAL;
	}
	return 0;
}

/*
 * Zephyr Driver API Functions
 */

/**
 * @brief Get LPComp output
 */
static int ifx_lpcomp_get_output(const struct device *dev)
{
	const struct ifx_lpcomp_channel_config *config = dev->config;
	LPCOMP_Type *base = (LPCOMP_Type *)DEVICE_MMIO_GET(dev);

	return (int)Cy_LPComp_GetCompare(base, (config->channel + 1));
}

/**
 * @brief Set LPComp trigger
 */
static int ifx_lpcomp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct ifx_lpcomp_channel_config *config = dev->config;
	struct ifx_lpcomp_data *data = dev->data;
	LPCOMP_Type *base = (LPCOMP_Type *)DEVICE_MMIO_GET(dev);
	cy_en_lpcomp_int_t intr_type = CY_LPCOMP_INTR_DISABLE;

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		intr_type = CY_LPCOMP_INTR_DISABLE;
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		intr_type = CY_LPCOMP_INTR_RISING;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		intr_type = CY_LPCOMP_INTR_FALLING;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		intr_type = CY_LPCOMP_INTR_BOTH;
		break;
	default:
		LOG_ERR("Invalid trigger type.");
		return -EINVAL;
	}

	/* Set interrupt trigger mode */
#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
	Cy_LPComp_SetInterruptTriggerMode_Ext(base, (config->channel + 1), intr_type,
					      &data->lpcomp_context);
#elif defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	Cy_LPComp_SetInterruptTriggerMode(base, (config->channel + 1), intr_type,
					  &data->lpcomp_context);
#else
#error "Unsupported platform. Please define the correct LPCOMP macro for the platform."
#endif
	/* Set interrupt mask for the specific channel */
	Cy_LPComp_SetInterruptMask(base,
				   (config->channel == 0) ? CY_LPCOMP_COMP0 : CY_LPCOMP_COMP1);

	return 0;
}

/**
 * @brief Check if LPComp trigger is pending
 */
static int ifx_lpcomp_trigger_is_pending(const struct device *dev)
{
	const struct ifx_lpcomp_channel_config *config = dev->config;

	if (config != NULL) {
		struct ifx_lpcomp_data *data = dev->data;

		if (data != NULL) {
			/* Check if the trigger is pending and clear it atomically */
			if (atomic_cas(&data->pending, 1, 0)) {
				return 1;
			}
		}
	}
	return 0;
}

/**
 * @brief LPComp interrupt handler
 */
static void lpcomp_irq_handler(const void *isr_param)
{
	/*
	 * When an interrupt is configured as a shared interrupt (not associated
	 * with a specific device instance), the handler receives a pointer to a
	 * struct containing all child channel pointers. Iterate over all channels
	 * using this struct to check which one triggered the interrupt.
	 */
	const struct ifx_lpcomp_instance *inst = isr_param;

	for (int idx = 0; idx < inst->num_channels; idx++) {
		if (inst->ch_dev[idx] != NULL) {
			const struct device *dev = inst->ch_dev[idx];
			struct ifx_lpcomp_data *data = dev->data;
			LPCOMP_Type *base = (LPCOMP_Type *)DEVICE_MMIO_GET(dev);

			if (data != NULL && base != NULL) {
				uint32_t status_flags = Cy_LPComp_GetInterruptStatusMasked(base);
				uint32_t mask = (idx == 0) ? CY_LPCOMP_COMP0 : CY_LPCOMP_COMP1;

				if (status_flags & mask) {
					atomic_set(&data->pending, 1);
					Cy_LPComp_ClearInterrupt(base, mask);
					if (data->callback != NULL) {
						atomic_set(&data->pending, 0);
						data->callback(dev, data->user_data);
					}
				}
			}
		}
	}
}

/**
 * @brief Set LPComp trigger callback
 */
static int ifx_lpcomp_set_trigger_callback(const struct device *dev, comparator_callback_t callback,
					   void *user_data)
{
	struct ifx_lpcomp_data *data = dev->data;

	/* Set the callback and user data */
	data->callback = callback;
	data->user_data = user_data;

	if (data->callback == NULL) {
		LOG_INF("Callback is not set.");
		return 0;
	}
	/* At this point, the callback is set and there might be a pending trigger */
	if (atomic_cas(&data->pending, 1, 0)) {
		data->callback(dev, data->user_data);
	}

	return 0;
}

/**
 * @brief Initialize LPComp device
 */
static int ifx_lpcomp_init(const struct device *dev)
{
	const struct ifx_lpcomp_channel_config *config = dev->config;
	struct ifx_lpcomp_data *data = dev->data;
	LPCOMP_Type *base;

	if (config == NULL) {
		LOG_ERR("Invalid channel configuration");
		return -EINVAL;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	base = (LPCOMP_Type *)DEVICE_MMIO_GET(dev);
	if (base == NULL) {
		LOG_ERR("Failed to map LPComp registers");
		return -EINVAL;
	}

	const cy_stc_lpcomp_config_t lpcomp_ch_config = {.outputMode = config->output_mode,
							 .hysteresis = config->hysteresis,
							 .power = config->power,
							 .intType = config->intr_type};

	/* Initialize inputs */
	if (lpcomp_init_inputs(dev) != 0) {
		return -EINVAL;
	}

	LOG_DBG("Initializing LPComp");

	/* Initialize LPComp */
#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
	cy_en_lpcomp_status_t lpcomp_status = Cy_LPComp_Init_Ext(
		base, (config->channel + 1), &lpcomp_ch_config, &data->lpcomp_context);
#elif defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	cy_en_lpcomp_status_t lpcomp_status = Cy_LPComp_Init(
		base, (config->channel + 1), &lpcomp_ch_config, &data->lpcomp_context);
#else
#error "Unsupported platform. Please define the correct LPCOMP macro for the platform."
#endif
	if (lpcomp_status != CY_LPCOMP_SUCCESS) {
		LOG_ERR("Failed to initialize LPComp");
		return -EIO;
	}

	/* Enable LPComp */
#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
	Cy_LPComp_Enable_Ext(base, (config->channel + 1), &data->lpcomp_context);
#elif defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	Cy_LPComp_Enable(base, (config->channel + 1), &data->lpcomp_context);
#else
#error "Unsupported platform. Please define the correct LPCOMP macro for the platform."
#endif

	atomic_set(&data->pending, 0);

	return 0;
}

static DEVICE_API(comparator, ifx_lpcomp_driver_api) = {
	.get_output = ifx_lpcomp_get_output,
	.set_trigger = ifx_lpcomp_set_trigger,
	.set_trigger_callback = ifx_lpcomp_set_trigger_callback,
	.trigger_is_pending = ifx_lpcomp_trigger_is_pending,
};

/* Device instantiation macro for every child (channel) of LPCOMP */
#define LPCOMP_CHANNEL_SETUP_IMPL(child)                                                           \
	static struct ifx_lpcomp_data ifx_lpcomp_data_##child;                                     \
	static const struct ifx_lpcomp_channel_config ifx_lpcomp_channel_config##child = {         \
		DEVICE_MMIO_ROM_INIT(DT_PARENT(child)),                                            \
		.channel = DT_PROP(child, channel),                                                \
		.output_mode = DT_ENUM_IDX_OR(child, output_mode, CY_LPCOMP_OUT_DIRECT),           \
		.hysteresis = DT_PROP_OR(child, hysteresis, CY_LPCOMP_HYST_DISABLE),               \
		.power = DT_ENUM_IDX_OR(child, power, CY_LPCOMP_MODE_NORMAL),                      \
		.intr_type = DT_ENUM_IDX_OR(child, intr_type, CY_LPCOMP_INTR_DISABLE),             \
		.input_pos =                                                                       \
			(DT_ENUM_IDX_OR(child, input_p, CY_LPCOMP_SW_GPIO) == 0)                   \
				? 0                                                                \
				: (1 << (DT_ENUM_IDX_OR(child, input_p, CY_LPCOMP_SW_GPIO) - 1)),  \
		.input_neg =                                                                       \
			(DT_ENUM_IDX_OR(child, input_n, CY_LPCOMP_SW_GPIO) == 0)                   \
				? 0                                                                \
				: (1 << (DT_ENUM_IDX_OR(child, input_n, CY_LPCOMP_SW_GPIO) - 1)),  \
	};                                                                                         \
	DEVICE_DT_DEFINE(child, ifx_lpcomp_init, NULL, &ifx_lpcomp_data_##child,                   \
			 &ifx_lpcomp_channel_config##child, POST_KERNEL,                           \
			 CONFIG_COMPARATOR_INIT_PRIORITY, &ifx_lpcomp_driver_api)

/* Wrapper that only invokes the implementation for compatible children. */
#define LPCOMP_CHANNEL_SETUP(child)                                                                \
	COND_CODE_1(DT_NODE_HAS_COMPAT(child, infineon_lp_comp_channel), \
	(LPCOMP_CHANNEL_SETUP_IMPL(child)), ())

/*
 * The IRQ is defined by "infineon,lp-comp" compatible node and shared by all its child nodes
 * (channels). Reference the parent node directly by label for IRQ setup. dev
 * passed to the handler will be NULL since the interrupt is shared, so the handler will need to
 * iterate through all channels to check which one triggered the interrupt.
 */
#define CONFIGURE_SHARED_INTERRUPT(inst)                                                           \
	IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), lpcomp_irq_handler,           \
		    &ifx_lpcomp_instance_##inst, 0);                                               \
	irq_enable(DT_INST_IRQN(inst));

/* Set up shared IRQ handler and instantiate devices for all enabled child channels */
#define LPCOMP_INSTANCE_SETUP(inst)                                                                \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, LPCOMP_CHANNEL_SETUP)                              \
	LPCOMP_INSTANCE_FILL(inst)                                                                 \
	static int ifx_lpcomp_irq_enable_func_##inst(void)                                         \
	{                                                                                          \
		CONFIGURE_SHARED_INTERRUPT(inst);                                                  \
		return 0;                                                                          \
	};                                                                                         \
	SYS_INIT(ifx_lpcomp_irq_enable_func_##inst, POST_KERNEL, 0);

#define LPCOMP_INSTANCE_DEV_ADD(child)                                                             \
	COND_CODE_1(DT_NODE_HAS_COMPAT(child, infineon_lp_comp_channel), \
		([DT_PROP(child, channel)] = DEVICE_DT_GET_OR_NULL(child),), ())

#define LPCOMP_INSTANCE_FILL(inst)                                                                 \
	static struct ifx_lpcomp_instance ifx_lpcomp_instance_##inst = {                           \
		.num_channels = DT_INST_CHILD_NUM(inst),                                           \
		.ch_dev = {DT_INST_FOREACH_CHILD(inst, LPCOMP_INSTANCE_DEV_ADD)},                  \
	};

DT_INST_FOREACH_STATUS_OKAY(LPCOMP_INSTANCE_SETUP)
