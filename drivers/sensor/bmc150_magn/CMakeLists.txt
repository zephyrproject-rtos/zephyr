
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BMC150_MAGN}         bmc150_magn.c
            IFDEF:${CONFIG_BMC150_MAGN_TRIGGER} bmc150_magn_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
