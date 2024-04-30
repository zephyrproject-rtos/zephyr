# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    icm42688.c
    icm42688_common.c
    icm42688_spi.c
)

zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API icm42688_rtio.c)
zephyr_library_sources_ifdef(CONFIG_ICM42688_DECODER icm42688_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ICM42688_STREAM icm42688_rtio_stream.c)
zephyr_library_sources_ifdef(CONFIG_ICM42688_TRIGGER icm42688_trigger.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_ICM42688 icm42688_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_ICM42688 .)
