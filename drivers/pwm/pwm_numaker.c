/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_pwm

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <soc.h>

#include "NuMicro.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_numaker, LOG_LEVEL_ERR);

/* 11-bit prescaler in Numaker EPWM modules */
#define NUMAKER_PWM_MAX_PRESCALER      (1UL << (11))
#define NUMAKER_PWM_CHANNEL_COUNT       6
#define NUMAKER_PWM_RELOAD_CNT          (0xFFFFU)

/* Device config */
struct pwm_numaker_config {
    /* EPWM base address */
    EPWM_T *epwm;
    uint32_t id_rst;
    uint32_t prescale;
    /* clock configuration */
	uint32_t clk_modidx;
    uint32_t clk_src;
    uint32_t clk_div;
    const struct device *clk_dev;
    const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
};

struct pwm_numaker_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
    /* Only support either one of PWM_CAPTURE_TYPE_PULSE, PWM_CAPTURE_TYPE_PERIOD */
	bool pulse_capture;
    bool single_mode;
    bool is_busy;
    uint32_t currChanEdgeMode;
    uint32_t nextChanEdgeMode;    
};

/* Driver context/data */
struct pwm_numaker_data {
    uint32_t clock_freq;
    uint32_t cycles_per_sec;
#ifdef CONFIG_PWM_CAPTURE
	uint32_t overflows;
	struct pwm_numaker_capture_data capture[NUMAKER_PWM_CHANNEL_COUNT];
#endif /* CONFIG_PWM_CAPTURE */
};


static void __pwm_numaker_configure(const struct device *dev)
{
    const struct pwm_numaker_config *cfg = dev->config;
    EPWM_T *epwm = cfg->epwm;
    
	/* Disable EPWM channel 0~5 before config */
    EPWM_ForceStop(epwm, 0x3F);

	/* Set EPWM default normal polarity as inverse disabled */
    epwm->POLCTL &= ~(0x3F << EPWM_POLCTL_PINV0_Pos);

}

/* PWM api functions */
static int pwm_numaker_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
    const struct pwm_numaker_config *cfg = dev->config;
	struct pwm_numaker_data *data = dev->data;
    EPWM_T *epwm = cfg->epwm;
    uint32_t channelMask = (1 << channel);

    LOG_INF("===>[%s] Channel=0x%x, CAPIEN=0x%x, CAPIF=0x%x", __FUNCTION__, channel, epwm->CAPIEN, epwm->CAPIF);

	/* Set EPWM polarity */
	if (flags & PWM_POLARITY_INVERTED) {
		epwm->POLCTL |= (1 << (EPWM_POLCTL_PINV0_Pos + channel));
	} else {
        epwm->POLCTL &= ~(1 << (EPWM_POLCTL_PINV0_Pos + channel));
	}

	/* Force disable EPWM channel as while pulse_cycles = 0 */
	if (period_cycles == 0U) {
        EPWM_Stop(epwm, 1 << channel);
        EPWM_ForceStop(epwm, 1 << channel);
        EPWM_DisableOutput(epwm, 1 << channel);
		return 0;
	}

    /* Set EPWM channel & output configuration */
    EPWM_ConfigOutputChannel(epwm, channel, data->cycles_per_sec / period_cycles, (100 * pulse_cycles) / period_cycles);
    
    /* Enable EPWM Output path for EPWM channel */
    EPWM_EnableOutput(epwm, channelMask);

    /* Enable Timer for EPWM channel */
    EPWM_Start(epwm, channelMask);

    LOG_INF("===[%s] cycles_per_sec=0x%x, pulse_cycles=0x%x, period_cycles=0x%x", __FUNCTION__, data->cycles_per_sec, pulse_cycles, period_cycles);
    LOG_INF("===[%s] CTL1=0x%x, POEN=0x%x, CNTEN=0x%x", __FUNCTION__, (epwm)->CTL1, (epwm)->POEN, (epwm)->CNTEN);
    LOG_INF("<===[%s] Channel=0x%x, CAPIEN=0x%x, CAPIF=0x%x", __FUNCTION__, channel, epwm->CAPIEN, epwm->CAPIF);

	return 0;
}

