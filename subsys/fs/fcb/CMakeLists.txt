zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            fcb_append.c
            fcb.c
            fcb_elem_info.c
            fcb_getnext.c
            fcb_rotate.c
            fcb_walk.c
)

target_sources(subsys_fs PRIVATE ${PRIVATE_SOURCES})
