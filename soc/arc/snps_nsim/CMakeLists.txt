zephyr_cc_option(-mcpu=${GCC_M_CPU})
zephyr_cc_option(-mno-sdata -mdiv-rem -mswap -mnorm)
zephyr_cc_option(-mmpy-option=6 -mbarrel-shifter)
zephyr_cc_option(--param l1-cache-size=16384)
zephyr_cc_option(--param l1-cache-line-size=32)
zephyr_cc_option_ifdef(CONFIG_CODE_DENSITY -mcode-density)
zephyr_cc_option_ifdef(CONFIG_FLOAT -mfpu=fpuda_all)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            soc.c
            soc_config.c
)

target_sources(soc_arc PRIVATE ${PRIVATE_SOURCES})
target_include_directories(soc_arc PRIVATE ${ZEPHYR_BASE}/drivers)