static int pwm_numaker_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
    const struct pwm_numaker_config *cfg = dev->config;
	struct pwm_numaker_data *data = dev->data;
    
	ARG_UNUSED(channel);

    data->cycles_per_sec = data->clock_freq / (cfg->prescale + 1U);
	*cycles = (uint64_t)data->cycles_per_sec;

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static int pwm_numaker_configure_capture(const struct device *dev,
				      uint32_t channel, pwm_flags_t flags,
				      pwm_capture_callback_handler_t cb,
				      void *user_data)
{
    const struct pwm_numaker_config *cfg = dev->config;
	struct pwm_numaker_data *data = dev->data;
    EPWM_T *epwm = cfg->epwm;
	uint32_t pair = channel;
    uint32_t intMask = ((EPWM_CAPTURE_INT_FALLING_LATCH | EPWM_CAPTURE_INT_RISING_LATCH) << channel);

    LOG_INF("=== Enter %s ...\n", __FUNCTION__);
    
	data->capture[pair].callback = cb;
	data->capture[pair].user_data = user_data;

    if(data->capture[pair].is_busy == true) {
		LOG_ERR("Capture already active on this channel %d", pair);
		return -EBUSY;
    }
	if ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_BOTH) {
		LOG_ERR("Cannot capture both period and pulse width");
		return -ENOTSUP;
	}

	if ((flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS) {
		data->capture[pair].single_mode = false;
	} else {
		data->capture[pair].single_mode = true;
	}

	if (flags & PWM_CAPTURE_TYPE_PERIOD) {
		data->capture[pair].pulse_capture = false;

		if (flags & PWM_POLARITY_INVERTED) {
			data->capture[pair].currChanEdgeMode = EPWM_CAPTURE_INT_FALLING_LATCH;
			data->capture[pair].nextChanEdgeMode = EPWM_CAPTURE_INT_FALLING_LATCH;
		} else {
			data->capture[pair].currChanEdgeMode = EPWM_CAPTURE_INT_RISING_LATCH;
			data->capture[pair].nextChanEdgeMode = EPWM_CAPTURE_INT_RISING_LATCH;
		}
	} else {
		data->capture[pair].pulse_capture = true;

		if (flags & PWM_POLARITY_INVERTED) {
			data->capture[pair].currChanEdgeMode = EPWM_CAPTURE_INT_FALLING_LATCH;
			data->capture[pair].nextChanEdgeMode = EPWM_CAPTURE_INT_RISING_LATCH;
		} else {
			data->capture[pair].currChanEdgeMode = EPWM_CAPTURE_INT_RISING_LATCH;
			data->capture[pair].nextChanEdgeMode = EPWM_CAPTURE_INT_FALLING_LATCH;
		}
	}
    
    return 0;
}

