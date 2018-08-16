
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_DMA_QMSI}          dma_qmsi.c
            IFDEF:${CONFIG_DMA_SAM_XDMAC}     dma_sam_xdmac.c
            IFDEF:${CONFIG_DMA_STM32F4X}      dma_stm32f4x.c
            IFDEF:${CONFIG_DMA_CAVS}          dma_cavs.c
            IFDEF:${CONFIG_DMA_NIOS2_MSGDMA}  dma_nios2_msgdma.c
            IFDEF:${CONFIG_USERSPACE}         dma_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
