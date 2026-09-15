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

#include <zephyr/dsp/types.h>
#include <zephyr/dsp/utils.h>

#include <stm32_ll_rcc.h>
#include <stm32_ll_cordic.h>


LOG_MODULE_REGISTER(stm32_cordic, CONFIG_CORDIC_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_cordic


struct stm32_cordic_cfg_private {
	CORDIC_TypeDef *base;
	struct stm32_pclken *pclken;
	void (*irq_init)(void);
	uint32_t irq_nr;
};

struct stm32_cordic_data {
	cordic_callback_t cb;
	enum stm32_cordic_data_mode data_mode;
	struct k_sem compute_sem;
	volatile int error;  /* TODO: Clarify if it is needed in the end */
};

/**
 * @brief IRQ handler for CORDIC
 */
static void stm32_cordic_irq_handler(const struct device *dev)
{
	const struct stm32_cordic_cfg_private *cfg = dev->config;
	struct stm32_cordic_data *data = dev->data;
	CORDIC_TypeDef *cordic = cfg->base;
	const uint32_t ll_out_size = LL_CORDIC_GetOutSize(cordic);
	const uint32_t ll_nres = LL_CORDIC_GetNbRead(cordic);
	const uint32_t nres = (ll_nres >> (CORDIC_CSR_NARGS_Pos - 1)) + 1;
	int arg_out[2];

	/* Read the result(s) */
	for(size_t i = 0; i < nres; i++) {
		arg_out[i] = (int)LL_CORDIC_ReadData(cordic);
	}

	/* Match 16-bit output */
	if (ll_out_size == LL_CORDIC_OUTSIZE_16BITS) {
		/* In case of x2 results both 16 bit results will be packed into first 32-bit arg */
		const uint32_t arg_out_2x16bit = arg_out[0];
		arg_out[0] = (uint16_t)arg_out_2x16bit;
		arg_out[1] = (uint16_t)(arg_out_2x16bit >> 16);
	}

	k_sem_give(&data->compute_sem);

	if(data->cb == NULL) {
		return;
	}

	/* Pass results to callback */
	data->cb(dev, arg_out, nres);
}

/**
 * @brief Maps cordic_function to LL_CORDIC_FUNCTION
 *
 * @param fn - cordic_function enum value
 * @param ll_fn - LL_CORDIC_FUNCTION value
 * @param ll_nargs - number of arguments a value of LL_CORDIC_NBWRITE
 * @param ll_nres - number of results a value of LL_CORDIC_NBREAD
 * @return 0 if ok, -EINVAL if mapping not found
 */
int stm32_cordic_function_map(const enum cordic_function fn, uint32_t *ll_fn,
							  uint32_t *ll_nargs, uint32_t *ll_nres)
{
	int ret = 0;

	switch(fn) {
		case CORDIC_FUNC_COSINE:
			*ll_fn = LL_CORDIC_FUNCTION_COSINE;
			*ll_nargs = LL_CORDIC_NBWRITE_2;
			*ll_nres = LL_CORDIC_NBREAD_2;
			break;
		case CORDIC_FUNC_SINE:
			*ll_fn = LL_CORDIC_FUNCTION_SINE;
			*ll_nargs = LL_CORDIC_NBWRITE_2;
			*ll_nres = LL_CORDIC_NBREAD_2;
			break;
		case CORDIC_FUNC_PHASE:
			*ll_fn = LL_CORDIC_FUNCTION_PHASE;
			*ll_nargs = LL_CORDIC_NBWRITE_2;
			*ll_nres = LL_CORDIC_NBREAD_2;
			break;
		case CORDIC_FUNC_MODULUS:
			*ll_fn = LL_CORDIC_FUNCTION_MODULUS;
			*ll_nargs = LL_CORDIC_NBWRITE_2;
			*ll_nres = LL_CORDIC_NBREAD_2;
			break;
		case CORDIC_FUNC_ARCTANGENT:
			*ll_fn = LL_CORDIC_FUNCTION_ARCTANGENT;
			/* For functions taking only one input argument, ARG1,
			 * it is recommended to set NARGS = 0.
			 * source: RM0440 Rev 9, pp. 472/2140
			 */
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_1;
			break;
		case CORDIC_FUNC_HCOSINE:
			*ll_fn = LL_CORDIC_FUNCTION_HCOSINE;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_2;
			break;
		case CORDIC_FUNC_HSINE:
			*ll_fn = LL_CORDIC_FUNCTION_HSINE;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_2;
			break;
		case CORDIC_FUNC_HARCTANGENT:
			*ll_fn = LL_CORDIC_FUNCTION_HARCTANGENT;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_1;
			break;
		case CORDIC_FUNC_NATURALLOG:
			*ll_fn = LL_CORDIC_FUNCTION_NATURALLOG;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_1;
			break;
		case CORDIC_FUNC_SQUAREROOT:
			*ll_fn = LL_CORDIC_FUNCTION_SQUAREROOT;
			*ll_nargs = LL_CORDIC_NBWRITE_1;
			*ll_nres = LL_CORDIC_NBREAD_1;
			break;
		default:
			ret = -EINVAL;
			LOG_ERR("unsupported function %d", fn);
			break;
	}
	return ret;
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
	int ret = 0;
	switch(prec) {
		case CORDIC_PRECISION_1:
			*ll_prec = LL_CORDIC_PRECISION_1CYCLE;
			break;
		case CORDIC_PRECISION_2:
			*ll_prec = LL_CORDIC_PRECISION_2CYCLES;
			break;
		case CORDIC_PRECISION_3:
			*ll_prec = LL_CORDIC_PRECISION_3CYCLES;
			break;
		case CORDIC_PRECISION_4:
			*ll_prec = LL_CORDIC_PRECISION_4CYCLES;
			break;
		case CORDIC_PRECISION_5:
			*ll_prec = LL_CORDIC_PRECISION_5CYCLES;
			break;
		case CORDIC_PRECISION_6:
			*ll_prec = LL_CORDIC_PRECISION_6CYCLES;
			break;
		case CORDIC_PRECISION_7:
			*ll_prec = LL_CORDIC_PRECISION_7CYCLES;
			break;
		case CORDIC_PRECISION_8:
			*ll_prec = LL_CORDIC_PRECISION_8CYCLES;
			break;
		case CORDIC_PRECISION_9:
			*ll_prec = LL_CORDIC_PRECISION_9CYCLES;
			break;
		case CORDIC_PRECISION_10:
			*ll_prec = LL_CORDIC_PRECISION_10CYCLES;
			break;
		case CORDIC_PRECISION_11:
			*ll_prec = LL_CORDIC_PRECISION_11CYCLES;
			break;
		case CORDIC_PRECISION_12:
			*ll_prec = LL_CORDIC_PRECISION_12CYCLES;
			break;
		case CORDIC_PRECISION_13:
			*ll_prec = LL_CORDIC_PRECISION_13CYCLES;
			break;
		case CORDIC_PRECISION_14:
			*ll_prec = LL_CORDIC_PRECISION_14CYCLES;
			break;
		case CORDIC_PRECISION_15:
			*ll_prec = LL_CORDIC_PRECISION_15CYCLES;
			break;
		default:
			ret = -EINVAL;
			LOG_ERR("unsupported precision %d", prec);
			break;
	}
	return ret;
}

int stm32_cordic_scale_map(const enum cordic_scale scale, uint32_t *ll_scale)
{
	int ret = 0;
	switch(scale) {
		case CORDIC_DATA_SCALE_0:
			*ll_scale = LL_CORDIC_SCALE_0;
			break;
		case CORDIC_DATA_SCALE_1:
			*ll_scale = LL_CORDIC_SCALE_1;
			break;
		case CORDIC_DATA_SCALE_2:
			*ll_scale = LL_CORDIC_SCALE_2;
			break;
		case CORDIC_DATA_SCALE_3:
			*ll_scale = LL_CORDIC_SCALE_3;
			break;
		case CORDIC_DATA_SCALE_4:
			*ll_scale = LL_CORDIC_SCALE_4;
			break;
		case CORDIC_DATA_SCALE_5:
			*ll_scale = LL_CORDIC_SCALE_5;
			break;
		case CORDIC_DATA_SCALE_6:
			*ll_scale = LL_CORDIC_SCALE_6;
			break;
		case CORDIC_DATA_SCALE_7:
			*ll_scale = LL_CORDIC_SCALE_7;
			break;
		default:
			ret = -EINVAL;
			LOG_ERR("unsupported scale %d", scale);
			break;
	}
	return ret;
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
	int ret = 0;
	switch(width) {
		case CORDIC_DATA_WIDTH_16BIT:
			/* 16 bits input data size (Q1.15 format) */
			*ll_in_width = LL_CORDIC_INSIZE_16BITS;
			break;
		case CORDIC_DATA_WIDTH_32BIT:
			/* 32 bits input data size (Q1.31 format) */
			*ll_in_width = LL_CORDIC_INSIZE_32BITS;
			break;
		default:
			ret = -EINVAL;
			LOG_ERR("unsupported input data width %d", width);
			break;
	}
	return ret;
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
	int ret = 0;
	switch(width) {
		case CORDIC_DATA_WIDTH_16BIT:
			/* 16 bits output data size (Q1.15 format) */
			*ll_out_width = LL_CORDIC_OUTSIZE_16BITS;
			break;
		case CORDIC_DATA_WIDTH_32BIT:
			/* 32 bits output data size (Q1.31 format) */
			*ll_out_width = LL_CORDIC_OUTSIZE_32BITS;
			break;
		default:
			ret = -EINVAL;
			LOG_ERR("unsupported output data width %d", width);
			break;
	}
	return ret;
}

/**
 * @brief Configure CORDIC peripheral
 */
static int stm32_cordic_configure(const struct device *dev,
								  const struct cordic_config *config)
{
	const struct stm32_cordic_cfg_private *cfg = dev->config;
	struct stm32_cordic_data *data = dev->data;
	CORDIC_TypeDef *cordic = cfg->base;
	uint32_t ll_function;
	uint32_t ll_precision;
	uint32_t ll_scale;
	uint32_t ll_in_width;
	uint32_t ll_out_width;
	uint32_t ll_nargs;
	uint32_t ll_nres;
	int ret = 0;

	ret = stm32_cordic_function_map(config->function, &ll_function,
									&ll_nargs, &ll_nres);
	if(ret != 0) {
		return ret;
	}

	ret = stm32_cordic_precision_map(config->precision, &ll_precision);
	if(ret != 0) {
		return ret;
	}

	ret = stm32_cordic_scale_map(config->scale, &ll_scale);
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

	/* Store callback */
	if (config->callback != NULL) {
		data->cb = config->callback;
	}

	/* Always reset the semaphore at config to not trigger any stale completion signals */
	k_sem_reset(&data->compute_sem);

	/* Disable interrupt to prevent it beeing enabled from previous configuration */
	LL_CORDIC_DisableIT(cordic);

	/* Disable any DMA requests to prevent it beeing enabled from previous configuration */
	LL_CORDIC_DisableDMAReq_RD(cordic);
	LL_CORDIC_DisableDMAReq_WR(cordic);

	/* Read configuration */
	data->data_mode = STM32_CORDIC_DATA_MODE_ZERO_OVERHEAD;  /* default to zero-overhead mode */
	if(config->options != NULL) {
		switch(config->options->data_mode) {
			case STM32_CORDIC_DATA_MODE_ZERO_OVERHEAD:
				break;
			case STM32_CORDIC_DATA_MODE_INTERRUPT:
				LL_CORDIC_EnableIT(cordic);
				break;
			case STM32_CORDIC_DATA_MODE_POLLING:
				break;
			case STM32_CORDIC_DATA_MODE_DMA:
				LL_CORDIC_EnableDMAReq_RD(cordic);
				LL_CORDIC_EnableDMAReq_WR(cordic);
				break;
			default:
				LOG_ERR("unsupported read mode %d", config->options->data_mode);
				return -EINVAL;
		}
		data->data_mode = config->options->data_mode;
	}

	/* Write cordic config to device */
	LL_CORDIC_Config(cordic, ll_function, ll_precision, ll_scale,
					 ll_nargs, ll_nres, ll_in_width, ll_out_width);

	return 0;
}

/**
 * @brief Perform CORDIC calculation
 */
static int stm32_cordic_compute(const struct device *dev,
								const int *arg_in,
								const size_t arg_in_len)
{
	const struct stm32_cordic_cfg_private *cfg = dev->config;
	CORDIC_TypeDef *cordic = cfg->base;
	int ret = 0;

	if(LL_CORDIC_IsEnabledDMAReq_RD(cordic) || LL_CORDIC_IsEnabledDMAReq_WR(cordic)) {
		LOG_DBG("%s: DMA mode enabled: comput beeing controlled by DMA", dev->name);
		return ret;
	}

	if(arg_in_len == 0) {
		return -EINVAL;
	}

	/* NOTE: The scale factor will not be calculated dynamically based on function and range
	 * intentionally to keep the API simple.
	 * The user is expected to set the correct scale factor in the config and
	 * perform necessary scaling of input and output data in the application.
	 */

	/* Using Zero-overhead mode (RM0440 Rev 9 pp. 472/2140)
	 * the CPU is blocked until result is ready:
	 */
	if(LL_CORDIC_GetInSize(cordic) == LL_CORDIC_INSIZE_32BITS &&
	   LL_CORDIC_GetNbWrite(cordic) == LL_CORDIC_NBWRITE_2) {
		/* 32-bit format x2 input arguments - x2 writes needed
		 * arg0 must be written first (472/2138 RM0440 Rev 8)
		 */
		LL_CORDIC_WriteData(cordic, (uint32_t)arg_in[0]);
		LL_CORDIC_WriteData(cordic, (uint32_t)arg_in[1]);
	} else if(LL_CORDIC_GetInSize(cordic) == LL_CORDIC_INSIZE_32BITS &&
			  LL_CORDIC_GetNbWrite(cordic) == LL_CORDIC_NBWRITE_1) {
		/* 32-bit format x1 input argument - x1 write needed */
		LL_CORDIC_WriteData(cordic, (uint32_t)arg_in[0]);
	} else if (LL_CORDIC_GetInSize(cordic) == LL_CORDIC_INSIZE_16BITS) {
		/* 16-bit format */
		const uint32_t arg_in_2x16bit = (arg_in[1] << 16) | (arg_in[0] & GENMASK(15, 0));
		LL_CORDIC_WriteData(cordic, (uint32_t)arg_in_2x16bit);
	} else {
		LOG_ERR("%s: unsupported input data width and nargs combination: in_size: %u, nargs: %u",
				dev->name, LL_CORDIC_GetInSize(cordic), LL_CORDIC_GetNbWrite(cordic));
		return -EINVAL;
	}

	return ret;
}

/**
 * @brief Perform CORDIC calculation
 */
static int stm32_cordic_read_result(const struct device *dev,
									int *arg_out,
									const size_t arg_out_len)
{
	const struct stm32_cordic_cfg_private *cfg = dev->config;
	struct stm32_cordic_data *data = dev->data;
	CORDIC_TypeDef *cordic = cfg->base;
	const uint32_t ll_out_size = LL_CORDIC_GetOutSize(cordic);
	const uint32_t ll_nres = LL_CORDIC_GetNbRead(cordic);
	const uint32_t nres = (ll_nres >> (CORDIC_CSR_NARGS_Pos - 1)) + 1;
	int ret = 0;

	/* arg_out_len - defines total number of results expected
	 * nres - defines number of total 32-bit readouts from the CODRIC HW.
	 */
	if (ll_out_size == LL_CORDIC_OUTSIZE_32BITS) {
		/* Expected output length for 32-bit results
		 * nres matches arg_out_len
		 */
		if(arg_out_len != nres) {
			return -EINVAL;
		}
	} else if (ll_out_size == LL_CORDIC_OUTSIZE_16BITS) {
		/* Expected output length for 16-bit results
		 * arg_out_len is at least one 32-bit value independent of nres
		 */
		if(arg_out_len < 1) {
			return -EINVAL;
		}
	}

	switch(data->data_mode) {
		case STM32_CORDIC_DATA_MODE_INTERRUPT:
			/* Wait for computation to complete signaled by the interrupt handler */
			ret = k_sem_take(&data->compute_sem, K_FOREVER);
			if(ret != 0) {
				LOG_ERR("%s: failed to take compute semaphore: %d", dev->name, ret);
				return ret;
			}
			break;
		case STM32_CORDIC_DATA_MODE_POLLING:
			/* Polling mode: wait until result is ready - busy wait */
			while(!LL_CORDIC_IsActiveFlag_RRDY(cordic));
			break;
		case STM32_CORDIC_DATA_MODE_ZERO_OVERHEAD:
			/* Zero-overhead mode: the CPU is already stalled until result is ready,
			 * so we can proceed to read the result immediately.
			 */
			break;
		case STM32_CORDIC_DATA_MODE_DMA:
			/* DMA mode: the result is expected to be read by the DMA controller.
			 * For this particular implementation it is still allowed to read the
			 * result directly from the CORDIC peripheral, thus we can proceed
			 * to read the result immediately using CPU.
			 */
			break;
		default:
			LOG_ERR("%s: unsupported read mode %d", dev->name, data->data_mode);
			return -EINVAL;
	}

	/* Read the result(s) */
	for(size_t i = 0; i < nres; i++) {
		arg_out[i] = (int)LL_CORDIC_ReadData(cordic);
	}

	/* Match 16-bit output */
	if (ll_out_size == LL_CORDIC_OUTSIZE_16BITS) {
		/* In case of x2 results both 16 bit results will be packed into first 32-bit arg */
		const uint32_t arg_out_2x16bit = arg_out[0];
		arg_out[0] = (uint16_t)arg_out_2x16bit;
		arg_out[1] = (uint16_t)(arg_out_2x16bit >> 16);
	}

	return ret;
}

static int stm32_cordic_pm_callback(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

static int cordic_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_cordic_cfg_private *cfg = dev->config;
	struct stm32_cordic_data *data = dev->data;
	CORDIC_TypeDef *cordic = cfg->base;
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

	k_sem_init(&data->compute_sem, 0, 1);

	cfg->irq_init();
	LL_CORDIC_DisableIT(cordic);  /* disable until enabled in config phase */

	return pm_device_driver_init(dev, stm32_cordic_pm_callback);
}

static DEVICE_API(cordic, stm32_cordic_api) = {
	.configure = stm32_cordic_configure,
	.compute = stm32_cordic_compute,
	.read_result = stm32_cordic_read_result,
};

#define STM32_CORDIC_IRQ_HANDLER_DEFINE(idx)                                    \
                                                                                \
        static void stm32_cordic_irq_init_##idx(void)                           \
        {                                                                       \
                IRQ_CONNECT(DT_INST_IRQN(idx),                                  \
                            DT_INST_IRQ(idx, priority),                         \
                            stm32_cordic_irq_handler,                           \
                            DEVICE_DT_INST_GET(idx), 0);                        \
                irq_enable(DT_INST_IRQN(idx));                                  \
        }

/* Device instantiation macro */
#define STM32_CORDIC_INIT(idx)                                                  \
                                                                                \
        static struct stm32_pclken cordic_clk_##idx[] =                         \
                                                    STM32_DT_INST_CLOCKS(idx);  \
                                                                                \
        static struct stm32_cordic_data cordic_data_##idx;                      \
                                                                                \
        STM32_CORDIC_IRQ_HANDLER_DEFINE(idx)                                    \
                                                                                \
        static const struct stm32_cordic_cfg_private cordic_config_##idx = { \
                .base = (CORDIC_TypeDef *)DT_INST_REG_ADDR(idx),                \
                .pclken = cordic_clk_##idx,                                     \
                .irq_init = stm32_cordic_irq_init_##idx,                        \
                .irq_nr = DT_INST_IRQN(idx),                                    \
        };                                                                      \
                                                                                \
        PM_DEVICE_DT_INST_DEFINE(idx, stm32_cordic_pm_callback);                \
                                                                                \
        DEVICE_DT_INST_DEFINE(idx,                                              \
                              cordic_stm32_init,                                \
                              PM_DEVICE_DT_INST_GET(idx),                       \
                              &cordic_data_##idx,                               \
                              &cordic_config_##idx,                             \
                              POST_KERNEL,                                      \
                              CONFIG_CORDIC_INIT_PRIORITY,                      \
                              &stm32_cordic_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_CORDIC_INIT)
