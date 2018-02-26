target_include_directories(MCUMGR INTERFACE 
    include
)

zephyr_library_sources(
    cborattr/src/cborattr.c
)
