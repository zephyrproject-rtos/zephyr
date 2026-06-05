/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_cordic

#include <errno.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/cordic.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <stm32_ll_rcc.h>
#include <stm32_ll_cordic.h>

LOG_MODULE_REGISTER(stm32_cordic, CONFIG_CORDIC_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_cordic


struct stm32_cordic_config {
	CORDIC_TypeDef *base;
	struct stm32_pclken *pclken;
	void (*irq_init)(void);
	uint32_t irq_nr;
};

struct stm32_cordic_data {
	struct k_sem calc_complete;
	volatile int error;
};

/**
 * @brief IRQ handler for CORDIC
 */
static void stm32_cordic_irq_handler(const struct device *dev)
{
	const struct stm32_cordic_config *cfg = dev->config;
	struct stm32_cordic_data *data = dev->data;
	CORDIC_TypeDef *cordic = cfg->base;

	/* Check if result is ready */
	if (LL_CORDIC_IsActiveFlag_RRDY(cordic)) {
		/* Signal completion */
		k_sem_give(&data->calc_complete);
	}
}

/**
 * @brief Maps cordic_function to LL_CORDIC_FUNCTION
 *
 * @param fn - cordic_function enum value
 * @param ll_fn - LL_CORDIC_FUNCTION value
 * @param ll_nargs - number of arguments a value of LL_CORDIC_NBWRITE
 * @param ll_nres - number of results a value of LL_CORDIC_NBREAD
 * @param ll_scale - scale value of LL_CORDIC_SCALE (function dependent)
 * @return 0 if ok, -EINVAL if mapping not found
 */
int stm32_cordic_function_map(const enum cordic_function fn, uint32_t *ll_fn,
			      uint32_t *ll_nargs, uint32_t *ll_nres, uint32_t *ll_scale)
{
	switch(fn) {
		case CORDIC_FUNC_COSINE:
			*ll_fn = LL_CORDIC_FUNCTION_COSINE;
			*ll_nargs = LL_CORDIC_NBWRITE_2;
			*ll_nres = LL_CORDIC_NBREAD_2;
			return 0;
		case CORDIC_FUNC_SINE:
			*ll_fn = LL_CORDIC_FUNCTION_SINE;
			*ll_nargs = LL_CORDIC_NBWRITE_2;
			*ll_nres = LL_CORDIC_NBREAD_2;
			return 0;
		case CORDIC_FUNC_PHASE:
			*ll_fn = LL_CORDIC_FUNCTION_PHASE;
			*ll_nargs = LL_CORDIC_NBWRITE_2;
			*ll_nres = LL_CORDIC_NBREAD_2;
			return 0;
		case CORDIC_FUNC_MODULUS:
			*ll_fn = LL_CORDIC_FUNCTION_MODULUS;
			*ll_nargs = LL_CORDIC_NBWRITE_2;
			*ll_nres = LL_CORDIC_NBREAD_2;
			return 0;
		case CORDIC_FUNC_ARCTANGENT:
			*ll_fn = LL_CORDIC_FUNCTION_ARCTANGENT;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_1;
			return 0;
		case CORDIC_FUNC_HCOSINE:
			*ll_fn = LL_CORDIC_FUNCTION_HCOSINE;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_2;
			*ll_scale = LL_CORDIC_SCALE_1; /* the only scale */
			return 0;
		case CORDIC_FUNC_HSINE:
			*ll_fn = LL_CORDIC_FUNCTION_HSINE;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_2;
			*ll_scale = LL_CORDIC_SCALE_1; /* the only scale */
			return 0;
		case CORDIC_FUNC_HARCTANGENT:
			*ll_fn = LL_CORDIC_FUNCTION_HARCTANGENT;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_1;
			*ll_scale = LL_CORDIC_SCALE_1; /* the only scale */
			return 0;
		case CORDIC_FUNC_NATURALLOG:
			*ll_fn = LL_CORDIC_FUNCTION_NATURALLOG;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_1;
			*ll_scale = LL_CORDIC_SCALE_1; /* starting scale factor */
			return 0;
		case CORDIC_FUNC_SQUAREROOT:
			*ll_fn = LL_CORDIC_FUNCTION_SQUAREROOT;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_1;
			return 0;
		default:
			LOG_ERR("unsupported function %d", fn);
			break;
	}
	return -EINVAL;
}

/**
 * @brief Maps cordic_precision to LL_CORDIC_PRECISION
 *
 * @param prec - cordic_precision enum value
 * @param ll_prec - LL_CORDIC_PRECISION value
 * @return 0 if ok, -EINVAL if mapping not found
 */
int stm32_cordic_precision_map(const enum cordic_precision prec, uint32_t *ll_prec)
{
	switch(prec) {
		case CORDIC_PRECISION_1:
			*ll_prec = LL_CORDIC_PRECISION_1CYCLE;
			return 0;
		case CORDIC_PRECISION_2:
			*ll_prec = LL_CORDIC_PRECISION_2CYCLES;
			return 0;
		case CORDIC_PRECISION_3:
			*ll_prec = LL_CORDIC_PRECISION_3CYCLES;
			return 0;
		case CORDIC_PRECISION_4:
			*ll_prec = LL_CORDIC_PRECISION_4CYCLES;
			return 0;
		case CORDIC_PRECISION_5:
			*ll_prec = LL_CORDIC_PRECISION_5CYCLES;
			return 0;
		case CORDIC_PRECISION_6:
			*ll_prec = LL_CORDIC_PRECISION_6CYCLES;
			return 0;
		case CORDIC_PRECISION_7:
			*ll_prec = LL_CORDIC_PRECISION_7CYCLES;
			return 0;
		case CORDIC_PRECISION_8:
			*ll_prec = LL_CORDIC_PRECISION_8CYCLES;
			return 0;
		case CORDIC_PRECISION_9:
			*ll_prec = LL_CORDIC_PRECISION_9CYCLES;
			return 0;
		case CORDIC_PRECISION_10:
			*ll_prec = LL_CORDIC_PRECISION_10CYCLES;
			return 0;
		case CORDIC_PRECISION_11:
			*ll_prec = LL_CORDIC_PRECISION_11CYCLES;
			return 0;
		case CORDIC_PRECISION_12:
			*ll_prec = LL_CORDIC_PRECISION_12CYCLES;
			return 0;
		case CORDIC_PRECISION_13:
			*ll_prec = LL_CORDIC_PRECISION_13CYCLES;
			return 0;
		case CORDIC_PRECISION_14:
			*ll_prec = LL_CORDIC_PRECISION_14CYCLES;
			return 0;
		case CORDIC_PRECISION_15:
			*ll_prec = LL_CORDIC_PRECISION_15CYCLES;
			return 0;
		default:
			LOG_ERR("unsupported precision %d", prec);
			break;
	}
	return -EINVAL;
}

/**
 * @brief Maps cordic_data_width to LL_CORDIC_INSIZE
 *
 * @param width - cordic_data_width enum value
 * @param ll_in_width - LL_CORDIC_INSIZE value
 * @return 0 if ok, -EINVAL if mapping not found
 */
int stm32_cordic_in_width_map(const enum cordic_data_width width, uint32_t *ll_in_width)
{
	switch(width) {
		case CORDIC_DATA_WIDTH_16BIT:
			/* 16 bits input data size (Q1.15 format) */
			*ll_in_width = LL_CORDIC_INSIZE_16BITS;
			return 0;
		case CORDIC_DATA_WIDTH_32BIT:
			/* 32 bits input data size (Q1.31 format) */
			*ll_in_width = LL_CORDIC_INSIZE_32BITS;
			return 0;
		default:
			LOG_ERR("unsupported input data width %d", width);
			break;
	}
	return -EINVAL;
}

/**
 * @brief Maps cordic_data_width to LL_CORDIC_OUTSIZE
 *
 * @param width - cordic_data_width enum value
 * @param ll_out_width - LL_CORDIC_OUTSIZE value
 * @return 0 if ok, -EINVAL if mapping not found
 */
int stm32_cordic_out_width_map(const enum cordic_data_width width, uint32_t *ll_out_width)
{
	switch(width) {
		case CORDIC_DATA_WIDTH_16BIT:
			/* 16 bits input data size (Q1.15 format) */
			*ll_out_width = LL_CORDIC_OUTSIZE_16BITS;
			return 0;
		case CORDIC_DATA_WIDTH_32BIT:
			/* 32 bits input data size (Q1.31 format) */
			*ll_out_width = LL_CORDIC_OUTSIZE_32BITS;
			return 0;
		default:
			LOG_ERR("unsupported output data width %d", width);
			break;
	}
	return -EINVAL;
}

/**
 * @brief Configure CORDIC peripheral
 */
static int stm32_cordic_configure(const struct device *dev,
				  const struct cordic_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);
	const struct stm32_cordic_config *cfg = dev->config;
	CORDIC_TypeDef *cordic = cfg->base;
	uint32_t ll_function;
	uint32_t ll_precision;
	uint32_t ll_scale = LL_CORDIC_SCALE_0; /* not applicable (default) */
	uint32_t ll_in_width;
	uint32_t ll_out_width;
	uint32_t ll_nargs;
	uint32_t ll_nres;
	int ret = 0;

	ret = stm32_cordic_function_map(config->function, &ll_function,
					&ll_nargs, &ll_nres, &ll_scale);
	if(ret != 0) {
		return ret;
	}

	ret = stm32_cordic_precision_map(config->precision, &ll_precision);
	if(ret != 0) {
		return ret;
	}

	ret = stm32_cordic_in_width_map(config->in_width, &ll_in_width);
	if(ret != 0) {
		return ret;
	}

	ret = stm32_cordic_out_width_map(config->out_width, &ll_out_width);
	if(ret != 0) {
		return ret;
	}

	/* 16-bit input data supports one input argument
	 * (RM0440 Rev 9, pp. 477-478/2140, CORDIC_WDATA.ARG and CORDIC_WDATA.RES)
	 */
	if (ll_in_width == LL_CORDIC_INSIZE_16BITS) {
		ll_nargs = LL_CORDIC_NBWRITE_1;
		ll_nres = LL_CORDIC_NBREAD_1;
	}

	LL_CORDIC_Config(cordic, ll_function, ll_precision, ll_scale,
			 ll_nargs, ll_nres, ll_in_width, ll_out_width);

	/* Ensure all memory operations are completed before continuing */
	// TODO: __sync_synchronize();

	return 0;
}

