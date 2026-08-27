/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_fvp_pwrc

#define LOG_LEVEL CONFIG_PM_CPU_OPS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fvp_pm_cpu_ops);

#include <zephyr/kernel.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

/* FVP Platform Constants */
#define FVP_PWRC_BASE			DT_INST_REG_ADDR(0)
#define FVP_V2M_SYSREGS_BASE		DT_REG_ADDR(DT_INST_PHANDLE(0, arm_vexpress_sysreg))

/* FVP Power Controller Register Offsets */
#define PWRC_PPONR_OFF		0x4	/* Power-on Request */
#define PWRC_PSYSR_OFF		0x10	/* System Status Register */

/* PSYSR Register Bits */
#define PSYSR_AFF_L0		BIT(29)	/* Affinity Level 0 */

/* V2M System Registers */
#define V2M_SYS_CFGCTRL_OFF		0xa4	/* System Configuration Control Register */

/* V2M Configuration Control Register bits */
#define V2M_CFGCTRL_START		BIT(31)	/* Start operation */
#define V2M_CFGCTRL_RW			BIT(30)	/* Read/Write operation */
#define V2M_CFGCTRL_FUNC_MASK		GENMASK(27, 20)	/* Function field */
#define V2M_CFGCTRL_FUNC(fn)		FIELD_PREP(V2M_CFGCTRL_FUNC_MASK, (fn))

/* V2M System Configuration Functions */
#define V2M_FUNC_REBOOT			0x09	/* System reboot */

/*
 * Memory mapping strategy:
 *
 * To conserve memory (especially page tables) in Zephyr, we use temporary
 * mappings for hardware register access. Each operation maps the required
 * registers, performs the operation, then unmaps them immediately.
 *
 * CPU power operations are infrequent and not performance-critical. Memory
 * conservation is therefore more important than runtime optimizations.
 */
#define FVP_REGISTER_MAP_SIZE		0x1000	/* 4KB pages for register mapping */

static inline void fvp_pwrc_write_pponr(uintptr_t pwrc_vaddr, unsigned long mpidr)
{
	sys_write32((uint32_t)mpidr, pwrc_vaddr + PWRC_PPONR_OFF);
	LOG_DBG("FVP: PPONR write: MPIDR=0x%lx", mpidr);
}

static inline uint32_t fvp_pwrc_read_psysr(uintptr_t pwrc_vaddr, unsigned long mpidr)
{
	/* Write MPIDR to PSYSR to select which CPU to query */
	sys_write32((uint32_t)mpidr, pwrc_vaddr + PWRC_PSYSR_OFF);

	/* Read the status for the selected CPU */
	return sys_read32(pwrc_vaddr + PWRC_PSYSR_OFF);
}

static int fvp_cpu_power_on(unsigned long target_mpidr, uintptr_t entry_point)
{
	uint8_t *pwrc_vaddr_ptr;
	uintptr_t pwrc_vaddr;
	uint32_t psysr;
	int timeout = 10000; /* 1 second timeout */

	LOG_DBG("FVP: Powering on CPU MPIDR=0x%lx, entry=0x%lx",
		target_mpidr, entry_point);

	/* Map power controller registers once for the entire operation */
	k_mem_map_phys_bare(&pwrc_vaddr_ptr, FVP_PWRC_BASE, FVP_REGISTER_MAP_SIZE,
			    K_MEM_PERM_RW | K_MEM_CACHE_NONE);
	pwrc_vaddr = (uintptr_t)pwrc_vaddr_ptr;

	/*
	 * Wait for any pending power-off to complete.
	 * The target CPU must be in OFF state before we can power it on.
	 */
	do {
		psysr = fvp_pwrc_read_psysr(pwrc_vaddr, target_mpidr);
		if (timeout-- <= 0) {
			LOG_ERR("FVP: Timeout waiting for CPU 0x%lx power-off "
				"to complete, PSYSR=0x%x", target_mpidr, psysr);
			k_mem_unmap_phys_bare(pwrc_vaddr_ptr, FVP_REGISTER_MAP_SIZE);
			return -ETIMEDOUT;
		}
		k_busy_wait(100);
	} while ((psysr & PSYSR_AFF_L0) != 0);

	LOG_DBG("FVP: CPU 0x%lx is powered off (PSYSR=0x%x)", target_mpidr, psysr);

	/* Power on the target CPU via FVP power controller */
	fvp_pwrc_write_pponr(pwrc_vaddr, target_mpidr);

	/* Unmap power controller registers */
	k_mem_unmap_phys_bare(pwrc_vaddr_ptr, FVP_REGISTER_MAP_SIZE);

	/* Ensure power-on request completes */
	barrier_dsync_fence_full();

	/* Send event to wake up the target CPU from WFE loop */
	__asm__ volatile("sev" ::: "memory");

	LOG_DBG("FVP: Power-on request completed for CPU 0x%lx", target_mpidr);
	return 0;
}

int pm_cpu_on(unsigned long mpidr, uintptr_t entry_point)
{
	return fvp_cpu_power_on(mpidr, entry_point);
}

int pm_cpu_off(void)
{
	/*
	 * This is incompatible with our register mapping strategy.
	 * A CPU that shuts itself down might not complete the register
	 * unmapping before losing power.
	 */
	return -ENOTSUP;
}

static inline void fvp_v2m_sys_cfgwrite(uint32_t function)
{
	uint8_t *v2m_vaddr_ptr;
	uintptr_t v2m_vaddr;
	uint32_t val = V2M_CFGCTRL_START | V2M_CFGCTRL_RW | V2M_CFGCTRL_FUNC(function);

	/* Temporarily map V2M system registers */
	k_mem_map_phys_bare(&v2m_vaddr_ptr, FVP_V2M_SYSREGS_BASE, FVP_REGISTER_MAP_SIZE,
			    K_MEM_PERM_RW | K_MEM_CACHE_NONE);
	v2m_vaddr = (uintptr_t)v2m_vaddr_ptr;

	sys_write32(val, v2m_vaddr + V2M_SYS_CFGCTRL_OFF);

	LOG_DBG("FVP: V2M SYS_CFGCTRL write: 0x%x (func=0x%x)", val, function);

	/* Unmap V2M registers */
	k_mem_unmap_phys_bare(v2m_vaddr_ptr, FVP_REGISTER_MAP_SIZE);
}

int pm_system_reset(unsigned char reset_type)
{
	LOG_DBG("FVP: System reset requested (type=%u)", reset_type);

	/*
	 * FVP supports system reset via the V2M System Configuration Controller.
	 * Both warm and cold reset use the same mechanism - the V2M reboot function.
	 */
	fvp_v2m_sys_cfgwrite(V2M_FUNC_REBOOT);

	/*
	 * The reset should happen immediately, but in case it doesn't work,
	 * we'll wait a bit and return an error.
	 */
	k_busy_wait(1000000); /* Wait 1 second */

	LOG_ERR("FVP: System reset failed - system did not reset");
	return -ETIMEDOUT;
}
