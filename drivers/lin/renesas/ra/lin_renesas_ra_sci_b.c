/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_lin_sci_b

#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/lin.h>
#include <zephyr/drivers/lin/transceiver.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <r_sci_b_lin.h>

#include "lin_renesas_ra_priv.h"

LOG_MODULE_REGISTER(lin_renesas_ra_sci_b, CONFIG_LIN_LOG_LEVEL);

extern void sci_b_lin_rxi_isr(void);
extern void sci_b_lin_tei_isr(void);
extern void sci_b_lin_txi_isr(void);
extern void sci_b_lin_eri_isr(void);
extern void sci_b_lin_aed_isr(void);
extern void sci_b_lin_bfd_isr(void);

__maybe_unused static void lin_renesas_ra_sci_b_rxi(void *arg)
{
	ARG_UNUSED(arg);
	sci_b_lin_rxi_isr();
}

__maybe_unused static void lin_renesas_ra_sci_b_tei(void *arg)
{
	ARG_UNUSED(arg);
	sci_b_lin_tei_isr();
}

__maybe_unused static void lin_renesas_ra_sci_b_txi(void *arg)
{
	ARG_UNUSED(arg);
	sci_b_lin_txi_isr();
}

__maybe_unused static void lin_renesas_ra_sci_b_eri(void *arg)
{
	ARG_UNUSED(arg);
	sci_b_lin_eri_isr();
}

__maybe_unused static void lin_renesas_ra_sci_b_aed(void *arg)
{
	ARG_UNUSED(arg);

#ifdef CONFIG_LIN_AUTO_SYNCHRONIZATION
	sci_b_lin_aed_isr();
#endif
}

__maybe_unused static void lin_renesas_ra_sci_b_bfd(void *arg)
{
	ARG_UNUSED(arg);
	sci_b_lin_bfd_isr();
}

struct lin_renesas_ra_sci_b_cfg {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_cfg;
	void (*irq_configure)(void);
};

struct lin_renesas_ra_sci_b_data {
	sci_b_lin_instance_ctrl_t fsp_lin_sci_b_ctrl;
	lin_cfg_t fsp_lin_cfg;
	sci_b_lin_extended_cfg_t fsp_lin_sci_b_extended_cfg;
};

static int lin_renesas_ra_sci_b_configure(const struct device *dev, const struct lin_config *cfg)
{
	struct lin_renesas_ra_data *data = dev->data;
	struct lin_renesas_ra_sci_b_data *priv = lin_renesas_ra_get_priv_data(dev);
	sci_b_lin_baud_params_t baud_params;
	sci_b_lin_baud_setting_t tmp_baud_setting;
	fsp_err_t fsp_err;

	if (data->common.started) {
		LOG_DBG("LIN device is running, cannot reconfigure");
		return -EFAULT;
	}

#ifndef CONFIG_LIN_AUTO_SYNCHRONIZATION
	if (cfg->flags & LIN_BUS_AUTO_SYNC) {
		LOG_DBG("Auto synchronization not enabled");
		return -EINVAL;
	}
#endif /* CONFIG_LIN_AUTO_SYNCHRONIZATION */

	baud_params.baudrate = cfg->baudrate;
	baud_params.clock_source = priv->fsp_lin_sci_b_extended_cfg.sci_b_settings_b.clock_source;
	baud_params.break_bits = cfg->break_len;

	fsp_err = R_SCI_B_LIN_BaudCalculate(&baud_params, &tmp_baud_setting);
	if (fsp_err != FSP_SUCCESS) {
		return -EINVAL;
	}

	priv->fsp_lin_cfg.mode = cfg->mode == LIN_MODE_COMMANDER ? LIN_MODE_MASTER : LIN_MODE_SLAVE;
	priv->fsp_lin_sci_b_extended_cfg.break_bits = baud_params.break_bits;
	priv->fsp_lin_sci_b_extended_cfg.sci_b_settings_b.auto_synchronization =
		cfg->flags & LIN_BUS_AUTO_SYNC ? 1 : 0;
	priv->fsp_lin_sci_b_extended_cfg.sci_b_settings_b.bus_conflict_detection =
		cfg->flags & LIN_BUS_CONFLICT_DETECTION ? 1 : 0;
	priv->fsp_lin_sci_b_extended_cfg.sci_b_settings_b.break_delimiter =
		cfg->break_delimiter_len == 2 ? 1 : 0;

	memcpy(&priv->fsp_lin_sci_b_extended_cfg.baud_setting, &tmp_baud_setting,
	       sizeof(sci_b_lin_baud_setting_t));
	memcpy(&data->common.config, cfg, sizeof(struct lin_config));

	return 0;
}

