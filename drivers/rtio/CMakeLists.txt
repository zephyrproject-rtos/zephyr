# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(rtio.c)

zephyr_library_sources_ifdef(CONFIG_RTIO_CONTEXT rtio_context.c)
