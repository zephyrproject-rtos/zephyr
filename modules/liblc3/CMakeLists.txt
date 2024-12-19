if(CONFIG_LIBLC3)
zephyr_library_named(liblc3)
zephyr_library_compile_options(
	-O3 -std=c11 -ffast-math -Wno-array-bounds -Wall -Wextra -Wdouble-promotion -Wvla -pedantic
)

# LC3plus and LC3plusHR support is enabled by default in liblc3.
# In our case, we prefer those to be explicitly enabled by the user if needed.
if(CONFIG_LIBLC3_PLUS)
zephyr_library_compile_options(-DLC3_PLUS=1)
else()
zephyr_library_compile_options(-DLC3_PLUS=0)
endif()

if(CONFIG_LIBLC3_PLUS_HR)
zephyr_library_compile_options(-DLC3_PLUS_HR=1)
else()
zephyr_library_compile_options(-DLC3_PLUS_HR=0)
endif()

zephyr_include_directories(
	${ZEPHYR_LIBLC3_MODULE_DIR}/include
	${ZEPHYR_LIBLC3_MODULE_DIR}/src
)

zephyr_library_sources(
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/attdet.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/bits.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/bwdet.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/energy.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/lc3.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/ltpf.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/mdct.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/plc.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/sns.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/spec.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/tables.c
	${ZEPHYR_LIBLC3_MODULE_DIR}/src/tns.c
)
endif()
