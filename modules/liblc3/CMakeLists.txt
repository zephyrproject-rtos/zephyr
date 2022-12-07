if(CONFIG_LIBLC3)

zephyr_library_named(liblc3)
zephyr_library_compile_options(-O3 -std=c11 -ffast-math -Wno-array-bounds)

zephyr_include_directories(${ZEPHYR_LIBLC3_MODULE_DIR}/include)
zephyr_include_directories(${ZEPHYR_LIBLC3_MODULE_DIR}/src)

zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/attdet.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/bits.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/bwdet.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/energy.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/lc3.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/ltpf.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/mdct.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/plc.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/sns.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/spec.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/tables.c)
zephyr_library_sources(${ZEPHYR_LIBLC3_MODULE_DIR}/src/tns.c)

endif()
