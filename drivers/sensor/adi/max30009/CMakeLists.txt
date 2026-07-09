# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(max30009.c)
zephyr_library_sources_ifdef(CONFIG_MAX30009_TRIGGER max30009_trigger.c)
zephyr_library_sources_ifdef(CONFIG_MAX30009_STREAM max30009_stream.c max30009_rtio.c)
