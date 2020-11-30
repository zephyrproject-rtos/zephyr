# SPDX-License-Identifier: Apache-2.0

project(lib_os_cbprintf)
set(SOURCES main.c)
set(EXTRA_CPPFLAGS "-DCONFIG_CBPRINTF_LIBC_SUBSTS=1")
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
