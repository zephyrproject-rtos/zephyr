zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_SOC_MKW24D5} wdog.S soc_kw2xd.c
            IFDEF:${CONFIG_SOC_MKW22D5} wdog.S soc_kw2xd.c
            IFDEF:${CONFIG_SOC_MKW41Z4} soc_kw4xz.c
            IFDEF:${CONFIG_SOC_MKW40Z4} soc_kw4xz.c
)

target_sources(soc_arm PRIVATE ${PRIVATE_SOURCES})
