zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            cpuhalt.c
            fatal.c
            posix_core.c
            swap.c
            thread.c
)

target_sources(arch_posix PRIVATE ${PRIVATE_SOURCES})
