#Copyright(c) 2025 Felipe Neves
#SPDX - License - Identifier : Apache - 2.0

zephyr_include_directories(.)
zephyr_linker_sources_ifdef(CONFIG_MEMC SECTIONS sdram.ld)
