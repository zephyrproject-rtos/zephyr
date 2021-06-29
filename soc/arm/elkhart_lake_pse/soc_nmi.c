/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*Note: This will be a temp file for all HW/PSS work around and
 * need to remove later.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <sedi.h>
#include <dashboard_reg.h>
#include <pse_hw_defs.h>
#include <soc_nmi.h>

#define LOG_LEVEL CONFIG_SOC_TEMP_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc_nmi);

#define read32(addr)            (*(volatile uint32_t *)(addr))
#define write32(addr, val)      (*(volatile uint32_t *)(addr) = (uint32_t)(val))

extern void z_NmiHandlerSet(void (*pHandler)(void));

static void soc_nmi_handler(void)
{
	LOG_DBG("%s\n", __func__);

	fw_info_t *const sb = (fw_info_t *)SNOWBALL;
	uint32_t nmi_stt = read32(MISC_NMI_STATUS);
	uint32_t err_no = read32(DASHBOARD_VRF_ERR_STS) & ERRNO_MASK;

	err_no |= (nmi_stt << NMI_STT_SHIFT) + ERRNO_NMI;
	sb->fw_status |= STS_NMI;

}

static void fabric_isr(void)
{

	LOG_DBG("Fabric interrupt %s", __func__);
	irq_disable(SOC_HBW_PER_FABRIC_IRQ);
}

int soc_nmi_init(void)
{
	LOG_DBG("%s\n", __func__);

	z_NmiHandlerSet(soc_nmi_handler);

	IRQ_CONNECT(SOC_HBW_PER_FABRIC_IRQ,
		    SOC_FABRIC_IRQ_PRI, fabric_isr,
		    NULL, 0);

	irq_enable(SOC_HBW_PER_FABRIC_IRQ);


	return 0;
}
