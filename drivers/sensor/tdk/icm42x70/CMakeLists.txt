# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(icm42x70.c)

zephyr_library_sources_ifdef(CONFIG_SPI icm42x70_spi.c)
zephyr_library_sources_ifdef(CONFIG_I2C icm42x70_i2c.c)

zephyr_library_sources_ifdef(CONFIG_USE_EMD_ICM42670 icm42670.c)

zephyr_library_sources_ifdef(CONFIG_ICM42X70_TRIGGER icm42x70_trigger.c)
zephyr_library_sources_ifdef(CONFIG_TDK_APEX icm42x70_apex.c)