/**
 * @brief Perform CORDIC calculation
 */
static int stm32_cordic_compute(const struct device *dev,
								const int *arg_in,
								int *arg_out,
								const size_t arg_in_len,
								const size_t arg_out_len)
{
	const struct stm32_cordic_config *cfg = dev->config;
	CORDIC_TypeDef *cordic = cfg->base;
	int ret = 0;

	if(arg_in_len == 0 || arg_out_len == 0) {
		LOG_ERR("%s: invalid argument lengths: in_len: %u; "
			"out_len: %u", dev->name, arg_in_len, arg_out_len);
		return -EINVAL;
	}

	/* Using Zero-overhead mode (RM0440 Rev 9 pp. 472/2140)
	 * the CPU is blocked until result is ready:
	 */
	if((LL_CORDIC_GetInSize(cordic) == LL_CORDIC_INSIZE_32BITS) &&
	   (LL_CORDIC_GetNbWrite(cordic) == LL_CORDIC_NBWRITE_2)) {
		/* 32-bit format x2 input arguments - x2 writes needed */
		LL_CORDIC_WriteData(cordic, (uint32_t)arg_in[0]);
		LL_CORDIC_WriteData(cordic, (uint32_t)arg_in[1]);
	} else if((LL_CORDIC_GetInSize(cordic) == LL_CORDIC_INSIZE_32BITS) &&
			  (LL_CORDIC_GetNbWrite(cordic) == LL_CORDIC_NBWRITE_1)) {
		/* 32-bit format x1 input argument - x1 write needed */
		LL_CORDIC_WriteData(cordic, (uint32_t)arg_in[0]);
	} else if (LL_CORDIC_GetInSize(cordic) == LL_CORDIC_INSIZE_16BITS) {
		/* 16-bit format */
		const uint32_t arg_2x16bit = (arg_in[1] << 16) | (arg_in[0] & GENMASK(15, 0));
		LL_CORDIC_WriteData(cordic, (uint32_t)arg_2x16bit);
	}

	/* TODO: Implement interrupt, polling and DMA (streaming) modes. Start it here! */

	for(uint32_t i = 0; i < (LL_CORDIC_IsActiveFlag_RRDY(cordic) + 1); i++) {
		arg_out[i] = (uint32_t)LL_CORDIC_ReadData(cordic);
	}

	return ret;
}

