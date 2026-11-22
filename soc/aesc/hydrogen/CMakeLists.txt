zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_REBOOT reboot.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
