# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    ocpp.c
    core.c
    key_mgmt.c
    ocpp_j.c
    ocpp_wamp_rpc.c
)