static int stm32_cordic_pm_callback(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	// const struct stm32_cordic_config *cfg = dev->config;
	// CORDIC_TypeDef *cordic = cfg->base;
	// if (action == PM_DEVICE_ACTION_RESUME) {
	// 	/* Sleep disable */
	// 	CLEAR_BIT(RCC->AHB1SMENR, RCC_AHB1SMENR_CORDICSMEN);
	// }
	// if (IS_ENABLED(CONFIG_PM_DEVICE) && action == PM_DEVICE_ACTION_SUSPEND) {
	// 	/* Sleep enable */
	// 	SET_BIT(RCC->AHB1SMENR, RCC_AHB1SMENR_CORDICSMEN);
	// }

	return 0;
}

static int cordic_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_cordic_config *cfg = dev->config;
	struct stm32_cordic_data *data = dev->data;
	int ret = 0;

	if (!device_is_ready(clk)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(clk, &cfg->pclken[0]);
	if (ret != 0) {
		LOG_ERR("%s clock on failed (%d)", dev->name, ret);
		return ret;
	}

	k_sem_init(&data->calc_complete, 0, 1);

	// cfg->irq_init();

	LOG_DBG("%s: CORDIC device initialized", dev->name);
	return pm_device_driver_init(dev, stm32_cordic_pm_callback);
}

