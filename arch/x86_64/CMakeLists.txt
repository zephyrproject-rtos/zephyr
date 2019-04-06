# SPDX-License-Identifier: Apache-2.0

set(X86_64_BASE_CFLAGS
    -ffreestanding
    -fno-pic
    -fno-asynchronous-unwind-tables
    -mno-sse
    -mno-red-zone)

add_subdirectory(core)

zephyr_compile_options(${X86_64_BASE_CFLAGS} -mx32)

zephyr_link_libraries(-mx32)
