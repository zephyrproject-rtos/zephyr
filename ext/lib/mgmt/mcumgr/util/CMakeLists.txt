target_include_directories(MCUMGR INTERFACE
    include
)

zephyr_library_sources(
    util/src/mcumgr_util.c
)

