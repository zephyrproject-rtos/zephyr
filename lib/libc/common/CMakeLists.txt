# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_ABORT source/stdlib/abort.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_ASCTIME source/time/asctime.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_GMTIME_R source/time/gmtime_r.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_LOCALTIME_R_UTC source/time/localtime_r_utc.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_CTIME source/time/ctime.c)
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
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_REMOVE source/stdio/remove.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_FEOF source/stdio/feof.c)

# Prevent compiler from optimizing calloc into an infinite recursive call
zephyr_library_compile_options($<TARGET_PROPERTY:compiler,no_builtin_malloc>)
