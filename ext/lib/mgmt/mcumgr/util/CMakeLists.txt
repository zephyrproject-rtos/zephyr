target_include_directories(MCUMGR INTERFACE
    include
)

zephyr_library_sources(
    src/mcumgr_util.c
)

