zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_SPI_DW}         spi_dw.c
            IFDEF:${CONFIG_SPI_INTEL}      spi_intel.c
            IFDEF:${CONFIG_SPI_STM32}      spi_ll_stm32.c
            IFDEF:${CONFIG_SPI_MCUX_DSPI}  spi_mcux_dspi.c
            IFDEF:${CONFIG_SPI_MCUX_LPSPI} spi_mcux_lpspi.c
            IFDEF:${CONFIG_SPI_SAM}        spi_sam.c
            IFDEF:${CONFIG_SPI_SAM0}       spi_sam0.c
            IFDEF:${CONFIG_NRFX_SPI}       spi_nrfx_spi.c
            IFDEF:${CONFIG_NRFX_SPIM}      spi_nrfx_spim.c
            IFDEF:${CONFIG_NRFX_SPIS}      spi_nrfx_spis.c
            IFDEF:${CONFIG_USERSPACE}      spi_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
