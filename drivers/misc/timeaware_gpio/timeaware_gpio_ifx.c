/*
 * copyright (c) 2026 Linumiz 
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT infineon_tcpwm_tgpio

#include <zephyr/device.h>
#include <zephyr/drivers/misc/timeaware_gpio/timeaware_gpio.h>
#include <zephyr/drivers/pinctrl.h> 
#include <zephyr/logging/log.h>                                
LOG_MODULE_REGISTER(tgpio_ifx_tcpwm, CONFIG_TGPIO_LOG_LEVEL);

#include <cy_tcpwm_counter.h>  
#include <cy_tcpwm_pwm.h>        
#include <cy_sysclk.h>      

struct tgpio_ifx_tcpwm_config {
	TCPWM_GRP_CNT_Type *reg_base;      
	bool resolution_32_bits;     
	cy_en_divider_types_t divider_type;
	uint32_t divider_sel;
	uint32_t divider_val;
	uint32_t clock_peri_group;
	const struct pinctrl_dev_config *pcfg;       
	void (*irq_enable_func)(const struct device *dev);  
};

struct tgpio_ifx_tcpwm_data {
	struct {
		uint64_t last_timestamp; 
		uint64_t event_count; 
	} pin; 
	uint32_t cnt_idx;                  
};

static void tgpio_ifx_tcpwm_isr(const struct device *dev)
{
	struct tgpio_ifx_tcpwm_data *data = dev->data;

	uint32_t intr = Cy_TCPWM_GetInterruptStatusMasked(TCPWM0, data->cnt_idx);

	if (intr & CY_TCPWM_INT_ON_CC0) {
		uint32_t cap = Cy_TCPWM_Counter_GetCapture(TCPWM0, data->cnt_idx);
		data->pin.last_timestamp = (uint64_t)cap;
		data->pin.event_count++;

	}
	Cy_TCPWM_ClearInterrupt(TCPWM0, data->cnt_idx, CY_TCPWM_INT_ON_CC0);

}

static int tgpio_ifx_tcpwm_get_time(const struct device *dev, uint64_t *t)
{
	struct tgpio_ifx_tcpwm_data *data = dev->data;
	*t = Cy_TCPWM_Counter_GetCounter(TCPWM0, data->cnt_idx); 
	return 0;
}

static int tgpio_ifx_tcpwm_cyc_per_sec(const struct device *dev, uint32_t *cyc)
{
	const struct tgpio_ifx_tcpwm_config *cfg = dev->config;
	*cyc = Cy_SysClk_PeriPclkGetFrequency(cfg->clock_peri_group, cfg->divider_type, cfg->divider_sel);
	return 0;
}

static int tgpio_ifx_tcpwm_pin_disable(const struct device *dev, uint32_t pin)
{
	struct tgpio_ifx_tcpwm_data *data = dev->data;
	if (pin != 0) return -EINVAL; 

	Cy_TCPWM_TriggerStopOrKill_Single(TCPWM0, data->cnt_idx);  
	Cy_TCPWM_Disable_Single(TCPWM0, data->cnt_idx);            
	return 0;
}

static int tgpio_ifx_tcpwm_set_per_out(const struct device *dev, uint32_t pin,
					uint64_t start_time, uint64_t period, bool periodic)
{
	struct tgpio_ifx_tcpwm_data *data = dev->data;
	const struct tgpio_ifx_tcpwm_config *cfg = dev->config;

	if (pin != 0) return -EINVAL;
	if (period > UINT32_MAX) return -EINVAL; 

	uint32_t idx = data->cnt_idx;
	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Pinctrl failed: %d", ret);
		return ret;
	}

	Cy_TCPWM_TriggerStopOrKill_Single(TCPWM0, idx);
	Cy_TCPWM_Disable_Single(TCPWM0, idx);

	cy_stc_tcpwm_pwm_config_t pwm_cfg = {
		.pwmMode = CY_TCPWM_PWM_MODE_PWM,
		.clockPrescaler = CY_TCPWM_PWM_PRESCALER_DIVBY_1,
		.pwmAlignment = CY_TCPWM_PWM_LEFT_ALIGN,
		.runMode = periodic ? CY_TCPWM_PWM_CONTINUOUS : CY_TCPWM_PWM_ONESHOT,
		.countInputMode = CY_TCPWM_INPUT_LEVEL,
		.countInput = CY_TCPWM_INPUT_1,
		.period0 = (uint32_t)period - 1,       
		.compare0 = (uint32_t)(period / 2),    
		.enablePeriodSwap = false,          
		.enableCompareSwap = false,
		.interruptSources = 0,                
	};

	if (Cy_TCPWM_PWM_Init(TCPWM0, idx, &pwm_cfg) != CY_TCPWM_SUCCESS) {
		return -EIO;
	}

	Cy_TCPWM_PWM_Enable(TCPWM0, idx);         
	Cy_TCPWM_TriggerStart_Single(TCPWM0, idx); 

	ARG_UNUSED(start_time);  
	data->pin.event_count = 0; 
	return 0;
}

static int tgpio_ifx_tcpwm_config_ext_ts(const struct device *dev, uint32_t pin,
					 uint32_t edge)
{
	struct tgpio_ifx_tcpwm_data *data = dev->data;
	const struct tgpio_ifx_tcpwm_config *cfg = dev->config;

	if (pin != 0) return -EINVAL;

	uint32_t idx = data->cnt_idx;

	Cy_TCPWM_TriggerStopOrKill_Single(TCPWM0, idx);
	Cy_TCPWM_Disable_Single(TCPWM0, idx);

	data->pin.last_timestamp = 0;
	data->pin.event_count = 0;

	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Pinctrl failed: %d", ret);
		return ret;
	}

	cy_stc_tcpwm_counter_config_t cap_cfg = {
		.period = cfg->resolution_32_bits ? 0xFFFFFFFFu : 0xFFFFu,
		.clockPrescaler = CY_TCPWM_COUNTER_PRESCALER_DIVBY_1,
		.runMode = CY_TCPWM_COUNTER_CONTINUOUS,
		.countDirection = CY_TCPWM_COUNTER_COUNT_UP,
		.compareOrCapture = CY_TCPWM_COUNTER_MODE_CAPTURE,
		.captureInputMode = edge == TGPIO_FALLING_EDGE ? CY_TCPWM_INPUT_FALLINGEDGE :
				    edge == TGPIO_TOGGLE_EDGE ? CY_TCPWM_INPUT_EITHEREDGE :
					    CY_TCPWM_INPUT_RISINGEDGE,
		.captureInput = CY_TCPWM_INPUT_TRIG_0,
		.interruptSources = CY_TCPWM_INT_ON_CC0,
		.countInput = CY_TCPWM_INPUT_1,
		.countInputMode = 0x03,
	};

	if (Cy_TCPWM_Counter_Init(TCPWM0, idx, &cap_cfg) != CY_TCPWM_SUCCESS) {
		return -EIO;
	}

	Cy_TCPWM_Counter_Enable(TCPWM0, idx);
	Cy_TCPWM_SetInterruptMask(TCPWM0, idx, CY_TCPWM_INT_ON_CC0);

	Cy_TCPWM_TriggerReloadOrIndex_Single(TCPWM0, idx);
	Cy_TCPWM_TriggerStart_Single(TCPWM0, idx); 

	return 0;
}

static int tgpio_ifx_tcpwm_read_ts_ec(const struct device *dev, uint32_t pin,
				      uint64_t *ts, uint64_t *ec)
{
	struct tgpio_ifx_tcpwm_data *data = dev->data;
	if (pin != 0) return -EINVAL;

	*ts = data->pin.last_timestamp;
	*ec = data->pin.event_count;
	return 0;
}

static const struct tgpio_driver_api tgpio_ifx_tcpwm_api = {
	.pin_disable = tgpio_ifx_tcpwm_pin_disable,
	.get_time = tgpio_ifx_tcpwm_get_time,
	.cyc_per_sec = tgpio_ifx_tcpwm_cyc_per_sec,
	.set_perout = tgpio_ifx_tcpwm_set_per_out,
	.config_ext_ts = tgpio_ifx_tcpwm_config_ext_ts,
	.read_ts_ec = tgpio_ifx_tcpwm_read_ts_ec,
};

static int tgpio_ifx_tcpwm_init(const struct device *dev)
{
	struct tgpio_ifx_tcpwm_data *data = dev->data;
	struct tgpio_ifx_tcpwm_config *cfg = (void *)dev->config;

	Cy_SysClk_PeriPclkDisableDivider(cfg->clock_peri_group, cfg->divider_type, cfg->divider_sel);
	Cy_SysClk_PeriPclkSetDivider(cfg->clock_peri_group, cfg->divider_type, cfg->divider_sel, cfg->divider_val);
	Cy_SysClk_PeriPclkEnableDivider(cfg->clock_peri_group, cfg->divider_type, cfg->divider_sel);

	uint32_t clk_connection;
	if ((uint32_t)cfg->reg_base >= (uint32_t)TCPWM0_GRP2) {
		clk_connection = PCLK_TCPWM0_CLOCKS512 + (((uint32_t)cfg->reg_base - (uint32_t)TCPWM0_GRP2) / sizeof(TCPWM_GRP_CNT_Type));
		data->cnt_idx = 512 + (((uint32_t)cfg->reg_base - (uint32_t)TCPWM0_GRP2) / sizeof(TCPWM_GRP_CNT_Type));
	} else if ((uint32_t)cfg->reg_base >= (uint32_t)TCPWM0_GRP1) {
		clk_connection = PCLK_TCPWM0_CLOCKS256 + (((uint32_t)cfg->reg_base - (uint32_t)TCPWM0_GRP1) / sizeof(TCPWM_GRP_CNT_Type));
		data->cnt_idx = 256 + (((uint32_t)cfg->reg_base - (uint32_t)TCPWM0_GRP1) / sizeof(TCPWM_GRP_CNT_Type));
	} else {  
		clk_connection = PCLK_TCPWM0_CLOCKS0 + (((uint32_t)cfg->reg_base - (uint32_t)TCPWM0_GRP0) / sizeof(TCPWM_GRP_CNT_Type));
		data->cnt_idx = ((uint32_t)cfg->reg_base - (uint32_t)TCPWM0_GRP0) / sizeof(TCPWM_GRP_CNT_Type);
	}
	Cy_SysClk_PeriPclkAssignDivider(clk_connection, cfg->divider_type, cfg->divider_sel);

	cfg->irq_enable_func(dev);

	return 0;
}

#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1C)

#define TGPIO_IFX_IRQ_ENABLE(inst)                                                   \
	static void tgpio_ifx_irq_enable_##inst(const struct device *dev) {          \
		enable_sys_int(DT_INST_PROP_BY_IDX(inst, system_interrupts, 0),      \
				DT_INST_PROP_BY_IDX(inst, system_interrupts, 1),     \
				(void (*)(const void *))tgpio_ifx_tcpwm_isr, dev);   \
	}

#define TGPIO_IFX_INST(inst)                                                 	     \
	TGPIO_IFX_IRQ_ENABLE(inst)                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                \
	static struct tgpio_ifx_tcpwm_data tgpio_data_##inst;                        \
	const static struct tgpio_ifx_tcpwm_config tgpio_config_##inst = {           \
		.reg_base = (TCPWM_GRP_CNT_Type *)DT_REG_ADDR(DT_INST_PARENT(inst)), \
		.resolution_32_bits = DT_PROP(DT_INST_PARENT(inst), resolution) == 32, \
		.divider_type = DT_PROP(DT_INST_PARENT(inst), divider_type),        \
		.divider_sel = DT_PROP(DT_INST_PARENT(inst), divider_sel),          \
		.divider_val = DT_PROP(DT_INST_PARENT(inst), divider_val),          \
		.clock_peri_group = DT_PROP(DT_INST_PARENT(inst), ifx_peri_group),  \
		.irq_enable_func = tgpio_ifx_irq_enable_##inst,                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                       \
	};                                                                     	    \
	DEVICE_DT_INST_DEFINE(inst, tgpio_ifx_tcpwm_init, NULL,                     \
			&tgpio_data_##inst, &tgpio_config_##inst,         	    \
			POST_KERNEL, CONFIG_TIMEAWARE_GPIO_INIT_PRIORITY,           \
			&tgpio_ifx_tcpwm_api);
#endif
DT_INST_FOREACH_STATUS_OKAY(TGPIO_IFX_INST)
