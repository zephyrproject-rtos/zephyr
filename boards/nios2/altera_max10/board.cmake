set(FLASH_SCRIPT nios2.sh)
set(DEBUG_SCRIPT nios2.sh)
set(NIOS2_CPU_SOF $ENV{ZEPHYR_BASE}/arch/nios2/soc/nios2f-zephyr/cpu/ghrd_10m50da.sof)
set_property(GLOBAL APPEND PROPERTY FLASH_SCRIPT_ENV_VARS NIOS2_CPU_SOF)
