# Copyright (c) 2023 Linumiz
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_JSONC)

set(JSONC_UPSTREAM ${ZEPHYR_JSON_C_MODULE_DIR})

zephyr_interface_library_named(JSONC)

zephyr_include_directories(include)
zephyr_include_directories(${JSONC_UPSTREAM})

zephyr_library()
zephyr_library_sources(
	${JSONC_UPSTREAM}/arraylist.c
	${JSONC_UPSTREAM}/debug.c
	${JSONC_UPSTREAM}/json_c_version.c
	${JSONC_UPSTREAM}/json_object.c
	${JSONC_UPSTREAM}/json_object_iterator.c
	${JSONC_UPSTREAM}/json_pointer.c
	${JSONC_UPSTREAM}/json_tokener.c
	${JSONC_UPSTREAM}/json_util.c
	${JSONC_UPSTREAM}/json_visit.c
	${JSONC_UPSTREAM}/libjson.c
	${JSONC_UPSTREAM}/linkhash.c
	${JSONC_UPSTREAM}/printbuf.c
	${JSONC_UPSTREAM}/random_seed.c
	${JSONC_UPSTREAM}/strerror_override.c
)

zephyr_library_link_libraries(JSONC)
target_link_libraries(JSONC INTERFACE zephyr_interface)

endif()
