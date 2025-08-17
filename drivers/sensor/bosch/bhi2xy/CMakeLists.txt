#
# Copyright (c) 2025, Bojan Sofronievski
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_named(bhi2xy)

zephyr_library_sources(bhi2xy.c bhi2xy_parsers.c bhi2xy_unit_conversions.c
                       bhi2xy_errors.c)
zephyr_library_sources_ifdef(CONFIG_BHI2XY_BUS_SPI bhi2xy_spi.c)
zephyr_library_sources_ifdef(CONFIG_BHI2XY_BUS_I2C bhi2xy_i2c.c)

if(NOT CONFIG_BHI2XY_USE_CUSTOM_FIRMWARE)
  # Check if preprovided firmware is used
  set(BHI2XY_FW_DIR "${ZEPHYR_BOSCH_APIS_MODULE_DIR}/zephyr/blobs")
  set(BHI2XY_FW_FILE "BHI260AP.fw")
  set(BHI2XY_FW_PATH "${BHI2XY_FW_DIR}/${BHI2XY_FW_FILE}")
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
  generate_inc_file_for_target(bhi2xy ${BHI2XY_FW_PATH}
                               "${gen_dir}/bhi2xy_firmware.inc")
endif()
