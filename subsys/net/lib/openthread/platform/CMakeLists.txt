zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            alarm.c
            flash.c
            logging.c
            misc.c
            platform.c
            radio.c
            random.c
            spi.c
            IFDEF:${CONFIG_OPENTHREAD_SHELL} shell.c
)

add_library(openthread_platform STATIC ${PRIVATE_SOURCES})
target_link_libraries(openthread_platform PRIVATE zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE openthread_platform)
# The source files here use header files from the OpenThread project
# target_link_libraries(app ot_lib) # Better?
add_dependencies(openthread_platform ot)
