# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

zephyr_interface_library_named(ocpp)

zephyr_library()

zephyr_library_sources(
src/ocpp.c
src/core.c
src/key_mgmt.c
src/ocpp_j.c
src/ocpp_wamp_rpc.c
)

zephyr_library_link_libraries(ocpp)

