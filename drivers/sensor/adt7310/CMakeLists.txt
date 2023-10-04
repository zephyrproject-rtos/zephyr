# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ADT7310 adt7310.c)
zephyr_library_sources_ifdef(CONFIG_ADT7310_TRIGGER adt7310_trigger.c)
