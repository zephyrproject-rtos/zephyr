# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_JDI_AMBIQ jdi_ambiq.c)
