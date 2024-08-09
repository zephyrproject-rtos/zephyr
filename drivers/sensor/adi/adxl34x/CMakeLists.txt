#
# Copyright (c) 2024 Chaim Zax
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(
  adxl34x.c
  adxl34x_configure.c
  adxl34x_attr.c
)
zephyr_library_sources_ifdef(CONFIG_ADXL34X_BUS_I2C adxl34x_i2c.c)
zephyr_library_sources_ifdef(CONFIG_ADXL34X_BUS_SPI adxl34x_spi.c)
zephyr_library_sources_ifdef(CONFIG_ADXL34X_TRIGGER adxl34x_trigger.c)
zephyr_library_sources_ifdef(CONFIG_ADXL34X_DECODER adxl34x_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ADXL34X_ASYNC_API adxl34x_rtio.c)

zephyr_library_sources_ifdef(CONFIG_EMUL_ADXL34X adxl34x_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_ADXL34X .)