static int lin_renesas_ra_sci_b_set_rx_filter(const struct device *dev,
					      const struct lin_filter *filter)
{
	struct lin_renesas_ra_data *data = dev->data;
	lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
	sci_b_lin_id_filter_setting_t tmp_filter;
	fsp_err_t fsp_err;
	int ret = 0;
	int key;

	if (filter == NULL) {
		return -EINVAL;
	}

	if (data->common.config.mode == LIN_MODE_COMMANDER) {
		return -EPERM;
	}

	tmp_filter.priority_compare_data = filter->primary_pid;
	tmp_filter.secondary_compare_data = filter->secondary_pid;
	tmp_filter.compare_data_mask = filter->mask;
	tmp_filter.compare_data_select = SCI_B_LIN_COMPARE_DATA_SELECT_BOTH;
	tmp_filter.priority_interrupt_bit_select = 0x00;
	tmp_filter.priority_interrupt_enable = SCI_B_LIN_PRIORITY_INTERRUPT_BIT_DISABLE;

	key = irq_lock();

	fsp_err = R_SCI_B_LIN_IdFilterSet(fsp_lin_instance->p_ctrl, &tmp_filter);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("Failed to set LIN RX filter: %d", fsp_err);
		ret = -EIO;
	}

	irq_unlock(key);

	return ret;
}

static DEVICE_API(lin, lin_renesas_ra_sci_b_driver_api) = {
	.start = lin_renesas_ra_start,
	.stop = lin_renesas_ra_stop,
	.configure = lin_renesas_ra_sci_b_configure,
	.get_config = lin_renesas_ra_get_config,
	.send = lin_renesas_ra_send,
	.receive = lin_renesas_ra_receive,
	.response = lin_renesas_ra_response,
	.read = lin_renesas_ra_read,
	.set_callback = lin_renesas_ra_set_callback,
	.set_rx_filter = lin_renesas_ra_sci_b_set_rx_filter,
};

static int lin_renesas_ra_sci_b_init(const struct device *dev)
{
	const struct lin_renesas_ra_sci_b_cfg *cfg = lin_renesas_ra_get_priv_config(dev);
	struct lin_renesas_ra_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(cfg->clock_dev, (clock_control_subsys_t *)&cfg->clock_cfg);
	if (ret < 0) {
		return ret;
	}

	ret = lin_renesas_ra_sci_b_configure(dev, &data->common.config);
	if (ret < 0) {
		return ret;
	}

	k_work_init_delayable(&data->timeout_work, lin_renesas_ra_timeout_work_handler);

	cfg->irq_configure();

	return 0;
}

#define EVENT_SCI(channel, event) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SCI, channel, _, event))

#define RENESAS_DT_GET_IRQN_BY_NAME(id, name)                                                      \
	COND_CODE_1(DT_IRQ_HAS_NAME(id, name),                                                     \
		    (DT_IRQ_BY_NAME(id, name, irq)), (FSP_INVALID_VECTOR))

#define RENESAS_DT_GET_IRQ_PRIORITY_BY_NAME(id, name)                                              \
	COND_CODE_1(DT_IRQ_HAS_NAME(id, name),                                                     \
		    (DT_IRQ_BY_NAME(id, name, priority)), (BSP_IRQ_DISABLED))

#define RENESAS_RA_IRQ_CONNECT(id, name, event, isr, arg, flags)                                   \
	IF_ENABLED(DT_IRQ_HAS_NAME(id, name), (                                                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(id, name, irq), DT_IRQ_BY_NAME(id, name, priority),     \
			    isr, arg, flags);                                                      \
		R_ICU->IELSR[DT_IRQ_BY_NAME(id, name, irq)] = event;                               \
	))

