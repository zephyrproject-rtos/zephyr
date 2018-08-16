
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_AIO_COMPARATOR_QMSI} aio_comparator_qmsi.c
            IFDEF:${CONFIG_USERSPACE}           aio_comparator_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
