cmake_minimum_required(VERSION 3.8.2)
set(BOARD qemu_cortex_m3)
set(CONF_FILE prj_qemu.conf)

set(QEMU_PIPE_STACK 1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main.c)