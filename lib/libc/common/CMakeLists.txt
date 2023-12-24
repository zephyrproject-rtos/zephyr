# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

set(GEN_DIR ${ZEPHYR_BINARY_DIR}/include/generated)
set(STRERROR_TABLE_H ${GEN_DIR}/libc/common/strerror_table.h)

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_ABORT source/stdlib/abort.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_TIME source/time/time.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_MALLOC source/stdlib/malloc.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_STRNLEN source/string/strnlen.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_STRERROR
    ${STRERROR_TABLE_H}
    source/string/strerror.c
    )
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_THRD
    source/thrd/cnd.c
    source/thrd/mtx.c
    source/thrd/once.c
    source/thrd/thrd.c
    source/thrd/tss.c
    )

# Prevent compiler from optimizing calloc into an infinite recursive call
zephyr_library_compile_options($<TARGET_PROPERTY:compiler,no_builtin_malloc>)

add_custom_command(
  OUTPUT ${STRERROR_TABLE_H}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/gen_strerror_table.py
  -i include/errno.h
  -o ${STRERROR_TABLE_H}
  DEPENDS include/errno.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
