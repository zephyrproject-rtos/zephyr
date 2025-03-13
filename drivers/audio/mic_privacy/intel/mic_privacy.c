/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include "mic_privacy_registers.h"
#include <zephyr/drivers/mic_privacy/intel/mic_privacy.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#define LOG_DOMAIN mic_priv_zephyr

LOG_MODULE_REGISTER(LOG_DOMAIN);

#define DT_DRV_COMPAT intel_adsp_mic_privacy

#define DMICPVC_ADDRESS      (DT_INST_REG_ADDR(0))
#define DFMICPVCP_ADDRESS    (DMICPVC_ADDRESS)
#define DFMICPVCS_ADDRESS    (DMICPVC_ADDRESS + 0x0004)
#define DFFWMICPVCCS_ADDRESS (DMICPVC_ADDRESS + 0x0006)

#define DMICVSSX_ADDRESS     (0x16000)
#define DMICXLVSCTL_ADDRESS  (DMICVSSX_ADDRESS + 0x0004)
#define DMICXPVCCS_ADDRESS   (DMICVSSX_ADDRESS + 0x0010)

static inline void ace_mic_priv_intc_unmask(void)
{
	ACE_DINT[0].ie[ACE_INTL_MIC_PRIV] = BIT(0);
}

static inline void ace_dmic_intc_unmask(void)
{
	ACE_DINT[0].ie[ACE_INTL_DMIC] = BIT(0);
}

static void mic_privacy_enable_fw_managed_irq(bool enable_irq, const void *fn)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DFFWMICPVCCS_ADDRESS);
	if (enable_irq) {
		pv_ccs.part.mdstschgie = 1;
	} else {
		pv_ccs.part.mdstschgie = 0;
	}
	sys_write16(pv_ccs.full, DFFWMICPVCCS_ADDRESS);

	if (enable_irq && !irq_is_enabled(DT_INST_IRQN(0))) {

		irq_connect_dynamic(DT_INST_IRQN(0), 0,
			fn,
			DEVICE_DT_INST_GET(0), 0);

		irq_enable(DT_INST_IRQN(0));
		ace_mic_priv_intc_unmask();
	}
}

static void mic_privacy_clear_fw_managed_irq(void)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DFFWMICPVCCS_ADDRESS);
	pv_ccs.part.mdstschg = 1;
	sys_write16(pv_ccs.full, DFFWMICPVCCS_ADDRESS);
}

static void mic_privacy_enable_dmic_irq(bool enable_irq, const void *fn)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DMICXPVCCS_ADDRESS);
	if (enable_irq) {
		pv_ccs.part.mdstschgie = 1;
	} else {
		pv_ccs.part.mdstschgie = 0;
	}
	sys_write16(pv_ccs.full, DMICXPVCCS_ADDRESS);

	if (enable_irq) {
		irq_connect_dynamic(ACE_INTL_DMIC, 0,
			fn,
			NULL, 0);
		irq_enable(ACE_INTL_DMIC);
		ace_dmic_intc_unmask();
	} else {
		irq_disable(ACE_INTL_DMIC);
	}
}

static bool mic_privacy_get_dmic_irq_status(void)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DMICXPVCCS_ADDRESS);
	return pv_ccs.part.mdstschg;
}

static void mic_privacy_clear_dmic_irq_status(void)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DMICXPVCCS_ADDRESS);
	pv_ccs.part.mdstschg = 1;
	sys_write16(pv_ccs.full, DMICXPVCCS_ADDRESS);
}

static enum mic_privacy_policy mic_privacy_get_policy(void)
{
	union DFMICPVCP micpvcp;

	micpvcp.full = sys_read32(DFMICPVCP_ADDRESS);

	if (micpvcp.part.ddze == 2 && micpvcp.part.ddzpl == 1) {
		return MIC_PRIVACY_HW_MANAGED;
	} else if (micpvcp.part.ddze == 2 && micpvcp.part.ddzpl == 0) {
		return MIC_PRIVACY_FW_MANAGED;
	} else if (micpvcp.part.ddze == 3) {
		return MIC_PRIVACY_FORCE_MIC_DISABLED;
	} else {
		return MIC_PRIVACY_DISABLED;
	}
}

static uint32_t mic_privacy_get_privacy_policy_register_raw_value(void)
{
	return sys_read32(DFMICPVCP_ADDRESS);
}

