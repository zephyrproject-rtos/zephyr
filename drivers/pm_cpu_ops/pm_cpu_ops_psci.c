/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_psci_0_2

#define LOG_LEVEL CONFIG_PM_CPU_OPS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psci);

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/drivers/pm_cpu_ops.h>
#include "pm_cpu_ops_psci.h"

#ifdef CONFIG_POWEROFF
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/poweroff.h>
#endif /* CONFIG_POWEROFF */

/* PSCI data object. */
static struct psci_data_t psci_data;

static int psci_to_dev_err(int ret)
{
	switch (ret) {
	case PSCI_RET_SUCCESS:
		return 0;
	case PSCI_RET_NOT_SUPPORTED:
		return -ENOTSUP;
	case PSCI_RET_INVALID_PARAMS:
	case PSCI_RET_INVALID_ADDRESS:
		return -EINVAL;
	case PSCI_RET_DENIED:
		return -EPERM;
	}

	return -EINVAL;
}

int pm_cpu_off(void)
{
	int ret;

	if (psci_data.conduit == SMCCC_CONDUIT_NONE) {
		return -EINVAL;
	}

	ret = psci_data.invoke_psci_fn(PSCI_0_2_FN_CPU_OFF, 0, 0, 0);

	return psci_to_dev_err(ret);
}

int pm_cpu_on(unsigned long cpuid,
	      uintptr_t entry_point)
{
	int ret;

	if (psci_data.conduit == SMCCC_CONDUIT_NONE) {
		return -EINVAL;
	}

	ret = psci_data.invoke_psci_fn(PSCI_FN_NATIVE(0_2, CPU_ON), cpuid,
				       (unsigned long) entry_point, 0);

	return psci_to_dev_err(ret);
}

#ifdef CONFIG_POWEROFF
void z_sys_poweroff(void)
{
	int ret;

	__ASSERT_NO_MSG(psci_data.conduit != SMCCC_CONDUIT_NONE);

	ret = psci_data.invoke_psci_fn(PSCI_0_2_FN_SYSTEM_OFF, 0, 0, 0);
	if (ret < 0) {
		printk("System power off failed (%d) - halting\n", ret);
	}

	for (;;) {
		/* wait for power off */
	}
}
#endif /* CONFIG_POWEROFF */

/**
 * This function checks whether the given ID is supported or not, using
 * PSCI_FEATURES command.PSCI_FEATURES is supported from version 1.0 onwards.
 */
static int psci_features_check(unsigned long function_id)
{
	/* PSCI_FEATURES function ID is supported from PSCI 1.0 onwards. */
	if (!(PSCI_VERSION_MAJOR(psci_data.ver) >= 1)) {
		LOG_ERR("Function ID %lu not supported", function_id);
		return -ENOTSUP;
	}

	return psci_data.invoke_psci_fn(PSCI_FN_NATIVE(1_0, PSCI_FEATURES), function_id, 0, 0);
}

int pm_system_reset(unsigned char reset_type)
{
	int ret;

	if (psci_data.conduit == SMCCC_CONDUIT_NONE) {
		return -EINVAL;
	}

	if ((reset_type == SYS_WARM_RESET) &&
	    (!psci_features_check(PSCI_FN_NATIVE(1_1, SYSTEM_RESET2)))) {
		ret = psci_data.invoke_psci_fn(PSCI_FN_NATIVE(1_1, SYSTEM_RESET2), 0, 0, 0);
	} else if (reset_type == SYS_COLD_RESET) {
		ret = psci_data.invoke_psci_fn(PSCI_FN_NATIVE(0_2, SYSTEM_RESET), 0, 0, 0);
	} else {
		LOG_ERR("Invalid system reset type issued");
		return -EINVAL;
	}

	return psci_to_dev_err(ret);

}

static unsigned long __invoke_psci_fn_hvc(unsigned long function_id,
					  unsigned long arg0,
					  unsigned long arg1,
					  unsigned long arg2)
{
	struct arm_smccc_res res;

	arm_smccc_hvc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return res.a0;
}

static unsigned long __invoke_psci_fn_smc(unsigned long function_id,
					  unsigned long arg0,
					  unsigned long arg1,
					  unsigned long arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return res.a0;
}

static uint32_t psci_get_version(void)
{
	return psci_data.invoke_psci_fn(PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0);
}

static int set_conduit_method(const struct device *dev)
{
	const struct psci_config_t *dev_config = (const struct psci_config_t *)dev->config;

	if (!strcmp("hvc", dev_config->method)) {
		psci_data.conduit = SMCCC_CONDUIT_HVC;
		psci_data.invoke_psci_fn = __invoke_psci_fn_hvc;
	} else if (!strcmp("smc", dev_config->method)) {
		psci_data.conduit = SMCCC_CONDUIT_SMC;
		psci_data.invoke_psci_fn = __invoke_psci_fn_smc;
	} else {
		LOG_ERR("Invalid conduit method");
		return -EINVAL;
	}

	return 0;
}

static int psci_detect(void)
{
	uint32_t ver = psci_get_version();

	LOG_DBG("Detected PSCIv%d.%d",
		PSCI_VERSION_MAJOR(ver),
		PSCI_VERSION_MINOR(ver));

	if (PSCI_VERSION_MAJOR(ver) == 0 && PSCI_VERSION_MINOR(ver) < 2) {
		LOG_ERR("PSCI unsupported version");
		return -ENOTSUP;
	}

	psci_data.ver = ver;

	return 0;
}

uint32_t psci_version(void)
{
	return psci_data.ver;
}

static int psci_init(const struct device *dev)
{
	psci_data.conduit = SMCCC_CONDUIT_NONE;

	if (set_conduit_method(dev)) {
		return -ENOTSUP;
	}

	return psci_detect();
}

/**
 * Each PSCI interface versions have different DT compatible strings like arm,psci-0.2,
 * arm,psci-1.1 and so on. However, the same driver can be used for all the versions with
 * the below mentioned DT method where we need to #undef the default version arm,psci-0.2
 * and #define the required version like arm,psci-1.0 or arm,psci-1.1.
 */
#define PSCI_DEFINE(inst, ver)						\
	static const struct psci_config_t psci_config_##inst##ver = {	\
		.method = DT_PROP(DT_DRV_INST(inst), method)		\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			&psci_init,					\
			NULL,						\
			&psci_data,					\
			&psci_config_##inst##ver,				\
			PRE_KERNEL_1,					\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			NULL);

#define PSCI_0_2_INIT(n) PSCI_DEFINE(n, PSCI_0_2)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT arm_psci_0_2
DT_INST_FOREACH_STATUS_OKAY(PSCI_0_2_INIT)

#define PSCI_1_1_INIT(n) PSCI_DEFINE(n, PSCI_1_1)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT arm_psci_1_1
DT_INST_FOREACH_STATUS_OKAY(PSCI_1_1_INIT)