static int pwm_numaker_enable_capture(const struct device *dev, uint32_t channel)
{
    const struct pwm_numaker_config *cfg = dev->config;
	struct pwm_numaker_data *data = dev->data;
    EPWM_T *epwm = cfg->epwm;
    uint32_t pair = channel;
    uint32_t channelMask = (1 << channel);
    uint32_t intMask = ((EPWM_CAPTURE_INT_FALLING_LATCH | EPWM_CAPTURE_INT_RISING_LATCH) << channel);
    uint32_t unitTimeNsec = (1000000000U / data->cycles_per_sec);

    LOG_INF("=== Enter %s ...\n", __FUNCTION__);
    
	if (!data->capture[pair].callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

    if(data->capture[pair].is_busy == true) {
		LOG_ERR("Capture already active on this channel %d", pair);
		return -EBUSY;
    }
    
    data->capture[pair].is_busy = true;
    
    /* Set capture configuration */
    EPWM_ConfigCaptureChannel(epwm, channel, unitTimeNsec, 0);

    /* Enable Capture Function for EPWM */
    EPWM_EnableCapture(epwm, channelMask);
    
    /* Enable Timer for EPWM */
    EPWM_Start(epwm, channelMask);

    EPWM_ClearCaptureIntFlag(epwm, channel, EPWM_CAPTURE_INT_FALLING_LATCH | EPWM_CAPTURE_INT_RISING_LATCH);

    /* EnableInterrupt */
	EPWM_EnableCaptureInt(epwm, channel, data->capture[pair].currChanEdgeMode);
    
    LOG_INF("===[%s] Channel=0x%x, CAPIEN=0x%x, CAPIF=0x%x", __FUNCTION__, channel, epwm->CAPIEN, epwm->CAPIF);

    return 0;
}

static int pwm_numaker_disable_capture(const struct device *dev, uint32_t channel)
{
    const struct pwm_numaker_config *cfg = dev->config;
	struct pwm_numaker_data *data = dev->data;
    EPWM_T *epwm = cfg->epwm;
    uint32_t channelMask = (1 << channel);

    LOG_INF("=== Enter %s ...\n", __FUNCTION__);

    data->capture[channel].is_busy = false;
    EPWM_Stop(epwm, channelMask);
    EPWM_ForceStop(epwm, channelMask);
    EPWM_DisableCapture(epwm, channelMask);
    EPWM_DisableCaptureInt(epwm, channel, EPWM_CAPTURE_INT_RISING_LATCH | EPWM_CAPTURE_INT_FALLING_LATCH);
    EPWM_ClearCaptureIntFlag(epwm, channel, EPWM_CAPTURE_INT_FALLING_LATCH | EPWM_CAPTURE_INT_RISING_LATCH);
    LOG_INF("=== Enter %s ...CAPIEN=0x%x\n", __FUNCTION__, epwm->CAPIEN);
    return 0;
}

/*--------------------------------------------------------------------------------------*/
/* Get capture cycles between current channel edge until next chnannel edge.            */
/* The capture period counter down count from 0x10000, and auto-reload to 0x10000.      */
/*--------------------------------------------------------------------------------------*/
static int __pwm_numaker_get_cap_cycle(EPWM_T *epwm, uint32_t channel, uint32_t currEdge, uint32_t nextEdge, uint32_t *cycles)
{
    uint16_t currCnt = 0;
    uint16_t nextCnt = 0;
    uint32_t next_ifMask;
    uint32_t capif_base;
    uint32_t timeOutCnt;
	int status = 0;
    uint32_t period_reloads = 0;
    
    /* PWM counter is timing critical, so avoid print message from irq_isr until getting cycles */
    LOG_DBG("=== Enter %s ...\n", __FUNCTION__);

    EPWM_ClearPeriodIntFlag(epwm, channel);
    
    capif_base = (nextEdge == EPWM_CAPTURE_INT_FALLING_LATCH) ? EPWM_CAPIF_CFLIF0_Pos : EPWM_CAPIF_CRLIF0_Pos;
    next_ifMask = (0x1ul << (capif_base + channel));
    timeOutCnt = DT_PROP(DT_NODELABEL(sysclk),clock_frequency) / 2; /* 500 ms time-out */
    LOG_DBG("===[%s] Channel=0x%x, R-Cnt=0x%x, F-Cnt0x%x, CNT-0x%x", __FUNCTION__, channel, 
            EPWM_GET_CAPTURE_RISING_DATA(epwm, channel), EPWM_GET_CAPTURE_FALLING_DATA(epwm, channel), epwm->CNT[channel]);
    currCnt = (currEdge == EPWM_CAPTURE_INT_FALLING_LATCH) ? 
        EPWM_GET_CAPTURE_FALLING_DATA(epwm, channel) : (uint16_t)EPWM_GET_CAPTURE_RISING_DATA(epwm, channel);

    /* Wait for Capture Next Indicator  */
    while((epwm->CAPIF & next_ifMask) == 0)
    {
		if(EPWM_GetPeriodIntFlag(epwm, channel)) {
			EPWM_ClearPeriodIntFlag(epwm, channel);
			period_reloads++;
        }
        if(--timeOutCnt == 0)
        {
            status = -EAGAIN;
            goto move_exit;
        }
    }

    /* Clear Capture Falling and Rising Indicator */
    EPWM_ClearCaptureIntFlag(epwm, channel, EPWM_CAPTURE_INT_FALLING_LATCH | EPWM_CAPTURE_INT_RISING_LATCH);

    /* Get Capture Latch Counter Data */
    nextCnt = (nextEdge == EPWM_CAPTURE_INT_FALLING_LATCH) ? 
                (uint16_t)EPWM_GET_CAPTURE_FALLING_DATA(epwm, channel) : (uint16_t)EPWM_GET_CAPTURE_RISING_DATA(epwm, channel);

    *cycles = (period_reloads * NUMAKER_PWM_RELOAD_CNT) + currCnt - nextCnt;
    LOG_INF("===[%s] cycles=0x%x, period_reloads=0x%x, CAPIF=0x%x, cur-0x%x ,next-0x%x", __FUNCTION__, *cycles, period_reloads, epwm->CAPIF, currCnt, nextCnt);

move_exit:
	return status;    
}

static void __pwm_numaker_isr(const struct device *dev, uint32_t stChannel, uint32_t endChannel)
{
    const struct pwm_numaker_config *cfg = dev->config;
	struct pwm_numaker_data *data = dev->data;
    EPWM_T *epwm = cfg->epwm;
    struct pwm_numaker_capture_data *capture;
	uint32_t int_status;
    uint32_t cap_intsts;
	uint32_t cycles = 0;
	int status = 0;
    int i;
    uint32_t intMask = (1UL << stChannel | 1UL << endChannel);
    uint32_t capIntMask = (EPWM_CAPIF_CFLIF0_Msk << stChannel | 
                            EPWM_CAPIF_CRLIF0_Msk << stChannel |
                            EPWM_CAPIF_CFLIF0_Msk << endChannel |
                            EPWM_CAPIF_CRLIF0_Msk << endChannel);
    
    /* Get Output int status  */
    int_status = (epwm->AINTSTS & intMask);
    /* Clear Output int status */
    if(int_status != 0x00)
        epwm->AINTSTS = int_status;
    
    /* Get CAP int status  */
    cap_intsts = (epwm->CAPIF & capIntMask);
    
    /* PWM counter is timing critical, so avoid print message from irq_isr until getting capture cycles */
    LOG_DBG("===[%s] Channel=0x%x, CAPIEN=0x%x, CAPIF=0x%x, capIntMask=0x%x", __FUNCTION__, stChannel, epwm->CAPIEN, epwm->CAPIF, capIntMask);
    if(cap_intsts != 0x00) {   /* Capture Interrupt */
        /* Clear CAP int status */
        epwm->CAPIF = cap_intsts;
        
        /* Rising latch or Falling latch */
        for( i = stChannel; i <= endChannel; i++ ) {
            capture = &data->capture[i];
            if(capture == NULL) continue;
            if(((EPWM_CAPTURE_INT_RISING_LATCH << i) | 
                (EPWM_CAPTURE_INT_FALLING_LATCH << i)) & cap_intsts) {

                EPWM_DisableCaptureInt(epwm, i, EPWM_CAPTURE_INT_RISING_LATCH | EPWM_CAPTURE_INT_FALLING_LATCH);
                /* Calculate cycles */
                status = __pwm_numaker_get_cap_cycle(epwm, i, data->capture[i].currChanEdgeMode, 
                                                                data->capture[i].nextChanEdgeMode, &cycles);
                if (capture->pulse_capture) {
                    /* For PWM_CAPTURE_TYPE_PULSE */
                    capture->callback(dev, i, 0, cycles, status,
                              capture->user_data);
                } else {                            
                    /* For PWM_CAPTURE_TYPE_PERIOD */
                    capture->callback(dev, i, cycles, 0, status,
                              capture->user_data);
                }

                if (capture->single_mode) {
                    EPWM_DisableCaptureInt(epwm, i, EPWM_CAPTURE_INT_RISING_LATCH | EPWM_CAPTURE_INT_FALLING_LATCH);
                    data->capture[i].is_busy = false;
                } else {
                    EPWM_ClearCaptureIntFlag(epwm, i, EPWM_CAPTURE_INT_FALLING_LATCH | EPWM_CAPTURE_INT_RISING_LATCH);
                    EPWM_EnableCaptureInt(epwm, i, data->capture[i].currChanEdgeMode);
                    //data->capture[i].is_busy = true;
                }
            }
        }
    }
}

static void pwm_numaker_p0_isr(const struct device *dev)
{
    /* Pair0 service channel 0, 1 */
    __pwm_numaker_isr(dev, 0, 1);
}

static void pwm_numaker_p1_isr(const struct device *dev)
{
    /* Pair1 service channel 2, 3 */
    __pwm_numaker_isr(dev, 2, 3);
}

static void pwm_numaker_p2_isr(const struct device *dev)
{
    /* Pair2 service channel 4, 5 */
    __pwm_numaker_isr(dev, 4, 5);
}
#endif /* CONFIG_PWM_CAPTURE */

/* PWM driver registration */
static const struct pwm_driver_api pwm_numaker_driver_api = {
	.set_cycles = pwm_numaker_set_cycles,
	.get_cycles_per_sec = pwm_numaker_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_numaker_configure_capture,
	.enable_capture = pwm_numaker_enable_capture,
	.disable_capture = pwm_numaker_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

/* Alternative EPWM clock get rate before support standard clock_control_get_rate */
static int __pwm_numaker_clk_get_rate(EPWM_T *epwm, uint32_t *rate)
{
    uint32_t u32Src;
    uint32_t u32EPWMClockSrc;


    if(epwm == EPWM0)
    {
        u32Src = CLK->CLKSEL2 & CLK_CLKSEL2_EPWM0SEL_Msk;
    }
    else     /* (epwm == EPWM1) */
    {
        u32Src = CLK->CLKSEL2 & CLK_CLKSEL2_EPWM1SEL_Msk;
    }

    if(u32Src == 0U)
    {
        /* clock source is from PLL clock */
        u32EPWMClockSrc = CLK_GetPLLClockFreq();
    }
    else
    {
        /* clock source is from PCLK */
        SystemCoreClockUpdate();
        if(epwm == EPWM0)
        {
            u32EPWMClockSrc = CLK_GetPCLK0Freq();
        }
        else     /* (epwm == EPWM1) */
        {
            u32EPWMClockSrc = CLK_GetPCLK1Freq();
        }
    }
    *rate = u32EPWMClockSrc;
    return 0;
}

static int pwm_numaker_init(const struct device *dev)
{
    const struct pwm_numaker_config *cfg = dev->config;
	struct pwm_numaker_data *data = dev->data;
    EPWM_T *epwm = cfg->epwm;
    uint32_t clock_freq;
	int err = 0;

    struct numaker_scc_subsys scc_subsys;
    
    SYS_UnlockReg();

    /* CLK controller */
    memset(&scc_subsys, 0x00, sizeof(scc_subsys));
    scc_subsys.subsys_id        = NUMAKER_SCC_SUBSYS_ID_PCC;
    scc_subsys.pcc.clk_modidx   = cfg->clk_modidx;
    scc_subsys.pcc.clk_src      = cfg->clk_src;
    scc_subsys.pcc.clk_div      = cfg->clk_div;

    /* Equivalent to CLK_EnableModuleClock() */
    err = clock_control_on(cfg->clk_dev, (clock_control_subsys_t) &scc_subsys);
    if (err != 0) {
        goto move_exit;
    }
    /* Equivalent to CLK_SetModuleClock() */
    err = clock_control_configure(cfg->clk_dev, (clock_control_subsys_t) &scc_subsys, NULL);
    if (err != 0) {
        goto move_exit;
    }

#if 0  /* Not support standard clock_control_get_rate yet */
	err = clock_control_get_rate(cfg->clk_dev, (clock_control_subsys_t) &scc_subsys, &clock_freq);
#else
	err = __pwm_numaker_clk_get_rate(epwm, &clock_freq);
#endif
	if (err < 0) {
		LOG_ERR("Get EPWM clock rate failure %d", err);
		goto move_exit;
	}
    data->clock_freq = clock_freq;
    data->cycles_per_sec = data->clock_freq / (cfg->prescale + 1U);
    
	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to apply pinctrl state");
        goto move_exit;
	}

    // Reset this module
    SYS_ResetModule(cfg->id_rst);

	/* Configure PWM device initially */
	__pwm_numaker_configure(dev);

#ifdef CONFIG_PWM_CAPTURE    
    /* Enable NVIC */
	cfg->irq_config_func(dev);
#endif
    
move_exit:
	SYS_LockReg();
	return err;
}

#ifdef CONFIG_PWM_CAPTURE
#define NUMAKER_PWM_IRQ_CONFIG_FUNC(n)					\
	static void pwm_numaker_irq_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, pair0, irq),	\
			    DT_INST_IRQ_BY_NAME(n, pair0, priority),	\
			    pwm_numaker_p0_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_NAME(n, pair0, irq));	\
									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, pair1, irq),		\
			    DT_INST_IRQ_BY_NAME(n, pair1, priority),	\
			    pwm_numaker_p1_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_NAME(n, pair1, irq));		\
									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, pair2, irq),		\
			    DT_INST_IRQ_BY_NAME(n, pair2, priority),	\
			    pwm_numaker_p2_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_NAME(n, pair2, irq));		\
	}
