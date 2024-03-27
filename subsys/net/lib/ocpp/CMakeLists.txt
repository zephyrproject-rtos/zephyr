# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_interface_library_named(ocpp)

zephyr_library()

zephyr_library_sources(
    ocpp.c
    core.c
    key_mgmt.c
    ocpp_j.c
    ocpp_wamp_rpc.c
)

zephyr_library_link_libraries(ocpp)
