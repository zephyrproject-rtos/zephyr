/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_wwdt

#include <zephyr/kernel.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <NuMicro.h>

LOG_MODULE_REGISTER(wwdt_numaker, CONFIG_WDT_LOG_LEVEL);

#define NUMAKER_PRESCALER_MAX 15U
#define NUMAKER_COUNTER_MAX   0x3eU
#define NUMAKER_COUNTER_MIN   0x01U

/* Device config */
struct wwdt_numaker_config {
	/* wdt base address */
	WWDT_T *wwdt_base;
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clk_dev;
};

struct wwdt_numaker_data {
	wdt_callback_t cb;
	bool timeout_valid;
	/* watchdog timeout in milliseconds */
	uint32_t timeout;
	uint32_t prescaler;
	uint32_t counter;
};

static int m_wwdt_numaker_clk_get_rate(const struct wwdt_numaker_config *cfg, uint32_t *rate)
{

	if (cfg->clk_src == CLK_CLKSEL1_WWDTSEL_LIRC) {
		*rate = __LIRC / (cfg->clk_div + 1);
	} else {
		/* clock source is from HCLK, CLK_CLKSEL1_WWDTSEL_HCLK_DIV2048 */
		SystemCoreClockUpdate();
		*rate = CLK_GetHCLKFreq() / 2048 / (cfg->clk_div + 1);
	}

	return 0;
}


/* Convert watchdog clock to nearest ms (rounded up) */
static uint32_t m_wwdt_numaker_calc_ms(const struct device *dev, uint32_t pow2)
{
	const struct wwdt_numaker_config *cfg = dev->config;
	uint32_t clk_freq;
	uint32_t prescale_clks;
	uint32_t period_ms;

	m_wwdt_numaker_clk_get_rate(cfg, &clk_freq);
	prescale_clks = (1 << pow2) * 64;
	period_ms = DIV_ROUND_UP(prescale_clks * MSEC_PER_SEC, clk_freq);

	return period_ms;
}

static int m_wwdt_numaker_calc_window(const struct device *dev,
				      const struct wdt_window *win,
				      uint32_t *timeout,
				      uint32_t *prescaler,
				      uint32_t *counter)
{
	uint32_t pow2;
	uint32_t gap;

	/* Find nearest period value (rounded up) */
	for (pow2 = 0U; pow2 <= NUMAKER_PRESCALER_MAX; pow2++) {
		*timeout = m_wwdt_numaker_calc_ms(dev, pow2);

		if (*timeout >= win->max) {
			*prescaler = pow2 << WWDT_CTL_PSCSEL_Pos;
			if (win->min == 0U) {
				*counter = NUMAKER_COUNTER_MAX;
			} else {
				gap = DIV_ROUND_UP(win->min
						   * NUMAKER_COUNTER_MAX,
						   *timeout);
				*counter = NUMAKER_COUNTER_MAX - gap;
				if (*counter < NUMAKER_COUNTER_MIN) {
					*counter = NUMAKER_COUNTER_MIN;
				}
			}

			return 0;
		}
	}

	return -EINVAL;
}

static int wwdt_numaker_install_timeout(const struct device *dev,
					const struct wdt_timeout_cfg *cfg)
{
	struct wwdt_numaker_data *data = dev->data;
	const struct wwdt_numaker_config *config = dev->config;
	uint32_t timeout;
	uint32_t prescaler;
	uint32_t counter;

	LOG_DBG("");
	/* Validate watchdog already running */
	if (config->wwdt_base->CTL & WWDT_CTL_WWDTEN_Msk) {
		LOG_ERR("watchdog is busy");
		return -EBUSY;
	}

	if (cfg->window.max == 0U) {
		LOG_ERR("window.max should be non-zero");
		return -EINVAL;
	}

	if (m_wwdt_numaker_calc_window(dev, &cfg->window, &timeout, &prescaler, &counter) != 0) {
		LOG_ERR("window.max is out of range");
		return -EINVAL;
	}

	LOG_DBG("counter=%d", counter);
	data->timeout = timeout;
	data->prescaler = prescaler;
	data->counter = counter;
	data->cb = cfg->callback;
	data->timeout_valid = true;

	return 0;
}

static int wwdt_numaker_disable(const struct device *dev)
{
	struct wwdt_numaker_data *data = dev->data;
	const struct wwdt_numaker_config *cfg = dev->config;
	WWDT_T *wwdt_base = cfg->wwdt_base;

	LOG_DBG("");
	/* stop counting */
	wwdt_base->CTL &= ~WWDT_CTL_WWDTEN_Msk;

	/* disable interrupt enable bit */
	wwdt_base->CTL &= ~WWDT_CTL_INTEN_Msk;

	/* disable interrupt */
	irq_disable(DT_INST_IRQN(0));

	data->timeout_valid = false;

	return 0;
}

