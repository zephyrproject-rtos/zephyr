/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/mp/ipc/mp_ipc.h>

#if DT_HAS_CHOSEN(zephyr_ipc_shm)

/* Real 2-Core Board: Define physical and virtual properties of shared memory */
#define SHM_PHYS_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_shm))
#define SHM_SIZE      DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_shm))

#if defined(CONFIG_SHM_VIRT_BASE)
#define SHM_VIRT_BASE CONFIG_SHM_VIRT_BASE
#else
#define SHM_VIRT_BASE SHM_PHYS_BASE
#endif

#if defined(CONFIG_MMU)
int arch_page_phys_get(void *virt, uintptr_t *phys);
#endif

__weak uintptr_t mp_ipc_virt_to_phys(void *virt_addr)
{
#if defined(CONFIG_MMU)
	uintptr_t phys_addr = 0;
	int ret;

	/* Translate Virtual Address to true Physical Address */
	ret = arch_page_phys_get(virt_addr, &phys_addr);
	if (ret == 0) {
		return phys_addr;
	}
#endif
	return (uintptr_t)virt_addr;
}

__weak void *mp_ipc_phys_to_virt(uintptr_t phys_addr)
{
	/* Translate Physical Address back to Receiver's Virtual Address space */
	return (void *)(uintptr_t)(phys_addr - SHM_PHYS_BASE + SHM_VIRT_BASE);
}

#else

/* Loopback / Symmetric build: No address translation is needed */
__weak uintptr_t mp_ipc_virt_to_phys(void *virt_addr)
{
	return (uintptr_t)virt_addr;
}

__weak void *mp_ipc_phys_to_virt(uintptr_t phys_addr)
{
	return (void *)phys_addr;
}

#endif