#define LIN_RENESAS_RA_IRQ_DT_DEFINE(inst)                                                         \
	static void lin_renesas_ra_sci_b_irq_configure_##inst(void)                                \
	{                                                                                          \
		RENESAS_RA_IRQ_CONNECT(DT_INST_PARENT(inst), rxi,                                  \
				       EVENT_SCI(DT_PROP(DT_INST_PARENT(inst), channel), RXI),     \
				       lin_renesas_ra_sci_b_rxi, DEVICE_DT_INST_GET(inst), 0);     \
                                                                                                   \
		RENESAS_RA_IRQ_CONNECT(DT_INST_PARENT(inst), txi,                                  \
				       EVENT_SCI(DT_PROP(DT_INST_PARENT(inst), channel), TXI),     \
				       lin_renesas_ra_sci_b_txi, DEVICE_DT_INST_GET(inst), 0);     \
                                                                                                   \
		RENESAS_RA_IRQ_CONNECT(DT_INST_PARENT(inst), tei,                                  \
				       EVENT_SCI(DT_PROP(DT_INST_PARENT(inst), channel), TEI),     \
				       lin_renesas_ra_sci_b_tei, DEVICE_DT_INST_GET(inst), 0);     \
                                                                                                   \
		RENESAS_RA_IRQ_CONNECT(DT_INST_PARENT(inst), eri,                                  \
				       EVENT_SCI(DT_PROP(DT_INST_PARENT(inst), channel), ERI),     \
				       lin_renesas_ra_sci_b_eri, DEVICE_DT_INST_GET(inst), 0);     \
                                                                                                   \
		RENESAS_RA_IRQ_CONNECT(DT_INST_PARENT(inst), aed,                                  \
				       EVENT_SCI(DT_PROP(DT_INST_PARENT(inst), channel), AED),     \
				       lin_renesas_ra_sci_b_aed, DEVICE_DT_INST_GET(inst), 0);     \
                                                                                                   \
		RENESAS_RA_IRQ_CONNECT(DT_INST_PARENT(inst), bfd,                                  \
				       EVENT_SCI(DT_PROP(DT_INST_PARENT(inst), channel), BFD),     \
				       lin_renesas_ra_sci_b_bfd, DEVICE_DT_INST_GET(inst), 0);     \
	}

