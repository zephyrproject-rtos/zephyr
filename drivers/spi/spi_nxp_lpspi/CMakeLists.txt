# Copyright 2024 NXP

zephyr_library_sources_ifdef(CONFIG_SPI_NXP_LPSPI spi_nxp_lpspi_common.c)
if(CONFIG_SPI_RTIO)
	zephyr_library_sources_ifdef(CONFIG_SPI_NXP_LPSPI_CPU spi_nxp_lpspi_rtio.c)
else()
	zephyr_library_sources_ifdef(CONFIG_SPI_NXP_LPSPI_CPU spi_nxp_lpspi.c)
endif()

zephyr_library_sources_ifdef(CONFIG_SPI_NXP_LPSPI_DMA spi_nxp_lpspi_dma.c)
