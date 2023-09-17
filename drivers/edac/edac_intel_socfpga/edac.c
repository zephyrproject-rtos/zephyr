/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/arch/cpu.h>
#include <zephyr/fatal.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <socfpga_system_manager.h>
#include "edac.h"

#define LOG_MODULE_NAME edac_intel_socfpga
#define LOG_LEVEL       CONFIG_EDAC_INTEL_SOCFPGA_LOG_LEVEL
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#define SYSMNGR_BOOT_SCRATCH_COLD3_OFFSET (0x20C)

static void edac_ecc_handler(const struct device *dev, bool dbe, bool sbe, void *user_data)
{

	if (sbe) {
		LOG_ERR("EDAC : Single bit error detected");
	}

	if (dbe) {
		LOG_ERR("EDAC : Double bit error detected");
#if CONFIG_IO96B_INTEL_SOCFPGA
		if ((dev == DEVICE_DT_GET(DT_NODELABEL(io96b0))) ||
		    (dev == DEVICE_DT_GET(DT_NODELABEL(io96b1)))) {
			/*
			 * Wait for DDR_ECC_DBE_STATUS to 1 to ensure SDM had done retrieved all DBE
			 * info, avoid race condition.
			 */
			LOG_DBG("EDAC: Wait for DDR_ECC_DBE_STATUS to 1 to ensure SDM had done "
				"retrieved all DBE info");
			while (!(sys_read32(SOCFPGA_SYSMGR_REG_BASE +
					    SYSMNGR_BOOT_SCRATCH_COLD3_OFFSET) &
				 DDR_ECC_DBE_STATUS)) {
				;
			}
		}
#endif
		/*
		 * The double bit error is fatal, requires a system reboot
		 */
		LOG_PANIC();
		LOG_ERR("Rebooting the system");
		pm_system_reset(SYS_COLD_RESET);
	}
}

static int edac_init(void)
{
	const struct device *ecc_dev;
	const struct edac_ecc_driver_api *api;

	/* Check boot DDR_ECC_DBE_STATUS is 1*/
	if (sys_read32(SOCFPGA_SYSMGR_REG_BASE + SYSMNGR_BOOT_SCRATCH_COLD3_OFFSET) &
	    DDR_ECC_DBE_STATUS) {
		LOG_ERR("EDAC: System rebooted from an Double Bit Error");
	}

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(hps_ecc), okay)
	ecc_dev = DEVICE_DT_GET(DT_NODELABEL(hps_ecc));
	if (!device_is_ready(ecc_dev)) {
		LOG_ERR("EDAC: System Manager ECC device is not ready");
		return -ENODEV;
	}
	api = ecc_dev->api;
	api->set_ecc_error_cb(ecc_dev, edac_ecc_handler, NULL);
	LOG_DBG("EDAC: Registered EDAC call back to HPS ECC driver");
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(io96b0), okay)
	ecc_dev = DEVICE_DT_GET(DT_NODELABEL(io96b0));
	if (!device_is_ready(ecc_dev)) {
		LOG_ERR("EDAC: IO96B0 ECC device is not ready");
		return -ENODEV;
	}
	api = ecc_dev->api;
	LOG_DBG("EDAC: Registered EDAC call back to IO96B0 ECC driver");
	api->set_ecc_error_cb(ecc_dev, edac_ecc_handler, NULL);
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(io96b1), okay)
	ecc_dev = DEVICE_DT_GET(DT_NODELABEL(io96b1));
	if (!device_is_ready(ecc_dev)) {
		LOG_ERR("EDAC: IO96B0 ECC device is not ready");
		return -ENODEV;
	}
	api = ecc_dev->api;
	LOG_DBG("EDAC: Registered EDAC call back to IO96B1 ECC driver");
	api->set_ecc_error_cb(ecc_dev, edac_ecc_handler, NULL);
#endif
	return 0;
}

void k_sys_fatal_error_handler(unsigned int reason,
				      const z_arch_esf_t *esf)
{

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(hps_ecc), okay)
	const struct device *ecc_dev;

	ecc_dev = DEVICE_DT_GET(DT_NODELABEL(hps_ecc));
	if (!device_is_ready(ecc_dev)) {
		LOG_ERR("EDAC: System Manager ECC device is not ready");
	} else {
		/* Read Double Bit Error status */
		process_serror_for_hps_dbe(ecc_dev);
	}
#endif
	ARG_UNUSED(esf);

	LOG_ERR("Halting system");
	LOG_PANIC();
	(void)arch_irq_lock();
	for (;;) {
		/* Spin endlessly */
	}
	CODE_UNREACHABLE;
}

SYS_INIT(edac_init, POST_KERNEL, CONFIG_EDAC_INTEL_SOC_FPGA_INIT_PRIORITY);