static int wwdt_numaker_setup(const struct device *dev, uint8_t options)
{
	struct wwdt_numaker_data *data = dev->data;
	const struct wwdt_numaker_config *cfg = dev->config;
	WWDT_T *wwdt_base = cfg->wwdt_base;
	uint32_t dbg_mask = 0U;

	LOG_DBG("");
	irq_disable(DT_INST_IRQN(0));

	/* Validate watchdog already running */
	if (wwdt_base->CTL & WWDT_CTL_WWDTEN_Msk) {
		LOG_ERR("watchdog is busy");
		return -EBUSY;
	}

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeout installed");
		return -EINVAL;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_ERR("WDT_OPT_PAUSE_IN_SLEEP is not supported");
		return -ENOTSUP;
	}

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		dbg_mask = WWDT_CTL_ICEDEBUG_Msk;
	}

	/* Clear  WWDT Reset & Compared Match Interrupt System Flag */
	wwdt_base->STATUS = WWDT_STATUS_WWDTRF_Msk |
			    WWDT_STATUS_WWDTIF_Msk;

	/* Open WWDT and start counting */
	wwdt_base->CTL = data->prescaler |
			 (data->counter << WWDT_CTL_CMPDAT_Pos) |
			 WWDT_CTL_INTEN_Msk |
			 WWDT_CTL_WWDTEN_Msk |
			 dbg_mask;

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static int wwdt_numaker_feed(const struct device *dev, int channel_id)
{
	const struct wwdt_numaker_config *cfg = dev->config;
	WWDT_T *wwdt_base = cfg->wwdt_base;

	LOG_DBG("CNT=%d, CTL=0x%x", wwdt_base->CNT, wwdt_base->CTL);
	ARG_UNUSED(channel_id);

	/* Reload WWDT Counter */
	wwdt_base->RLDCNT = WWDT_RELOAD_WORD;

	return 0;
}

static void wwdt_numaker_isr(const struct device *dev)
{
	struct wwdt_numaker_data *data = dev->data;
	const struct wwdt_numaker_config *cfg = dev->config;
	WWDT_T *wwdt_base = cfg->wwdt_base;

	LOG_DBG("CNT=%d", wwdt_base->CNT);
	if (wwdt_base->STATUS & WWDT_STATUS_WWDTIF_Msk) {
		/* Clear WWDT Compared Match Interrupt Flag */
		wwdt_base->STATUS = WWDT_STATUS_WWDTIF_Msk;

		if (data->cb != NULL) {
			data->cb(dev, 0);
		}
	}
}

static DEVICE_API(wdt, wwdt_numaker_api) = {
	.setup = wwdt_numaker_setup,
	.disable = wwdt_numaker_disable,
	.install_timeout = wwdt_numaker_install_timeout,
	.feed = wwdt_numaker_feed,
};

static int wwdt_numaker_init(const struct device *dev)
{
	const struct wwdt_numaker_config *cfg = dev->config;
	struct numaker_scc_subsys scc_subsys;
	int err;

	SYS_UnlockReg();

	irq_disable(DT_INST_IRQN(0));
	/* CLK controller */
	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = cfg->clk_modidx;
	scc_subsys.pcc.clk_src = cfg->clk_src;
	scc_subsys.pcc.clk_div = cfg->clk_div;

	/* Equivalent to CLK_EnableModuleClock() */
	err = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&scc_subsys);
	if (err != 0) {
		goto done;
	}

	/* Equivalent to CLK_SetModuleClock() */
	err = clock_control_configure(cfg->clk_dev, (clock_control_subsys_t)&scc_subsys, NULL);
	if (err != 0) {
		goto done;
	}

	/* Enable NVIC */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    wwdt_numaker_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

done:
	SYS_LockReg();
	return err;

}

/* Set config based on DTS */
static struct wwdt_numaker_config wwdt_numaker_cfg_inst = {
	.wwdt_base = (WWDT_T *)DT_INST_REG_ADDR(0),
	.clk_modidx = DT_INST_CLOCKS_CELL(0, clock_module_index),
	.clk_src = DT_INST_CLOCKS_CELL(0, clock_source),
	.clk_div = DT_INST_CLOCKS_CELL(0, clock_divider),
	.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(0))),
};

static struct wwdt_numaker_data wwdt_numaker_data_inst;

DEVICE_DT_INST_DEFINE(0, wwdt_numaker_init, NULL,
		      &wwdt_numaker_data_inst, &wwdt_numaker_cfg_inst,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &wwdt_numaker_api);
