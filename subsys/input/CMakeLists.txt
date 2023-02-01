# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(input.c)

zephyr_linker_sources(SECTIONS input.ld)

zephyr_library_sources_ifdef(CONFIG_INPUT_ABS_TO_REL       input_abs_to_rel.c)
