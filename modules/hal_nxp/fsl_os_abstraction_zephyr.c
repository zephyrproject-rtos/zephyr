/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsl_os_abstraction.h"

#ifdef CONFIG_NXP_FSL_OSA

#ifdef CONFIG_NXP_FSL_OSA_HEAP
/* calculate the required heap size of mcux usb host controller */
#ifdef CONFIG_USB_UHC_NXP_MCUX
#include "usb_host_config.h"
#include "usb_host_mcux_drv_port.h"
#ifdef CONFIG_USB_UHC_NXP_EHCI
#include "usb_host_ehci.h"
#define UHC_NXP_EHCI_ALLOC_SIZE (sizeof(usb_host_ehci_instance_t) * \
		DT_NUM_INST_STATUS_OKAY(nxp_uhc_ehci))
#else
#define UHC_NXP_EHCI_ALLOC_SIZE (0)
#endif

#ifdef CONFIG_USB_UHC_NXP_KHCI
#include "usb_host_khci.h"
#define UHC_NXP_KHCI_ALLOC_SIZE ((sizeof(usb_khci_host_state_struct_t) + \
	USB_HOST_CONFIG_KHCI_DMA_ALIGN_BUFFER + 4 + sizeof(usb_host_pipe_t) * \
	USB_HOST_CONFIG_MAX_PIPES) * DT_NUM_INST_STATUS_OKAY(nxp_uhc_khci))
#else
#define UHC_NXP_KHCI_ALLOC_SIZE (0)
#endif

#define UHC_NXP_MCUX_RERUIRED_SIZE (UHC_NXP_EHCI_ALLOC_SIZE + UHC_NXP_KHCI_ALLOC_SIZE)
#else
#define UHC_NXP_MCUX_RERUIRED_SIZE (0)
#endif

/* define the heap size based on the enabled modules */
#ifdef CONFIG_NXP_FSL_OSA_HEAP_CACHEABLE
K_HEAP_DEFINE_NOCACHE(fsl_osa_alloc_pool, UHC_NXP_MCUX_RERUIRED_SIZE);
#else
K_HEAP_DEFINE(fsl_osa_alloc_pool, UHC_NXP_MCUX_RERUIRED_SIZE);
#endif

void *OSA_MemoryAllocate(uint32_t memLength)
{
	void *p = (void *)k_heap_alloc(&fsl_osa_alloc_pool, memLength, K_NO_WAIT);

	if (p != NULL) {
		(void)memset(p, 0, memLength);
	}

	return p;
}

void OSA_MemoryFree(void *p)
{
	k_heap_free(&fsl_osa_alloc_pool, p);
}

#endif /* CONFIG_NXP_FSL_OSA_HEAP */

#endif /* CONFIG_NXP_FSL_OSA */
