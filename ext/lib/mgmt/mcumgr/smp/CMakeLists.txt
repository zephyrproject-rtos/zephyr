target_include_directories(MCUMGR INTERFACE 
    include
)

zephyr_library_sources(
    smp/src/smp.c
)
