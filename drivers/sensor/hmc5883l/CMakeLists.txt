
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_HMC5883L}         hmc5883l.c
            IFDEF:${CONFIG_HMC5883L_TRIGGER} hmc5883l_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
