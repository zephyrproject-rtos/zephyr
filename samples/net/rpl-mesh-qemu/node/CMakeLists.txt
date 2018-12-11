cmake_minimum_required(VERSION 3.13.1)
set(BOARD qemu_cortex_m3)
set(CONF_FILE prj_qemu.conf)

set(QEMU_PIPE_STACK 1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(rpl_mesh_qemu_node)

target_sources(app PRIVATE src/main.c)