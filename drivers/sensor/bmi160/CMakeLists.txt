
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BMI160}         bmi160.c
            IFDEF:${CONFIG_BMI160_TRIGGER} bmi160_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
