# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    icm4268x.c
    icm4268x_common.c
    icm4268x_spi.c
    icm4268x_bus.c
)

zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API icm4268x_rtio.c)
zephyr_library_sources_ifdef(CONFIG_ICM4268X_DECODER icm4268x_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ICM4268X_STREAM icm4268x_rtio_stream.c)
zephyr_library_sources_ifdef(CONFIG_ICM4268X_TRIGGER icm4268x_trigger.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_ICM4268X icm4268x_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_ICM4268X .)