static uint32_t mic_privacy_get_dma_data_zeroing_wait_time(void)
{
	union DFMICPVCP micpvcp;

	micpvcp.full = sys_read32(DFMICPVCP_ADDRESS);
	return micpvcp.part.ddzwt;
}

static uint32_t mic_privacy_get_dma_data_zeroing_link_select(void)
{
	union DFMICPVCP micpvcp;

	micpvcp.full = sys_read32(DFMICPVCP_ADDRESS);
	return micpvcp.part.ddzls;
}

static uint32_t mic_privacy_get_dmic_mic_disable_status(void)
{
	union DMICXPVCCS pvccs;

	pvccs.full = sys_read32(DMICXPVCCS_ADDRESS);
	return pvccs.part.mdsts;
}

static uint32_t mic_privacy_get_fw_managed_mic_disable_status(void)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DFFWMICPVCCS_ADDRESS);
	return pv_ccs.part.mdsts;
}

static void mic_privacy_set_fw_managed_mode(bool is_fw_managed_enabled)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DFFWMICPVCCS_ADDRESS);
	if (is_fw_managed_enabled) {
		pv_ccs.part.fmmd = 1;
	} else {
		pv_ccs.part.fmmd = 0;
	}
	sys_write16(pv_ccs.full, DFFWMICPVCCS_ADDRESS);
}

static void mic_privacy_set_fw_mic_disable_status(bool fw_mic_disable_status)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DFFWMICPVCCS_ADDRESS);
	if (fw_mic_disable_status) {
		pv_ccs.part.fmdsts = 1;
	} else {
		pv_ccs.part.fmdsts = 0;
	}
	sys_write16(pv_ccs.full, DFFWMICPVCCS_ADDRESS);
}

static uint32_t mic_privacy_get_fw_mic_disable_status(void)
{
	union DFFWMICPVCCS pv_ccs;

	pv_ccs.full = sys_read16(DFFWMICPVCCS_ADDRESS);
	return pv_ccs.part.fmdsts;
}

static int intel_adsp_mic_priv_init(const struct device *dev)
{
	return 0;
};

static const struct mic_privacy_api_funcs mic_privacy_ops = {
	.enable_fw_managed_irq = mic_privacy_enable_fw_managed_irq,
	.clear_fw_managed_irq = mic_privacy_clear_fw_managed_irq,
	.enable_dmic_irq = mic_privacy_enable_dmic_irq,
	.get_dmic_irq_status = mic_privacy_get_dmic_irq_status,
	.clear_dmic_irq_status = mic_privacy_clear_dmic_irq_status,
	.get_policy = mic_privacy_get_policy,
	.get_privacy_policy_register_raw_value = mic_privacy_get_privacy_policy_register_raw_value,
	.get_dma_data_zeroing_wait_time = mic_privacy_get_dma_data_zeroing_wait_time,
	.get_dma_data_zeroing_link_select = mic_privacy_get_dma_data_zeroing_link_select,
	.get_dmic_mic_disable_status = mic_privacy_get_dmic_mic_disable_status,
	.get_fw_managed_mic_disable_status = mic_privacy_get_fw_managed_mic_disable_status,
	.set_fw_managed_mode = mic_privacy_set_fw_managed_mode,
	.set_fw_mic_disable_status = mic_privacy_set_fw_mic_disable_status,
	.get_fw_mic_disable_status = mic_privacy_get_fw_mic_disable_status
};

#define INTEL_ADSP_MIC_PRIVACY_INIT(inst)							\
												\
	static const struct intel_adsp_mic_priv_cfg intel_adsp_mic_priv##inst##_config = {	\
		.base = DT_INST_REG_ADDR(inst),							\
		.regblock_size  = DT_INST_REG_SIZE(inst),					\
	};											\
												\
	static struct intel_adsp_mic_priv_data intel_adsp_mic_priv##inst##_data = {};		\
												\
	DEVICE_DT_INST_DEFINE(inst, &intel_adsp_mic_priv_init,					\
			      NULL,								\
			      &intel_adsp_mic_priv##inst##_data,				\
			      &intel_adsp_mic_priv##inst##_config, POST_KERNEL,			\
			      0,								\
			      &mic_privacy_ops);						\
												\

DT_INST_FOREACH_STATUS_OKAY(INTEL_ADSP_MIC_PRIVACY_INIT)