#define IRQ_FUNC_INIT(n)					\
	.irq_config_func = pwm_numaker_irq_config_##n
#else
#define NUMAKER_PWM_IRQ_CONFIG_FUNC(n)
#define IRQ_FUNC_INIT(n)
#endif

#define NUMAKER_PWM_INIT(inst)                                                    \
	PINCTRL_DT_INST_DEFINE(inst);					       \
    NUMAKER_PWM_IRQ_CONFIG_FUNC(inst)              \
                                                            \
	static const struct pwm_numaker_config pwm_numaker_cfg_##inst = {            \
		.epwm = (EPWM_T *)DT_INST_REG_ADDR(inst),	                               \
        .prescale =  DT_INST_PROP(inst, prescaler),                        \
        .id_rst = DT_INST_PROP(inst, reset),					\
        .clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),    \
        .clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),          \
        .clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),    \
	    .clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))), \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	         \
        IRQ_FUNC_INIT(inst)                                      \
	};                                     \
									       \
	static struct pwm_numaker_data pwm_numaker_data_##inst;                      \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			    &pwm_numaker_init, NULL,			       \
			    &pwm_numaker_data_##inst, &pwm_numaker_cfg_##inst,       \
			    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			    &pwm_numaker_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMAKER_PWM_INIT)
