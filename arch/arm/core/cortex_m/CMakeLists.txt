
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            vector_table.S
            reset.S
            nmi_on_reset.S
            prep_c.c
            scb.c
            nmi.c
            exc_manage.c
)

target_sources(arch_arm PRIVATE ${PRIVATE_SOURCES})
