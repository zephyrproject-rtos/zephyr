# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

# This abort.c is only a shim for k_panic(), which in posix/arch.h ends
# up calling...  abort()! Break that cycle and use the real POSIX
# abort().
if(NOT CONFIG_ARCH_POSIX)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_ABORT source/stdlib/abort.c)
endif()

zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_TIME source/time/time.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_MALLOC source/stdlib/malloc.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_STRNLEN source/string/strnlen.c)

zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_THRD
    source/thrd/cnd.c
    source/thrd/mtx.c
    source/thrd/once.c
    source/thrd/thrd.c
    source/thrd/tss.c
    )

# Prevent compiler from optimizing calloc into an infinite recursive call
zephyr_library_compile_options($<TARGET_PROPERTY:compiler,no_builtin_malloc>)
