# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(hts221.c)
zephyr_library_sources_ifdef(CONFIG_HTS221_TRIGGER hts221_trigger.c)

zephyr_library_include_directories(../stmemsc)
