# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_ABORT source/stdlib/abort.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_TIME source/time/time.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_MALLOC source/stdlib/malloc.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_STRNLEN source/string/strnlen.c)

# Prevent compiler from optimizing calloc into an infinite recursive call
zephyr_library_compile_options($<TARGET_PROPERTY:compiler,no_builtin_malloc>)
