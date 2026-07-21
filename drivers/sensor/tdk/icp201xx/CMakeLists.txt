# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(icp201xx_drv.c)
zephyr_library_sources_ifdef(CONFIG_SPI icp201xx_spi.c)
zephyr_library_sources_ifdef(CONFIG_I2C icp201xx_i2c.c)

zephyr_library_sources_ifdef(CONFIG_ICP201XX_TRIGGER icp201xx_trigger.c)
zephyr_library_include_directories(.)
