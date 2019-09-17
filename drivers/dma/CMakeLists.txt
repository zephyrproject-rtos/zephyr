# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DMA_SAM_XDMAC	dma_sam_xdmac.c)
zephyr_library_sources_ifdef(CONFIG_DMA_STM32F4X	dma_stm32f4x.c)
zephyr_library_sources_ifdef(CONFIG_DMA_CAVS		dma_cavs.c)
zephyr_library_sources_ifdef(CONFIG_DMA_NIOS2_MSGDMA	dma_nios2_msgdma.c)
zephyr_library_sources_ifdef(CONFIG_DMA_SAM0		dma_sam0.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE		dma_handlers.c)
