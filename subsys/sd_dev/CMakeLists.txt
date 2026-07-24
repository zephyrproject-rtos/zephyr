#SPDX - License - Identifier : Apache - 2.0

zephyr_library()

#Core SD device framework
zephyr_library_sources_ifdef(CONFIG_SD_DEV sd_dev.c sd_dev_io.c sd_dev_pkt.c)

#SDIO protocol support
zephyr_library_sources_ifdef(CONFIG_SDIO_DEV sdio_dev.c)

#MMC protocol support(optional)
zephyr_library_sources_ifdef(CONFIG_SDMMC_DEV sd_dev_mmc.c)
