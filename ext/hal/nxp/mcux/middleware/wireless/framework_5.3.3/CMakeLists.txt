zephyr_include_directories(Common)
zephyr_include_directories(OSAbstraction/Interface)
zephyr_include_directories(XCVR/${MCUX_DEVICE})

add_subdirectory(XCVR/${MCUX_DEVICE})

zephyr_sources(OSAbstraction/Source/fsl_os_abstraction_zephyr.c)