static DEVICE_API(cordic, stm32_cordic_api) = {
	.configure = stm32_cordic_configure,
	.compute = stm32_cordic_compute,
};

#define STM32_CORDIC_IRQ_HANDLER_DEFINE(idx)                                   \
                                                                               \
        static void stm32_cordic_irq_init_##idx(void)                          \
        {                                                                      \
                IRQ_CONNECT(DT_INST_IRQN(idx),                                 \
                            DT_INST_IRQ(idx, priority),                        \
                            stm32_cordic_irq_handler,                          \
                            DEVICE_DT_INST_GET(idx), 0);                       \
                irq_enable(DT_INST_IRQN(idx));                                 \
        }

/* Device instantiation macro */
#define STM32_CORDIC_INIT(idx)                                                 \
                                                                               \
        static struct stm32_pclken cordic_clk_##idx[] =                        \
                                                    STM32_DT_INST_CLOCKS(idx); \
                                                                               \
        static struct stm32_cordic_data cordic_data_##idx;                     \
                                                                               \
        STM32_CORDIC_IRQ_HANDLER_DEFINE(idx)                                   \
                                                                               \
        static const struct stm32_cordic_config cordic_config_##idx = {        \
                .base = (CORDIC_TypeDef *)DT_INST_REG_ADDR(idx),               \
                .pclken = cordic_clk_##idx,                                    \
                .irq_init = stm32_cordic_irq_init_##idx,                       \
                .irq_nr = DT_INST_IRQN(idx),                                   \
        };                                                                     \
                                                                               \
        PM_DEVICE_DT_INST_DEFINE(idx, stm32_cordic_pm_callback);               \
                                                                               \
        DEVICE_DT_INST_DEFINE(idx,                                             \
                              cordic_stm32_init,                               \
                              PM_DEVICE_DT_INST_GET(idx),                      \
                              &cordic_data_##idx,                              \
                              &cordic_config_##idx,                            \
                              POST_KERNEL,                                     \
                              CONFIG_CORDIC_INIT_PRIORITY,                     \
                              &stm32_cordic_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_CORDIC_INIT)
