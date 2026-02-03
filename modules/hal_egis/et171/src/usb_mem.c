/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <assert.h>

#include <usb_mem.h>
#include <et171_usb/sdudc_if.h>
#include <et171_usb/sgdma_if.h>

#ifdef CONFIG_XIP
#define mem_attr __ramfunc
#else
#define mem_attr
#endif

#define USB_MEM_SIZE                                                                         \
	(sizeof(UDC_Device) + sizeof(DmaController) /* USB private data: 0x1640 + 0xE30 */   \
	 + TRB_POOL_SIZE + UDC_PRIV_BUFFER_SIZE     /* DMA memory: 0x3000 + 0x40 */          \
	 + (sizeof(UdcRequest) * 16)                /* Max 16 endpoint: 0x44 * 16 */         \
	 + 256                                      /* add some buffer for heap structure */ \
	)

Z_HEAP_DEFINE_IN_SECT(usb_heap, USB_MEM_SIZE, __attribute__((section(".usb_mem"))));

mem_attr void *USB_mem_alloc(size_t xWantedSize)
{
#ifdef CONFIG_DCACHE
	return k_heap_aligned_alloc(&usb_heap, CONFIG_DCACHE_LINE_SIZE, xWantedSize, K_NO_WAIT);
#else
	return k_heap_alloc(&usb_heap, xWantedSize, K_NO_WAIT);
#endif
}

mem_attr void USB_mem_free(void *pv)
{
	k_heap_free(&usb_heap, pv);
}
