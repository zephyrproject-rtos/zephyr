/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
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

static struct psci psci_data;

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

int pm_system_off(void)
{
	int ret;

	if (psci_data.conduit == SMCCC_CONDUIT_NONE) {
		return -EINVAL;
	}

	/* A compliant PSCI implementation will never return from this call */
	ret = psci_data.invoke_psci_fn(PSCI_0_2_FN_SYSTEM_OFF, 0, 0, 0);

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

static int set_conduit_method(void)
{
	const char *method;

	method = DT_PROP(DT_INST(0, DT_DRV_COMPAT), method);

	if (!strcmp("hvc", method)) {
		psci_data.conduit = SMCCC_CONDUIT_HVC;
		psci_data.invoke_psci_fn = __invoke_psci_fn_hvc;
	} else if (!strcmp("smc", method)) {
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

	if (set_conduit_method()) {
		return -ENOTSUP;
	}

	return psci_detect();
}

DEVICE_DT_INST_DEFINE(0, psci_init, NULL,
	&psci_data, NULL, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	NULL);
