/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_psci_0_2

#define LOG_LEVEL CONFIG_PSCI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(psci);

#include <kernel.h>
#include <arch/cpu.h>

#include <soc.h>
#include <device.h>
#include <init.h>

#include <drivers/psci.h>
#include "psci.h"

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
	};

	return -EINVAL;
}

static uint32_t psci_api_get_version(const struct device *dev)
{
	struct psci *data = dev->data;

	return data->invoke_psci_fn(PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0);
}

static int psci_api_cpu_off(const struct device *dev, uint32_t state)
{
	struct psci *data = dev->data;
	int ret;

	ret = data->invoke_psci_fn(PSCI_0_2_FN_CPU_OFF, state, 0, 0);

	return psci_to_dev_err(ret);
}

static int psci_api_cpu_on(const struct device *dev, unsigned long cpuid,
			   unsigned long entry_point)
{
	struct psci *data = dev->data;
	int ret;

	ret = data->invoke_psci_fn(PSCI_FN_NATIVE(0_2, CPU_ON), cpuid,
				   entry_point, 0);

	return psci_to_dev_err(ret);
}

static int psci_api_affinity_info(const struct device *dev,
				  unsigned long target_affinity,
				  unsigned long lowest_affinity_level)
{
	struct psci *data = dev->data;

	return data->invoke_psci_fn(PSCI_FN_NATIVE(0_2, AFFINITY_INFO),
				    target_affinity, lowest_affinity_level, 0);
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

static int set_conduit_method(struct psci *data)
{
	const char *method;

	method = DT_PROP(DT_INST(0, DT_DRV_COMPAT), method);

	if (!strcmp("hvc", method)) {
		data->conduit = SMCCC_CONDUIT_HVC;
		data->invoke_psci_fn = __invoke_psci_fn_hvc;
	} else if (!strcmp("smc", method)) {
		data->conduit = SMCCC_CONDUIT_SMC;
		data->invoke_psci_fn = __invoke_psci_fn_smc;
	} else {
		LOG_ERR("Invalid conduit method");
		return -EINVAL;
	}

	return 0;
}

static int psci_detect(const struct device *dev)
{
	uint32_t ver = psci_api_get_version(dev);

	LOG_DBG("Detected PSCIv%d.%d",
		PSCI_VERSION_MAJOR(ver),
		PSCI_VERSION_MINOR(ver));

	if (PSCI_VERSION_MAJOR(ver) == 0 && PSCI_VERSION_MINOR(ver) < 2) {
		LOG_ERR("PSCI unsupported version");
		return -ENOTSUP;
	}

	return 0;
}

static int psci_init(const struct device *dev)
{
	struct psci *data = dev->data;

	if (set_conduit_method(data)) {
		return -ENOTSUP;
	}

	return psci_detect(dev);
}

static const struct psci_driver_api psci_api = {
	.get_version = psci_api_get_version,
	.cpu_off = psci_api_cpu_off,
	.cpu_on = psci_api_cpu_on,
	.affinity_info = psci_api_affinity_info,
};

static struct psci psci_data;

DEVICE_DT_INST_DEFINE(0, psci_init, device_pm_control_nop,
	&psci_data, NULL, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	&psci_api);