/* clang-format off */
#define LIN_RENESAS_RA_SCI_B_INIT(inst)                                                            \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(inst));                                                   \
	LIN_RENESAS_RA_IRQ_DT_DEFINE(inst);                                                        \
												   \
	static const struct lin_renesas_ra_sci_b_cfg lin_renesas_ra_sci_b_config_##inst = {        \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(inst)),                           \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(inst))),                  \
		.clock_cfg = {                                                                     \
			.mstp = DT_CLOCKS_CELL(DT_INST_PARENT(inst), mstp),                        \
			.stop_bit = DT_CLOCKS_CELL(DT_INST_PARENT(inst), stop_bit),                \
		},                                                                                 \
		.irq_configure = lin_renesas_ra_sci_b_irq_configure_##inst,                        \
	};                                                                                         \
												   \
	static const struct lin_renesas_ra_cfg lin_renesas_ra_cfg_##inst = {                       \
		.common = LIN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 20000),                           \
		.priv = &lin_renesas_ra_sci_b_config_##inst,                                       \
	};                                                                                         \
												   \
	static struct lin_renesas_ra_sci_b_data lin_renesas_ra_sci_b_data_##inst = {               \
		.fsp_lin_cfg.channel = DT_PROP(DT_INST_PARENT(inst), channel),                     \
		.fsp_lin_cfg.p_callback = lin_renesas_ra_callback_adapter,                         \
		.fsp_lin_cfg.p_context = (void *)DEVICE_DT_INST_GET(inst),                         \
		.fsp_lin_cfg.p_extend =                                                            \
			&lin_renesas_ra_sci_b_data_##inst.fsp_lin_sci_b_extended_cfg,              \
		.fsp_lin_sci_b_extended_cfg.sci_b_settings_b.clock_source =                        \
			SCI_B_LIN_CLOCK_SOURCE_SCICLK,                                             \
		.fsp_lin_sci_b_extended_cfg.sci_b_settings_b.noise_cancel =                        \
			DT_INST_PROP(inst, noise_filter),                                          \
		.fsp_lin_sci_b_extended_cfg.sci_b_settings_b.bus_conflict_detection =              \
			DT_INST_PROP(inst, conflict_detection),                                    \
		.fsp_lin_sci_b_extended_cfg.sci_b_settings_b.bus_conflict_clock =                  \
			CONCAT(SCI_B_LIN_BUS_CONFLICT_DETECTION_BASE_CLOCK_DIV_,                   \
			       DT_INST_PROP(inst, bus_conflict_detection_clk_div)),                \
		.fsp_lin_sci_b_extended_cfg.sci_b_settings_b.auto_synchronization =                \
			DT_INST_PROP(inst, auto_sync),                                             \
		.fsp_lin_sci_b_extended_cfg.sci_b_settings_b.noise_cancel_clock =                  \
			DT_INST_ENUM_IDX(inst, noise_filter_clk) == 0 ?                            \
				SCI_B_LIN_NOISE_CANCELLATION_CLOCK_BASE_CLOCK_DIV_1 :              \
				CONCAT(SCI_B_LIN_NOISE_CANCELLATION_CLOCK_,                        \
				       BAUDRATE_GENERATOR_CLOCK_DIV_,                              \
				       DT_INST_PROP(inst, noise_filter_clk_div)),                  \
		.fsp_lin_sci_b_extended_cfg.sci_b_settings_b.base_clock_cycles_per_bit =           \
			SCI_B_LIN_BASE_CLOCK_AUTO_CYCLES_PER_BIT,				   \
		.fsp_lin_sci_b_extended_cfg.filter_setting.compare_data_mask = 0x00,               \
		.fsp_lin_sci_b_extended_cfg.bfd_irq =                                              \
			RENESAS_DT_GET_IRQN_BY_NAME(DT_INST_PARENT(inst), bfd),                    \
		.fsp_lin_sci_b_extended_cfg.bfd_ipl =						   \
			RENESAS_DT_GET_IRQ_PRIORITY_BY_NAME(DT_INST_PARENT(inst), bfd),            \
		.fsp_lin_sci_b_extended_cfg.aed_irq =                                              \
			RENESAS_DT_GET_IRQN_BY_NAME(DT_INST_PARENT(inst), aed),                    \
		.fsp_lin_sci_b_extended_cfg.aed_ipl =                                              \
			RENESAS_DT_GET_IRQ_PRIORITY_BY_NAME(DT_INST_PARENT(inst), aed),            \
		.fsp_lin_sci_b_extended_cfg.rxi_irq =                                              \
			RENESAS_DT_GET_IRQN_BY_NAME(DT_INST_PARENT(inst), rxi),                    \
		.fsp_lin_sci_b_extended_cfg.rxi_ipl =                                              \
			RENESAS_DT_GET_IRQ_PRIORITY_BY_NAME(DT_INST_PARENT(inst), rxi),            \
		.fsp_lin_sci_b_extended_cfg.txi_irq =                                              \
			RENESAS_DT_GET_IRQN_BY_NAME(DT_INST_PARENT(inst), txi),                    \
		.fsp_lin_sci_b_extended_cfg.txi_ipl =                                              \
			RENESAS_DT_GET_IRQ_PRIORITY_BY_NAME(DT_INST_PARENT(inst), txi),            \
		.fsp_lin_sci_b_extended_cfg.tei_irq =                                              \
			RENESAS_DT_GET_IRQN_BY_NAME(DT_INST_PARENT(inst), tei),                    \
		.fsp_lin_sci_b_extended_cfg.tei_ipl =                                              \
			RENESAS_DT_GET_IRQ_PRIORITY_BY_NAME(DT_INST_PARENT(inst), tei),            \
		.fsp_lin_sci_b_extended_cfg.eri_irq =                                              \
			RENESAS_DT_GET_IRQN_BY_NAME(DT_INST_PARENT(inst), eri),                    \
		.fsp_lin_sci_b_extended_cfg.eri_ipl =                                              \
			RENESAS_DT_GET_IRQ_PRIORITY_BY_NAME(DT_INST_PARENT(inst), eri),            \
	};                                                                                         \
												   \
	static struct lin_renesas_ra_data lin_renesas_ra_data_##inst = {                           \
		.common.config = {                                                                 \
			.mode = DT_INST_PROP(inst, commander) ? LIN_MODE_COMMANDER                 \
							      : LIN_MODE_RESPONDER,                \
			.baudrate = DT_INST_PROP_OR(inst, bitrate, CONFIG_LIN_DEFAULT_BITRATE),    \
			.break_len = DT_INST_PROP(inst, break_len),                                \
			.break_delimiter_len = DT_INST_PROP(inst, break_delimiter),                \
			.flags = (DT_INST_PROP(inst, auto_sync) ? LIN_BUS_AUTO_SYNC : 0 |          \
				  DT_INST_PROP(inst, conflict_detection)                           \
						  ? LIN_BUS_CONFLICT_DETECTION : 0),               \
		},                                                                                 \
		.common.started = false,                                                           \
		.fsp_lin_instance = {                                                              \
			.p_ctrl = &lin_renesas_ra_sci_b_data_##inst.fsp_lin_sci_b_ctrl,            \
			.p_cfg = &lin_renesas_ra_sci_b_data_##inst.fsp_lin_cfg,                    \
			.p_api = &g_lin_on_sci_b,                                                  \
		},                                                                                 \
		.transmission_sem =                                                                \
			Z_SEM_INITIALIZER(lin_renesas_ra_data_##inst.transmission_sem, 1, 1),      \
		.device_state = ATOMIC_INIT(LIN_RENESAS_RA_STATE_IDLE),                            \
		.priv = &lin_renesas_ra_sci_b_data_##inst,                                         \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(inst, lin_renesas_ra_sci_b_init, NULL, &lin_renesas_ra_data_##inst,  \
			      &lin_renesas_ra_cfg_##inst, POST_KERNEL, CONFIG_LIN_INIT_PRIORITY,   \
			      &lin_renesas_ra_sci_b_driver_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(LIN_RENESAS_RA_SCI_B_INIT)
