# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ad5940.c)
zephyr_library_sources(ad5940_rtio.c)
zephyr_library_sources_ifdef(CONFIG_AD5940_STREAM ad5940_stream.c)
zephyr_library_sources_ifdef(CONFIG_AD5940_STREAM ad5940_decoder.c)

zephyr_library_sources_ifdef(CONFIG_AD5940_TRIGGER ad5940_trigger.c)

zephyr_library_link_libraries_ifdef(CONFIG_AD5940_STREAM m)

zephyr_library_include_directories(
    ${ZEPHYR_BASE}/include
)
